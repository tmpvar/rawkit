

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rawkit/rawkit.h>

#include <rawkit/mesh.h>

#include <cglm/cglm.h>

typedef struct render_mesh_state_t {
  rawkit_mesh_t *mesh;
  rawkit_gpu_vertex_buffer_t *vertex_buffer;
} render_mesh_state_t;

void render_mesh_file(
  const char *mesh_file,
  uint32_t instances,
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
      mesh->vertex_data,
      rawkit_mesh_index_count(mesh),
      mesh->index_data
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

    vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      shader->pipeline_layout,
      0,
      shader->descriptor_set_count,
      shader->descriptor_sets,
      0,
      0
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
      instances,
      0,
      0
    );
  }
}

void setup() {}

typedef struct ubo_t {
  float color[4];
  mat4 mvp;
} ubo_t;

void loop() {
  ubo_t ubo = {};

  float aspect = (float)rawkit_window_width() / (float)rawkit_window_height();
  mat4 proj;
  glm_perspective(90.0f, aspect, 0.1f, 1000.0f, proj);

  vec3 eye = {};
  eye[1] = -5.0;
  float now = (float)rawkit_now();
  eye[0] = sin(now) * 5.0f;
  eye[2] = cos(now) * 5.0f;

  mat4 view;
  glm_lookat(
    eye,
    (vec3){ 0.0f, 0.0f, 0.0f},
    (vec3){ 0.0f, 1.0f, 0.0f},
    view
  );

  glm_mat4_mul(proj, view, ubo.mvp);

  // single instance
  {
    rawkit_shader_params_t params = {};
    rawkit_shader_params(params,
      rawkit_shader_ubo("UBO", &ubo)
    );

    render_mesh_file(
      "cube.stl",
      1,
      2,
      (const char *[]){ "mesh.vert", "mesh.frag" },
      params
    );
  }

  {

     float offsets[40] = {};
     for (int i=0; i<40; i+=4) {
       offsets[i] = (float)i * 0.75;
       offsets[i+1] = 3;
     }

    rawkit_shader_params_t params = {};
    rawkit_shader_params(params,
      rawkit_shader_ubo("UBO", &ubo),
      rawkit_shader_ubo("Offsets", &offsets)
    );

    render_mesh_file(
      "cube2.stl",
      10,
      2,
      (const char *[]){ "mesh-instanced.vert", "mesh-instanced.frag" },
      params
    );

  }


}