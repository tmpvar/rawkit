#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct rawkit_jit_t rawkit_jit_t;

#ifdef __cplusplus
extern "C" {
#endif

enum rawkit_jit_status {
  RAWKIT_JIT_STATUS_ERR = -1,
  RAWKIT_JIT_STATUS_OK = 0,
};

rawkit_jit_t *rawkit_jit_create(const char *file);
rawkit_jit_status rawkit_jit_get_status(rawkit_jit_t *jit);

void _rawkit_jit_destroy(rawkit_jit_t **jit);
#define rawkit_jit_destroy(jit) _rawkit_jit_destroy(&jit)

const char *rawkit_jit_program_path(rawkit_jit_t *jit);

void _rawkit_jit_add_export(rawkit_jit_t *jit, const char *name, void *address);
#define rawkit_jit_add_export(jit, name, address) _rawkit_jit_add_export(jit, name, (void *)address)

void rawkit_jit_add_define(rawkit_jit_t *jit, const char *value);

enum rawkit_jit_tick_status {
  RAWKIT_JIT_TICK_INVALID = 0,
  RAWKIT_JIT_TICK_ERROR = 1,
  RAWKIT_JIT_TICK_BUILT = 2,
  RAWKIT_JIT_TICK_CLEAN = 3,
};

rawkit_jit_tick_status rawkit_jit_tick(rawkit_jit_t *jit);

enum rawkit_jit_message_level {
  RAWKIT_JIT_MESSAGE_LEVEL_NONE = 0,
  RAWKIT_JIT_MESSAGE_LEVEL_NOTE,
  RAWKIT_JIT_MESSAGE_LEVEL_WARNING,
  RAWKIT_JIT_MESSAGE_LEVEL_REMARK,
  RAWKIT_JIT_MESSAGE_LEVEL_ERROR,
  RAWKIT_JIT_MESSAGE_LEVEL_FATAL,
};

typedef struct rawkit_jit_message_t {
  rawkit_jit_message_level level;
  const char *filename;
  uint32_t line;
  uint32_t column;
  const char *str;
} rawkit_jit_message_t;

bool rawkit_jit_get_message(const rawkit_jit_t *jit, uint32_t index, rawkit_jit_message_t *msg);


void rawkit_jit_call_setup(rawkit_jit_t *jit);
void rawkit_jit_call_loop(rawkit_jit_t *jit);

uint64_t rawkit_jit_version(const rawkit_jit_t *jit);

typedef void(*rawkit_teardown_fn_t)(void *user_data);

void rawkit_set_default_jit(rawkit_jit_t *jit);
rawkit_jit_t *rawkit_default_jit();

#define rawkit_teardown_fn(data, fn) rawkit_teardown_fn_ex(rawkit_default_jit(), data, fn)
void rawkit_teardown_fn_ex(rawkit_jit_t *jit, void *user_data, rawkit_teardown_fn_t fn);

void rawkit_jit_set_debug(rawkit_jit_t *jit, bool v);

uint32_t rawkit_jit_get_version(rawkit_jit_t *jit);

#ifdef __cplusplus
}
#endif
