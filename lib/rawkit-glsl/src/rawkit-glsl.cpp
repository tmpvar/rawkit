#include <rawkit/glsl.h>

#include "../internal/includer.h"
#include "../internal/defaults.h"

#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvTools.h>
#include <SPIRV/disassemble.h>
#include <glslang/Public/ShaderLang.h>

#include <spirv_cross.hpp>
#include <spirv_reflect.hpp>


#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

class RawkitStage {
  public:
    RawkitStage() {}
    uint32_t *data = nullptr;
    uint64_t len = 0;
    uint64_t bytes = 0;
    uint32_t workgroup_size[3] = {0, 0, 0};
    rawkit_glsl_stage_mask mask = RAWKIT_GLSL_STAGE_NONE_BIT;
};

typedef struct rawkit_glsl_t {
  bool valid;
  bool compute;
  rawkit_glsl_paths_t included_files;

  vector<rawkit_glsl_reflection_entry_t> *reflection_entries;
  unordered_map<string, uint64_t> *reflection_name_to_index;
  unordered_map<uint64_t, string> *binding_offset;

  // the number of bindings per descriptor set
  unordered_map<uint32_t, uint32_t> *bindings_per_set;
  vector<RawkitStage *> *stages;
} rawkit_glsl_t;

bool rawkit_glsl_valid(const rawkit_glsl_t *ref) {
  return ref && ref->valid;
}

static const RawkitStage *get_stage(const rawkit_glsl_t *ref, uint8_t index) {
  if (!ref || !ref->stages || index >= ref->stages->size()) {
    return NULL;
  }

  return (*ref->stages)[index];
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
  if (!ref || !ref->stages) {
    return 0;
  }

  return static_cast<uint8_t>(ref->stages->size());
}


const uint32_t rawkit_glsl_reflection_descriptor_set_max(const rawkit_glsl_t* ref) {
  if (!ref || !ref->bindings_per_set) {
    printf("rawkit_glsl_reflection_descriptor_set_max: null ref\n");
    return 0;
  }

  return static_cast<uint32_t>(ref->bindings_per_set->size());
}

const uint32_t rawkit_glsl_reflection_binding_count_for_set(const rawkit_glsl_t* ref, uint32_t set) {
  if (!ref || !ref->bindings_per_set) {
    return 0;
  }

  auto it = ref->bindings_per_set->find(set);
  if (it == ref->bindings_per_set->end()) {
    return 0;
  }

  return it->second;
}


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

const rawkit_glsl_reflection_entry_t rawkit_glsl_reflection_entry(const rawkit_glsl_t *ref, const char *name) {
  rawkit_glsl_reflection_entry_t entry = {};
  entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_NOT_FOUND;

  if (!ref || !ref->reflection_entries || !ref->reflection_name_to_index) {
    return entry;
  }

  auto it = ref->reflection_name_to_index->find(name);
  if (it == ref->reflection_name_to_index->end()) {
    return entry;
  }

  if (ref->reflection_entries->size() <= it->second) {
    return entry;
  }

  entry = ref->reflection_entries->at(it->second);

  return entry;
}


const rawkit_glsl_reflection_vector_t rawkit_glsl_reflection_entries(const rawkit_glsl_t* ref) {
  rawkit_glsl_reflection_vector_t vec = {};
  if (!ref || !ref->reflection_entries) {
    return vec;
  }

  vec.len = static_cast<uint32_t>(ref->reflection_entries->size());
  vec.entries = ref->reflection_entries->data();

  return vec;
}

