
float min_component(vec3 a) {
  return min(a.x, min(a.y, a.z));
}

float max_component(vec3 a) {
  return max(a.x, max(a.y, a.z));
}

bool ray_aabb(vec3 p0, vec3 p1, vec3 rayOrigin, vec3 invRaydir) {
  vec3 t0 = (p0 - rayOrigin) * invRaydir;
  vec3 t1 = (p1 - rayOrigin) * invRaydir;
  vec3 tmin = min(t0, t1);
  vec3 tmax = max(t0, t1);
  return max_component(tmin) <= min_component(tmax);
}

float ray_aabb_time(vec3 boxMin, vec3 boxMax, vec3 rayOrigin, vec3 rayDir) {
  vec3 tmin = (boxMin - rayOrigin) / rayDir;
  vec3 tmax = (boxMax - rayOrigin) / rayDir;
  vec3 t1 = min(tmin, tmax);
  vec3 t2 = max(tmin, tmax);
  float tNear = max(max(t1.x, t1.y), t1.z);
  float tFar = min(min(t2.x, t2.y), t2.z);
  return max(tNear, tFar);
}


bool test_ray_aabb(vec3 boxCenter, vec3 boxRadius, vec3 rayOrigin, vec3 rayDirection, vec3 invRayDirection) {
  rayOrigin -= boxCenter;
  vec3 distanceToPlane = (-boxRadius * sign(rayDirection) - rayOrigin) * invRayDirection;

# define TEST(U,V,W) (float(distanceToPlane.U >= 0.0) * float(abs(rayOrigin.V + rayDirection.V * distanceToPlane.U) < boxRadius.V) * float(abs(rayOrigin.W + rayDirection.W * distanceToPlane.U) < boxRadius.W))

  // If the ray is in the box or there is a hit along any axis, then there is a hit
  return bool(float(abs(rayOrigin.x) < boxRadius.x) *
    float(abs(rayOrigin.y) < boxRadius.y) *
    float(abs(rayOrigin.z) < boxRadius.z) +
    TEST(x, y, z) +
    TEST(y, z, x) +
    TEST(z, x, y));
# undef TEST
}

// Returns true if R intersects the oriented box. If there is an intersection, sets distance and normal based on the
// hit point. If no intersection, then distance and normal are undefined.
int indexOfMaxComponent(vec3 v) { return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : (v.z > v.y) ? 2 : 1; }
float maxComponent(vec3 a) {
  return max(a.x, max(a.y, a.z));
}

float minComponent(vec3 a) {
  return min(a.x, min(a.y, a.z));
}


// Returns true if R intersects the oriented box. If there is an intersection, sets distance and normal based on the
// hit point. If no intersection, then distance and normal are undefined.
bool ailaWaldHitAABox(
  in const vec3 boxCenter,
  in const vec3 boxRadius,
  in const vec3 rayOrigin,
  in const vec3 rayDirection,
  in const vec3 invRayDirection,
  out float found_distance,
  out vec3 found_normal
) {
  vec3 origin = rayOrigin - boxCenter;

  vec3 t_min = (-boxRadius - origin) * invRayDirection;
  vec3 t_max = (boxRadius - origin) * invRayDirection;
  float t0 = maxComponent(min(t_min, t_max));
  float t1 = minComponent(max(t_min, t_max));

  // Compute the intersection distance
  found_distance = (t0 > 0.0) ? t0 : t1;

  vec3 V = boxCenter - (rayOrigin + rayDirection * found_distance) * (1.0 / boxRadius);
  vec3 mask = vec3(lessThan(vec3(lessThan(V.xyz, V.yzx)), V.zxy));// step(V.xyz, V.yzx) * step(V.xyz, V.zxy);
  found_normal = mask;// * (all(lessThan(abs(origin), boxRadius)) ? -1.0 : 1.0);


  return (t0 <= t1) && (found_distance > 0.0) && !isinf(found_distance);
}

