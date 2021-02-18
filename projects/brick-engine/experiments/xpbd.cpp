#include <rawkit/rawkit.h>

#include "polygon.h"
#include "blob.h"
#include "mouse.h"

#include "context-2d.h"

#include <glm/glm.hpp>
using namespace glm;

#define PIXEL_PER_METER 1000
#define GRAVITY_CONSTANT -9.8
#define GRAVITY vec2(0.0, GRAVITY_CONSTANT * PIXEL_PER_METER)

float cross(vec2 a, vec2 b) {
  return a.x * b.y - a.y * b.x;
}

float angle_between(vec2 a, vec2 b) {
  return atan2(
    cross(a, b),
    glm::dot(a, b)
  );
}

struct State {
  Mouse mouse;
  Blob **blobs;
  float last_time;

  vec2 *prev_positions;
  float *prev_rotations;
};

void setup() {
  State *state = rawkit_hot_state("state", State);
  if (state->last_time == 0.0f) {
    state->last_time = rawkit_now();
  }

  if (sb_count(state->blobs)) {
    u32 blob_count = sb_count(state->blobs);
    for (u32 i=0; i<blob_count; i++) {
      delete state->blobs[i];
    }
    sb_reset(state->blobs);
  }

  if (0){
    CircleBlob *c = new CircleBlob("circle", 200);
    c->pos = vec2(425, 200);
    sb_push(state->blobs, c);
  }

  if (0) {
    Polygon *polygon = new Polygon("poly");
    float w = 400.0;
    polygon->append(vec2(0.0, 0.0));
    polygon->append(vec2(w, 0.0));
    polygon->append(vec2(w, 150.0));
    polygon->append(vec2(0.0, 150.0));
    polygon->append(vec2(0.0, 75.0));
    polygon->append(vec2(w-50.0, 100.0));
    polygon->append(vec2(w-50.0, 50.0));
    polygon->append(vec2(0.0, 75.0));
    // polygon->append(vec2(150.0, 100.0));
    // polygon->append(vec2(0.0, 75.0));

    sprintf(blob_tmp_str, "blob#0");
    Blob *blob = new Blob(
      blob_tmp_str,
      vec2(polygon->aabb.width(), polygon->aabb.height())
    );

    polygon->build_sdf(blob->sdf);

    blob->pos = vec2(700.0, 200.0);
    blob->compute_center_of_mass();
    blob->circle_pack();
    blob->circle_graph();
    blob->extract_islands();
    sb_push(state->blobs, blob);
    delete polygon;
  }

  {
    BoxBlob *blob = new BoxBlob("box", vec2(100, 600));
    blob->pos = vec2(200, 400);
    sb_push(state->blobs, blob);
  }



    {
    BoxBlob *blob = new BoxBlob("bigbox", vec2(6, 600));
    blob->pos = vec2(200, 400);
    sb_push(state->blobs, blob);
  }
}



void particle_vs_blob(vec2 &pos, float radius, float inv_mass, Blob *blob, float dt) {
  float d = blob->sample_bilinear_world(pos) - radius;
  if (d >= 0.0) {
    return;
  }

  vec2 delta = blob->calc_normal_world(pos) * d;
  vec2 rel = pos - blob->pos;

  float inertia = distance(blob->pos, pos) / (length(blob->dims) * 0.5f);

  float r = angle_between(
    rel,
    rel + delta
  );

  dt = 1.0/60.0f;
  blob->rot += r * inertia * dt;
  blob->pos += delta * dt;
}


void blob_vs_bounds(Blob *blob, vec2 dims) {
  if (!blob || !blob->circles) {
    return;
  }

  u32 c = sb_count(blob->circles);
  for (u32 i=0; i<c; i++) {
    PackedCircle circle = blob->circles[i];
    if (circle.radius < 1.0f || circle.signed_distance >= 0.0) {
      printf("WARN: invalid packed circle\n");
      continue;
    }
    vec2 pos = blob->gridToWorld(circle.pos);
    float radius = abs(circle.radius) + 10.0;
    vec2 pos2 = clamp(
      pos,
      vec2(radius),
      dims - radius
    );

    if (all(equal(pos, pos2))) {
      continue;
    }
    // vec2 blob_dir = blob->calc_normal_world(pos);

    // // TODO: this doesn't take into account the mass ratio along the extent of the shape
    float inertia = distance(blob->pos, pos) / (length(blob->dims) * 0.5f);
    // float inv_inertia = 1.0 / inertia;

    vec2 delta = pos2 - pos;
    vec2 rel = pos - blob->pos;
    float r = angle_between(
      rel,
      rel + delta
    );

    float dt = 1.0/60.0f;
    blob->rot += r * inertia * dt;
    blob->pos += delta * dt;// * (1.0f - inertia);
  }
}

