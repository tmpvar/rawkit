#pragma once

#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <stdlib.h>
#include "aabb.h"
#include "sdf.h"
#include "segseg.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/compatibility.hpp>
using namespace glm;

#include <vector>
using namespace std;

static char polygon_tmp_str[4096] = "";

typedef struct PolygonIntersection {
  uint32_t start_idx;
  uint32_t end_idx;
  vec2 pos;
} PolygonIntersection;

float orientation(vec2 start, vec2 end, vec2 point) {

  float v = (
    (end.y - start.y) *
    (point.x - end.x) -
    (end.x - start.x) *
    (point.y - end.y)
  );

  printf("orient start(%f, %f) end(%f, %f) point(%f, %f) -> %f\n", start.x, start.y, end.x, end.y, point.x, point.y, v);

  if (v == 0.0f) {
    return v;
  }

  return v < 0.0f ? -1.0f : 1.0f;
}

typedef struct Polygon {
  vec2 pos = vec2(0.0);
  vec2 prev_pos = vec2(0.0);
  float rot = 0.0f;
  float prev_rot = 0.0f;
  float angular_velocity = 0.0f;
  vec2 velocity = vec2(0.0);

  vec2 center_of_mass = vec2(0.0);
  vec2 center_of_mass_offset = vec2(0.0);

  SDF *sdf = NULL;
  AABB aabb;
  vec2 *points = NULL;
  char *name = NULL;

  float density = 1.0f / 25.0f;

  float mass = 0.0f;
  float inv_mass = 1.0f;

  mat2 inertia;
  mat2 inv_inertia;

  bool dirty = false;
  Polygon(const char *name) {
    if (name) {
      this->name = (char *)calloc(strlen(name) + 1, 1);
      strcpy(this->name, name);
    }
  }


  void rebuild_sdf() {
    // if no new points have been added
    if (!this->dirty) return;

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

    // TODO: changing this may break the sdf->write call below
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

    printf("compute sdf for %s\n", this->name);
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
          d = glm::min( d, dot(pq, pq));

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

    printf("polygon %s:\n", this->name);
    printf("  inside_cell_sum(%u, %u)\n", inside_cell_sum.x, inside_cell_sum.y);
    float area = (float)inside_cell_count;
    // this->mass = (area * this->density) * 0.02;
    this->mass = area * density;
    printf("  mass(%f) = dims(%f, %f) * density(%f)\n", this->mass, dims.x, dims.y, this->density);
    this->inv_mass = 1.0f / this->mass;

    // compute the inertial tensor
    // TODO: this assumes a square.. we need to handle arbitrary polygons.
    {
      vec2 b2 = 4.0f * pow(dims, vec2(2.0f));
      float x = 1.0f / 12.0f * this->mass * b2.x;
      //float x = this->mass / area;

      mat2 I(
        x  , 0.0,
        0.0, x
      );
      vec2 pos = this->aabb.lb;

      // Transform tensor to local space
      // I = local.rotation * I * -this->rot;
      // I += (identity * dot(pos, pos) - glm::outerProduct(pos, pos)) * this->mass;
      mat2 identity(1.0);
      this->inertia = I;
      this->inv_inertia = inverse(I);
      printf("  inertia\n    (%f, %f,\n    %f, %f)\n", I[0][0], I[0][1], I[1][0], I[1][1]);
      printf("  inv_inertia\n    (%f, %f,\n    %f, %f)\n",
        this->inv_inertia[0][0],
        this->inv_inertia[0][1],
        this->inv_inertia[1][0],
        this->inv_inertia[1][1]
      );
    }


    this->center_of_mass = vec2(inside_cell_sum) / (float)inside_cell_count;

    // recenter so that the center of mass is (0,0)
    {
      vec2 center = (this->aabb.ub - this->aabb.lb) * 0.5f;
      this->center_of_mass_offset = this->center_of_mass - center;
      vec2 diff = this->center_of_mass;
      this->center_of_mass = vec2(0.0);
      this->aabb.lb = vec2(FLT_MAX);
      this->aabb.ub = vec2(-FLT_MAX);

      for(int i=0; i<count; i++) {
        this->points[i] -= diff;
        this->aabb.grow(this->points[i]);
      }
    }

    this->dirty = false;
    this->sdf->upload();
  }

  void append(vec2 point) {
    this->dirty = true;
    sb_push(this->points, point);
  }

  vec2 worldToLocal(vec2 p) {
    vec2 diff = p - this->pos;
    vec2 grid_pos = rotate(diff, -this->rot);
    return grid_pos + this->aabb.ub + this->center_of_mass_offset * 2.0f;
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
      rawkit_vg_translate(vg, this->pos.x, this->pos.y);
      rawkit_vg_rotate(vg, this->rot);

      if (0) {
        rawkit_vg_draw_texture(
          vg,
          this->aabb.lb.x,
          this->aabb.lb.y,
          this->aabb.ub.x - this->aabb.lb.x,
          this->aabb.ub.y - this->aabb.lb.y,
          this->sdf->tex,
          this->sdf->tex->default_sampler
        );
      }


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

      // draw the center of mass
      rawkit_vg_fill_color(vg, rawkit_vg_RGB(0xFF, 0, 0));
      rawkit_vg_begin_path(vg);
        rawkit_vg_arc(
          vg,
          0.0,
          0.0,
          4.0,
          0.0,
          6.283185307179586,
          1
        );
        rawkit_vg_fill(vg);
    rawkit_vg_restore(vg);
  }

  PolygonIntersection *isect_segment(vec2 start, vec2 end) {
    PolygonIntersection *ret = nullptr;
    uint32_t count = sb_count(this->points);
    if (!count) return nullptr;

    vec2 prev = glm::rotate(this->points[count - 1], this->rot) + this->pos;
    for (uint32_t i=0; i<count; i++) {

      vec2 cur = glm::rotate(this->points[i], this->rot) + this->pos;
      vec2 res(0.0);
      if (segseg(prev, cur, start, end, &res) == SEGSEG_DO_INTERSECT) {
        PolygonIntersection isect = {
          .start_idx = i == 0 ? count - 1 : i - 1,
          .end_idx = i,
          .pos = res
        };

        sb_push(ret, isect);
      }
      prev = cur;
    }

    return ret;
  }

  Polygon **split_by_segment(vec2 start, vec2 end) {
    uint32_t count = sb_count(this->points);
    if (!count) return nullptr;


    vec2 prev = glm::rotate(this->points[count - 1], this->rot) + this->pos;
    float o_init = orientation(start, end, prev);
    Polygon **polygons = nullptr;

    sprintf(polygon_tmp_str, "%s::split((%f,%f)->(%f,%f))@orient(%f)", this->name, start.x, start.y, end.x, end.y, o_init);
    sb_push(polygons, new Polygon(polygon_tmp_str));

    for (uint32_t i=0; i<count; i++) {
      vec2 cur = glm::rotate(this->points[i], this->rot) + this->pos;
      float o_cur = orientation(start, end, cur);
      // TODO: add the intersection point to both polygons
      if (o_cur != o_init) {
        o_init = o_cur;
        sprintf(polygon_tmp_str, "%s::split((%f,%f)->(%f,%f))@orient(%f)", this->name, start.x, start.y, end.x, end.y, o_init);
        sb_push(polygons, new Polygon(polygon_tmp_str));
      }


      printf("add point to polygon: %u\n", sb_count(polygons) - 1);
      sb_push(sb_last(polygons)->points, this->points[i]);

      // vec2 res(0.0);
      // if (segseg(prev, cur, start, end, &res) == SEGSEG_DO_INTERSECT) {

      // }
    }

    return polygons;
  }

} Polygon;