// vec3 box.radius:       independent half-length along the X, Y, and Z axes
// mat3 box.rotation:     box-to-world rotation (orthonormal 3x3 matrix) transformation
// bool rayCanStartInBox: if true, assume the origin is never in a box. GLSL optimizes this at compile time
// bool oriented:         if false, ignore box.rotation
bool ourIntersectBoxCommon(
  vec3 boxCenter,
  vec3 boxRadius,
  vec3 rayOrigin,
  vec3 rayDirection,
  vec3 invRayDirection,
  out float distance,
  out vec3 normal
) {
    // Move to the box's reference frame. This is unavoidable and un-optimizable.
    rayOrigin = rayOrigin - boxCenter;

    // We'll use the negated sign of the ray direction in several places, so precompute it.
    // The sign() instruction is fast...but surprisingly not so fast that storing the result
    // temporarily isn't an advantage.
    vec3 sgn = -sign(rayDirection);

	// Ray-plane intersection. For each pair of planes, choose the one that is front-facing
    // to the ray and compute the distance to it.
    vec3 distanceToPlane = (boxRadius * sgn - rayOrigin) * invRayDirection;

    // Perform all three ray-box tests and cast to 0 or 1 on each axis.
    // Use a macro to eliminate the redundant code (no efficiency boost from doing so, of course!)
    // Could be written with
#   define TEST(U, VW) (distanceToPlane.U >= 0.0) && all(lessThan(abs(rayOrigin.VW + rayDirection.VW * distanceToPlane.U), boxRadius.VW))

    bvec3 test = bvec3(TEST(x, yz), TEST(y, zx), TEST(z, xy));

    // CMOV chain that guarantees exactly one element of sgn is preserved and that the value has the right sign
    sgn = test.x ? vec3(sgn.x, 0.0, 0.0) : (test.y ? vec3(0.0, sgn.y, 0.0) : vec3(0.0, 0.0, test.z ? sgn.z : 0.0));
#   undef TEST

    // At most one element of sgn is non-zero now. That element carries the negative sign of the
    // ray direction as well. Notice that we were able to drop storage of the test vector from registers,
    // because it will never be used again.

    // Mask the distance by the non-zero axis
    // Dot product is faster than this CMOV chain, but doesn't work when distanceToPlane contains nans or infs.
    //
    distance = (sgn.x != 0.0) ? distanceToPlane.x : ((sgn.y != 0.0) ? distanceToPlane.y : distanceToPlane.z);

    // Normal must face back along the ray. If you need
    // to know whether we're entering or leaving the box,
    // then just look at the value of winding. If you need
    // texture coordinates, then use box.invDirection * hitPoint.
    normal = sgn;

    return (sgn.x != 0) || (sgn.y != 0) || (sgn.z != 0);
}
//
// // vec3 box.radius:       independent half-length along the X, Y, and Z axes
// // mat3 box.rotation:     box-to-world rotation (orthonormal 3x3 matrix) transformation
// // bool rayCanStartInBox: if true, assume the origin is never in a box. GLSL optimizes this at compile time
// // bool oriented:         if false, ignore box.rotation
// bool ourIntersectBoxCommon(
//   vec3 boxCenter,
//   vec3 boxRadius,
//   vec3 rayOrigin,
//   vec3 rayDirection,
//   vec3 invRayDirection,
//   out float distance,
//   out vec3 normal
// ) {
//
//     // Move to the box's reference frame. This is unavoidable and un-optimizable.
//     vec3 origin = rayOrigin - boxCenter;
//
//     // This "rayCanStartInBox" branch is evaluated at compile time because `const` in GLSL
//     // means compile-time constant. The multiplication by 1.0 will likewise be compiled out
//     // when rayCanStartInBox = false.
//     float winding = 1.0;
//
//     // We'll use the negated sign of the ray direction in several places, so precompute it.
//     // The sign() instruction is fast...but surprisingly not so fast that storing the result
//     // temporarily isn't an advantage.
//     vec3 sgn = -sign(rayDirection);
//
// 	// Ray-plane intersection. For each pair of planes, choose the one that is front-facing
//     // to the ray and compute the distance to it.
//     vec3 distanceToPlane = boxRadius * winding * sgn - rayOrigin;
//     distanceToPlane *= invRayDirection;
//
//     // Perform all three ray-box tests and cast to 0 or 1 on each axis.
//     // Use a macro to eliminate the redundant code (no efficiency boost from doing so, of course!)
//     // Could be written with
//     #define TEST(U, VW) (distanceToPlane.U >= 0.0) && all(lessThan(abs(rayOrigin.VW + rayDirection.VW * distanceToPlane.U), boxRadius.VW))
//     bvec3 test = bvec3(TEST(x, yz), TEST(y, zx), TEST(z, xy));
//
//     // CMOV chain that guarantees exactly one element of sgn is preserved and that the value has the right sign
//     sgn = test.x ? vec3(sgn.x, 0.0, 0.0) : (test.y ? vec3(0.0, sgn.y, 0.0) : vec3(0.0, 0.0, test.z ? sgn.z : 0.0));
//     #undef TEST
//
//     // At most one element of sgn is non-zero now. That element carries the negative sign of the
//     // ray direction as well. Notice that we were able to drop storage of the test vector from registers,
//     // because it will never be used again.
//
//     // Mask the distance by the non-zero axis
//     // Dot product is faster than this CMOV chain, but doesn't work when distanceToPlane contains nans or infs.
//     //
//     distance = (sgn.x != 0.0) ? distanceToPlane.x : ((sgn.y != 0.0) ? distanceToPlane.y : distanceToPlane.z);
//
//     // Normal must face back along the ray. If you need
//     // to know whether we're entering or leaving the box,
//     // then just look at the value of winding. If you need
//     // texture coordinates, then use box.invDirection * hitPoint.
//
//     normal = sgn;
//
//     return (sgn.x != 0) || (sgn.y != 0) || (sgn.z != 0);
// }
