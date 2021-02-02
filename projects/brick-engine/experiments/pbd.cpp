// implementation of Position Based Dynamics (Matthias Müller et al.)
#define NO_GDI
#include <rawkit/rawkit.h>

#include <glm/glm.hpp>
using namespace glm;

#include "polygon.h"
#include "mouse.h"
#include <stb_sb.h>

#define GRAVITY vec2(0.0, -90.8)
#define TAU 6.283185307179586
#define MAX_PARTICLES 200 + 1

enum class BodyType {
  POINT = 0,
  POLYGON
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
  Polygon **polygons;

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

  // polygons
  if (1) {
    uint32_t c = sb_count(state->polygons);
    if (!c) {
      // {
      //   Polygon *polygon = new Polygon("floor");
      //   polygon->pos = vec2(0.0);
      //   polygon->append(vec2(0.0, 0.0));
      //   polygon->append(vec2(state->screen.x, 0.0));
      //   polygon->append(vec2(state->screen.x, 800.0));
      //   polygon->append(vec2(state->screen.x - 30.0f, 800.0));
      //   polygon->append(vec2(state->screen.x - 100.0f, 200.0));
      //   polygon->append(vec2(300.0, 30.0));
      //   polygon->append(vec2(0.0, 800.0));

      //   polygon->rebuild_sdf();
      //   sb_push(state->polygons, polygon);
      // }

      {
        Polygon *polygon = new Polygon("square 1");
        polygon->pos = vec2(300.0, 300.0);
        polygon->append(vec2(0.0, 0.0));
        polygon->append(vec2(100.0, 100.0));
        polygon->append(vec2(200.0, 0.0));
        polygon->append(vec2(400.0, 100.0));
        polygon->append(vec2(200.0, 200.0));
        polygon->append(vec2(100.0, 150.0));
        polygon->append(vec2(0.0, 200.0));
        polygon->append(vec2(0.0, 0.0));
        polygon->rebuild_sdf();
        sb_push(state->polygons, polygon);
      }

      {
        Polygon *polygon = new Polygon("square 2");
        polygon->pos = vec2(600.0, 300.0);
        polygon->append(vec2(0.0, 0.0));
        polygon->append(vec2(200.0, 0.0));
        polygon->append(vec2(200.0, 200.0));
        polygon->append(vec2(0.0, 0.0));
        polygon->rebuild_sdf();
        sb_push(state->polygons, polygon);
      }
    } else {
      for (uint32_t i=0; i<c; i++) {
        state->polygons[i]->rebuild_sdf();
        state->polygons[i]->pos = vec2(300.0 + (float)i * 300.0, 300.0);
        state->polygons[i]->prev_pos = state->polygons[i]->pos;
        state->polygons[i]->rot = 0.0f;
        state->polygons[i]->velocity = vec2(0.0);
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
  vec2 d = a - b;
  return -atan2(d.y, d.x);
}

vec2 projectSDFConstraintDelta(vec2 pos, Polygon *polygon, float radius) {
  float d = polygon->sample_bilinear_world(pos) - radius;

  if (d > 0.0) {
    return pos;
  }

  return polygon->calc_normal_world(pos) * d;
}

vec2 projectSDFConstraint(vec2 prev_pos, vec2 pos, Polygon *polygon, float radius, float dt, float inv_mass = 1.0f) {
  float d = polygon->sample_bilinear_world(pos) - radius;

  if (d > 0.0) {
    return pos;
  }

  vec2 ndir = prev_pos - pos;
  vec2 sdf_normal = polygon->calc_normal_world(pos);
  float diff = d * 0.5;

  float inv_mass_sum = (inv_mass + polygon->inv_mass);
  float particle_ratio = inv_mass / inv_mass_sum;
  float polygon_ratio = polygon->inv_mass / inv_mass_sum;

  vec2 new_poly_pos = polygon->pos + polygon_ratio * diff * sdf_normal;
  vec2 particle_pos = pos - particle_ratio * diff * sdf_normal;

  float old_angle = angle(polygon->pos, pos);
  float new_angle = angle(polygon->pos, pos - diff * sdf_normal);

  float w = (polygon->aabb.ub.x - polygon->aabb.lb.x);
  float angle_diff = (new_angle - old_angle) * polygon_ratio;
  polygon->rot += angle_diff;// * w / distance(polygon->pos, pos);
  polygon->angular_velocity += angle_diff;
  polygon->pos = new_poly_pos;

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

vec2 projectSDFConstraint1(vec2 prev_pos, vec2 pos, vec2 *velocity, Polygon *polygon, float radius, float inv_mass = 1.0f) {
  vec2 local = polygon->worldToLocal(pos);
  float d = polygon->sample_bilinear_local(local) - radius;

  if (d > 0.0) {
    return pos;
  }

  vec2 ndir = prev_pos - pos;
  vec2 sdf_normal = polygon->calc_normal_world(pos);
  float diff = abs(d);

  // TODO: actual mass ratio
  float inv_mass_sum = (inv_mass + polygon->inv_mass);
  float particle_ratio = inv_mass / inv_mass_sum;
  float polygon_ratio = polygon->inv_mass / inv_mass_sum;

  // move back to the surface
  // TODO: this is not quite right, but I'm not sure how to setup `ret` such
  //       that the next velocity calculation results in `reflect(ndir, sdf_normal)`
  /*
    prev ret
      \  .
      -o-.--
        \.
        pos
  */
  vec2 particle_pos = pos + particle_ratio * diff * sdf_normal;
  vec2 new_local = local + polygon_ratio * diff * sdf_normal;

  // TODO: this assumes a square.
  vec2 world_center = pos + polygon->center_of_mass;

  // vec2 od = local - polygon->center_of_mass;
  // vec2 nd = new_local - polygon->center_of_mass;

  vec2 od = pos - world_center;
  vec2 nd = new_local - polygon->center_of_mass;

  float old_angle = -atan2(od.y, od.x);
  float new_angle = -atan2(nd.y, nd.x);

  float w = polygon->aabb.ub.x - polygon->aabb.lb.x;

  vec2 dd = nd - od;
  if (dd.y == 0.0) {
    dd.y = 0.000000000000001;
  }
  // float angle = atan2(dd.x/dd.y, 1.0);
  float angle = new_angle - old_angle;
  printf("angle(%f) = %f - %f\n", angle, old_angle, new_angle);
  // polygon->rot += angle * length(nd) * polygon_ratio;
  polygon->rot += angle * w / length(nd) * polygon_ratio * 0.1;
  polygon->angular_velocity += angle * w / length(nd) * polygon_ratio * 0.1;
  // polygon->angular_velocity += (angle * w / length(nd) * polygon_ratio);

  // compute the angular delta between center->local and center->new_local
  // and apply it to rot/angular_velocity
  // based on the distance from the center of mass (for now..)
  // TODO: use the computed inertial tensor




  // append the reflection vector
  // TODO: this adds synthetic friction but it shouldn't
  // ret += reflect(normalize(pos - ret), sdf_normal) * diff;
  // *velocity += reflect(normalize(ndir), sdf_normal) * diff;
  return particle_pos;
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  state->mouse.tick();
  double now = rawkit_now();
  float dt = min(.02, now - state->last_time);
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


  // add a polygon under the mouse with the `space` key
  if (igIsKeyDown(32)) {
    Polygon *polygon = new Polygon("shared-square-sdf");
    polygon->pos = mouse;
    polygon->append(vec2(0.0, 0.0));
    polygon->append(vec2(100.0, 0.0));
    polygon->append(vec2(100.0, 100.0));
    polygon->append(vec2(0.0, 100.0));
    polygon->rebuild_sdf();
    sb_push(state->polygons, polygon);
  }


  rawkit_vg_t *vg = rawkit_default_vg();
  rawkit_vg_scale(vg, 1.0, -1.0);
  rawkit_vg_translate(vg, 0.0, -(float)rawkit_window_height());

  uint32_t particle_count = sb_count(state->positions);
  uint32_t polygon_count = sb_count(state->polygons);

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

  // mouse particle -> polygons
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
      // prepare polygon positions and rotations
      for (uint32_t i=0; i<polygon_count; i++) {
        Polygon *polygon = state->polygons[i];
        polygon->prev_pos = polygon->pos;
        polygon->rot = fmodf(polygon->rot, TAU);
        polygon->prev_rot = polygon->rot;
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
        for (uint32_t i=0; i<polygon_count; i++) {
          Polygon *polygon = state->polygons[i];

          uint32_t polygon_particle_count = sb_count(polygon->points);
          vec2 gravity(0.0);
          for (uint32_t i=0; i<polygon_particle_count; i++) {
            vec2 pos = rotate(polygon->points[i], polygon->rot) + polygon->pos;
            vec2 delta = substep_dt * GRAVITY;
            polygon->velocity += delta;
          }

          polygon->pos += polygon->velocity * substep_dt;
          polygon->rot += polygon->angular_velocity * substep_dt;
        }
      }

      // polygon vs particle collisions
      {
        for (uint32_t particle_idx=0; particle_idx<particle_count; particle_idx++) {
          for (uint32_t polygon_idx=0; polygon_idx<polygon_count; polygon_idx++) {
            state->next_positions[particle_idx] = projectSDFConstraint(
              state->positions[particle_idx],
              state->next_positions[particle_idx],
              state->polygons[polygon_idx],
              radius,
              substep_dt,
              1.0f
              //particle_idx == 0 ? 0.000001f : 1.0f
            );
          }
        }
      }

      // poly vs poly
      {
        float damp = 0.1f;
        for (uint32_t a_idx=0; a_idx<polygon_count; a_idx++) {
          Polygon *a = state->polygons[a_idx];
          uint32_t polygon_particle_count = sb_count(a->points);
          for (uint32_t b_idx=0; b_idx<polygon_count; b_idx++) {
            if (a_idx==b_idx) {
              continue;
            }

            Polygon *b = state->polygons[b_idx];

            for (uint32_t i=0; i<=polygon_particle_count; i++) {
              vec2 pos = (i==polygon_particle_count)
                ? a->pos
                : rotate(a->points[i], a->rot) + a->pos;


              vec2 prev_pos = (i==polygon_particle_count)
                ? a->prev_pos
                : rotate(a->points[i], a->prev_rot) + a->prev_pos;

              vec2 next_pos = projectSDFConstraint(prev_pos, pos, b, 1.0f, a->inv_mass);

              // vec2 next_pos = glm::clamp(pos, vec2(0.0), state->screen);

              // TODO: apply proper angular + positional corrections
              // TODO: apply to b as well
              a->pos += (next_pos - pos);
            }
          }
        }
      }

      // sdf particle vs wall
      {
        for (uint32_t polygon_idx=0; polygon_idx<polygon_count; polygon_idx++) {
          Polygon *polygon = state->polygons[polygon_idx];
          uint32_t polygon_particle_count = sb_count(polygon->points);

          for (uint32_t i=0; i<polygon_particle_count; i++) {
            vec2 pos = rotate(polygon->points[i], polygon->rot) + polygon->pos;
            vec2 delta = projectWallConstraint(pos, state->screen, 0.0);

            // TODO: apply proper angular + positional corrections

            vec2 new_poly_pos = polygon->pos - delta;

            float old_angle = angle(polygon->pos, pos);
            float new_angle = angle(polygon->pos, pos + delta);

            float w = (polygon->aabb.ub.x - polygon->aabb.lb.x);
            float angle_diff = (new_angle - old_angle);
            polygon->rot += angle_diff;// * (1.0 - distance(polygon->pos, pos) / w);
            polygon->angular_velocity += angle_diff;
            polygon->pos = new_poly_pos;
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

      // apply polygon results
      {
        for (uint32_t i=0; i<polygon_count; i++) {
          state->polygons[i]->velocity = (state->polygons[i]->pos - state->polygons[i]->prev_pos) / substep_dt;
          state->polygons[i]->angular_velocity = (state->polygons[i]->rot - state->polygons[i]->prev_rot) / substep_dt;
          state->polygons[i]->prev_pos = state->polygons[i]->pos;
          state->polygons[i]->prev_rot = state->polygons[i]->rot;
        }
      }
    }
  }

  igText("sim time: %f", (rawkit_now() - now) * 1000.0);

  // render polygons
  {
    for (uint32_t i=0; i<polygon_count; i++) {
      state->polygons[i]->render(vg);
    }
  }

  // draw all of the particles
  {
    rawkit_vg_fill_color(vg, rawkit_vg_RGB(0x00, 0x00, 0xFF));
    for (uint32_t i=0; i<particle_count; i++) {
      rawkit_vg_fill_color(vg, rawkit_vg_HSL((float)i/(float)particle_count, 0.5, 0.5));

      rawkit_vg_begin_path(vg);
        rawkit_vg_arc(
          vg,
          state->positions[i].x,
          state->positions[i].y,
          radius,
          0.0,
          TAU,
          1
        );
        rawkit_vg_fill(vg);
    }
  }

  // draw the constraints as lines
  {
    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xFF, 0xFF, 0xFF));
    for (uint32_t i=0; i<constraint_count; i++) {
      Constraint *c = &state->constraints[i];

      vec2 a = state->positions[c->a];
      vec2 b = state->positions[c->b];
      rawkit_vg_begin_path(vg);
        rawkit_vg_move_to(vg, a.x, a.y);
        rawkit_vg_line_to(vg, b.x, b.y);
        rawkit_vg_stroke(vg);
    }
  }
}