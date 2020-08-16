#include <utest/utest.h>
#define eq ASSERT_EQ
#define ok EXPECT_TRUE

#include "parser.h"

UTEST(gcode, letter_letter_fails) {
  GCODEParser p;
  ok(p.push('G') == GCODE_RESULT_TRUE);
  ok(p.push('G') == GCODE_RESULT_ERROR);
}

UTEST(gcode, tokenize_G1) {
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

UTEST(gcode, tokenize_M4) {
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

UTEST(gcode, comment_full_line) {
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

UTEST(gcode, block_delete) {
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

UTEST(gcode, duplicate_words) {
  GCODEParser p;
  ok(p.push("X1x2\n") == GCODE_RESULT_ERROR);
  ok(stb_sb_count(p.handle->lines) == 1);
}

UTEST(gcode, non_and_duplicate_words) {
  GCODEParser p;
  ok(p.push("g10X1Y5X2\n") == GCODE_RESULT_ERROR);
  ok(stb_sb_count(p.handle->lines) == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_G);
  ok(p.line(0)->code == 10);
  ok(stb_sb_count(p.line(0)->pairs) == 1);
}
