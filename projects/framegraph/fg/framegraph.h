#pragma once

#include <rawkit/rawkit.h>
#include "ring-buffer.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <vector>
using namespace std;

#include <glm/glm.hpp>

#define framegraph_tmp_str_len 4096
static char framegraph_tmp_str[framegraph_tmp_str_len + 1];

// Forward Declarations
struct Shader;
struct FrameGraph;
template <typename T> struct Buffer;
struct Upload;
struct Download;

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

  u32 node_idx = 0xFFFFFFFF;
  NodeType node_type = NodeType::NONE;
  string name = "<invalid>";
  function<void()> resolve = nullptr;

  u32 node(FrameGraph *fg = nullptr);
  virtual rawkit_resource_t *resource();
};

struct FrameGraph {
  rawkit_gpu_t *gpu = nullptr;
  RingBuffer *ring_buffer;

  vector<FrameGraphNode *> nodes;
  unordered_map<u32, vector<u32>> input_edges;
  unordered_map<u32, vector<u32>> output_edges;

  FrameGraph();
  ~FrameGraph();

  u32 addNode(FrameGraphNode *node);
  void addInputEdge(u32 node, u32 src);
  void addOutputEdge(u32 node, u32 dst);

  void end();

  // Builder interfaces for primitives
  Shader *shader(const char *name, vector<const char *> filenames);

  template<typename T>
  Buffer<T> *buffer(const char *name, u32 size);
};

struct Upload : FrameGraphNode {
  RingBufferAllocation allocation ={};
  u32 offset = 0;
  function<void()> resolve = nullptr;
  Upload(const RingBufferAllocation &allocation, u32 offset, FrameGraph *framegraph);
};

struct Download : FrameGraphNode {
  RingBufferAllocation allocation ={};
  u32 offset = 0;
  function<void()> resolve = nullptr;
  Download(const RingBufferAllocation &allocation, u32 offset, FrameGraph *framegraph);
};

template <typename T>
struct Buffer : FrameGraphNode {
  rawkit_gpu_buffer_t *_buffer;
  const u32 length = 0;
  const u32 size = 0;
  Buffer(const char *name, u32 length, FrameGraph *framegraph);
  rawkit_gpu_buffer_t *handle();
  Buffer<T> *write(vector<T> values, u32 offset = 0);
  Buffer<T> *read(u32 offset, u32 size, function<void(T *data, u32 size)> cb);

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
}

void FrameGraph::addOutputEdge(u32 node, u32 dst) {
  this->output_edges[node].push_back(dst);
}

void FrameGraph::end() {
  // TODO: ensure each shader's resources are ready to use
  //       prior to resolving it
  for (auto node : this->nodes) {
    if (node->node_type == NodeType::SHADER_INVOCATION) {
      igText("resolve: %s", node->name.c_str());
      node->resolve();
    }
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
  invocation->resolve = [this, invocation, dims]() {
    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      this->handle,
      // TODO: use the framegraph command buffer
      NULL,
      0
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
      dims.x,
      dims.y,
      dims.z
    );

    rawkit_shader_instance_end(inst);
  };

  const rawkit_glsl_t *glsl = rawkit_shader_glsl(this->handle);
  igText("add input %s -> %s", this->name.c_str(), invocation->name.c_str());
  this->framegraph->addInputEdge(
    invocation->node(),
    this->node()
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
      this->framegraph->addOutputEdge(
        invocation->node(),
        param.node->node(this->framegraph)
      );
    }
  }

  return invocation;
}

// Upload Implementation
Upload::Upload(const RingBufferAllocation &allocation, u32 offset, FrameGraph *framegraph) {
  this->framegraph = framegraph;
  this->allocation = allocation;
  this->offset = offset;
}

// Download Implementation
Download::Download(const RingBufferAllocation &allocation, u32 offset, FrameGraph *framegraph) {
  this->framegraph = framegraph;
  this->allocation = allocation;
  this->offset = offset;
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
rawkit_gpu_buffer_t *Buffer<T>::handle() {
  return this->_buffer;
}

template <typename T>
rawkit_resource_t *Buffer<T>::resource() {
  return (rawkit_resource_t *)this->_buffer;
}

template <typename T>
Buffer<T> *Buffer<T>::write(vector<T> values, u32 offset) {
  if (offset >= length) {
    return this;
  }

  // create an allocation in the ring buffer and copy the incoming data
  u32 size_bytes = values.size() * sizeof(T);
  u32 offset_bytes = offset * sizeof(T);
  auto allocation = this->framegraph->ring_buffer->alloc(size_bytes);
  memcpy(allocation.data(), values.data(), size_bytes);

  auto upload = new Upload(allocation, offset, this->framegraph);
  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "upload bytes(%u)", allocation.size);
  upload->name.assign(framegraph_tmp_str);
  upload->resolve = [this]() {
    printf("TODO: resolve upload\n");
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

  auto download = new Download(allocation, offset, this->framegraph);
  snprintf(framegraph_tmp_str, framegraph_tmp_str_len, "download bytes(%u)", allocation.size);
  download->name.assign(framegraph_tmp_str);
  download->resolve = [cb](){
    // TODO: add the buffer copy command to the command buffer
    //       and setup a listener for when it completes (via Fences) to call cb()
    //       with the allocation->data(), allocation.size
    printf("TODO: resolve download\n");
  };

  // setup the transfer from the ring buffer to the gpu buffer
  this->framegraph->addOutputEdge(
    this->node(),
    download->node()
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
