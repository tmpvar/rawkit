// implementation of Position Based Dynamics (Matthias Müller et al.)
#define NO_GDI
#include <rawkit/rawkit.h>

#include <glm/glm.hpp>
using namespace glm;


#include "blob.h"
#include "mouse.h"
#include <stb_sb.h>

//#define GRAVITY vec2(0.0, -90.8)
#define GRAVITY vec2(0.0, -9.0)
#define TAU 6.283185307179586
#define MAX_PARTICLES 0 + 1

enum class BodyType {
  POINT = 0,
  BLOB
};

struct Constraint {
  BodyType a_type = BodyType::POINT;
  uint32_t a;

  BodyType b_type = BodyType::POINT;
  uint32_t b;
  // distance constraint only...
  float rest_length;
  // inequality constraint
  bool contact;
};

struct State {
  Mouse mouse;
  vec2 screen;
  vec2 *positions;
  vec2 *velocities;
  float *inv_masses;
  vec2 *next_positions;

  Constraint *constraints;
  Blob **blobs;

  double last_time;
};

static inline vec2 reflect_elastic(vec2 I, vec2 N, float elasticity) {
  return I - (1.0f + elasticity) * N * dot(N, I);
}

void add_constraint(uint32_t a, uint32_t b, uint32_t type) {
  State *state = rawkit_hot_state("state", State);
  Constraint constraint = {
    .a = a,
    .b = b,
    // .type = type,
    .rest_length = distance(
      vec2(state->positions[a]),
      vec2(state->positions[b])
    )
  };


  sb_push(state->constraints, constraint);
}

void setup() {
  State *state = rawkit_hot_state("state", State);
  state->last_time = rawkit_now();
  state->screen = vec2(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );

  // reset the simulation on save
  sb_reset(state->positions);
  sb_reset(state->velocities);
  sb_reset(state->constraints);

  // randomly position some particles
  {
    // mouse
    sb_push(state->positions, vec2(0.0));
    sb_push(state->velocities, vec2(0.0));
    sb_push(state->inv_masses, 0.0);

    // i=1 is leaving space for the mouse as a particle
    for (uint32_t i=1; i<MAX_PARTICLES; i++) {
      vec2 pos(
        rawkit_randf() * state->screen.x,
        (rawkit_randf() * (state->screen.y - 200.0f)) + 200.f
      );

      vec2 vel(
        (rawkit_randf() * 2.0 - 1.0) * 100.0f,
        (rawkit_randf() * 2.0 - 1.0) * 100.0f
      );


      sb_push(state->positions, pos);
      sb_push(state->velocities, vel);
      sb_push(state->inv_masses, 1.0);
    }
  }

  // blobs
  if (1) {
    uint32_t c = sb_count(state->blobs);
    if (!c) {
      Polygon *polygon = new Polygon("poly");
      polygon->pos = vec2(300.0, 300.0);

      polygon->append(vec2(0.0, 0.0));
      polygon->append(vec2(200.0, 0.0));
      polygon->append(vec2(200.0, 200.0));
      polygon->append(vec2(150.0, 200.0));
      polygon->append(vec2(150.0, 50.0));
      polygon->append(vec2(0.0, 200.0));

      Blob *blob = new Blob(
        "square",
        vec2(polygon->aabb.width(), polygon->aabb.height())
      );

      blob->pos = polygon->pos;
      polygon->build_sdf(blob->sdf);
      delete polygon;

      blob->compute_center_of_mass();
      blob->pos += blob->center_of_mass;
      blob->rot = 0.9f;
      blob->circle_pack();
      blob->circle_graph();
      blob->extract_islands();
      sb_push(state->blobs, blob);
    }
  }

  {
    if (MAX_PARTICLES > 0) {
      vec2 *tmp = (vec2 *)realloc(state->next_positions, sizeof(vec2) * MAX_PARTICLES);
      if (!tmp) {
        printf("failed to realloc next positions\n");
        return;
      }
      state->next_positions = tmp;
    }
  }
}

