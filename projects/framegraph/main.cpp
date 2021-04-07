
#include <rawkit/rawkit.h>
#include "fg/framegraph.h"
#include <glm/glm.hpp>

#include "ctx/context-2d.h"
#include "ctx/camera-2d.h"
#include "ctx/mouse.h"

using namespace glm;

void setup() {}
void loop() {
  FrameGraph fg;
  // rawkit_default_gpu(),
  // default_memory_flags | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
  // default_buffer_usage_flags

  auto input_buf = fg.buffer<u32>("inputs", 64);
  input_buf->write({
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
  });

  auto output_buf = fg.buffer<u32>("outputs", 4);
  output_buf->write({0, 0, 0, 0});

  fg.shader("sum", {"sum.comp"})
    ->dispatch(
      uvec3(input_buf->length, 1, 1),
      {
        { "input_buf", input_buf },
        { "output_buf", output_buf },
      }
    );

  output_buf->read(0, 4, [](u32 *data, u32 size) {
    igText("0: %u", data[0]);
    igText("1: %u", data[1]);
    igText("2: %u", data[2]);
    igText("3: %u", data[3]);
  });

  fg.end();

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
        output_buf->_buffer->handle,
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



  // render the graph
  {
    Context2D ctx;

    ctx.scale(1.0, -1.0);
    ctx.translate(vec2(0.0, -(float)rawkit_window_height()));



  }

  // output .dot format
  {
    static bool first = true;
    if (first) {
      first = false;
      const char *fill_color = "#BBBBBBFF";
      const char *border_color = "#AAAAAAFF";
      const char *font_color = "#000000FF";
      const char *edge_color = "#666666FF";

      printf(
        "digraph framegraph {\n"
        "  graph [truecolor=true bgcolor=\"#00000000\"]\n"
        "  node [style=filled fillcolor=\"%s\" color=\"%s\" fontcolor=\"%s\" fontname=\"Roboto Light\"]\n"
        "  edge [color=\"%s\"]\n",
        fill_color,
        border_color,
        font_color,
        edge_color
      );

      for (auto node : fg.nodes) {
        u32 idx = node->node();
        string params = string("label=\"") + node->name + "\"";

        switch (node->node_type) {
          case NodeType::SHADER_INVOCATION:
              params += " shape=square style=\"bold,filled\"";
            break;
          case NodeType::SHADER:
              params += " shape=square style=\"filled\"";
            break;
          default:
            params += " shape=box style=\"rounded,filled\"";
        }

        printf("  node_%u [%s];\n", idx, params.c_str());
      }

      for (auto node : fg.nodes) {
        u32 idx = node->node();

        auto inputs = fg.input_edges.find(idx);
        if (inputs != fg.input_edges.end()) {
          for (auto input : inputs->second) {
            printf("  node_%u -> node_%u;\n", input, idx);
          }
        }

        auto outputs = fg.output_edges.find(idx);
        if (outputs != fg.output_edges.end()) {
          for (auto output : outputs->second) {
            printf("  node_%u -> node_%u;\n", idx, output);
          }
        }

      }
      printf("}\n");
    }
  }
}