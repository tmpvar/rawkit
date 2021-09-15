#include <stdarg.h>

#include <rawkit/hash.h>
#include <rawkit/hot.h>
#include <rawkit/diskwatcher.h>
#include "util.h"

static const rawkit_glsl_t ERR = {};
static const char *resource_name = "rawkit::glsl";

const rawkit_glsl_t *_rawkit_glsl_va(uint8_t count, ...) {
  if (!count) {
    return &ERR;
  }

  rawkit_file_t *files[256] = {};

  va_list vl;
  va_start(vl, count);
  for (uint32_t i=0; i<count; i++) {
    files[i] = va_arg(vl, rawkit_file_t *);
  }

  va_end(vl);

  return rawkit_glsl_file_array(count, (const rawkit_file_t **)files);
}

static bool compile_shader(glslang::TShader* shader, const char *name, const char* src, uint64_t len, GLSLIncluder &includer) {
  if (!shader || !name || !src) {
    return false;
  }

  int length = static_cast<int>(strlen(src));
  shader->setStringsWithLengthsAndNames(
    &src,
    &length,
    &name,
    1
  );

  shader->setEnvInput(
    glslang::EShSourceGlsl,
    shader->getStage(),
    glslang::EShClientOpenGL,
    450
  );

  shader->setEnvClient(
    glslang::EShClientVulkan,
    glslang::EShTargetVulkan_1_2
  );

  shader->setEnvTarget(
    glslang::EShTargetSpv,
    glslang::EShTargetSpv_1_5
  );



  string preamble =
    "\n#extension GL_GOOGLE_cpp_style_line_directive: enable\n"
    "#extension GL_GOOGLE_include_directive: enable\n";

  shader->setPreamble(preamble.c_str());

  shader->setAutoMapBindings(true);
  shader->setAutoMapLocations(true);
  shader->setEntryPoint("main");

  string output;

  TBuiltInResource resource_limits = get_default_resource_limits();

  shader->preprocess(
    &resource_limits,
    110,
    ENoProfile,
    false,
    false,
    EShMsgDefault,
    &output,
    includer
  );


  int r = shader->parse(
    &resource_limits,
    460,
    false,
    EShMsgRelaxedErrors,
    includer
  );

  if (!r) {
    printf("source after preprocessing:\n %s\n", output.c_str());
    printf("Debug log for %s\n%s\n", name, shader->getInfoDebugLog());
    printf("glslang: failed to parse shader %s\nLOG: %s",
      name,
      shader->getInfoLog()
    );

    return false;
  } else {
    printf("\e[0;32m" "rebuilt %s\n" "\e[0m", name);
  }

  return true;
}