void projectConstraint(State *state, Constraint constraint, float dt) {
  vec2 a = state->next_positions[constraint.a];
  float inv_mass_a = state->inv_masses[constraint.a];
  vec2 b = state->next_positions[constraint.b];
  float inv_mass_b = state->inv_masses[constraint.b];

  vec2 ndir = normalize(a - b);
  float d = distance(a, b);

  if (constraint.contact && d > constraint.rest_length) {
    return;
  }
  float inv_mass_sum = (inv_mass_a + inv_mass_b);


  float mass_ratio_a = 0.5f;
  float mass_ratio_b = 0.5f;
  if (inv_mass_sum != 0.0f) {
    mass_ratio_a = inv_mass_a / inv_mass_sum;
    mass_ratio_b = inv_mass_b / inv_mass_sum;
  }
  float diff = (d - constraint.rest_length) * dt;
  state->next_positions[constraint.a] -= mass_ratio_a * diff * ndir;
  state->next_positions[constraint.b] += mass_ratio_b * diff * ndir;
}


float angle(vec2 a, vec2 b) {
  vec2 d = a - b;
  return -atan2(d.y, d.x);
}

vec2 projectSDFConstraintDelta(vec2 pos, Blob *blob, float radius) {
  float d = blob->sample_bilinear_world(pos) - radius;

  if (d > 0.0) {
    return pos;
  }

  return blob->calc_normal_world(pos) * d;
}

vec2 projectSDFConstraint(vec2 prev_pos, vec2 pos, Blob *blob, float radius, float dt, float inv_mass = 1.0f) {
  float d = blob->sample_bilinear_world(pos) - radius;

  if (d > 0.0) {
    return pos;
  }

  vec2 ndir = prev_pos - pos;
  vec2 sdf_normal = blob->calc_normal_world(pos);
  float diff = d * 0.5;

  float inv_mass_sum = (inv_mass + blob->inv_mass);
  float particle_ratio = inv_mass / inv_mass_sum;
  float blob_ratio = blob->inv_mass / inv_mass_sum;

  vec2 new_poly_pos = blob->pos + blob_ratio * diff * sdf_normal;
  vec2 particle_pos = pos - particle_ratio * diff * sdf_normal;

  float old_angle = angle(blob->pos, pos);
  float new_angle = angle(blob->pos, pos - diff * sdf_normal);

  float w = blob->dims.x;
  float angle_diff = (new_angle - old_angle) * blob_ratio;
  blob->rot += angle_diff;// * w / distance(blob->pos, pos);
  blob->angular_velocity += angle_diff;
  blob->pos = new_poly_pos;

  return particle_pos;
}