static bool rawkit_glsl_reflection_add_entry(rawkit_glsl_t* glsl, string name, rawkit_glsl_reflection_entry_t entry) {
  if (!glsl) {
    printf("ERROR: glsl was null\n");
    return false;
  }

  auto it = glsl->reflection_name_to_index->find(name);
  if (it != glsl->reflection_name_to_index->end()) {
    if (it->second >= glsl->reflection_entries->size()) {
      printf("ERROR: reflection name map pointed to entry that is out of range\n");
      return false;
    }

    // TODO: if any of the other fields differ we have a problem
    (*glsl->reflection_entries)[it->second].stage |= entry.stage;
    return true;
  }

  uint64_t idx = glsl->reflection_entries->size();
  glsl->reflection_name_to_index->emplace(name, idx);
  glsl->reflection_entries->push_back(entry);

  // Keep a running total of how many entries there are in each
  // descriptor set. This allows us to cleanly separate rawkit-glsl
  // from rawkit-shader and rawkit-gpu.
  {
    // skip entries that do not have a valid descriptor set number
    if (entry.set < 0 || entry.binding < 0) {
      return true;
    }

    {
      uint64_t key = (entry.set << 16) | entry.binding;
      auto it = glsl->binding_offset->find(key);
      if (it == glsl->binding_offset->end()) {
        glsl->binding_offset->emplace(key, name);
      } else {
        // TODO: this is not an error if the resource is an array
        // TODO: add the shader file name(s) where this occurred
        printf("ERROR: set(%u) && binding(%u) combination for '%s' already specified for '%s'\n",
          entry.set,
          entry.binding,
          name.c_str(),
          it->second.c_str()
        );

        glsl->valid = false;
        return false;
      }
    }

    uint32_t set = static_cast<uint32_t>(entry.set);
    auto it = glsl->bindings_per_set->find(set);

    if (it == glsl->bindings_per_set->end()) {
      glsl->bindings_per_set->emplace(set, 1);
    } else {
      it->second++;
    }
  }

  return true;
}

class RawkitGLSLCompiler : public spirv_cross::CompilerReflection {
  rawkit_glsl_stage_mask stage_mask = RAWKIT_GLSL_STAGE_NONE_BIT;

  public:
    RawkitGLSLCompiler(std::vector<uint32_t> ir, rawkit_glsl_stage_mask stage_mask)
      : CompilerReflection(ir),
        stage_mask(stage_mask)
    {

    }

    void populate_push_constants(rawkit_glsl_t *glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      // NOTE: there should only ever be one of these.
      for (auto& buffer : res.push_constant_buffers) {
        auto& type = this->get_type(buffer.type_id);
        auto& base_type = this->get_type(buffer.base_type_id);

        size_t size = type.member_types.size();
        for (uint32_t i = 0; i < size; ++i) {
          string name = this->get_member_name(buffer.base_type_id, i);

          rawkit_glsl_reflection_entry_t entry = {};
          entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER;
          entry.stage = this->stage_mask;
          entry.location = -1;
          entry.set = -1;
          entry.binding = -1;
          entry.readable = true;
          entry.writable = false;
          entry.block_size = static_cast<uint32_t>(
            this->get_declared_struct_member_size(base_type, i)
          );

          //static_cast<uint32_t>(this->get_declared_struct_size(type));
          entry.offset = this->type_struct_member_offset(base_type, i);
          if (!rawkit_glsl_reflection_add_entry(glsl, name, entry)) {
            return;
          }
        }
      }
    }

