#include "../v8utils.h"

#include "../RoaringBitmap32Iterator/RoaringBitmap32Iterator.h"
#include "../TypedArrays.h"
#include "RoaringBitmap32.h"

#define MAX_SERIALIZATION_ARRAY_SIZE_IN_BYTES 0x00FFFFFF

Nan::Persistent<v8::FunctionTemplate> RoaringBitmap32::constructorTemplate;
Nan::Persistent<v8::Function> RoaringBitmap32::constructor;

static roaring_bitmap_t roaring_bitmap_zero;

void RoaringBitmap32::Init(v8::Local<v8::Object> exports) {
  memset(&roaring_bitmap_zero, 0, sizeof(roaring_bitmap_zero));

  auto className = Nan::New("RoaringBitmap32").ToLocalChecked();

  v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(RoaringBitmap32::New);
  RoaringBitmap32::constructorTemplate.Reset(ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(className);

  auto ctorInstanceTemplate = ctor->InstanceTemplate();

  Nan::SetAccessor(ctorInstanceTemplate, Nan::New("isEmpty").ToLocalChecked(), isEmpty_getter);
  Nan::SetAccessor(ctorInstanceTemplate, Nan::New("size").ToLocalChecked(), size_getter);

  Nan::SetNamedPropertyHandler(ctorInstanceTemplate, namedPropertyGetter);

  Nan::SetPrototypeMethod(ctor, "minimum", minimum);
  Nan::SetPrototypeMethod(ctor, "maximum", maximum);
  Nan::SetPrototypeMethod(ctor, "has", has);
  Nan::SetPrototypeMethod(ctor, "contains", has);
  Nan::SetPrototypeMethod(ctor, "copyFrom", copyFrom);
  Nan::SetPrototypeMethod(ctor, "add", add);
  Nan::SetPrototypeMethod(ctor, "tryAdd", tryAdd);
  Nan::SetPrototypeMethod(ctor, "addMany", addMany);
  Nan::SetPrototypeMethod(ctor, "remove", remove);
  Nan::SetPrototypeMethod(ctor, "removeMany", removeMany);
  Nan::SetPrototypeMethod(ctor, "delete", removeChecked);
  Nan::SetPrototypeMethod(ctor, "clear", clear);
  Nan::SetPrototypeMethod(ctor, "orInPlace", addMany);
  Nan::SetPrototypeMethod(ctor, "andNotInPlace", removeMany);
  Nan::SetPrototypeMethod(ctor, "andInPlace", andInPlace);
  Nan::SetPrototypeMethod(ctor, "xorInPlace", xorInPlace);
  Nan::SetPrototypeMethod(ctor, "isSubset", isSubset);
  Nan::SetPrototypeMethod(ctor, "isStrictSubset", isStrictSubset);
  Nan::SetPrototypeMethod(ctor, "isEqual", isEqual);
  Nan::SetPrototypeMethod(ctor, "intersects", intersects);
  Nan::SetPrototypeMethod(ctor, "andCardinality", andCardinality);
  Nan::SetPrototypeMethod(ctor, "orCardinality", orCardinality);
  Nan::SetPrototypeMethod(ctor, "andNotCardinality", andNotCardinality);
  Nan::SetPrototypeMethod(ctor, "xorCardinality", xorCardinality);
  Nan::SetPrototypeMethod(ctor, "jaccardIndex", jaccardIndex);
  Nan::SetPrototypeMethod(ctor, "flipRange", flipRange);
  Nan::SetPrototypeMethod(ctor, "removeRunCompression", removeRunCompression);
  Nan::SetPrototypeMethod(ctor, "runOptimize", runOptimize);
  Nan::SetPrototypeMethod(ctor, "shrinkToFit", shrinkToFit);
  Nan::SetPrototypeMethod(ctor, "rank", rank);
  Nan::SetPrototypeMethod(ctor, "toUint32Array", toUint32Array);
  Nan::SetPrototypeMethod(ctor, "getSerializationSizeInBytes", getSerializationSizeInBytes);
  Nan::SetPrototypeMethod(ctor, "serialize", serialize);
  Nan::SetPrototypeMethod(ctor, "deserialize", deserialize);

  auto ctorFunction = ctor->GetFunction();
  auto ctorObject = ctorFunction->ToObject();

  Nan::SetMethod(ctorObject, "deserialize", deserializeStatic);
  Nan::SetMethod(ctorObject, "swap", swapStatic);

  v8utils::defineHiddenField(ctorObject, "default", ctorObject);

  exports->Set(className, ctorFunction);
  constructor.Reset(ctorFunction);
}

RoaringBitmap32::RoaringBitmap32() : roaring(roaring_bitmap_zero) {
}

RoaringBitmap32::~RoaringBitmap32() {
  ra_clear(&roaring.high_low_container);
}

void RoaringBitmap32::New(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  if (!info.IsConstructCall()) {
    v8::Local<v8::Function> cons = Nan::New(constructor);
    if (info.Length() < 1) {
      v8::Local<v8::Value> argv[0] = {};
      info.GetReturnValue().Set(Nan::NewInstance(cons, 0, argv).ToLocalChecked());
    } else {
      v8::Local<v8::Value> argv[1] = {info[0]};
      info.GetReturnValue().Set(Nan::NewInstance(cons, 1, argv).ToLocalChecked());
    }
    return;
  }

  // create a new instance and wrap our javascript instance
  RoaringBitmap32 * instance = new RoaringBitmap32();

  if (!instance || !ra_init(&instance->roaring.high_low_container)) {
    if (instance != nullptr)
      delete instance;
    Nan::ThrowError(Nan::New("RoaringBitmap32::ctor - failed to initialize native roaring container").ToLocalChecked());
  }

  instance->Wrap(info.Holder());

  // return the wrapped javascript instance
  info.GetReturnValue().Set(info.Holder());

  if (info.Length() >= 1) {
    if (RoaringBitmap32::constructorTemplate.Get(info.GetIsolate())->HasInstance(info[0])) {
      instance->copyFrom(info);
    } else {
      instance->addMany(info);
    }
  }
}

NAN_PROPERTY_GETTER(RoaringBitmap32::namedPropertyGetter) {
  if (property->IsSymbol()) {
    if (Nan::Equals(property, v8::Symbol::GetIterator(info.GetIsolate())).FromJust()) {
      auto self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.This());
      auto iter_template = Nan::New<v8::FunctionTemplate>();
      Nan::SetCallHandler(iter_template,
          [](const Nan::FunctionCallbackInfo<v8::Value> & info) {
            v8::Local<v8::Value> argv[1] = {info.This()};
            v8::Local<v8::Function> cons = Nan::New(RoaringBitmap32Iterator::constructor);
            info.GetReturnValue().Set(Nan::NewInstance(cons, 1, argv).ToLocalChecked());
          },
          Nan::New<v8::External>(self));
      info.GetReturnValue().Set(iter_template->GetFunction());
    }
  }
}

