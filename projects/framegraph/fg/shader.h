#pragma once

#include "buffer.h"

#include <vector>
#include <functional>
using namespace std;

struct Shader {
  rawkit_shader_t *handle = nullptr;
  vector<const rawkit_file_t *> files;
  rawkit_shader_options_t options;

  vector<std::function<void (rawkit_shader_instance_t *)>> io;

  ~Shader() {
    this->files.clear();
  }

  Shader(const char *name, const vector<const char *> &filenames) {
    this->options = rawkit_shader_default_options();

    for (const char *filename : filenames) {
      this->files.push_back(
        rawkit_file_relative_to(filename, RAWKIT_ENTRY_DIRNAME)
      );
    }
  }

  template <typename T>
  Shader &buffer(const char *name, Buffer<T> &buffer) {
    // TODO: register this with the FrameGraph
    this->io.push_back([name, &buffer, this](rawkit_shader_instance_t *inst) {
      // TODO: this is where we'd figure out if we need to add a barrier and
      //       add it to a queue or something
      rawkit_shader_instance_param_buffer(inst, name, buffer.handle());
    });
    return *this;
  }

  void rebuild() {
    this->handle = rawkit_shader_ex(
        rawkit_default_gpu(),
        rawkit_vulkan_renderpass(),
        nullptr,
        files.size(),
        files.data()
    );
  }

  // TODO: some sugar `dispatchOver(texture)`

  void dispatch(u32 x, u32 y = 1, u32 z = 1) {

    this->rebuild();

    rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
      rawkit_default_gpu(),
      this->handle,
      NULL,
      0
    );

    if (inst) {
      for (auto entry : this->io) {
        entry(inst);
      }

      rawkit_shader_instance_dispatch_compute(
        inst,
        x,
        y,
        z
      );

      rawkit_shader_instance_end(inst);
    }

  }
};