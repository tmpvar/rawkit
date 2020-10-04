#pragma once

#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

ps_t *create_counter();
ps_t *create_multiplier(uint64_t scale);

// nooper - the smallest possible through stream
ps_t *create_nooper();

// taker - let `n` entries through before forcing DONE
ps_t *create_taker(int64_t n);

// collector - buffer all input until the source stream is DONE and then output the buffer
ps_t *create_collector();

#ifdef __cplusplus
}
#endif