    static rawkit_glsl_image_format image_format(spv::ImageFormat input) {
      switch (input) {
          case spv::ImageFormatUnknown: return RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN;
          case spv::ImageFormatRgba32f: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA32F;
          case spv::ImageFormatRgba16f: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA16F;
          case spv::ImageFormatR32f: return RAWKIT_GLSL_IMAGE_FORMAT_R32F;
          case spv::ImageFormatRgba8: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA8;
          case spv::ImageFormatRgba8Snorm: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA8SNORM;
          case spv::ImageFormatRg32f: return RAWKIT_GLSL_IMAGE_FORMAT_RG32F;
          case spv::ImageFormatRg16f: return RAWKIT_GLSL_IMAGE_FORMAT_RG16F;
          case spv::ImageFormatR11fG11fB10f: return RAWKIT_GLSL_IMAGE_FORMAT_R11FG11FB10F;
          case spv::ImageFormatR16f: return RAWKIT_GLSL_IMAGE_FORMAT_R16F;
          case spv::ImageFormatRgba16: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA16;
          case spv::ImageFormatRgb10A2: return RAWKIT_GLSL_IMAGE_FORMAT_RGB10A2;
          case spv::ImageFormatRg16: return RAWKIT_GLSL_IMAGE_FORMAT_RG16;
          case spv::ImageFormatRg8: return RAWKIT_GLSL_IMAGE_FORMAT_RG8;
          case spv::ImageFormatR16: return RAWKIT_GLSL_IMAGE_FORMAT_R16;
          case spv::ImageFormatR8: return RAWKIT_GLSL_IMAGE_FORMAT_R8;
          case spv::ImageFormatRgba16Snorm: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA16SNORM;
          case spv::ImageFormatRg16Snorm: return RAWKIT_GLSL_IMAGE_FORMAT_RG16SNORM;
          case spv::ImageFormatRg8Snorm: return RAWKIT_GLSL_IMAGE_FORMAT_RG8SNORM;
          case spv::ImageFormatR16Snorm: return RAWKIT_GLSL_IMAGE_FORMAT_R16SNORM;
          case spv::ImageFormatR8Snorm: return RAWKIT_GLSL_IMAGE_FORMAT_R8SNORM;
          case spv::ImageFormatRgba32i: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA32I;
          case spv::ImageFormatRgba16i: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA16I;
          case spv::ImageFormatRgba8i: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA8I;
          case spv::ImageFormatR32i: return RAWKIT_GLSL_IMAGE_FORMAT_R32I;
          case spv::ImageFormatRg32i: return RAWKIT_GLSL_IMAGE_FORMAT_RG32I;
          case spv::ImageFormatRg16i: return RAWKIT_GLSL_IMAGE_FORMAT_RG16I;
          case spv::ImageFormatRg8i: return RAWKIT_GLSL_IMAGE_FORMAT_RG8I;
          case spv::ImageFormatR16i: return RAWKIT_GLSL_IMAGE_FORMAT_R16I;
          case spv::ImageFormatR8i: return RAWKIT_GLSL_IMAGE_FORMAT_R8I;
          case spv::ImageFormatRgba32ui: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA32UI;
          case spv::ImageFormatRgba16ui: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA16UI;
          case spv::ImageFormatRgba8ui: return RAWKIT_GLSL_IMAGE_FORMAT_RGBA8UI;
          case spv::ImageFormatR32ui: return RAWKIT_GLSL_IMAGE_FORMAT_R32UI;
          case spv::ImageFormatRgb10a2ui: return RAWKIT_GLSL_IMAGE_FORMAT_RGB10A2UI;
          case spv::ImageFormatRg32ui: return RAWKIT_GLSL_IMAGE_FORMAT_RG32UI;
          case spv::ImageFormatRg16ui: return RAWKIT_GLSL_IMAGE_FORMAT_RG16UI;
          case spv::ImageFormatRg8ui: return RAWKIT_GLSL_IMAGE_FORMAT_RG8UI;
          case spv::ImageFormatR16ui: return RAWKIT_GLSL_IMAGE_FORMAT_R16UI;
          case spv::ImageFormatR8ui: return RAWKIT_GLSL_IMAGE_FORMAT_R8UI;
          case spv::ImageFormatMax: return RAWKIT_GLSL_IMAGE_FORMAT_MAX;
          default:
            return RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN;
      }
    }

    static rawkit_glsl_dims get_image_dims(const spirv_cross::SPIRType &type) {
      switch (type.image.dim) {
        case spv::Dim1D: return RAWKIT_GLSL_DIMS_1D;
        case spv::Dim2D: return RAWKIT_GLSL_DIMS_2D;
        case spv::Dim3D: return RAWKIT_GLSL_DIMS_3D;
        case spv::DimCube: return RAWKIT_GLSL_DIMS_CUBE;
        case spv::DimRect: return RAWKIT_GLSL_DIMS_RECT;
        case spv::DimBuffer: return RAWKIT_GLSL_DIMS_BUFFER;
        case spv::DimSubpassData: return RAWKIT_GLSL_DIMS_SUBPASS_DATA;
        case spv::DimMax: return RAWKIT_GLSL_DIMS_MAX;
        default: return RAWKIT_GLSL_DIMS_MAX;
      }
    }

    void populate_storage_images(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& image : res.storage_images) {
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE;
        entry.stage = this->stage_mask;
        entry.location = this->get_decoration(image.id, spv::DecorationLocation);
        entry.set = this->get_decoration(image.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(image.id, spv::DecorationBinding);
        entry.offset = -1;

        spirv_cross::Bitset mask = this->get_decoration_bitset(image.id);
        entry.readable = !mask.get(spv::DecorationNonReadable);
        entry.writable = !mask.get(spv::DecorationNonWritable);

        auto type = this->get_type(image.type_id);
        entry.image_format = RawkitGLSLCompiler::image_format(type.image.format);
        entry.dims = RawkitGLSLCompiler::get_image_dims(type);

        if (!rawkit_glsl_reflection_add_entry(glsl, image.name, entry)) {
          return;
        }
      }
    }

