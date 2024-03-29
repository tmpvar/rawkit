#pragma once

#include <rawkit/hot.h>
#include <vulkan/vulkan.h>
#include <pull/stream.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct rawkit_worker_t {
  RAWKIT_RESOURCE_FIELDS

  char full_path[2048];

  // internal
  void *_state;
} rawkit_worker_t;

typedef struct rawkit_worker_message_t {
  PS_VALUE_FIELDS
} rawkit_worker_message_t;


typedef struct rawkit_worker_queue_status_t {
  u64 tx_count;
  u64 rx_count;
} rawkit_worker_queue_status_t;

rawkit_worker_t *rawkit_worker_create_ex(const char *name, const char *file, const char *relative_file, bool jit_debug);
#define rawkit_worker(file) rawkit_worker_create_ex(file, file, __FILE__, RAWKIT_JIT_DEBUG)
#define rawkit_worker_named(name, file) rawkit_worker_create_ex(name, file, __FILE__, RAWKIT_JIT_DEBUG)

#ifndef RAWKIT_WORKER_HOST_ADDRESS
  #define RAWKIT_WORKER_HOST_ADDRESS ((void *)0)
#endif

#define rawkit_worker_host() RAWKIT_WORKER_HOST_ADDRESS

void rawkit_worker_send_ex(rawkit_worker_t *worker, void *data, u32 size, bool target_host);
rawkit_worker_message_t *rawkit_worker_recv_ex(rawkit_worker_t *worker, bool target_host);
#define rawkit_worker_message_release ps_destroy

rawkit_worker_queue_status_t rawkit_worker_queue_status(rawkit_worker_t *worker);

rawkit_hot_context_t *rawkit_worker_hot_context(rawkit_worker_t *worker);


#define rawkit_worker_send(worker, data) rawkit_worker_send_ex(worker, data, sizeof(*data), worker == rawkit_worker_host())
#define rawkit_worker_recv(worker) rawkit_worker_recv_ex(worker, worker == rawkit_worker_host())


// Vulkan Synchronization
VkSemaphore rawkit_worker_timeline_semaphore(rawkit_worker_t *worker);

// adds one to the counter and returns the original value
//   usage:
//   u64 wait = rawkit_worker_timeline_counter_next(...)
//   u64 signal = wait + 1;
//   use wait and signal in a VkQueueSubmit call
u64 rawkit_worker_timeline_counter_next(rawkit_worker_t *worker);

// Compatibility

#ifdef RAWKIT_WORKER
  rawkit_gpu_t *rawkit_worker_default_gpu(rawkit_worker_t *worker);
  #define rawkit_default_gpu() rawkit_worker_default_gpu(rawkit_worker_host())
  #define rawkit_default_hot_context() rawkit_worker_hot_context(rawkit_worker_host())
#endif

#ifdef __cplusplus
}
#endif
