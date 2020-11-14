#include <rawkit/rawkit.h>
#include <cglm/vec2.h>
#include "stb_sb.h"

typedef struct boundary_t {
  float top;
  float bottom;
  float left;
  float right;
} boundary_t;

typedef struct int_vector_t {
  int32_t *vals;
  uint32_t maxSize;
  uint32_t size;
} int_vector_t;

typedef struct hash_t {
  uint32_t size;

  int32_t *first;
  int32_t *marks;
  uint32_t currentMark;
  int32_t *next;

  vec2 orig;
} hash_t;

void int_vector_init(int_vector_t *vec, uint32_t size) {
  if (vec->vals) {
    return;
  }

  vec->vals = (int32_t *)malloc(sizeof(int32_t) * size);
  vec->maxSize = size;
  vec->size = 0;
}

void int_vector_clear(int_vector_t *vec) {
  vec->size = 0;
}

void int_vector_push_back(int_vector_t *vec, int32_t val) {
  if (vec->size >= vec->maxSize) {
    vec->maxSize *= 2;

    int32_t *new_ptr = realloc(vec->vals, sizeof(int32_t) * vec->maxSize);
    if (!new_ptr) {
      printf("ERROR: could not realloc vector!\n");
      exit(1);
    }
  }

  vec->vals[vec->size++] = val;
}

typedef struct state_t {
  float lastTime;

  bool paused;
  bool sand;

  uint32_t numParticles;
  float drawScale;

	float gravity;
	float particleRadius;
	bool unilateral;
	float viscosity;

	uint32_t numIters;
	uint32_t numSubSteps;

	uint32_t maxParticles;

  vec2 fluidOrig;

  boundary_t *boundaries;

  float *velocities;
  float *positions;
  float *prev_positions;

  // derived state
	float particleDiameter;
	float restDensity;
	float kernelRadius;
	float h2;
	float kernelScale;
	// 2d poly6 (SPH based shallow water simulation

	float gridSpacing;
	float invGridSpacing;

	float maxVel;

  // simulation scratch buffer
  float *grads;

  // nearest neighbor / spacial hash scratch buffers
  int32_t *firstNeighbor;

  int_vector_t neighbors;


  // hash
  uint32_t hashSize;
  hash_t hash;

} state_t;

state_t *state = NULL;


void init_state(bool reset) {
  state = rawkit_hot_state("fluid2d state", state_t);

  if (reset || !state->numParticles) {
    state->maxParticles = 40000;
    state->drawScale = 300.0f;
    state->gravity = -10.0f;
    state->particleRadius = 0.01f;
    state->unilateral = true;
    state->viscosity = 0.0;

    state->numSubSteps = 10;
    state->numIters = 1;

    state->fluidOrig[0] = 0.0;
    state->fluidOrig[1] =  1.3;

    int_vector_init(&state->neighbors, 10 * state->maxParticles);

    if (reset) {
      sb_free(state->boundaries);
      state->boundaries = NULL;
    }

    state->particleDiameter = 2 * state->particleRadius;
    state->restDensity = 1.0 / (state->particleDiameter * state->particleDiameter);
    state->kernelRadius = 3.0 * state->particleRadius;
    state->h2 = state->kernelRadius * state->kernelRadius;
    state->kernelScale = 4.0 / (GLM_PI * state->h2 * state->h2 * state->h2 * state->h2);
    // 2d poly6 (SPH based shallow water simulation

    state->gridSpacing = state->kernelRadius * 1.5;
    state->invGridSpacing = 1.0 / state->gridSpacing;

    state->maxVel = 0.4 * state->particleRadius;
  }

  uint32_t len = (state->maxParticles * 2) * 2;

  if (!state->positions) {
    state->positions = (float *)calloc(sizeof(float) * len, 1);
  }

  if (!state->prev_positions) {
    state->prev_positions = (float *)calloc(sizeof(float) * len, 1);
  }

  if (!state->velocities) {
    state->velocities = (float *)calloc(sizeof(float) * len, 1);
  }

  if (!state->grads) {
    // NOTE: in the original this was locked to 1000
    state->grads = (float *)calloc(sizeof(float) * len, 1);
  }

  if (!state->firstNeighbor) {
    state->firstNeighbor = (int32_t *)calloc(sizeof(float) * len, 1);
  }

  if (!state->boundaries) {
    float width = 1.0f;
	  float height = 2.0;

    {
      boundary_t boundary = {
        .left = -width * 0.5f - 0.1f,
        .right = -width * 0.5f,
        .bottom = 0.0f,
        .top = height
      };
      sb_push(state->boundaries, boundary);
    }

    {
      boundary_t boundary = {
        .left = width * 0.25,
        .right = width * 0.25 + 0.1,
        .bottom = 0.05f,
        .top = height
      };
      sb_push(state->boundaries, boundary);
    }

    {
      boundary_t boundary = {
        .left = 1 + width * 0.25,
        .right = 1 +width * 0.75 + 0.1,
        .bottom = 0.0f,
        .top = height / 10.0f
      };
      sb_push(state->boundaries, boundary);
    }
  }

  if (!state->hashSize) {
    state->hashSize = 370111;
    state->hash.size = state->hashSize;

    state->hash.first = (int32_t *)calloc(sizeof(int32_t) * state->hashSize*state->maxParticles, 1),
    state->hash.marks = (int32_t *)calloc(sizeof(int32_t) * state->hashSize*state->maxParticles, 1),
    state->hash.next = (int32_t *)calloc(sizeof(int32_t) * len, 1),
    state->hash.currentMark = 0,
    state->hash.orig[0] = -100.0f;
    state->hash.orig[1] = -1.0f;
  }
}

