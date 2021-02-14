#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

vec3 getQuatAxis0(quat q) {
	float x2 = q.x * 2.0;
  float w2 = q.w * 2.0;
  return vec3(
    ( q.w * w2) - 1.0 + q.x * x2,
    ( q.z * w2) + q.y * x2,
    (-q.y * w2) + q.z * x2
  );
}

vec3 getQuatAxis1(quat q) {
	float y2 = q.y * 2.0;
  float w2 = q.w * 2.0;
  return vec3(
    (-q.z * w2) + q.x * y2,
    ( q.w * w2) - 1.0 + q.y * y2,
    ( q.x * w2) + q.z * y2
  );
}

vec3 getQuatAxis2(quat q) {
  float z2 = q.z * 2.0;
	float w2 = q.w * 2.0;
	return vec3(
    ( q.y * w2) + q.x * z2,
    (-q.x * w2) + q.y * z2,
    ( q.w * w2) - 1.0 + q.z * z2
  );
}
