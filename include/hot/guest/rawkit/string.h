#pragma once

#define GB_STRING_IMPLEMENTATION
#include <stdlib.h>
#include <gb_string.h>

typedef struct String {
  gbString handle = nullptr;

  String() {}
  String(const char *str) {
    this->handle = gb_make_string(str);
  }

  static String *copy(const String *str) {
    String *ret = (String *)malloc(sizeof(String));
    ret->handle = nullptr;
    ret->append_string(str);
    return ret;
  }

  String *append_char(char c) {
    if (this->handle == nullptr) {
      this->handle = gb_make_string("");
    }
    char p[2] = {c, 0};
    this->handle = gb_append_cstring(this->handle, p);
    return this;
  }

  String *append_string(const String *str) {
    if (this->handle == nullptr) {
      this->handle = gb_duplicate_string(str->handle);
    } else {
      this->handle = gb_append_string(this->handle, str->handle);
    }
    return this;
  }

  String *append_c_str(const char *str) {
    if (this->handle == nullptr) {
      this->handle = gb_make_string(str);
    } else {
      this->handle = gb_append_cstring(this->handle, str);
    }
    return this;
  }

  String *clear() {
    if (this->handle != nullptr) {
      gb_clear_string(this->handle);
    }
    return this;
  }

  void set_c_str(const char *str) {
    if (this->handle == nullptr) {
      this->handle = gb_make_string(str);
    } else {
      this->handle = gb_set_string(this->handle, str);
    }
  }

  size_t length() {
    if (this->handle == nullptr) {
      return 0;
    }
    return gb_string_length(this->handle);
  }

  const char *c_str() {
    return this->handle;
  }
} String;

typedef struct StringList {
  String **handle = nullptr;
  size_t capacity = 0;
  size_t count = 0;

  void resize(size_t new_capacity) {
    if (new_capacity == 0) {
      new_capacity = 2;
    }

    size_t new_size = sizeof(String *) * (new_capacity + 1);

    if (this->handle == nullptr) {
      this->handle = (String **)malloc(new_size);
    } else {
      this->handle = (String **)realloc(this->handle, new_size);
    }
    this->capacity = new_capacity;
  }

  size_t length() {
    return this->count;
  }

  size_t push(const String *str) {

    if (this->count + 1 >= this->capacity) {
      this->resize(this->capacity * 2);
    }

    String *entry = String::copy(str);
    this->handle[this->count++] = entry;
    return this->count;
  }

  String *item(const size_t line) {
    if (line >= count) {
      return nullptr;
    }
    return this->handle[line];
  }
} StringList;