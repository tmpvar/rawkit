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

#include <stdlib.h>
#include <stb_sb.h>

#ifndef GRBL_PARSER_BUFFER_LEN
  #define GRBL_PARSER_BUFFER_LEN 1024
#endif

#define GRBL_TRUE 1
#define GRBL_FALSE 0
#define GRBL_ERROR -1

enum grbl_response_token_type {
  GRBL_TOKEN_TYPE_STATUS,
  GRBL_TOKEN_TYPE_ALARM,
  GRBL_TOKEN_TYPE_WELCOME,
  GRBL_TOKEN_TYPE_VERSION,
  GRBL_TOKEN_TYPE_STATUS_REPORT,
  GRBL_TOKEN_TYPE_MACHINE_STATE,
  GRBL_TOKEN_TYPE_MACHINE_POSITION,
  GRBL_TOKEN_TYPE_WCO_POSITION,
  GRBL_TOKEN_TYPE_WCO_OFFSET,
  GRBL_TOKEN_TYPE_BUFFER_STATE,
  GRBL_TOKEN_TYPE_LINE_NUMBER,
  GRBL_TOKEN_TYPE_CURRENT_FEED_AND_SPINDLE,
  GRBL_TOKEN_TYPE_INPUT_PIN_STATE,
  GRBL_TOKEN_TYPE_OVERRIDES,
  GRBL_TOKEN_TYPE_ACCESSORY_STATE,
  GRBL_TOKEN_TYPE_SETTING_KEY,
  GRBL_TOKEN_TYPE_SETTING_VALUE,
  GRBL_TOKEN_TYPE_MESSAGE_MSG,
  GRBL_TOKEN_TYPE_MESSAGE_HLP,
  GRBL_TOKEN_TYPE_MESSAGE_VER,
  GRBL_TOKEN_TYPE_MESSAGE_OPT,
  GRBL_TOKEN_TYPE_MESSAGE_GC,
  GRBL_TOKEN_TYPE_MESSAGE_POS_G28,
  GRBL_TOKEN_TYPE_MESSAGE_POS_G30,
  GRBL_TOKEN_TYPE_MESSAGE_WCO_G54,
  GRBL_TOKEN_TYPE_MESSAGE_WCO_G55,
  GRBL_TOKEN_TYPE_MESSAGE_WCO_G56,
  GRBL_TOKEN_TYPE_MESSAGE_WCO_G57,
  GRBL_TOKEN_TYPE_MESSAGE_WCO_G58,
  GRBL_TOKEN_TYPE_MESSAGE_WCO_G59,
  GRBL_TOKEN_TYPE_MESSAGE_CSO_G92,
  GRBL_TOKEN_TYPE_MESSAGE_TLO,
  GRBL_TOKEN_TYPE_MESSAGE_PRB,
};

enum grbl_alarm_code {
  GRBL_ALARM_NONE = 0,
  GRBL_ALARM_HARD_LIMIT,
  GRBL_ALARM_SOFT_LIMIT,
  GRBL_ALARM_ABORT_DURING_CYCLE,
  GRBL_ALARM_PROBE_FAIL_UNEXPECTED_INITIAL_STATE,
  GRBL_ALARM_PROBE_FAIL_NO_CONTACT,
  GRBL_ALARM_HOMING_FAIL_RESET,
  GRBL_ALARM_HOMING_FAIL_SAFETY_DOOR,
  GRBL_ALARM_HOMING_FAIL_PULL_OFF,
  GRBL_ALARM_HOMING_FAIL_NO_CONTACT,
  GRBL_ALARM_HOMING_FAIL_NO_CONTACT_DUAL_AXIS,
};

enum grbl_error_code {
  GRBL_ERROR_NONE = 0,
  GRBL_ERROR_EXPECTED_COMMAND_LETTER,
  GRBL_ERROR_BAD_NUMBER_FORMAT,
  GRBL_ERROR_INVALID_STATEMENT,
  GRBL_ERROR_UNEXPECTED_NEGATIVE_VALUE,
  GRBL_ERROR_HOMING_NOT_ENABLED,
  GRBL_ERROR_MINIMUM_STEP_PULSE_TIME,
  GRBL_ERROR_EEPROM_READ_FAIL,
  GRBL_ERROR_NOT_IDLE,
  GRBL_ERROR_LOCKED_OR_JOGGING,
  GRBL_ERROR_SOFT_LIMITS_REQUIRE_HOMING,
  GRBL_ERROR_LINE_OVERFLOW,
  GRBL_ERROR_MAX_STEP_RATE_EXCEEDED,
  GRBL_ERROR_CHECK_DOOR,
  GRBL_ERROR_LINE_LENGTH_EXCEEDED,
  GRBL_ERROR_TRAVEL_EXCEEDED,
  GRBL_ERROR_INVALID_JOG_COMMAND,
  GRBL_ERROR_LASER_MODE_REQUIRES_PWM_OUTPUT,
  GRBL_ERROR_UNSUPPORTED_COMMAND,
  GRBL_ERROR_MODAL_GROUP_VIOLATION,
  GRBL_ERROR_UNDEFINED_FEED_RATE,
  GRBL_ERROR_GCODE_EXPECTED_INTEGER_VALUE,
  GRBL_ERROR_GCODE_EXPECTED_ONE_COMMAND,
  GRBL_ERROR_GCODE_UNEXPECTED_REPEATED_GCODE_WORD,
  GRBL_ERROR_GCODE_EXPECTED_AXIS_WORDS,
  GRBL_ERROR_GCODE_INVALID_LINE_NUMBER,
  GRBL_ERROR_GCODE_MISSING_REQUIRED_VALUE,
  GRBL_ERROR_GCODE_G59_WORK_COORDINATES_NOT_SUPPORTED,
  GRBL_ERROR_GCODE_G53_NOT_ALLOWED,
  GRBL_ERROR_GCODE_UNEXPECTED_AXIS_WORDS,
  GRBL_ERROR_GCODE_EXPECTED_ARC_AXIS_WORDS,
  GRBL_ERROR_GCODE_INVALID_MOTION_COMMAND_TARGET,
  GRBL_ERROR_GCODE_INVALID_ARC_RADIUS,
  GRBL_ERROR_GCODE_EXPECTED_ARC_AXIS_WORDS_2,
  GRBL_ERROR_GCODE_UNUSED_WORDS,
  GRBL_ERROR_GCODE_TOOL_LENGTH_OFFSET_NOT_ASSIGNED,
  GRBL_ERROR_GCODE_MAX_TOOL_NUMBER_EXCEEDED
};

