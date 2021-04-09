#pragma once

#include <rawkit/rawkit.h>
#include "ring-buffer.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <vector>
using namespace std;

#include <glm/glm.hpp>
using namespace glm;

#define framegraph_tmp_str_len 4096
static char framegraph_tmp_str[framegraph_tmp_str_len + 1];

// Forward Declarations
struct Shader;
struct FrameGraph;
template <typename T> struct Buffer;
template <typename T> struct Upload;
template <typename T> struct Download;

// Struct Definitions
enum NodeType {
  NONE,
  BUFFER,
  SHADER,
  SHADER_INVOCATION,
  TEXTURE,
  UBO,
  SSBO,
  CONSTANT_BUFFER,
  TRANSFER
};

#define FRAMEGRAPH_NODE_INVALID 0xFFFFFFFF
struct FrameGraphNode {
  FrameGraph *framegraph = nullptr;

  VkAccessFlags accessMask = 0;

  u32 node_idx = 0xFFFFFFFF;
  NodeType node_type = NodeType::NONE;
  string name = "<invalid>";
  function<void(FrameGraphNode *)> _resolve_fn = nullptr;

  u32 node(FrameGraph *fg = nullptr);
  void resolve();
  virtual rawkit_resource_t *resource();
};

struct FrameGraph {
  rawkit_gpu_t *gpu = nullptr;
  RingBuffer *ring_buffer;


  VkCommandPool command_pool = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  VkQueue queue;
  VkFence fence = VK_NULL_HANDLE;

  vector<FrameGraphNode *> nodes;
  unordered_map<u32, vector<u32>> input_edges;
  unordered_map<u32, vector<u32>> output_edges;

  FrameGraph();
  ~FrameGraph();

  u32 addNode(FrameGraphNode *node);
  void addInputEdge(u32 node, u32 src);

  void begin();
  void end();

  // Builder interfaces for primitives
  Shader *shader(const char *name, vector<const char *> filenames);

  template<typename T>
  Buffer<T> *buffer(const char *name, u32 size);

  // Debugging
  void render_force_directed_imgui();
};

template <typename T>
struct Upload : FrameGraphNode {
  RingBufferAllocation allocation ={};
  u32 offset = 0;
  Buffer<T> *dest = nullptr;

  Upload(
    const RingBufferAllocation &allocation,
    Buffer<T> *dest,
    u32 offset,
    FrameGraph *framegraph
  );
};

template <typename T>
struct Download : FrameGraphNode {
  RingBufferAllocation allocation = {};
  u32 offset = 0;
  function<void(T *data, u32 size)> cb = nullptr;
  Buffer<T> *dest = nullptr;
  Download(
    const RingBufferAllocation &allocation,
    Buffer<T> *dest,
    u32 offset,
    FrameGraph *framegraph
  );
};

template <typename T>
struct Fill : FrameGraphNode {
  u64 offset = 0;
  u64 size = 0;
  u32 value = 0;
  Buffer<T> *dest = nullptr;
  Fill(
    Buffer<T> *dest,
    u64 offset,
    u64 size,
    u32 value,
    FrameGraph *framegraph
  );
};

template <typename T>
struct Buffer : FrameGraphNode {
  rawkit_gpu_buffer_t *_buffer;
  const u32 length = 0;
  const u32 size = 0;
  Buffer(const char *name, u32 length, FrameGraph *framegraph);
  rawkit_gpu_buffer_t *handle();
  Buffer<T> *write(vector<T> values, u32 offset = 0);
  Buffer<T> *write(T *values, u32 size, u32 offset = 0);
  // deferred read
  Buffer<T> *read(u32 offset, u32 size, function<void(T *data, u32 size)> cb);
  // fill the entire buffer with a constant value
  Buffer<T> *fill(u32 value, u64 offset = 0, u64 size = VK_WHOLE_SIZE);

  T *readOne(u32 index);
  VkResult map(std::function<void(T *)> cb);
  rawkit_resource_t *resource() override;
};

struct ShaderParam {
  const char *name;
  FrameGraphNode *node;
};

struct ShaderInvocation : FrameGraphNode {
  vector<ShaderParam> params;
  Shader *shader = nullptr;
  glm::uvec3 dims;
  ShaderInvocation();
};

struct Shader : public FrameGraphNode {
  rawkit_shader_t *handle = nullptr;
  vector<const rawkit_file_t *> files;
  rawkit_shader_options_t options;
  u32 invocations = 0;

