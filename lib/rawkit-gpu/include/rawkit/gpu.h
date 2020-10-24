#pragma once

#include <rawkit/glsl.h>

#include <string.h>

#ifdef __cplusplus
  extern "C" {
#endif


typedef struct rawkit_gpu_pipeline_t {

} rawkit_gpu_pipeline_t;

rawkit_gpu_pipeline_t *rawkit_gpu_pipeline(
  const char *name
);


#ifdef __cplusplus
  }
#endif
