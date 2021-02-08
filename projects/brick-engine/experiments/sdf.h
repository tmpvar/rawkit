#pragma once
#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

#define PI 3.1415926f

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

// distance meater from https://www.shadertoy.com/view/ldK3zD
// the meter uses the "fusion" gradient, which goes from dark magenta (0) to white (1)
// (often seen in heatmaps in papers etc)
//

vec3 fusion(float x) {
	float t = glm::clamp(x, 0.0f, 1.0f);
	return glm::clamp(
    vec3(
      sqrt(t),
      t*t*t,
      glm::max(sin(PI*1.75f*t), pow(t, 12.0f))
    ),
    0.0f,
    1.0f
  );
}

// HDR version
vec3 fusionHDR(float x) {
	float t = clamp(x, 0.0f, 1.0f);
	return fusion(sqrt(t))*(0.5f + 2.0f * t);
}


//
// distance meter function. needs a bit more than just the distance
// to estimate the zoom level that it paints at.
//
// if you have real opengl, you can additionally use derivatives (dFdx, dFdy)
// to detect discontinuities, i had to strip that for webgl
//
// visualizing the magnitude of the gradient is also useful
//

vec3 distanceMeter(float dist, float rayLength, vec3 rayDir, float camHeight) {
    float idealGridDistance = 20.0f/rayLength*pow(abs(rayDir.y),0.8f);
    float nearestBase = floor(log(idealGridDistance)/log(10.0f));
    float relativeDist = abs(dist/camHeight);

    float largerDistance = glm::pow(10.0f,nearestBase+1.0f);
    float smallerDistance = glm::pow(10.0f,nearestBase);


    vec3 col = fusionHDR(log(1.+relativeDist));
    col = max(vec3(0.),col);
    if (sign(dist) < 0.) {
        col = vec3(col.g, col.r, col.b)*3.f;
    }

    float l0 = (glm::pow(0.5f+0.5f*cos(dist * PI * 2.f * smallerDistance),10.0f));
    float l1 = (glm::pow(0.5f+0.5f*cos(dist * PI * 2.f * largerDistance),10.0f));

    float x = fract(log(idealGridDistance)/log(10.0f));
    l0 = glm::mix(l0, 0.0f, glm::smoothstep(0.5f,1.0f,x));
    l1 = glm::mix(0.0f, l1, glm::smoothstep(0.0f,0.5f,x));

    col *= 0.1f+0.9f*(1.0f-l0)*(1.0f-l1);
    return col;
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
    this->tex = rawkit_texture_mem(name, dims.x, dims.y, dims.z, VK_FORMAT_R32G32B32_SFLOAT);
    sprintf(sdf_tmp_str, "%s::tex_buf", name);
    this->tex_buf = rawkit_cpu_buffer(sdf_tmp_str, this->tex->options.size);
    sprintf(sdf_tmp_str, "%s::dist_buf", name);
    this->dist_buf = rawkit_cpu_buffer(sdf_tmp_str, static_cast<uint64_t>(dims.x * dims.y * sizeof(float)));
    this->half_dims = vec2(this->dims) * 0.5f;
  }

  void upload() {
    rawkit_texture_update_buffer(this->tex, this->tex_buf);
  }

  void debug_dist() {
    if (this->dirty) {
      this->dirty = false;
      vec2 p(0.0);
      for (p.x = 0.0; p.x < dims.x; p.x++) {
        for (p.y = 0.0; p.y < dims.y; p.y++) {

          uint64_t src = static_cast<uint64_t>(
            p.x +
            p.y * dims.x
          );

          uint64_t dst = static_cast<uint64_t>(
            p.x +
            p.y * dims.x
          );

          float dist = ((float *)this->dist_buf->data)[src];
          vec3 col = distanceMeter(
            dist * 150.0f,
            40000.0f,
            vec3(0.0, 1.0, 0.0),
            50000.0f
          );

          ((vec3 *)this->tex_buf->data)[dst] = col;
        }
      }
    }

    this->upload();
    {
      ImTextureID texture = rawkit_imgui_texture(this->tex, this->tex->default_sampler);
      if (!texture) {
        printf("could not create a valid imgui texture from distance texture\n");
        return;
      }

      igText("sdf->tex '%s' dims(%f, %f)",
        this->name,
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
    this->dirty = true;
  }
} SDF;
