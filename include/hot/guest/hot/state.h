#pragma once
#include <stdint.h>
#include <stddef.h>
typedef uint64_t HotStateID;

#if defined(HOT_GUEST)
extern "C" {
#endif

void *hotState(HotStateID id, size_t size, void *default_value);

#if defined(HOT_GUEST)
}
#endif
