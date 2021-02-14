#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

#include "quat.h"

struct Pose {
  vec3 p = vec3(0.0);
  quat q = quat(0.0, 0.0, 0.0, 1.0);

  void copy(Pose *p) {
    this->q = p->q;
    this->p = p->p;
  }

  Pose clone() {
    return *this;
  }

  vec3 rotate(const vec3 &v) {
    return glm::rotate(this->q, v);
  }

  vec3 invRotate(const vec3 &v) {
    quat q = glm::conjugate(this->q);
    return glm::rotate(q, v);
  }

  vec3 transform(const vec3 &v) {
    return glm::rotate(this->q, v) + this->p;
  }

  vec3 invTransform(const vec3 &v) {
    return this->invRotate(v - this->p);
  }

  void transformPose(Pose *pose) {
    pose->q = this->q * pose->q;
    this->rotate(pose->p);
    pose->p += this->p;
  }
};