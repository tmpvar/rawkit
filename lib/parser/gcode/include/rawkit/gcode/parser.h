/*
The MIT License (MIT)
Copyright © 2020 Elijah Insua <tmpvar@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the “Software”), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stb_sb.h>

/*
List of Supported G-Codes in Grbl v1.1:
  - Non-Modal Commands: G4, G10L2, G10L20, G28, G30, G28.1, G30.1, G53, G92, G92.1
  - Motion Modes: G0, G1, G2, G3, G38.2, G38.3, G38.4, G38.5, G80
  - Feed Rate Modes: G93, G94
  - Unit Modes: G20, G21
  - Distance Modes: G90, G91
  - Arc IJK Distance Modes: G91.1
  - Plane Select Modes: G17, G18, G19
  - Tool Length Offset Modes: G43.1, G49
  - Cutter Compensation Modes: G40
  - Coordinate System Modes: G54, G55, G56, G57, G58, G59
  - Control Modes: G61
  - Program Flow: M0, M1, M2, M30*
  - Coolant Control: M7*, M8, M9
  - Spindle Control: M3, M4, M5
  - Valid Non-Command Words: F, I, J, K, L, N, P, R, S, T, X, Y, Z
*/


#define GCODE_PARSER_BUFFER_LEN 1024

#if !defined(gcode_debug)
  #define gcode_debug printf
#endif

enum gcode_parse_result {
  GCODE_RESULT_ERROR = -1,
  GCODE_RESULT_FALSE = 0,
  GCODE_RESULT_TRUE = 1
};

enum gcode_word {
  GCODE_WORD_NONE = -1,
  GCODE_WORD_G = 0,
  GCODE_WORD_M,
  GCODE_WORD_F,
  GCODE_WORD_I,
  GCODE_WORD_J,
  GCODE_WORD_K,
  GCODE_WORD_L,
  GCODE_WORD_N,
  GCODE_WORD_P,
  GCODE_WORD_R,
  GCODE_WORD_S,
  GCODE_WORD_T,
  GCODE_WORD_X,
  GCODE_WORD_Y,
  GCODE_WORD_Z,
  GCODE_WORD_COUNT,
};

const char gcode_word_map[16] = {
  'G','M','F','I','J','K','L','N','P','R','S','T','X','Y','Z'
};

enum gcode_line_type {
  GCODE_LINE_TYPE_EOF = -1,
  GCODE_LINE_TYPE_G = GCODE_WORD_G,
  GCODE_LINE_TYPE_M = GCODE_WORD_M,
  GCODE_LINE_TYPE_COMMENT,
  GCODE_LINE_TYPE_REMOVE_BLOCK,

  // GRBL specific commands
  GCODE_LINE_TYPE_COMMAND_SOFT_RESET = 0x18,
  GCODE_LINE_TYPE_COMMAND_DOLLAR = '$',
  GCODE_LINE_TYPE_COMMAND_STATUS_REPORT = '?',
  GCODE_LINE_TYPE_COMMAND_CYCLE_START = '~',
  GCODE_LINE_TYPE_COMMAND_FEED_HOLD = '!',
  GCODE_LINE_TYPE_COMMAND_SAFETY_DOOR = 0x84,
  GCODE_LINE_TYPE_COMMAND_JOG_CANCEL = 0x85,

  // feed overrides
  GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_SET_100_PERCENT = 0x90,
  GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_INCREASE_10_PERCENT = 0x91,
  GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_DECREASE_10_PERCENT = 0x92,
  GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_INCREASE_1_PERCENT = 0x93,
  GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_DECREASE_1_PERCENT = 0x94,

  // rapid (seek) overrides
  GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_100_PERCENT = 0x95,
  GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_50_PERCENT = 0x96,
  GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_25_PERCENT = 0x97,

  // feed overrides
  GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_SET_100_PERCENT = 0x99,
  GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_INCREASE_10_PERCENT = 0x9A,
  GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_DECREASE_10_PERCENT = 0x9B,
  GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_INCREASE_1_PERCENT = 0x9C,
  GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_DECREASE_1_PERCENT = 0x9D,

  GCODE_LINE_TYPE_COMMAND_TOGGLE_SPINDLE_STOP = 0x9E,
  GCODE_LINE_TYPE_COMMAND_TOGGLE_FLOOD_COOLANT = 0xA0,
  GCODE_LINE_TYPE_COMMAND_TOGGLE_MIST_COOLANT = 0xA1,

  GCODE_LINE_TYPE_COMMAND_SET_SETTING = 0xF00000, // $\d = \d
  GCODE_LINE_TYPE_COMMAND_GET_SETTINGS,           // $$
  GCODE_LINE_TYPE_COMMAND_GET_GCODE_PARAMETERS,   // $$
  GCODE_LINE_TYPE_COMMAND_GET_PARSER_STATE,       // $%
  GCODE_LINE_TYPE_COMMAND_GET_BUILD_INFO,         // $I

  GCODE_LINE_TYPE_COMMAND_GET_STARTUP_BLOCKS,   // $N
  GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK,   // $Nx=...
  GCODE_LINE_TYPE_COMMAND_SET_GCODE_CHECK_MODE, // $C
  GCODE_LINE_TYPE_COMMAND_UNLOCK,               // $X
  GCODE_LINE_TYPE_COMMAND_HOME,                 // $h
  GCODE_LINE_TYPE_COMMAND_JOG,                  // $J=<gcode>

  GCODE_LINE_TYPE_COMMAND_CLEAR_EEPROM_SETTINGS, // $RST=$
  GCODE_LINE_TYPE_COMMAND_CLEAR_EEPROM_COORDS, // $RST=#
  GCODE_LINE_TYPE_COMMAND_CLEAR_EEPROM_ALL, // $RST=*

  GCODE_LINE_TYPE_COMMAND_SLEEP, // $SLP
};

enum gcode_axis_words {
  GCODE_AXIS_WORD_X = GCODE_WORD_X,
  GCODE_AXIS_WORD_Y = GCODE_WORD_Y,
  GCODE_AXIS_WORD_Z = GCODE_WORD_Z
};

