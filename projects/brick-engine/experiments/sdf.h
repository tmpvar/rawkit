#pragma once
#include <rawkit/rawkit.h>
#include <glm/glm.hpp>
using namespace glm;

static char sdf_tmp_str[4096] = "";

// 1D squared distance transform
static void edt1d(float *f, float *d, uint32_t *v, float *z, uint32_t n) {
    v[0] = 0;
    z[0] = -FLT_MAX;
    z[1] = +FLT_MAX;

    for (uint32_t q = 1, k = 0; q < n; q++) {
      float s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);
      while (s <= z[k]) {
        k--;
        s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);
      }
      k++;
      v[k] = q;
      z[k] = s;
      z[k + 1] = +FLT_MAX;
    }

    for (uint32_t q = 0, k = 0; q < n; q++) {
        while (z[k + 1] < q) {
          k++;
        }
        d[q] = (q - v[k]) * (q - v[k]) + f[v[k]];
    }
}

// 2D Euclidean distance transform by Felzenszwalb & Huttenlocher https://cs.brown.edu/~pff/dt/
static void edt(float *data, uint32_t width, uint32_t height, float *f, float *d, uint32_t *v, float *z) {
    for (uint32_t x = 0; x < width; x++) {
        for (uint32_t y = 0; y < height; y++) {
          f[y] = data[y * width + x];
        }
        edt1d(f, d, v, z, height);
        for (uint32_t y = 0; y < height; y++) {
          data[y * width + x] = d[y];
        }
    }
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
          f[x] = data[y * width + x];
        }
        edt1d(f, d, v, z, width);
        for (uint32_t x = 0; x < width; x++) {
          data[y * width + x] = sqrt(d[x]);
        }
    }
}

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
  rawkit_cpu_buffer_t *raster_buf;
  bool dirty = false;
  SDF(const char *name, vec3 dims)
    : dims(dims), name(name)
  {
    this->diagonal_length = length(dims);
    this->tex = rawkit_texture_mem(name, dims.x, dims.y, dims.z, VK_FORMAT_R32_SFLOAT);
    sprintf(sdf_tmp_str, "%s::tex_buf", name);
    this->tex_buf = rawkit_cpu_buffer(sdf_tmp_str, this->tex->options.size);
    sprintf(sdf_tmp_str, "%s::raster_buf", name);
    this->raster_buf = rawkit_cpu_buffer(sdf_tmp_str, this->tex->options.size);
    this->half_dims = this->dims * 0.5f;
  }

  void debug_raster() {
    return;
    rawkit_texture_update_buffer(this->tex, this->raster_buf);
    {
      ImTextureID texture = rawkit_imgui_texture(this->tex, this->tex->default_sampler);
      if (!texture) {
        return;
      }

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

  void debug_dist() {
    return;
    rawkit_texture_update_buffer(this->tex, this->tex_buf);
    {
      ImTextureID texture = rawkit_imgui_texture(this->tex, this->tex->default_sampler);
      if (!texture) {
        return;
      }

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

  void rebuild() {
    uint32_t w = (uint32_t)ceil(this->dims.x);
    uint32_t h = (uint32_t)ceil(this->dims.y);
    uint32_t size = max(w, h);
    uint32_t pixel_count = w * h;

    // temporary arrays for the distance transform
    float *gridOuter = (float *)malloc(sizeof(float) * pixel_count);
    float *gridInner = (float *)malloc(sizeof(float) * pixel_count);
    float *f = (float *)malloc(sizeof(float) * size);
    float *d = (float *)malloc(sizeof(float) * size);
    float *z = (float *)malloc(sizeof(float) * size + 1);
    uint32_t *v = (uint32_t *)malloc(sizeof(uint32_t) * size);

    for (uint32_t i = 0; i < pixel_count; i++) {
        float a = ((float *)this->raster_buf->data)[i];

        // outer
        if (a >= 1.0f) {
          gridOuter[i] = 0.0f;
        } else if (a<=0.0f) {
          gridOuter[i] = FLT_MAX;
        } else {
          gridOuter[i] = pow(max(0.0f, 0.5f - a), 2.0f);
        }

        // inner
        if (a >= 1.0f) {
          gridInner[i] = FLT_MAX;
        } else if (a<=0.0f) {
          gridInner[i] = 0.0f;
        } else {
          gridInner[i] = pow(max(0.0f, a - 0.5f), 2.0f);
        }
    }

    edt(gridOuter, w, h, f, d, v, z);
    edt(gridInner, w, h, f, d, v, z);

    float cutoff = 0.25f;//size;//.25f;
    float radius = size;//8.0f;

    float *out = (float *)this->tex_buf->data;
    for (uint32_t i = 0; i < pixel_count; i++) {
      out[i] = clamp(
        ( (gridOuter[i] - gridInner[i]) / radius + cutoff),
        -1.0f,
         1.0f
      );

      out[i] =  ( (gridOuter[i] - gridInner[i]) / radius);// + cutoff);

    }

    // TODO: if we resize then we should copy the old raster buffer
    this->dirty = false;
  }

  float sample(vec2 p) {
    if (
      p.x < 0.0f || p.x >= this->dims.x ||
      p.y < 0.0f || p.y >= this->dims.y
    ) {
      return sdBox(p - this->half_dims, this->half_dims);
    }

    uint64_t i = (
      static_cast<uint64_t>(p.x) +
      static_cast<uint64_t>(p.y * this->dims.x)
    );
    // normalized distance (-1..1)
    #if 0
      float d = ((float *)this->tex_buf->data)[i] * max(this->dims.x, this->dims.y);
    // exact distance
    #else
      float d = ((float *)this->tex_buf->data)[i];
    #endif
    return d;
  }

  float sample_interp(const vec2 p) {
    float lx = floor(p.x);
    float ly = floor(p.y);

    vec2 lb = floor(p);
    vec2 ub = lb + 1.0f;

    return (
      this->sample(vec2(lb.x, lb.y)) * (ub.x - p.x)  * (ub.y - p.y)  +
      this->sample(vec2(ub.x, lb.y)) * (p.x  - lb.x) * (ub.y - p.y)  +
      this->sample(vec2(lb.x, ub.y)) * (ub.x - p.x)  * (p.y  - lb.y) +
      this->sample(vec2(ub.x, ub.y)) * (p.x  - lb.x) * (p.y  - lb.y)
    );
  }


  // grabbed from https://github.com/chriscummings100/signeddistancefields/blob/master/Assets/SignedDistanceFields/SignedDistanceFieldGenerator.cs
  vec2 calcNormal(vec2 p) {
    //get d, and also its sign (i.e. inside or outside)
    float d = this->sample_interp(p);
    float sign = d >= 0 ? 1.0f : -1.0f;
    float maxval = FLT_MAX * sign;

    //read neighbour distances, ignoring border pixels
    float o = 1.5f;
    float x0 = this->sample_interp(p + vec2(-o,  0.0));
    float x1 = this->sample_interp(p + vec2( o,  0.0));
    float y0 = this->sample_interp(p + vec2( 0.0, -o));
    float y1 = this->sample_interp(p + vec2( 0.0,  o));

    //use the smallest neighbour in each direction to calculate the partial deriviates
    float xgrad = sign*x0 < sign*x1 ? -(x0-d) : (x1-d);
    float ygrad = sign*y0 < sign*y1 ? -(y0-d) : (y1-d);

    return vec2(xgrad, ygrad);
  }


  void write(uvec2 p, float val) {
    uint64_t i = (
      static_cast<uint64_t>(p.x) +
      static_cast<uint64_t>(p.y * this->dims.x)
    );
    if (i >= this->tex_buf->size) {
      return;
    }

    ((float *)this->tex_buf->data)[i] = val;
  }
} SDF;