  vector<std::function<void (rawkit_shader_instance_t *)>> io;

  ~Shader();
  Shader(const char *name, const vector<const char *> &filenames, FrameGraph *framegraph);
  template <typename T>
  Shader &buffer(const char *name, Buffer<T> &buffer);
  void rebuild();
  // TODO: make dims a param..
  ShaderInvocation *dispatch(const glm::uvec3 &dims, vector<ShaderParam> params);

  rawkit_resource_t *resource() override;

};

// FrameGraphNode Implementation
u32 FrameGraphNode::node(FrameGraph *fg) {
  if (!this->framegraph) {
    if (!fg) {
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " FrameGraphNode::node is not linked to a FrameGraph\n");
      return FRAMEGRAPH_NODE_INVALID;
    }
    this->framegraph = fg;
  }

  if (this->node_idx != FRAMEGRAPH_NODE_INVALID) {
    return this->node_idx;
  }

  this->node_idx = this->framegraph->addNode(this);
  return this->node_idx;
}

void FrameGraphNode::resolve() {
  if (this->_resolve_fn) {
    this->_resolve_fn(this);
  }

}
rawkit_resource_t *FrameGraphNode::resource() {
  return nullptr;
}

// FrameGraph Implementation
FrameGraph::FrameGraph() {
  this->ring_buffer = new RingBuffer("FrameGraph::ring_buffer", 64 * 1024 * 1024);
  this->gpu = rawkit_default_gpu();
}

FrameGraph::~FrameGraph() {
  delete this->ring_buffer;
}

Shader *FrameGraph::shader(const char *name, vector<const char *> filenames) {
  Shader *s = new Shader(name, filenames, this);
  s->framegraph = this;
  return s;
}

template<typename T>
Buffer<T> *FrameGraph::buffer(const char *name, u32 size) {
  return new Buffer<T>(name, size, this);
}

u32 FrameGraph::addNode(FrameGraphNode *node) {
  node->node_idx = this->nodes.size();
  this->nodes.push_back(node);
  return node->node_idx;
}

void FrameGraph::addInputEdge(u32 node, u32 src) {
  this->input_edges[node].push_back(src);
  this->output_edges[src].push_back(node);
}

void FrameGraph::begin() {
  this->input_edges.clear();
  this->output_edges.clear();
  this->nodes.clear();
}

void FrameGraph::end() {
  this->command_pool = this->gpu->command_pool;
  this->command_buffer = rawkit_gpu_create_command_buffer(this->gpu, nullptr);
  // begin the command buffer
  {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkResult err = vkBeginCommandBuffer(
      this->command_buffer,
      &info
    );

    if (err != VK_SUCCESS) {
      printf("ERROR: FrameGraph::end: could not begin command buffer");
      return;
    }
  }

  // linearize the graph
  {
    printf("linearize\n");
    for (auto node : this->nodes) {
      printf("  %s\n", node->name.c_str());
      if (node->node_type == NodeType::TRANSFER) {
        node->resolve();
        continue;
      }

      if (node->node_type == NodeType::SHADER_INVOCATION) {
        for (auto input_idx : this->input_edges[node->node_idx]) {
          FrameGraphNode *input = this->nodes[input_idx];
          if (!input) {
            continue;
          }

          printf("    input node %s\n", input->name.c_str());

          switch (input->node_type) {
            case NodeType::BUFFER: {
              VkBufferMemoryBarrier barrier = {};
              barrier.srcAccessMask = input->accessMask;
              barrier.dstAccessMask = input->accessMask | VK_ACCESS_SHADER_READ_BIT;
              input->accessMask = barrier.dstAccessMask;

              rawkit_gpu_buffer_transition(
                (rawkit_gpu_buffer_t *)input->resource(),
                this->command_buffer,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                barrier
              );
              break;
            }

            default:
              break;
          }
        }

        // for (auto input_idx : this->input_edges[node->node_idx]) {
        //   FrameGraphNode *output = this->nodes[input_idx];
        //   // TODO: mark the outputs as dirty
        // }

        node->resolve();
      }
    }
  }

  // TODO: ensure each shader's resources are ready to use
  //       prior to resolving it
  // for (auto node : this->nodes) {
  //   if (node->node_type == NodeType::SHADER_INVOCATION) {
  //     igText("resolve: %s", node->name.c_str());
  //     node->resolve();
  //   }
  // }

  vkEndCommandBuffer(command_buffer);
  {
    VkFenceCreateInfo create = {};
    create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create.flags = 0;
    VkResult err = vkCreateFence(
      gpu->device,
      &create,
      gpu->allocator,
      &this->fence
    );

    if (err) {
      printf("ERROR: FrameGraph::end: create fence failed (%i)\n", err);
      return;
    }
  }

  // submit the command buffer to the queue
  {
    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffer;
    VkResult err = vkQueueSubmit(
      this->queue,
      1,
      &submit,
      this->fence
    );

    if (err) {
      printf("ERROR: FrameGraph::end: unable to submit command buffer (%i)\n", err);
      return;
    }
  }

  // schedule the buffer for deletion
  {
    rawkit_gpu_queue_command_buffer_for_deletion(
      gpu,
      command_buffer,
      fence,
      this->command_pool
    );
  }
}

