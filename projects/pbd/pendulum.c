// ported from https://github.com/matthias-research/pages/blob/master/challenges/pendulum.html

#include <rawkit/rawkit.h>
#include <cglm/vec2.h>
#include <math.h>
#include "stb_sb.h"


typedef struct point_t {
  float mass;
  float invMass;
  float radius;
  float size;
  vec2 pos;
  vec2 prev;
  vec2 vel;
  float compliance;
  bool unilateral;
  float force;
  float elongation;
} point_t;

typedef struct tail_t {
  vec2 pos;
} tail_t;

typedef struct state_t {
  uint32_t numSubsteps;

  float defaultRadius;
  float defaultMass;

  float gravity;
  double lastTime;
  float edgeDampingCoeff;
  float globalDampingCoeff;

  bool conserveEnergy;
  bool collisionHandling;
  bool showTrail;
  bool showForces;

  float mouseCompliance;
  float mouseDampingCoeff;

  float simWidth;
  float drawScale;

  bool paused;

  uint32_t maxPoints;
  uint32_t numPoints;
  float pointSize;
  point_t *points;

  tail_t *trail;
  uint32_t trailLast;
  uint32_t maxTrailLen;
  float trailDist;
} state_t;

state_t *state = NULL;

vec2 canvasOrig;

void trailAdd(vec2 p) {
  uint32_t trail_length = sb_count(state->trail);
  if (trail_length == 0) {
    tail_t t = {};
    glm_vec2_copy(p, t.pos);
    sb_push(state->trail, t);
    return;
  }

  float d2 = glm_vec2_distance2(state->trail[state->trailLast].pos, p);
  if (d2 > state->trailDist * state->trailDist) {
    state->trailLast = (state->trailLast + 1) % state->maxTrailLen;
    if (trail_length < state->maxTrailLen) {
      tail_t t = {};
      glm_vec2_copy(p, t.pos);
      sb_push(state->trail, t);
    } else {
      glm_vec2_copy(p, (&state->trail[state->trailLast])->pos);
    }
  }
}


void resetPos(bool equilibrium) {
  vec2 pos;
  pos[0] = equilibrium ? 0 : state->points[1].radius;
  pos[1] = equilibrium ? 0 : - state->points[1].radius;

  for (uint32_t i = 1; i < state->numPoints; i++) {
    point_t *p = &state->points[i];
    p->size = sqrt(1.0 / p->invMass);
    pos[1] = equilibrium ? pos[1] - p->radius : pos[1] + p->radius;
    glm_vec2_copy(pos, p->pos);
    glm_vec2_copy(pos, p->prev);

    p->vel[0] = 0;
    p->vel[1] = 0;
  }
  sb_free(state->trail);
  state->trail = NULL;
  state->trailLast = 0;
}

float computeEnergy() {
  float e = 0.0f;
  for (uint32_t i = 1; i < state->numPoints; i++) {
    point_t *p = &state->points[i];
    e += p->pos[1] / p->invMass * state->gravity + 0.5 / p->invMass * glm_vec2_distance2(GLM_VEC2_ZERO, p->vel);
  }
  return e;
}

void forceEnergyConservation(float prevE) {
  float dE = (computeEnergy() - prevE) / (state->numPoints - 1);
  if (dE < 0) {
    float postE = computeEnergy();

    for (uint32_t i = 1; i < state->numPoints; i++) {
      point_t *p = &state->points[i];
      float Ek = 0.5 / p->invMass * glm_vec2_distance2(GLM_VEC2_ZERO, p->vel);
      float s = sqrt((Ek - dE) / Ek);
      glm_vec2_scale(p->vel, s, p->vel);
    }
  }
}

