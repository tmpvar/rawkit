#pragma once

#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <stdlib.h>
#include "aabb.h"
#include "sdf.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
using namespace glm;

static char polygon_tmp_str[4096] = "";

// adapted from : http://alienryderflex.com/polygon_fill/
//   public-domain code by Darel Rex Finley, 2007
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

typedef struct Polygon {
  vec2 pos;
  quat rot;


  SDF *sdf = NULL;
  AABB aabb;
  vec2 *points = NULL;
  char *name = NULL;

  bool dirty = false;
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

    // TODO: changing this will break the sdf->write call below
    AABB inflated = this->aabb.copy_inflated(0.0f);

    if (!this->sdf) {
      this->sdf = new SDF(
        this->name,
        vec3(
          inflated.width(),
          inflated.height(),
          1.0
        )
      );
    } else if (this->sdf->raster_buf && this->sdf->raster_buf->data) {
      memset(this->sdf->raster_buf->data, 0, this->sdf->raster_buf->size);
    }
    // distance transform on a binary image
    // TODO: -x gradients are busted
    #if 0
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

      this->sdf->rebuild();
    // brute force, exact distance field
    #else

      vec2 p(0.0);
      vec2 dims = inflated.ub - inflated.lb;

      for (p.x = 0.0; p.x < dims.x; p.x++) {
        for (p.y = 0.0; p.y < dims.y; p.y++) {
          // from: https://www.shadertoy.com/view/3d23WK
          // signed distance to a 2D polygon
          // adapted from triangle
          // http://www.iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
          float s = 1.0;
          float d = dot(p - this->points[0], p - this->points[0]);
          for(int i=0; i<count; i++) {
            // distance
            int i2 = (i+1)%count;
            vec2 e = this->points[i2] - this->points[i];
            vec2 v = p - this->points[i];
            vec2 pq = v - e * clamp( dot(v,e) / dot(e,e), 0.0f, 1.0f );
            d = min( d, dot(pq, pq));

            // winding number from http://geomalgorithms.com/a03-_inclusion.html
            vec2 v2 = p - this->points[i2];
            float val3 = e.x * v.y - e.y * v.x; //isLeft
            bvec3 cond = bvec3(
              v.y >= 0.0f,
              v2.y < 0.0f,
              val3 > 0.0f
            );

            bvec3 counter(!cond.x, !cond.y, !cond.z);

            if(all(cond) || all(not_(cond))) {
              s*=-1.0f;  // have a valid up or down intersect
            }
          }

          float signed_distance = glm::sqrt(d) * s;
          this->sdf->write(uvec2(p - inflated.lb), signed_distance);
        }
      }
    #endif
    this->dirty = false;
  }

  void append(vec2 point) {
    this->dirty = true;
    sb_push(this->points, point);
  }

  float sample(vec2 p) {
    if (!this->sdf || this->dirty) {
      this->rebuild_sdf();
    }

    if (!this->aabb.contains(p)) {
      vec2 nearest = this->aabb.nearest(p);
      return distance(nearest, p);
    }

    return this->sdf->sample(p);
  }

  float sample_bilinear(vec2 p) {
    if (!this->sdf || this->dirty) {
      this->rebuild_sdf();
    }
    return this->sdf->sample(p);
  }

  void render(rawkit_vg_t *vg) {
    uint32_t count = sb_count(this->points);
    if (!count) return;

    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
    rawkit_vg_begin_path(vg);
    vec2 p = this->points[0] + this->pos;
    rawkit_vg_move_to(vg, p.x, p.y);
    for (uint32_t i=1; i<count; i++) {
      p = this->points[i] + this->pos;
      rawkit_vg_line_to(vg, p.x, p.y);
    }
    rawkit_vg_close_path(vg);
    rawkit_vg_fill(vg);

    // rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0x00, 0x00));
    // rawkit_vg_begin_path(vg);
    //   rawkit_vg_rect(
    //     vg,
    //     this->aabb.lb.x,
    //     this->aabb.lb.y,
    //     this->aabb.ub.x - this->aabb.lb.x,
    //     this->aabb.ub.y - this->aabb.lb.y
    //   );
    //   rawkit_vg_stroke(vg);

  }
} Polygon;