void FrameGraph::render_force_directed_imgui() {
  // render with imgui drawlist
  igBegin("FrameGraph (forced directed)", 0, 0);
  ImDrawList* dl = igGetWindowDrawList();
  vec2 min;
  igGetItemRectMin((ImVec2 *)&min);
  min.y += 20.0;
  struct Node {
    vec2 pos;
    vec2 dims;
    const char *name;
    u32 idx;

    float radius;
  };
  vector<Node> nodes;

  float padding = 5.0f;
  float y = 20.0f;
  for (auto node : this->nodes) {
    Node n = {
      .pos = min + vec2(10, y),
      .dims = vec2(
        padding * 2.0f + (float)node->name.size() * 7.0f,
        20.0f
      ),
      .name = node->name.c_str(),
      .idx = static_cast<u32>(nodes.size()),
    };

    n.radius = glm::length(n.dims * 0.5f);

    nodes.push_back(n);
    y+=10.0f;
  }

  // force directed graph layout
  {
    for (u32 steps=0; steps<10; steps++) {
      // distance constraint
      for (auto &src : nodes) {
        vec2 src_center = src.pos + src.dims * 0.5f;
        for (auto &dst : nodes) {
          vec2 dst_center = dst.pos + dst.dims * 0.5f;
          if (src.idx == dst.idx) {
            continue;
          }

          float d = glm::distance(src_center, dst_center);
          float diff = d - (src.radius + dst.radius) * 1.5f;

          if (diff > 0) {
            continue;
          }

          vec2 n = normalize(src_center - dst_center);
          src.pos -= (n * diff) * 0.5f;
          dst.pos += (n * diff) * 0.5f;
        }
      }

      // sprint constraint
      for (auto &src : nodes) {
        vec2 src_center = src.pos + src.dims * 0.5f;
        for (auto &dst_idx : this->input_edges[src.idx]) {
          auto &dst = nodes[dst_idx];

          if (dst.idx > src.idx) {
            dst.pos.y -= 10.0f;
          }

          vec2 dst_center = dst.pos + dst.dims * 0.5f;

          float d = distance(src_center, dst_center);
          float diff = d - (src.radius + dst.radius);

          vec2 n = normalize(src_center - dst_center);
          src.pos -= (n * diff) * 0.5f;
          dst.pos += (n * diff) * 0.5f;
        }
      }
    }
  }

  // reposition nodes to start inside the viewing area
  {
    // compute the lowest corner
    vec2 lb(FLT_MAX);
    for (auto &node : nodes) {
      lb = glm::min(lb, node.pos - node.radius);
    }

    // apply the bounding box to each node
    for (auto &node : nodes) {
      node.pos = min + node.pos - lb;
    }
  }


  // draw the edges first
  for (auto &node : nodes) {
    u32 l = strlen(node.name);
    vec2 c = node.pos + node.dims * 0.5f;

    // draw inputs
    for (auto edge : this->input_edges[node.idx]) {
      const auto &other = nodes[edge];
      vec2 oc = other.pos + other.dims * 0.5f;
      ImDrawList_AddLine(
        dl,
        {oc.x, oc.y},
        {c.x, c.y},
        0xFF00FF00,
        1.0f
      );
    }

    // // draw inputs
    // for (auto edge : fg->output_edges[node.idx]) {
    //   const auto &other = nodes[edge];
    //   vec2 oc = other.pos + other.dims * 0.5f;
    //   ImDrawList_AddLine(
    //     dl,
    //     {oc.x, oc.y},
    //     {c.x, c.y},
    //     0xFF0000FF,
    //     1.0f
    //   );
    // }
  }

  for (auto &node : nodes) {
    u32 l = strlen(node.name);
    vec2 ub = node.pos + node.dims;
    vec2 c = node.pos + node.dims * 0.5f;

    ImDrawList_AddRectFilled(
      dl,
      {node.pos.x, node.pos.y},
      {ub.x, ub.y},
      0xFFFFFFFF,
      4.0,
      ImDrawCornerFlags_All
    );

    ImDrawList_AddTextVec2(
      dl,
      {node.pos.x + 5, c.y - 7.0f},
      0xFF000000,
      node.name,
      node.name + l
    );
  }

}

