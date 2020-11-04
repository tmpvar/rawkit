#include <stdio.h>
#include <string.h>

#include <rawkit/rawkit.h>

void setup() {

}


// typedef struct render_

typedef struct render_mesh_state_t {
  rawkit_shader_t *shaders;
  rawkit_glsl_t *glsl;
  uint32_t source_count;
  rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];
} render_mesh_state_t;

VkResult build_shaders(render_mesh_state_t *state, rawkit_glsl_t *glsl) {
  if (!state) {
    return VK_INCOMPLETE;
  }

  // TODO: this can change between frames
  uint32_t fb_count = rawkit_window_frame_count();

  if (state->shaders == NULL) {
    state->shaders = (rawkit_shader_t *)calloc(
      fb_count * sizeof(rawkit_shader_t),
      1
    );
  }

  for (uint32_t idx=0; idx < fb_count; idx++) {
    state->shaders[idx].physical_device = rawkit_vulkan_physical_device();
    VkResult err = rawkit_shader_init(glsl, &state->shaders[idx]);

    if (err != VK_SUCCESS) {
      printf("ERROR: rawkit_shader_init failed (%i)\n", err);
      return err;
    }
  }

  return VK_SUCCESS;
}

void render_mesh(uint8_t source_count, const char **source_files) {
  if (source_count > RAWKIT_GLSL_STAGE_COUNT) {
    printf("ERROR: source count greater than the number of stages\n");
    return;
  }

  VkDevice device = rawkit_vulkan_device();
  if (device == VK_NULL_HANDLE) {
    printf("invalid vulkan device\n");
    return;
  }

  VkResult err;
  const char *id = "rawkit::render_mesh";
  render_mesh_state_t *state = rawkit_hot_state(id, render_mesh_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];

  bool changed = false;
  bool ready = true;
  for (uint8_t i=0; i<source_count; i++) {
    const rawkit_file_t *file = rawkit_file(source_files[i]);

    if (!file) {
      if (!state->sources[i].data) {
        ready = false;
        continue;
      }

      sources[i] = state->sources[i];
      continue;
    }
    sources[i].filename = source_files[i];
    sources[i].data = (const char *)file->data;

    printf("CHANGED: %s\n", source_files[i]);
    changed = true;
  }

  if (!changed || !ready) {
    return;
  }

  rawkit_glsl_t *glsl = rawkit_glsl_compile(
    source_count,
    sources,
    NULL
  );

  if (!rawkit_glsl_valid(glsl)) {
    return;
  }

  memcpy(state->sources, sources, sizeof(sources));

  rawkit_glsl_destroy(state->glsl);
  state->glsl = glsl;

  err = build_shaders(state, glsl);
  if (err) {
    printf("ERROR: could not build shaders\n");
    return;
  }

  printf("built shaders!\n");
}

void loop() {
  const char *sources[] = {
    "mesh.vert",
    "mesh.frag"
  };

  render_mesh(2, sources);
}