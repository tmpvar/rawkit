#pragma once

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <string.h>

#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;

static const char *fixturePath(string name) {
  static unordered_map<std::string, std::string> fixtures;
  auto it = fixtures.find(name);
  if (it == fixtures.end()) {
    fs::path p = fs::path(__FILE__).remove_filename() / "fixtures";
    if (name != "") {
      p = p / name;
    }
    std::string s = p.string();
    fixtures.emplace(name, s);
    return fixturePath(name);
  }

  return it->second.c_str();
}

static char *create_tmpfile() {
  char *input = tmpnam(NULL);
  size_t len = strlen(input);
  char *output = (char *)calloc(len+1, 1);
  memcpy(output, input, len);
  return output;
}

static int write_file(const char *filename, const char *contents) {
  FILE *f = fopen(filename, "w+");
  if (!f) {
    return 0;
  }

  size_t len = strlen(contents);
  size_t w = fwrite((void *)contents, 1, len, f);
  fclose(f);
  return w == len ? 1 : 0;
}