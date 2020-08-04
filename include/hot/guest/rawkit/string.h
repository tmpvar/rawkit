#pragma once


#define GB_STRING_IMPLEMENTATION
#include <gb_string.h>

struct String {
  gbString handle = nullptr;
  String() {

  }

  void append(char c) {
    if (this->handle == nullptr) {
      this->handle = gb_make_string("");
    }
    char p[2] = {c, 0};
    this->handle = gb_append_cstring(this->handle, p);
  }
};