// Shader Implementation
Shader::~Shader() {
  this->files.clear();
}

Shader::Shader(const char *name, const vector<const char *> &filenames, FrameGraph *framegraph) {
  this->node_type = NodeType::SHADER;
  this->name.assign(name);
  this->options = rawkit_shader_default_options();

  for (const char *filename : filenames) {
    this->files.push_back(
      rawkit_file_relative_to(filename, RAWKIT_ENTRY_DIRNAME)
    );
  }

  this->node(framegraph);
}

template <typename T>
Shader &Shader::buffer(const char *name, Buffer<T> &buffer) {
  // TODO: register this with the FrameGraph
  this->io.push_back([name, &buffer, this](rawkit_shader_instance_t *inst) {
    // TODO: this is where we'd figure out if we need to add a barrier and
    //       add it to a queue or something

  });
  return *this;
}

void Shader::rebuild() {
  this->handle = rawkit_shader_ex(
    rawkit_default_gpu(),
    rawkit_vulkan_renderpass(),
    nullptr,
    files.size(),
    files.data()
  );
}

rawkit_resource_t *Shader::resource() {
  return (rawkit_resource_t *)this->handle;
}

ShaderInvocation* Shader::dispatch(const glm::uvec3 &dims, vector<ShaderParam> params) {
  this->rebuild();

  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "%s-invocation#%u", this->name.c_str(), this->invocations++);

  ShaderInvocation *invocation = new ShaderInvocation();
  invocation->framegraph = this->framegraph;
  invocation->name.assign(framegraph_tmp_str);
  invocation->params = params;
  invocation->shader = this;
  invocation->dims = dims;
  invocation->_resolve_fn = [](FrameGraphNode *node) {
    ShaderInvocation *invocation = (ShaderInvocation *)node;
    Shader *shader = invocation->shader;

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      shader->handle,
      // TODO: use the framegraph command buffer
      node->framegraph->command_buffer,
      rawkit_window_frame_index()
    );

    if (!inst) {
      return;
    }

    for (auto param : invocation->params) {
      if (!param.name || !param.node) {
        continue;
      }

      auto resource = param.node->resource();
      if (!resource) {
        continue;
      }

      switch (param.node->node_type) {
        case NodeType::BUFFER: {
          if (!resource) {
            continue;
          }

          rawkit_shader_instance_param_buffer(
            inst,
            param.name,
            (rawkit_gpu_buffer_t *)resource
          );
          break;
        }

        default: {
          printf("ERROR: unhandled shader invocation param %s\n", param.name);
        }
      }
    }

    rawkit_shader_instance_dispatch_compute(
      inst,
      invocation->dims.x,
      invocation->dims.y,
      invocation->dims.z
    );

    rawkit_shader_instance_end(inst);
  };

  const rawkit_glsl_t *glsl = rawkit_shader_glsl(this->handle);
  igText("add input %s -> %s", this->name.c_str(), invocation->name.c_str());



  this->framegraph->addInputEdge(
    this->node(),
    invocation->node()
  );

  for (auto param : params) {
    const rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(
      glsl,
      param.name
    );

    if (entry.readable) {
      igText("add input %s -> %s", param.node->name.c_str(), invocation->name.c_str());
      this->framegraph->addInputEdge(
        invocation->node(),
        param.node->node(this->framegraph)
      );
    }

    if (entry.writable) {
      igText("add output %s -> %s", invocation->name.c_str(), param.node->name.c_str());
      this->framegraph->addInputEdge(
        param.node->node(this->framegraph),
        invocation->node()
      );
    }
  }

  return invocation;
}

