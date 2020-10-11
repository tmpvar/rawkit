#include <doctest.h>

#include <rawkit-jit-internal.h>

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;

static const char *fixturePath(string name) {
  static unordered_map<string, string> fixtures;
  auto it = fixtures.find(name);
  if (it == fixtures.end()) {
    fs::path p = fs::path(__FILE__).remove_filename() / "fixtures" / name;
    cout << "fixture: " << p.string() << endl;
    string s = p.string();
    fixtures.emplace(name, s);
    return fixturePath(name);
  }

  return it->second.c_str();
}

TEST_CASE("[rawkit/jit] construction") {
  {
    const char *args[] = { fixturePath("noop.c") };
    cout << "args[0] " << args[0] << endl;
    JitJob *job = JitJob::create(1, args);
    job->rebuild();
    REQUIRE(job != nullptr);
    REQUIRE(job->active_runnable);
  }

  {
    const char *args[] = { fixturePath("noop.cpp") };
    cout << "args[0] " << args[0] << endl;
    JitJob *job = JitJob::create(1, args);
    job->rebuild();
    REQUIRE(job != nullptr);
    REQUIRE(job->active_runnable);
  }
}