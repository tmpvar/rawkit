// ported from https://github.com/matthias-research/pages/blob/master/challenges/pendulum.html

#include <rawkit/rawkit.h>
#include <cglm/vec2.h>
#include <math.h>
#include "stb_sb.h"

uint32_t numSubsteps = 50;

float defaultRadius = 0.15f;
float defaultMass = 1.0f;
float gravity = 10.0f;
float dt = 1.0f / 60.0f;
float edgeDampingCoeff = 0.0f;
float globalDampingCoeff = 0.0f;

bool conserveEnergy = false;
bool collisionHandling = false;
bool showTrail = true;
bool showForces = false;
uint32_t maxPoints = 50;
uint32_t numPoints = 4;

uint32_t maxTrailLen = 100000;
float trailDist = 0.01f;

float mouseCompliance = 0.001f;
float mouseDampingCoeff = 100.0f;

vec2 canvasOrig;
float simWidth = 2.0f;
float pointSize = 5.0f;
float drawScale = 1.0f;

uint32_t timeFrames = 0;
bool paused = false;

typedef struct point_t {
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

point_t *points = NULL;
tail_t *trail = NULL;
uint32_t trailLast = 0;

void trailAdd(vec2 p) {
  uint32_t trail_length = sb_count(trail);
  if (trail_length == 0) {
    tail_t t = {};
    glm_vec2_copy(p, t.pos);
    sb_push(trail, t);
    return;
  }

  float d2 = glm_vec2_distance2(trail[trailLast].pos, p);
  if (d2 > trailDist * trailDist) {
    trailLast = (trailLast + 1) % maxTrailLen;
    if (trail_length < maxTrailLen) {
      tail_t t = {};
      glm_vec2_copy(p, t.pos);
      sb_push(trail, t);
    } else {
      glm_vec2_copy(p, (&trail[trailLast])->pos);
    }
  }
}


void resetPos(bool equilibrium) {
  vec2 pos;
  pos[0] = equilibrium ? 0 : points[1].radius;
  pos[1] = equilibrium ? 0 : - points[1].radius;
  printf("reset pos?? %f, %f\n", pos[0], pos[1]);

  uint32_t points_length = sb_count(points);

  for (uint32_t i = 1; i < points_length; i++) {
    point_t *p = &points[i];
    p->size = sqrt(1.0 / p->invMass);
    pos[1] = equilibrium ? pos[1] - p->radius : pos[1] + p->radius;
    glm_vec2_copy(pos, p->pos);
    glm_vec2_copy(pos, p->prev);

    p->vel[0] = 0;
    p->vel[1] = 0;
  }
  sb_free(trail);
  trailLast = 0;
}

float computeEnergy() {
  float e = 0.0f;
  for (uint32_t i = 1; i < numPoints; i++) {
    point_t *p = &points[i];
    e += p->pos[1] / p->invMass * gravity + 0.5 / p->invMass * glm_vec2_distance2(GLM_VEC2_ZERO, p->vel);
  }
  return e;
}

void forceEnergyConservation(float prevE) {
  float dE = (computeEnergy() - prevE) / (numPoints - 1);
  if (dE < 0) {
    float postE = computeEnergy();

    for (uint32_t i = 1; i < numPoints; i++) {
      point_t *p = &points[i];
      float Ek = 0.5 / p->invMass * glm_vec2_distance2(GLM_VEC2_ZERO, p->vel);
      float s = sqrt((Ek - dE) / Ek);
      glm_vec2_scale(p->vel, s, p->vel);
    }
  }
}

void draw() {
  rawkit_vg_t *vg = rawkit_vg_default();
  rawkit_vg_stroke_width(vg, 1.0f);
  float x = canvasOrig[0];
  float y = canvasOrig[1];

  for (uint32_t i = 1; i < numPoints; i++) {
    float avgX = x;
    float avgY = y;
    point_t *p = &points[i];
    if (p->compliance > 0.0f) {
      rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0, 0, 0xFF));
    } else if (p->unilateral) {
      rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0, 0xFF, 0));
    } else {
      rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0x00, 0x0));
    }

    rawkit_vg_begin_path(vg);
    rawkit_vg_move_to(vg, x, y);
    x = canvasOrig[0] + p->pos[0] * drawScale;
    y = canvasOrig[1] - p->pos[1] * drawScale;
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
//     c.moveTo(canvasOrig.x + grabPoint.pos.x * drawScale, canvasOrig.y - grabPoint.pos.y * drawScale);
//     c.lineTo(canvasOrig.x + points[grabPointNr].pos.x * drawScale, canvasOrig.y - points[grabPointNr].pos.y * drawScale);
//     c.stroke();
//   }

  rawkit_vg_fill_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
  for (uint32_t i = 1; i < numPoints; i++) {
    point_t *p = &points[i];
    igText("p%u: vel(%.2f, %.2f) pos(%.2f, %.2f)", i, p->unilateral, p->vel[0], p->vel[1]);
    x = canvasOrig[0] + p->pos[0] * drawScale;
    y = canvasOrig[1] - p->pos[1] * drawScale;
    rawkit_vg_begin_path(vg);
    rawkit_vg_arc(vg, x, y, pointSize * p->size, 0, M_PI*2.0f, 1);
    rawkit_vg_close_path(vg);
    rawkit_vg_fill(vg);
  }

  uint32_t trail_length = sb_count(trail);
  if (trail_length > 1) {
    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(52, 235, 137));
    rawkit_vg_begin_path(vg);
    uint32_t pos = (trailLast + 1) % trail_length;
    rawkit_vg_move_to(vg,
      canvasOrig[0] + trail[pos].pos[0] * drawScale,
      canvasOrig[1] - trail[pos].pos[1] * drawScale
    );

    for (uint32_t i = 1; i < trail_length - 1; i++) {
      pos = (pos + 1) % trail_length;
      rawkit_vg_line_to(vg,
        canvasOrig[0] + trail[pos].pos[0] * drawScale,
        canvasOrig[1] - trail[pos].pos[1] * drawScale
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

void simulate(float dt)
{
  float sdt = dt / (float)numSubsteps;
  igText("df: %f, sdt: %f", dt, sdt);
  for (uint32_t step = 0; step < numSubsteps; step++) {

    // predict
    for (uint32_t i = 1; i < numPoints; i++) {
      point_t *p = &points[i];
      p->vel[1] -= gravity * sdt;
      glm_vec2_copy(p->pos, p->prev);
      glm_vec2_muladds(p->vel, sdt, p->pos);
    }

    // solve positions
    for (uint32_t i = 0; i < numPoints - 1; i++) {
      point_t *p = &points[i + 1];
      solveDistPos(
        &points[i],
        p,
        p->radius,
        p->compliance,
        p->unilateral,
        sdt
      );
    }

    // if (grabPointNr >= 0)
    //   solveDistPos(grabPoint, points[grabPointNr], 0, mouseCompliance, false, sdt);

    // if (collisionHandling) {
    //   var minX = 0;
    //   p = points[numPoints - 1];
    //   if (p.pos.x < minX) {
    //     p.pos.x = minX;
    //     if (p.vel.x < 0)
    //       p.prev.x = p.pos.x + p.vel.x * sdt;
    //   }
    // }

    // update velocities
    for (uint32_t i = 1; i < numPoints; i++) {
      point_t *p = &points[i];
      glm_vec2_sub(p->pos, p->prev, p->vel);
      glm_vec2_scale(p->vel, 1.0f / sdt, p->vel);
      solvePointVel(p, globalDampingCoeff, sdt);
    }

    for (uint32_t i = 0; i < numPoints - 1; i++) {
      point_t *p = &points[i + 1];
      if (p->compliance > 0.0) {
        solveDistVel(
          &points[i],
          p,
          edgeDampingCoeff,
          sdt
        );
      }
    }
    // if (grabPointNr >= 0)
    //   solveDistVel(grabPoint, points[grabPointNr], mouseDampingCoeff, sdt);

    trailAdd(points[numPoints-1].pos);
  }
}

double lastTime = 0.0;
void timeStep() {
  float prevE = 0.0f;
  if (conserveEnergy) {
    prevE = computeEnergy();
  }
  double startTime = rawkit_now();
  if (lastTime != 0.0) {
    simulate(startTime - lastTime);
  }
  lastTime = startTime;
  double endTime = rawkit_now();
  if (conserveEnergy) {
    forceEnergyConservation(prevE);
  }

  igText("%.03f", endTime - startTime);

  draw();
}

void setup() {
  lastTime = rawkit_now();
  // populate points
  {
    for (uint32_t i = 0; i < maxPoints; i++) {
      point_t point = {};

      point.invMass = i == 0 ? 0 : 1 / defaultMass;
      point.radius = i == 0 ? 0 : defaultRadius;
      point.compliance = 0;
      point.unilateral = false;
      point.force = 0;
      point.elongation = 0;

      sb_push(points, point);
    }
  }

  resetPos(false);
}

void loop() {
  canvasOrig[0] = (float)rawkit_window_width() / 2.0;
  canvasOrig[1] = (float)rawkit_window_height() / 2.0;
  drawScale = (float)rawkit_window_width() / simWidth;

  timeStep();
}
