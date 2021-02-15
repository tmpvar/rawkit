#pragma once

#include <glm/glm.hpp>
using namespace glm;

#include "quat.h"

enum JointType {
  SPHERICAL,
  HINGE,
  FIXED
};


void applyBodyPairCorrection(
  Body *body0,
  Body *body1,
  vec3 corr,
  float compliance,
  float dt,
  const vec3 *pos0 = nullptr,
  const vec3 *pos1 = nullptr,
  bool velocityLevel = false
) {
  float C = corr.length();
  if ( C == 0.0f) {
    return;
  }

  vec3 normal = normalize(corr);

  float w0 = body0
    ? body0->getInverseMass(normal, pos0)
    : 0.0f;


  float w1 = body1
    ? body1->getInverseMass(normal, pos1)
    : 0.0f;

  float w = w0 + w1;
  if (isnan(w) || w == 0.0f) {
    return;
  }

  float den = (w + compliance / dt / dt);
  if (isnan(den) || den == 0.0f) {
    return;
  }

  float lambda = -C / den;
  normal *= -lambda;
  if (body0) {
    body0->applyCorrection(normal, pos0, velocityLevel);
  }

  if (body1) {
    normal *= -1.0f;
    body1->applyCorrection(normal, pos1, velocityLevel);
  }
}

void limitAngle(
  Body *body0,
  Body *body1,
  vec3 n,
  vec3 a,
  vec3 b,
  float minAngle,
  float maxAngle,
  float compliance,
  float dt,
  float maxCorr = glm::pi<float>()
) {
    // the key function to handle all angular joint limits
    vec3 c = cross(a, b);
    float PI = glm::pi<float>();
    float phi = glm::asin(dot(c, n));
    if (dot(a, b) < 0.0) {
      phi = PI - phi;
    }

    if (phi > PI) {
      phi -= 2.0 * PI;
    }

    if (phi < -PI) {
      phi += 2.0 * PI;
    }

    if (phi < minAngle || phi > maxAngle) {
      phi = glm::min(glm::max(minAngle, phi), maxAngle);
      quat q = glm::angleAxis(phi, n);
      vec3 omega = cross(rotate(q, a), b);

      phi = length(omega);
      if (phi > maxCorr) {
        omega *= maxCorr / phi;
      }

      applyBodyPairCorrection(body0, body1, omega, compliance, dt);
    }
}


struct Joint {
  Body *body0;
  Body *body1;

  Pose localPose0;
  Pose localPose1;
  Pose globalPose0;
  Pose globalPose1;

  JointType type;

  float compliance = 0.0;
  float rotDamping = 0.0;
  float posDamping = 0.0;

  // swing limits
  bool hasSwingLimits = false;
  float minSwingAngle = -2.0 * glm::pi<float>();
  float maxSwingAngle = 2.0 * glm::pi<float>();
  float swingLimitsCompliance = 0.0;

  // twist limits
  bool hasTwistLimits = false;
  float minTwistAngle = -2.0 * glm::pi<float>();
  float maxTwistAngle = 2.0 * glm::pi<float>();
  float twistLimitCompliance = 0.0;

  Joint(JointType type, Body *body0, Body *body1, Pose *localPose0, Pose *localPose1) {
    this->body0 = body0;
    this->body1 = body1;
    this->localPose0 = localPose0->clone();
    this->localPose1 = localPose1->clone();
    this->globalPose0 = localPose0->clone();
    this->globalPose1 = localPose1->clone();
    this->type = type;
  }

  void updateGlobalPoses() {
    this->globalPose0.copy(&this->localPose0);
    if (this->body0) {
      this->body0->pose.transformPose(&this->globalPose0);
    }

    this->globalPose1.copy(&this->localPose1);
    if (this->body1) {
      this->body1->pose.transformPose(&this->globalPose1);
    }
  }

