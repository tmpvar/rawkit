#include "parser.h"

TEST_CASE("gcode parser") {

  SECTION("<letter><letter> fails") {
    GCODEParser p;
    REQUIRE(p.push('G') == GCODE_RESULT_TRUE);
    REQUIRE(p.push('G') == GCODE_RESULT_ERROR);
  }

  SECTION("G1 parsing") {
    GCODEParser p;
    REQUIRE(p.push('G') == GCODE_RESULT_TRUE);
    REQUIRE(p.push('1') == GCODE_RESULT_TRUE);
    REQUIRE(p.push('\n') == GCODE_RESULT_TRUE);
    REQUIRE(stb_sb_count(p.handle->lines) == 2);
    REQUIRE(p.line(0)->pairs != nullptr);
    
    REQUIRE(stb_sb_count(p.line(0)->pairs) == 1);
    REQUIRE(p.line(0)->type == GCODE_LINE_TYPE_G);
    REQUIRE(p.line(0)->pairs[0].letter == 'G');
    REQUIRE(p.line(0)->pairs[0].value == 1);
    
    REQUIRE(stb_sb_count(p.line(1)->pairs) == 0);
  }

  SECTION("M4 parsing") {
    GCODEParser p;
    REQUIRE(p.push('M') == GCODE_RESULT_TRUE);
    REQUIRE(p.push('4') == GCODE_RESULT_TRUE);
    REQUIRE(p.push('\n') == GCODE_RESULT_TRUE);
    REQUIRE(stb_sb_count(p.handle->lines) == 2);
    REQUIRE(p.line(0)->pairs != nullptr);
    
    REQUIRE(stb_sb_count(p.line(0)->pairs) == 1);
    REQUIRE(p.line(0)->type == GCODE_LINE_TYPE_M);
    REQUIRE(p.line(0)->pairs[0].letter == 'M');
    REQUIRE(p.line(0)->pairs[0].value == 4);
    
    REQUIRE(stb_sb_count(p.line(1)->pairs) == 0);
  }

  SECTION("comment parsing") {
    GCODEParser p;
    REQUIRE(p.push("(a comment line)\n") == GCODE_RESULT_TRUE);
    REQUIRE(stb_sb_count(p.handle->lines) == 2);
    REQUIRE(p.line(0)->pairs == nullptr);
    REQUIRE(p.line(0)->type == GCODE_LINE_TYPE_COMMENT);
    REQUIRE(p.line(0)->start_loc == 0);
    REQUIRE(p.line(0)->end_loc == 15);
    
    REQUIRE(stb_sb_count(p.line(0)->pairs) == 0);
    REQUIRE(stb_sb_count(p.line(1)->pairs) == 0);
  }

  SECTION("block delete") {
    GCODEParser p;
    REQUIRE(p.push("/G1 IGNORE ME\n") == GCODE_RESULT_TRUE);
    REQUIRE(stb_sb_count(p.handle->lines) == 2);
    REQUIRE(p.line(0)->pairs == nullptr);
    REQUIRE(p.line(0)->type == GCODE_LINE_TYPE_REMOVE_BLOCK);
    REQUIRE(p.line(0)->start_loc == 0);
    REQUIRE(p.line(0)->end_loc == 12);
    
    REQUIRE(stb_sb_count(p.line(0)->pairs) == 0);
    REQUIRE(stb_sb_count(p.line(1)->pairs) == 0);
  }

  SECTION("overlapping command words") {
    GCODEParser p;
    REQUIRE(p.push("G1G0\n") == GCODE_RESULT_ERROR);
    REQUIRE(p.push("M3M4\n") == GCODE_RESULT_ERROR);
  }

}