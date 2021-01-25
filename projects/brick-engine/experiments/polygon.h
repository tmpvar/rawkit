#pragma once

#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <stdlib.h>
#include "aabb.h"
#include "sdf.h"
#include <glm/glm.hpp>
using namespace glm;

static char polygon_tmp_str[4096] = "";

typedef struct Polygon {
  vec2 pos;
  float rot;


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
    }

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

    return this->sdf->sample(p);
  }

  float sample_bilinear(vec2 p) {
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
    float d = this->sample_bilinear(p);
    float sign = d >= 0 ? 1.0f : -1.0f;
    float maxval = FLT_MAX * sign;

    //read neighbour distances, ignoring border pixels
    float o = 1.5f;
    float x0 = this->sample_bilinear(p + vec2(-o,  0.0));
    float x1 = this->sample_bilinear(p + vec2( o,  0.0));
    float y0 = this->sample_bilinear(p + vec2( 0.0, -o));
    float y1 = this->sample_bilinear(p + vec2( 0.0,  o));

    //use the smallest neighbour in each direction to calculate the partial deriviates
    float xgrad = sign*x0 < sign*x1 ? -(x0-d) : (x1-d);
    float ygrad = sign*y0 < sign*y1 ? -(y0-d) : (y1-d);

    return normalize(vec2(xgrad, ygrad));
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
;
      //   rawkit_vg_rect(
      //     vg,
      //     this->aabb.lb.x,
      //     this->aabb.lb.y,
      //     this->aabb.ub.x - this->aabb.lb.x,
      //     this->aabb.ub.y - this->aabb.lb.y
      //   );
      //   rawkit_vg_stroke(vg);

    rawkit_vg_restore(vg);

  }
} Polygon;
