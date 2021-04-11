#pragma once

#include <rawkit/rawkit.h>
#include "ring-buffer.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <vector>
#include <sstream>
#include "bitset.h"
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

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
struct Texture;
struct PendingEvent;

// Struct Definitions
enum NodeType {
  NONE,
  BUFFER,
  SHADER,
  SHADER_INVOCATION,
  TEXTURE,
  TRANSFER,
  DEBUG,
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

struct PendingEvent {
  FrameGraphNode *node = nullptr;
  VkEvent handle;
  function<void(FrameGraphNode *)> cb;
  bool valid = true;

  PendingEvent(
    FrameGraphNode *node,
    VkCommandBuffer command_buffer,
    VkPipelineStageFlags stageMask,
    function<void(FrameGraphNode *)> cb
  );

  ~PendingEvent();

  bool complete();
};

struct FrameGraph {
  rawkit_gpu_t *gpu = nullptr;
  RingBuffer *ring_buffer;

  vector<PendingEvent *> pending_events;

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

  void reset();
  void begin();
  void end();

  // Builder interfaces for primitives

  // Create a Shader FrameGraphNode
  //
  // uses a clang builtin to retrieve the caller's filename, so the list of filenames can be relative
  // see: https://clang.llvm.org/docs/LanguageExtensions.html#source-location-builtins
  Shader *shader(const char *name, vector<string> filenames, const char *relative_file = __builtin_FILE());

  template<typename T>
  Buffer<T> *buffer(const char *name, u32 size);

  Texture *texture(const char *name, VkFormat format, uvec3 dims);
  Texture *texture(const char *filename, const char *relative_file = __builtin_FILE());


  // Debugging
  void render_force_directed_imgui();
  string render_graphviz_dot();
};

struct ExecutionTree {

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

struct DebugForceDirected : FrameGraphNode {
  DebugForceDirected(FrameGraph *framegraph) {
    this->name.assign("DebugForceDirected");
    this->framegraph = framegraph;
    this->node_type = NodeType::DEBUG;
    this->node();
  }
  rawkit_resource_t *resource() override {
    return nullptr;
  };
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

  rawkit_resource_t *resource() override;
};

struct TextureDebugImGui : FrameGraphNode {
  Texture *texture = nullptr;
  vec2 dims = vec2(256.0);
  vec2 pos = vec2(0.0);
  ImDrawList *draw_list = nullptr;
  rawkit_texture_sampler_t *sampler = nullptr;

  TextureDebugImGui(
    Texture *texture,
    FrameGraph *framegraph,
    vec2 dims = vec2(256.0),
    vec2 pos = vec2(0.0),
    ImDrawList *dl = nullptr,
    rawkit_texture_sampler_t *sampler = nullptr
  );
  rawkit_resource_t *resource() override;
};

struct Texture : FrameGraphNode {
  VkFormat format;
  rawkit_texture_t *texture = nullptr;
  Texture(const char *name, VkFormat format, const uvec3 &dims, FrameGraph *framegraph);
  Texture(const char *filename, const char *relative_file, FrameGraph *framegraph);

  rawkit_resource_t *resource() override;
  uvec3 dims();

  void debug_imgui(
    const vec2 &dims = vec2(256.0),
    rawkit_texture_sampler_t *sampler = nullptr,
    const vec2 &pos = vec2(0.0),
    ImDrawList *dl = nullptr
  );
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

