#pragma once


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

void rawkit_jit_add_export(rawkit_jit_t *jit, const char *name, void *address);
bool rawkit_jit_tick(rawkit_jit_t *jit);

void rawkit_jit_call_setup(rawkit_jit_t *jit);
void rawkit_jit_call_loop(rawkit_jit_t *jit);

#ifdef __cplusplus
}
#endif
