#include <pull/stream.h>

#include <stdlib.h>

ps_t *create_nooper() {
  ps_t *cb = (ps_t *)calloc(sizeof(ps_t), 1);
  cb->fn = pull_through;
  return cb;
}