typedef struct gcode_word_pair_t {
  char letter;
  float value;
} gcode_line_token_t;

/*
List of Supported G-Codes in Grbl v1.1:
  - Control Modes: G61
  - Valid Non-Command Words: F, I, J, K, L, N, P, R, S, T, X, Y, Z
*/

enum gcode_non_modal_command {
  GCODE_NON_MODAL_COMMAND_NONE,
  GCODE_NON_MODAL_COMMAND_G4,
  GCODE_NON_MODAL_COMMAND_G10L2,
  GCODE_NON_MODAL_COMMAND_G10L20,
  GCODE_NON_MODAL_COMMAND_G28,
  GCODE_NON_MODAL_COMMAND_G30,
  GCODE_NON_MODAL_COMMAND_G28_1,
  GCODE_NON_MODAL_COMMAND_G30_1,
  GCODE_NON_MODAL_COMMAND_G53,
  GCODE_NON_MODAL_COMMAND_G92,
  GCODE_NON_MODAL_COMMAND_G92_1,
};

enum gcode_control_mode {
  GCODE_CONTROl_MODES_NONE,
  GCODE_CONTROl_MODES_G61,
};

enum gcode_motion_mode {
  GCODE_MOTION_MODE_NONE,
  GCODE_MOTION_MODE_G0,
  GCODE_MOTION_MODE_G1,
  GCODE_MOTION_MODE_G2,
  GCODE_MOTION_MODE_G3,
  GCODE_MOTION_MODE_G38_2,
  GCODE_MOTION_MODE_G38_3,
  GCODE_MOTION_MODE_G38_4,
  GCODE_MOTION_MODE_G38_5,
  GCODE_MOTION_MODE_G80,
};

enum gcode_wcs_select {
  GCODE_WCS_SELECT_NONE,
  GCODE_WCS_SELECT_G54,
  GCODE_WCS_SELECT_G55,
  GCODE_WCS_SELECT_G56,
  GCODE_WCS_SELECT_G57,
  GCODE_WCS_SELECT_G58,
  GCODE_WCS_SELECT_G59,
};

enum gcode_plane_select {
  GCODE_PLANE_SELECT_NONE,
  GCODE_PLANE_SELECT_G17,
  GCODE_PLANE_SELECT_G18,
  GCODE_PLANE_SELECT_G19,
};

enum gcode_distance_mode {
  GCODE_DISTANCE_MODE_NONE,
  GCODE_DISTANCE_MODE_G90,
  GCODE_DISTANCE_MODE_G91,
};

enum gcode_arc_ijk_distance_mode {
  GCODE_ARC_IJK_DISTANCE_MODE_NONE,
  GCODE_ARC_IJK_DISTANCE_MODE_G91_1
};

enum gcode_feed_rate_mode {
  GCODE_FEED_RATE_MODE_NONE,
  GCODE_FEED_RATE_MODE_G93,
  GCODE_FEED_RATE_MODE_G94,
};

enum gcode_units_mode {
  GCODE_UNITS_MODE_NONE,
  GCODE_UNITS_MODE_G20,
  GCODE_UNITS_MODE_G21,
};

enum gcode_cutter_radius_compensation {
  GCODE_CUTTER_RADIUS_COMPENSATION_MODE_NONE,
  GCODE_CUTTER_RADIUS_COMPENSATION_MODE_G40,
};

enum gcode_tool_length_offset {
  GCODE_TOOL_LENGTH_OFFSET_NONE,
  GCODE_TOOL_LENGTH_OFFSET_G43_1,
  GCODE_TOOL_LENGTH_OFFSET_G49,
};

enum gcode_program_mode {
  GCODE_PROGRAM_MODE_NONE,
  GCODE_PROGRAM_MODE_M0,
  GCODE_PROGRAM_MODE_M1,
  GCODE_PROGRAM_MODE_M2,
  GCODE_PROGRAM_MODE_M30,
};

enum gcode_spindle_state {
  GCODE_SPINDLE_STATE_NONE,
  GCODE_SPINDLE_STATE_M3,
  GCODE_SPINDLE_STATE_M4,
  GCODE_SPINDLE_STATE_M5,
};

enum gcode_coolant_state {
  GCODE_COOLANT_STATE_NONE,
  GCODE_COOLANT_STATE_M7,
  GCODE_COOLANT_STATE_M8,
  GCODE_COOLANT_STATE_M9,
};


struct gcode_parser_state_t {
  gcode_non_modal_command non_modal_command;
  gcode_control_mode control_mode;
  gcode_program_mode program_mode;
  gcode_motion_mode motion_mode;
  gcode_wcs_select wcs_select;
  gcode_plane_select plane_select;
  gcode_distance_mode distance_mode;
  gcode_arc_ijk_distance_mode arc_ijk_distance_mode;
  gcode_feed_rate_mode feed_rate_mode;
  gcode_units_mode units_mode;
  gcode_cutter_radius_compensation cutter_radius_compensation;
  gcode_tool_length_offset tool_length_offset;
  gcode_spindle_state spindle_state;
  gcode_coolant_state coolant_state;
  float feed_rate;
  int32_t spindle_speed;
  int16_t tool;
};

typedef struct gcode_line_t {
  uint64_t start_loc;
  uint64_t end_loc;
  uint32_t words;
  gcode_line_type type;
  float code;
  gcode_word_pair_t* pairs;

  gcode_parser_state_t parser_state;
} gcode_line_t;

#define GCODE_DEBUG_STRING_LEN 2048
char gcode_debug_string[GCODE_DEBUG_STRING_LEN] = {0};

