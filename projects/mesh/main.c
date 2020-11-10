

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rawkit/rawkit.h>

#include <rawkit/mesh.h>

#include <cglm/cglm.h>

typedef struct render_mesh_state_t {
  uint32_t vb_resource_version;
} render_mesh_state_t;

void render_mesh_file(
  rawkit_gpu_t *gpu,
  const char *mesh_file,
  uint32_t instances,
  uint8_t file_count,
  const rawkit_file_t **files,
  rawkit_shader_params_t params
) {

  VkDevice device = gpu->device;
  if (device == VK_NULL_HANDLE) {
    printf("invalid vulkan device\n");
    return;
  }

  VkResult err;
  // TODO: generate a better id
  char id[4096] = "rawkit::render_mesh ";
  strcat(id, mesh_file);
  for (uint8_t i=0; i<file_count; i++) {
    strcat(id, files[i]->resource_name);
  }

  render_mesh_state_t *state = rawkit_hot_state(id, render_mesh_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  rawkit_shader_t *shader = rawkit_shader_ex(
    gpu,
    rawkit_window_frame_count(),
    rawkit_vulkan_renderpass(),
    file_count,
    files
  );

  rawkit_mesh_t *mesh = rawkit_mesh(mesh_file);
  rawkit_gpu_vertex_buffer_t *vb = rawkit_gpu_vertex_buffer_create(
    gpu,
    rawkit_vulkan_queue(),
    rawkit_vulkan_command_pool(),
    mesh
  );

  // rebuild the vertex buffer if the mesh changed
  if (mesh && vb->resource_version != state->vb_resource_version) {
    // TODO: use the standardized mechanism for dirty tracking of resources.
    state->vb_resource_version = vb->resource_version;

    if (!vb) {
      printf("ERROR: could not create vertex buffer\n");
      return;
    }
  }

  if (!vb || !shader->resource_version) {
    return;
  }

  // render the mesh
  VkCommandBuffer command_buffer = rawkit_vulkan_command_buffer();
  if (!command_buffer) {
    return;
  }

  rawkit_shader_bind(
    shader,
    rawkit_window_frame_index(),
    command_buffer,
    params
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

  VkDeviceSize offsets[1] = { 0 };
  vkCmdBindVertexBuffers(
    command_buffer,
    0,
    1,
    &vb->vertices->handle,
    offsets
  );

  uint32_t index_count = rawkit_mesh_index_count(mesh);
  if (index_count > 0) {
    vkCmdBindIndexBuffer(
      command_buffer,
      vb->indices->handle,
      0,
      VK_INDEX_TYPE_UINT32
    );

    vkCmdDrawIndexed(
      command_buffer,
      index_count,
      instances,
      0,
      0,
      0
    );
  } else {
    vkCmdDraw(
      command_buffer,
      rawkit_mesh_vertex_count(mesh),
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
  // TODO: this should be exposed some where
  rawkit_gpu_t gpu = {};
  gpu.physical_device = rawkit_vulkan_physical_device();
  gpu.device = rawkit_vulkan_device();
  gpu.pipeline_cache = rawkit_vulkan_pipeline_cache();

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
      &gpu,
      "cube.stl",
      1,
      2,
      (const rawkit_file_t *[]){
        rawkit_file("mesh.vert"),
        rawkit_file("mesh.frag")
      },
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
      &gpu,
      "cube.stl",
      10,
      2,
      (const rawkit_file_t *[]){
        rawkit_file("mesh-instanced.vert"),
        rawkit_file("mesh.frag")
      },
      params
    );
  }
}
