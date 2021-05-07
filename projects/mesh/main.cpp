

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <rawkit/rawkit.h>

#include <rawkit/mesh.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>

#include <glm/glm.hpp>
using namespace glm;

typedef struct render_mesh_state_t {
  uint32_t vb_resource_version;
} render_mesh_state_t;

void render_mesh_file(
  const char *mesh_file,
  uint32_t instances,
  rawkit_shader_instance_t *inst
) {

  rawkit_gpu_t *gpu = inst->gpu;
  VkDevice device = gpu->device;
  if (device == VK_NULL_HANDLE) {
    printf("invalid vulkan device\n");
    return;
  }

  VkResult err;
  // TODO: generate a better id - use the instance's id
  char id[4096] = "rawkit::render_mesh ";
  strcat(id, mesh_file);

  render_mesh_state_t *state = rawkit_hot_state(id, render_mesh_state_t);
  if (!state) {
    printf("no state\n");
    return;
  }

  rawkit_mesh_t *mesh = rawkit_mesh(mesh_file);
  if (!mesh || !mesh->resource_version) {
    return;
  }

  rawkit_gpu_vertex_buffer_t *vb = rawkit_gpu_vertex_buffer_create(
    gpu,
    rawkit_vulkan_queue(),
    rawkit_vulkan_command_pool(),
    mesh
  );

  // rebuild the vertex buffer if the mesh changed
  if (vb->resource_version != state->vb_resource_version) {
    // TODO: use the standardized mechanism for dirty tracking of resources.
    state->vb_resource_version = vb->resource_version;

    if (!vb) {
      printf("ERROR: could not create vertex buffer\n");
      return;
    }
  }

  if (!vb) {
    return;
  }

  // render the mesh
  VkCommandBuffer command_buffer = inst->command_buffer;
  if (!command_buffer) {
    return;
  }


  VkViewport viewport = {};
  viewport.width = rawkit_window_width();
  viewport.height = rawkit_window_height();
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
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

  rawkit_gpu_t *gpu = rawkit_default_gpu();

  ubo_t ubo = {};

  float aspect = (float)rawkit_window_width() / (float)rawkit_window_height();
  mat4 proj = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 100.0f);

  vec3 eye = {};
  eye[1] = -5.0;
  float now = (float)rawkit_now();
  eye[0] = sin(now) * 5.0f;
  eye[2] = cos(now) * 5.0f;

  mat4 view = glm::lookAt(
    eye,
    (vec3){ 0.0f, 0.0f, 0.0f},
    (vec3){ 0.0f, 1.0f, 0.0f}
  );

  ubo.mvp = proj * view;


  // render meshes into a texture
  rawkit_texture_target_t *main_pass = rawkit_texture_target_begin(
    gpu,
    "main_pass",
    f32(rawkit_window_width()),
    f32(rawkit_window_height()),
    true
  );

  {
    rawkit_shader_options_t options = rawkit_shader_default_options();
    options.depthTest = true;
    options.depthWrite = true;

    // single instance
    {
      const rawkit_file_t *files[2] = {
        rawkit_file("mesh.vert"),
        rawkit_file("mesh.frag")
      };

      rawkit_shader_t *shader = rawkit_shader_ex(
        gpu,
        main_pass->render_pass,
        &options,
        2,
        files
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
        gpu,
        shader,
        main_pass->command_buffer,
        rawkit_window_frame_index()
      );

      if (inst) {
        rawkit_shader_instance_param_ubo(inst, "UBO", &ubo);

        render_mesh_file(
          "cube.stl",
          1,
          inst
        );

        rawkit_shader_instance_end(inst);
      }
    }

    // instanced rendering
    {
      const rawkit_file_t *files[2] = {
        rawkit_file("mesh-instanced.vert"),
        rawkit_file("mesh.frag")
      };

      rawkit_shader_t *shader = rawkit_shader_ex(
        gpu,
        main_pass->render_pass,
        &options,
        2,
        files
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_begin_ex(
        gpu,
        shader,
        main_pass->command_buffer,
        rawkit_window_frame_index()
      );

      if (inst) {
        float offsets[40] = {};
        for (int i=0; i<40; i+=4) {
          offsets[i] = (float)i * 0.75;
          offsets[i+1] = 3;
        }
        rawkit_shader_instance_param_ubo(inst, "UBO", &ubo);
        rawkit_shader_instance_param_ubo(inst, "Offsets", &offsets);

        render_mesh_file(
        "cube.stl",
          10,
          inst
        );
        rawkit_shader_instance_end(inst);
      }
    }
  }

  if (main_pass) {
    rawkit_texture_target_end(main_pass);


    // debug color texture
    {
      float scale = 0.5;
      ImTextureID texture = rawkit_imgui_texture(main_pass->color, main_pass->color->default_sampler);
      if (!texture) {
        return;
      }

      igImage(
        texture,
        (ImVec2){ 768.0f * scale, 512.0f * scale },
        (ImVec2){ 0.0f, 0.0f }, // uv0
        (ImVec2){ 1.0f, 1.0f }, // uv1
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
      );
    }

    // debug depth texture
    {
      float scale = 0.5;
      ImTextureID texture = rawkit_imgui_texture(main_pass->depth, main_pass->depth->default_sampler);
      if (!texture) {
        return;
      }

      igImage(
        texture,
        (ImVec2){ 768.0f * scale, 512.0f * scale },
        (ImVec2){ 0.0f, 0.0f }, // uv0
        (ImVec2){ 1.0f, 1.0f }, // uv1
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
      );
    }


    // render fullscreen depth texture
    {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("fullscreen.vert"),
        rawkit_file("fullscreen.frag")
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(shader);
      if (inst) {
        rawkit_shader_instance_param_texture(inst, "tex", main_pass->depth, NULL);

        VkViewport viewport = {
          .x = 0.0f,
          .y = 0.0f,
          .width = (float)rawkit_window_width(),
          .height = (float)rawkit_window_height()
        };

        vkCmdSetViewport(
          inst->command_buffer,
          0,
          1,
          &viewport
        );

        VkRect2D scissor = {};
        scissor.extent.width = viewport.width;
        scissor.extent.height = viewport.height;
        vkCmdSetScissor(
          inst->command_buffer,
          0,
          1,
          &scissor
        );
        vkCmdDraw(inst->command_buffer, 3, 1, 0, 0);
        rawkit_shader_instance_end(inst);
      }
    }
  }
}
