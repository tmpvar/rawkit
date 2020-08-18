#include <doctest/doctest.h>

#define ok CHECK
#define gcode_debug INFO
#include "parser.h"

TEST_CASE("gcode, letter letter fails") {
  GCODEParser p;
  ok(p.push('G') == GCODE_RESULT_TRUE);
  ok(p.push('G') == GCODE_RESULT_ERROR);
}


TEST_CASE("gcode, tokenize G1") {
  GCODEParser p;
  ok(p.push('G') == GCODE_RESULT_TRUE);
  ok(p.push('1') == GCODE_RESULT_TRUE);
  ok(p.push('\n') == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->pairs != nullptr);
  
  ok(stb_sb_count(p.line(0)->pairs) == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_G);
  ok(p.line(0)->pairs[0].letter == 'G');
  ok(p.line(0)->pairs[0].value == 1);
  
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("gcode, tokenize M4") {
  GCODEParser p;
  ok(p.push('M') == GCODE_RESULT_TRUE);
  ok(p.push('4') == GCODE_RESULT_TRUE);
  ok(p.push('\n') == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->pairs != nullptr);
  
  ok(stb_sb_count(p.line(0)->pairs) == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_M);
  ok(p.line(0)->pairs[0].letter == 'M');
  ok(p.line(0)->pairs[0].value == 4);
  
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("gcode, comment full line") {
  GCODEParser p;
  ok(p.push("(a comment line)\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->pairs == nullptr);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMENT);
  ok(p.line(0)->start_loc == 0);
  ok(p.line(0)->end_loc == 15);
  
  ok(stb_sb_count(p.line(0)->pairs) == 0);
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("gcode, block_delete") {
  GCODEParser p;
  ok(p.push("/G1 IGNORE ME\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->pairs == nullptr);
  ok(p.line(0)->type == GCODE_LINE_TYPE_REMOVE_BLOCK);
  ok(p.line(0)->start_loc == 0);
  ok(p.line(0)->end_loc == 12);
  
  ok(stb_sb_count(p.line(0)->pairs) == 0);
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("gcode, duplicate_words") {
  GCODEParser p;
  ok(p.push("X1x2\n") == GCODE_RESULT_ERROR);
  ok(stb_sb_count(p.handle->lines) == 1);
}

TEST_CASE("gcode, non_and_duplicate_words") {
  GCODEParser p;
  ok(p.push("g10X1Y5X2\n") == GCODE_RESULT_ERROR);
  ok(stb_sb_count(p.handle->lines) == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_G);
  ok(p.line(0)->code == 10);
  ok(stb_sb_count(p.line(0)->pairs) == 3);
}

TEST_CASE("gcode, setting set") {
  GCODEParser p;
  ok(p.push("$10    = 123\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_SET_SETTING);
  ok(p.line(0)->code == 10);
  ok(stb_sb_count(p.line(0)->pairs) == 0);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("gcode, setting set (error)") {
  GCODEParser p;
  ok(p.push("$1000 = 123\n") == GCODE_RESULT_ERROR);
}

TEST_CASE("gcode, setting set (error)") {
  GCODEParser p;
  ok(p.push("$$\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_SETTINGS);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("gcode, get gcode parameters $#") {
  GCODEParser p;
  ok(p.push("$#\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_GCODE_PARAMETERS);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("gcode, get gcode parser state $G") {
  GCODEParser p;
  ok(p.push("$G\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_PARSER_STATE);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("gcode, get build info $I") {
  GCODEParser p;
  ok(p.push("$I\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_BUILD_INFO);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("gcode, get startup blocks $N") {
  GCODEParser p;
  ok(p.push("$N\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_STARTUP_BLOCKS);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("gcode, set startup blocks $N") {
  GCODEParser p;
  ok(p.push("$N1=$h\n") == GCODE_RESULT_TRUE);
  ok(p.push("$N2=G1X0F1000\n") == GCODE_RESULT_TRUE);
  ok(stb_sb_count(p.handle->lines) == 3);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK);
  ok(p.line(0)->code == 1);
  ok(p.line(1)->type == GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK);
  ok(p.line(1)->code == 2);
  ok(p.line(2)->type == GCODE_LINE_TYPE_EOF);
}