const rawkit_glsl_t *rawkit_glsl_file_array(uint8_t file_count, const rawkit_file_t **files) {
  static bool initialized = false;
  if (!initialized) {
    glslang::InitializeProcess();
    initialized = true;
  }

  if (!file_count || !files) {
    return &ERR;
  }

  uint64_t id = rawkit_hash_resources(resource_name, file_count, (const rawkit_resource_t **)files);

  rawkit_glsl_t *glsl = rawkit_hot_resource_id(resource_name, id, rawkit_glsl_t);
  if (!glsl) {
    return &ERR;
  }

  bool dirty = rawkit_resource_sources_array(
    (rawkit_resource_t *)glsl,
    file_count,
    (rawkit_resource_t **)files
  );

  GLSLState *old_state = (GLSLState *)glsl->_state;
  auto watcher = rawkit_default_diskwatcher();

  // look for dependency changes
  if (old_state) {
    auto it = old_state->included_file_versions.begin();
    for (it; it != old_state->included_file_versions.end(); it++) {
      uint32_t current_version = rawkit_diskwatcher_file_version(
        watcher,
        it->first.c_str()
      );

      if (current_version != it->second) {
        dirty = true;
        break;
      }
    }
  }

  if (!dirty) {
    return glsl;
  }

  // attempt a compile
  GLSLState *state = new GLSLState();
  GLSLIncluder includer;
  for (uint8_t i = 0; i < file_count; i++) {
    const rawkit_file_t* file = files[i];
    if (!file || file->resource_version == 0) {
      return glsl;
    }

    includer.pushExternalLocalDirectory(
      fs::path(file->resource_name).remove_filename().string()
    );
  }



  glslang::TProgram program;

  // compile all of the provided sources
  vector<EShLanguage> stage_masks;
  vector<glslang::TShader *> shaders;
  {
    for (uint8_t i=0; i<file_count; i++) {
      const rawkit_file_t *file = files[i];

      EShLanguage stage = filename_to_stage(files[i]->resource_name);
      if (stage == EShLanguage::EShLangCount) {
        printf("ERROR: invalid stage language\n");
        return NULL;
      }

      if (stage == EShLanguage::EShLangCompute) {
        state->compute = true;
      }

      glslang::TShader *shader = new glslang::TShader(stage);
      bool compiled = compile_shader(
        shader,
        file->resource_name,
        (const char *)file->data,
        file->len,
        includer
      );

      if (!compiled) {
        return glsl;
      }

      program.addShader(shader);
      stage_masks.push_back(stage);
      shaders.push_back(shader);
    }
  }

  if (!program.link(EShMsgDefault)) {
    printf("glslang: unable to link\nLOG:%s\n",
      program.getInfoLog()
    );

    return glsl;
  }

  program.mapIO();

  glslang::SpvOptions options;
  options.generateDebugInfo = true;
  options.disableOptimizer = false;
  options.optimizeSize = false;
  options.validate = true;
  options.stripDebugInfo = false;

  for (auto &glslang_stage : stage_masks) {
    // Note the call to GlslangToSpv also populates compilation_output_data.
    std::vector<uint32_t> spirv_binary;
    glslang::GlslangToSpv(
      // TODO: compute spirv for each stage
      *program.getIntermediate(glslang_stage),
      spirv_binary,
      &options
    );

    RawkitStage *stage = new RawkitStage();
    state->stages.push_back(stage);
    // spv::Disassemble(std::cout, spirv_binary);

    stage->mask = glsl_language_to_stage_mask(glslang_stage);
    stage->len = spirv_binary.size();
    stage->bytes = stage->len * sizeof(uint32_t);
    stage->data = (uint32_t *)malloc(stage->bytes);
    memcpy(stage->data, spirv_binary.data(), stage->bytes);

    RawkitGLSLCompiler comp(std::move(spirv_binary), stage->mask);

    // Store: workgroup_size
    {
      auto entries = comp.get_entry_points_and_stages();
      for (auto &e : entries) {
        auto &ep =  comp.get_entry_point(e.name, e.execution_model);
        spirv_cross::SpecializationConstant x, y, z;
        comp.get_work_group_size_specialization_constants(x, y, z);

        stage->workgroup_size[0] = (x.id != spirv_cross::ID(0)) ? x.constant_id : ep.workgroup_size.x;
        stage->workgroup_size[1] = (y.id != spirv_cross::ID(0)) ? y.constant_id : ep.workgroup_size.y;
        stage->workgroup_size[2] = (z.id != spirv_cross::ID(0)) ? z.constant_id : ep.workgroup_size.z;
        break;
      }
    }

    unordered_map<spirv_cross::TypeID, spirv_cross::SPIRType> types;

    comp.populate_reflection(state);
    if (!state->valid) {
      delete state;
      return glsl;
    }
    // comp.set_format("json");
    // auto json = comp.compile();
    // printf("reflection:\n%s\n", json.c_str());
  }

  if (old_state) {
    delete old_state;
  }

  for (const auto &dep : includer.dependencies) {
    state->included_file_versions[dep] = rawkit_diskwatcher_file_version(
      watcher,
      dep.c_str()
    );
  }

  glsl->_state = (void *)state;
  glsl->resource_version++;
  return glsl;
}