char *gcode_parser_state_debug(const gcode_parser_state_t parser_state) {

  char motion_mode[6] = {0};
  switch (parser_state.motion_mode) {
    case GCODE_MOTION_MODE_G0:    strncpy(motion_mode, "G0", 2); break;
    case GCODE_MOTION_MODE_G1:    strncpy(motion_mode, "G1", 2); break;
    case GCODE_MOTION_MODE_G2:    strncpy(motion_mode, "G2", 2); break;
    case GCODE_MOTION_MODE_G3:    strncpy(motion_mode, "G3", 2); break;
    case GCODE_MOTION_MODE_G38_2: strncpy(motion_mode, "G38.2", 5); break;
    case GCODE_MOTION_MODE_G38_3: strncpy(motion_mode, "G38.3", 5); break;
    case GCODE_MOTION_MODE_G38_4: strncpy(motion_mode, "G38.4", 5); break;
    case GCODE_MOTION_MODE_G38_5: strncpy(motion_mode, "G38.5", 5); break;
    case GCODE_MOTION_MODE_G80:   strncpy(motion_mode, "G80", 3); break;
    default:
      break;
  }

  char wcs_select[4] = {0};
  switch (parser_state.wcs_select) {
    case GCODE_WCS_SELECT_G54: strcpy(wcs_select, "G54"); break;
    case GCODE_WCS_SELECT_G55: strcpy(wcs_select, "G55"); break;
    case GCODE_WCS_SELECT_G56: strcpy(wcs_select, "G56"); break;
    case GCODE_WCS_SELECT_G57: strcpy(wcs_select, "G57"); break;
    case GCODE_WCS_SELECT_G58: strcpy(wcs_select, "G58"); break;
    case GCODE_WCS_SELECT_G59: strcpy(wcs_select, "G59"); break;
    default:
      break;
  }

  char plane_select[4] = {0};
  switch (parser_state.plane_select) {
    case GCODE_PLANE_SELECT_G17: strcpy(plane_select, "G17"); break;
    case GCODE_PLANE_SELECT_G18: strcpy(plane_select, "G18"); break;
    case GCODE_PLANE_SELECT_G19: strcpy(plane_select, "G19"); break;
    default:
      break;
  }

  char units_mode[4] = {0};
  switch (parser_state.units_mode) {
    case GCODE_UNITS_MODE_G20: strcpy(units_mode, "G20"); break;
    case GCODE_UNITS_MODE_G21: strcpy(units_mode, "G21"); break;
    default:
      break;
  }

  char distance_mode[4] = {0};
  switch (parser_state.distance_mode) {
    case GCODE_DISTANCE_MODE_G90: strcpy(distance_mode, "G90"); break;
    case GCODE_DISTANCE_MODE_G91: strcpy(distance_mode, "G91"); break;
    default:
      break;
  }

  char feed_rate_mode[4] = {0};
  switch (parser_state.feed_rate_mode) {
    case GCODE_FEED_RATE_MODE_G93: strcpy(feed_rate_mode, "G93"); break;
    case GCODE_FEED_RATE_MODE_G94: strcpy(feed_rate_mode, "G94"); break;
    default:
      break;
  }

  char spindle_state[4] = {0};
  switch (parser_state.spindle_state) {
    case GCODE_SPINDLE_STATE_M3: strcpy(spindle_state, "M3"); break;
    case GCODE_SPINDLE_STATE_M4: strcpy(spindle_state, "M4"); break;
    case GCODE_SPINDLE_STATE_M5: strcpy(spindle_state, "M5"); break;
    default:
      break;
  }

  char coolant_state[4] = {0};
  switch (parser_state.coolant_state) {
    case GCODE_COOLANT_STATE_M7: strcpy(coolant_state, "M7"); break;
    case GCODE_COOLANT_STATE_M8: strcpy(coolant_state, "M8"); break;
    case GCODE_COOLANT_STATE_M9: strcpy(coolant_state, "M9"); break;
    default:
      break;
  }

  sprintf(
    gcode_debug_string,
    // "[GC:G0 G54 G17 G21 G90 G94 M5 M9 T0 F0 S0]",
    "[GC:%s %s %s %s %s %s %s %s T%u F%0.3f S%u]",
    // non_modal_command,
    motion_mode,
    wcs_select,
    plane_select,
    units_mode,
    distance_mode,
    feed_rate_mode,
    spindle_state,
    coolant_state,
    parser_state.tool,
    parser_state.feed_rate,
    parser_state.spindle_speed

    // control_mode,
    // program_mode,
    // arc_ijk_distance_mode,
    // cutter_radius_compensation,
  );

  return gcode_debug_string;
}