void reset(uint32_t numX, uint32_t numY) {
  if (numX * numY > state->maxParticles) {
    return;
  }
  state->numParticles = numX * numY;

  uint32_t nr = 0;
  for (uint32_t j = 0; j < numY; j++) {
    for (uint32_t i = 0; i < numX; i++) {
      state->positions[nr] = state->fluidOrig[0] + i * state->particleDiameter + 0.00001 * (float)(j % 2);
      state->positions[nr + 1] = state->fluidOrig[1] + (float)j * state->particleDiameter;
      state->velocities[nr] = 100.0;
      state->velocities[nr + 1] = -10000.0;
      nr += 2;
    }
  }

  for (uint32_t i = 0; i < state->hashSize; i++) {
    state->hash.first[i] = -1;
    state->hash.marks[i] = 0;
  }
}

void findNeighbors() {
  // hash particles
  state->hash.currentMark++;
  for (uint32_t i = 0; i < state->numParticles; i++) {
    float px = state->positions[2 * i];
    float py = state->positions[2 * i + 1];

    int32_t gx = (int32_t)floorf((px - state->hash.orig[0]) * state->invGridSpacing);
    int32_t gy = (int32_t)floorf((py - state->hash.orig[1]) * state->invGridSpacing);

    int32_t h = (abs((gx * 92837111) ^ (gy * 689287499))) % state->hash.size;

    if (state->hash.marks[h] != state->hash.currentMark) {
      state->hash.marks[h] = state->hash.currentMark;
      state->hash.first[h] = -1;
    }

    state->hash.next[i] = state-> hash.first[h];
    state->hash.first[h] = i;
  }
  // collect neighbors

  int_vector_clear(&state->neighbors);

  float h2 = glm_pow2(state->gridSpacing);

  for (uint32_t i = 0; i < state->numParticles; i++) {
    state->firstNeighbor[i] = state->neighbors.size;

    float px = state->positions[2 * i];
    float py = state->positions[2 * i + 1];

    int32_t gx = (int32_t)floorf((px - state->hash.orig[0]) * state->invGridSpacing);
    int32_t gy = (int32_t)floorf((py - state->hash.orig[1]) * state->invGridSpacing);

    for (int32_t x = gx - 1; x <= gx + 1; x++) {
      for (int32_t y = gy - 1; y <= gy + 1; y++) {

        int32_t h = (abs((x * 92837111) ^ (y * 689287499))) % state->hash.size;

        if (state->hash.marks[h] != state->hash.currentMark) {
          continue;
        }
        int32_t id = state->hash.first[h];
        while (id >= 0) {
          float dx = state->positions[2 * id] - px;
          float dy = state->positions[2 * id + 1] - py;

          if (dx * dx + dy * dy < h2) {
            int_vector_push_back(&state->neighbors, id);
          }

          id = state->hash.next[id];
        }
      }
    }
  }
  state->firstNeighbor[state->numParticles] = state->neighbors.size;
}

