#pragma once

#include <pull/stream.h>

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif


ps_t *create_file_source(const char *path, uv_loop_t *loop);
ps_t *create_file_sink(const char *path, uv_loop_t *loop);


#ifdef __cplusplus
}
#endif
