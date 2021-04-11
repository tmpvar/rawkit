#pragma once

#include <rawkit/rawkit.h>
#include <roaring/roaring.h>


struct Bitset {
  roaring_bitmap_t *_bitmap = nullptr;
  Bitset(u32 capacity = 0) {
    if (capacity) {
      _bitmap = roaring_bitmap_create_with_capacity(capacity);
    } else {
      _bitmap = roaring_bitmap_create();

    }
  }

  ~Bitset() {
    roaring_bitmap_free(_bitmap);
  }

  bool get(u32 idx) {
    return roaring_bitmap_contains(this->_bitmap, idx);
  }

  void set(u32 idx) {
    roaring_bitmap_add(this->_bitmap, idx);
  }
};