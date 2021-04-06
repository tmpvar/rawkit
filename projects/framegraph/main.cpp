
#include <rawkit/rawkit.h>
#include "fg/framegraph.h"

void setup() {}
void loop() {
  igText("here");

  FrameGraph fg;

  Buffer<u32> input_buf("inputs", 64);
  input_buf.write({
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
  });

  Buffer<u32> output_buf("outputs", 4);
  output_buf.write({0, 0, 0, 0});

  fg.shader("sum", {"sum.comp"})
    .buffer("input_buf", input_buf)
    .buffer("output_buf", output_buf)
    .dispatch(input_buf.length);

  vkDeviceWaitIdle(rawkit_default_gpu()->device);

  {
    rawkit_gpu_t *gpu = rawkit_default_gpu();
    // Create a command Buffer
    VkCommandBuffer command_buffer = rawkit_gpu_create_command_buffer(
      gpu
    );
    {
      if (!command_buffer) {
        printf("ERROR: framegraph/main.cpp: could not create command buffer\n");
        return;
      }
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      VkResult begin_result = vkBeginCommandBuffer(command_buffer, &begin_info);
    }

    // add a command to copy the buffer from our SSBO's main buffer
    // to our ring buffer
    {
      VkBufferCopy region;
      region.dstOffset = 0;
      region.srcOffset = 0;
      region.size = sizeof(u32) * 4;

      // copy the outputs to the ringbuffer so we can read them back
      vkCmdCopyBuffer(
        command_buffer,
        output_buf._buffer->handle,
        fg.ring_buffer->_state->buffer->handle,
        1,
        &region
      );
    }

    // submit
    {
      VkSubmitInfo end_info = {};
      end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      end_info.commandBufferCount = 1;
      end_info.pCommandBuffers = &command_buffer;
      VkResult err = vkEndCommandBuffer(command_buffer);
      if (err) {
        printf("ERROR: rawkit_gpu_ssbo_ex: could not end command buffer");
        return;
      }

      VkFence fence;
      {
        VkFenceCreateInfo create = {};
        create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create.flags = 0;
        err = vkCreateFence(gpu->device, &create, gpu->allocator, &fence);
        if (err) {
          printf("ERROR: rawkit_gpu_ssbo_ex: create fence failed (%i)\n", err);
          return;
        }
      }

      err = vkQueueSubmit(gpu->graphics_queue, 1, &end_info, fence);
      rawkit_gpu_queue_command_buffer_for_deletion(gpu, command_buffer, fence, gpu->command_pool);
      if (err) {
        printf("ERROR: rawkit_gpu_ssbo_ex: could not submit command buffer");
        return;
      }
    }
  }

  vkDeviceWaitIdle(rawkit_default_gpu()->device);

  u32 *buf = (u32 *)fg.ring_buffer->_state->data;
  igText("0: %u", buf[0]);
  igText("1: %u", buf[1]);
  igText("2: %u", buf[2]);
  igText("3: %u", buf[3]);


  // TODO: readback should probably be done
  // output_buf.map([](u32 *buf) {
  //   printf("0: %u\n", buf[0]);
  //   printf("1: %u\n", buf[1]);
  //   printf("2: %u\n", buf[2]);
  //   printf("3: %u\n", buf[3]);
  // });

  // vkDeviceWaitIdle(rawkit_default_gpu()->device);
  // u32 *val = output_buf.readOne(0);
  // if (val) {
  //   igText("result: %u", *val);
  // } else {
  //   igText("result: null");
  // }

}