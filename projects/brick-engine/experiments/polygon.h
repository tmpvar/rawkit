#pragma once

#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <stdlib.h>
#include "aabb.h"
#include <glm/glm.hpp>
using namespace glm;

static char polygon_tmp_str[4096] = "";

// from: https://github.com/anael-seghezzi/Maratis-Tiny-C-library/blob/master/include/m_raster.h BSD-like license
static void m_raster_line(float *dest, int width, int height, int comp, float *p0, float *p1, float *color) {
   float *data = dest;
   int x0 = (int)p0[0];
   int y0 = (int)p0[1];
   int x1 = (int)p1[0];
   int y1 = (int)p1[1];
   int w = width;
   int h = height;
   int dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
   int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
   int err = dx + dy, e2;

   while (1) {
      // if (x0 > -1 && y0 > -1 && x0 < w && y0 < h) { /* safe, but should be taken out of the loop for speed (clipping ?) */
      float *pixel = data + (y0 * w + x0) * comp; int c;
      for (c = 0; c < comp; c++)
        pixel[c] = color[c];
      // }

      if (x0 == x1 && y0 == y1)
         break;

      e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
   }
}

/* adapted from : http://alienryderflex.com/polygon_fill/
   public-domain code by Darel Rex Finley, 2007
*/
static void m_raster_polygon(float *dest, int width, int height, int comp, float *points, int count, float *color)
{
   float *data = dest;
   int *nodeX;
   int w = width;
   int h = height;
   int pixelY;
   int IMAGE_LEFT, IMAGE_RIGHT;
   int IMAGE_TOP, IMAGE_BOT;

   if (count < 3)
      return;

   nodeX = (int *)malloc(count * sizeof(int));

   /* bounding box */
   {
    float *point = points;
    float lb[2], ub[2];
    int i;

    lb[0] = point[0];
    ub[0] = point[0];
    lb[1] = point[1];
    ub[1] = point[1];
    point += 2;

    for (i = 1; i < count; i++) {
      lb[0] = min(lb[0], point[0]);
      lb[1] = min(lb[1], point[1]);
      ub[0] = max(ub[0], point[0]);
      ub[1] = max(ub[1], point[1]);
      point += 2;
    }

    IMAGE_LEFT =  max(0, (int)lb[0]);
    IMAGE_TOP =   max(0, (int)lb[1]);
    IMAGE_RIGHT = min(w - 1, (int)ub[0] + 1);
    IMAGE_BOT =   min(h - 1, (int)ub[1] + 1);
   }

   /* loop through the rows of the image. */
   for (pixelY = IMAGE_TOP; pixelY < IMAGE_BOT; pixelY++) {

      /* build a list of nodes. */
      float *pointi, *pointj;
      int nodes = 0;
      int i, j;

      pointi = points;
      pointj = points + ((count - 1) * 2);

      for (i = 0; i < count; i++) {
         if ((pointi[1] < (float)pixelY && pointj[1] >= (float)pixelY) ||
             (pointj[1] < (float)pixelY && pointi[1] >= (float)pixelY))
            nodeX[nodes++] = (int)(pointi[0] + (pixelY - pointi[1]) / (pointj[1] - pointi[1]) * (pointj[0] - pointi[0]));
         pointj = pointi;
         pointi += 2;
      }

      /*  sort the nodes, via a simple Bubble sort. */
      i = 0;
      while (i < (nodes - 1)) {
         if (nodeX[i] > nodeX[i + 1]) {
            int swap = nodeX[i]; nodeX[i] = nodeX[i + 1]; nodeX[i + 1] = swap; if (i) i--;
         }
         else i++;
      }

      /* fill the pixels between node pairs. */
      for (i = 0; i < nodes; i += 2) {

         if (nodeX[i] >= IMAGE_RIGHT)
            break;

         if (nodeX[i + 1] > IMAGE_LEFT) {

            float *pixel;
            nodeX[i]     = max(nodeX[i], IMAGE_LEFT);
            nodeX[i + 1] = min(nodeX[i + 1], IMAGE_RIGHT);
            pixel = data + (pixelY * w + nodeX[i]) * comp;

            for (j = nodeX[i]; j < nodeX[i + 1]; j++) {
               int c;
               for (c = 0; c < comp; c++)
                  pixel[c] = color[c];
               pixel += comp;
            }
         }
      }
   }

   free(nodeX);
}

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

