// implementation of Position Based Dynamics (Matthias Müller et al.)
#define NO_GDI
#include <rawkit/rawkit.h>

#include <glm/glm.hpp>
using namespace glm;

#include "blob.h"
#include "mouse.h"
#include "segment.h"
#include <stb_sb.h>

//#define GRAVITY vec2(0.0, -90.8)
#define GRAVITY vec2(0.0, -90.0)
#define TAU 6.283185307179586
#define MAX_PARTICLES 2 + 1

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

      for (uint32_t i=0; i<2; i++) {
        Polygon *polygon = new Polygon("poly");
        polygon->pos = vec2(300.0, 300.0);

        polygon->append(vec2(0.0, 0.0));
        polygon->append(vec2(200.0, 0.0));
        polygon->append(vec2(200.0, 200.0));
        polygon->append(vec2(150.0, 200.0));
        polygon->append(vec2(150.0, 50.0));
        polygon->append(vec2(0.0, 200.0));
        sprintf(blob_tmp_str, "blob#%u", i);
        Blob *blob = new Blob(
          blob_tmp_str,
          vec2(polygon->aabb.width(), polygon->aabb.height())
        );

        blob->pos = vec2(100.0f + (float)i * 250.0f, 100.0f);
        polygon->build_sdf(blob->sdf);

        blob->compute_center_of_mass();
        blob->pos += blob->center_of_mass;
        blob->rot = 0.9f;
        blob->circle_pack();
        blob->circle_graph();
        blob->extract_islands();
        sb_push(state->blobs, blob);
        delete polygon;
      }
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
  CNAN(a);
  CNAN(b);
  vec2 d = a - b;
  CNAN(d);

  float r = -atan2(d.y, d.x);
  CNAN(r);
  return r;
}

