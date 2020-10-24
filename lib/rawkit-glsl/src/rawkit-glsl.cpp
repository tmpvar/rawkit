#include <rawkit/glsl.h>

#include "../internal/includer.h"
#include "../internal/defaults.h"

#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvTools.h>
#include <SPIRV/disassemble.h>
#include <glslang/Public/ShaderLang.h>


#include <string.h>
#include <string>
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

static EShLanguage filename_to_stage(string filename) {
  fs::path p = filename;
  string ext = p.extension().string();
  if (ext == ".frag") {
    return EShLanguage::EShLangFragment;
  } else if (ext == ".comp") {
    return EShLanguage::EShLangCompute;
  } else if (ext == ".vert") {
    return EShLanguage::EShLangVertex;
  } else if (ext == ".geom") {
    return EShLanguage::EShLangGeometry;
  }

  // TODO: set an error string or something somewhere

  return EShLanguage::EShLangCount;
}

rawkit_glsl_t *rawkit_glsl_compile(const char *name, const char *src, const rawkit_glsl_paths_t *include_dirs) {
  if (!name || !src) {
    return NULL;
  }

  EShLanguage stage = filename_to_stage(name);
  if (stage == EShLanguage::EShLangCount) {
    return NULL;
  }

  static bool initialized = false;
  if (!initialized) {
    glslang::InitializeProcess();

    initialized = true;
  }

  rawkit_glsl_t *ret = (rawkit_glsl_t *)calloc(sizeof(rawkit_glsl_t), 1);
  if (!ret) {
    return NULL;
  }


  glslang::TShader shader(stage);

  int length = static_cast<int>(strlen(src));
  shader.setStringsWithLengths(
    &src,
    &length,
    1
  );

  shader.setEnvInput(
    glslang::EShSourceGlsl,
    stage,
    glslang::EShClientOpenGL,
    450
  );

  shader.setEnvClient(
    glslang::EShClientVulkan,
    glslang::EShTargetVulkan_1_2
  );

  shader.setEnvTarget(
    glslang::EShTargetSpv,
    glslang::EShTargetSpv_1_3
  );

  shader.setPreamble(
    "\n#extension GL_GOOGLE_include_directive: enable\n"
  );

  shader.setAutoMapBindings(true);
  shader.setAutoMapLocations(true);
  shader.setEntryPoint("main");

  string output;
  GLSLIncluder includer;

  if (include_dirs && include_dirs->count) {
    for (uint32_t i=0; i<include_dirs->count; i++) {
      const char *include = include_dirs->entry[i];
      if (include != nullptr) {
        includer.pushExternalLocalDirectory(include);
      }
    }
  }

  TBuiltInResource resource_limits = get_default_resource_limits();

  shader.preprocess(
    &resource_limits,
    110,
    ENoProfile,
    false,
    false,
    EShMsgDefault,
    &output,
    includer
  );

  int r = shader.parse(
    &resource_limits,
    450,
    false,
    EShMsgRelaxedErrors,
    includer
  );

  printf("Debug log for %s\n%s\n", name, shader.getInfoDebugLog());

  if (!r) {
    printf("glslang: failed to parse shader %s\nLOG: %s",
      name,
      shader.getInfoLog()
    );

    return ret;
  }

  glslang::TProgram program;
  program.addShader(&shader);
  if (!program.link(EShMsgDefault)) {
    printf("glslang: unable to link\nLOG:%s\n",
      program.getInfoLog()
    );
    return ret;
  }

  program.mapIO();

  program.buildReflection(
    EShReflectionDefault /* |
    EShReflectionSeparateBuffers |
    EShReflectionAllBlockVariables |
    EShReflectionUnwrapIOBlocks*/
  );
  //program.dumpReflection();

  glslang::SpvOptions options;
  options.generateDebugInfo = true;
  options.disableOptimizer = true;
  options.optimizeSize = false;
  options.validate = true;


  // Note the call to GlslangToSpv also populates compilation_output_data.
  std::vector<uint32_t> spirv;
  glslang::GlslangToSpv(
    *program.getIntermediate(stage),
    spirv,
    &options
  );

  // spv::Disassemble(std::cout, spirv);
  ret->len = spirv.size();
  ret->bytes = ret->len * sizeof(uint32_t);
  ret->data = (uint32_t *)malloc(ret->bytes);
  memcpy(ret->data, spirv.data(), ret->bytes);

  ret->valid = true;
  return ret;
}

void rawkit_glsl_paths_destroy(rawkit_glsl_paths_t *paths) {
  if (paths->count) {
    for (uint32_t i=0; paths->count; i++) {
      free(paths->entry[i]);
      paths->entry[i] = NULL;
    }
    paths->count = 0;
    free(paths->entry);
    paths->entry = NULL;
  }
}

void rawkit_glsl_destroy(rawkit_glsl_t *ref) {
  if (!ref) {
    return;
  }

  if (ref->len && ref->data) {
    free(ref->data);
    ref->len = 0;
    ref->data = NULL;
  }

  rawkit_glsl_paths_destroy(&ref->included_files);
}

static void val_destroy_fn(ps_handle_t *base) {
  if (!base) {
    return;
  }

  ps_val_t *v = (ps_val_t *)base;
  if (v->data == NULL || !v->len || v->handle_type != PS_HANDLE_VALUE) {
    return;
  }

  rawkit_glsl_destroy((rawkit_glsl_t *)v->data);
  free(base);
}

typedef struct ps_rawkit_glsl_t {
  PS_FIELDS

  char *name;
  rawkit_glsl_paths_t include_dirs;
} ps_rawkit_glsl_t;

static void stream_destroy_fn(ps_handle_t *base) {
  if (!base) {
    return;
  }

  ps_rawkit_glsl_t *s = (ps_rawkit_glsl_t *)base;
  if (!s->name) {
    free(s->name);
    s->name = NULL;
  }

  free(s);
}

static ps_val_t *rawkit_glsl_read_fn(ps_t *base, ps_stream_status status) {
  if (ps_status(base, status)) {
    return NULL;
  }

  ps_val_t *input = ps_pull(base, PS_OK);

  // if the pull caused the stream to go ERR/DONE
  if (base->status != PS_OK) {
    // This should never happen
    if (input) {
      ps_destroy(input);
      abort();
    }
    return NULL;
  }

  if (!input || !input->len) {
    return NULL;
  }

  ps_rawkit_glsl_t *s = (ps_rawkit_glsl_t *)base;
  rawkit_glsl_t *r = rawkit_glsl_compile(
    s->name,
    (const char *)input->data,
    &s->include_dirs
  );

  if (!r) {
    return NULL;
  }

  ps_destroy(input);
  ps_val_t *output = ps_create_value(ps_val_t, val_destroy_fn);
  output->data = (void *)r;
  output->len = sizeof(rawkit_glsl_t);
  return output;
}

ps_t *rawkit_glsl_compiler(const char *name, const rawkit_glsl_paths_t *includes) {
  ps_rawkit_glsl_t *s = ps_create_stream(ps_rawkit_glsl_t, stream_destroy_fn);

  s->name = strdup(name);
  s->include_dirs.entry = (char **)malloc(sizeof(char *) * includes->count);

  for (uint32_t i=0; includes->count; i++) {
    s->include_dirs.entry[i] = strdup(includes->entry[i]);
  }
  s->include_dirs.count = includes->count;

  s->fn = rawkit_glsl_read_fn;

  return (ps_t *)s;
}