vec2 projectWallConstraint(vec2 pos, vec2 screen, float radius, float inv_mass = 1.0f) {
  vec2 center = screen * 0.5f;
  auto sample = [center](vec2 pos) -> float {
    return -sdBox(pos - center, center);
  };

  float d = sample(pos) - radius;

  if (d > 0.0) {
    return vec2(0.0);
  }

  vec2 sdf_normal = calc_sdf_normal(pos, sample);
  return sdf_normal * d;
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  Context2D ctx;
  state->mouse.tick();
  double now = rawkit_now();
  float dt = glm::min(.02, now - state->last_time);
  // dt = 0.005;
  state->last_time = now;

  state->screen = vec2(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );

  vec2 mouse = vec2(
    state->mouse.pos.x,
    (float)rawkit_window_height() - state->mouse.pos.y
  );


  // add a blob under the mouse with the `space` key
  // if (igIsKeyDown(32)) {
  //   Blob *blob = new Blob("shared-square-sdf");
  //   blob->pos = mouse;
  //   blob->append(vec2(0.0, 0.0));
  //   blob->append(vec2(100.0, 0.0));
  //   blob->append(vec2(100.0, 100.0));
  //   blob->append(vec2(0.0, 100.0));
  //   blob->build_sdf();
  //   sb_push(state->blobs, blob);
  // }


  ctx.scale(vec2(1.0f, -1.0f));
  ctx.translate(vec2(0.0, -(float)rawkit_window_height()));

  uint32_t particle_count = sb_count(state->positions);
  uint32_t blob_count = sb_count(state->blobs);

  // TODO: pull these into their own lists
  float invMass = 1.0;
  float radius = 5.0;

  // generate collision constraints
  {
    sb_reset(state->constraints);
    for (uint32_t i=0; i<particle_count; i++) {
      for (uint32_t j=0; j<particle_count; j++) {
        if (i == j) {
          continue;
        }

        if (distance(state->next_positions[i], state->next_positions[j]) < radius + radius) {
          Constraint constraint = {
            .a = i,
            .b = j,
            .rest_length = radius+radius,
            .contact = true
          };

          sb_push(state->constraints, constraint);
        }
      }
    }
  }

  // mouse particle -> blobs
  {
    state->next_positions[0] = mouse;
    state->positions[0] = mouse;
    state->velocities[0] = vec2(0.0f);
  }

  uint32_t constraint_count = sb_count(state->constraints);

  // process the constraints
  float substeps = 10.0f;
  float substep_dt = dt/substeps;

  float elasticity = 0.5f;
  {

    for (uint32_t i=1; i<particle_count; i++) {
      state->next_positions[i] = state->positions[i];
    }


    for (float substep = 0.0; substep < substeps; substep++) {
      // prepare blob positions and rotations
      for (uint32_t i=0; i<blob_count; i++) {
        Blob *blob = state->blobs[i];
        blob->prev_pos = blob->pos;
        blob->rot = fmodf(blob->rot, TAU);
        blob->prev_rot = blob->rot;
      }

      // integrate with external forces
      {
        for (uint32_t i=0; i<particle_count; i++) {
          state->velocities[i] += substep_dt * GRAVITY;
          state->positions[i] = state->next_positions[i];
          state->next_positions[i] += state->velocities[i] * substep_dt;
        }
      }

      for (uint32_t i=0; i<constraint_count; i++) {
        projectConstraint(state, state->constraints[i], substep_dt);
      }

      // loose particle vs wall
      {
        for (uint32_t i=1; i<particle_count; i++) {
          vec2 pos = state->next_positions[i];
          vec2 delta = projectWallConstraint(pos, state->screen, radius);
          state->next_positions[i] -= delta;
        }
      }

      // integrate velocity w/ position to find the next potential location
      {
        for (uint32_t i=0; i<blob_count; i++) {
          Blob *blob = state->blobs[i];

          uint32_t blob_circle_count = sb_count(blob->circles);
          vec2 gravity(0.0);
          for (uint32_t i=0; i<blob_circle_count; i++) {
            PackedCircle circle = blob->circles[i];
            vec2 pos = rotate(circle.pos - blob->center_of_mass, blob->rot) + blob->pos;
            vec2 delta = substep_dt * GRAVITY * blob->inv_mass * 3.1459f * circle.radius  * circle.radius;
            blob->velocity += delta;
          }

          blob->pos += blob->velocity * substep_dt;
          blob->rot += blob->angular_velocity * substep_dt;
        }
      }

      // blob vs particle collisions
      {
        for (uint32_t particle_idx=0; particle_idx<particle_count; particle_idx++) {
          for (uint32_t blob_idx=0; blob_idx<blob_count; blob_idx++) {
            state->next_positions[particle_idx] = projectSDFConstraint(
              state->positions[particle_idx],
              state->next_positions[particle_idx],
              state->blobs[blob_idx],
              radius,
              substep_dt,
              // 1.0f
              particle_idx == 0 ? 0.001f : 1.0f
            );
          }
        }
      }

      // poly vs poly
      {
        float damp = 0.1f;
        for (uint32_t a_idx=0; a_idx<blob_count; a_idx++) {
          Blob *a = state->blobs[a_idx];
          uint32_t blob_circle_count = sb_count(a->circles);
          for (uint32_t b_idx=0; b_idx<blob_count; b_idx++) {
            if (a_idx==b_idx) {
              continue;
            }

            Blob *b = state->blobs[b_idx];

            for (uint32_t i=0; i<=blob_circle_count; i++) {
              PackedCircle circle = a->circles[i];
              vec2 pos = (i==blob_circle_count)
                ? a->pos
                : rotate(circle.pos - a->center_of_mass, a->rot) + a->pos;


              vec2 prev_pos = (i==blob_circle_count)
                ? a->prev_pos
                : rotate(circle.pos - a->center_of_mass, a->prev_rot) + a->prev_pos;

              vec2 next_pos = projectSDFConstraint(prev_pos, pos, b, 1.0f, a->inv_mass);

              // vec2 next_pos = glm::clamp(pos, vec2(0.0), state->screen);

              // TODO: apply proper angular + positional corrections
              // TODO: apply to b as well
              a->pos += (next_pos - pos);
            }
          }
        }
      }

      // sdf vs wall
      {
        for (uint32_t blob_idx=0; blob_idx<blob_count; blob_idx++) {
          Blob *blob = state->blobs[blob_idx];
          uint32_t blob_circle_count = sb_count(blob->circles);

          for (uint32_t i=0; i<blob_circle_count; i++) {
            PackedCircle circle = blob->circles[i];
            vec2 pos = rotate(circle.pos - blob->center_of_mass, blob->rot) + blob->pos;
            vec2 delta = projectWallConstraint(pos, state->screen, circle.radius);

            // TODO: apply proper angular + positional corrections

            vec2 new_poly_pos = blob->pos - delta;

            float old_angle = angle(blob->pos, pos);
            float new_angle = angle(blob->pos, pos + delta);

            float w = blob->dims.x;
            float angle_diff = (new_angle - old_angle);
            blob->rot += angle_diff;// * (1.0 - distance(blob->pos, pos) / w);
            blob->angular_velocity += angle_diff;
            blob->pos = new_poly_pos;
          }
        }
      }

      // apply particle results
      {
        for (uint32_t i=0; i<particle_count; i++) {
          state->velocities[i] = (state->next_positions[i] - state->positions[i]) / substep_dt;
          state->positions[i] = state->next_positions[i];
        }
      }

      // apply blob results
      {
        for (uint32_t i=0; i<blob_count; i++) {
          state->blobs[i]->velocity = (state->blobs[i]->pos - state->blobs[i]->prev_pos) / substep_dt;
          state->blobs[i]->angular_velocity = (state->blobs[i]->rot - state->blobs[i]->prev_rot) / substep_dt;
          state->blobs[i]->prev_pos = state->blobs[i]->pos;
          state->blobs[i]->prev_rot = state->blobs[i]->rot;
        }
      }
    }
  }

  igText("sim time: %f", (rawkit_now() - now) * 1000.0);

  // render blobs
  {
    for (uint32_t i=0; i<blob_count; i++) {
      state->blobs[i]->render(ctx);
    }
  }

  // draw all of the particles
  {
    ctx.fillColor(rgb(0x00, 0x00, 0xFF));
    for (uint32_t i=0; i<particle_count; i++) {
      ctx.fillColor(hsl((float)i/(float)particle_count, 0.5, 0.5));

      ctx.beginPath();
        ctx.arc(
          state->positions[i],
          radius
        );
        ctx.fill();
    }
  }

  // draw the constraints as lines
  {
    ctx.strokeColor(rgb(0xFF, 0xFF, 0xFF));
    for (uint32_t i=0; i<constraint_count; i++) {
      Constraint *c = &state->constraints[i];
      ctx.beginPath();
        ctx.moveTo(state->positions[c->a]);
        ctx.lineTo(state->positions[c->b]);
        ctx.stroke();
    }
  }
}