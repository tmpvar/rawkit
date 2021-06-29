#pragma once

#include <rawkit/glsl.h>


#include "../internal/includer.h"
#include "../internal/defaults.h"

#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SpvTools.h>
#include <SPIRV/disassemble.h>
#include <glslang/Public/ShaderLang.h>

#include <spirv_cross.hpp>
#include <spirv_reflect.hpp>


#include <string>
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

class State {
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

    ~State() {
      for (auto &entry : this->reflection_entries) {
        free(entry.name);
      }
    }
};



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


static bool rawkit_glsl_reflection_add_entry(State *state, string name, rawkit_glsl_reflection_entry_t entry) {
  if (!state) {
    printf("ERROR: glsl was null\n");
    return false;
  }

  entry.name = strdup(name.c_str());

  auto it = state->reflection_name_to_index.find(name);
  if (it != state->reflection_name_to_index.end()) {
    if (it->second >= state->reflection_entries.size()) {
      printf("ERROR: reflection name map pointed to entry that is out of range\n");
      return false;
    }

    // TODO: if any of the other fields differ we have a problem
    state->reflection_entries[it->second].stage |= entry.stage;
    return true;
  }

  uint64_t idx = state->reflection_entries.size();
  state->reflection_name_to_index.emplace(name, idx);
  state->reflection_entries.push_back(entry);

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
      auto it = state->binding_offset.find(key);
      if (it == state->binding_offset.end()) {
        state->binding_offset.emplace(key, name);
      } else {
        // TODO: this is not an error if the resource is an array
        // TODO: add the shader file name(s) where this occurred
        printf("ERROR: set(%u) && binding(%u) combination for '%s' already specified for '%s'\n",
          entry.set,
          entry.binding,
          name.c_str(),
          it->second.c_str()
        );

        state->valid = false;
        return false;
      }
    }

    uint32_t set = static_cast<uint32_t>(entry.set);
    auto it = state->bindings_per_set.find(set);

    if (it == state->bindings_per_set.end()) {
      state->bindings_per_set.emplace(set, 1);
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

    void populate_push_constants(State *state) {
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
          if (!rawkit_glsl_reflection_add_entry(state, name, entry)) {
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

    void populate_storage_images(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, image.name, entry)) {
          return;
        }
      }
    }

    void populate_textures(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, texture.name, entry)) {
          return;
        }
      }
    }

    void populate_ssbos(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, ssbo.name, entry)) {
          return;
        }
      }
    }

    void populate_separate_images(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, image.name, entry)) {
          return;
        }
      }
    }

    void populate_separate_samplers(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, sampler.name, entry)) {
          return;
        }
      }
    }

    void populate_uniform_buffers(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, ubo.name, entry)) {
          return;
        }
      }
    }

    void populate_inputs(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, input.name, entry)) {
          return;
        }
      }
    }

    void populate_outputs(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, output.name, entry)) {
          return;
        }
      }
    }

    void populate_subpass_inputs(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, input.name, entry)) {
          return;
        }
      }
    }

    void populate_acceleration_structures(State *state) {
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

        if (!rawkit_glsl_reflection_add_entry(state, acc.name, entry)) {
          return;
        }
      }
    }

    void populate_reflection(State *state) {
      this->populate_push_constants(state);
      this->populate_storage_images(state);
      this->populate_textures(state);
      this->populate_ssbos(state);
      this->populate_separate_images(state);
      this->populate_separate_samplers(state);
      this->populate_uniform_buffers(state);
      this->populate_inputs(state);
      this->populate_outputs(state);
      this->populate_subpass_inputs(state);
      this->populate_acceleration_structures(state);
    }
};


