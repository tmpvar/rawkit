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
  #define gcode_debug (void)(...)
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

// TODO: add a bitmask and sub-struct / array of floats for adding fields

typedef struct gcode_line_t {
  uint64_t start_loc;
  uint64_t end_loc;
  uint32_t words;
  gcode_line_type type;
  float code;
  gcode_word_pair_t *pairs;
} gcode_line_t;

typedef struct gcode_parser_t {
  gcode_line_t *lines = NULL;
  char pending_buf[GCODE_PARSER_BUFFER_LEN] = {0};
  uint16_t pending_loc = 0;
  int64_t total_loc = 0;
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

gcode_parse_result gcode_parser_new_line(gcode_parser_t *parser) {
  gcode_line_t line;
  line.type = GCODE_LINE_TYPE_EOF;
  line.pairs = NULL;
  line.start_loc = parser->total_loc;
  line.end_loc = -1;
  stb_sb_push(parser->lines, line);
  return GCODE_RESULT_TRUE;
}

inline bool gcode_is_numeric(const char c) {
  if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
    return true;
  }
  return false;
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
    return GCODE_RESULT_ERROR;
  }

  parser->pending_buf[parser->pending_loc] = 0;
  pair.value = atof(parser->pending_buf + 1);
  gcode_line_t *line = &stb_sb_last(parser->lines);

  uint32_t mask = 1<<(int)word;
  if (line->words & mask) {
    printf("ERROR: duplicate word '%c' found\n", pair.letter);
    return GCODE_RESULT_ERROR;
  }

  stb_sb_push(line->pairs, pair);

  // extract the enumeration value of the letter in the pair
  switch (pair.letter) {
    case 'G':
      if (line->type != GCODE_LINE_TYPE_EOF) {
        printf("ERROR: multiple overlapping command words were specified.\n");
        return GCODE_RESULT_ERROR;
      }
      line->type = GCODE_LINE_TYPE_G;
      break;
    case 'M':
      if (line->type != GCODE_LINE_TYPE_EOF) {
        printf("ERROR: multiple overlapping command words were specified.\n");
        return GCODE_RESULT_ERROR;
      }

      line->type = GCODE_LINE_TYPE_M;
      break;
  }

  // ensure the line is no longer marked as EOF
  if (line->type == GCODE_LINE_TYPE_EOF) {

  }

  return GCODE_RESULT_TRUE;
}

gcode_parse_result gcode_parser_input(gcode_parser_t *parser, char c) {
  parser->total_loc++;

  if (parser->lines == NULL) {
    if (gcode_parser_new_line(parser) != GCODE_RESULT_TRUE) {
      printf("ERROR: gcode_parser_new_line failed\n");
      return GCODE_RESULT_ERROR;
    }
  }

  gcode_line_t *current_line = &parser->lines[stb_sb_count(parser->lines) - 1];

  if (is_whitespace(c)) {
    if (c == '\n') {
      current_line->end_loc = parser->total_loc - 1;
      bool skip_pairs = (
        current_line->type == GCODE_LINE_TYPE_COMMENT ||
        current_line->type == GCODE_LINE_TYPE_REMOVE_BLOCK
      );

      if (!skip_pairs && gcode_parser_line_add_pending_pair(parser) != GCODE_RESULT_TRUE) {
        printf("ERROR: failed to add a pending pair\n");
        return GCODE_RESULT_ERROR;
      }
      return gcode_parser_new_line(parser);
    }
    return GCODE_RESULT_TRUE;
  }

  if (parser->pending_loc >= GCODE_PARSER_BUFFER_LEN - 1) {
    printf("ERROR: overran pending buffer\n");
    return GCODE_RESULT_ERROR;
  }
  
  if (current_line->type == GCODE_LINE_TYPE_COMMENT) {
    return GCODE_RESULT_TRUE;
  }

  if (current_line->type == GCODE_LINE_TYPE_REMOVE_BLOCK) {
    return GCODE_RESULT_TRUE;
  }

  // upper case all letters
  if (c >= 'a' && c <= 'z') {
    c -= 32;
  }

  // collect digits up to the next letter
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

    // expect this to be a letter
    if (c < 'A' && c > 'Z') {
      printf("ERROR: expect first char to be a letter\n");
      return GCODE_RESULT_ERROR;
    }
    parser->pending_buf[parser->pending_loc++] = c;
    return GCODE_RESULT_TRUE;
  }

  if (c >= 'A' && c <= 'Z') {
    if (gcode_parser_line_add_pending_pair(parser) != GCODE_RESULT_TRUE) {
      printf("ERROR: adding a pending pair failed\n");
      return GCODE_RESULT_ERROR;
    }

    // reset the pending buffer and add the current letter
    parser->pending_buf[0] = c;
    parser->pending_loc = 1;
  }

  if (!gcode_is_numeric(c)) {
    printf("ERROR: expect characters after first to be numeric\n");
    return GCODE_RESULT_ERROR;
  }

  parser->pending_buf[parser->pending_loc++] = c;
  return GCODE_RESULT_TRUE;
}

class GCODEParser {
  public:
    gcode_parser_t *handle = nullptr;
    GCODEParser() {
      this->handle = (gcode_parser_t *)malloc(sizeof(gcode_parser_t));
      memset(this->handle, 0, sizeof(gcode_parser_t));
      this->handle->total_loc = -1;
    }
    ~GCODEParser() {
      if (this->handle != nullptr) {
        free(this->handle);
      }
    }

    gcode_parse_result push(char c) {
      return gcode_parser_input(this->handle, c);
    }

    gcode_parse_result push(char *str) {
      for (size_t i = 0; i<strlen(str); i++) {
        gcode_parse_result r = gcode_parser_input(this->handle, str[i]);
        if (r != GCODE_RESULT_TRUE) {
          return r;
        }
      }
      return GCODE_RESULT_TRUE;
    }

    gcode_line_t *line(const uint64_t number) {
      if (stb_sb_count(this->handle->lines) <= number) {
        return nullptr;
      }

      return &this->handle->lines[number];
    }
};