    void populate_textures(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& texture : res.sampled_images) {
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE;
        entry.stage = this->stage_mask;
        entry.image_format = RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN;
        entry.offset = -1;
        entry.readable = true;
        entry.writable = false;
        entry.location = this->get_decoration(texture.id, spv::DecorationLocation);
        entry.set = this->get_decoration(texture.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(texture.id, spv::DecorationBinding);

        auto type = this->get_type(texture.type_id);
        entry.dims = RawkitGLSLCompiler::get_image_dims(type);

        if (!rawkit_glsl_reflection_add_entry(glsl, texture.name, entry)) {
          return;
        }
      }
    }

    void populate_ssbos(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& ssbo : res.storage_buffers) {
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER;
        entry.stage = this->stage_mask;
        entry.image_format = RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN;
        entry.dims = RAWKIT_GLSL_DIMS_BUFFER;
        entry.offset = -1;

        entry.location = this->get_decoration(ssbo.id, spv::DecorationLocation);
        entry.set = this->get_decoration(ssbo.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(ssbo.id, spv::DecorationBinding);

        auto type = this->get_type(ssbo.type_id);
        auto base_type = this->get_type(ssbo.base_type_id);
        auto mask = this->get_buffer_block_flags(ssbo.id);
        entry.readable = !mask.get(spv::DecorationNonReadable);
        entry.writable = !mask.get(spv::DecorationNonWritable);

        if (!base_type.member_types.empty()) {
  	      entry.block_size = this->get_declared_struct_size(base_type);
        }

        if (!rawkit_glsl_reflection_add_entry(glsl, ssbo.name, entry)) {
          return;
        }
      }
    }

    void populate_separate_images(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& image : res.separate_images) {
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_IMAGE;
        entry.stage = this->stage_mask;
        entry.location = this->get_decoration(image.id, spv::DecorationLocation);
        entry.set = this->get_decoration(image.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(image.id, spv::DecorationBinding);
        entry.offset = -1;

        spirv_cross::Bitset mask = this->get_decoration_bitset(image.id);
        entry.readable = !mask.get(spv::DecorationNonReadable);
        entry.writable = !mask.get(spv::DecorationNonWritable);

        auto type = this->get_type(image.type_id);
        entry.image_format = RawkitGLSLCompiler::image_format(type.image.format);
        entry.dims = RawkitGLSLCompiler::get_image_dims(type);

        if (!rawkit_glsl_reflection_add_entry(glsl, image.name, entry)) {
          return;
        }
      }
    }

    void populate_separate_samplers(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& sampler : res.separate_samplers) {
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_SAMPLER;
        entry.stage = this->stage_mask;
        entry.location = this->get_decoration(sampler.id, spv::DecorationLocation);
        entry.set = this->get_decoration(sampler.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(sampler.id, spv::DecorationBinding);
        entry.offset = -1;

        spirv_cross::Bitset mask = this->get_decoration_bitset(sampler.id);
        entry.readable = !mask.get(spv::DecorationNonReadable);
        entry.writable = !mask.get(spv::DecorationNonWritable);

        auto type = this->get_type(sampler.type_id);
        entry.image_format = RawkitGLSLCompiler::image_format(type.image.format);
        entry.dims = RawkitGLSLCompiler::get_image_dims(type);

        if (!rawkit_glsl_reflection_add_entry(glsl, sampler.name, entry)) {
          return;
        }
      }
    }

    void populate_uniform_buffers(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& ubo : res.uniform_buffers) {
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER;
        entry.stage = this->stage_mask;
        entry.image_format = RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN;
        entry.dims = RAWKIT_GLSL_DIMS_BUFFER;
        entry.offset = -1;

        entry.location = this->get_decoration(ubo.id, spv::DecorationLocation);
        entry.set = this->get_decoration(ubo.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(ubo.id, spv::DecorationBinding);

        auto type = this->get_type(ubo.type_id);
        auto base_type = this->get_type(ubo.base_type_id);
        auto mask = this->get_buffer_block_flags(ubo.id);
        entry.readable = !mask.get(spv::DecorationNonReadable);
        entry.writable = !mask.get(spv::DecorationNonWritable);

        if (!base_type.member_types.empty()) {
  	      entry.block_size = this->get_declared_struct_size(base_type);
        }

        if (!rawkit_glsl_reflection_add_entry(glsl, ubo.name, entry)) {
          return;
        }
      }
    }

