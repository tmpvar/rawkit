#pragma once
#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

#include <functional>

static char sdf_tmp_str[4096] = "";

// from https://www.iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
static float sdBox(vec2 p, vec2 b ) {
  vec2 d = abs(p)-b;
  return length(max(d, 0.0f)) + min(max(d.x,d.y),0.0f);
}

static vec2 calc_sdf_normal(vec2 p, std::function<float(vec2)> sample) {
  //get d, and also its sign (i.e. inside or outside)
  float d = sample(p);
  float sign = d >= 0 ? 1.0f : -1.0f;
  float maxval = FLT_MAX * sign;

  //read neighbour distances, ignoring border pixels
  float o = 2.0f;
  float x0 = sample(p + vec2(-o,  0.0));
  float x1 = sample(p + vec2( o,  0.0));
  float y0 = sample(p + vec2( 0.0, -o));
  float y1 = sample(p + vec2( 0.0,  o));

  //use the smallest neighbour in each direction to calculate the partial deriviates
  float xgrad = sign*x0 < sign*x1 ? -(x0-d) : (x1-d);
  float ygrad = sign*y0 < sign*y1 ? -(y0-d) : (y1-d);

  return normalize(vec2(xgrad, ygrad));
}


typedef struct SDF {
  const char *name;
  vec3 dims;
  vec2 half_dims;
  float diagonal_length;
  rawkit_texture_t *tex;
  rawkit_cpu_buffer_t *tex_buf;
  rawkit_cpu_buffer_t *dist_buf;
  bool dirty = true;
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

  void upload() {
    rawkit_texture_update_buffer(this->tex, this->tex_buf);
  }

  void debug_dist() {
    this->upload();
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