void draw() {
  rawkit_vg_t *vg = rawkit_default_vg();
  rawkit_vg_stroke_width(vg, 1.0f);
  float x = canvasOrig[0];
  float y = canvasOrig[1];

  for (uint32_t i = 1; i < state->numPoints; i++) {
    float avgX = x;
    float avgY = y;
    point_t *p = &state->points[i];
    if (p->compliance > 0.0f) {
      rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0, 0, 0xFF));
    } else if (p->unilateral) {
      rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0, 0xFF, 0));
    } else {
      rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0x00, 0x0));
    }

    rawkit_vg_begin_path(vg);
    rawkit_vg_move_to(vg, x, y);
    x = canvasOrig[0] + p->pos[0] * state->drawScale;
    y = canvasOrig[1] - p->pos[1] * state->drawScale;
    rawkit_vg_line_to(vg, x, y);
    rawkit_vg_stroke(vg);
    avgX = (avgX + x) / 2.0f; avgY = (avgY + y) / 2.0f;

    // if (showForces)
    //   c.fillText("  f=" + p.force.toFixed(0) + "N, dx=" + p.elongation.toFixed(4) + "m", avgX, avgY);
    // }
  }
  rawkit_vg_stroke_width(vg, 1.0f);

  //   if (grabPointNr > 0) {
  //     c.strokeStyle = "#FF8000";
  //     c.beginPath();
  //     c.moveTo(canvasOrig.x + grabPoint.pos.x * state->drawScale, canvasOrig.y - grabPoint.pos.y * state->drawScale);
  //     c.lineTo(canvasOrig.x + state->points[grabPointNr].pos.x * state->drawScale, canvasOrig.y - state->points[grabPointNr].pos.y * state->drawScale);
  //     c.stroke();
  //   }

  rawkit_vg_fill_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
  for (uint32_t i = 1; i < state->numPoints; i++) {
    point_t *p = &state->points[i];
    igText("p%u: vel(%.2f, %.2f) pos(%.2f, %.2f)", i, p->unilateral, p->vel[0], p->vel[1]);
    x = canvasOrig[0] + p->pos[0] * state->drawScale;
    y = canvasOrig[1] - p->pos[1] * state->drawScale;
    rawkit_vg_begin_path(vg);
    rawkit_vg_arc(vg, x, y, state->pointSize * p->size, 0, M_PI*2.0f, 1);
    rawkit_vg_close_path(vg);
    rawkit_vg_fill(vg);
  }

  uint32_t trail_length = sb_count(state->trail);
  if (state->showTrail && trail_length > 1) {
    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(52, 235, 137));
    rawkit_vg_begin_path(vg);
    uint32_t pos = (state->trailLast + 1) % trail_length;
    rawkit_vg_move_to(vg,
      canvasOrig[0] + state->trail[pos].pos[0] * state->drawScale,
      canvasOrig[1] - state->trail[pos].pos[1] * state->drawScale
    );

    for (uint32_t i = 1; i < trail_length - 1; i++) {
      pos = (pos + 1) % trail_length;
      rawkit_vg_line_to(vg,
        canvasOrig[0] + state->trail[pos].pos[0] * state->drawScale,
        canvasOrig[1] - state->trail[pos].pos[1] * state->drawScale
      );
    }
    rawkit_vg_stroke(vg);
    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
  }
}

void solveDistPos(point_t *p0, point_t *p1, float d0, float compliance, bool unilateral, float dt) {
  float w = p0->invMass + p1->invMass;
  if (w == 0) {
    return;
  }
  vec2 grad;
  glm_vec2_sub(p1->pos, p0->pos, grad);
  float d = glm_vec2_distance(GLM_VEC2_ZERO, grad);
  grad[0] /= d;
  grad[1] /= d;


  w += compliance / dt / dt;
  float lambda = (d - d0) / w;

  if (lambda < 0 && unilateral) {
    return;
  }

  p1->force = lambda / dt / dt;
  p1->elongation = d - d0;
  glm_vec2_muladds(grad,  p0->invMass * lambda, p0->pos);
  glm_vec2_muladds(grad, -p1->invMass * lambda, p1->pos);
}

void solveDistVel(point_t *p0, point_t *p1, float dampingCoeff, float dt) {
  vec2 n;
  glm_vec2_sub(p1->pos, p0->pos, n);
  glm_vec2_normalize(n);
  float v0 = glm_vec2_dot(n, p0->vel);
  float v1 = glm_vec2_dot(n, p1->vel);
  float dv0 = (v1 - v0) * min(0.5, dampingCoeff * dt * p0->invMass);
  float dv1 = (v0 - v1) * min(0.5, dampingCoeff * dt * p1->invMass);
  glm_vec2_muladds(n, dv0, p0->vel);
  glm_vec2_muladds(n, dv1, p1->vel);
}

