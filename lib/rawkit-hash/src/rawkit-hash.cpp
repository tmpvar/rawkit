#include <string.h>

#include <rawkit/core.h>
#include <rawkit/hash.h>

#include "meow_hash.h"

uint64_t rawkit_hash(uint64_t len, void *data) {
  if (!len || !data) {
    return 0;
  }

  meow_u128 hash = MeowHash(MeowDefaultSeed, len, data);
  return MeowU64From(hash, 0);
}

uint64_t rawkit_hash_resources(const char *name, uint32_t resource_count, const rawkit_resource_t **resources) {
  if (!name || !resource_count || !resources) {
    return 0;
  }

  meow_state state;
  MeowBegin(&state, MeowDefaultSeed);

  const char *null_str = "<null>";
  uint32_t null_len = 6;

  uint64_t name_len = strlen(name);
  MeowAbsorb(&state, name_len, (void *)name);

  for (uint32_t i=0; i<resource_count; i++) {
    const rawkit_resource_t *res = resources[i];
    if (!res) {
      MeowAbsorb(&state, null_len, (void *)null_str);
      continue;
    }

    MeowAbsorb(&state, sizeof(res->resource_id), (void *)&res->resource_id);
  }

  meow_u128 hash = MeowEnd(&state, NULL);
  return MeowU64From(hash, 0);
}

uint64_t rawkit_hash_composite(uint64_t len, uint64_t *ids) {
  if (!len || !ids) {
    return 0;
  }

  meow_state state;
  MeowBegin(&state, MeowDefaultSeed);
  for (uint32_t i=0; i<len; i++) {
    MeowAbsorb(&state, sizeof(uint64_t), (void *)&ids[i]);
  }
  meow_u128 hash = MeowEnd(&state, NULL);
  return MeowU64From(hash, 0);
}


typedef struct ps_rawkit_hasher_t {
  PS_FIELDS

  meow_state state;
  meow_u128 hash;
} ps_rawkit_hasher_t;

ps_val_t *hasher_fn(ps_t *base, ps_stream_status status) {
  if (!base) {
    return NULL;
  }

  if (ps_status(base, status)) {
    return NULL;
  }

  ps_val_t *val = ps_pull(base, PS_OK);
  ps_rawkit_hasher_t *s = (ps_rawkit_hasher_t *)base;

  if (val) {
    uint8_t *bytes = (uint8_t *)val->data;
    const uint64_t chunk_size = sizeof(s->state.Buffer);

    for (uint64_t i=0; i<val->len; i+=chunk_size) {
      uint64_t len = val->len - i;
      if (len > chunk_size) {
        len = chunk_size;
      }

      MeowAbsorb(
        &s->state,
        len,
        // TODO: will MeowAbsorb mutate this value???
        (void *)&bytes[i]
      );
    }
  }

  if (base->status == PS_DONE) {
    s->hash = MeowEnd(&s->state, NULL);
  }

  return val;
}

ps_t *ps_rawkit_hasher() {
  ps_rawkit_hasher_t *s = ps_create_stream(ps_rawkit_hasher_t, NULL);
  if (!s) {
    return NULL;
  }

  MeowBegin(&s->state, MeowDefaultSeed);
  s->fn = hasher_fn;
  return (ps_t *)s;
}

uint64_t ps_rawkit_get_hash64(ps_t *base) {
  if (!base) {
    return 0;
  }

  ps_rawkit_hasher_t *s = (ps_rawkit_hasher_t *)base;
  return MeowU64From(s->hash, 0);
}