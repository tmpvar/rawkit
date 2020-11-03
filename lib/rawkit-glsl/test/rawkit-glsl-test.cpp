#include <doctest.h>

#include <rawkit/glsl.h>
#include "util.h"

#include <string>
using namespace std;

TEST_CASE("[rawkit/glsl] invalid args") {
  REQUIRE(rawkit_glsl_compile(0, nullptr, nullptr) == nullptr);
  REQUIRE(rawkit_glsl_compile(1, nullptr, nullptr) == nullptr);

  rawkit_glsl_source_t source = {"invalid.extension", "BLAH"};
  REQUIRE(rawkit_glsl_compile(0, &source, nullptr) == nullptr);
  REQUIRE(rawkit_glsl_compile(1, &source, nullptr) == nullptr);
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv") {
  rawkit_glsl_source_t source = {
    "simple.frag",
    "#version 450\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = 1.23456 * 5.678;\n"
    "}\n",
  };

  rawkit_glsl_t *s = rawkit_glsl_compile(1, &source, nullptr);
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv (absolute include)") {
  char src[4096] = {0};
  string include;
  include.assign(fixturePath("multiply.glsl"));
  std::replace( include.begin(), include.end(), '\\', '/');

  sprintf(
    src,
    "#version 450\n"
    "#include \"%s\"\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = multiply(1.23456, 5.678);\n"
    "}\n",
    include.c_str()
  );

  rawkit_glsl_source_t source = {"simple.frag", src};
  rawkit_glsl_t *s = rawkit_glsl_compile(1, &source, nullptr);
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv (relative include)") {
  char src[4096] = {0};
  string include;
  include.assign(fixturePath("nested.glsl"));
  std::replace( include.begin(), include.end(), '\\', '/');

  sprintf(
    src,
    "#version 450\n"
    "#include \"%s\"\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = nested(1.23456);\n"
    "}\n",
    include.c_str()
  );

  rawkit_glsl_source_t source = {"simple.frag", src};
  rawkit_glsl_t *s = rawkit_glsl_compile(1, &source, nullptr);
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv (system include)") {
  char *paths[] = { strdup(fixturePath("")) };
  rawkit_glsl_paths_t includes = {
    paths,
    1
  };

  rawkit_glsl_source_t source = {
    "simple.frag",
    "#version 450\n"
    "#include <system.glsl>\n"
    "out float outF;"
    "void main() {\n"
    "  float outF = system();\n"
    "}\n"
  };

  rawkit_glsl_t *s = rawkit_glsl_compile(
    1,
    &source,
    &includes
  );

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));

  free(paths[0]);
  free(s);
}

