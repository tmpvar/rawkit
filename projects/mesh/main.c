#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rawkit/rawkit.h>

#include <rawkit/mesh.h>

typedef struct render_mesh_state_t {
  rawkit_mesh_t *mesh;
  rawkit_gpu_vertex_buffer_t *vertex_buffer;
} render_mesh_state_t;

void render_mesh_file(
  const char *mesh_file,
  uint8_t source_count,
  const char **source_files,
  rawkit_shader_params_t params
) {
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

  rawkit_shader_t *shaders = rawkit_shader(
    rawkit_vulkan_physical_device(),
    rawkit_window_frame_count(),
    source_count,
    source_files
  );

  if (!shaders) {
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
    state->mesh = mesh;
  }

  // ensure we have a vertex buffer
  if (!state->vertex_buffer) {
    return;
  }

  // render the mesh
  {
    rawkit_shader_t *shader = &shaders[rawkit_window_frame_index()];
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

    VkViewport viewport = {};
    viewport.width = rawkit_window_width();
    viewport.height = rawkit_window_height();
    vkCmdSetViewport(
      command_buffer,
      0,
      1,
      &viewport
    );

    VkRect2D scissor = {};
    scissor.extent.width = rawkit_window_width();
    scissor.extent.height = rawkit_window_height();
    vkCmdSetScissor(
      command_buffer,
      0,
      1,
      &scissor
    );

    rawkit_shader_apply_params(shader, command_buffer, params);

    vkCmdDraw(
      command_buffer,
      rawkit_mesh_vertex_count(state->mesh),
      1,
      0,
      0
    );
  }
}

void setup() {}

void loop() {
  rawkit_shader_params_t params = {};

  render_mesh_file(
    "cube.stl",
    2,
    (const char *[]){ "mesh.vert", "mesh.frag" },
    params
  );
}