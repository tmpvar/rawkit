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
  GRBL_TOKEN_TYPE_WELCOME,
  GRBL_TOKEN_TYPE_VERSION,
};

typedef struct grbl_version_t {
  uint16_t major;
  uint16_t minor;
  char letter;
} grbl_version_t;

union grbl_response_token_value_t {
  uint8_t error_code;
  grbl_version_t version;
};

typedef struct grbl_response_token_t {
  grbl_response_token_type type;
  //grbl_response_token_value_t value;
  union {
    uint8_t error_code;
    grbl_version_t version;
  };
} grbl_response_token_t;

typedef struct grbl_parser_t {
  char input[GRBL_PARSER_BUFFER_LEN] = {0};
  grbl_response_token_t *tokens = NULL;
  uint16_t loc=0;
} grbl_parser_t;

// grbl_response_token_t *grbl_response_token_create(grbl_response_token_type type) {
//   grbl_response_token_t *token = malloc(sizeof(grbl_response_token));
//   token->type = type;
//   return token; 
// }

// void grbl_response_token_destroy(grbl_response_token_t *token) {
//   if (token != NULL) {
//     free(token);
//   }
// }

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

char *read_int(int *acc, char *buf, uint32_t n) {
  *acc = 0;
  bool negative = false;
  if (buf[0] == '-') {
    negative = true;
  }

  for (uint32_t i = 0; buf[0] != 0, i<n; i++) {
    char c = buf[i];
    if (c < '0' || c > '9') {
      break;
    }

    (*acc) = ((*acc) * 10) + (buf[i] - '0');
    buf++;
  }

  if (negative) {
    (*acc) = -(*acc);
  }
  return buf;
}

char *read_uint16(uint16_t *acc, char *buf, uint32_t n) {
  *acc = 0;
  uint32_t i = 0;
  for (i;  buf[i] != 0, i<n; i++) {
    char c = buf[i];
    if (c < '0' || c > '9') {
      break;
    }

    (*acc) = ((*acc) * 10) + (buf[i] - '0');
  }

  return buf + i;
}

char *advance(char *buf, uint32_t *len, uint32_t n) {
  if (*len < n) {
    return NULL;
  }

  (*len) -= n;
  return buf + n;  
}

int grbl_parser_tokenize_current_line(grbl_parser_t *parser) {
  uint32_t len = parser->loc;
  char *buf = parser->input;
  printf("buf(%u): %s\n", len, buf);
  if (len == 2 && buf[0] == 'o' && buf[1] == 'k') {
    grbl_response_token_t token;
    token.type = GRBL_TOKEN_TYPE_STATUS;
    token.error_code = 0;

    sb_push(parser->tokens, token);
    return GRBL_TRUE;
  }
  

  if (len > 6 && strstr(buf, "error:") == buf) {
    uint8_t error_code = atoi(&buf[6]);
    grbl_response_token_t token;
    token.type = GRBL_TOKEN_TYPE_STATUS;
    token.error_code = error_code;
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
    buf = read_uint16(&version_token.version.major, buf, len);

    buf = advance(buf, &len, 1);

    buf = read_uint16(&version_token.version.minor, buf, len);
    version_token.version.letter = buf[0];
    buf = advance(buf, &len, 1);

    sb_push(parser->tokens, version_token);

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
    uint16_t len = GRBL_PARSER_BUFFER_LEN-1 > parser->loc
    ? parser->loc
    : GRBL_PARSER_BUFFER_LEN-1;

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

};