typedef struct SDF {
  const char *name;
  vec3 dims;
  rawkit_texture_t *tex;
  rawkit_cpu_buffer_t *tex_buf;
  rawkit_cpu_buffer_t *raster_buf;
  bool dirty = false;
  SDF(const char *name, vec3 dims)
    : dims(dims), name(name)
  {
    this->tex = rawkit_texture_mem(name, dims.x, dims.y, dims.z, VK_FORMAT_R32_SFLOAT);
    sprintf(polygon_tmp_str, "%s::tex_buf", name);
    this->tex_buf = rawkit_cpu_buffer(polygon_tmp_str, this->tex->options.size);
    sprintf(polygon_tmp_str, "%s::raster_buf", name);
    this->raster_buf = rawkit_cpu_buffer(polygon_tmp_str, this->tex->options.size);
  }

  void debug_raster() {
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
        0.0f,
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
      // right now this is our boundary condition
      return FLT_MAX;
    }

    uint64_t i = (
      static_cast<uint64_t>(p.x) +
      static_cast<uint64_t>(p.y * this->dims.x)
    );
    float d = ((float *)this->tex_buf->data)[i] * max(this->dims.x, this->dims.y);
    return d;
  }

  float sample_interp(const vec2 p) {
      float lx = floor(p.x);
      float ly = floor(p.y);
      float ux = lx + 1.0;
      float uy = ly + 1.0;
      int lxi = (int)lx;
      int lyi = (int)ly;
      int uxi = (int)ux;
      int uyi = (int)uy;
      return (
        this->sample(vec2(lxi, lyi)) * (ux - p.x) * (uy - p.y) +
        this->sample(vec2(uxi, lyi)) * (p.x - lx) * (uy - p.y) +
        this->sample(vec2(lxi, uyi)) * (ux - p.x) * (p.y - ly) +
        this->sample(vec2(uxi, uyi)) * (p.x - lx) * (p.y - ly)
      );
  }


  // grabbed from https://github.com/chriscummings100/signeddistancefields/blob/master/Assets/SignedDistanceFields/SignedDistanceFieldGenerator.cs
  vec2 calcNormal(vec2 p ) {

    //get d, and also its sign (i.e. inside or outside)
    float d = this->sample(p);
    float sign = d >= 0 ? 1.0f : -1.0f;
    float maxval = FLT_MAX * sign;

    //read neighbour distances, ignoring border pixels

    float x0 = this->sample_interp(p + vec2(-1.0, 0.0));
    float x1 = this->sample_interp(p + vec2( 1.0, 0.0));
    float y0 = this->sample_interp(p + vec2(0.0, -1.0));
    float y1 = this->sample_interp(p + vec2(0.0, 1.0));

    //use the smallest neighbour in each direction to calculate the partial deriviates
    float xgrad = sign*x0 < sign*x1 ? -(x0-d) : (x1-d);
    float ygrad = sign*y0 < sign*y1 ? -(y0-d) : (y1-d);

    return vec2(xgrad, ygrad);
  }


  void add_point(uvec2 p, float val) {
    uint64_t i = (
      static_cast<uint64_t>(p.x) +
      static_cast<uint64_t>(p.y * this->dims.x)
    );
    if (i >= this->raster_buf->size) {
      return;
    }

    ((float *)this->raster_buf->data)[i] = val;
  }

  void add_line(vec2 p1, vec2 p2) {
    float color = 1.0f;
    m_raster_line(
      (float *)this->raster_buf->data,
      this->tex->options.width,
      this->tex->options.height,
      1,
      (float *)&p1,
      (float *)&p2,
      &color
    );
    this->dirty = true;
  }
} SDF;

typedef struct Polygon {
  SDF *sdf = NULL;
  AABB aabb;
  vec2 *points = NULL;
  char *name = NULL;
  Polygon(const char *name) {
    if (name) {
      this->name = (char *)calloc(strlen(name) + 1, 1);
      strcpy(this->name, name);
    }
  }

  void rebuild_sdf() {
    // see: https://github.com/dy/bitmap-sdf/blob/master/index.js#L92
    uint32_t count = sb_count(this->points);
    if (!count) return;
    this->aabb.lb = vec2(FLT_MAX);
    this->aabb.ub = vec2(-FLT_MAX);
    for (uint32_t i=0; i<count; i++) {
      this->aabb.grow(this->points[i]);
    }

    AABB inflated = this->aabb.copy_inflated(0.0);

    if (!this->sdf) {
      this->sdf = new SDF(
        this->name,
        vec3(
          inflated.width(),
          inflated.height(),
          1.0
        )
      );
    } else {
      memset(this->sdf->raster_buf->data, 0, this->sdf->raster_buf->size);
    }
#if 0
    vec2 prev = this->points[0] - inflated.lb;
    for (uint32_t i=1; i<count; i++) {
      vec2 cur = this->points[i] - inflated.lb;
      this->sdf->add_line(prev, cur);
      prev = cur;
    }

    // auto close
    this->sdf->add_line(prev, this->points[0] - inflated.lb);
#else

    float *transformed = (float *)malloc(sizeof(float) * count * 2);
    for (uint32_t i=0; i<count; i++) {
      vec2 cur = this->points[i] - inflated.lb;
      transformed[i*2 + 0] = cur.x;
      transformed[i*2 + 1] = cur.y;
    }

    float color = 1.0f;
    m_raster_polygon(
      (float *)this->sdf->raster_buf->data,
      this->sdf->tex->options.width,
      this->sdf->tex->options.height,
      1,
      transformed,
      count,
      &color
    );

    free(transformed);

#endif

    this->sdf->rebuild();
  }

  void append(vec2 point) {
    sb_push(this->points, point);
  }

  void render(rawkit_vg_t *vg) {
    uint32_t count = sb_count(this->points);
    if (!count) return;
    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));

    rawkit_vg_begin_path(vg);
    vec2 p = this->points[0];
    rawkit_vg_move_to(vg, p.x, p.y);
    for (uint32_t i=1; i<count; i++) {
      p = this->points[i];
      rawkit_vg_line_to(vg, p.x, p.y);
    }
    rawkit_vg_close_path(vg);
    rawkit_vg_fill(vg);

    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0x00, 0x00));
    rawkit_vg_begin_path(vg);
      rawkit_vg_rect(
        vg,
        this->aabb.lb.x,
        this->aabb.lb.y,
        this->aabb.ub.x - this->aabb.lb.x,
        this->aabb.ub.y - this->aabb.lb.y
      );
      rawkit_vg_stroke(vg);

  }
} Polygon;