  ~Shader();
  Shader(const char *name, const vector<string> &filenames, FrameGraph *framegraph);
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
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " FrameGraphNode::node(%s) is not linked to a FrameGraph\n", this->name.c_str());
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
  this->ring_buffer = new RingBuffer("FrameGraph::ring_buffer", 1024);
  this->gpu = rawkit_default_gpu();
}

FrameGraph::~FrameGraph() {
  delete this->ring_buffer;
}

Shader *FrameGraph::shader(const char *name, vector<string> filenames, const char *relative_file) {
  fs::path rel = fs::absolute(fs::path(relative_file).remove_filename());
  for (auto &file : filenames) {
    file.assign((rel / file).string());
  }

  Shader *s = new Shader(name, filenames, this);
  s->framegraph = this;
  return s;
}

template<typename T>
Buffer<T> *FrameGraph::buffer(const char *name, u32 size) {
  return new Buffer<T>(name, size, this);
}

Texture *FrameGraph::texture(const char *name, VkFormat format, uvec3 dims) {
  return new Texture(name, format, dims, this);
}

Texture *FrameGraph::texture(const char *filename, const char *relative_file) {
  return new Texture(filename, relative_file, this);
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

void FrameGraph::reset() {
  this->input_edges.clear();
  this->output_edges.clear();
  this->nodes.clear();
  this->pending_events.clear();
}

void FrameGraph::begin() {
  this->input_edges.clear();
  this->output_edges.clear();
  this->nodes.clear();

  // service the pending events
  {
    i64 l = static_cast<i64>(this->pending_events.size());
    for (i64 i=l-1; i>=0; i--) {
      auto event = this->pending_events[i];
      if (event->complete()) {
        this->pending_events.erase(
          this->pending_events.begin() + i
        );
      }
    }
  }
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
    igText("linearize\n");
    for (auto node : this->nodes) {
      igText("  %s\n", node->name.c_str());
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

          igText("    input node %s\n", input->name.c_str());

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

      if (node->node_type == NodeType::DEBUG) {
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


void graph_isolator(FrameGraph *fg, Bitset &bitset, const u32 node, vector<u32> &graph) {
  if (bitset.get(node)) {
    return;
  }

  bitset.set(node);
  graph.push_back(node);
  for (u32 input : fg->input_edges[node]) {
    graph_isolator(fg, bitset, input, graph);
  }

  for (u32 output : fg->output_edges[node]) {
    graph_isolator(fg, bitset, output, graph);
  }
}

void FrameGraph::render_force_directed_imgui() {
  auto debug = new DebugForceDirected(this);
  debug->_resolve_fn = [](FrameGraphNode *node) {
    auto that = node->framegraph;

    // identify isolated graphs
    vector<vector<u32>> graphs;
    {
      u32 l = that->nodes.size();
      Bitset seen(that->nodes.size());
      for (u32 i = 0; i<l; i++) {
        if (seen.get(i)) {
          continue;
        }

        vector<u32> graph;
        graph_isolator(
          that,
          seen,
          i,
          graph
        );

        if (graph.size()) {
          graphs.push_back(graph);
        }
      }
    }

    // render with imgui drawlist
    igBegin("FrameGraph (forced directed)", 0, 0);
    ImDrawList* dl = igGetWindowDrawList();
    vec2 min;
    igGetItemRectMin((ImVec2 *)&min);
    min.y += 20.0;
    min.x += 20.0;
    struct Node {
      vec2 pos;
      vec2 dims;
      const char *name;
      u32 idx;

      float radius;
    };

    float padding = 5.0f;
    float graph_y = 20.0f;

    vector<Node> nodes;

    // initialize the visual nodes
    float y = 0.0f;
    for (auto node : that->nodes) {
      if (!node) {
        continue;
      }

      Node n = {
        .pos = vec2(0.0, y),
        .dims = vec2(
          padding * 2.0f + (float)node->name.size() * 7.0f,
          padding * 2.0f + 13.0f
        ),

        .name = node->name.c_str(),
        .idx = static_cast<u32>(nodes.size()),
      };

      if (node->node_type == NodeType::TEXTURE) {
        auto tex = (Texture *)node;
        float aspect = (float)tex->texture->options.height / (float)tex->texture->options.width;
        n.dims.y += n.dims.x * aspect + 5.0f;
      }

      n.radius = glm::length(n.dims * 0.5f);

      nodes.push_back(n);
      y+=100.0f;
    }

    for (const auto &graph : graphs) {
      // force directed graph layout
      {
        for (u32 steps=0; steps<10; steps++) {
          // distance constraint
          for (u32 src_idx : graph) {
            auto &src = nodes[src_idx];
            vec2 src_center = src.pos + src.dims * 0.5f;
            for (auto &dst : nodes) {
              vec2 dst_center = dst.pos + dst.dims * 0.5f;
              if (src.idx == dst.idx) {
                continue;
              }

              float d = glm::distance(src_center, dst_center);
              float diff = d - (src.radius + dst.radius) * 1.15f;

              if (diff > 0) {
                continue;
              }

              vec2 n = normalize(src_center - dst_center);
              src.pos -= (n * diff) * 0.95f;
              dst.pos += (n * diff) * 0.95f;
            }
          }

          // sprint constraint
          for (u32 src_idx : graph) {
            auto &src = nodes[src_idx];
            vec2 src_center = src.pos + src.dims * 0.5f;
            for (auto &dst_idx : that->input_edges[src.idx]) {
              auto &dst = nodes[dst_idx];

              if (dst.idx > src.idx) {
                dst.pos.y -= 10.0f;
              }

              vec2 dst_center = dst.pos + dst.dims * 0.5f;

              float d = distance(src_center, dst_center);
              float diff = d - (src.radius + dst.radius);
              if (diff >= 0.0) {

              }
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
        vec2 ub(-FLT_MAX);
        for (u32 node_idx : graph) {
          auto &node = nodes[node_idx];

          lb = glm::min(lb, node.pos);
          ub = glm::max(ub, node.pos + node.dims);
        }

        ImDrawList_AddRect(
          dl,
          {min.x - padding , min.y + graph_y - padding},
          {
            min.x + (ub.x - lb.x) + padding,
            min.y + graph_y + (ub.y - lb.y) + padding
          },
          0xFF00FF00,
          1.0f,
          0,
          2.0f
        );

        // apply the bounding box to each node
        for (u32 node_idx : graph) {
          auto &node = nodes[node_idx];
          node.pos = min + node.pos - lb;
          node.pos.y += graph_y;
        }


        graph_y += (ub.y - lb.y) + 20.0f;
      }


      // draw the edges first
      for (u32 node_idx : graph) {
        auto &node = nodes[node_idx];
        u32 l = strlen(node.name);
        vec2 c = node.pos + node.dims * 0.5f;

        // draw inputs
        for (auto edge : that->input_edges[node.idx]) {
          const auto &other = nodes[edge];
          vec2 oc = other.pos + other.dims * 0.5f;
          ImDrawList_AddLine(
            dl,
            {oc.x, oc.y},
            {c.x, c.y},
            0xFF00FF00,
            1.0f
          );

          // draw cirles on the dest
          {
            float radius = 1.0f;

            vec2 rd = c - oc;
            vec2 ird = 1.0f / rd;
            vec2 tbot = ird * (node.pos - oc);
            vec2 ttop = ird * ((node.pos + node.dims) - oc);
            vec2 tmin = glm::min(ttop, tbot);
            vec2 isect = oc + rd * glm::max(tmin.x, tmin.y);

            ImDrawList_AddCircleFilled(
              dl,
              { isect.x, isect.y },
              4.0,
              0xFF00FF00,
              16
            );
          }
        }
      }

      for (u32 node_idx : graph) {
        auto &node = nodes[node_idx];
        auto graph_node = that->nodes[node.idx];
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
          {node.pos.x + 5, node.pos.y + padding},
          0xFF000000,
          node.name,
          node.name + l
        );

        if (graph_node->node_type == NodeType::TEXTURE) {
          auto tex = (Texture *)graph_node;
          ImTextureID dst_tex = rawkit_imgui_texture(
            tex->texture,
            tex->texture->default_sampler
          );

          if (!dst_tex) {
            printf("no dst_tex\n");
            return;
          }

          float y = padding + 7.0f + padding * 2.0f;
          vec2 lb = vec2(node.pos.x + padding, node.pos.y + y);
          vec2 ub = lb + vec2(node.dims.x - padding * 2.0f, node.dims.y - y - padding);

          ImDrawList_AddImage(
            dl,
            dst_tex,
            { lb.x, lb.y },
            { ub.x, ub.y },
            { 0.0f, 0.0f },
            { 1.0f, 1.0f },
            0xFFFFFFFF
          );

        }
      }
    }

    igEnd();
  };
}

string FrameGraph::render_graphviz_dot() {
  stringstream s;
  // output .dot format
  const char *fill_color = "#BBBBBBFF";
  const char *border_color = "#AAAAAAFF";
  const char *font_color = "#000000FF";
  const char *edge_color = "#666666FF";

  s << "digraph framegraph {\n"
        "  concentrate=true\n"
        "  graph [truecolor=true bgcolor=\"#00000000\"]\n"
        "  node [style=filled fillcolor=\"" << fill_color << "\" color=\"" << border_color << "\" fontcolor=\"" << font_color << "\" fontname=\"Roboto Light\"]\n"
        "  edge [color=\"" << edge_color << "\"]\n";

  for (auto node : this->nodes) {
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
    s << "  node_" << idx << "[" << params << "];\n";
  }

  for (auto node : this->nodes) {
    u32 idx = node->node();

    auto inputs = this->input_edges.find(idx);
    if (inputs != this->input_edges.end()) {
      for (auto input : inputs->second) {
        s << "  node_" << input << " -> node_" << idx << ";\n";
      }
    }
  }
  printf("}\n");
  s << "}\n";
  return s.str();
}

// Shader Implementation
Shader::~Shader() {
  this->files.clear();
}

Shader::Shader(const char *name, const vector<string> &filenames, FrameGraph *framegraph) {
  this->node_type = NodeType::SHADER;
  this->name.assign(name);
  this->options = rawkit_shader_default_options();

  for (const auto &filename : filenames) {
    this->files.push_back(
      rawkit_file(filename.c_str())
    );
  }

  this->node(framegraph);
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

        case NodeType::TEXTURE: {
          if (!resource) {
            continue;
          }

          auto tex = (rawkit_texture_t *)resource;
          rawkit_shader_instance_param_texture(
            inst,
            param.name,
            tex,
            // TODO: pass in sampler
            nullptr
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
      this->framegraph->addInputEdge(
        param.node->node(this->framegraph),
        invocation->node()
      );
    }
  }

  return invocation;
}

// Pending Event Implementation
PendingEvent::PendingEvent(
  FrameGraphNode *node,
  VkCommandBuffer command_buffer,
  VkPipelineStageFlags stageMask,
  function<void(FrameGraphNode *)> cb
)
{
  if (!node || !node->framegraph || !node->framegraph->gpu) {
    printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " PendingEvent passed an invalid node\n");
    this->valid = false;
    return;
  }

  this->cb = cb;
  this->node = node;
  VkResult err = VK_SUCCESS;
  {
    VkEventCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    err = vkCreateEvent(
      node->framegraph->gpu->device,
      &info,
      node->framegraph->gpu->allocator,
      &this->handle
    );
    if (err) {
      printf(ANSI_CODE_RED "ERROR:" ANSI_CODE_RESET " PendingEvent failed to create event\n");
      this->valid = false;
      return;
    }
  }

  vkCmdSetEvent(
    command_buffer,
    this->handle,
    stageMask
  );
}

PendingEvent::~PendingEvent() {
  if (!this->handle && !this->valid) {
    return;
  }

  vkDestroyEvent(
    this->node->framegraph->gpu->device,
    this->handle,
    this->node->framegraph->gpu->allocator
  );
}

bool PendingEvent::complete() {
  VkResult status = vkGetEventStatus(
    this->node->framegraph->gpu->device,
    this->handle
  );

  if (status == VK_EVENT_SET) {
    this->cb(this->node);
    return true;
  }

  return false;
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
    copyRegion.srcOffset = allocation.physical_offset;
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
    copyRegion.dstOffset = allocation.physical_offset;

    vkCmdCopyBuffer(
      fg->command_buffer,
      src->handle,
      dest->handle,
      1,
      &copyRegion
    );

    auto event = new PendingEvent(
      node,
      fg->command_buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      [](FrameGraphNode *node) {
        auto dl = (Download<T> *)node;
        dl->cb(
          (T *)dl->allocation.data(),
          dl->allocation.size
        );
        dl->allocation.release();
      }
    );

    fg->pending_events.push_back(event);
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

// TextureDebugImGui Implementation
TextureDebugImGui::TextureDebugImGui(
  Texture *texture,
  FrameGraph *framegraph,
  vec2 pos,
  vec2 dims,
  ImDrawList *dl,
  rawkit_texture_sampler_t *sampler
)
{
  this->texture = texture;
  this->sampler = sampler;
  this->framegraph = framegraph;
  this->node_type = NodeType::DEBUG;

  this->draw_list = dl;
  this->dims = dims;
  this->pos = pos;

  this->name.assign(
    this->texture->name + "-debug-imgui"
  );

  this->_resolve_fn = [](FrameGraphNode *node) {
    auto that = (TextureDebugImGui *)node;
    auto src_tex = (rawkit_texture_t *)node->resource();
    if (!src_tex) {
      return;
    }

    float scale = 0.5;
    ImTextureID dst_tex = rawkit_imgui_texture(
      src_tex,
      that->sampler ? that->sampler : src_tex->default_sampler
    );
    if (!dst_tex) {
      return;
    }

    if (that->draw_list) {
      ImDrawList_AddImage(
        that->draw_list,
        dst_tex,
        { that->pos.x, that->pos.y },
        { that->pos.x + that->dims.x, that->pos.y + that->dims.y },
        { 0.0f, 0.0f },
        { 1.0f, 1.0f },
        0xFF000000
      );
    } else {
      igImage(
        dst_tex,
        (ImVec2){ (f32)that->dims.x, (f32)that->dims.y },
        (ImVec2){ 0.0f, 0.0f }, // uv0
        (ImVec2){ 1.0f, 1.0f }, // uv1
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
      );
    }
  };
}

rawkit_resource_t *TextureDebugImGui::resource() {
  if (!this->texture) {
    return nullptr;
  }
  return this->texture->resource();
}

// Texture Implementation
Texture::Texture(const char *name, VkFormat format, const uvec3 &dims, FrameGraph *framegraph) {
  this->framegraph = framegraph;
  this->node_type = NodeType::TEXTURE;
  this->name.assign(name);

  this->texture = rawkit_texture_mem(
    name,
    dims.x,
    dims.y,
    dims.z,
    format
  );
}

Texture::Texture(const char *filename, const char *relative_file, FrameGraph *framegraph) {
  this->framegraph = framegraph;
  this->node_type = NodeType::TEXTURE;
  this->name.assign(name);

  fs::path rel = fs::absolute(fs::path(relative_file));
  this->name.assign(
    fs::relative((rel / filename).string(), fs::path(RAWKIT_ENTRY_DIRNAME))
  );

  this->texture = _rawkit_texture_ex(
    framegraph->gpu,
    rel.string().c_str(),
    filename,
    uv_default_loop(),
    rawkit_default_diskwatcher()
  );
  this->node();
}

uvec3 Texture::dims() {
  if (!this->texture || !this->texture->resource_version) {
    return uvec3(0);
  }

  return uvec3(
    this->texture->options.width,
    this->texture->options.height,
    this->texture->options.depth
  );
}

rawkit_resource_t *Texture::resource() {
  return (rawkit_resource_t *)this->texture;
}


void Texture::debug_imgui(
  const vec2 &dims,
  rawkit_texture_sampler_t *sampler,
  const vec2 &pos,
  ImDrawList *dl
) {
  auto debug_node = new TextureDebugImGui(
    this,
    this->framegraph,
    pos,
    dims,
    dl
  );
  this->framegraph->addInputEdge(
    debug_node->node(),
    this->node()
  );
}