enum grbl_setting_code {
  GRBL_SETTING_STEP_PULSE_TIME = 0,                  // u32
  GRBL_SETTING_STEP_IDLE_DELAY = 1,                  // u32
  GRBL_SETTING_INVERT_STEP_PULSE_MASK = 2,           // u8
  GRBL_SETTING_INVERT_STEP_DIRECTION_MASK = 3,       // u8
  GRBL_SETTING_INVERT_STEP_ENABLE_PIN = 4,           // bool
  GRBL_SETTING_INVERT_LIMIT_PINS = 5,                // bool
  GRBL_SETTING_INVERT_PROBE_PIN = 6,                 // bool
  GRBL_SETTING_STATUS_REPORT_OPTIONS_MASK = 10,      // u8
  GRBL_SETTING_JUCTION_DEVIATION = 11,               // float
  GRBL_SETTING_ARC_TOLERANCE = 12,                   // float
  GRBL_SETTING_REPORT_IN_INCHES = 13,                // bool
  GRBL_SETTING_SOFT_LIMITS_ENABLE = 20,              // bool
  GRBL_SETTING_HARD_LIMITS_ENABLE = 21,              // bool
  GRBL_SETTING_HOMING_CYCLE_ENABLE = 22,             // bool
  GRBL_SETTING_INVERT_HOMING_DIRECTION_MASK = 23,    // bool
  GRBL_SETTING_HOMING_LOCATE_FEED_RATE = 24,         // float
  GRBL_SETTING_HOMING_SEARCH_SEEK_RATE = 25,         // float
  GRBL_SETTING_HOMING_SWITCH_DEBOUNCE_DELAY = 26,    // u32
  GRBL_SETTING_HOMING_SWITCH_PULL_OFF_DISTANCE = 27, // float
  GRBL_SETTING_MAXIMUM_SPINDLE_SPEED = 30,           // u32
  GRBL_SETTING_MINIMUM_SPINDLE_SPEED = 31,           // u32
  GRBL_SETTING_LASER_MODE_ENABLE = 32,               // bool
  GRBL_SETTING_X_AXIS_TRAVEL_RESOLUTION = 100,       // float
  GRBL_SETTING_Y_AXIS_TRAVEL_RESOLUTION = 101,       // float
  GRBL_SETTING_Z_AXIS_TRAVEL_RESOLUTION = 102,       // float
  GRBL_SETTING_X_AXIS_MAXIMUM_RATE = 110,            // float
  GRBL_SETTING_Y_AXIS_MAXIMUM_RATE = 111,            // float
  GRBL_SETTING_Z_AXIS_MAXIMUM_RATE = 112,            // float
  GRBL_SETTING_X_AXIS_ACCELERATION = 120,            // float
  GRBL_SETTING_Y_AXIS_ACCELERATION = 121,            // float
  GRBL_SETTING_Z_AXIS_ACCELERATION = 122,            // float
  GRBL_SETTING_X_AXIS_MAXIMUM_TRAVEL = 130,           // u32
  GRBL_SETTING_Y_AXIS_MAXIMUM_TRAVEL = 131,           // u32
  GRBL_SETTING_Z_AXIS_MAXIMUM_TRAVEL = 132,           // u32
};

typedef struct grbl_msg_version_t {
  uint16_t major;
  uint16_t minor;
  char letter;
} grbl_msg_version_t;

enum grbl_machine_state {
  GRBL_MACHINE_STATE_ALARM,
  GRBL_MACHINE_STATE_CHECK,
  GRBL_MACHINE_STATE_DOOR,
  GRBL_MACHINE_STATE_HOLD,
  GRBL_MACHINE_STATE_HOME,
  GRBL_MACHINE_STATE_IDLE,
  GRBL_MACHINE_STATE_JOG,
  GRBL_MACHINE_STATE_RUN,
  GRBL_MACHINE_STATE_SLEEP,

  GRBL_MACHINE_STATE_COUNT
};

enum grbl_motion_mode {
  G0,
  G1,
  G2,
  G3,
  G38_2,
  G38_3,
  G38_4,
  G38_5,
  G80,
};

enum grbl_coordinate_system {
  G54,
  G55,
  G56,
  G57,
  G58,
  G59,
};

enum grbl_plane_select {
  G17,
  G18,
  G19,
};

enum grbl_distance_mode {
  G90,
  G91,
};

enum grbl_arc_ijk_distance_mode {
  G91_1
};

enum grbl_feed_rate_mode {
  G93,
  G94,
};

