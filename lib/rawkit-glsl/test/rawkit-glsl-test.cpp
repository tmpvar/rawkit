#include <doctest.h>

#include <rawkit/glsl.h>

TEST_CASE("[rawkit/glsl] compile glsl into spirv") {
  // invalid args
  {
    REQUIRE(rawkit_glsl_compile(nullptr, nullptr) == nullptr);
    REQUIRE(rawkit_glsl_compile("BLAH", nullptr) == nullptr);
    REQUIRE(rawkit_glsl_compile(nullptr, "BLAH") == nullptr);
    REQUIRE(rawkit_glsl_compile("invalid.extension", "BLAH") == nullptr);
  }
}