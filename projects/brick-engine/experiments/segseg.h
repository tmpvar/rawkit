/*  Ported from Mukesh Prasad's public domain code:
 *    http://www.realtimerendering.com/resources/GraphicsGems/gemsii/xlines.c
 *
 *   This function computes whether two line segments,
 *   respectively joining the input points (x1,y1) -- (x2,y2)
 *   and the input points (x3,y3) -- (x4,y4) intersect.
 *   If the lines intersect, the return value is an array
 *   containing coordinates of the point of intersection.
 *
 *   Params
 *        p1, p2   Coordinates of endpoints of one segment.
 *        p3, p4   Coordinates of endpoints of other segment.
 *
 *   The value returned by the function is an enumeration of DONT_INTERSECT | DO_INTERSECT | COLINEAR
 */

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;

enum segseg_result {
  SEGSEG_DONT_INTERSECT = 0,
  SEGSEG_DO_INTERSECT = 1,
  SEGSEG_COLINEAR = 2
};

segseg_result segseg(vec2 p1, vec2 p2, vec2 p3, vec2 p4, vec2 *res) {
  float x1 = p1.x;
  float y1 = p1.y;
  float x2 = p2.x;
  float y2 = p2.y;
  float x3 = p3.x;
  float y3 = p3.y;
  float x4 = p4.x;
  float y4 = p4.y;

  float a1, a2, b1, b2, c1, c2; // Coefficients of line eqns.
  float r1, r2, r3, r4;         // 'Sign' values
  float denom, offset;          // Intermediate values
  float x, y;                   // Intermediate return values

  // Compute a1, b1, c1, where line joining points 1 and 2
  // is "a1 x  +  b1 y  +  c1  =  0".
  a1 = y2 - y1;
  b1 = x1 - x2;
  c1 = x2 * y1 - x1 * y2;

  // Compute r3 and r4.
  r3 = a1 * x3 + b1 * y3 + c1;
  r4 = a1 * x4 + b1 * y4 + c1;

  // Check signs of r3 and r4.  If both point 3 and point 4 lie on
  // same side of line 1, the line segments do not intersect.
  if ( r3 != 0.0f && r4 != 0.0f && ((r3 >= 0.0f && r4 >= 0.0f) || (r3 < 0.0f && r4 < 0.0f)))
    return SEGSEG_DONT_INTERSECT;

  // Compute a2, b2, c2
  a2 = y4 - y3;
  b2 = x3 - x4;
  c2 = x4 * y3 - x3 * y4;

  // Compute r1 and r2
  r1 = a2 * x1 + b2 * y1 + c2;
  r2 = a2 * x2 + b2 * y2 + c2;

  // Check signs of r1 and r2.  If both point 1 and point 2 lie
  // on same side of second line segment, the line segments do
  // not intersect.
  if (r1 != 0.0f && r2 != 0.0f && ((r1 >= 0.0f && r2 >= 0.0f) || (r1 < 0.0f && r2 < 0.0f)))
    return SEGSEG_DONT_INTERSECT;

  // Line segments intersect: compute intersection point.
  denom = a1 * b2 - a2 * b1;

  if (denom == 0.0f)
    return SEGSEG_COLINEAR;

  offset = denom < 0.0f ? - denom / 2.0f : denom / 2.0f;

  x = b1 * c2 - b2 * c1;
  y = a2 * c1 - a1 * c2;

  res->x = ( x < 0.0f ? x : x ) / denom;
  res->y = ( y < 0.0f ? y : y ) / denom;

  return SEGSEG_DO_INTERSECT;
}

vec2 segment_closest_point(vec2 a, vec2 b, vec2 p) {
  vec2 pa = p - a;
  vec2 ab = b - a;
  float t = glm::clamp(dot(pa, ab) / length2(ab), 0.0f, 1.0f);
  return a + ab * t;
};

vec2 line_closest_point(vec2 a, vec2 b, vec2 p) {
  vec2 pa = p - a;
  vec2 ab = b - a;
  float t = dot(pa, ab) / length2(ab);
  return a + ab * t;
};

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