vec2 projectSDFConstraint(vec2 prev_pos, vec2 pos, Blob *blob, float radius, float dt, float inv_mass = 1.0f) {
  float d = blob->sample_bilinear_world(pos) - radius;

  CNAN(prev_pos);
  CNAN(pos);
  CNAN(radius);

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

void projectBlobVsBlobConstraint(Blob *a, Blob *b, float dt) {
  // test a's circles against b's sdf

  uint32_t a_circle_count = sb_count(a->circles);
  for (uint32_t i=0; i<a_circle_count; i++) {
    PackedCircle circle = a->circles[i];

    if (circle.radius <= 0.0f) {
      printf("detected circle with <= 0 radius\n");
      continue;
    }

    vec2 pos = CNAN(rotate(circle.pos - a->center_of_mass, a->rot) + a->pos);
    vec2 prev_pos = CNAN(rotate(circle.pos - a->center_of_mass, a->prev_rot) + a->prev_pos);

    float d = b->sample_bilinear_world(pos) - abs(circle.signed_distance);
    if (d > 0.0) {
      continue;
    }

    float mass = CNAN(glm::pi<float>() * glm::pow(circle.radius, 2.0f) * a->density);

    double inv_mass = CNAN(1.0 / mass);
    vec2 sdf_normal = CNAN(b->calc_normal_world(pos));

    double inv_mass_sum = CNAN(inv_mass + b->inv_mass);
    float a_ratio = CNAN(inv_mass / inv_mass_sum);
    CNAN(a_ratio);
    float b_ratio = CNAN(b->inv_mass / inv_mass_sum);
    CNAN(b_ratio);


    b->pos -= CNAN(abs(d) * b_ratio * sdf_normal * dt);
    a->pos += CNAN(abs(d) * a_ratio * sdf_normal * dt);

    // float old_angle = angle(a->pos, pos);
    // float new_angle = angle(a->pos, pos - diff * sdf_normal);

    // float w = a->dims.x;
    // float angle_diff = (new_angle - old_angle) * a_ratio;
    // a->rot += angle_diff;// * w / distance(blob->pos, pos);
    // a->angular_velocity += angle_diff;


    // TODO: apply proper angular + positional corrections
    // TODO: apply to b as well
    // a->pos += (next_pos - pos);
  }



  // vec2 ndir = prev_pos - pos;
  // vec2 sdf_normal = blob->calc_normal_world(pos);
  // float diff = d * 0.5;

  // float inv_mass_sum = (inv_mass + blob->inv_mass);
  // float particle_ratio = inv_mass / inv_mass_sum;
  // float blob_ratio = blob->inv_mass / inv_mass_sum;

  // vec2 new_poly_pos = blob->pos + blob_ratio * diff * sdf_normal;
  // vec2 particle_pos = pos - particle_ratio * diff * sdf_normal;

  // float old_angle = angle(blob->pos, pos);
  // float new_angle = angle(blob->pos, pos - diff * sdf_normal);

  // float w = blob->dims.x;
  // float angle_diff = (new_angle - old_angle) * blob_ratio;
  // blob->rot += angle_diff;// * w / distance(blob->pos, pos);
  // blob->angular_velocity += angle_diff;
  // blob->pos = new_poly_pos;

  // return particle_pos;
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

  if (nan_found) {
    igText("nan found .. stopping");
    return;
  }
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


  uint32_t blob_count = sb_count(state->blobs);
  if (state->mouse.was_down && !state->mouse.down) {
    Segment cut = {
      .start = vec2(
        state->mouse.down_pos.x,
        (float)rawkit_window_height() - state->mouse.down_pos.y
      ),
      .end = mouse
    };

    vec2 cut_normal = cut.normal();

    Blob **new_blobs = nullptr;
    printf("brick-engine %u\n", __LINE__);
    for (uint32_t i=0; i<blob_count; i++) {
      Blob *blob = state->blobs[i];
      if (!blob) {
        printf("invalid blob!\n");
        continue;
      }
      {
        Blob **blobs = blob->slice_with_line(
          cut.start,
          cut.end
        );

        if (!blobs) {
          sb_push(new_blobs, blob);
          continue;
        }

        delete blob;
        state->blobs[i] = nullptr;

        uint32_t blob_count = sb_count(blobs);
        for (uint32_t i=0; i<blob_count; i++) {
          if (!blobs[i]) {
            continue;
          }

            blobs[i]->circle_pack();
          #if 1
            blobs[i]->circle_graph();
            Blob **islands = blobs[i]->extract_islands();
            uint32_t island_count = sb_count(islands);
            for (uint32_t j=0; j<island_count; j++) {
              if (!islands[j]) {
                continue;
              }
              // TODO: move islands away from each other along the cut normal
              vec2 center = islands[j]->pos;
              float o = cut.orientation(center);
              islands[j]->pos -= (o * cut_normal) * 10.0f;

              sb_push(new_blobs, islands[j]);
            }
            delete blobs[i];
            blobs[i] = nullptr;
          #else
            sb_push(new_blobs, blobs[i]);
          #endif
        }

        sb_free(blobs);
      }
    }

    sb_free(state->blobs);
    state->blobs = new_blobs;
    blob_count = sb_count(state->blobs);
  }


  ctx.scale(vec2(1.0f, -1.0f));
  ctx.translate(vec2(0.0, -(float)rawkit_window_height()));

  uint32_t particle_count = sb_count(state->positions);

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
igText("substep_dt(%f)", substep_dt);
  float elasticity = 0.5f;
  {

    for (uint32_t i=1; i<particle_count; i++) {
      state->next_positions[i] = state->positions[i];
    }


    for (float substep = 0.0; substep < substeps; substep++) {
      // prepare blob positions and rotations
      for (uint32_t i=0; i<blob_count; i++) {
        Blob *blob = state->blobs[i];
        blob->prev_pos = CNAN(blob->pos);
        blob->rot = CNAN(fmodf(blob->rot, TAU));
        blob->prev_rot = CNAN(blob->rot);
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
            vec2 pos = CNAN(rotate(circle.pos - blob->center_of_mass, blob->rot) + blob->pos);
            vec2 delta = CNAN(substep_dt * GRAVITY * blob->inv_mass * glm::pi<float>() * glm::pow(circle.radius, 2.0f));
            blob->velocity += delta;
          }

          blob->pos += CNAN(blob->velocity * substep_dt);
          blob->rot += CNAN(blob->angular_velocity * substep_dt);
        }
      }

      // blob vs particle collisions
      if (1) {
        for (uint32_t particle_idx=0; particle_idx<particle_count; particle_idx++) {
          if (particle_idx == 0 && state->mouse.down) {
            continue;
          }

          for (uint32_t blob_idx=0; blob_idx<blob_count; blob_idx++) {
            state->next_positions[particle_idx] = projectSDFConstraint(
              state->positions[particle_idx],
              state->next_positions[particle_idx],
              state->blobs[blob_idx],
              radius,
              substep_dt,
              // 1.0f
              particle_idx == 0 ? 0.01f : 1.0f
            );
          }
        }
      }

      // blob vs blob
      {
        float damp = 0.1f;
        for (uint32_t a_idx=0; a_idx<blob_count; a_idx++) {
          Blob *a = state->blobs[a_idx];
          uint32_t blob_circle_count = sb_count(a->circles);
          for (uint32_t b_idx=0; b_idx<blob_count; b_idx++) {
            if (a_idx==b_idx) {
              continue;
            }
            projectBlobVsBlobConstraint(a, state->blobs[b_idx], substep_dt);
          }
        }
      }

      // sdf vs wall
      if (1) {
        for (uint32_t blob_idx=0; blob_idx<blob_count; blob_idx++) {
          Blob *blob = state->blobs[blob_idx];

          // blob->pos = glm::clamp(
          //   blob->pos,
          //   vec2(0.0),
          //   vec2(
          //     (float)rawkit_window_width(),
          //     (float)rawkit_window_height()
          //   )
          // );
          // continue;


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
      state->blobs[i]->render_circles(ctx);
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