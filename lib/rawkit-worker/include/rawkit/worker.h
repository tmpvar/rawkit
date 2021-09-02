#pragma once

#include <rawkit/hot.h>

typedef struct rawkit_worker_t {
  RAWKIT_RESOURCE_FIELDS

  char full_path[2048];

  // internal
  void *_state;
} rawkit_worker_t;

typedef struct rawkit_worker_queue_status_t {
  u64 tx_count;
  u64 rx_count;
} rawkit_worker_queue_status_t;

rawkit_worker_t *rawkit_worker_create_ex(const char *name, const char *file, const char *relative_file);
#define rawkit_worker(name, file) rawkit_worker_create_ex(name, file, __FILE__);

#ifndef RAWKIT_WORKER_HOST_ADDRESS
  #define RAWKIT_WORKER_HOST_ADDRESS ((void *)0)
#endif

#define rawkit_worker_host() RAWKIT_WORKER_HOST_ADDRESS

void rawkit_worker_send_ex(rawkit_worker_t *worker, void *data, bool target_host);
void *rawkit_worker_recv_ex(rawkit_worker_t *worker, bool target_host);
rawkit_worker_queue_status_t rawkit_worker_queue_status(rawkit_worker_t *worker);


#define rawkit_worker_send(worker, data) rawkit_worker_send_ex(worker, data, worker == rawkit_worker_host())
#define rawkit_worker_recv(worker) rawkit_worker_recv_ex(worker, worker == rawkit_worker_host())