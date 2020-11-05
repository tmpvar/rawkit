#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rawkit/rawkit.h>

#include <rawkit/mesh.h>

typedef struct render_mesh_state_t {
  rawkit_shader_t *shaders;
  rawkit_glsl_t *glsl;
  uint32_t source_count;
  rawkit_glsl_source_t sources[RAWKIT_GLSL_STAGE_COUNT];

  rawkit_mesh_t *mesh;
  rawkit_gpu_vertex_buffer_t *vertex_buffer;
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

void render_mesh_file(const char *mesh_file, uint8_t source_count, const char **source_files) {
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
  // TODO: generate a better id
  char id[4096] = "rawkit::render_mesh ";
  strcat(id, mesh_file);
  strcat(id, " ");
  for (uint8_t i=0; i<source_count; i++) {
    strcat(id, source_files[i]);
    strcat(id, " ");
  }

  render_mesh_state_t *state = rawkit_hot_state(id, render_mesh_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  // rebuild the pipeline if any of the shaders changed
  {
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

    if (!ready) {
      return;
    }

    if (changed) {
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
    }
  }

  if (!state->shaders) {
    return;
  }

  // rebuild the vertex buffer if the mesh changed
  rawkit_mesh_t *mesh = rawkit_mesh(mesh_file);
  if (mesh) {

    // TODO: more optimally, reuse the existing buffer if possible
    rawkit_gpu_vertex_buffer_t *vb = rawkit_gpu_vertex_buffer_create(
      rawkit_vulkan_physical_device(),
      rawkit_vulkan_device(),
      rawkit_vulkan_queue(),
      rawkit_vulkan_command_pool(),
      rawkit_mesh_vertex_count(mesh),
      mesh->vertex_data
    );

    if (!vb) {
      printf("ERROR: could not create vertex buffer\n");
      return;
    }

    if (state->vertex_buffer) {
      rawkit_gpu_vertex_buffer_destroy(device, state->vertex_buffer);
    }

    state->vertex_buffer = vb;
  }

  // ensure we have a vertex buffer
  if (!state->vertex_buffer) {
    return;
  }

  // render the mesh
  {
    rawkit_shader_t *shader = &state->shaders[rawkit_window_frame_index()];
    VkCommandBuffer command_buffer = rawkit_vulkan_command_buffer();
    if (!command_buffer) {
      return;
    }

    VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(
      command_buffer,
      0,
      1,
      &state->vertex_buffer->vertices->handle,
      offsets
    );

    vkCmdBindPipeline(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      shader->pipeline
    );

    vkCmdDraw(command_buffer, 3, 1, 0, 0);
  }
}

void setup() {}

void loop() {
  const char *sources[] = {
    "mesh.vert",
    "mesh.frag"
  };

  render_mesh_file(
    "cube.stl",
    2,
    sources
  );
}