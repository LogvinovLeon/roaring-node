{
  "targets": [
    {
      "target_name": "roaring",
      "cflags": ["-Wall"],
      "include_dirs": ["src", "<!(node -e \"require('nan')\")"],
      "sources": [
        "src/cpp/module.cpp",
        "src/cpp/TypedArrays.cpp",
        "src/cpp/RoaringBitmap32/RoaringBitmap32.cpp",
        "src/cpp/RoaringBitmap32Iterator/RoaringBitmap32Iterator.cpp",
        "src/cpp/roaring.cpp"
      ]
    }
  ]
}
