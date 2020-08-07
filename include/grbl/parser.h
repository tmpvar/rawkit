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
  GRBL_ALARM_HOMING_FAIL_PULLOFF,
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
    uint32_t u32;
    grbl_msg_current_pins_t pins;
    grbl_msg_overrides_t overrides;
    grbl_msg_accessory_state_t accessories;
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
  for (uint32_t i = 0; buf[0] != 0, i < l; i++) {
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
  for (i;  buf[i] != 0, i<(*len); i++) {
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
  for (i;  buf[i] != 0, i<(*len); i++) {
    char c = buf[i];
    if (c < '0' || c > '9') {
      break;
    }

    (*acc) = ((*acc) * 10) + (buf[i] - '0');
  }

  return advance(buf, len, i);
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

  (*acc) = (float)whole + (float)frac / pow(10, frac_len);

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
  GrblParser() {
    this->handle = grbl_parser_create();
  }
  ~GrblParser() {
    grbl_parser_destroy(this->handle);
  }
  int read(const char c) {
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

