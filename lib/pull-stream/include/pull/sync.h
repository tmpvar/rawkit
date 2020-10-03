#pragma once

#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

ps_cb_t *create_counter();
ps_cb_t *create_multiplier(uint64_t scale);

// nooper - the smallest possible through stream
ps_cb_t *create_nooper();

#ifdef __cplusplus
}
#endif
