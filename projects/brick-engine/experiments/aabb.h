#pragma once

#include <glm/glm.hpp>
using namespace glm;


struct AABB {
  vec2 lb;
  vec2 ub;

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
};