// Rectify the state between two parser states. This is generally performed at the end
// of a line to pull in modal changes from the line to the main parser state. Also, in
// order to access state as of a line, the main parser's state is memcpy'd back to the
// line's parser state. Meaning, we can access the complete parser state as it was at
// any line - this is important for resuming jobs that have been aborted.
void gcode_parser_state_merge(gcode_parser_state_t *main_state, gcode_line_t *line) {
  gcode_parser_state_t *line_state = &line->parser_state;
  if (main_state == NULL || line_state == NULL) {
    gcode_debug("gcode_parser_state_merge was passed a null pointer");
    return;
  }

  if (line_state->non_modal_command != GCODE_NON_MODAL_COMMAND_NONE) {
    main_state->non_modal_command = line_state->non_modal_command;
  }

  if (line_state->control_mode != GCODE_CONTROl_MODES_NONE) {
    main_state->control_mode = line_state->control_mode;
  }

  if (line_state->program_mode != GCODE_PROGRAM_MODE_NONE) {
    main_state->program_mode = line_state->program_mode;
  }

  if (line_state->motion_mode != GCODE_MOTION_MODE_NONE) {
    main_state->motion_mode = line_state->motion_mode;
  } else if (main_state->motion_mode == GCODE_MOTION_MODE_NONE) {
    main_state->motion_mode = GCODE_MOTION_MODE_G0;
/*    if (
      (line->words & GCODE_AXIS_WORD_X) ||
      (line->words & GCODE_AXIS_WORD_Y) ||
      (line->words & GCODE_AXIS_WORD_Z)
    ) {
      line->
    }*/
  }

  if (line_state->wcs_select != GCODE_WCS_SELECT_NONE) {
    main_state->wcs_select = line_state->wcs_select;
  } else if (main_state->wcs_select == GCODE_WCS_SELECT_NONE) {
    main_state->wcs_select = GCODE_WCS_SELECT_G54;
  }

  if (line_state->plane_select != GCODE_PLANE_SELECT_NONE) {
    main_state->plane_select = line_state->plane_select;
  } else if (main_state->plane_select == GCODE_PLANE_SELECT_NONE) {
    main_state->plane_select = GCODE_PLANE_SELECT_G17;
  }

  if (line_state->distance_mode != GCODE_DISTANCE_MODE_NONE) {
    main_state->distance_mode = line_state->distance_mode;
  } else if (main_state->distance_mode == GCODE_DISTANCE_MODE_NONE) {
    main_state->distance_mode = GCODE_DISTANCE_MODE_G90;
  }

  if (line_state->arc_ijk_distance_mode != GCODE_ARC_IJK_DISTANCE_MODE_NONE) {
    main_state->arc_ijk_distance_mode = line_state->arc_ijk_distance_mode;
  }

  if (line_state->feed_rate_mode != GCODE_FEED_RATE_MODE_NONE) {
    main_state->feed_rate_mode = line_state->feed_rate_mode;
  } else if (main_state->feed_rate_mode == GCODE_FEED_RATE_MODE_NONE) {
    main_state->feed_rate_mode = GCODE_FEED_RATE_MODE_G93;
  }

  if (line_state->units_mode != GCODE_UNITS_MODE_NONE) {
    main_state->units_mode = line_state->units_mode;
  } else if (main_state->units_mode == GCODE_UNITS_MODE_NONE) {
    main_state->units_mode = GCODE_UNITS_MODE_G21;
  }

  if (line_state->cutter_radius_compensation != GCODE_CUTTER_RADIUS_COMPENSATION_MODE_NONE) {
    main_state->cutter_radius_compensation = line_state->cutter_radius_compensation;
  }

  if (line_state->tool_length_offset != GCODE_TOOL_LENGTH_OFFSET_NONE) {
    main_state->tool_length_offset = line_state->tool_length_offset;
  }

  if (line_state->spindle_state != GCODE_SPINDLE_STATE_NONE) {
    main_state->spindle_state = line_state->spindle_state;
  } else if (main_state->spindle_state == GCODE_SPINDLE_STATE_NONE) {
    main_state->spindle_state = GCODE_SPINDLE_STATE_M5;
  }

  if (line_state->coolant_state != GCODE_COOLANT_STATE_NONE) {
    main_state->coolant_state = line_state->coolant_state;
  } else if (main_state->coolant_state == GCODE_COOLANT_STATE_NONE) {
    main_state->coolant_state = GCODE_COOLANT_STATE_M9;
  }

  if (line->words & (1<<GCODE_WORD_F)) {
    main_state->feed_rate = line_state->feed_rate;
  }

  if (line->words & (1<<GCODE_WORD_S)) {
    main_state->spindle_speed = line_state->spindle_speed;
  }

  if (line_state->tool > -1) {
    main_state->tool = line_state->tool;
  }

  gcode_debug("main parser state: %s\n", gcode_parser_state_debug(*main_state));
  gcode_debug("line parser state: %s\n", gcode_parser_state_debug(*line_state));

  memcpy(line_state, main_state, sizeof(gcode_parser_state_t));
  gcode_debug("line parser state: %s\n", gcode_parser_state_debug(*line_state));
}

typedef struct gcode_parser_t {
  gcode_line_t *lines = NULL;
  char pending_buf[GCODE_PARSER_BUFFER_LEN] = {0};
  uint16_t pending_loc = 0;
  int64_t total_loc = 0;
  gcode_parser_state_t parser_state;
} gcode_parser_t;


bool is_whitespace(const char c) {
  switch (c) {
    case '\r':
    case '\n':
    case '\t':
    case ' ':
      return true;
    default:
      return false;
  }
}

inline gcode_parse_result gcode_parser_reset(gcode_parser_t *parser, gcode_parse_result result) {
  parser->pending_loc = 0;
  parser->pending_buf[0] = 0;
  return result;
}

gcode_parse_result gcode_parser_new_line(gcode_parser_t *parser) {
  gcode_line_t line;
  memset(&line, 0, sizeof(gcode_line_t));
  line.type = GCODE_LINE_TYPE_EOF;
  line.pairs = NULL;
  line.start_loc = parser->total_loc;
  line.end_loc = 0;
  line.words = 0;
  line.code = 0.0;
  stb_sb_push(parser->lines, line);


  memcpy(
    &line.parser_state,
    &parser->parser_state,
    sizeof(gcode_parser_state_t)
  );

  line.parser_state.feed_rate = -1.0f;
  line.parser_state.tool = -1;
  line.parser_state.spindle_speed = -1;

  return gcode_parser_reset(parser, GCODE_RESULT_TRUE);
}

inline bool gcode_is_digit(const char c) {
  if (c >= '0' && c <= '9') {
    return true;
  }
  return false;
}

inline bool gcode_is_numeric(const char c) {
  if (gcode_is_digit(c) || c == '-' || c == '+' || c == '.') {
    return true;
  }
  return false;
}

