// implementation of Position Based Dynamics (Matthias Müller et al.)
#define NO_GDI
#include <rawkit/rawkit.h>

#include <glm/glm.hpp>
using namespace glm;

#include "polygon.h"
#include "mouse.h"
#include <stb_sb.h>

#define GRAVITY vec2(0.0, -9.8)
#define TAU 6.283185307179586
#define MAX_PARTICLES 660

struct Constraint {
  uint32_t a;
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
  if (1) {
    for (uint32_t i=0; i<MAX_PARTICLES; i++) {
      vec2 pos(
        rawkit_randf() * state->screen.x,
        (rawkit_randf() * (state->screen.y - 200.0f)) + 200.f
      );

      vec2 vel(
        (rawkit_randf() * 2.0 - 1.0) * 10.0f,
        (rawkit_randf() * 2.0 - 1.0) * 10.0f
      );


      sb_push(state->positions, pos);
      sb_push(state->velocities, vel);
    }
  } else {
      sb_push(state->positions, vec2(100.0, 0.0) + vec2(100.0, 100.0));
      sb_push(state->velocities, vec2(.0, 0.0));

      sb_push(state->positions, vec2(100.0, 0.0) + vec2(200.0, 100.0));
      sb_push(state->velocities, vec2(.0, 0.0));

      sb_push(state->positions, vec2(100.0, 0.0) + vec2(200.0, 200.0));
      sb_push(state->velocities, vec2(.0, 0.0));

      sb_push(state->positions, vec2(100.0, 0.0) + vec2(100.0, 200.0));
      sb_push(state->velocities, vec2(.0, 0.0));

      add_constraint(0, 1, 1);
      add_constraint(1, 2, 1);
      add_constraint(2, 3, 1);
      add_constraint(3, 0, 1);

      add_constraint(0, 2, 1);
      add_constraint(1, 3, 1);
  }

