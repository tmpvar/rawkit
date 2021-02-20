#pragma once

#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

// counter - increments a uint64_t on every pull, the first value returned is `1`
PS_EXPORT ps_t *create_counter();

// multiplier - multiply a uint64_t by `scale`
PS_EXPORT ps_t *create_multiplier(uint64_t scale);

// nooper - the smallest possible through stream
PS_EXPORT ps_t *create_nooper();

// taker - let `n` entries through before forcing {next_status}
PS_EXPORT ps_t *create_taker(int64_t n, ps_stream_status next_status);

// collector - buffer all input until the source stream is DONE and then output the buffer
PS_EXPORT ps_t *create_collector();

// hex_printer - through stream that dumps all packets to the provided FILE before forwarding
//               the packet.
// Note: hex_printer will not close the incoming stream - this allows it to be used with stdout/stderr
PS_EXPORT ps_t *create_hex_printer(FILE *output);

// single_value - source stream that outputs the provided value and then changes its status to PS_DONE
PS_EXPORT ps_t *create_single_value(const void *data, uint64_t len);

// reverser - through stream that will reverse the bytes of a packet
PS_EXPORT ps_t *create_reverser();

// splitter - split a stream by a series of bytes
PS_EXPORT ps_t *create_splitter(uint64_t len, const uint8_t *bytes);

// user-value - allow a user to provide a value on the source side of a pipeline
typedef struct ps_user_value_t {
  PS_FIELDS
  ps_val_t *value;
} ps_user_value_t;

PS_EXPORT ps_t *create_user_value();
PS_EXPORT void ps_user_value_from_str(ps_user_value_t *s, const char *str);

#ifdef __cplusplus
}
#endif