static inline GLSLState *get_state(const rawkit_glsl_t *ref) {
  if (!ref || !ref->_state) {
    return NULL;
  }

  return (GLSLState *)ref->_state;
}

bool rawkit_glsl_valid(const rawkit_glsl_t *ref) {
  GLSLState *state = get_state(ref);
  return state && state->valid;
}

static const RawkitStage *get_stage(const rawkit_glsl_t *ref, uint8_t index) {
  GLSLState *state = get_state(ref);
  if (!state || index >= state->stages.size()) {
    return NULL;
  }

  return state->stages[index];
}

const uint32_t *rawkit_glsl_spirv_data(const rawkit_glsl_t *ref, uint8_t index) {
  const RawkitStage *stage = get_stage(ref, index);
  if (!stage) {
    return NULL;
  }
  return stage->data;
}

const uint64_t rawkit_glsl_spirv_byte_len(const rawkit_glsl_t *ref,uint8_t index) {
  const RawkitStage *stage = get_stage(ref, index);
  if (!stage) {
    return 0;
  }
  return stage->bytes;
}

const uint32_t *rawkit_glsl_workgroup_size(const rawkit_glsl_t *ref, uint8_t index) {
  const RawkitStage *stage = get_stage(ref, index);
  if (!stage) {
    return NULL;
  }
  return stage->workgroup_size;
}

rawkit_glsl_stage_mask rawkit_glsl_stage_at_index(const rawkit_glsl_t *ref, uint8_t index) {
  const RawkitStage *stage = get_stage(ref, index);
  if (!stage) {
    return RAWKIT_GLSL_STAGE_NONE_BIT;
  }
  return stage->mask;
}


uint8_t rawkit_glsl_stage_count(const rawkit_glsl_t *ref) {
  GLSLState *state = get_state(ref);
  if (!state) {
    return 0;
  }

  return static_cast<uint8_t>(state->stages.size());
}

const uint32_t rawkit_glsl_reflection_descriptor_set_max(const rawkit_glsl_t* ref) {
  GLSLState *state = get_state(ref);
  if (!state) {
    printf("rawkit_glsl_reflection_descriptor_set_max: null ref\n");
    return 0;
  }

  return static_cast<uint32_t>(state->bindings_per_set.size());
}

const uint32_t rawkit_glsl_reflection_binding_count_for_set(const rawkit_glsl_t* ref, uint32_t set) {
  GLSLState *state = get_state(ref);
  if (!state) {
    return 0;
  }

  auto it = state->bindings_per_set.find(set);
  if (it == state->bindings_per_set.end()) {
    return 0;
  }

  return it->second;
}

const rawkit_glsl_reflection_entry_t rawkit_glsl_reflection_entry(const rawkit_glsl_t *ref, const char *name) {
  GLSLState *state = get_state(ref);
  rawkit_glsl_reflection_entry_t entry = {};
  entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_NOT_FOUND;

  if (!state) {
    return entry;
  }

  auto it = state->reflection_name_to_index.find(name);
  if (it == state->reflection_name_to_index.end()) {
    return entry;
  }

  if (state->reflection_entries.size() <= it->second) {
    return entry;
  }

  return state->reflection_entries[it->second];
}


const rawkit_glsl_reflection_vector_t rawkit_glsl_reflection_entries(const rawkit_glsl_t* ref) {
  GLSLState *state = get_state(ref);
  rawkit_glsl_reflection_vector_t vec = {};
  if (!state) {
    return vec;
  }

  vec.len = static_cast<uint32_t>(state->reflection_entries.size());
  vec.entries = state->reflection_entries.data();

  return vec;
}

bool rawkit_glsl_is_compute(const rawkit_glsl_t *ref) {
  GLSLState *state = get_state(ref);
  if (!state) {
    return false;
  }

  return state->compute;
}
