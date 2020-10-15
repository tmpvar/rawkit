#include <doctest.h>

#include <rawkit/glsl.h>
#include "util.h"

#include <string>
using namespace std;

TEST_CASE("[rawkit/glsl] invalid args") {
  REQUIRE(rawkit_glsl_compile(nullptr, nullptr, nullptr) == nullptr);
  REQUIRE(rawkit_glsl_compile("BLAH", nullptr, nullptr) == nullptr);
  REQUIRE(rawkit_glsl_compile(nullptr, "BLAH", nullptr) == nullptr);
  REQUIRE(rawkit_glsl_compile("invalid.extension", "BLAH", nullptr) == nullptr);
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv") {
  rawkit_glsl_t *s = rawkit_glsl_compile("simple.frag",
    "#version 450\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = 1.23456 * 5.678;\n"
    "}\n",
    nullptr
  );
  REQUIRE(s != nullptr);
  CHECK(s->valid);
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv (absolute include)") {
  char src[4096] = {0};
  string include;
  include.assign(fixturePath("multiply.glsl"));
  std::replace( include.begin(), include.end(), '\\', '/');

  sprintf(
    src,
    "#version 450\n"
    "#include \"%s\"\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = multiply(1.23456, 5.678);\n"
    "}\n",
    include.c_str()
  );

  rawkit_glsl_t *s = rawkit_glsl_compile("simple.frag", src, nullptr);
  REQUIRE(s != nullptr);
  CHECK(s->valid);
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv (relative include)") {
  char src[4096] = {0};
  string include;
  include.assign(fixturePath("nested.glsl"));
  std::replace( include.begin(), include.end(), '\\', '/');

  sprintf(
    src,
    "#version 450\n"
    "#include \"%s\"\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = nested(1.23456);\n"
    "}\n",
    include.c_str()
  );

  rawkit_glsl_t *s = rawkit_glsl_compile("simple.frag", src, nullptr);
  REQUIRE(s != nullptr);
  CHECK(s->valid);
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv (system include)") {
  char *paths[] = { strdup(fixturePath("")) };
  rawkit_glsl_paths_t includes = {
    paths,
    1
  };

  rawkit_glsl_t *s = rawkit_glsl_compile(
    "simple.frag",
    "#version 450\n"
    "#include <system.glsl>\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = system();\n"
    "}\n",
    &includes
  );

  REQUIRE(s != nullptr);
  CHECK(s->valid);

  free(paths[0]);
  free(s);
}