enum grbl_units_mode {
  G20,
  G21,
};

enum grbl_cutter_radius_compensation {
  G40,
};

enum grbl_tool_length_offset {
  G43_1,
  G49,
};

enum grbl_program_mode {
  M0,
  M1,
  M2,
  M30,
};

enum grbl_spindle_state {
  M3,
  M4,
  M5,
};

enum grbl_coolant_state {
  M7,
  M8,
  M9,
};

struct grbl_gcode_state {
  grbl_motion_mode motion_mode;
  grbl_coordinate_system coordinate_system;
  grbl_plane_select plane;
  grbl_units_mode units;
  grbl_distance_mode distance_mode;
  grbl_feed_rate_mode feed_rate_mode;
  grbl_spindle_state spindle_state;
  grbl_coolant_state coolant_state;
  grbl_program_mode program_mode;
  uint8_t tool_index;
  uint16_t feed;
  uint16_t spindle_rpm;
};

const char *grbl_machine_state_str[GRBL_MACHINE_STATE_COUNT] = {
  "Alarm",
  "Check",
  "Door",
  "Hold",
  "Home",
  "Idle",
  "Jog",
  "Run",
  "Sleep"
};

typedef struct grbl_msg_machine_state_t {
  grbl_machine_state value;
  uint16_t code;
} grbl_msg_status_report_t;

union grbl_response_token_value_t {
  uint8_t error_code;
  grbl_msg_version_t version;
};

typedef struct grbl_vec3_t {
  float x, y, z;
} grbl_vec3_t;

typedef struct grbl_probe_state_t {
  grbl_vec3_t vec3;
  uint32_t success;
} grbl_probe_state_t;

typedef struct grbl_msg_overrides_t {
  float feed, rapids, spindle;
} grbl_msg_overrides_t;

typedef struct grbl_buffer_state_t {
  uint32_t available_blocks;
  uint32_t available_bytes;
} grbl_buffer_state_t;

enum grbl_spindle_direction_state {
  GRBL_SPINDLE_DIRECTION_OFF,
  GRBL_SPINDLE_DIRECTION_CW,
  GRBL_SPINDLE_DIRECTION_CCW,
};

typedef struct grbl_msg_accessory_state_t {
  grbl_spindle_direction_state spindle;
  uint8_t flood_coolant;
  uint8_t mist_coolant;
} grbl_msg_accessory_state_t;

typedef struct grbl_msg_current_feed_and_speed_t {
  uint32_t feed;
  uint32_t spindle;
} grbl_msg_current_feed_and_speed_t;

typedef struct grbl_msg_current_pins_t {
  uint8_t limit_x;
  uint8_t limit_y;
  uint8_t limit_z;
  uint8_t limit_a;
  uint8_t probe;
  uint8_t door;
  uint8_t hold;
  uint8_t soft_reset;
  uint8_t cycle_start;
} grbl_msg_current_pins_t;


typedef struct grbl_msg_build_options_t {
  uint8_t variable_spindle;                          // V
  uint8_t line_numbers;                              // N
  uint8_t mist_coolant;                              // M
  uint8_t core_xy;                                   // C
  uint8_t parking_motion;                            // P
  uint8_t homing_force_origin;                       // Z
  uint8_t homing_single_axis;                        // H
  uint8_t two_limit_switches_on_axis;                // T
  uint8_t allow_feed_rate_overrides_in_probe_cycles; // A
  uint8_t use_spindle_direction_as_enable_pin;       // D
  uint8_t spindle_off_when_speed_is_zero;            // 0
  uint8_t software_limit_pin_debouncing;             // S
  uint8_t parking_override_control;                  // R
  uint8_t safety_door_input_pin;                     // +
  uint8_t restore_all_eeprom;                        // *
  uint8_t restore_eeprom;                            // $
  uint8_t restore_eeprom_param_data_command;         // #
  uint8_t build_info_write_user_string_command;      // I
  uint8_t force_sync_upon_eeprom_write;              // E
  uint8_t force_sync_upon_wco_offset_change;         // W
  uint8_t homing_initialization_auto_lock;           // L
  uint8_t dual_axis_motors;                          // 2
  grbl_buffer_state_t buffer_state;
} grbl_msg_build_options_t;

typedef struct grbl_response_token_t {
  grbl_response_token_type type;
  //grbl_response_token_value_t value;
  union {
    grbl_error_code error_code;
    grbl_alarm_code alarm_code;
    grbl_msg_version_t version;
    grbl_msg_machine_state_t machine_state;
    grbl_vec3_t vec3;
    grbl_buffer_state_t buffer_state;
    grbl_msg_current_feed_and_speed_t feed_and_spindle;
    grbl_setting_code setting;
    uint8_t u8;
    uint32_t u32;
    int32_t i32;
    float f32;
    grbl_msg_current_pins_t pins;
    grbl_msg_overrides_t overrides;
    grbl_msg_accessory_state_t accessories;
    char *str;
    grbl_gcode_state gcode_state;
    grbl_probe_state_t probe_state;
    grbl_msg_build_options_t build_options;
  };
} grbl_response_token_t;

typedef struct grbl_parser_t {
  char input[GRBL_PARSER_BUFFER_LEN] = {0};
  grbl_response_token_t *tokens = NULL;
  uint16_t loc=0;
} grbl_parser_t;

grbl_parser_t *grbl_parser_create() {
  grbl_parser_t *parser = (grbl_parser_t *)malloc(sizeof(grbl_parser_t));
  memset(parser, 0, sizeof(grbl_parser_t));
  return parser;
}