void solveFluid() {
  float h = state->kernelRadius;
  float h2 = h * h;
  float avgRho = 0.0f;

  for (uint32_t i = 0; i < state->numParticles; i++) {

    float px = state->positions[2 * i];
    float py = state->positions[2 * i + 1];

    int32_t first = state->firstNeighbor[i];
    int32_t num = state->firstNeighbor[i + 1] - first;

    float rho = 0.0f;
    float sumGrad2 = 0.0f;

    float gradix = 0.0f;
    float gradiy = 0.0f;

    for (uint32_t j = 0; j < num; j++) {

      int32_t id = state->neighbors.vals[first + j];
      float nx = state->positions[2 * id] - px;
      float ny = state->positions[2 * id + 1] - py;
      float r = sqrt(nx * nx + ny * ny);

      if (r > 0) {
        nx /= r;
        ny /= r;
      }

      if (state->sand) {
        if (r < 2.0f * state->particleRadius) {
          float d = 0.5f * (2.0f * state->particleRadius - r);
          state->positions[2 * i] -= nx * d;
          state->positions[2 * i + 1] -= ny * d;
          state->positions[2 * id] += nx * d;
          state->positions[2 * id + 1] += ny * d;
          /*
          var tx = ny;
          var ty = -nx;
          var vx0 = state->positions[2 * id] - paricles.prev[2 * id];
          var vy0 = state->positions[2 * id + 1] - paricles.prev[2 * id + 1];
          */
        }
        continue;
      }

      if (r > h) {
        state->grads[2 * j] = 0.0;
        state->grads[2 * j + 1] = 0.0;
      }
      else {
        float r2 = r * r;
        float w = (h2 - r2);
        rho += state->kernelScale * w * w * w;
        float grad = (state->kernelScale * 3.0 * w * w * (-2.0 * r)) / state->restDensity;
        state->grads[2 * j] = nx * grad;
        state->grads[2 * j + 1] = ny * grad;
        gradix -= nx * grad;
        gradiy -= ny * grad;
        sumGrad2 += grad * grad;
      }
    }
    sumGrad2 += (gradix * gradix + gradiy * gradiy);

    avgRho += rho;

    float C = rho / state->restDensity - 1.0f;
    if (state->unilateral && C < 0.0f)
      continue;

    float lambda = -C / (sumGrad2 + 0.0001f);

    for (uint32_t j = 0; j < num; j++) {

      int32_t id = state->neighbors.vals[first + j];
      if (id == i) {
        state->positions[2 * id] += lambda * gradix;
        state->positions[2 * id + 1] += lambda * gradiy;

      }
      else {
        state->positions[2 * id] += lambda * state->grads[2 * j];
        state->positions[2 * id + 1] += lambda * state->grads[2 * j + 1];
      }
    }
  }
}

void solveBoundaries() {
  float canvas_width = (float)rawkit_window_width();
	float minX = canvas_width * 0.5 / state->drawScale;

  float r = state->particleRadius;
  for (uint32_t i = 0; i < state->numParticles; i++) {
    float px = state->positions[2 * i];
    float py = state->positions[2 * i + 1];

    if (py - r < 0.0) {		// ground
      state->positions[2 * i + 1] = r;
    }

    if (px - r < -minX) {
      state->positions[2 * i] = -minX + r;
    }

    if (px + r > minX) {
      state->positions[2 * i] = minX - r;
    }
    uint32_t boundaries_length = sb_count(state->boundaries);
    for (uint32_t j = 0; j < boundaries_length; j++) {
      boundary_t *b = &state->boundaries[j];
      if (px + r < b->left || px - r > b->right || py + r < b->bottom || py - r > b->top) {
        continue;
      }

      float dx;
      if (px < (b->left + b->right) * 0.5) {
        dx = b->left - px - r;
      } else {
        dx = b->right - px + r;
      }

      float dy;
      if (py + r < (b->bottom + b->top) * 0.5) {
        dy = b->bottom - (py + r);
      } else {
        dy = b->top - (py - r);
      }

      if (fabsf(dx) < fabsf(dy)) {
        state->positions[2 * i] += dx;
      } else {
        state->positions[2 * i + 1] += dy;
      }
    }
  }
}

void applyViscosity(uint32_t pnr, float dt) {
		uint32_t first = state->firstNeighbor[pnr];
		uint32_t num = state->firstNeighbor[pnr + 1] - first;

		if (num == 0) {
			return;
    }

		float avgVelX = 0.0;
		float avgVelY = 0.0;

		for (uint32_t j = 0; j < num; j++) {
			uint32_t id = state->neighbors.vals[first + j];
			avgVelX += state->velocities[2 * id];
			avgVelY += state->velocities[2 * id + 1];
		}

		avgVelX /= num;
		avgVelY /= num;

		float deltaX = avgVelX - state->velocities[2 * pnr];
		float deltaY = avgVelY - state->velocities[2 * pnr + 1];

		state->velocities[2 * pnr] += state->viscosity * deltaX;
		state->velocities[2 * pnr + 1] += state->viscosity * deltaY;
	}

