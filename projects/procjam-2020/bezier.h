#include "bricks/shared.h"
#include "bricks/aabb.h"

static vec3 bezier(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float t) {
  vec3 a = mix(p1, p2, t);
  vec3 b = mix(p2, p3, t);
  vec3 c = mix(p3, p4, t);

  return mix(
    mix(a, b, t),
    mix(b, c, t),
    t
  );
}


// //---------------------------------------------------------------------------------------
// // bounding box for a bezier (http://iquilezles.org/www/articles/bezierbbox/bezierbbox.htm)
// //---------------------------------------------------------------------------------------
AABB bezier_aabb(vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
  // extremes
  vec3 mi = min(p0,p3);
  vec3 ma = max(p0,p3);

  // note pascal triangle coefficnets
  vec3 c = -1.0f*p0 + 1.0f*p1;
  vec3 b =  1.0f*p0 - 2.0f*p1 + 1.0f*p2;
  vec3 a = -1.0f*p0 + 3.0f*p1 - 3.0f*p2 + 1.0f*p3;

  vec3 h = b*b - a*c;

  // real solutions
  if( any(greaterThan(h,vec3(0.0)))) {
    vec3 g = sqrt(abs(h));
    vec3 t1 = clamp((-b - g)/a, 0.0f, 1.0f); vec3 s1 = 1.0f-t1;
    vec3 t2 = clamp((-b + g)/a, 0.0f, 1.0f); vec3 s2 = 1.0f-t2;
    vec3 q1 = s1*s1*s1*p0 + 3.0f*s1*s1*t1*p1 + 3.0f*s1*t1*t1*p2 + t1*t1*t1*p3;
    vec3 q2 = s2*s2*s2*p0 + 3.0f*s2*s2*t2*p1 + 3.0f*s2*t2*t2*p2 + t2*t2*t2*p3;

    if( h.x > 0.0f ) {
      mi.x = min(mi.x,min(q1.x,q2.x));
      ma.x = max(ma.x,max(q1.x,q2.x));
    }

    if( h.y > 0.0f ) {
      mi.y = min(mi.y,min(q1.y,q2.y));
      ma.y = max(ma.y,max(q1.y,q2.y));
    }
    if( h.z > 0.0f  ) {
      mi.z = min(mi.z,min(q1.z,q2.z));
      ma.z = max(ma.z,max(q1.z,q2.z));
    }
  }

  AABB ret;
  ret.lb = mi;
  ret.ub = ma;
  return ret;
}

