#pragma once

#include <rawkit/glsl.h>

#include <glslang/Public/ShaderLang.h>

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

class RawkitStage {
  public:
    RawkitStage() {}
    uint32_t *data = nullptr;
    uint64_t len = 0;
    uint64_t bytes = 0;
    uint32_t workgroup_size[3] = {0, 0, 0};
    rawkit_glsl_stage_mask mask = RAWKIT_GLSL_STAGE_NONE_BIT;
};

class GLSLState {
  public:
    bool compute = false;
    bool valid = true;
    vector<rawkit_glsl_reflection_entry_t> reflection_entries;
    unordered_map<string, uint64_t> reflection_name_to_index;
    unordered_map<uint64_t, string> binding_offset;

    // the number of bindings per descriptor set
    unordered_map<uint32_t, uint32_t> bindings_per_set;
    vector<RawkitStage *> stages;

    unordered_map<string, uint32_t> included_file_versions;

    ~GLSLState() {
      for (auto &entry : this->reflection_entries) {
        free(entry.name);
      }
    }
};



struct rawkit_glsl_compile_status_t {
  bool valid;
  string name;
  string log;
};

typedef void (*rawkit_glsl_compile_callback_t)(rawkit_glsl_compile_status_t status);
void rawkit_glsl_set_global_status_callback(rawkit_glsl_compile_callback_t cb);
