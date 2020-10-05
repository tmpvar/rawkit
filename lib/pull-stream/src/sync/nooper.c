#include <pull/stream.h>

#include <stdlib.h>

ps_t *create_nooper() {
  ps_t *cb = ps_create_stream(ps_t, NULL);

  cb->fn = ps_pull;

  return cb;
}