// Upload Implementation
template <typename T>
Upload<T>::Upload(
  const RingBufferAllocation &allocation,
  Buffer<T> *dest,
  u32 offset,
  FrameGraph *framegraph
) {
  this->framegraph = framegraph;
  this->allocation = allocation;
  this->offset = offset;
  this->node_type = NodeType::TRANSFER;
  this->dest = dest;
}

// Download Implementation
template <typename T>
Download<T>::Download(
  const RingBufferAllocation &allocation,
  Buffer<T> *dest,
  u32 offset,
  FrameGraph *framegraph
) {
  this->framegraph = framegraph;
  this->allocation = allocation;
  this->offset = offset;
  this->node_type = NodeType::TRANSFER;
  this->dest = dest;
}

// Fill Implementation
template <typename T>
Fill<T>::Fill(
  Buffer<T> *dest,
  u64 offset,
  u64 size,
  u32 value,
  FrameGraph *framegraph
) {
  this->framegraph = framegraph;
  this->offset = offset;
  this->dest = dest;
  this->size = size;
  this->node_type = NodeType::TRANSFER;

  snprintf(
    framegraph_tmp_str,
    framegraph_tmp_str_len,
    "fill(%x)::%lu->%lu",
    value,
    offset,
    glm::min(size, static_cast<u32>(dest->size) - offset)
  );

  this->name.assign(framegraph_tmp_str);

  this->_resolve_fn = [](FrameGraphNode *node) {
    auto fill = (Fill<T> *)node;

    u32 not_multiple_of_4 = 3;
    if (fill->offset & not_multiple_of_4) {
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " Buffer::fill() offset must be a multiple of 4\n");
      return;
    }

    auto buf = (rawkit_gpu_buffer_t *)fill->dest->resource();
    u32 buf_size = static_cast<u32>(fill->dest->size);
    u64 size = glm::min(fill->size, buf_size - fill->offset);

    if (fill->offset >= buf_size) {
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " Buffer::fill() offset must be less than size\n");
      return;
    }

    if (size & not_multiple_of_4) {
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " Buffer::fill() size must be a multiple of 4\n");
      return;
    }

    if (size > buf_size) {
      size = VK_WHOLE_SIZE;
    }

    vkCmdFillBuffer(
      node->framegraph->command_buffer,
      buf->handle,
      fill->offset,
      size,
      fill->value
    );
  };
}

// ShaderInvocation Implementation
ShaderInvocation::ShaderInvocation() {
  this->node_type = NodeType::SHADER_INVOCATION;
}

// Buffer Implementation
const static VkMemoryPropertyFlags default_memory_flags = (
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
);

const static VkBufferUsageFlags default_buffer_usage_flags = (
  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT
);

template <typename T>
Buffer<T>::Buffer(const char *name, u32 length, FrameGraph *framegraph)
  : length(length), size(length * sizeof(T))
{
  this->framegraph = framegraph;
  this->node_type = NodeType::BUFFER;
  this->name.assign(name);

  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "FrameGraph::Buffer::%s", name);
  this->_buffer = rawkit_gpu_buffer_create(
    framegraph_tmp_str,
    framegraph->gpu,
    this->size,
    default_memory_flags,
    default_buffer_usage_flags
  );

  igText("buffer %s version %u", framegraph_tmp_str, this->_buffer->resource_version);
}

template <typename T>
rawkit_resource_t *Buffer<T>::resource() {
  return (rawkit_resource_t *)this->_buffer;
}

template <typename T>
Buffer<T> *Buffer<T>::write(vector<T> values, u32 offset) {
  return this->write(values.data(), values.size(), offset);
}

