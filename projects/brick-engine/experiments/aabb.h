#pragma once

#include <glm/glm.hpp>
using namespace glm;



float static aabb_orientation(vec2 start, vec2 end, vec2 point) {
  return glm::sign(
    (end.y - start.y) *
    (point.x - end.x) -
    (end.x - start.x) *
    (point.y - end.y)
  );
}


struct AABB {
  vec2 lb = vec2(FLT_MAX);
  vec2 ub = vec2(-FLT_MAX);

  void grow(vec2 point) {
    this->lb = min(this->lb, point);
    this->ub = max(this->ub, point);
  }

  bool contains(vec2 p) {
    return all(greaterThanEqual(p, lb)) && all(lessThanEqual(p, ub));
  }

  vec2 nearest(vec2 p) {
    p = clamp(p, lb, ub);
    float dl = abs(p.x-lb.x);
    float dr = abs(p.x-ub.x);
    float dt = abs(p.y-lb.y);
    float db = abs(p.y-ub.y);
    float m = min(dl, min(dr, min(dt, db)));

    if (m == dt) {
      return vec2(p.x, lb.y);
    }
    if (m == db) {
      return vec2(p.x, ub.y);
    }
    if (m == dl) {
      return vec2(lb.x, p.y);
    }
    return vec2(ub.x, p.y);
  }

  float width() {
    return this->ub.x - this->lb.x;
  }

  float height() {
    return this->ub.y - this->lb.y;
  }

  AABB copy_inflated(float r) {
    return AABB{
      .lb = this->lb - r,
      .ub = this->ub + r
    };
  }

  bool isect_line(vec2 start, vec2 end) {
    float a = aabb_orientation(start, end, this->lb);
    float b = aabb_orientation(start, end, vec2(this->ub.x, this->lb.y));
    float c = aabb_orientation(start, end, vec2(this->ub.x, this->ub.y));
    float d = aabb_orientation(start, end, vec2(this->lb.x, this->ub.y));
    return (a != b || b != c || c != d);
  }
};