void simulate(float dt) {
  dt /= (float)state->numSubSteps;
  findNeighbors();

  for (uint32_t step = 0; step < state->numSubSteps; step ++) {
    // predict
    for (uint32_t i = 0; i < state->numParticles; i++) {
      state->velocities[2 * i + 1] += state->gravity * dt;
      state->prev_positions[2 * i] = state->positions[2 * i];
      state->prev_positions[2 * i + 1] = state->positions[2 * i + 1];
      state->positions[2 * i] += state->velocities[2 * i] * dt;
      state->positions[2 * i + 1] += state->velocities[2 * i + 1] * dt;
    }

    // solve

    solveBoundaries();
    solveFluid();

    // derive velocities

    for (uint32_t i = 0; i < state->numParticles; i++) {
      float vx = state->positions[2 * i] - state->prev_positions[2 * i];
      float vy = state->positions[2 * i + 1] - state->prev_positions[2 * i + 1];

      // CFL

      float v = sqrt(vx * vx + vy * vy);
      if (v > state->maxVel) {
        vx *= state->maxVel / v;
        vy *= state->maxVel / v;
        state->positions[2 * i] = state->prev_positions[2 * i] + vx;
        state->positions[2 * i + 1] = state->prev_positions[2 * i + 1] + vy;
      }
      state->velocities[2 * i] = vx / dt;
      state->velocities[2 * i + 1] = vy / dt;

      applyViscosity(i, dt);
    }
  }
}


void render_state() {

}

void draw() {
  float canvas_width = (float)rawkit_window_width();
  float canvas_height = (float)rawkit_window_height();
  float center_x = canvas_width / 2.0f;
  float center_y = canvas_height * 0.95;

  rawkit_vg_t *vg = rawkit_vg_default();

  uint32_t nr = 0;
  for (uint32_t i = 0; i < state->numParticles; i++) {
    // if (((i / 1000) % 2) == 0) {
    //   rawkit_vg_fill_color(vg, rawkit_vg_RGB(0x00, 0x00, 0xFF));
    // } else {
    //   rawkit_vg_fill_color(vg, rawkit_vg_RGB(0xFF, 0x00, 0x00));
    // }

    rawkit_vg_fill_color(vg, rawkit_vg_HSL((float)i/(float)state->numParticles, 0.75, 0.5));
    float px = center_x + state->positions[nr] * state->drawScale;
    float py = center_y - state->positions[nr + 1] * state->drawScale;

    nr += 2;

    rawkit_vg_begin_path(vg);
    rawkit_vg_arc(
      vg,
      px,
      py,
      state->particleRadius * state->drawScale,
      0,
      GLM_PI*2.0f,
      1
    );
    rawkit_vg_close_path(vg);
    rawkit_vg_fill(vg);
  }
  // // boundaries
  rawkit_vg_stroke_color(vg, rawkit_vg_RGB(0xEE, 0xEE, 0xEE));
  uint32_t boundary_count = sb_count(state->boundaries);

  for (uint8_t i = 0; i < boundary_count; i++) {
    boundary_t *b = &state->boundaries[i];
    float left = center_x + b->left * state->drawScale;
    float width = (b->right - b->left) * state->drawScale;
    float top = center_y - b->top * state->drawScale;
    float height = (b->top - b->bottom) * state->drawScale;
    rawkit_vg_begin_path(vg);
    rawkit_vg_rect(vg, left, top, width, height);
    rawkit_vg_stroke(vg);
  }

  rawkit_vg_stroke_width(vg, 1.0f);
  rawkit_vg_begin_path(vg);
  rawkit_vg_move_to(vg, 0.0f, center_y);
  rawkit_vg_line_to(vg, (float)rawkit_window_width(), center_y);
  rawkit_vg_stroke(vg);
}


void setup() {
  init_state(true);
  reset(15, 100);
}

void loop() {
  igShowDemoWindow(0);

  // step
  {
    double startTime = rawkit_now();
    if (state->lastTime != 0.0 && !state->paused) {
      // TODO: this causes the fluid to boil!
      // simulate((startTime - state->lastTime));

      simulate(0.01);

    }
    double endTime = rawkit_now();
    state->lastTime = endTime;

    igText("%f", endTime - startTime);
  }

  draw();

  render_state();
}