void blob_vs_blob(Blob *blob0, Blob *blob1) {

  if (!blob0 || !blob0->circles || !blob1) {
    return;
  }

  u32 c = sb_count(blob0->circles);
  for (u32 i=0; i<c; i++) {
    PackedCircle circle = blob0->circles[i];
    vec2 pos = blob0->gridToWorld(circle.pos);
    float radius = abs(circle.signed_distance);
    float d = blob1->sample_bilinear_world(pos) - radius;
    if (d >= 0.0) {
      continue;
    }

    vec2 delta = blob1->calc_normal_world(pos) * -d;
    // printf("delta(%f, %f) dist(%f)\n", delta.x, delta.y, d);

    // // TODO: this doesn't take into account the mass ratio along the extent of the shape
    vec2 rel = pos - blob0->pos;
    float inertia = distance(blob0->pos, pos) / (length(blob0->dims) * 0.5f);
    // inertia *= 1.5f;

    float r = angle_between(
      rel,
      rel + delta
    );


    float w = (blob0->inv_mass) / (blob0->inv_mass + blob1->inv_mass);

    float dt = 1.0/60.0f;
    blob0->rot += r * inertia * w * dt;
    blob0->pos += delta * w * dt;
  }
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  Context2D ctx;


  state->mouse.tick();

  float now = rawkit_now();
  float dt = now - state->last_time;
  state->last_time = now;

  vec2 screen = vec2(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );

  vec2 mouse = vec2(
    state->mouse.pos.x,
    (float)rawkit_window_height() - state->mouse.pos.y
  );

  ctx.save();
    ctx.scale(vec2(1.0f, -1.0f));
    ctx.translate(vec2(0.0, -screen.y));

  u32 substeps = 10;
  float substep_dt = dt / (float)substeps;

  // render the blobs
  {
    u32 c = sb_count(state->blobs);
    for(u32 i=0; i<c; i++) {
      state->blobs[i]->render(ctx);
      state->blobs[i]->render_circles(ctx);
      igText("blob %s: inertia(%f)", state->blobs[i]->name, state->blobs[i]->inertia);
    }
  }

  for (u32 step=0.0f; step<substeps; step++) {
    // integration / reset
    {
      sb_reset(state->prev_positions);
      sb_reset(state->prev_rotations);

      u32 c = sb_count(state->blobs);
      for(u32 i=0; i<c; i++) {

        Blob *blob = state->blobs[i];
        sb_push(state->prev_positions, blob->pos);
        sb_push(state->prev_rotations, blob->rot);

        blob->velocity += GRAVITY * substep_dt;
        blob->pos += blob->velocity * substep_dt;
        blob->rot += blob->angular_velocity * substep_dt;
      }
    }

    // mouse vs blobs
    {
      float mouse_radius = 3.0f;
      u32 c = sb_count(state->blobs);
      for(u32 i=0; i<c; i++) {
        particle_vs_blob(mouse, mouse_radius, 0.0f, state->blobs[i], substep_dt);
        blob_vs_bounds(state->blobs[i], screen);
      }
    }

    // blob vs blob
    {
      u32 c = sb_count(state->blobs);
      for (u32 i=0; i<c; i++) {
        for (u32 j=0; j<c; j++) {
          if (i == j) {
            continue;
          }
          blob_vs_blob(state->blobs[j], state->blobs[i]);
        }
      }
    }

    // compute new velocity
    {
      u32 c = sb_count(state->blobs);
      for(u32 i=0; i<c; i++) {
        Blob *blob = state->blobs[i];
        blob->velocity = (blob->pos - state->prev_positions[i]) / substep_dt;
        blob->angular_velocity = (blob->rot - state->prev_rotations[i]) / substep_dt * 0.99;
      }
    }
  }



  // render the mouse
  {
    ctx.fillColor(rgb(0xFF, 0, 0));
    ctx.beginPath();
      ctx.arc(mouse, 3.0f);
      ctx.fill();
  }


}