void grbl_parser_destroy(grbl_parser_t *parser) {
  if (parser == NULL) {
    return;
  }
  free(parser);
}

char *advance(char *buf, uint32_t *len, uint32_t n) {
  if (*len < n) {
    return NULL;
  }

  (*len) -= n;
  return buf + n;
}

char *read_int(int *acc, char *buf, uint32_t *len) {
  *acc = 0;
  bool negative = false;
  if (buf[0] == '-') {
    negative = true;
    buf = advance(buf, len, 1);
  }
  const uint32_t l = (*len);
  for (uint32_t i = 0; buf[0] != 0 && i < l; i++) {
    char c = buf[0];
    if (c < '0' || c > '9') {
      break;
    }

    (*acc) = ((*acc) * 10) + (c - '0');
    buf = advance(buf, len, 1);
  }

  if (negative) {
    (*acc) = -(*acc);
  }
  return buf;
}

char *read_uint16(uint16_t *acc, char *buf, uint32_t *len) {
  *acc = 0;
  uint32_t i = 0;
  for (i=0;  buf[i] != 0 && i<(*len); i++) {
    char c = buf[i];
    if (c < '0' || c > '9') {
      break;
    }

    (*acc) = ((*acc) * 10) + (buf[i] - '0');
  }

  return advance(buf, len, i);
}

char *read_uint32(uint32_t *acc, char *buf, uint32_t *len) {
  *acc = 0;
  uint32_t i = 0;
  for (i = 0;  buf[i] != 0 && i<(*len); i++) {
    char c = buf[i];
    if (c < '0' || c > '9') {
      break;
    }

    (*acc) = ((*acc) * 10) + (buf[i] - '0');
  }

  return advance(buf, len, i);
}

bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

char *read_float(float *acc, char *buf, uint32_t *len) {
  (*acc) = 0;
  int whole = 0;
  int frac = 0;
  buf = read_int(&whole, buf, len);
  if (buf[0] != '.') {
    (*acc) = (float)whole;
    return buf;
  } else {
    buf = advance(buf, len, 1); // skip the '.'
  }

  char *new_buf = read_int(&frac, buf, len);
  size_t frac_len = new_buf - buf;
  float places = 1.0f;
  for (size_t i = 0; i<frac_len; i++) {
    places *= 10.0f;
  }

  (*acc) = (float)whole + (float)frac / places;

  return new_buf;
}

char *read_vec3(grbl_vec3_t *acc, char *buf, uint32_t *len) {
  // read x
  buf = read_float(&acc->x, buf, len);
  if (!buf || buf[0] != ',') {
    return NULL;
  }
  buf = advance(buf, len, 1); // skip ','

  // read y
  buf = read_float(&acc->y, buf, len);
  if (!buf || buf[0] != ',') {
    return NULL;
  }
  buf = advance(buf, len, 1); // skip ','

  // read z
  return read_float(&acc->z, buf, len);
}

char *read_str(grbl_response_token_t *token, char *buf, uint32_t *len) {
  if (*len == 0) {
    return NULL;
  }

  token->str = (char *)malloc(*len);
  memcpy(token->str, buf, *len - 1);
  token->str[(*len) - 1] = 0;

  return buf;
}

