#pragma once

#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <stdlib.h>
#include "aabb.h"
#include "sdf.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
using namespace glm;

static char polygon_tmp_str[4096] = "";

typedef struct Polygon {
  vec2 pos = vec2(0.0);
  float rot = 0.0;

  vec2 center_of_mass = vec2(0.0);

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

    printf("rebuild sdf for %s\n", this->name);

    this->aabb.lb = vec2(FLT_MAX);
    this->aabb.ub = vec2(-FLT_MAX);
    for (uint32_t i=0; i<count; i++) {
      this->aabb.grow(this->points[i]);
    }

    uint32_t inside_cell_count = 0;
    uvec2 inside_cell_sum(0);

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
        vec2 cur = this->points[0];

        float d = dot(p - cur, p - cur);
        for(int i=0; i<count; i++) {
          // distance
          cur = this->points[i];
          vec2 next = this->points[(i+1)%count];
          vec2 e = next - cur;
          vec2 v = p - cur;
          vec2 pq = v - e * clamp( dot(v,e) / dot(e,e), 0.0f, 1.0f );
          d = min( d, dot(pq, pq));

          // winding number from http://geomalgorithms.com/a03-_inclusion.html
          vec2 v2 = p - next;
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

        if (signed_distance <= 0.0) {
          inside_cell_sum += uvec2(p);
          inside_cell_count++;
        }

        this->sdf->write(uvec2(p - inflated.lb), signed_distance);
      }
    }
    this->center_of_mass = vec2(inside_cell_sum / inside_cell_count);
    this->dirty = false;
  }

  void append(vec2 point) {
    this->dirty = true;
    sb_push(this->points, point);
  }

  vec2 worldToLocal(vec2 p) {
    // translate p into grid space
    vec2 local_center = (this->aabb.ub - this->aabb.lb) * 0.5f;
    vec2 world_center = local_center + this->pos;

    vec2 diff = p - world_center;
    vec2 grid_pos = rotate(diff, -this->rot);
    return grid_pos + local_center;
  }

  float sample_world(vec2 p) {
    return this->sample_local(this->worldToLocal(p));
  }

  float sample_local(vec2 p) {
    if (!this->sdf || this->dirty) {
      this->rebuild_sdf();
    }

    return this->sdf->sample(p);
  }

  float sample_bilinear_world(vec2 p) {
    return this->sample_bilinear_local(this->worldToLocal(p));
  }

  float sample_bilinear_local(vec2 p) {
    float lx = floor(p.x);
    float ly = floor(p.y);
    vec2 lb = floor(p);
    vec2 ub = lb + 1.0f;
    return (
      this->sample_local(vec2(lb.x, lb.y)) * (ub.x - p.x)  * (ub.y - p.y)  +
      this->sample_local(vec2(ub.x, lb.y)) * (p.x  - lb.x) * (ub.y - p.y)  +
      this->sample_local(vec2(lb.x, ub.y)) * (ub.x - p.x)  * (p.y  - lb.y) +
      this->sample_local(vec2(ub.x, ub.y)) * (p.x  - lb.x) * (p.y  - lb.y)
    );
  }


  // grabbed from https://github.com/chriscummings100/signeddistancefields/blob/master/Assets/SignedDistanceFields/SignedDistanceFieldGenerator.cs
  vec2 calc_normal_world(vec2 p) {
    //get d, and also its sign (i.e. inside or outside)
    float d = this->sample_bilinear_world(p);
    float sign = d >= 0 ? 1.0f : -1.0f;
    float maxval = FLT_MAX * sign;

    //read neighbour distances, ignoring border pixels
    float o = 2.0f;
    float x0 = this->sample_bilinear_world(p + vec2(-o,  0.0));
    float x1 = this->sample_bilinear_world(p + vec2( o,  0.0));
    float y0 = this->sample_bilinear_world(p + vec2( 0.0, -o));
    float y1 = this->sample_bilinear_world(p + vec2( 0.0,  o));

    //use the smallest neighbour in each direction to calculate the partial deriviates
    float xgrad = sign*x0 < sign*x1 ? -(x0-d) : (x1-d);
    float ygrad = sign*y0 < sign*y1 ? -(y0-d) : (y1-d);

    return normalize(vec2(xgrad, ygrad));
  }

  void render(rawkit_vg_t *vg) {
    uint32_t count = sb_count(this->points);
    if (!count) return;

    rawkit_vg_save(vg);
    // this->rot = 0.5;//rawkit_now();
    vec2 bounds = this->aabb.ub - this->aabb.lb;
    vec2 center = bounds * 0.5f;

    rawkit_vg_translate(vg, this->pos.x + center.x, this->pos.y + center.y);
    rawkit_vg_rotate(vg, this->rot);
    rawkit_vg_translate(vg, -center.x, -center.y);
    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
    rawkit_vg_begin_path(vg);
    vec2 p = this->points[0];
    rawkit_vg_move_to(vg, p.x, p.y);
    for (uint32_t i=1; i<count; i++) {
      p = this->points[i];
      rawkit_vg_line_to(vg, p.x, p.y);
    }
    rawkit_vg_close_path(vg);
    rawkit_vg_stroke(vg);
    rawkit_vg_restore(vg);


    rawkit_vg_fill_color(vg, rawkit_vg_RGB(0xFF, 0, 0));
    rawkit_vg_begin_path(vg);
      rawkit_vg_arc(
        vg,
        this->pos.x + this->center_of_mass.x,
        this->pos.y + this->center_of_mass.y,
        4.0,
        0.0,
        6.283185307179586,
        1
      );
      rawkit_vg_fill(vg);

  }
} Polygon;