void solvePointVel(point_t *p, float dampingCoeff, float dt) {
  vec2 n;
  glm_vec2_copy(p->vel, n);

  float v = glm_vec2_distance(GLM_VEC2_ZERO, n);
  if (v == 0.0f) {
    v = 1.0f;
  }
  n[0] /= v;
  n[1] /= v;

  float dv = -v * min(1.0, dampingCoeff * dt * p->invMass);
  glm_vec2_muladds(n, dv, p->vel);
}

void simulate(float dt) {
  float sdt = dt / (float)state->numSubsteps;
  igText("df: %f, sdt: %f", dt, sdt);
  for (uint32_t step = 0; step < state->numSubsteps; step++) {

    // predict
    for (uint32_t i = 1; i < state->numPoints; i++) {
      point_t *p = &state->points[i];
      p->vel[1] -= state->gravity * sdt;
      glm_vec2_copy(p->pos, p->prev);
      glm_vec2_muladds(p->vel, sdt, p->pos);
    }

    // solve positions
    for (uint32_t i = 0; i < state->numPoints - 1; i++) {
      point_t *p = &state->points[i + 1];
      solveDistPos(
        &state->points[i],
        p,
        p->radius,
        p->compliance,
        p->unilateral,
        sdt
      );
    }

    // if (grabPointNr >= 0)
    //   solveDistPos(grabPoint, state->points[grabPointNr], 0, mouseCompliance, false, sdt);

    if (state->collisionHandling) {
      float minX = 0;
      point_t *p = &state->points[state->numPoints - 1];
      if (p->pos[0] < minX) {
        p->pos[0] = minX;
        if (p->vel[0] < 0)
          p->prev[0] = p->pos[0] + p->vel[0] * sdt;
      }
    }

    // update velocities
    for (uint32_t i = 1; i < state->numPoints; i++) {
      point_t *p = &state->points[i];
      glm_vec2_sub(p->pos, p->prev, p->vel);
      glm_vec2_scale(p->vel, 1.0f / sdt, p->vel);
      solvePointVel(p, state->globalDampingCoeff, sdt);
    }

    for (uint32_t i = 0; i < state->numPoints - 1; i++) {
      point_t *p = &state->points[i + 1];
      if (p->compliance > 0.0) {
        solveDistVel(
          &state->points[i],
          p,
          state->edgeDampingCoeff,
          sdt
        );
      }
    }
    // if (grabPointNr >= 0)
    //   solveDistVel(grabPoint, state->points[grabPointNr], mouseDampingCoeff, sdt);

    trailAdd(state->points[state->numPoints-1].pos);
  }
}

void timeStep() {
  float prevE = 0.0f;
  if (state->conserveEnergy) {
    prevE = computeEnergy();
  }
  double startTime = rawkit_now();
  if (state->lastTime != 0.0 && !state->paused) {
    simulate(startTime - state->lastTime);
  }
  state->lastTime = startTime;
  double endTime = rawkit_now();
  if (state->conserveEnergy) {
    forceEnergyConservation(prevE);
  }

  igText("%f", endTime - startTime);

  draw();
}

void init_state(bool reset) {
  state = rawkit_hot_state("state", state_t);
  if (reset || !state->numSubsteps) {

    state->numSubsteps = 50;
    state->lastTime = rawkit_now();
    state->gravity = 10.0f;
    state->edgeDampingCoeff = 0.0f;
    state->globalDampingCoeff = 0.0f;
    state->conserveEnergy = false;
    state->collisionHandling = false;
    state->showForces = false;
    state->mouseCompliance = 0.001f;
    state->mouseDampingCoeff = 100.0f;
    state->simWidth = 2.0f;
    state->drawScale = 1.0f;
    state->paused = false;

    // trail
    state->trailDist = 0.01f;
    state->showTrail = true;
    state->maxTrailLen = 100;
    state->trail = NULL;
    state->trailLast = 0;

    // points
    state->maxPoints = 16;
    state->numPoints = 4;
    state->pointSize = 5.0f;
    state->defaultRadius = 0.15f;
    state->defaultMass = 1.0f;
  }

  reset = reset || state->points == NULL;

  if (!state->points) {
    // TODO: if someone tweaks the "maxPoints" and causes a reload we should realloc this
    state->points = (point_t *)calloc(sizeof(point_t) * state->maxPoints, 1);
  }


  if (reset) {
    // populate points
    for (uint32_t i = 0; i < state->maxPoints; i++) {
      point_t *point = &state->points[i];

      point->mass =  i == 0 ? 0 : state->defaultMass;
      point->invMass = i == 0 ? 0 : 1 / state->defaultMass;
      point->radius = state->defaultRadius;
      point->compliance = 0;
      point->unilateral = false;
      point->force = 0;
      point->elongation = 0;
    }
  }
}