TEST_CASE("[rawkit/glsl] reflection (compute)") {
  rawkit_glsl_source_t source = {
    "reflection.comp",
    "#version 450\n"
    "layout(local_size_x = 4, local_size_y = 4) in;\n"
    "layout (binding = 0, rgba8) uniform writeonly image2D writable_image;\n"
    "layout (binding = 1, r32f) uniform readonly image3D readable_image;\n"
    "layout (push_constant) uniform BockName {\n"
    "  float var_float;\n"
    "  int var_int;\n"
    "} consts;"
    "layout (binding = 2) uniform sampler2D texture_no_layout;\n"
    "layout (location = 3, binding = 2, set = 1) uniform sampler2D texture_with_layout;\n"
    "layout (binding = 3, std430) buffer ssbo_buffer {  float ssbo_buffer_floats[]; };\n"
    "layout (binding = 4, std430) readonly buffer ssbo_buffer_ro {  float ssbo_buffer_floats_ro[]; };\n"
    "layout (binding = 5,std430) writeonly buffer ssbo_buffer_wo {  float ssbo_buffer_floats_wo[]; };\n"
    "struct ssbo_struct { float a; int b; };\n"
    "layout (binding = 6, std430) buffer ssbo_buffer_sized {  ssbo_struct block_sized_struct;  };\n"
    "layout (binding = 7) uniform sampler separate_sampler;\n"
    "layout (binding = 8) uniform texture2D separate_image;\n"
    "layout (binding = 9, std430) uniform ubo { float ubo_float; };\n"
    "void main() {\n"
    "  imageStore(writable_image, ivec2(0, 0), vec4(ssbo_buffer_floats[0]));\n"
    "  ssbo_buffer_floats_wo[0] = ssbo_buffer_floats[0] + ssbo_buffer_floats_ro[0];\n"
    "  block_sized_struct.a = ssbo_buffer_floats[0] + ssbo_buffer_floats_ro[0];\n"
    "}\n",
  };
  rawkit_glsl_t *s = rawkit_glsl_compile(1, &source, NULL);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(s);
  REQUIRE(workgroup_size != nullptr);
  CHECK(workgroup_size[0] == 4);
  CHECK(workgroup_size[1] == 4);
  CHECK(workgroup_size[2] == 1);

  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "writable_image");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_RGBA8);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_2D);
    CHECK(entry.location == 0);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 0);
    CHECK(entry.readable == false);
    CHECK(entry.writable == true);
  }

  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "readable_image");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_IMAGE);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_R32F);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_3D);
    CHECK(entry.location == 1);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 1);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "var_float");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER);
    CHECK(entry.offset == 0);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "var_int");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_PUSH_CONSTANT_BUFFER);
    CHECK(entry.offset == 4);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // Texture: texture_no_layout
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "texture_no_layout");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_2D);
    CHECK(entry.location == 2);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 2);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // Texture: texture_with_layout
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "texture_with_layout");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_SAMPLED_IMAGE);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_2D);
    CHECK(entry.location == 3);
    CHECK(entry.set == 1);
    CHECK(entry.binding == 2);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // SSBO: ssbo_buffer
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "ssbo_buffer");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_BUFFER);
    CHECK(entry.location == 0);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 3);
    CHECK(entry.block_size == 0);
    CHECK(entry.readable == true);
    CHECK(entry.writable == true);
  }

  // SSBO: ssbo_buffer_ro
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "ssbo_buffer_ro");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_BUFFER);
    CHECK(entry.location == 0);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 4);
    CHECK(entry.block_size == 0);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // SSBO: ssbo_buffer_wo
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "ssbo_buffer_wo");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_BUFFER);
    CHECK(entry.location == 0);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 5);
    CHECK(entry.block_size == 0);
    CHECK(entry.readable == false);
    CHECK(entry.writable == true);
  }

  // SSBO: ssbo_buffer_sized
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "ssbo_buffer_sized");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STORAGE_BUFFER);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_BUFFER);
    CHECK(entry.location == 0);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 6);
    CHECK(entry.block_size == 8);
    CHECK(entry.readable == true);
    CHECK(entry.writable == true);
  }

  // Separate Sampler
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "separate_sampler");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_SAMPLER);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_1D);
    CHECK(entry.location == 3);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 7);
    CHECK(entry.readable == true);
    CHECK(entry.writable == true);
  }

  // Separate Images: separate_image (aka texture2D)
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "separate_image");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_SEPARATE_IMAGE);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_2D);
    CHECK(entry.location == 4);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 8);
    CHECK(entry.readable == true);
    CHECK(entry.writable == true);
  }

  // UBO
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "ubo");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_UNIFORM_BUFFER);
    CHECK(entry.offset == -1);
    CHECK(entry.image_format == RAWKIT_GLSL_IMAGE_FORMAT_UNKNOWN);
    CHECK(entry.dims == RAWKIT_GLSL_DIMS_BUFFER);
    CHECK(entry.location == 0);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 9);
    CHECK(entry.block_size == 4);
    CHECK(entry.readable == true);
    CHECK(entry.writable == true);
  }

  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "doesn't exist");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_NOT_FOUND);
  }

  rawkit_glsl_destroy(s);
}

