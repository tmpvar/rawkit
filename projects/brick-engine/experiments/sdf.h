#pragma once
#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

static char sdf_tmp_str[4096] = "";

// from https://www.iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
static float sdBox(vec2 p, vec2 b ) {
  vec2 d = abs(p)-b;
  return length(max(d, 0.0f)) + min(max(d.x,d.y),0.0f);
}

typedef struct SDF {
  const char *name;
  vec3 dims;
  vec2 half_dims;
  float diagonal_length;
  rawkit_texture_t *tex;
  rawkit_cpu_buffer_t *tex_buf;
  rawkit_cpu_buffer_t *dist_buf;
  bool dirty = false;
  SDF(const char *name, vec3 dims)
    : dims(dims), name(name)
  {
    this->diagonal_length = length(vec2(dims));
    this->tex = rawkit_texture_mem(name, dims.x, dims.y, dims.z, VK_FORMAT_R32_SFLOAT);
    sprintf(sdf_tmp_str, "%s::tex_buf", name);
    this->tex_buf = rawkit_cpu_buffer(sdf_tmp_str, this->tex->options.size);
    sprintf(sdf_tmp_str, "%s::dist_buf", name);
    this->dist_buf = rawkit_cpu_buffer(sdf_tmp_str, this->tex->options.size);
    this->half_dims = vec2(this->dims) * 0.5f;
  }

  void debug_dist() {
    rawkit_texture_update_buffer(this->tex, this->tex_buf);
    {
      ImTextureID texture = rawkit_imgui_texture(this->tex, this->tex->default_sampler);
      if (!texture) {
        printf("could not create a valid imgui texture from distance texture\n");
        return;
      }

      igText("sdf->tex dims(%f, %f)",
        (float)this->tex->options.width, (float)this->tex->options.height
      );
      // render the actual image
      igImage(
        texture,
        (ImVec2){ (float)this->tex->options.width, (float)this->tex->options.height },
        (ImVec2){ 0.0f, 0.0f }, // uv0
        (ImVec2){ 1.0f, 1.0f }, // uv1
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}, // tint color
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f} // border color
      );
    }
  }

  float sample(vec2 p) {
    if (
      p.x < 0.0f || p.x >= this->dims.x ||
      p.y < 0.0f || p.y >= this->dims.y
    ) {
      return max(
        sdBox(p - this->half_dims, this->half_dims),
        this->sample(clamp(p, vec2(0.0f), vec2(this->dims) - 1.0f))
      );
    }

    uint64_t i = (
      static_cast<uint64_t>(p.x) +
      static_cast<uint64_t>(p.y * this->dims.x)
    );


    float d = ((float *)this->dist_buf->data)[i];

    return d;
  }

  void write(uvec2 p, float val) {
    uint64_t i = (
      static_cast<uint64_t>(p.x) +
      static_cast<uint64_t>(p.y * this->dims.x)
    );
    if (i >= this->tex_buf->size) {
      return;
    }
    ((float *)this->dist_buf->data)[i] = val;
    // debugging
    ((float *)this->tex_buf->data)[i] = (val / length(this->half_dims)) * 0.5f + 0.5f;
  }
} SDF;
