#include <doctest/doctest.h>

#define ok CHECK
#define gcode_debug INFO
#include "parser.h"

TEST_CASE("[gcode] letter letter fails") {
  GCODEParser p;
  ok(p.push('G') == GCODE_RESULT_TRUE);
  ok(p.push('G') == GCODE_RESULT_ERROR);
}


TEST_CASE("[gcode] tokenize G1") {
  GCODEParser p;
  ok(p.push('G') == GCODE_RESULT_TRUE);
  ok(p.push('1') == GCODE_RESULT_TRUE);
  ok(p.push('\n') == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->pairs != nullptr);
  
  ok(stb_sb_count(p.line(0)->pairs) == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_G);
  ok(p.line(0)->pairs[0].letter == 'G');
  ok(p.line(0)->pairs[0].value == 1);
  
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("[gcode] tokenize M4") {
  GCODEParser p;
  ok(p.push('M') == GCODE_RESULT_TRUE);
  ok(p.push('4') == GCODE_RESULT_TRUE);
  ok(p.push('\n') == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->pairs != nullptr);
  
  ok(stb_sb_count(p.line(0)->pairs) == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_M);
  ok(p.line(0)->pairs[0].letter == 'M');
  ok(p.line(0)->pairs[0].value == 4);
  
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("[gcode] comment full line") {
  GCODEParser p;
  ok(p.push("(a comment line)\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->pairs == nullptr);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMENT);
  ok(p.line(0)->start_loc == 0);
  ok(p.line(0)->end_loc == 15);
  
  ok(stb_sb_count(p.line(0)->pairs) == 0);
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("[gcode] block_delete") {
  GCODEParser p;
  ok(p.push("/G1 IGNORE ME\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->pairs == nullptr);
  ok(p.line(0)->type == GCODE_LINE_TYPE_REMOVE_BLOCK);
  ok(p.line(0)->start_loc == 0);
  ok(p.line(0)->end_loc == 12);
  
  ok(stb_sb_count(p.line(0)->pairs) == 0);
  ok(stb_sb_count(p.line(1)->pairs) == 0);
}

TEST_CASE("[gcode] duplicate_words") {
  GCODEParser p;
  ok(p.push("X1x2\n") == GCODE_RESULT_ERROR);
  ok(p.line_count() == 1);
}

TEST_CASE("[gcode] non_and_duplicate_words") {
  GCODEParser p;
  ok(p.push("g10X1Y5X2\n") == GCODE_RESULT_ERROR);
  ok(p.line_count() == 1);
  ok(p.line(0)->type == GCODE_LINE_TYPE_G);
  ok(p.line(0)->code == 10);
  ok(stb_sb_count(p.line(0)->pairs) == 3);
}

TEST_CASE("[gcode] setting set") {
  GCODEParser p;
  ok(p.push("$10    = 123\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_SET_SETTING);
  ok(p.line(0)->code == 10);
  ok(stb_sb_count(p.line(0)->pairs) == 0);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[gcode] setting set (error)") {
  GCODEParser p;
  ok(p.push("$1000 = 123\n") == GCODE_RESULT_ERROR);
}

TEST_CASE("[gcode] setting set (error)") {
  GCODEParser p;
  ok(p.push("$$\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_SETTINGS);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[gcode] get gcode parameters $#") {
  GCODEParser p;
  ok(p.push("$#\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_GCODE_PARAMETERS);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[gcode] get gcode parser state $G") {
  GCODEParser p;
  ok(p.push("$G\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_PARSER_STATE);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[gcode] get build info $I") {
  GCODEParser p;
  ok(p.push("$I\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_BUILD_INFO);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[gcode] get startup blocks $N") {
  GCODEParser p;
  ok(p.push("$N\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 2);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_GET_STARTUP_BLOCKS);
  ok(p.line(1)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[gcode] set startup blocks $N") {
  GCODEParser p;
  ok(p.push("$N1=$h\n") == GCODE_RESULT_TRUE);
  ok(p.push("$N2=G1X0F1000\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 3);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK);
  ok(p.line(0)->code == 1);
  ok(p.line(1)->type == GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK);
  ok(p.line(1)->code == 2);
  ok(p.line(2)->type == GCODE_LINE_TYPE_EOF);
}


TEST_CASE("[gcode] set startup blocks $N") {
  GCODEParser p;
  ok(p.push("$N1=$h\n") == GCODE_RESULT_TRUE);
  ok(p.push("$N2=G1X0F1000\n") == GCODE_RESULT_TRUE);
  ok(p.line_count() == 3);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK);
  ok(p.line(0)->code == 1);
  ok(p.line(1)->type == GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK);
  ok(p.line(1)->code == 2);
  ok(p.line(2)->type == GCODE_LINE_TYPE_EOF);
}

TEST_CASE("[grbl,gcode] commands") {
  GCODEParser p;
  ok(p.push(0x18) == GCODE_RESULT_TRUE);
  ok(p.line(0)->type == GCODE_LINE_TYPE_COMMAND_SOFT_RESET);
  ok(p.line_count() == 2);

  ok(p.push('?') == GCODE_RESULT_TRUE);
  ok(p.line(1)->type == GCODE_LINE_TYPE_COMMAND_STATUS_REPORT);
  ok(p.line_count() == 3);

  ok(p.push('~') == GCODE_RESULT_TRUE);
  ok(p.line(2)->type == GCODE_LINE_TYPE_COMMAND_CYCLE_START);
  ok(p.line_count() == 4);

  ok(p.push('!') == GCODE_RESULT_TRUE);
  ok(p.line(3)->type == GCODE_LINE_TYPE_COMMAND_FEED_HOLD);
  ok(p.line_count() == 5);

  ok(p.push(0x84) == GCODE_RESULT_TRUE);
  ok(p.line(4)->type == GCODE_LINE_TYPE_COMMAND_SAFETY_DOOR);
  ok(p.line_count() == 6);

  ok(p.push(0x85) == GCODE_RESULT_TRUE);
  ok(p.line(5)->type == GCODE_LINE_TYPE_COMMAND_JOG_CANCEL);
  ok(p.line_count() == 7);

  ok(p.push(0x90) == GCODE_RESULT_TRUE);
  ok(p.line(6)->type == GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_SET_100_PERCENT);
  ok(p.line_count() == 8);

  ok(p.push(0x91) == GCODE_RESULT_TRUE);
  ok(p.line(7)->type == GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_INCREASE_10_PERCENT);
  ok(p.line_count() == 9);

  ok(p.push(0x92) == GCODE_RESULT_TRUE);
  ok(p.line(8)->type == GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_DECREASE_10_PERCENT);
  ok(p.line_count() == 10);

  ok(p.push(0x93) == GCODE_RESULT_TRUE);
  ok(p.line(9)->type == GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_INCREASE_1_PERCENT);
  ok(p.line_count() == 11);

  ok(p.push(0x94) == GCODE_RESULT_TRUE);
  ok(p.line(10)->type == GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_DECREASE_1_PERCENT);
  ok(p.line_count() == 12);

  ok(p.push(0x95) == GCODE_RESULT_TRUE);
  ok(p.line(11)->type == GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_100_PERCENT);
  ok(p.line_count() == 13);

  ok(p.push(0x96) == GCODE_RESULT_TRUE);
  ok(p.line(12)->type == GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_50_PERCENT);
  ok(p.line_count() == 14);

  ok(p.push(0x97) == GCODE_RESULT_TRUE);
  ok(p.line(13)->type == GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_25_PERCENT);
  ok(p.line_count() == 15);

  ok(p.push(0x99) == GCODE_RESULT_TRUE);
  ok(p.line(14)->type == GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_SET_100_PERCENT);
  ok(p.line_count() == 16);

  ok(p.push(0x9A) == GCODE_RESULT_TRUE);
  ok(p.line(15)->type == GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_INCREASE_10_PERCENT);
  ok(p.line_count() == 17);

  ok(p.push(0x9B) == GCODE_RESULT_TRUE);
  ok(p.line(16)->type == GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_DECREASE_10_PERCENT);
  ok(p.line_count() == 18);

  ok(p.push(0x9C) == GCODE_RESULT_TRUE);
  ok(p.line(17)->type == GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_INCREASE_1_PERCENT);
  ok(p.line_count() == 19);

  ok(p.push(0x9D) == GCODE_RESULT_TRUE);
  ok(p.line(18)->type == GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_DECREASE_1_PERCENT);
  ok(p.line_count() == 20);

  ok(p.push(0x9E) == GCODE_RESULT_TRUE);
  ok(p.line(19)->type == GCODE_LINE_TYPE_COMMAND_TOGGLE_SPINDLE_STOP);
  ok(p.line_count() == 21);

  ok(p.push(0xA0) == GCODE_RESULT_TRUE);
  ok(p.line(20)->type == GCODE_LINE_TYPE_COMMAND_TOGGLE_FLOOD_COOLANT);
  ok(p.line_count() == 22);

  ok(p.push(0xA1) == GCODE_RESULT_TRUE);
  ok(p.line(21)->type == GCODE_LINE_TYPE_COMMAND_TOGGLE_MIST_COOLANT);
  ok(p.line_count() == 23);
}