TEST_CASE("[rawkit/glsl] reflection (frag)") {
  rawkit_glsl_source_t source = {
    "reflection.frag",
    "#version 450\n"
    "#extension GL_EXT_ray_tracing : require\n"
    "in vec4 in_color;\n"
    "in float in_float;\n"
    "out vec4 out_color;\n"
    "out float out_float;\n"
    "layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;\n"
    "layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepth;\n"
    "layout(binding = 1, set = 0) uniform accelerationStructureEXT acc;\n"
    "void main() {\n"
    "  out_color = in_color;\n"
    "  out_float = in_float;\n"
    "}\n",
  };

  rawkit_glsl_t *s = rawkit_glsl_compile(1, &source, NULL);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(s);
  REQUIRE(workgroup_size != nullptr);
  CHECK(workgroup_size[0] == 0);
  CHECK(workgroup_size[1] == 0);
  CHECK(workgroup_size[2] == 0);

  // Input: in_color
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "in_color");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_INPUT);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 0);
    CHECK(entry.set == -1);
    CHECK(entry.binding == -1);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // Input: in_float
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "in_float");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_INPUT);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 1);
    CHECK(entry.set == -1);
    CHECK(entry.binding == -1);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // Output: out_color
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "out_color");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_OUTPUT);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 0);
    CHECK(entry.set == -1);
    CHECK(entry.binding == -1);
    CHECK(entry.readable == false);
    CHECK(entry.writable == true);
  }

  // Output: out_float
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "out_float");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_STAGE_OUTPUT);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 1);
    CHECK(entry.set == -1);
    CHECK(entry.binding == -1);
    CHECK(entry.readable == false);
    CHECK(entry.writable == true);
  }

  // Subpass Input: inputColor
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "inputColor");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_SUBPASS_INPUT);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 0);
    CHECK(entry.set ==  0);
    CHECK(entry.binding == 0);
    CHECK(entry.input_attachment_index == 0);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // Subpass Input: inputDepth
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "inputDepth");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_SUBPASS_INPUT);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 1);
    CHECK(entry.set == 1);
    CHECK(entry.binding == 1);
    CHECK(entry.input_attachment_index == 1);
    CHECK(entry.readable == true);
    CHECK(entry.writable == false);
  }

  // Acceleration Structures: acc
  {
    rawkit_glsl_reflection_entry_t entry = rawkit_glsl_reflection_entry(s, "acc");
    REQUIRE(entry.entry_type == RAWKIT_GLSL_REFLECTION_ENTRY_ACCELERATION_STRUCTURE);
    CHECK(entry.offset == 0);
    CHECK(entry.location == 2);
    CHECK(entry.set == 0);
    CHECK(entry.binding == 1);
  }
}

TEST_CASE("[rawkit/glsl] reflection push constant buffer size") {
  rawkit_glsl_source_t source = {
    "push_constant_buffer_size.comp",
    "#version 450\n"
    "layout(local_size_x = 4, local_size_y = 4) in;\n"
    "layout (push_constant) uniform BockName1 {\n"
    "  float var_float;\n"
    "  int var_int;\n"
    "} consts;\n"
    "void main() {\n"
    "}\n",
  };

  rawkit_glsl_t* s = rawkit_glsl_compile(1, &source, NULL);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  const uint32_t* workgroup_size = rawkit_glsl_workgroup_size(s);
  REQUIRE(workgroup_size != nullptr);
  CHECK(workgroup_size[0] == 4);
  CHECK(workgroup_size[1] == 4);
  CHECK(workgroup_size[2] == 1);

  {
    auto entry = rawkit_glsl_reflection_entry(s, "var_float");
    CHECK(entry.block_size == 4);
  }

  {
    auto entry = rawkit_glsl_reflection_entry(s, "var_int");
    CHECK(entry.block_size == 4);
  }
}

TEST_CASE("[rawkit/glsl] bindings cannot be shared") {
  rawkit_glsl_source_t source = {
    "reflection.comp",
    "#version 450\n"
    "layout(local_size_x = 4, local_size_y = 4) in;\n"
    "layout (rgba8) uniform writeonly image2D writable_image;\n"
    "layout (std430) uniform ubo { float ubo_float; };\n"
    "void main() {\n"
    "}\n",
  };

  rawkit_glsl_t *s = rawkit_glsl_compile(1, &source, NULL);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s) == false);
}