    void populate_inputs(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& input : res.stage_inputs) {
        auto& type = this->get_type(input.type_id);
        auto& base_type = this->get_type(input.base_type_id);

        size_t size = type.member_types.size();
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_INPUT;
        entry.stage = this->stage_mask;
        entry.location = this->get_decoration(input.id, spv::DecorationLocation);
        entry.set = -1;
        entry.binding = this->get_decoration(input.id, spv::DecorationBinding);
        entry.offset = this->get_decoration(input.id, spv::DecorationOffset);
        entry.input_attachment_index = this->get_decoration(input.id, spv::DecorationInputAttachmentIndex);
        entry.readable = true;
        entry.writable = false;

        entry.base_type = (rawkit_glsl_base_type)type.basetype;
        entry.columns = type.columns;
        entry.vecsize = type.vecsize;

        if (!base_type.member_types.empty()) {
          entry.block_size = this->get_declared_struct_size(base_type);
        } else {
          // compute the block size from the bit width and component count
          entry.block_size = (type.width / 8) * type.vecsize * type.columns;
        }

        if (!rawkit_glsl_reflection_add_entry(glsl, input.name, entry)) {
          return;
        }
      }
    }

    void populate_outputs(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& output : res.stage_outputs) {
        auto& type = this->get_type(output.type_id);
        auto& base_type = this->get_type(output.base_type_id);

        size_t size = type.member_types.size();
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_OUTPUT;
        entry.stage = this->stage_mask;
        entry.location = this->get_decoration(output.id, spv::DecorationLocation);
        entry.set = -1;
        entry.binding = -1;
        entry.readable = false;
        entry.writable = true;

        if (!rawkit_glsl_reflection_add_entry(glsl, output.name, entry)) {
          return;
        }
      }
    }

    void populate_subpass_inputs(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& input : res.subpass_inputs) {
        auto& type = this->get_type(input.type_id);
        auto& base_type = this->get_type(input.base_type_id);

        size_t size = type.member_types.size();
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_SUBPASS_INPUT;
        entry.location = this->get_decoration(input.id, spv::DecorationLocation);
        entry.set = this->get_decoration(input.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(input.id, spv::DecorationBinding);
        entry.input_attachment_index = this->get_decoration(input.id, spv::DecorationInputAttachmentIndex);

        entry.readable = true;
        entry.writable = false;

        if (!rawkit_glsl_reflection_add_entry(glsl, input.name, entry)) {
          return;
        }
      }
    }

    void populate_acceleration_structures(rawkit_glsl_t* glsl) {
      spirv_cross::ShaderResources res = this->get_shader_resources();
      for (auto& acc : res.acceleration_structures) {
        auto& type = this->get_type(acc.type_id);
        auto& base_type = this->get_type(acc.base_type_id);

        size_t size = type.member_types.size();
        rawkit_glsl_reflection_entry_t entry = {};
        entry.entry_type = RAWKIT_GLSL_REFLECTION_ENTRY_ACCELERATION_STRUCTURE;
        entry.stage = this->stage_mask;
        entry.location = this->get_decoration(acc.id, spv::DecorationLocation);
        entry.set = this->get_decoration(acc.id, spv::DecorationDescriptorSet);
        entry.binding = this->get_decoration(acc.id, spv::DecorationBinding);
        entry.input_attachment_index = this->get_decoration(acc.id, spv::DecorationInputAttachmentIndex);

        entry.readable = true;
        entry.writable = false;

        if (!rawkit_glsl_reflection_add_entry(glsl, acc.name, entry)) {
          return;
        }
      }
    }

    void populate_reflection(rawkit_glsl_t *ret) {
      this->populate_push_constants(ret);
      this->populate_storage_images(ret);
      this->populate_textures(ret);
      this->populate_ssbos(ret);
      this->populate_separate_images(ret);
      this->populate_separate_samplers(ret);
      this->populate_uniform_buffers(ret);
      this->populate_inputs(ret);
      this->populate_outputs(ret);
      this->populate_subpass_inputs(ret);
      this->populate_acceleration_structures(ret);
    }
};

