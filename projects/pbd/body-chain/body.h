#pragma once

#include "pose.h"

struct Body {
  Pose pose;
  Pose prevPose;
  Pose origPose;
  vec3 size;

  vec3 vel = vec3(0.0f);
  vec3 omega = vec3(0.0f);

  float invMass = 1.0f;
  vec3 invInertia = vec3(1.0f);

  Body(Pose pose) {
    this->pose = pose.clone();
    this->prevPose = pose.clone();
    this->origPose = pose.clone();
    this->vel = vec3(0.0, 0.0, 0.0);
    this->omega = vec3(0.0, 0.0, 0.0);
    this->invMass = 1.0;
    this->invInertia = vec3(1.0, 1.0, 1.0);
    this->size = vec3(0.0);
  }

  void setBox(vec3 size, float density = 1.0f) {
    this->size = size;
    float mass = size.x * size.y * size.z * density;
    this->invMass = 1.0 / mass;
    mass /= 12.0;
    this->invInertia = vec3(
      1.0 / (size.y * size.y + size.z * size.z) / mass,
      1.0 / (size.z * size.z + size.x * size.x) / mass,
      1.0 / (size.x * size.x + size.y * size.y) / mass
    );
  }

  void applyRotation(vec3 rot, float scale = 1.0f, float maxRotationPerSubstep = 0.5f) {
    // safety clamping. This happens very rarely if the solver
    // wants to turn the body by more than 30 degrees in the
    // orders of milliseconds
    float maxPhi = 0.5f;
    float phi = length(rot);
    if (phi * scale > maxRotationPerSubstep) {
      scale = maxRotationPerSubstep / phi;
    }

    quat dq =  this->pose.q * quat(rot.x * scale, rot.y * scale, rot.z * scale, 0.0);
    this->pose.q = normalize(this->pose.q + dq * 0.5f);
  }

  void integrate(float dt, vec3 gravity, float maxRotationPerSubstep = 0.5f) {
    this->prevPose.copy(&this->pose);
    this->vel += gravity * dt;
    this->pose.p += this->vel * dt;
    this->applyRotation(this->omega, dt, maxRotationPerSubstep);
  }

  void update(float dt) {
    this->vel = (this->pose.p - this->prevPose.p) / dt;
    quat dq = this->pose.q * conjugate(this->prevPose.q);
    this->omega = vec3(
      dq.x * 2.0f / dt,
      dq.y * 2.0f / dt,
      dq.z * 2.0f / dt
    );

    if (dq.w < 0.0) {
      this->omega = -this->omega;
    }

    // NOTE(tmpvar): commented out in the original impl
    // this->omega *= (1.0f - 1.0f * dt);
    // this->vel *= (1.0f - 1.0f * dt);

    // this->mesh.position.copy(this->pose.p);
    // this->mesh.quaternion.copy(this->pose.q);
  }

  vec3 getVelocityAt(const vec3 &pos) {
    return this->vel - cross(
      pos - this->pose.p,
      this->omega
    );
  }

  float getInverseMass(const vec3 &normal, const vec3 *pos = nullptr) {
    vec3 n = pos == nullptr
      ? normal
      : cross((*pos) - this->pose.p, normal);

    n = this->pose.invRotate(n);
    float w = (
      n.x * n.x * this->invInertia.x +
      n.y * n.y * this->invInertia.y +
      n.z * n.z * this->invInertia.z
    );

    if (pos != nullptr) {
      w += this->invMass;
    }
    return w;
  }

  void applyCorrection(const vec3 &corr, const vec3 *pos = nullptr, bool velocityLevel = false) {
    vec3 dq;

    if (pos == nullptr) {
      dq = corr;
    } else {
      if (velocityLevel) {
        this->vel += corr * this->invMass;
      } else {
        this->pose.p += corr * this->invMass;
      }
      dq = cross((*pos) - this->pose.p, corr);
    }

    dq = this->pose.invRotate(dq);
    dq *= this->invInertia;

    dq = this->pose.rotate(dq);
    if (velocityLevel) {
      this->omega += dq;
    } else {
      this->applyRotation(dq);
    }
  }
};