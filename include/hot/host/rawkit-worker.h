#pragma once

#include <rawkit/jit.h>
#include <rawkit/worker.h>

static void host_init_rawkit_worker(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_worker_create_ex", (void *)&rawkit_worker_create_ex);
  rawkit_jit_add_export(jit, "rawkit_worker_send_ex", (void *)&rawkit_worker_send_ex);
  rawkit_jit_add_export(jit, "rawkit_worker_recv_ex", (void *)&rawkit_worker_recv_ex);
  rawkit_jit_add_export(jit, "rawkit_worker_queue_status", (void *)&rawkit_worker_queue_status);
  rawkit_jit_add_export(jit, "rawkit_worker_hot_context", (void *)&rawkit_worker_hot_context);
  rawkit_jit_add_export(jit, "rawkit_worker_timeline_semaphore", (void *)&rawkit_worker_timeline_semaphore);
  rawkit_jit_add_export(jit, "rawkit_worker_timeline_counter_next", (void *)&rawkit_worker_timeline_counter_next);
}