#include <pull/stream.h>

#include <stdbool.h>
#include <string.h>

typedef struct {
  const uint8_t *bytes;
  const uint64_t len;
} pattern_t;

typedef struct {
  PS_FIELDS

  pattern_t pattern;
  ps_val_t *tmp;
  uint64_t tmp_loc;
  uint64_t loc;
} splitter_t;

static ps_val_t* emit_value(splitter_t *s, uint64_t len) {
  ps_val_t* val = ps_create_value(ps_val_t, NULL);
  if (val == NULL) {
    ps_status(s, PS_ERR);
    return NULL;
  }

  uint8_t *data = s->tmp->data;

  if (s->tmp_loc + len > s->tmp->len) {
    ps_status(s, PS_ERR);
    return NULL;
  }

  val->len = len;
  val->data = calloc(val->len + 1, 1);
  if (val->data == NULL) {
    ps_status(s, PS_ERR);
    return NULL;
  }

  memcpy(val->data, &data[s->tmp_loc], val->len);
  return val;
}

static ps_val_t *splitter_fn(ps_t *base, ps_stream_status status) {
  splitter_t *s = (splitter_t *)base;
  ps_stream_status computed_status = ps_status(base, status);
  if (computed_status == PS_ERR) {
    ps_destroy(s->tmp);
    return NULL;
  }

  if (computed_status && !s->tmp) {
    return NULL;
  }

  // force OK until s->tmp has drained
  s->status = PS_OK;

  bool pulled = false;
  if (!s->tmp) {
    s->tmp = ps__pull_from_source(base, status);
    pulled = true;
    if (!s->tmp) {
      return NULL;
    }
  }

  uint8_t *data = (uint8_t *)s->tmp->data;

  for (; s->loc < s->tmp->len; s->loc++) {
    if (data[s->loc] == s->pattern.bytes[0]) {
      bool match = true;
      uint64_t pattern_loc = 1;
      for (; s->loc + pattern_loc < s->tmp->len && pattern_loc < s->pattern.len; pattern_loc++) {
        if (data[s->loc + pattern_loc] != s->pattern.bytes[pattern_loc]) {
          match = false;
          break;
        }
      }

      if (match) {
        ps_val_t *val = emit_value(s, s->loc - s->tmp_loc);

        s->tmp_loc = s->loc + pattern_loc;
        if (s->tmp_loc == s->tmp->len) {
          ps_destroy(s->tmp);
          s->tmp = NULL;
          s->tmp_loc = 0;
        }
        s->loc = s->tmp_loc;
        return val;
      }
    }
  }

  if (computed_status == PS_DONE) {
    ps_val_t* val = emit_value(s, s->tmp->len - s->tmp_loc);
    ps_destroy(s->tmp);
    s->tmp = NULL;
    s->tmp_loc = 0;
    s->loc = 0;
    return val;
  }

  // if we've reached the end and tmp still has a value
  if (s->tmp && !pulled) {
    ps_val_t *val = ps__pull_from_source(base, status);
    if (!val) {
      // dequeue the rest of s->tmp as are are effectively EOF
      if (base->status == PS_DONE) {
        // ensure status checks on this stream respond with OK until the next pull
        base->status = PS_OK;
        ps_val_t *val = emit_value(s, s->tmp->len - s->tmp_loc);
        ps_destroy(s->tmp);
        s->tmp = NULL;
        s->tmp_loc = 0;
        s->loc = 0;
        return val;
      }

      return NULL;
    }

    uint64_t new_len = s->tmp->len + val->len;
    uint8_t *data = (uint8_t *)realloc(s->tmp->data, new_len + 1);
    if (!data) {
      ps_destroy(val);
      ps_status(base, PS_ERR);
      return NULL;
    }

    memcpy(&data[s->tmp->len], val->data, val->len);
    data[new_len] = 0;
    s->tmp->data = data;
    s->tmp->len = new_len;

    // recurse!
    return splitter_fn(base, PS_OK);
  }

  return NULL;
}

ps_t *create_splitter(uint64_t len, const uint8_t *bytes) {
  if (!len || !bytes) {
    return NULL;
  }

  pattern_t pattern = {
    bytes,
    len
  };

  splitter_t *s = ps_create_stream(splitter_t, NULL);
  s->fn = splitter_fn;
  memcpy(&s->pattern, &pattern, sizeof(pattern_t));

  return (ps_t *)s;
}