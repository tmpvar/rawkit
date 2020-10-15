#pragma once

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

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