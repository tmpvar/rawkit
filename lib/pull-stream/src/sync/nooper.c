#include <pull/stream.h>

#include <stdlib.h>

ps_cb_t *create_nooper() {
  ps_cb_t *cb = (ps_cb_t *)calloc(sizeof(ps_cb_t), 1);
  cb->fn = pull_through;
  return cb;
}
