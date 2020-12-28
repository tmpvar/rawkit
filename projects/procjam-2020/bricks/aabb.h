#ifdef CPU_HOST
  #pragma once
  #define GLM_FORCE_RADIANS
  #include "../glm/glm.hpp"
  #include "../glm/gtc/matrix_transform.hpp"
  #include "../glm/gtx/compatibility.hpp"

  #include "../glm/glm.hpp"
  using namespace glm;
#endif

struct AABB {
  vec3 lb;
  vec3 ub;
};

static AABB aabb_union(AABB a, AABB b) {
  AABB ret;
  ret.lb = min(a.lb, b.lb);
  ret.ub = max(a.ub, b.ub);
  return ret;
}

static AABB aabb_add(AABB a, vec3 p, float radius) {
  AABB ret;
  ret.lb = min(a.lb, p - radius);
  ret.ub = max(a.ub, p + radius);
  return ret;
}

static bool aabb_contains(AABB a, vec3 p, float radius) {
  return (all(lessThanEqual(a.lb, p + radius)) && all(greaterThanEqual(a.ub, p - radius)));
}