// adapted from  https://github.com/matthias-research/pages/blob/master/challenges/PBD.js (MIT license)

#include <rawkit/rawkit.h>
#include <stb_sb.h>

#include "body-chain/pose.h"
#include "body-chain/body.h"
#include "body-chain/joint.h"

#define DEFAULT_GRAVITY vec3(0.0, 0, 0.0)
#define DEFAULT_SUBSTEPS 10
#define BODY_COUNT 10
#define BODY_DIMS vec3(1.0)

struct State {
  float maxRotationPerSubstep = 0.5f;
  Body **bodies;
  Joint **joints;
  vec3 gravity;
  u32 substeps;
  float last_time;
};


void setup() {
  State *state = rawkit_hot_state("state", State);

  if (state->last_time == 0.0f) {
    state->last_time = rawkit_now();
    state->gravity = DEFAULT_GRAVITY;
    state->substeps = DEFAULT_SUBSTEPS;

    // add bodies
    {
      for (u32 i=0; i<BODY_COUNT; i++) {
        Body *body = new Body({
          .p = vec3(0.0, (float)i * BODY_DIMS.x * 1.4f + 0.2f, 0.0f)
        });
        body->setBox(vec3(1.0f));
        sb_push(state->bodies, body);

        if (i == 0) {
          continue;
        }

        Pose jointPose = {
          // set the connection point to the furthest extent, for now.
          .p = BODY_DIMS
        };

        Joint *joint = new Joint(
          JointType::SPHERICAL,
          state->bodies[i-1],
          state->bodies[i],
          &jointPose,
          &jointPose
        );

        sb_push(state->joints, joint);
      }

      // // fix the last one in the chain
      {
        Body *last = state->bodies[BODY_COUNT-1];

        Pose jointPoseA = {};
        Pose jointPoseB = {
          // set the connection point to the furthest extent, for now.
          .p = last->pose.p,
        };
        Joint *joint = new Joint(
          JointType::SPHERICAL,
          last,
          nullptr,
          &jointPoseA,
          &jointPoseB
        );

        sb_push(state->joints, joint);
      }

    }
  }
}



void simulate(Body **bodies, Joint **joints, float timeStep, u32 numSubsteps, vec3 gravity) {
  float dt = timeStep / numSubsteps;

  u32 body_count = sb_count(bodies);
  u32 joint_count = sb_count(joints);
  for (u32 i = 0; i < numSubsteps; i++) {
    for (u32 j = 0; j < body_count; j++) {
      bodies[j]->integrate(dt, gravity);
    }

    for (u32 j = 0; j < joint_count; j++) {
      joints[j]->solvePos(dt);
    }

    for (u32 j = 0; j < body_count; j++) {
      bodies[j]->update(dt);
    }

    for (u32 j = 0; j < joint_count; j++) {
      joints[j]->solveVel(dt);
    }
  }
}


void loop() {
  State *state = rawkit_hot_state("state", State);
  float now = rawkit_now();
  float dt = now - state->last_time;
  state->last_time = now;
  igText("dt: %fms", dt);

  {
    printf("\n-----\n");
    u32 body_count = sb_count(state->bodies);
    for (u32 i=0; i<body_count; i++) {
      Body *body = state->bodies[i];
      printf("p(%f, %f, %f)\n", body->pose.p.x, body->pose.p.y, body->pose.p.z);
    }
  }



  simulate(
    state->bodies,
    state->joints,
    60.0f / 1000.0f,
    state->substeps,
    state->gravity
  );



}
