#pragma once

#include "pose.h"

struct Body {
  Pose pose;
  Pose prevPose;
  Pose origPose;

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

    // this->mesh = mesh;
    // this->mesh.position.copy(this->pose.p);
    // this->mesh.quaternion.copy(this->pose.q);
    // mesh.userData.physicsBody = this;
  }

  void setBox(vec3 size, float density = 1.0f) {
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

    quat dq(rot.x * scale, rot.y * scale, rot.z * scale, 0.0);
    dq *= this->pose.q;

    this->pose.q = quat(
      this->pose.q.x + 0.5 * dq.x,
      this->pose.q.y + 0.5 * dq.y,
      this->pose.q.z + 0.5 * dq.z,
      this->pose.q.w + 0.5 * dq.w
    );
    this->pose.q = normalize(this->pose.q);
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
      this->omega = vec3(-this->omega.x, -this->omega.y, -this->omega.z);
    }

    // NOTE(tmpvar): commented out in the original impl
    // this->omega.multiplyScalar(1.0 - 1.0 * dt);
    // this->vel.multiplyScalar(1.0 - 1.0 * dt);

    // this->mesh.position.copy(this->pose.p);
    // this->mesh.quaternion.copy(this->pose.q);
  }

  vec3 getVelocityAt(vec3 pos) {
    return this->vel - cross(
      pos - this->pose.p,
      this->omega
    );
  }

  float getInverseMass(vec3 normal) {
    vec3 n = this->pose.invRotate(normal);
    return (
      n.x * n.x * this->invInertia.x +
      n.y * n.y * this->invInertia.y +
      n.z * n.z * this->invInertia.z
    );
  }

  float getInverseMass(vec3 normal, vec3 pos) {
    vec3 n = cross(pos - this->pose.p, normal);
    return this->getInverseMass(n) + this->invMass;
  }

  void applyCorrection(vec3 corr, bool velocityLevel = false) {
    vec3 dq = this->pose.invRotate(corr);
    dq *= this->invInertia;
    dq = this->pose.rotate(dq);
    if (velocityLevel) {
      this->omega += dq;
    } else {
      this->applyRotation(dq);
    }
  }

  void applyCorrection(vec3 corr, vec3 pos, bool velocityLevel = false) {
    vec3 dq = corr;
    if (velocityLevel) {
      this->vel += corr * this->invMass;
    } else {
      this->pose.p += corr * this->invMass;
    }

    dq = cross(pos - this->pose.p, corr);

    this->applyCorrection(dq);
  }
};