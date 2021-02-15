// adapted from  https://github.com/matthias-research/pages/blob/master/challenges/PBD.js (MIT license)

#include <rawkit/rawkit.h>
#include <stb_sb.h>
#define CPU
#include "body-chain/shared.h"
#include "body-chain/pose.h"
#include "body-chain/body.h"
#include "body-chain/joint.h"

#define DEFAULT_GRAVITY vec3(0.0, -9.8, 0.0)
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
  Scene scene;
  rawkit_gpu_ssbo_t *body_proxies;
  BodyProxy *body_proxies_buffer;

  u32 sim_frame;
};

void createObjects(State *state) {
  vec3 objectsSize(0.02, 0.04, 0.02);
  vec3 lastObjectsSize(0.2, 0.04, 0.2);
  u32 numObjects = BODY_COUNT;

	float rotDamping = 1000.0;
  float posDamping = 1000.0;

  vec3 pos(0.0, (float)(numObjects * objectsSize.y + lastObjectsSize.y) * 1.4 + 0.2, 0.0);
  Pose pose = {};
  Body *lastBody = nullptr;
  Pose jointPose0 = {};
  Pose jointPose1 = {};
  jointPose0.q = glm::angleAxis(0.5f * glm::pi<float>(), vec3(0.0, 0.0, 1.0));
  jointPose1.q = glm::angleAxis(0.5f * glm::pi<float>(), vec3(0.0, 0.0, 1.0));
  vec3 lastSize = objectsSize;

  for (u32 i = 0; i < numObjects; i++) {

    vec3 size = i < numObjects - 1 ? objectsSize : lastObjectsSize;
    pose.p = vec3(pos.x, pos.y - i * objectsSize.y, pos.z);
    Body *boxBody = new Body(pose);
    boxBody->setBox(size);
    sb_push(state->bodies, boxBody);

    float s = i % 2 == 0 ? -0.5 : 0.5;
    jointPose0.p = vec3(s * size.x, 0.5 * size.y, s * size.z);
    jointPose1.p = vec3(s * lastSize.x, -0.5 * lastSize.y, s * lastSize.z);

    if (!lastBody) {
      jointPose1.copy(&jointPose0);
      jointPose1.p += pose.p;
    }

    Joint *joint = new Joint(JointType::SPHERICAL, boxBody, lastBody, &jointPose0, &jointPose1);
    joint->rotDamping = rotDamping;
    joint->posDamping = posDamping;
    sb_push(state->joints, joint);

    lastBody = boxBody;
    lastSize = size;
  }
}

void setup() {
  State *state = rawkit_hot_state("state", State);

  if (state->last_time == 0.0f) {
    state->last_time = rawkit_now();
    state->gravity = DEFAULT_GRAVITY;
    state->substeps = DEFAULT_SUBSTEPS;

    // add bodies
    {
      createObjects(state);

      {
        u32 size = BODY_COUNT * sizeof(BodyProxy);
        state->body_proxies = rawkit_gpu_ssbo("bodies", size);
        state->body_proxies_buffer = (BodyProxy *)calloc(size, 1);
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

    // for (u32 j = 0; j < joint_count; j++) {
    //   joints[j]->solveVel(dt);
    // }
  }
}


void loop() {
  State *state = rawkit_hot_state("state", State);
  float now = rawkit_now();
  float dt = now - state->last_time;
  dt = 1.0f/60.0f;//60.0 / 1000.0;
  vec2 screen(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );
  state->last_time = now;
  igText("dt: %fms", dt);

  {
    u32 body_count = sb_count(state->bodies);
    for (u32 i=0; i<body_count; i++) {
      Body *body = state->bodies[i];
      igText("body %u", i);
      igText("  p(%f, %f, %f)", body->pose.p.x, body->pose.p.y, body->pose.p.z);
      igText("  q(%f, %f, %f, %f)", body->pose.q.x, body->pose.q.y, body->pose.q.z, body->pose.q.w);
      igText("  size(%f, %f, %f)", body->size.x, body->size.y, body->size.z);
    }
  }

  if (true || igIsMouseDown(ImGuiMouseButton_Left)) {
    simulate(
      state->bodies,
      state->joints,
      dt,
      state->substeps,
      state->gravity
    );
    state->sim_frame ++;
  }

  igText("sim frame: %u", state->sim_frame);

  {
    mat4 proj = glm::perspective(
      glm::radians(90.0f),
      screen.x/screen.y,
      0.1f,
      10000.0f
    );

    float dist = 5.0f;
    float now = (float)rawkit_now() * .5;
    vec3 eye = vec3(
      sin(now) * dist,
      0,
      cos(now) * dist
    );

    mat4 view = glm::lookAt(
      eye,
      vec3(0.0, eye.y, 0.0),
      vec3(0.0f, 1.0f, 0.0f)
    );

    state->scene.worldToScreen = proj * view;
    state->scene.time = (float)rawkit_now();
  }

  rawkit_shader_t *cuboid_shader = rawkit_shader(
    rawkit_file("body-chain/cuboid.vert"),
    rawkit_file("body-chain/cuboid.frag")
  );

  // render the world shader
  rawkit_shader_instance_t *inst = rawkit_shader_instance_begin(cuboid_shader);
  if (inst) {
    rawkit_shader_instance_param_ubo(inst, "UBO", &state->scene);
    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = screen.x,
      .height = screen.y,
      .minDepth = 0.0,
      .maxDepth = 1.0
    };

    vkCmdSetViewport(
      inst->command_buffer,
      0,
      1,
      &viewport
    );

    VkRect2D scissor = {};
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    vkCmdSetScissor(
      inst->command_buffer,
      0,
      1,
      &scissor
    );

    u32 body_count = sb_count(state->bodies);
    // update
    {
      for (u32 i=0; i<body_count; i++) {
        Body *body = state->bodies[i];
        BodyProxy *proxy = &state->body_proxies_buffer[i];
        proxy->pos = vec4(body->pose.p, 0.0);
        proxy->rot = vec4(
          body->pose.q.x,
          body->pose.q.y,
          body->pose.q.z,
          body->pose.q.w
        );
        proxy->size = vec4(body->size, 0.0);
      }

      rawkit_gpu_ssbo_update(
        state->body_proxies,
        rawkit_vulkan_queue(),
        rawkit_vulkan_command_pool(),
        (void *)state->body_proxies_buffer,
        sizeof(BodyProxy) * body_count
      );
      state->body_proxies->resource_version++;
    }

    rawkit_shader_instance_param_ssbo(inst, "Bodies", state->body_proxies);
    vkCmdDraw(inst->command_buffer, 36, body_count, 0, 0);
    rawkit_shader_instance_end(inst);
  }
}
