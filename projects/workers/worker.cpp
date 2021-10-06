#include <rawkit/rawkit.h>
#include "shared.h"

void setup() {
  auto state = rawkit_hot_state("state", State);
  printf("WORKER: state(%p)\n", state);
}

void loop() {
  auto tex = rawkit_texture_mem("my texture", 512, 512, 0, VK_FORMAT_R32G32B32A32_SFLOAT);

  auto state = rawkit_hot_state("state", State);
  state->counter++;
  printf("WORKER: counter(%u)\n", state->counter);

  auto shader = rawkit_shader(rawkit_file("worker.comp"));
  auto gpu = rawkit_default_gpu();

  auto inst = rawkit_shader_instance_create_q(shader, VK_QUEUE_COMPUTE_BIT);

  if (inst) {
    rawkit_shader_instance_begin(inst);

    rawkit_shader_instance_param_texture(inst, "output_tex", tex, nullptr);
    rawkit_shader_instance_dispatch_compute(
      inst,
      tex->options.width,
      tex->options.height,
      0
    );

    rawkit_shader_instance_end(inst);
  }


  rawkit_worker_t *host = rawkit_worker_host();
  while(1) {
    auto msg = rawkit_worker_recv(host);
    if (!msg) {
      break;
    }

    auto data = (Message *)msg->data;
    rawkit_worker_message_release(msg);
  }
  Message outgoing_message = { .text = "i am ok" };
  rawkit_worker_send(host, &outgoing_message);
}