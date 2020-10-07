#pragma once

#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

// counter - increments a uint64_t on every pull, the first value returned is `1`
ps_t *create_counter();

// multiplier - multiply a uint64_t by `scale`
ps_t *create_multiplier(uint64_t scale);

// nooper - the smallest possible through stream
ps_t *create_nooper();

// taker - let `n` entries through before forcing {next_status}
ps_t *create_taker(int64_t n, ps_stream_status next_status);

// collector - buffer all input until the source stream is DONE and then output the buffer
ps_t *create_collector();

// hex_printer - through stream that dumps all packets to the provided FILE before forwarding
//               the packet.
// Note: hex_printer will not close the incoming stream - this allows it to be used with stdout/stderr
ps_t *create_hex_printer(FILE *output);

// single_value - source stream that outputs the provided value and then changes its status to PS_DONE
ps_t *create_single_value(const void *data, uint64_t len);

// reverser - through stream that will reverse the bytes of a packet
ps_t *create_reverser();

#ifdef __cplusplus
}
#endif