int grbl_parser_tokenize_current_line(grbl_parser_t *parser) {
  uint32_t len = parser->loc;
  char *buf = parser->input;
  printf("buf(%u): %s\n", len, buf);
  if (len == 2 && buf[0] == 'o' && buf[1] == 'k') {
    grbl_response_token_t token;
    token.type = GRBL_TOKEN_TYPE_STATUS;
    token.error_code = GRBL_ERROR_NONE;

    sb_push(parser->tokens, token);
    return GRBL_TRUE;
  }

  if (len > 6 && strstr(buf, "error:") == buf) {
    grbl_response_token_t token;
    token.type = GRBL_TOKEN_TYPE_STATUS;
    token.error_code = (grbl_error_code)atoi(&buf[6]);
    sb_push(parser->tokens, token);
    return GRBL_TRUE;
  }

  if (len > 6 && strstr(buf, "ALARM:") == buf) {
    buf = advance(buf, &len, 6);
    grbl_response_token_t token;
    token.type = GRBL_TOKEN_TYPE_ALARM;
    read_int((int *)&token.alarm_code, buf, &len);
    sb_push(parser->tokens, token);
    return GRBL_TRUE;
  }

  if (len > 8 && strstr(buf, "Grbl ") == buf) {
    grbl_response_token_t welcome_token;
    welcome_token.type = GRBL_TOKEN_TYPE_WELCOME;
    sb_push(parser->tokens, welcome_token);

    buf = advance(buf, &len, 5);

    grbl_response_token_t version_token;
    version_token.type = GRBL_TOKEN_TYPE_VERSION;
    buf = read_uint16(&version_token.version.major, buf, &len);

    // skip the '.'
    buf = advance(buf, &len, 1);

    buf = read_uint16(&version_token.version.minor, buf, &len);
    version_token.version.letter = buf[0];
    buf = advance(buf, &len, 1);

    sb_push(parser->tokens, version_token);

    return GRBL_TRUE;
  }

  // handle machine status report
  if (buf[0] == '<') {
    {
      grbl_response_token_t token;
      token.type = GRBL_TOKEN_TYPE_STATUS_REPORT;
      sb_push(parser->tokens, token);

      buf = advance(buf, &len, 1);
    }

    // parse machine status string
    {
      grbl_response_token_t token;
      token.type = GRBL_TOKEN_TYPE_MACHINE_STATE;

      for (size_t i=0; i<GRBL_MACHINE_STATE_COUNT; i++) {
        const char *state_str = grbl_machine_state_str[i];
        const size_t l = strlen(state_str);

        if (strstr(buf, state_str) == buf) {
          buf = advance(buf, &len, l);
          token.machine_state.value = (grbl_machine_state)i;

          // handle sub codes (currently only Hold:<d> & Door:<d>)
          if (buf[0] == ':') {
            buf = advance(buf, &len, 1);
            buf = read_uint16(&token.machine_state.code, buf, &len);
          } else {
            token.machine_state.code = 0;
          }
          break;
        }
      }
      sb_push(parser->tokens, token);
    }

    while (buf[0] != '>' && buf[0] != 0) {
      // expect all fields to be separated by a |
      if (buf[0] != '|') {
        return GRBL_ERROR;
      }
      buf = advance(buf, &len,  1);

      grbl_response_token_t token;

      // Machine Position
      if (strstr(buf, "MPos:") == buf) {
        buf = advance(buf, &len, 5);
        buf = read_vec3(&token.vec3, buf, &len);
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        token.type = GRBL_TOKEN_TYPE_MACHINE_POSITION;
        sb_push(parser->tokens, token);
        continue;
      }

      // Work Coordinate Position
      if (strstr(buf, "WPos:") == buf) {
        token.type = GRBL_TOKEN_TYPE_WCO_POSITION;
        buf = advance(buf, &len, 5);
        buf = read_vec3(&token.vec3, buf, &len);
        if (buf == NULL) {
          return GRBL_ERROR;
        }
        
        sb_push(parser->tokens, token);
        continue;
      }

      // Work Coordinate Offset
      if (strstr(buf, "WCO:") == buf) {
        token.type = GRBL_TOKEN_TYPE_WCO_OFFSET;
        buf = advance(buf, &len, 4);
        buf = read_vec3(&token.vec3, buf, &len);
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        sb_push(parser->tokens, token);
        continue;
      }

      // Buffer State
      if (strstr(buf, "Bf:") == buf) {
        token.type = GRBL_TOKEN_TYPE_BUFFER_STATE;
        buf = advance(buf, &len, 3);
        
        buf = read_uint32(
          &token.buffer_state.available_blocks,
          buf,
          &len
        );

        if (buf[0] != ',') {
          return GRBL_ERROR;
        }
        buf = advance(buf, &len, 1);
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        buf = read_uint32(
          &token.buffer_state.available_bytes,
          buf,
          &len
        );

        sb_push(parser->tokens, token);
        continue;
      }

      // Line number
      if (strstr(buf, "Ln:") == buf) {
        token.type = GRBL_TOKEN_TYPE_LINE_NUMBER;
        buf = advance(buf, &len, 3);
        buf = read_uint32(&token.u32, buf, &len);
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        sb_push(parser->tokens, token);
        continue;
      }

      // Current Feed (no spindle)
      if (strstr(buf, "F:") == buf) {
        token.type = GRBL_TOKEN_TYPE_CURRENT_FEED_AND_SPINDLE;
        buf = advance(buf, &len, 2);
        buf = read_uint32(&token.feed_and_spindle.feed, buf, &len);
        token.feed_and_spindle.spindle = 0;
        
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        sb_push(parser->tokens, token);
        continue;
      }

      // Current Feed and Spindle
      if (strstr(buf, "FS:") == buf) {
        token.type = GRBL_TOKEN_TYPE_CURRENT_FEED_AND_SPINDLE;
        buf = advance(buf, &len, 3);
        buf = read_uint32(&token.feed_and_spindle.feed, buf, &len);

        if (buf[0] != ',') {
          return GRBL_ERROR;
        }
        buf = advance(buf, &len, 1);
        if (!buf) {
          return GRBL_ERROR;
        }

        buf = read_uint32(&token.feed_and_spindle.spindle, buf, &len);

        if (buf == NULL) {
          return GRBL_ERROR;
        }

        sb_push(parser->tokens, token);
        continue;
      }

      // Input Pin State
      if (strstr(buf, "Pn:") == buf) {
        token.type = GRBL_TOKEN_TYPE_INPUT_PIN_STATE;
        buf = advance(buf, &len, 3);
        if (!buf) {
          return GRBL_ERROR;
        }

        memset(&token.pins, 0, sizeof(grbl_msg_current_pins_t));
        while (buf[0] != 0 && buf[0] != '>' && buf[0] != '|') {
          switch (buf[0]) {
            case 'X': token.pins.limit_x = 1; break;
            case 'Y': token.pins.limit_y = 1; break;
            case 'Z': token.pins.limit_z = 1; break;
            case 'A': token.pins.limit_a = 1; break;
            case 'P': token.pins.probe = 1; break;
            case 'D': token.pins.door = 1; break;
            case 'H': token.pins.hold = 1; break;
            case 'R': token.pins.soft_reset = 1; break;
            case 'S': token.pins.cycle_start = 1; break;
            default:
              printf("WARN: unknown pin state %c\n", buf[0]);
          }
          buf = advance(buf, &len, 1);
          if (!buf) {
            return GRBL_ERROR;
          }
        }
        sb_push(parser->tokens, token);
        continue;
      }

      // Overrides
      if (strstr(buf, "Ov:") == buf) {
        token.type = GRBL_TOKEN_TYPE_OVERRIDES;
        buf = advance(buf, &len, 3);
        grbl_vec3_t v = {0};
        buf = read_vec3(&v, buf, &len);
        // turn these into actual percentages
        token.overrides.feed = v.x / 100.0f;
        token.overrides.rapids = v.y / 100.0f;
        token.overrides.spindle = v.z / 100.0f;
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        sb_push(parser->tokens, token);
        continue;
      }

      // Accessories
      if (strstr(buf, "A:") == buf) {
        token.type = GRBL_TOKEN_TYPE_ACCESSORY_STATE;
        buf = advance(buf, &len, 2);
        memset(&token.accessories, 0, sizeof(grbl_msg_accessory_state_t));
        while (buf[0] != 0 && buf[0] != '>' && buf[0] != '|') {
          switch (buf[0]) {
            case 'S': token.accessories.spindle = GRBL_SPINDLE_DIRECTION_CW; break;
            case 'C': token.accessories.spindle = GRBL_SPINDLE_DIRECTION_CCW; break;
            case 'F': token.accessories.flood_coolant = 1; break;
            case 'M': token.accessories.mist_coolant = 1; break;
            default:
              printf("WARN: unknown accessory: %c\n", buf[0]);
          }
          buf = advance(buf, &len, 1);
          if (!buf) {
            return GRBL_ERROR;
          }
        }
        sb_push(parser->tokens, token);
        continue;

      }
    }
    return GRBL_TRUE;
  }

  // handle settings, this generates two tokens: a key and value
  if (buf[0] == '$') {
    // read the key
    grbl_setting_code key;
    {
      grbl_response_token_t token;
      token.type = GRBL_TOKEN_TYPE_SETTING_KEY;
      buf = advance(buf, &len, 1);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_int((int*)&key, buf, &len);
      token.setting = (grbl_setting_code)key;
      if (buf == NULL) {
        return GRBL_ERROR;
      }
      sb_push(parser->tokens, token);
    }

    if (buf[0] != '=') {
      return GRBL_ERROR;
    }
    buf = advance(buf, &len, 1);
    if (buf == NULL) {
      return GRBL_ERROR;
    }

    // read the value
    {
      grbl_response_token_t token;
      token.type = GRBL_TOKEN_TYPE_SETTING_VALUE;
      
      switch (key) {
        // mask values
        case GRBL_SETTING_INVERT_STEP_PULSE_MASK:
        case GRBL_SETTING_INVERT_STEP_DIRECTION_MASK:
        case GRBL_SETTING_STATUS_REPORT_OPTIONS_MASK:
        case GRBL_SETTING_INVERT_HOMING_DIRECTION_MASK:
          {
            uint32_t v = 0;
            buf = read_uint32(&v, buf, &len);
            if (buf == NULL) {
              return GRBL_ERROR;
            }
            token.u8 = v & 0xFF;
          }
        break;
        
        // boolean values
        case GRBL_SETTING_INVERT_STEP_ENABLE_PIN:
        case GRBL_SETTING_INVERT_LIMIT_PINS:
        case GRBL_SETTING_INVERT_PROBE_PIN:
        case GRBL_SETTING_REPORT_IN_INCHES:
        case GRBL_SETTING_SOFT_LIMITS_ENABLE:
        case GRBL_SETTING_HARD_LIMITS_ENABLE:
        case GRBL_SETTING_HOMING_CYCLE_ENABLE:
        case GRBL_SETTING_LASER_MODE_ENABLE:
          {
            uint32_t v = 0;
            buf = read_uint32(&v, buf, &len);
            if (buf == NULL) {
              return GRBL_ERROR;
            }
            token.u8 = v & 0xFF;
          }
        break;

        // u32 values
        case GRBL_SETTING_STEP_PULSE_TIME:
        case GRBL_SETTING_STEP_IDLE_DELAY:
        case GRBL_SETTING_HOMING_SWITCH_DEBOUNCE_DELAY:
        case GRBL_SETTING_MAXIMUM_SPINDLE_SPEED:
        case GRBL_SETTING_MINIMUM_SPINDLE_SPEED:
          {
            buf = read_uint32(&token.u32, buf, &len);
            if (buf == NULL) {
              return GRBL_ERROR;
            }
          }
        break;

        // scalar values
        case GRBL_SETTING_JUCTION_DEVIATION:
        case GRBL_SETTING_ARC_TOLERANCE:
        case GRBL_SETTING_HOMING_LOCATE_FEED_RATE:
        case GRBL_SETTING_HOMING_SEARCH_SEEK_RATE:
        case GRBL_SETTING_HOMING_SWITCH_PULL_OFF_DISTANCE:
        case GRBL_SETTING_X_AXIS_TRAVEL_RESOLUTION:
        case GRBL_SETTING_Y_AXIS_TRAVEL_RESOLUTION:
        case GRBL_SETTING_Z_AXIS_TRAVEL_RESOLUTION:
        case GRBL_SETTING_X_AXIS_MAXIMUM_RATE:
        case GRBL_SETTING_Y_AXIS_MAXIMUM_RATE:
        case GRBL_SETTING_Z_AXIS_MAXIMUM_RATE:
        case GRBL_SETTING_X_AXIS_ACCELERATION:
        case GRBL_SETTING_Y_AXIS_ACCELERATION:
        case GRBL_SETTING_Z_AXIS_ACCELERATION:
        case GRBL_SETTING_X_AXIS_MAXIMUM_TRAVEL:
        case GRBL_SETTING_Y_AXIS_MAXIMUM_TRAVEL:
        case GRBL_SETTING_Z_AXIS_MAXIMUM_TRAVEL:
          {
            buf = read_float(&token.f32, buf, &len);
            if (buf == NULL) {
              return GRBL_ERROR;
            }
          }
        break;

        default:
          printf("WARN: invalid setting %u", (uint32_t)key);
          return GRBL_ERROR;
      }
      sb_push(parser->tokens, token);
      return GRBL_TRUE;
    }
  }

  // handle messages
  if (buf[0] == '[') {
    grbl_response_token_t token;
    
    buf = advance(buf, &len, 1);
    if (buf == NULL) {
      return GRBL_ERROR;
    }

    if (len > 4 && strstr(buf, "MSG:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_MSG;
      buf = advance(buf, &len, 4);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_str(&token, buf, &len);
    }

    if (len > 4 && strstr(buf, "HLP:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_HLP;
      buf = advance(buf, &len, 4);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_str(&token, buf, &len);
    }

    if (len > 4 && strstr(buf, "VER:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_VER;
      buf = advance(buf, &len, 4);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_str(&token, buf, &len);
    }

    if (len > 4 && strstr(buf, "OPT:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_OPT;
      buf = advance(buf, &len, 4);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      memset(&token.build_options, 0, sizeof(grbl_msg_build_options_t));
      // read the build options
      while(buf[0] != ',') {
        char c = buf[0];

        switch (c) {
          case 'V': token.build_options.variable_spindle = 1; break;
          case 'N': token.build_options.line_numbers = 1; break;
          case 'M': token.build_options.mist_coolant = 1; break;
          case 'C': token.build_options.core_xy = 1; break;
          case 'P': token.build_options.parking_motion = 1; break;
          case 'Z': token.build_options.homing_force_origin = 1; break;
          case 'H': token.build_options.homing_single_axis = 1; break;
          case 'T': token.build_options.two_limit_switches_on_axis = 1; break;
          case 'A': token.build_options.allow_feed_rate_overrides_in_probe_cycles = 1; break;
          case 'D': token.build_options.use_spindle_direction_as_enable_pin = 1; break;
          case '0': token.build_options.spindle_off_when_speed_is_zero = 1; break;
          case 'S': token.build_options.software_limit_pin_debouncing = 1; break;
          case 'R': token.build_options.parking_override_control = 1; break;
          case '+': token.build_options.safety_door_input_pin = 1; break;
          case '*': token.build_options.restore_all_eeprom = 1; break;
          case '$': token.build_options.restore_eeprom = 1; break;
          case '#': token.build_options.restore_eeprom_param_data_command = 1; break;
          case 'I': token.build_options.build_info_write_user_string_command = 1; break;
          case 'E': token.build_options.force_sync_upon_eeprom_write = 1; break;
          case 'W': token.build_options.force_sync_upon_wco_offset_change = 1; break;
          case 'L': token.build_options.homing_initialization_auto_lock = 1; break;
          case '2': token.build_options.dual_axis_motors = 1; break;
          default:
            printf("WARN: unknown build option (%c)", c);
            return GRBL_ERROR;
        }
        buf = advance(buf, &len, 1);
        if (buf == NULL) {
          return GRBL_ERROR;
        }
      }

      // read past the :
      buf = advance(buf, &len, 1);
      if (buf == NULL) {
        return GRBL_ERROR;
      }
      
      buf = read_uint32(&token.build_options.buffer_state.available_blocks, buf, &len);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      // read past the ,
      if (buf[0] != ',') {
        printf("WARN: expected , between available_blocks and available_bytes\n");
        return GRBL_ERROR;
      }
      buf = advance(buf, &len, 1);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_uint32(&token.build_options.buffer_state.available_bytes, buf, &len);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      if (buf[0] != ']') {
        printf("WARN: build option includes unparsed data (%s)\n", buf);
        return GRBL_ERROR;
      }
    }

    if (len > 3 && strstr(buf, "GC:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_GC;
      buf = advance(buf, &len, 3);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      grbl_gcode_state *state = &token.gcode_state;

      while(buf && buf[0] != ']' && buf[0] != 0) {
        char c = buf[0];
        buf = advance(buf, &len, 1);
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        if (c == ' ') {
          continue;
        }

        uint32_t whole = 0;
        uint32_t frac = 0;
        buf = read_uint32(&whole, buf, &len);
        if (buf == NULL) {
          return GRBL_ERROR;
        }

        if (buf[0] == '.') {
          buf = advance(buf, &len, 1);
          if (buf == NULL) {
            return GRBL_ERROR;
          }

          buf = read_uint32(&frac, buf, &len);
        }

        if (c == 'G') {
          switch (whole) {
            case 0: state->motion_mode = G0; break;
            case 1: state->motion_mode = G1; break;
            case 2: state->motion_mode = G0; break;
            case 3: state->motion_mode = G0; break;
            case 17: state->plane = G17; break;
            case 18: state->plane = G18; break;
            case 19: state->plane = G19; break;
            case 20: state->units = G20; break;
            case 21: state->units = G21; break;
            case 54: state->coordinate_system = G54; break;
            case 55: state->coordinate_system = G55; break;
            case 56: state->coordinate_system = G56; break;
            case 57: state->coordinate_system = G57; break;
            case 58: state->coordinate_system = G58; break;
            case 59: state->coordinate_system = G59; break;
            case 38:
              switch (frac) {
                case 2: state->motion_mode = G38_2; break;
                case 3: state->motion_mode = G38_3; break;
                case 4: state->motion_mode = G38_4; break;
                case 5: state->motion_mode = G38_5; break;
                default:
                  printf("WARN: invalid G38.x (%u)\n", frac);
                  return GRBL_ERROR;
              }
            case 80: state->motion_mode = G80; break;
            case 90: state->distance_mode = G90; break;
            case 91: state->distance_mode = G91; break;
            case 93: state->feed_rate_mode = G93; break;
            case 94: state->feed_rate_mode = G94; break;
            default:
              printf("WARN: invalid G state in GC: message (%u)\n", whole);
              return GRBL_ERROR;
          }
          continue;
        }
        
        if (c == 'M') {
          switch (whole) {
            case 0: state->program_mode = M0; break;
            case 1: state->program_mode = M1; break;
            case 2: state->program_mode = M2; break;
            case 3: state->spindle_state = M3; break;
            case 4: state->spindle_state = M4; break;
            case 5: state->spindle_state = M5; break;
            case 7: state->coolant_state = M7; break;
            case 8: state->coolant_state = M8; break;
            case 9: state->coolant_state = M9; break;
            case 30: state->program_mode = M30; break;
            default:
              printf("WARN: invalid M state in GC: message (%u)\n", whole);
              return GRBL_ERROR;
          }
          continue;
        }

        if (c == 'T') {
          state->tool_index = whole;
          continue;
        }

        if (c == 'S') {
          state->spindle_rpm = whole;
          continue;
        }

        if (c == 'F') {
          state->feed = whole;
          continue;
        }

        return GRBL_ERROR;
      }
    }

    if (len > 4 && buf[0] == 'G') {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_MSG;
      buf = advance(buf, &len, 1);
      if (buf == NULL) {
        return GRBL_ERROR;
      }
      uint32_t whole = 0;
      buf = read_uint32(&whole, buf, &len);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      switch (whole) {
        case 28: token.type = GRBL_TOKEN_TYPE_MESSAGE_POS_G28; break;
        case 30: token.type = GRBL_TOKEN_TYPE_MESSAGE_POS_G30; break;
        case 54: token.type = GRBL_TOKEN_TYPE_MESSAGE_WCO_G54; break;
        case 55: token.type = GRBL_TOKEN_TYPE_MESSAGE_WCO_G55; break;
        case 56: token.type = GRBL_TOKEN_TYPE_MESSAGE_WCO_G56; break;
        case 57: token.type = GRBL_TOKEN_TYPE_MESSAGE_WCO_G57; break;
        case 58: token.type = GRBL_TOKEN_TYPE_MESSAGE_WCO_G58; break;
        case 59: token.type = GRBL_TOKEN_TYPE_MESSAGE_WCO_G59; break;
        case 92: token.type = GRBL_TOKEN_TYPE_MESSAGE_CSO_G92; break;
        default:
          printf("WARN: unknown message (G%u)\n", whole);
          return GRBL_ERROR;
      }

      if (buf[0] != ':') {
        return GRBL_ERROR;
      }
      buf = advance(buf, &len, 1);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_vec3(&token.vec3, buf, &len);
      if (buf == NULL) {
        return GRBL_ERROR;
      }
    }

    if (len > 4 && strstr(buf, "TLO:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_TLO;
      buf = advance(buf, &len, 4);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_float(&token.f32, buf, &len);
    }

    if (len > 4 && strstr(buf, "PRB:") == buf) {
      token.type = GRBL_TOKEN_TYPE_MESSAGE_PRB;
      buf = advance(buf, &len, 4);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_vec3(&token.probe_state.vec3, buf, &len);

      if (buf[0] != ':') {
        printf("WARN: expected probe status\n");
        return GRBL_ERROR;
      }

      buf = advance(buf, &len, 1);
      if (buf == NULL) {
        return GRBL_ERROR;
      }

      buf = read_uint32(&token.probe_state.success, buf, &len);
      if (buf == NULL) {
        return GRBL_ERROR;
      }
    }

    sb_push(parser->tokens, token);
  }

  return GRBL_FALSE;
}

// returns true on newline because at that point we should have a valid
// parser->response
int grbl_parser_input(grbl_parser_t *parser, const char c) {
  // ignore
  if (c == '\r') {
    return GRBL_FALSE;
  }

  if (c == '\n') {
    uint16_t len = GRBL_PARSER_BUFFER_LEN > parser->loc
    ? parser->loc
    : GRBL_PARSER_BUFFER_LEN;

    int ret = GRBL_FALSE;
    parser->input[len] = 0;
    if (parser->loc > 0) {
      ret = grbl_parser_tokenize_current_line(parser);
    }

    // the tokenizer had a chance to copy the line over, now we need to reset
    // and prepare for future input.
    parser->loc = 0;
    parser->input[0] = 0;
    return ret;
  }

  if (parser->loc + 1 == GRBL_PARSER_BUFFER_LEN) {
    printf("parser overflow\n");
    return GRBL_ERROR;
  }

  parser->input[parser->loc++] = c;
  return GRBL_FALSE;
}

struct GrblParser {
  grbl_parser_t *handle = NULL;
  ~GrblParser() {
    if (this->handle != NULL) {
      grbl_parser_destroy(this->handle);
    }
  }

  int read(const char c) {
    if (this->handle == NULL) {
      this->handle = grbl_parser_create();
    }
    return grbl_parser_input(this->handle, c);
  }

  int read(const char *str) {
    const size_t l = strlen(str);
    for (size_t i=0; i<l; i++) {
      if (this->read(str[i]) == GRBL_ERROR) {
        return GRBL_ERROR;
      }
    }
    return GRBL_TRUE;
  }

};