  void solvePos(float dt) {
    this->updateGlobalPoses();
    // orientation
    // NOTE(tmpvar): I think this is broken in the initial impl
    if (this->type == JointType::FIXED) {
      quat q = this->globalPose1.q * conjugate(this->globalPose0.q);
      vec3 omega(2.0 * q.x, 2.0 * q.y, 2.0 * q.z);

      if (q.w < 0.0f) {
        omega *= -1.0f;
      }
      applyBodyPairCorrection(body0, body1, omega, this->compliance, dt);
    }

    if (this->type == JointType::HINGE) {
      // align axes
      vec3 a0 = getQuatAxis0(this->globalPose0.q);
      vec3 a1 = getQuatAxis0(this->globalPose1.q);
      applyBodyPairCorrection(this->body0, this->body1, cross(a0, a1), 0.0, dt);

      // limits
      if (this->hasSwingLimits) {
        this->updateGlobalPoses();
        vec3 n = getQuatAxis0(this->globalPose0.q);
        vec3 b0 = getQuatAxis1(this->globalPose0.q);
        vec3 b1 = getQuatAxis1(this->globalPose1.q);
        limitAngle(
          this->body0,
          this->body1,
          n,
          b0,
          b1,
          this->minSwingAngle,
          this->maxSwingAngle,
          this->swingLimitsCompliance,
          dt
        );
      }
    }

    if (this->type == JointType::SPHERICAL) {
      // swing limits
      if (this->hasSwingLimits) {
        this->updateGlobalPoses();
        vec3 a0 = getQuatAxis0(this->globalPose0.q);
        vec3 a1 = getQuatAxis0(this->globalPose1.q);
        limitAngle(
          this->body0,
          this->body1,
          normalize(cross(a0, a1)),
          a0,
          a1,
          this->minSwingAngle,
          this->maxSwingAngle,
          this->swingLimitsCompliance,
          dt
        );
      }

      // twist limits
      if (this->hasTwistLimits) {
          this->updateGlobalPoses();
          vec3 n0 = getQuatAxis0(this->globalPose0.q);
          vec3 n1 = getQuatAxis0(this->globalPose1.q);
          vec3 n = normalize(n0 + n1);
          vec3 a0 = normalize(
            getQuatAxis1(this->globalPose0.q) + n * -dot(n, a0)
          );
          vec3 a1 = normalize(
            getQuatAxis1(this->globalPose1.q) + n * -dot(n, a1)
          );

          // handling gimbal lock problem
          float maxCorr = dot(n0, n1) > -0.5
            ? 2.0 * glm::pi<float>()
            : 1.0 * dt;

          limitAngle(
            this->body0,
            this->body1,
            n,
            a0,
            a1,
            this->minTwistAngle,
            this->maxTwistAngle,
            this->twistLimitCompliance,
            dt,
            maxCorr
          );
      }
    }

    // position
    // simple attachment
    this->updateGlobalPoses();
    vec3 corr = this->globalPose1.p - this->globalPose0.p;

    applyBodyPairCorrection(
      this->body0,
      this->body1,
      corr,
      this->compliance,
      dt,
      &this->globalPose0.p,
      &this->globalPose1.p
    );
  }

  void solveVel(float dt) {
    // Gauss-Seidel lets us make damping unconditionally stable in a
    // very simple way. We clamp the correction for each constraint
    // to the magnitude of the currect velocity making sure that
    // we never subtract more than there actually is.

    if (this->rotDamping > 0.0f) {
      vec3 omega(0.0);
      if (this->body0) {
        omega -= this->body0->omega;
      }
      if (this->body1) {
        omega += this->body1->omega;
      }

      omega *= glm::min(1.0f, this->rotDamping * dt);
      applyBodyPairCorrection(
        this->body0,
        this->body1,
        omega,
        0.0f,
        dt,
        nullptr,
        nullptr,
        true
      );
    }

    if (this->posDamping > 0.0) {
      this->updateGlobalPoses();
      vec3 vel(0.0);
      if (this->body0) {
        vel -= this->body0->getVelocityAt(this->globalPose0.p);
      }

      if (this->body1) {
        vel += this->body1->getVelocityAt(this->globalPose1.p);
      }

      vel *= glm::min(1.0f, this->posDamping * dt);
      applyBodyPairCorrection(
        this->body0,
        this->body1,
        vel,
        0.0,
        dt,
        &this->globalPose0.p,
        &this->globalPose1.p,
        true
      );
    }
  }
};