gcode_parse_result gcode_parser_line_process_command_dollar(gcode_parser_t *parser) {
  gcode_line_t *current_line = &parser->lines[stb_sb_count(parser->lines) - 1];
  const char *buf = parser->pending_buf;

  parser->pending_buf[parser->pending_loc] = 0;
  if (gcode_is_digit(buf[1])) {
    const char *equal = strstr(buf, "=");
    const int8_t l = (equal - buf) - 1;

    if (equal == NULL) {
      gcode_debug("ERROR: setting set command did not include an =\n");
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
    }

    if (l > 3) {
      gcode_debug("ERROR: setting set command numeric code > 999\n");
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
    }

    char str[4] = {0, 0, 0, 0};
    memcpy(&str, buf+1, l);
    int v = atoi(str);
    current_line->code = v;
    current_line->type = GCODE_LINE_TYPE_COMMAND_SET_SETTING;
    return GCODE_RESULT_TRUE;
  }

  switch (buf[1]) {
    case '$':
      current_line->type = GCODE_LINE_TYPE_COMMAND_GET_SETTINGS;
      if (parser->pending_loc > 2) {
        gcode_debug("ERROR: failed to parse $$, extra characters ('%s')\n", parser->pending_buf);
        return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
      }
      return GCODE_RESULT_TRUE;

    case '#':
      current_line->type = GCODE_LINE_TYPE_COMMAND_GET_GCODE_PARAMETERS;
      if (parser->pending_loc > 2) {
        gcode_debug("ERROR: failed to parse $#, extra characters ('%s')\n", parser->pending_buf);
        return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
      }
      return GCODE_RESULT_TRUE;

    case 'G':
      current_line->type = GCODE_LINE_TYPE_COMMAND_GET_PARSER_STATE;
      if (parser->pending_loc > 2) {
        gcode_debug("ERROR: failed to parse $G, extra characters ('%s')\n", parser->pending_buf);
        return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
      }
      return GCODE_RESULT_TRUE;

    case 'H':
      current_line->type = GCODE_LINE_TYPE_COMMAND_HOME;
      if (parser->pending_loc > 2) {
        gcode_debug("ERROR: failed to parse $H, extra characters ('%s')\n", parser->pending_buf);
        return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
      }
      return GCODE_RESULT_TRUE;

    case 'I':
      current_line->type = GCODE_LINE_TYPE_COMMAND_GET_BUILD_INFO;
      if (parser->pending_loc > 2) {
        gcode_debug("ERROR: failed to parse $I, extra characters ('%s')\n", parser->pending_buf);
        return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
      }
      return GCODE_RESULT_TRUE;

    case 'J':
      current_line->type = GCODE_LINE_TYPE_COMMAND_JOG;
      if (parser->pending_loc < 4) {
        gcode_debug("ERROR: failed to parse $J, extra characters ('%s')\n", parser->pending_buf);
        return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
      }

      return GCODE_RESULT_TRUE;

    case 'N':
      {
        if (parser->pending_loc < 3) {
          current_line->type = GCODE_LINE_TYPE_COMMAND_GET_STARTUP_BLOCKS;
          return GCODE_RESULT_TRUE;
        }

        if (!gcode_is_digit(buf[2])) {
          gcode_debug("ERROR: setting startup block $N<d>=... expected digit\n");
          return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
        }

        const char *equal = strstr(buf, "=");
        int l = (equal - buf) - 1;
        if (equal == NULL || l < 0) {
          gcode_debug("ERROR: setting startup block $N<d>=... expected =\n");
          return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
        }

        if (l>4) {
          gcode_debug("ERROR: setting startup block $N<d>=... d > 9\n");
          return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
        }

        current_line->type = GCODE_LINE_TYPE_COMMAND_SET_STARTUP_BLOCK;
        // grbl only supports 2 startup blocks so we avoid doing atoi and the
        // associated hoop jumps here.
        current_line->code = (buf[2] - '0');
        return GCODE_RESULT_TRUE;
      }

    default:
      gcode_debug("ERROR: unknown $ command (%c)\n", buf[1]);
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
  }
}

