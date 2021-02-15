#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

#include "quat.h"

vec3 applyQuaternion(vec3 v, quat q) {
  // calculate quat * vector
  float ix =  q.w * v.x + q.y * v.z - q.z * v.y;
  float iy =  q.w * v.y + q.z * v.x - q.x * v.z;
  float iz =  q.w * v.z + q.x * v.y - q.y * v.x;
  float iw = -q.x * v.x - q.y * v.y - q.z * v.z;

  // calculate result * inverse quat
  return vec3(
    v.x = ix * q.w + iw * - q.x + iy * - q.z - iz * - q.y,
    v.y = iy * q.w + iw * - q.y + iz * - q.x - ix * - q.z,
    v.z = iz * q.w + iw * - q.z + ix * - q.y - iy * - q.x
  );
}

struct Pose {
  vec3 p = vec3(0.0);
  quat q = quat(0.0, 0.0, 0.0, 1.0);

  void copy(Pose *p) {
    this->q = p->q;
    this->p = p->p;
  }

  Pose clone() {
    Pose c;
    c.copy(this);
    return c;
  }

  vec3 rotate(const vec3 &v) {
    return applyQuaternion(v, this->q);
  }

  vec3 invRotate(const vec3 &v) {
    quat q = glm::conjugate(this->q);
    return applyQuaternion(v, q);
  }

  void transformPose(Pose *pose) {
    pose->q = this->q * pose->q;
    pose->p = this->rotate(pose->p) + this->p;
  }
};