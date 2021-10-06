#include <rawkit/rawkit.h>

#include "shared.h"

void setup() {
  auto state = rawkit_hot_state("state", State);
  printf("MAIN: state(%p)\n", state);
}

void loop() {
  rawkit_worker_t *worker = rawkit_worker_named("worker name", "worker.cpp");

  auto state = rawkit_hot_state("state", State);
  printf("MAIN: counter(%u)\n", state->counter);

  Message outgoing_message = { .text = "are you ok?" };
  rawkit_worker_send(worker, &outgoing_message);

  auto status = rawkit_worker_queue_status(worker);

  igText("queue status tx(%llu) rx(%llu)", status.tx_count, status.rx_count);

  while(1) {
    auto msg = rawkit_worker_recv(worker);
    if (!msg) {
      break;
    }

    auto data = (Message *)msg->data;
    // printf("host recv: %s\n", data->text);
    rawkit_worker_message_release(msg);
  }
}