gcode_parse_result gcode_parser_line_add_pending_pair(gcode_parser_t *parser) {
  gcode_word_pair_t pair;
  pair.letter = parser->pending_buf[0];

  gcode_word word = GCODE_WORD_NONE;
  for (int i=0; i<GCODE_WORD_COUNT; i++) {
    if (gcode_word_map[i] == pair.letter) {
      word = (gcode_word)i;
      break;
    }
  }

  if (word < 0 || word >= GCODE_WORD_COUNT) {
    gcode_debug("ERROR: invalid gcode word '%c'\n", pair.letter);
    return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
  }

  parser->pending_buf[parser->pending_loc] = 0;
  pair.value = atof(parser->pending_buf + 1);
  gcode_line_t *line = &stb_sb_last(parser->lines);

  if (pair.letter == 'G') {
    switch ((int)pair.value) {

      // Motion Modes
      case 0:
        if (line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
          gcode_debug("ERROR: motion mode already specified (G0).\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.motion_mode = GCODE_MOTION_MODE_G0;
        parser->parser_state.motion_mode = GCODE_MOTION_MODE_G0;
        break;
      case 1:
        if (line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
          gcode_debug("ERROR: motion mode already specified (G1).\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.motion_mode = GCODE_MOTION_MODE_G1;
        parser->parser_state.motion_mode = GCODE_MOTION_MODE_G1;
        break;
      case 2:
        if (line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
          gcode_debug("ERROR: motion mode already specified (G2).\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.motion_mode = GCODE_MOTION_MODE_G2;
        parser->parser_state.motion_mode = GCODE_MOTION_MODE_G2;
        break;
      case 3:
        if (line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
          gcode_debug("ERROR: motion mode already specified (G3).\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.motion_mode = GCODE_MOTION_MODE_G3;
        parser->parser_state.motion_mode = GCODE_MOTION_MODE_G3;
        break;
      case 38:
        if (line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
          gcode_debug("ERROR: motion mode already specified (G38).\n");
          return GCODE_RESULT_ERROR;
        }

        if (pair.value == 38.2) {
          line->parser_state.motion_mode = GCODE_MOTION_MODE_G38_2;
          parser->parser_state.motion_mode = GCODE_MOTION_MODE_G38_2;
        } else if (pair.value == 38.3) {
          line->parser_state.motion_mode = GCODE_MOTION_MODE_G38_3;
          parser->parser_state.motion_mode = GCODE_MOTION_MODE_G38_3;
        } else if (pair.value == 38.4) {
          line->parser_state.motion_mode = GCODE_MOTION_MODE_G38_4;
          parser->parser_state.motion_mode = GCODE_MOTION_MODE_G38_4;
        } else if (pair.value == 38.5) {
          line->parser_state.motion_mode = GCODE_MOTION_MODE_G38_5;
          parser->parser_state.motion_mode = GCODE_MOTION_MODE_G38_5;
        }
        break;
      case 80:
        if (line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
          gcode_debug("ERROR: motion mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.motion_mode = GCODE_MOTION_MODE_G80;
        parser->parser_state.motion_mode = GCODE_MOTION_MODE_G80;
        break;

      // Plane Select Modes
      case 17:
        if (line->parser_state.plane_select != GCODE_PLANE_SELECT_NONE) {
          gcode_debug("ERROR: plane select already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.plane_select = GCODE_PLANE_SELECT_G17;
        parser->parser_state.plane_select = GCODE_PLANE_SELECT_G17;
        break;
      case 18:
        if (line->parser_state.plane_select != GCODE_PLANE_SELECT_NONE) {
          gcode_debug("ERROR: plane select already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.plane_select = GCODE_PLANE_SELECT_G18;
        parser->parser_state.plane_select = GCODE_PLANE_SELECT_G18;
        break;
      case 19:
        if (line->parser_state.plane_select != GCODE_PLANE_SELECT_NONE) {
          gcode_debug("ERROR: plane select already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.plane_select = GCODE_PLANE_SELECT_G19;
        parser->parser_state.plane_select = GCODE_PLANE_SELECT_G19;
        break;

      // Unit Modes
      case 20:
        if (line->parser_state.units_mode != GCODE_UNITS_MODE_NONE) {
          gcode_debug("ERROR: unit mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.units_mode = GCODE_UNITS_MODE_G20;
        parser->parser_state.units_mode = GCODE_UNITS_MODE_G20;
        break;
      case 21:
        if (line->parser_state.units_mode != GCODE_UNITS_MODE_NONE) {
          gcode_debug("ERROR: unit mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.units_mode = GCODE_UNITS_MODE_G21;
        parser->parser_state.units_mode = GCODE_UNITS_MODE_G21;
        break;

      // WCS Modes
      case 54:
        if (line->parser_state.wcs_select != GCODE_WCS_SELECT_NONE) {
          gcode_debug("ERROR: wcs already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.wcs_select = GCODE_WCS_SELECT_G54;
        parser->parser_state.wcs_select = GCODE_WCS_SELECT_G54;
        break;
      case 55:
        if (line->parser_state.wcs_select != GCODE_WCS_SELECT_NONE) {
          gcode_debug("ERROR: wcs already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.wcs_select = GCODE_WCS_SELECT_G55;
        parser->parser_state.wcs_select = GCODE_WCS_SELECT_G55;
        break;
      case 56:
        if (line->parser_state.wcs_select != GCODE_WCS_SELECT_NONE) {
          gcode_debug("ERROR: wcs already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.wcs_select = GCODE_WCS_SELECT_G56;
        parser->parser_state.wcs_select = GCODE_WCS_SELECT_G56;
        break;
      case 57:
        if (line->parser_state.wcs_select != GCODE_WCS_SELECT_NONE) {
          gcode_debug("ERROR: wcs already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.wcs_select = GCODE_WCS_SELECT_G57;
        parser->parser_state.wcs_select = GCODE_WCS_SELECT_G57;
        break;
      case 58:
        if (line->parser_state.wcs_select != GCODE_WCS_SELECT_NONE) {
          gcode_debug("ERROR: wcs already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.wcs_select = GCODE_WCS_SELECT_G58;
        parser->parser_state.wcs_select = GCODE_WCS_SELECT_G58;
        break;
      case 59:
        if (line->parser_state.wcs_select != GCODE_WCS_SELECT_NONE) {
          gcode_debug("ERROR: wcs already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.wcs_select = GCODE_WCS_SELECT_G59;
        parser->parser_state.wcs_select = GCODE_WCS_SELECT_G59;
        break;

      // Distance Modes
      case 90:
        if (line->parser_state.distance_mode != GCODE_DISTANCE_MODE_NONE) {
          gcode_debug("ERROR: distance mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.distance_mode = GCODE_DISTANCE_MODE_G90;
        parser->parser_state.distance_mode = GCODE_DISTANCE_MODE_G90;
        break;
      case 91:
        if (line->parser_state.distance_mode != GCODE_DISTANCE_MODE_NONE) {
          gcode_debug("ERROR: distance mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.distance_mode = GCODE_DISTANCE_MODE_G91;
        parser->parser_state.distance_mode = GCODE_DISTANCE_MODE_G91;
        break;

      // Feed Rate Modes
      case 93:
        if (line->parser_state.feed_rate_mode != GCODE_FEED_RATE_MODE_NONE) {
          gcode_debug("ERROR: feed rate mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.feed_rate_mode = GCODE_FEED_RATE_MODE_G93;
        parser->parser_state.feed_rate_mode = GCODE_FEED_RATE_MODE_G93;
        break;
      case 94:
        if (line->parser_state.feed_rate_mode != GCODE_FEED_RATE_MODE_NONE) {
          gcode_debug("ERROR: feed rate mode already specified.\n");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.feed_rate_mode = GCODE_FEED_RATE_MODE_G94;
        parser->parser_state.feed_rate_mode = GCODE_FEED_RATE_MODE_G94;
        break;
    }

    line->type = (gcode_line_type)word;
    line->code = pair.value;

  } else if (pair.letter == 'M') {

    switch ((int)pair.value) {
      // Program Modes
      case 0:
        if (line->parser_state.program_mode != GCODE_PROGRAM_MODE_NONE) {
          gcode_debug("ERROR: program mode already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.program_mode = GCODE_PROGRAM_MODE_M0;
        parser->parser_state.program_mode = GCODE_PROGRAM_MODE_M0;
        break;
      case 1:
        if (line->parser_state.program_mode != GCODE_PROGRAM_MODE_NONE) {
          gcode_debug("ERROR: program mode already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.program_mode = GCODE_PROGRAM_MODE_M1;
        parser->parser_state.program_mode = GCODE_PROGRAM_MODE_M1;
        break;
      case 2:
        if (line->parser_state.program_mode != GCODE_PROGRAM_MODE_NONE) {
          gcode_debug("ERROR: program mode already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.program_mode = GCODE_PROGRAM_MODE_M2;
        parser->parser_state.program_mode = GCODE_PROGRAM_MODE_M2;
        break;
      case 30:
        if (line->parser_state.program_mode != GCODE_PROGRAM_MODE_NONE) {
          gcode_debug("ERROR: program mode already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.program_mode = GCODE_PROGRAM_MODE_M30;
        parser->parser_state.program_mode = GCODE_PROGRAM_MODE_M30;
        break;

      // Coolant Control
      case 7:
        if (line->parser_state.coolant_state != GCODE_COOLANT_STATE_NONE) {
          gcode_debug("ERROR: coolant state already specified (M7)");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.coolant_state = GCODE_COOLANT_STATE_M7;
        parser->parser_state.coolant_state = GCODE_COOLANT_STATE_M7;
        break;
      case 8:
        if (line->parser_state.coolant_state != GCODE_COOLANT_STATE_NONE) {
          gcode_debug("ERROR: coolant state already specified (M8)");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.coolant_state = GCODE_COOLANT_STATE_M8;
        parser->parser_state.coolant_state = GCODE_COOLANT_STATE_M8;
        break;
      case 9:
        if (line->parser_state.coolant_state != GCODE_COOLANT_STATE_NONE) {
          gcode_debug("ERROR: coolant state already specified (M9)");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.coolant_state = GCODE_COOLANT_STATE_M9;
        parser->parser_state.coolant_state = GCODE_COOLANT_STATE_M9;
        break;

      // Spindle Control
      case 3:
        if (line->parser_state.spindle_state != GCODE_SPINDLE_STATE_NONE) {
          gcode_debug("ERROR: spindle state already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.spindle_state = GCODE_SPINDLE_STATE_M3;
        parser->parser_state.spindle_state = GCODE_SPINDLE_STATE_M3;
        break;
      case 4:
        if (line->parser_state.spindle_state != GCODE_SPINDLE_STATE_NONE) {
          gcode_debug("ERROR: spindle state already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.spindle_state = GCODE_SPINDLE_STATE_M4;
        parser->parser_state.spindle_state = GCODE_SPINDLE_STATE_M4;
        break;
      case 5:
        if (line->parser_state.spindle_state != GCODE_SPINDLE_STATE_NONE) {
          gcode_debug("ERROR: spindle state already specified");
          return GCODE_RESULT_ERROR;
        }
        line->parser_state.spindle_state = GCODE_SPINDLE_STATE_M5;
        parser->parser_state.spindle_state = GCODE_SPINDLE_STATE_M5;
        break;
    }

    line->type = (gcode_line_type)word;
    line->code = pair.value;
  } else {
    uint32_t mask = 1<<(int)word;
    if (line->words & mask) {
      gcode_debug("ERROR: duplicate word '%c' found\n", pair.letter);
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
    }

    switch (pair.letter) {
      case 'S':
        line->parser_state.spindle_speed = (int32_t)pair.value;
        break;
      case 'F':
        line->parser_state.feed_rate = pair.value;
        break;
    }

    line->words |= mask;
  }

  stb_sb_push(line->pairs, pair);
  return GCODE_RESULT_TRUE;
}

gcode_parse_result gcode_parser_add_pending_char(gcode_parser_t *parser, char c) {
  if (parser == NULL) {
    gcode_debug("ERROR: could not add pending char to null parser\n");
    return GCODE_RESULT_ERROR;
  }
  parser->pending_buf[parser->pending_loc++] = c;
  return GCODE_RESULT_TRUE;
}

gcode_parse_result gcode_parser_input(gcode_parser_t *parser, uint8_t c) {
  parser->total_loc++;
  if (parser->lines == NULL) {
    if (gcode_parser_new_line(parser) != GCODE_RESULT_TRUE) {
      gcode_debug("ERROR: gcode_parser_new_line failed\n");
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
    }
  }

  gcode_line_t *current_line = &parser->lines[stb_sb_count(parser->lines) - 1];
  if (c == '\n') {
    if  (parser->pending_loc == 0) {
      return GCODE_RESULT_TRUE;
    }

    current_line->end_loc = parser->total_loc - 1;
    gcode_parse_result result = GCODE_RESULT_FALSE;
    switch (current_line->type) {
      // handle comments `(` and remove blocks `/` by doing nothing
      case GCODE_LINE_TYPE_COMMENT:
      case GCODE_LINE_TYPE_REMOVE_BLOCK:
        break;

      // handle $ commands
      case GCODE_LINE_TYPE_COMMAND_DOLLAR:
        if (gcode_parser_line_process_command_dollar(parser) == GCODE_RESULT_ERROR) {
          gcode_parser_new_line(parser);
          return GCODE_RESULT_ERROR;
        }
        break;

      // normal gcode lines
      default:
        if (gcode_parser_line_add_pending_pair(parser) != GCODE_RESULT_TRUE) {
          gcode_debug("ERROR: failed to add a pending pair\n");
          gcode_parser_new_line(parser);
          return GCODE_RESULT_ERROR;
        }
    }

    if (current_line->parser_state.motion_mode != GCODE_MOTION_MODE_NONE) {
      if (!current_line->words) {
        gcode_debug("ERROR: motion mode specified without axis word");
        return GCODE_RESULT_ERROR;
      }

      // TODO: error if coordinated motion without feed rate
      // TODO: feed rate is also persistent between commands so
      //       there probably needs to be global parserstate for this.
    }

    gcode_parser_state_merge(&parser->parser_state, current_line);

    return gcode_parser_new_line(parser);
  }

  if (is_whitespace(c)) {
    return GCODE_RESULT_TRUE;
  }

  if (parser->pending_loc >= GCODE_PARSER_BUFFER_LEN - 1) {
    gcode_debug("ERROR: overran pending buffer\n");
    return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
  }

  if (
    current_line->type == GCODE_LINE_TYPE_REMOVE_BLOCK ||
    current_line->type == GCODE_LINE_TYPE_COMMENT
  ) {
    return gcode_parser_add_pending_char(parser, c);
  }

  // upper case all letters
  if (c >= 'a' && c <= 'z') {
    c -= 32;
  }

  if (current_line->type == GCODE_LINE_TYPE_COMMAND_DOLLAR) {
    return gcode_parser_add_pending_char(parser, c);
  }

  // collect a letter
  if (parser->pending_loc == 0) {
    // handle comments
    // TODO: it is assumed that a comment spans the entire line..
    if (c == '(') {
      current_line->type = GCODE_LINE_TYPE_COMMENT;
      return GCODE_RESULT_TRUE;
    }

    if (c == '/') {
      current_line->type = GCODE_LINE_TYPE_REMOVE_BLOCK;
      return GCODE_RESULT_TRUE;
    }

    switch (c) {
      case '$':
        current_line->type = GCODE_LINE_TYPE_COMMAND_DOLLAR;
        break;
      case 0x18:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SOFT_RESET;
        return gcode_parser_new_line(parser);
      case '?':
        current_line->type = GCODE_LINE_TYPE_COMMAND_STATUS_REPORT;
        return gcode_parser_new_line(parser);
      case '~':
        current_line->type = GCODE_LINE_TYPE_COMMAND_CYCLE_START;
        return gcode_parser_new_line(parser);
      case '!':
        current_line->type = GCODE_LINE_TYPE_COMMAND_FEED_HOLD;
        return gcode_parser_new_line(parser);
      case 0x84:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SAFETY_DOOR;
        return gcode_parser_new_line(parser);
      case 0x85:
        current_line->type = GCODE_LINE_TYPE_COMMAND_JOG_CANCEL;
        return gcode_parser_new_line(parser);
      case 0x90:
        current_line->type = GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_SET_100_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x91:
        current_line->type = GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_INCREASE_10_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x92:
        current_line->type = GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_DECREASE_10_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x93:
        current_line->type = GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_INCREASE_1_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x94:
        current_line->type = GCODE_LINE_TYPE_COMMAND_FEED_OVERRIDE_DECREASE_1_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x95:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_100_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x96:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_50_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x97:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SEEK_OVERRIDE_SET_25_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x99:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_SET_100_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x9A:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_INCREASE_10_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x9B:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_DECREASE_10_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x9C:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_INCREASE_1_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x9D:
        current_line->type = GCODE_LINE_TYPE_COMMAND_SPINDLE_OVERRIDE_DECREASE_1_PERCENT;
        return gcode_parser_new_line(parser);
      case 0x9E:
        current_line->type = GCODE_LINE_TYPE_COMMAND_TOGGLE_SPINDLE_STOP;
        return gcode_parser_new_line(parser);
      case 0xA0:
        current_line->type = GCODE_LINE_TYPE_COMMAND_TOGGLE_FLOOD_COOLANT;
        return gcode_parser_new_line(parser);
      case 0xA1:
        current_line->type = GCODE_LINE_TYPE_COMMAND_TOGGLE_MIST_COOLANT;
        return gcode_parser_new_line(parser);
      default:
        // expect this to be a letter
        if (c < 'A' && c > 'Z') {
          gcode_debug("ERROR: expect first char to be a letter\n");
          return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
        }
    }

    return gcode_parser_add_pending_char(parser, c);
  }

  // handle normal gcode word pairs
  if (c >= 'A' && c <= 'Z') {
    if (gcode_parser_line_add_pending_pair(parser) != GCODE_RESULT_TRUE) {
      gcode_debug("ERROR: failed to add pending pair\n");
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
    }

    if (parser->pending_loc == 1) {
      gcode_debug("ERROR: two letters found in sequence\n");
      return gcode_parser_reset(parser, GCODE_RESULT_ERROR);
    }

    // reset the pending buffer and add the current letter
    parser->pending_loc = 0;
  }

  return gcode_parser_add_pending_char(parser, c);
}

class GCODEParser {
  public:
    gcode_parser_t *handle = nullptr;
    GCODEParser() {
      this->init();
    }
    ~GCODEParser() {
      if (this->handle != nullptr) {
        free(this->handle);
      }
    }

    void init() {
      if (this->handle == NULL) {
        this->handle = (gcode_parser_t *)malloc(sizeof(gcode_parser_t));
        memset(this->handle, 0, sizeof(gcode_parser_t));
        this->handle->total_loc = -1;

        // setup the default parser state
        {
          this->handle->parser_state.motion_mode = GCODE_MOTION_MODE_G0;
          this->handle->parser_state.wcs_select = GCODE_WCS_SELECT_G54;
          this->handle->parser_state.plane_select = GCODE_PLANE_SELECT_G17;
          this->handle->parser_state.distance_mode = GCODE_DISTANCE_MODE_G90;
          this->handle->parser_state.feed_rate_mode = GCODE_FEED_RATE_MODE_G93;
          this->handle->parser_state.units_mode = GCODE_UNITS_MODE_G21;
          this->handle->parser_state.spindle_state = GCODE_SPINDLE_STATE_M5;
          this->handle->parser_state.coolant_state = GCODE_COOLANT_STATE_M9;
        }
      }
    }

    gcode_parse_result push(const char c) {
      init();
      return gcode_parser_input(this->handle, c);
    }

    gcode_parse_result push(const char *str) {
      init();
      for (size_t i = 0; i<strlen(str); i++) {
        gcode_parse_result r = gcode_parser_input(this->handle, str[i]);
        if (r != GCODE_RESULT_TRUE) {
          return r;
        }
      }
      return GCODE_RESULT_TRUE;
    }

    gcode_line_t *line(const uint64_t number) {
      if (this->handle == NULL) {
        return NULL;
      }

      if (stb_sb_count(this->handle->lines) <= number) {
        return NULL;
      }

      return &this->handle->lines[number];
    }

    gcode_line_t *last_line() {
      if (this->handle == NULL || this->handle->lines == NULL) {
        return NULL;
      }

      return &stb_sb_last(this->handle->lines);
    }

    uint64_t line_count() {
      if (this->handle == NULL) {
        return 0;
      }
      return stb_sb_count(this->handle->lines);
    }
};