NAN_PROPERTY_GETTER(RoaringBitmap32::size_getter) {
  const RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<const RoaringBitmap32>(info.Holder());
  auto size = roaring_bitmap_get_cardinality(&self->roaring);
  if (size <= 0xFFFFFFFF) {
    info.GetReturnValue().Set((uint32_t)size);
  } else {
    info.GetReturnValue().Set((double)size);
  }
}

NAN_PROPERTY_GETTER(RoaringBitmap32::isEmpty_getter) {
  const RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<const RoaringBitmap32>(info.Holder());
  info.GetReturnValue().Set(roaring_bitmap_is_empty(&self->roaring));
}

void RoaringBitmap32::has(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    info.GetReturnValue().Set(false);
  } else {
    const RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<const RoaringBitmap32>(info.Holder());
    info.GetReturnValue().Set(roaring_bitmap_contains(&self->roaring, info[0]->Uint32Value()));
  }
}

void RoaringBitmap32::minimum(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());
  return info.GetReturnValue().Set(roaring_bitmap_minimum(&self->roaring));
}

void RoaringBitmap32::maximum(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());
  return info.GetReturnValue().Set(roaring_bitmap_maximum(&self->roaring));
}

void RoaringBitmap32::rank(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  if (info.Length() <= 1 || !info[0]->IsUint32()) {
    return info.GetReturnValue().Set(0);
  }

  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());
  info.GetReturnValue().Set((double)roaring_bitmap_rank(&self->roaring, info[0]->Uint32Value()));
}

void RoaringBitmap32::removeRunCompression(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());
  info.GetReturnValue().Set(roaring_bitmap_remove_run_compression(&self->roaring));
}

void RoaringBitmap32::runOptimize(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());
  info.GetReturnValue().Set(roaring_bitmap_run_optimize(&self->roaring));
}

void RoaringBitmap32::shrinkToFit(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());
  info.GetReturnValue().Set((double)roaring_bitmap_shrink_to_fit(&self->roaring));
}

void RoaringBitmap32::toUint32Array(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  RoaringBitmap32 * self = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info.Holder());

  auto size = roaring_bitmap_get_cardinality(&self->roaring);

  if (size >= 0xFFFFFFFF) {
    Nan::ThrowError(Nan::New("RoaringBitmap32::toUint32Array - array too big").ToLocalChecked());
  }

  v8::Local<v8::Value> argv[1] = {Nan::New((uint32_t)size)};
  auto typedArray = Nan::NewInstance(TypedArrays::Uint32Array_ctor.Get(info.GetIsolate()), 1, argv).ToLocalChecked();

  if (size != 0) {
    Nan::TypedArrayContents<uint32_t> typedArrayContent(typedArray);
    if (!typedArrayContent.length() || !*typedArrayContent)
      Nan::ThrowError(Nan::New("RoaringBitmap32::toUint32Array - failed to allocate").ToLocalChecked());

    roaring_bitmap_to_uint32_array(&self->roaring, *typedArrayContent);
  }

  info.GetReturnValue().Set(typedArray);
}

void RoaringBitmap32::swapStatic(const Nan::FunctionCallbackInfo<v8::Value> & info) {
  if (info.Length() < 2) {
    return Nan::ThrowTypeError(Nan::New("RoaringBitmap32::swap expects 2 arguments").ToLocalChecked());
  }

  if (!RoaringBitmap32::constructorTemplate.Get(info.GetIsolate())->HasInstance(info[0])) {
    return Nan::ThrowTypeError(Nan::New("RoaringBitmap32::swap first argument must be a RoaringBitmap32").ToLocalChecked());
  }

  if (!RoaringBitmap32::constructorTemplate.Get(info.GetIsolate())->HasInstance(info[1])) {
    return Nan::ThrowTypeError(Nan::New("RoaringBitmap32::swap second argument must be a RoaringBitmap32").ToLocalChecked());
  }

  RoaringBitmap32 * a = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info[0]->ToObject());
  RoaringBitmap32 * b = Nan::ObjectWrap::Unwrap<RoaringBitmap32>(info[1]->ToObject());

  if (a != b) {
    std::swap(a->roaring, b->roaring);
  }
}