static rawkit_glsl_stage_mask glsl_language_to_stage_mask(EShLanguage lang) {
  switch (lang) {
    case EShLangVertex: return RAWKIT_GLSL_STAGE_VERTEX_BIT;
    case EShLangTessControl: return RAWKIT_GLSL_STAGE_TESSELLATION_CONTROL_BIT ;
    case EShLangTessEvaluation: return RAWKIT_GLSL_STAGE_TESSELLATION_EVALUATION_BIT;
    case EShLangGeometry: return RAWKIT_GLSL_STAGE_GEOMETRY_BIT;
    case EShLangFragment: return RAWKIT_GLSL_STAGE_FRAGMENT_BIT;
    case EShLangCompute: return RAWKIT_GLSL_STAGE_COMPUTE_BIT;
    case EShLangRayGen: return RAWKIT_GLSL_STAGE_RAYGEN_BIT;
    case EShLangIntersect: return RAWKIT_GLSL_STAGE_INTERSECTION_BIT;
    case EShLangAnyHit: return RAWKIT_GLSL_STAGE_ANY_HIT_BIT;
    case EShLangClosestHit: return RAWKIT_GLSL_STAGE_CLOSEST_HIT_BIT;
    case EShLangMiss: return RAWKIT_GLSL_STAGE_MISS_BIT;
    case EShLangCallable: return RAWKIT_GLSL_STAGE_CALLABLE_BIT;
    case EShLangCount: return RAWKIT_GLSL_STAGE_ALL;
    default:
      return RAWKIT_GLSL_STAGE_NONE_BIT;
  }
}

static bool compile_shader(glslang::TShader* shader, const char *name, const char* src, GLSLIncluder &includer) {
  if (!shader || !name || !src) {
    return false;
  }

  int length = static_cast<int>(strlen(src));
  shader->setStringsWithLengths(
    &src,
    &length,
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

  shader->setPreamble(
    "\n#extension GL_GOOGLE_include_directive: enable\n"
  );

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
    450,
    false,
    EShMsgRelaxedErrors,
    includer
  );

  if (!r) {
    printf("Debug log for %s\n%s\n", name, shader->getInfoDebugLog());
    printf("glslang: failed to parse shader %s\nLOG: %s",
      name,
      shader->getInfoLog()
    );

    return false;
  }

  return true;
}

bool rawkit_glsl_is_compute(const rawkit_glsl_t *ref) {
  if (!ref) {
    return false;
  }

  return ref->compute;
}

rawkit_glsl_t *rawkit_glsl_compile(uint8_t source_count, rawkit_glsl_source_t *sources, const rawkit_glsl_paths_t *include_dirs) {
  if (!sources || !source_count) {
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

  ret->valid = true;
  ret->reflection_entries = new vector<rawkit_glsl_reflection_entry_t>();
  ret->reflection_name_to_index = new unordered_map<string, uint64_t>();
  ret->bindings_per_set = new unordered_map<uint32_t, uint32_t>();
  ret->binding_offset = new unordered_map<uint64_t, string>();
  ret->stages = new vector<RawkitStage *>();

  GLSLIncluder includer;

  if (include_dirs && include_dirs->count) {
    for (uint32_t i = 0; i < include_dirs->count; i++) {
      const char* include = include_dirs->entry[i];
      if (include != nullptr) {
        includer.pushExternalLocalDirectory(include);
      }
    }
  }

  glslang::TProgram program;

  // compile all of the provided sources
  vector<EShLanguage> stage_masks;
  vector<glslang::TShader *> shaders;
  {
    for (uint8_t i=0; i<source_count; i++) {
      EShLanguage stage = filename_to_stage(sources[i].filename);
      if (stage == EShLanguage::EShLangCount) {
        printf("ERROR: invalid stage language\n");
        return NULL;
      }

      if (stage == EShLanguage::EShLangCompute) {
        ret->compute = true;
      }

      glslang::TShader *shader = new glslang::TShader(stage);

      if (!compile_shader(shader, sources[i].filename, sources[i].data, includer)) {
        ret->valid = false;
        return ret;
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

    return ret;
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
    ret->stages->push_back(stage);
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

    comp.populate_reflection(ret);

    // comp.set_format("json");
    // auto json = comp.compile();
    // printf("reflection:\n%s\n", json.c_str());
  }
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

  if (ref->stages) {
    ref->stages->clear();
  }

  if (ref->reflection_name_to_index) {
    ref->reflection_name_to_index->clear();
    delete ref->reflection_name_to_index;
    ref->reflection_name_to_index = nullptr;
  }

  if (ref->reflection_entries) {
    ref->reflection_entries->clear();
    delete ref->reflection_entries;
    ref->reflection_entries = nullptr;
  }

  if (ref->bindings_per_set) {
    ref->bindings_per_set->clear();
    delete ref->bindings_per_set;
    ref->bindings_per_set = nullptr;
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

  rawkit_glsl_source_t source = { s->name, (const char *)input->data };
  rawkit_glsl_t *r = rawkit_glsl_compile(
    1,
    &source,
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
