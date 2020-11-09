#include <doctest.h>
#include <uv.h>
#include <rawkit/glsl.h>
#include "util.h"

#include <string>
using namespace std;


// TEST_CASE("[rawkit/glsl] invalid args") {
//   REQUIRE(rawkit_glsl_compile(0, nullptr, nullptr) == nullptr);
//   REQUIRE(rawkit_glsl_compile(1, nullptr, nullptr) == nullptr);

//   rawkit_glsl_source_t source = {"invalid.extension", "BLAH"};
//   REQUIRE(rawkit_glsl_compile(0, &source, nullptr) == nullptr);
//   REQUIRE(rawkit_glsl_compile(1, &source, nullptr) == nullptr);
// }

static const rawkit_file_t* load_file(uv_loop_t *loop, const char *path, bool should_fail = false) {

  int16_t c = 10000;

  while(c--) {
    const rawkit_file_t* f = _rawkit_file_ex(
      __FILE__,
      path,
      loop,
      rawkit_default_diskwatcher()
    );

    uv_run(loop, UV_RUN_NOWAIT);

    if (f && f->resource_version > 0) {
      return f;
    }
  }

  if (should_fail) {
    FAIL("could not load file");
  }

  return nullptr;
}

TEST_CASE("[rawkit/glsl] compile glsl into spirv") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t *s = rawkit_glsl(
    load_file(&loop, "fixtures/simple.frag")
  );
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  uv_loop_close(&loop);
}


TEST_CASE("[rawkit/glsl] compile glsl into spirv (relative include)") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/top-level-nester.frag")
  );

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
}

TEST_CASE("[rawkit/glsl] reflection (compute)") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/reflection.comp")
  );

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  uv_loop_close(&loop);

  const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(s, 0);
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
}

TEST_CASE("[rawkit/glsl] reflection (frag)") {

  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/reflection.frag")
  );
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  uv_loop_close(&loop);

  const uint32_t *workgroup_size = rawkit_glsl_workgroup_size(s, 0);
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
    CHECK(entry.binding == 0);
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
    CHECK(entry.binding == 0);
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
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/push_constant_buffer_size.comp")
  );
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s));
  uv_loop_close(&loop);

  const uint32_t* workgroup_size = rawkit_glsl_workgroup_size(s, 0);
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
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/binding-sharing.comp")
  );
  uv_loop_close(&loop);
  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s) == false);
}

TEST_CASE("[rawkit/glsl] multiple stages") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/multi-stage/basic.vert"),
    load_file(&loop, "fixtures/multi-stage/basic.frag")
  );
  uv_loop_close(&loop);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s) == true);
  CHECK(rawkit_glsl_stage_count(s) == 2);
  CHECK(rawkit_glsl_stage_at_index(s, 0) == RAWKIT_GLSL_STAGE_VERTEX_BIT);
  CHECK(rawkit_glsl_stage_at_index(s, 1) == RAWKIT_GLSL_STAGE_FRAGMENT_BIT);

  CHECK(rawkit_glsl_spirv_data(s, 0) != nullptr);
  CHECK(rawkit_glsl_spirv_byte_len(s, 0) != 0);

  CHECK(rawkit_glsl_spirv_data(s, 1) != nullptr);
  CHECK(rawkit_glsl_spirv_byte_len(s, 1) != 0);

  CHECK(rawkit_glsl_spirv_data(s, 0) != rawkit_glsl_spirv_data(s, 1));

  const rawkit_glsl_reflection_entry_t ubo = rawkit_glsl_reflection_entry(s, "ubo");
  CHECK(ubo.stage == (RAWKIT_GLSL_STAGE_FRAGMENT_BIT | RAWKIT_GLSL_STAGE_VERTEX_BIT));

  const rawkit_glsl_reflection_entry_t ubo2 = rawkit_glsl_reflection_entry(s, "UBO2");
  CHECK(ubo2.stage == (RAWKIT_GLSL_STAGE_FRAGMENT_BIT));

  const rawkit_glsl_reflection_entry_t fragColor = rawkit_glsl_reflection_entry(s, "fragColor");
  CHECK(fragColor.stage == (RAWKIT_GLSL_STAGE_FRAGMENT_BIT));

  const rawkit_glsl_reflection_entry_t inPosition = rawkit_glsl_reflection_entry(s, "inPosition");
  CHECK(inPosition.stage == (RAWKIT_GLSL_STAGE_VERTEX_BIT));
}


TEST_CASE("[rawkit/glsl] overlapping uniforms (set and binding)") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/multi-stage/overlap-uniforms.vert"),
    load_file(&loop, "fixtures/multi-stage/overlap-uniforms.frag")
  );
  uv_loop_close(&loop);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s) == false);
}


TEST_CASE("[rawkit/glsl] overlapping uniforms (set)") {
  uv_loop_t loop;
  uv_loop_init(&loop);
  const rawkit_glsl_t* s = rawkit_glsl(
    load_file(&loop, "fixtures/multi-stage/overlap-uniforms-set.vert"),
    load_file(&loop, "fixtures/multi-stage/overlap-uniforms-set.frag")
  );
  uv_loop_close(&loop);

  REQUIRE(s != nullptr);
  CHECK(rawkit_glsl_valid(s) == true);
}