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

  // circle packing
  vec4 *circles = NULL;
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

  void rebase_on_origin() {
    vec2 lb = this->aabb.lb;
    if (all(equal(lb, vec2(0.0)))) {
      return;
    }

    uint32_t count = sb_count(this->points);
    for (uint32_t i=0; i<count; i++) {
      this->points[i] -= lb;
    }
    this->pos = (this->aabb.lb + this->aabb.ub) * 0.5f;
    this->aabb.ub -= lb;
    this->aabb.lb = vec2(0.0);
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

  // the actual recursive packing algorithm
  void circle_pack_split_region(vec2 lb, vec2 ub, uint32_t depth = 0, vec4 best_parent = vec4(-1.0f)) {
    if (depth > 5) {
      // printf("break depth\n");
      return;
    }

    vec2 cp = (lb + ub) * 0.5f;
    float dist = this->sample_local(cp);// + this->aabb.ub + this->center_of_mass_offset * 2.0f);
    float udist = abs(dist);
    float radius = glm::min(
      glm::abs(ub.x - cp.x),
      glm::abs(ub.y - cp.y)
    );

    // if (radius < .01f) {
    //   return;
    // }

    vec4 circle = vec4(cp, radius, (float)depth);
    printf("dist(%f) radius(%f)\n", dist, radius);
    uint32_t new_depth = depth + 1;
#if 0
    // a crossing
    if (dist < -radius || dist > radius) {
      return;
    }

    if (best_parent.x != -1.0f) {
      printf("not first: %f -> %f\n", circle.z, abs(distance(vec2(best_parent), cp) - (circle.z + best_parent.z)));
      circle.z = glm::min(
        circle.z,
        abs(distance(vec2(best_parent), cp) - (circle.z + best_parent.z))
      );
    }

    // fully inside
    // if (dist + radius < 0.0f) {
    //   if (depth == 0) {//} && distance(vec2(best_parent), cp) + radius < best_parent.z) {
    //     return;
    //   }

      // sb_push(this->circles, circle);
    //   // best_parent = circle;
    //      return;
    //  }
      best_parent = circle;
      sb_push(this->circles, circle);

    // // crossing
    // if (dist <= 0.0f && dist + radius >= 0.0f) {

    // }

    // if (udist > radius) {
    //   return;
    // }


    // // skip splits that could occur inside the best parent
    // if (depth > 0) {
    //   float dist = distance(vec2(best_parent), cp);

    //   // if this cell is completely inside the best parent then stop
    //   if (dist + radius < best_parent.z) {
    //     return;
    //   }

    //   // cell is outside parent, so it will be the children's new best parent
    //   if (abs(dist - best_parent.z) > radius) {
    //     best_parent = circle;
    //     sb_push(this->circles, circle);
    //   }
    // } else {
    //   if (radius > udist) {
    //     //new_depth = 0;
    //   } else {
    //     sb_push(this->circles, circle);
    //     best_parent = circle;
    //   }
    // }
    // printf("split (%f, %f) -> (%f, %f)\n", lb.x, lb.y, ub.x, ub.y);
#else

    float a = glm::sign(this->sample_local(cp + vec2(-radius, -radius)));
    float b = glm::sign(this->sample_local(cp + vec2( radius, -radius)));
    float c = glm::sign(this->sample_local(cp + vec2( radius,  radius)));
    float d = glm::sign(this->sample_local(cp + vec2(-radius,  radius)));

    bool crossing = (
      a != b ||
      b != c ||
      c != d ||
      d != glm::sign(dist)
    );

    if (dist < -radius || dist > radius) {
      printf("bail..\n");
      return;
    }

    if (crossing) {
      sb_push(this->circles, circle);
    }
    // if (!crossing) {
    //   return;
    // }


    // if (new_depth > 0) {
    //   if (dist > 0.0f) {
    //     return;
    //   }
    // }

    best_parent = circle;



    // // only begin work after we've split to a reasonable level
    // if (best_parent.x == -1.0f) {
    //   if (dist <= radius) {
    //     best_parent = circle;
    //     sb_push(this->circles, circle);
    //   }
    // } else {
    //   if (distance(vec2(best_parent), cp) - radius > best_parent.z) {
    //     best_parent = circle;
    //     sb_push(this->circles, circle);
    //   }
    // }
#endif

    // lower left
    this->circle_pack_split_region(
      lb,
      cp,
      new_depth,
      best_parent
    );

    // lower right
    this->circle_pack_split_region(
      vec2(cp.x, lb.y),
      vec2(ub.x, cp.y),
      new_depth,
      best_parent
    );

    // upper right
    this->circle_pack_split_region(
      cp,
      ub,
      new_depth,
      best_parent
    );

    // upper left
    this->circle_pack_split_region(
      vec2(lb.x, cp.y),
      vec2(cp.x, ub.y),
      new_depth,
      best_parent
    );
  }

  static int circle_distance_sort(const void *ap, const void *bp) {
    const vec4 *a = (vec4 *)ap;
    const vec4 *b = (vec4 *)bp;

    // sort by validity first
    if (glm::sign(a->w) != glm::sign(b->w)) {
      return a->w > 0.0f ? 1 : -1;
    }

    // order by distance ascending
    return a->z <= b->z ? -1 : 1;
  }

  void circle_pack_inner_sphere_tree(vec2 lb, vec2 ub) {

    // compute a list of pixels that are inside the shape
    vec4 *inside = nullptr;
    {
      vec4 p(0.0);
      for (p.x = lb.x; p.x < ub.x; p.x++) {
        for (p.y = lb.y; p.y < ub.y; p.y++) {
          // maximum radius (e.g, unsigned distance to the nearest surface)
          p.z = this->sample_local(p);
          p.w = this->sample_local(p);
          if (p.z <= 0.0f) {
            sb_push(inside, p);
          }
        }
      }
    }

    // sort the list by distance ascending
    uint32_t inside_count = sb_count(inside);
    {
      qsort(inside, inside_count, sizeof(vec4), Polygon::circle_distance_sort);
    }

    // place the first circle
    sb_push(this->circles, vec4(inside[0].x, inside[0].y, inside[0].z, inside[0].z));

    // loop through the rest of the circles and add them to the list if
    // the do not intersect any previously placed circle
    int sentinel = 100;
    while (inside_count && sentinel--) {
      for (uint32_t i=1; i<inside_count; i++) {
        vec4 circle = inside[i];
        float radius = abs(circle.z);
        // test the current circle against all previous circles
        uint32_t circle_count = sb_count(this->circles);
        bool skip = false;
        for (uint32_t cidx=0; cidx<circle_count; cidx++) {
          vec4 existing_circle = this->circles[cidx];
          float d = distance(vec2(circle), vec2(existing_circle));
          if (d < abs(existing_circle.z) + radius) {
            if (d < abs(existing_circle.z)) {
              inside[i].w = 1.0f;
            } else {
              inside[i].z = d - (abs(existing_circle.z) + radius);
            }

            if (inside[i].z > -0.001f) {
              inside[i].w = 1.0f;
            }

            // if (abs(inside[i].z) < 0.001f) {
            //   inside[i].w = 1.0f;
            // }

            skip = true;
            break;
          }
        }

        if (!skip) {
          inside[i].w = 1.0f;
          sb_push(this->circles, vec4(
            circle.x, // center x
            circle.y, // center y
            radius,   // reduced packing radius
            circle.w  // actual signed-distance
          ));
        }
      }

      // sort the list to bring pending entries back to the top and then
      // resize the array
      {
        qsort(inside, inside_count, sizeof(vec4), Polygon::circle_distance_sort);
        uint32_t i=0;
        for (i=0; i<inside_count; i++) {
          if (inside[i].w == 0.0f) {
            break;
          }
        }
        stb__sbn(inside) = i;
        inside_count = i;
      }
    }
  }

  void circle_pack_uniform(vec2 lb, vec2 ub) {
    vec2 p;
    float radius = 5.0f;
    for (p.x=lb.x-radius; p.x<=ub.x+radius*2.0f; p.x+=radius*2.0f) {
      for (p.y=lb.y-radius; p.y<=ub.y+radius*2.0f; p.y+=radius*2.0f) {
        float dist = this->sample_local(p);
        if (dist <= -radius) {
          if(dist + radius >= -radius * 3.0) {
            sb_push(this->circles, vec4(
              p.x,
              p.y,
              radius,
              0
            ));

            // this->circle_pack_split_region(
            //   p - vec2(radius),
            //   p + vec2(radius)
            // );
          } else {
            sb_push(this->circles, vec4(
              p.x,
              p.y,
              radius,
              0
            ));
          }
        } else if (dist + radius < 0.0f) {
          sb_push(this->circles, vec4(
            p.x,
            p.y,
            radius,
            0
          ));
        }
      }
    }
  }

  void circle_pack() {
    sb_reset(this->circles);
    printf("circle_pack (%f, %f) -> (%f, %f)\n",
      this->aabb.lb.x,
      this->aabb.lb.y,
      this->aabb.ub.x,
      this->aabb.ub.y
    );

    vec2 diff = this->aabb.ub - this->aabb.lb;
    float m = glm::max(diff.x, diff.y);

    uint32_t approach = 2;

    switch(approach) {
      // naive uniform grid
      case 0: this->circle_pack_uniform(vec2(0.0), diff); break;
      // naive quadtree that is broken
      case 1: this->circle_pack_split_region(vec2(0.0), diff); break;
      // adaption of the approach described in "Inner Sphere Trees for Proximity and Penetration Queries"
      case 2: this->circle_pack_inner_sphere_tree(vec2(0.0), diff); break;
    }
  }

  void append(vec2 point) {
    this->dirty = true;
    this->aabb.grow(point);
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

      if (0) {
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
      }


      // draw the packed circles
      {
        uint32_t circle_count = sb_count(this->circles);
        igText("circle count: %u", circle_count);
        for (uint32_t i=0; i<circle_count; i++) {
          vec4 circle = this->circles[i];
          rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0x99, 0x99, 0x99));
          rawkit_vg_fill_color(vg, rawkit_vg_HSL(circle.z / 400.0f, 0.6f, 0.5f));
          rawkit_vg_begin_path(vg);
#if 1
            rawkit_vg_arc(
              vg,
              circle.x + this->aabb.lb.x,
              circle.y + this->aabb.lb.y,
              circle.z, // radius
              0.0,
              6.283185307179586,
              1
            );
#else
          rawkit_vg_rect(
            vg,
            circle.x + this->aabb.lb.x,
            circle.y + this->aabb.lb.y,
            circle.z,
            circle.z
          );
#endif
          rawkit_vg_stroke(vg);
        }
      }
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
