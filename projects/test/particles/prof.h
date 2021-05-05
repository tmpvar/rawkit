#pragma once

struct Prof {
  const char *name;
  double start = 0.0;
  bool finished = false;
  Prof(const char *name)
  : name(name)
  {
    this->start = rawkit_now();
  }

  double diff_ms() {
    return (rawkit_now() - this->start) * 1000.0;
  }

  ~Prof() {
    this->finish();
  }

  void finish() {
    if (this->finished) {
      return;
    }
    this->finished = true;
    igText("%s took %f ms", this->name, diff_ms());
  }
};