template <typename T>
Buffer<T> *Buffer<T>::write(T *data, u32 size, u32 offset) {
  if (offset >= length) {
    printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " Buffer::write attempt to write past the end of the buffer\n");
    return this;
  }

  // create an allocation in the ring buffer and copy the incoming data
  u32 size_bytes = size * sizeof(T);
  u32 offset_bytes = offset * sizeof(T);

  auto allocation = this->framegraph->ring_buffer->alloc(size_bytes);
  if (!allocation.size) {
    printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " Buffer::write could not allocate transfer memory in ring buffer\n");
    return this;
  }

  memcpy(allocation.data(), data, size_bytes);

  if (0) {
    printf("%s write @ %u\n  ", this->name.c_str(),  allocation.offset);
    for (u32 i=0; i<size; i++) {
      if (i > 0 && i%16 == 0) {
        printf("\n  ");
      }
      printf("%u|%u ", ((u32 *)allocation.data())[i], data[i]);
    }
    printf("\n");

    igText("copy %u into ring buffer", size_bytes);
  }

  auto upload = new Upload<T>(allocation, this, offset, this->framegraph);
  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "upload bytes(%u)", allocation.size);
  upload->name.assign(framegraph_tmp_str);
  upload->_resolve_fn = [](FrameGraphNode *node) {
    FrameGraph *fg = node->framegraph;
    Upload<T> *upload = (Upload<T> *)node;
    auto *src = fg->ring_buffer->gpuBuffer();
    auto *dest = (rawkit_gpu_buffer_t *)upload->dest->resource();
    const auto &allocation = upload->allocation;
    VkBufferCopy copyRegion = {};
    copyRegion.size = allocation.size;
    copyRegion.srcOffset = allocation.offset;
    copyRegion.dstOffset = upload->offset;

    vkCmdCopyBuffer(
      fg->command_buffer,
      src->handle,
      dest->handle,
      1,
      &copyRegion
    );
  };

  // setup the transfer from the ring buffer to the gpu buffer
  this->framegraph->addInputEdge(
    this->node(),
    upload->node()
  );

  return this;
}

template <typename T>
Buffer<T> *Buffer<T>::read(u32 offset, u32 count, function<void(T *data, u32 size)> cb) {
  if (offset >= length) {
    return this;
  }

  // create an allocation in the ring buffer and copy the incoming data
  u32 size_bytes = count * sizeof(T);
  u32 offset_bytes = offset * sizeof(T);
  if (size_bytes + offset_bytes >= this->size) {
    size_bytes = this->size - offset_bytes;
  }

  auto allocation = this->framegraph->ring_buffer->alloc(size_bytes);

  auto download = new Download<T>(allocation, this, offset, this->framegraph);
  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "download bytes(%u)", allocation.size);
  download->name.assign(framegraph_tmp_str);
  download->cb = cb;
  download->_resolve_fn = [](FrameGraphNode *node){
    FrameGraph *fg = node->framegraph;
    Download<T> *download = (Download<T> *)node;
    auto *src = (rawkit_gpu_buffer_t *)download->dest->resource();
    auto *dest = fg->ring_buffer->gpuBuffer();

    const auto &allocation = download->allocation;
    VkBufferCopy copyRegion = {};
    copyRegion.size = allocation.size;
    copyRegion.srcOffset = download->offset;
    copyRegion.dstOffset = allocation.offset;

    vkCmdCopyBuffer(
      fg->command_buffer,
      src->handle,
      dest->handle,
      1,
      &copyRegion
    );

    // TODO: add the buffer copy command to the command buffer
    //       and setup a listener for when it completes (via Fences) to call cb()
    //       with the allocation->data(), allocation.size

  };

  // setup the transfer from the ring buffer to the gpu buffer
  this->framegraph->addInputEdge(
    download->node(),
    this->node()
  );

  return this;
}

template <typename T>
Buffer<T> *Buffer<T>::fill(u32 value, u64 offset, u64 size) {
  auto fill = new Fill<T>(
    this,
    offset,
    size,
    value,
    this->framegraph
  );

  this->framegraph->addInputEdge(
    this->node(),
    fill->node()
  );

  return this;
}

template <typename T>
T *Buffer<T>::readOne(u32 index) {
  // if (index >= length) {
  //   return nullptr;
  // }

  // return &this->slab[index];
  return nullptr;
}

template <typename T>
VkResult Buffer<T>::map(std::function<void(T *)> cb) {
  u64 offset = 0;
  u64 size = VK_WHOLE_SIZE;

  rawkit_gpu_ssbo_t *ssbo = this->handle();

  void *ptr;
  VkResult err = vkMapMemory(
    ssbo->gpu->device,
    ssbo->buffer->memory,
    offset,
    size,
    0,
    &ptr
  );

  if (err) {
    printf("ERROR: unable to map memory (%i)\n", err);
    return err;
  }

  cb((T *)ptr);

  // flush
  {
    VkMappedMemoryRange flush = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = ssbo->buffer->memory,
      .offset = offset,
      .size = size,
    };

    err = vkFlushMappedMemoryRanges(
      ssbo->gpu->device,
      1,
      &flush
    );

    if (err) {
      printf("ERROR: unable to flush mapped memory ranges (%i)\n", err);
    }
  }

  vkUnmapMemory(
    ssbo->gpu->device,
    ssbo->buffer->memory
  );

  return err;
}
