#include "parser.h"

TEST_CASE("basic p tests") {
  GrblParser p;
  // empty lines are ignored
  REQUIRE(p.read('\n') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 0);

  // carriage returns are ignored
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 0);

  // ok
  REQUIRE(p.read('o') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 1);
  REQUIRE(p.read('k') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 2);
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 2);
  REQUIRE(p.read('\n') == GRBL_TRUE);
  REQUIRE(p.handle->loc == 0);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS);
  REQUIRE(p.handle->tokens[0].error_code == 0);
}

TEST_CASE("error:\\d+") {
  GrblParser p;

  REQUIRE(p.read('e') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read('o') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read(':') == GRBL_FALSE);
  REQUIRE(p.read('9') == GRBL_FALSE);
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.read('\n') == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS);
  REQUIRE(p.handle->tokens[0].error_code == 9);
}

TEST_CASE("welcome message") {
  GrblParser p;

  REQUIRE(p.read('G') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read('b') == GRBL_FALSE);
  REQUIRE(p.read('l') == GRBL_FALSE);
  REQUIRE(p.read(' ') == GRBL_FALSE);
  REQUIRE(p.read('1') == GRBL_FALSE);
  REQUIRE(p.read('.') == GRBL_FALSE);
  REQUIRE(p.read('2') == GRBL_FALSE);
  REQUIRE(p.read('g') == GRBL_FALSE);
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.read('\n') == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_WELCOME);
  REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_VERSION);
  REQUIRE(p.handle->tokens[1].version.major == 1);
  REQUIRE(p.handle->tokens[1].version.minor == 2);
  REQUIRE(p.handle->tokens[1].version.letter == 'g');
}