void render_state() {
  // render the state
  ImVec2 size = {70, 30};
  igBegin("state", 0, 0);
  if (igButton("restart", size)) {
    resetPos(false);
  }
  igSameLine(0.0f, 10.0f);
  if (igButton("defaults", size)) {
    init_state(true);
    resetPos(false);
  }

  igCheckbox("pause", &state->paused);
  igCheckbox("conserve energy", &state->conserveEnergy);
  igCheckbox("collisions", &state->collisionHandling);

  igSliderInt("substeps", (int *)&state->numSubsteps,  1, 500, "%u");

  // sim width
  {
    float dmin = 0.01f;
    float dmax = 10.0f;
    igSliderScalar("##sim-width", ImGuiDataType_Float, &state->simWidth,  &dmin, &dmax, "sim width %f", 1.0f);
  }

  // gravity
  {
    float dmin = -100.0f;
    float dmax = 100.0f;
    igSliderScalar("##sim-gravity", ImGuiDataType_Float, &state->gravity,  &dmin, &dmax, "gravity %f", 1.0f);
  }

  // edge damping coefficient
  {
    float dmin = 0.0f;
    float dmax = 1.0f;
    igSliderScalar("##sim-edgeDampingCoeff", ImGuiDataType_Float, &state->edgeDampingCoeff,  &dmin, &dmax, "edge damping %f", 1.0f);
  }

  // global damping coefficient
  {
    float dmin = 0.0f;
    float dmax = 1.0f;
    igSliderScalar("##sim-globalDampingCoeff", ImGuiDataType_Float, &state->globalDampingCoeff,  &dmin, &dmax, "global damping %f", 1.0f);
  }


  if (igCollapsingHeaderTreeNodeFlags("points", 0)) {
    igIndent(5.0f);
    if (igSliderInt("##count", (int *)&state->numPoints,  2, state->maxPoints, "count %u")) {
      resetPos(false);
    }

    char id[256] = {0};
    for (uint32_t i=1; i<state->numPoints;i++) {
      point_t *point = &state->points[i];
      igBeginGroup();
      igText("point %u", i, point->radius);
      igIndent(20.0);
      // mass
      {
        float dmin = 0.0000f;
        float dmax = 10.0f;
        sprintf(id, "##mass-%u", i);
        if (igSliderScalar(id, ImGuiDataType_Float, &point->mass,  &dmin, &dmax, "mass %f", 1.0f)) {
          if (point->mass != 0.0f) {
            point->invMass = 1.0f / point->mass;
            point->size = sqrt(point->mass);
          } else {
            point->invMass = 0.0f;
          }
        }
      }

      // compliance
      {
        float dmin = 0.0000f;
        float dmax = 0.01f;
        sprintf(id, "##compliance-%u", i);
        igSliderScalar(id, ImGuiDataType_Float, &point->compliance,  &dmin, &dmax, "compliance %f", 1.0f);
      }
      // state->pointSize = 5.0f;
      // state->defaultRadius = 0.15f;
      // state->defaultMass = 1.0f;
      igEndGroup();
    }

  }

  if (igCollapsingHeaderTreeNodeFlags("trail", 0)) {
    igIndent(5.0f);
    igCheckbox("enable", &state->showTrail);
    if (igSliderInt("max length", (int *)&state->maxTrailLen,  1, 10000, "%u")) {
      sb_free(state->trail);
      state->trail = NULL;
      state->trailLast = 0;
    }

    {
      float dmin = 0.00001f;
      float dmax = 0.1f;
      igSliderScalar("distance", ImGuiDataType_Float, &state->trailDist,  &dmin, &dmax, "%f", 1.0f);
    }
  }

  igEnd();
}

void setup() {
  init_state(false);
  resetPos(false);
}

void loop() {
  render_state();

  canvasOrig[0] = (float)rawkit_window_width() / 2.0;
  canvasOrig[1] = (float)rawkit_window_height() / 2.0;
  state->drawScale = (float)rawkit_window_width() / state->simWidth;

  timeStep();
}