  // build up a floor polygon
  {
    if (!sb_count(state->polygons)) {
      Polygon *polygon = new Polygon("floor");
      polygon->append(vec2(0.0, 0.0));
      polygon->append(vec2(state->screen.x, 0.0));
      polygon->append(vec2(state->screen.x, 100.0));
      polygon->append(vec2(state->screen.x*0.05, 10.0));
      // polygon->append(vec2(state->screen.x*0.75, 20.0));
      polygon->append(vec2(0.0, 100.0));
      polygon->rebuild_sdf();
      sb_push(state->polygons, polygon);
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

void projectConstraint(vec2 *positions, Constraint constraint) {
  vec2 a = positions[constraint.a];
  vec2 b = positions[constraint.b];

  vec2 ndir = normalize(a - b);
  float d = distance(a, b);
  float diff = d - constraint.rest_length;

  if (constraint.contact && d > constraint.rest_length) {
    return;
  }

  // TODO: compute the mass ratio from another list
  float massRatio = 0.5; // invMassA / (invMassA + invMassB)

  positions[constraint.a] = a - massRatio * diff * ndir;
  positions[constraint.b] = b + massRatio * diff * ndir;
}

vec2 projectSDFConstraint(vec2 pos, Polygon *polygon, float radius) {
  float d = 0.0f;
  vec2 local_pos = pos - polygon->aabb.lb;
  if (!polygon->aabb.contains(pos)) {
    vec2 nearest = polygon->aabb.nearest(pos);
    local_pos = nearest - polygon->aabb.lb;
    d = distance(nearest, pos);
  }

  local_pos = clamp(
    local_pos,
    vec2(0.0f),
    (polygon->aabb.ub - polygon->aabb.lb) - 1.0f
  );

  d += polygon->sdf->sample_interp(local_pos);
  if (d > radius) {
    return pos;
  }

  float diff = abs(-radius +  d) ;
  vec2 ndir = normalize(polygon->sdf->calcNormal(local_pos));

  // TODO: actual mass ratio
  float massRatio = 1.0;

  return pos + massRatio * diff * ndir;// * 0.5f;
}

void loop() {
  State *state = rawkit_hot_state("state", State);
  state->mouse.tick();
  double now = rawkit_now();
  float dt = now - state->last_time;
  state->last_time = now;

  state->screen = vec2(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );

  rawkit_vg_t *vg = rawkit_default_vg();
  rawkit_vg_scale(vg, 1.0, -1.0);
  rawkit_vg_translate(vg, 0.0, -(float)rawkit_window_height());

  uint32_t particle_count = sb_count(state->positions);
  uint32_t polygon_count = sb_count(state->polygons);

  // TODO: pull these into their own lists
  float invMass = 1.0;
  float radius = 5.0;

  // integrate velocity with external forces
  {
    for (uint32_t i=0; i<particle_count; i++) {
      state->velocities[i] += dt * invMass * GRAVITY;
    }
  }

  // integrate velocity w/ position to find the next potential location
  {
    for (uint32_t i=0; i<particle_count; i++) {
      state->next_positions[i] = state->positions[i] + state->velocities[i] * dt;
    }
  }

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

  uint32_t constraint_count = sb_count(state->constraints);

  // process the constraints
  float substeps = 20.0;
  float substep_dt = dt/substeps;

  float elasticity = 0.5f;
  {

    for (float substep = 0.0; substep < substeps; substep++) {
      for (uint32_t i=0; i<constraint_count; i++) {
        projectConstraint(state->next_positions, state->constraints[i]);
        // state->positions[i] = state->next_positions[i];
      }
      // polygon collisions
      {
        for (uint32_t particle_idx=0; particle_idx<particle_count; particle_idx++) {
          for (uint32_t polygon_idx=0; polygon_idx<polygon_count; polygon_idx++) {
            state->next_positions[particle_idx] = projectSDFConstraint(
              state->next_positions[particle_idx],
              state->polygons[polygon_idx],
              radius
            );
          }
        }
      }

      // wall collisions
      // TODO: make these valid plane vs particle constraints
      /*
         pos
          |
          |
          | -> +normal
          |
          |
      */
      {
        for (uint32_t i=0; i<particle_count; i++) {
          vec2 *pos = &state->next_positions[i];
          if (pos->x - radius <= 0.0 && state->velocities[i].x < 0.0) {
            state->velocities[i] = reflect_elastic(state->velocities[i], vec3(1.0, 0.0, 0.0), elasticity);
            pos->x = radius*2.0f + -pos->x;
          }

          if (pos->x + radius >= state->screen.x && state->velocities[i].x > 0.0) {
            state->velocities[i] = reflect_elastic(state->velocities[i], vec3(-1.0, 0.0, 0.0), elasticity);
            pos->x = state->screen.x - (state->screen.x - pos->x + radius);
          }

          if (pos->y - radius <= 0.0 && state->velocities[i].y < 0.0) {
            state->velocities[i] = reflect_elastic(state->velocities[i], vec3(0.0, 1.0, 0.0), elasticity);
            pos->y = radius*2.0 + -pos->y;
          }

          if (pos->y + radius >= state->screen.y && state->velocities[i].y > 0.0) {
            state->velocities[i] = reflect_elastic(state->velocities[i], vec3(0.0, -1.0, 0.0), elasticity);
            pos->y = state->screen.y - (state->screen.y - pos->y + radius);
          }
        }
      }


    }
  }

  // apply results
  {
    for (uint32_t i=0; i<particle_count; i++) {
      state->velocities[i] = (state->next_positions[i] - state->positions[i]) / dt;
      state->positions[i] = state->next_positions[i];
    }
  }


  igText("sim time: %f", (rawkit_now() - now) * 1000.0);

  // render polygons
  {

    for (uint32_t i=0; i<polygon_count; i++) {
      state->polygons[i]->render(vg);
    }
    // if (c>0 && state->polygons[0] && state->polygons[0]->sdf) {
    //   state->polygons[0]->rebuild_sdf();
    //   state->polygons[0]->sdf->debug_dist();
    // }
  }


  // draw all of the particles
  {
    for (uint32_t i=0; i<particle_count; i++) {
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

  // debug mouse constraint projection
  {
    vec2 mouse = state->mouse.pos;
    mouse.y = state->screen.y - mouse.y;

    vec2 next_pos = projectSDFConstraint(mouse, state->polygons[0], 20);

    rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0x0, 0xFF, 0x0));
    rawkit_vg_fill_color(vg, rawkit_vg_RGB(0x0, 0xFF, 0xFF));
      rawkit_vg_begin_path(vg);
        rawkit_vg_move_to(vg, mouse.x, mouse.y);
        rawkit_vg_line_to(vg, next_pos.x, next_pos.y);
        rawkit_vg_stroke(vg);

  }
}