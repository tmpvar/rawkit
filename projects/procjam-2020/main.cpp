#undef GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <rawkit/rawkit.h>

#define CPU_HOST
#include "shared.h"

#include "bezier.h"

#include "perlin.h"

#include <stb_sb.h>

#define TAU 6.2831853f
#include <vector>
using namespace std;

struct segment_t {
  vec3 start;
  vec3 end;
  float start_radius;
  float end_radius;
  vec3 start_color;
  vec3 end_color;
};


vec4 sample_blue_noise(const rawkit_texture_t *tex, uint64_t loc) {
  if (!tex || !tex->resource_version || !tex->options.source || !tex->options.source->data) {
    return vec4(1.0, 0.0, 1.0, 1.0);
  }

  loc*=4;

  const rawkit_image_t *bn = tex->options.source;
  const uint8_t *data = (const uint8_t *)bn->data;
  uint64_t len = bn->width * bn->height * 4;

  return vec4(
    (float)data[(loc + 0) % len] / 255.0f,
    (float)data[(loc + 1) % len] / 255.0f,
    (float)data[(loc + 2) % len] / 255.0f,
    (float)data[(loc + 3) % len] / 255.0f
  );
}

vec3 world_dims(256.0);

typedef struct state_t {
  bool initialized;
  bool rebuild_on_save;
  int approach;
} state_t;

state_t *state = NULL;

bool rebuild_world = false;
void setup(){
  state = rawkit_hot_state("procjam-2020-state", state_t);

  if (!state->initialized) {
    state->rebuild_on_save = true;
    state->initialized = true;
  }

  rebuild_world = state->rebuild_on_save;
}

void add_sphere(
  rawkit_cpu_buffer_t *world_buffer,
  rawkit_cpu_buffer_t *world_occlusion_buffer,
  rawkit_texture_t *noise,
  vec3 pos,
  float radius,
  vec4 color
) {

  vec4 *color_data = (vec4 *)world_buffer->data;
  uint8_t *occlusion_data = (uint8_t *)world_occlusion_buffer->data;

  vec3 lb = floor(pos - radius * 1.1f);
  vec3 ub = ceil(pos + radius * 1.1f);

  vec3 p;
  for (p.x = lb.x; p.x<ub.x; p.x+=1.0f) {
    for (p.y = lb.y; p.y<ub.y; p.y+=1.0f) {
      for (p.z = lb.z; p.z<ub.z; p.z+=1.0f) {

        if (
          glm::any(glm::lessThan(p, vec3(0.0))) ||
          glm::any(glm::greaterThanEqual(p, world_dims))
        ) {
          continue;
        }

        uint64_t loc = (
          static_cast<uint64_t>(p.x) +
          static_cast<uint64_t>(p.y * world_dims.x) +
          static_cast<uint64_t>(p.z * world_dims.x * world_dims.y)
        );

        if (glm::distance(p, pos) - radius <= 0.0) {
          occlusion_data[loc] = 255;
          color_data[loc] = color;
        }

      }
    }
  }
}

void add_palm_tree(
  rawkit_cpu_buffer_t *world_buffer,
  rawkit_cpu_buffer_t *world_occlusion_buffer,
  rawkit_texture_t *noise,
  vec3 pos,
  float noise_offset,
  float color_basis
) {
  vec4 *color_data = (vec4 *)world_buffer->data;
  uint8_t *occlusion_data = (uint8_t *)world_occlusion_buffer->data;
  vec3 half = world_dims * 0.5f;
  float radius = 4.0f;
  pos.y -= radius * 2.0;


  vec3 trunk[4] = {};
  vec4 n;
  n = sample_blue_noise(noise, pos.x + pos.y + pos.z) * 2.0f - 1.0f;
  float height = fabs(n.x) * 128.0f;
  float half_height = height / 2.0f;
  {
    trunk[0] = pos + vec3(n.x * half_height, fabs(n.y) , n.z * half_height);
    n = sample_blue_noise(noise, pos.x + pos.y + pos.z + 1.0) * 2.0f - 1.0f;
    trunk[1] = trunk[0] + vec3(n.y * half_height, height / 4.0f, n.z * half_height);
    n = sample_blue_noise(noise, pos.x + pos.y + pos.z + 2.0) * 2.0f - 1.0f;
    trunk[2] = trunk[1] + vec3(n.x * height / 4.0f, height / 4.0f, n.z * height / 4.0f);
    n = sample_blue_noise(noise, pos.x + pos.y + pos.z + 3.0) * 2.0f - 1.0f;
    trunk[3] = trunk[2] + vec3(n.z * height / 8.0f, height / 4.0f, n.z * height / 8.0f);
  }

  for (float t = 0.0; t<1.0; t+=0.01) {
    vec3 pos = bezier(trunk[0], trunk[1], trunk[2], trunk[3], t);
    vec4 color = vec4(0.81, 0.66, 0.62, 1.0);

    add_sphere(
      world_buffer,
      world_occlusion_buffer,
      noise,
      pos,
      ((1.0 - t) + 0.25) * radius,
      color + (2.0f * sample_blue_noise(noise, 10.0 * t * pos.x * pos.y).r - 1.0f) * 0.3f
    );
  }

  float frond_count = 30.0f;
  for (float frond = 0.0f; frond<frond_count; frond++) {
    n = sample_blue_noise(noise, (pos.x + pos.y + pos.z) * frond) * 2.0f - 1.0f;
    vec3 p(
      n.x * height / 2.0,
      n.z * height / 4.0,
      n.y * height / 2.0
    );

    radius = 1.5f;
    for (float t = 0.0; t<1.0; t+=0.01) {
      vec3 pos = bezier(
        trunk[3],
        trunk[3] + vec3(p.x / 2.0f, fabs(p.y), p.z / 2.0f),
        trunk[3] + vec3(p.x / 2.0f, p.y / 2.0, p.z/ 2.0f),
        trunk[3] + vec3(p.x / 2.0, p.y / 2.0, p.z / 2.0),
        t
      );
      vec4 color = mix(
        vec4(0.2, 0.6, 0.0, 1.0),
        vec4(1.0, 1.0, 0.0, 1.0),
        t
      );

      add_sphere(
        world_buffer,
        world_occlusion_buffer,
        noise,
        pos,
        ((1.0 - t) + 0.125) * radius,
        color
      );
    }


  }
}

void add_rock(
  rawkit_cpu_buffer_t *world_buffer,
  rawkit_cpu_buffer_t *world_occlusion_buffer,
  rawkit_texture_t *noise,
  vec3 pos,
  vec3 dims,
  float noise_offset,
  float color_basis
) {
  vec3 half = dims * 0.5f;
  vec4 *color_data = (vec4 *)world_buffer->data;
  uint8_t *occlusion_data = (uint8_t *)world_occlusion_buffer->data;
  dims = glm::ceil(dims);

  uint64_t a = 0;
  vec3 p(0.0f);
  for (p.x=0; p.x<dims.x; p.x++) {
    for (p.y=0; p.y<dims.y; p.y++) {
      for (p.z=0; p.z<dims.z; p.z++) {
        vec3 rel = pos - half + p;

        if (
          glm::any(glm::lessThan(rel, vec3(0.0))) ||
          glm::any(glm::greaterThanEqual(rel, world_dims))
        ) {
          continue;
        }

        uint64_t loc = (
            static_cast<uint64_t>(rel.x)
          + static_cast<uint64_t>(rel.y * world_dims.x)
          + static_cast<uint64_t>(rel.z * world_dims.x * world_dims.y)
        );

        if (loc >= world_occlusion_buffer->size) {
          continue;
        }
        float dist = (
          glm::distance(p, half)
          - half.x
          + perlin2d(vec2(rel.x + noise_offset, rel.z), 0.1f, 10) * half.x
        );

        if (dist <= 0.0f) {
          occlusion_data[loc] = 255;//static_cast<uint8_t>(-dist/world_dims.x * 64.0);
          float c = color_basis + sample_blue_noise(noise, loc).r * (color_basis / 3.0);
          color_data[loc].r = c;
          color_data[loc].g = c;
          color_data[loc].b = c;
          color_data[loc].a = ROCK_ALPHA;
        } else {
          // occlusion_data[loc] = 4;//static_cast<uint8_t>(-dist/world_dims.x * 64.0);
          // color_data[loc].r = 0.0;
          // color_data[loc].g = 0.2;
          // color_data[loc].b = 0.5;
          // color_data[loc].a = WATER_ALPHA;
        }
      }
    }
  }
}

visible_voxel *visible_voxels = NULL;

inline bool read_occlusion(rawkit_cpu_buffer_t *buf, vec3 pos) {
  if (
    glm::any(glm::lessThan(pos, vec3(0.0))) ||
    glm::any(glm::greaterThanEqual(pos, world_dims))
  ) {
    return false;
  }

  uint8_t *data = (uint8_t *)buf->data;
  uint64_t loc = (
    static_cast<uint64_t>(pos.x) +
    static_cast<uint64_t>(pos.y * world_dims.x) +
    static_cast<uint64_t>(pos.z * world_dims.x * world_dims.y)
  );

  if (buf->size <= loc) {
    return false;
  }

  return data[loc] > 0;
}

void compute_visible_voxels(rawkit_cpu_buffer_t *buf) {
  if (!buf) {
    return;
  }

  if (visible_voxels) {
    stb__sbm(visible_voxels) = 0;
  }

  vec3 pos(0.0f);
  printf("compute shells ");
  for (pos.x = 0.0f; pos.x < world_dims.x; pos.x++) {
    printf("%0.2f ", pos.x);
    for (pos.y = 0.0f; pos.y < world_dims.y; pos.y++) {
      for (pos.z = 0.0f; pos.z < world_dims.z; pos.z++) {
        // skip empties
        if (!read_occlusion(buf, pos)) {
          continue;
        }

        // TODO: store this on a per face basis
        bool visible = (
          !read_occlusion(buf, pos + vec3(-1.0,  0.0,  0.0)) ||
          !read_occlusion(buf, pos + vec3( 0.0, -1.0,  0.0)) ||
          !read_occlusion(buf, pos + vec3( 0.0,  0.0, -1.0)) ||
          !read_occlusion(buf, pos + vec3( 1.0,  0.0,  0.0)) ||
          !read_occlusion(buf, pos + vec3( 0.0,  1.0,  0.0)) ||
          !read_occlusion(buf, pos + vec3( 0.0,  0.0,  1.0))
        );

        if (visible) {
          visible_voxel v = {
            .pos = uvec4(pos, 0),
            .face_color = {
              vec4(0.1, 0.1, 0.1, 1.0),
              vec4(0.1, 0.1, 0.1, 1.0),
              vec4(0.1, 0.1, 0.1, 1.0),
              vec4(0.1, 0.1, 0.1, 1.0),
              vec4(0.1, 0.1, 0.1, 1.0),
              vec4(0.1, 0.1, 0.1, 1.0),
            },
          };
          sb_push(visible_voxels, v);
        }
      }
    }
  }
  printf(" done\n");
}

void world_build(rawkit_texture_t *world_texture, rawkit_texture_t *world_occlusion_texture, rawkit_texture_t *noise) {
  double start_build = rawkit_now();
  rawkit_cpu_buffer_t *world_buffer = rawkit_cpu_buffer(
    "world-buffer",
    world_texture->options.size
  );

  rawkit_cpu_buffer_t *world_occlusion_buffer = rawkit_cpu_buffer(
    "world-occlusion-buffer",
    world_occlusion_texture->options.size
  );

  memset(world_buffer->data, 0, world_buffer->size);
  memset(world_occlusion_buffer->data, 0, world_occlusion_buffer->size);

  vec4 *color_data = (vec4 *)world_buffer->data;
  uint8_t *occlusion_data = (uint8_t *)world_occlusion_buffer->data;
  vec3 half = vec3(world_dims) / 2.0f;

  uint64_t seed = 0;
  float rock_offset = 1000.0;

  // fill all
  if (0) {
    memset(occlusion_data, 255, world_occlusion_buffer->size);
  }
  printf("filled occlusion data\n");
  // fill the ground
  if (1) {
    vec4 init = sample_blue_noise(noise, seed);
    uint32_t max_y = (init.x * world_dims.y);

    for (uint32_t y=0; y<max_y; y++) {
      for (uint32_t x=0; x<world_dims.x; x++) {
        for (uint32_t z=0; z<world_dims.z; z++) {
          vec3 p((float)x, (float)y, (float)z);

          uint64_t loc = (
            x +
            static_cast<uint64_t>(y * world_dims.x) +
            static_cast<uint64_t>(z * world_dims.x * world_dims.y)
          );

          if (occlusion_data[loc] > 0) {
            continue;
          }

          if (0) {
            if (y < 1) {
              occlusion_data[loc] = 255;
              color_data[loc].r = 0.5f;
              color_data[loc].g = 0.5f;
              color_data[loc].b = 0.5f;
              color_data[loc].a = ROCK_ALPHA;
              continue;
            }

            float radius = 6.0f;
            if (
              p.x > half.x - radius && p.x < half.x + radius &&
              p.z > half.z - radius && p.z < half.z + radius
            ) {
              occlusion_data[loc] = 255;
              color_data[loc].r = 0.5f;
              color_data[loc].g = 0.5f;
              color_data[loc].b = 0.5f;
              color_data[loc].a = ROCK_ALPHA;
              continue;
            }


            float v = perlin2d(vec2((float)x + seed, (float)z + seed), 0.004f, 10) * max_y - half.y / 2.0f;

            if (v > p.y) {
              occlusion_data[loc] = 255;
              color_data[loc].r = 0.5f;
              color_data[loc].g = 0.5f;
              color_data[loc].b = 0.5f;
              color_data[loc].a = ROCK_ALPHA;
              continue;
            }
          }

          if (1) {
            float v = perlin2d(vec2((float)x + seed, (float)z + seed), 0.004f, 10) * max_y - half.y / 2.0f;
            vec4 r = sample_blue_noise(noise, (seed + x * z));
            if (v < p.y) {
              // water
              if (y < max_y * 0.7 - half.y / 2.0f) {
                occlusion_data[loc] = 32;
                color_data[loc].r = 0.0f;
                color_data[loc].g = 0.3f;
                color_data[loc].b = 0.5f;
                color_data[loc].a = WATER_ALPHA;
                continue;
              }


              uint64_t below = (
                x +
                static_cast<uint64_t>((y - 1) * world_dims.x) +
                static_cast<uint64_t>(z * world_dims.x * world_dims.y)
              );

              if (y && occlusion_data[below] == 255) {


                if (color_data[below].a == DIRT_ALPHA && r.b > 0.999 && r.g > 0.5) {
                  add_palm_tree(
                    world_buffer,
                    world_occlusion_buffer,
                    noise,
                    p,
                    0,
                    0
                  );

                  // occlusion_data[loc] = 255;

                  // color_data[loc].r = 0.0;
                  // color_data[loc].g = 1;
                  // color_data[loc].b = 0;
                  // color_data[loc].a = PLANT_ALPHA;
                  continue;
                }
              }

              if (color_data[below].a == DIRT_ALPHA) {
                  if (r.g > 0.9990) {
                    add_rock(
                      world_buffer,
                      world_occlusion_buffer,
                      noise,
                      p,
                      vec3(r.g * 16.0f),
                      rock_offset,
                      0.5
                    );
                    rock_offset += 10.0;
                    continue;
                  }

                  if (r.g < 0.0) {
                    add_rock(
                      world_buffer,
                      world_occlusion_buffer,
                      noise,
                      p,
                      vec3(4),
                      rock_offset,
                      0.7
                    );
                    rock_offset += 10.0;
                    continue;
                  }
                }

              continue;
            }

            // sand
            if (y + perlin2d(vec2((float)x + (float)seed, (float)z + seed), 0.2f, 4) * 5.0 < max_y * 0.75) {
              occlusion_data[loc] = 255;
              vec4 r = sample_blue_noise(noise, (seed + x * z * y));
              color_data[loc].r = r.g * 0.25 + 186 / 255.0f;
              color_data[loc].g = r.g * 0.125 + 180 / 255.0f;
              color_data[loc].b = r.g * 0.125 + 95 / 255.0f;
              color_data[loc].a = DIRT_ALPHA;


              // vec4 r = sample_blue_noise(noise, (seed + x * z * y));
              // float c = 0.1 + r.y * 0.9;
              // color_data[loc] = vec4(c, c, c, DIRT_ALPHA);

              continue;
            }

            occlusion_data[loc] = 255;

            color_data[loc].r = 145 / 255.0f;
            color_data[loc].g = 107 / 255.0f;
            color_data[loc].b = 86 / 255.0f;
            color_data[loc].a = DIRT_ALPHA;
          }
        }
      }
    }
  }


  // fill the world with noise
  if (0) {
    uint64_t a = 0;
    for (uint32_t x=0; x<world_dims.x; x++) {
      for (uint32_t y=0; y<world_dims.y; y++) {
        for (uint32_t z=0; z<world_dims.z; z++) {
          vec3 p((float)x, (float)y, (float)z);

          uint64_t loc = (
            x
            + static_cast<uint64_t>(y * world_dims.x)
            + static_cast<uint64_t>(z * world_dims.x * world_dims.y)
          );

          float v = perlin2d(vec2((float)x, (float)z), 0.004f, 10) * world_dims.y;

          if (v < y) {
            continue;
          }

          occlusion_data[loc] = 255;
          color_data[loc].r = p.x / world_dims.x;
          color_data[loc].g = p.y / world_dims.y;
          color_data[loc].b = p.z / world_dims.z;
          color_data[loc].a = 1.0f;



          // float dist = glm::distance(p, half) - half.x;
          // if (dist <= 0.0f) {
          //   occlusion_data[loc] = 255;//static_cast<uint8_t>(-dist/world_dims.x * 64.0);

          //   color_data[loc].r = p.x / world_dims.x;
          //   color_data[loc].g = p.y / world_dims.y;
          //   color_data[loc].b = p.z / world_dims.z;
          //   color_data[loc].a = 1.0f;
          // } else {
          //   // occlusion_data[loc] = 0x00;
          //   // color_data[loc] = vec4(0.0);
          // }

        }
      }
    }
  }


  // fill the world with a sphere
  if (0) {
    uint64_t a = 0;
    for (uint32_t x=0; x<world_dims.x; x++) {
      for (uint32_t y=0; y<world_dims.y; y++) {
        for (uint32_t z=0; z<world_dims.z; z++) {
          vec3 p((float)x, (float)y, (float)z);
          uint64_t loc = x;
          loc += static_cast<uint64_t>(y * world_dims.x);
          loc += static_cast<uint64_t>(z * world_dims.x * world_dims.y);

          float dist = glm::distance(p, half) - half.x + perlin2d(vec2((float)x+seed*2, (float)z), 0.05f, 1) * half.y;
          if (dist <= 0.0f) {
            occlusion_data[loc] = 255;//static_cast<uint8_t>(-dist/world_dims.x * 64.0);

            color_data[loc].r = p.x / world_dims.x;
            color_data[loc].g = p.y / world_dims.y;
            color_data[loc].b = p.z / world_dims.z;
            color_data[loc].a = 1.0f;
          } else {
            // occlusion_data[loc] = 0x00;
            // color_data[loc] = vec4(0.0);
          }

        }
      }
    }
  }


  // fill the world with reflection test
  if (0) {
    uint64_t a = 0;
    for (uint32_t x=0; x<world_dims.x; x++) {
      for (uint32_t y=0; y<world_dims.y; y++) {
        for (uint32_t z=0; z<world_dims.z; z++) {
          vec3 p((float)x, (float)y, (float)z);
          uint64_t loc = x;
          loc += static_cast<uint64_t>(y * world_dims.x);
          loc += static_cast<uint64_t>(z * world_dims.x * world_dims.y);

          if (y < 4) {
            occlusion_data[loc] = 255;//static_cast<uint8_t>(-dist/world_dims.x * 64.0);

            color_data[loc].r = p.x / world_dims.x;
            color_data[loc].g = p.y / world_dims.y;
            color_data[loc].b = p.z / world_dims.z;
            color_data[loc].a = WATER_ALPHA;
            continue;
          }


          float dist = glm::distance(p, half - vec3(0.0, half.y* 0.5, 0.0)) - half.x + perlin2d(vec2((float)x+seed*2, (float)z), 0.05f, 1) * half.y;
          if (dist <= 0.0f) {
            occlusion_data[loc] = 255;//static_cast<uint8_t>(-dist/world_dims.x * 64.0);

            color_data[loc].r = p.x / world_dims.x;
            color_data[loc].g = p.y / world_dims.y;
            color_data[loc].b = p.z / world_dims.z;
            color_data[loc].a = 1.0f;
          } else {
            // occlusion_data[loc] = 0x00;
            // color_data[loc] = vec4(0.0);
          }

        }
      }
    }
  }

  // build a tree
  if (0) {

    add_palm_tree(
      world_buffer,
      world_occlusion_buffer,
      noise,
      vec3(32, 32, 32),
      0,
      0
    );

    add_palm_tree(
      world_buffer,
      world_occlusion_buffer,
      noise,
      vec3(64, 32, 64),
      0,
      0
    );

  }


  rawkit_texture_update_buffer(world_texture, world_buffer);

  world_texture->resource_version++;
  world_buffer->resource_version++;

  rawkit_texture_update_buffer(world_occlusion_texture, world_occlusion_buffer);
  world_occlusion_texture->resource_version++;
  world_occlusion_buffer->resource_version++;
  double end_build = rawkit_now();

  double start_visibility = rawkit_now();
  compute_visible_voxels(
    world_occlusion_buffer

  );
  double end_visibility = rawkit_now();


  printf("setup complete: build: %f, visibility: %f\n", end_build - start_build, end_visibility - start_visibility);
}

enum render_approach {
  APPROACH_PIXEL_TRACING,
  APPROACH_VOXEL_FACE_TRACING,
};

void loop() {
igShowDemoWindow(0);
  igBegin("config", 0, 0);
  igCheckbox("rebuild on save", &state->rebuild_on_save);
  vec2 window_dims(
    (float)rawkit_window_width(),
    (float)rawkit_window_height()
  );

  // approach options
  {
    const char *items[2] = { "Pixel Tracing", "Voxel Face Tracing" };
    igComboStr_arr("rendering approach", &state->approach, items, 2, 10);
  }

  ImVec2 size = {110, 30};
  rebuild_world = rebuild_world || igButton("rebuild world", size);
  igEnd();
  rawkit_texture_t *blue_noise = rawkit_texture("blue-noise-ldr.png");

  rawkit_texture_t *world_texture = rawkit_texture_mem(
    "world-texture",
    world_dims.x,
    world_dims.y,
    world_dims.z,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );


  rawkit_texture_t *world_occlusion_texture = rawkit_texture_mem(
    "world-occlusion-texture",
    world_dims.x,
    world_dims.y,
    world_dims.z,
    VK_FORMAT_R8_UNORM
  );


  bool rebuilt = false;
  if (blue_noise->resource_version && rebuild_world) {
    world_build(
      world_texture,
      world_occlusion_texture,
      blue_noise
    );

    rebuild_world = false;
    rebuilt = true;
    printf("visible voxel count: %llu\n", sb_count(visible_voxels));
  }


  // setup world UBO
  struct world_ubo_t world_ubo = {};
  {
    mat4 proj = glm::perspective(
      glm::radians(90.0f),
      window_dims.x/window_dims.y,
      0.1f,
      10000.0f
    );

    vec3 half = world_dims / 2.0f;
    half = vec3(0.5f);


    float dist = glm::length(half * 1.4f);
    float now = (float)rawkit_now() * .1 + 5.0;
    vec3 eye = half + vec3(
      sin(now) * dist,
      half.y * 0.125,
      cos(now) * dist
    );

    mat4 view = glm::lookAt(
      eye,
      half,
      vec3(0.0f, -1.0f, 0.0f)
    );

    world_ubo.worldToScreen = proj * view;
    world_ubo.world_dims = vec4(world_dims, 0.0f);
    world_ubo.eye = vec4(eye, 1.0f);
    world_ubo.time = (float)rawkit_now();
    world_ubo.screen_dims = vec4(
      (float)rawkit_window_width(),
      (float)rawkit_window_height(),
      0,
      0
    );
  }

  if (!blue_noise->resource_version) {
    return;
  }

  const rawkit_texture_sampler_t *nearest_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    0.0f,
    false,
    0.0f,
    false,
    VK_COMPARE_OP_NEVER,
    0,
    1,
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
    false
  );

  // Approach: per pixel raytracing of primary and secondary rays in a single fragment shader
  if (state->approach == APPROACH_PIXEL_TRACING) {
    rawkit_shader_t *world_shader = rawkit_shader(
      rawkit_file("shader/per-pixel-world.vert"),
      rawkit_file("shader/per-pixel-world.frag")
    );
    rawkit_shader_instance_t *inst = rawkit_shader_instance_create(world_shader);
    // render the world shader
    if (inst) {
      rawkit_shader_instance_begin(inst);
      rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);

      const rawkit_texture_sampler_t *linear_sampler = rawkit_texture_sampler(
        inst->gpu,
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        VK_SAMPLER_ADDRESS_MODE_REPEAT,
        0.0f,
        false,
        0.0f,
        false,
        VK_COMPARE_OP_NEVER,
        0,
        1,
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        false
      );

      rawkit_shader_instance_param_texture(inst, "world_texture", world_texture, nearest_sampler);
      rawkit_shader_instance_param_texture(inst, "world_occlusion_texture", world_occlusion_texture, nearest_sampler);
      rawkit_shader_instance_param_texture(inst, "blue_noise", blue_noise, linear_sampler);
      VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = window_dims.x,
        .height = window_dims.y,
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

      vkCmdDraw(inst->command_buffer, 36, 1, 0, 0);

      rawkit_shader_instance_end(inst);
    }
  }

  // Approach: per face raytracing w/ voxel splatting
  if (state->approach == APPROACH_VOXEL_FACE_TRACING) {

    uint64_t count = sb_count(visible_voxels);

    uint64_t total = static_cast<uint64_t>(world_dims.x * world_dims.y * world_dims.z);
    double percent = (double)count / (double)total * 100.0;

    rawkit_gpu_ssbo_t *pos_ssbo = rawkit_gpu_ssbo(
      "visible-voxel-positions",
      count * sizeof(visible_voxel)
    );

    if (rebuilt || pos_ssbo->resource_version == 0) {
      printf("rebuild ssbo\n");
      VkResult err = rawkit_gpu_ssbo_update(
        pos_ssbo,
        rawkit_vulkan_queue(),
        rawkit_vulkan_command_pool(),
        (void *)visible_voxels,
        count * sizeof(visible_voxel)
      );

      rawkit_gpu_ssbo_transition(pos_ssbo, (
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT
      ));

      if (err) {
        printf("could not update ssbo %i\n", err);
      }
      printf("done updating ssbo\n");

      pos_ssbo->resource_version++;
    }
    igText("visible voxel count: %llu/%llu (%.02f%%)",
      count,
      total,
      percent
    );

    // trace from every visible voxel's face to the sun
    if (0) {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/trace-voxel-faces.comp")
      );

      rawkit_shader_instance_t *inst = rawkit_shader_instance_create(shader);
      if (inst) {
        uint32_t visible_count = sb_count(visible_voxels);

        rawkit_shader_instance_begin(inst);
        rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);
        rawkit_shader_instance_param_texture(inst, "world_texture", world_texture, nearest_sampler);
        rawkit_shader_instance_param_texture(inst, "world_occlusion_texture", world_occlusion_texture, nearest_sampler);
        rawkit_shader_instance_param_ssbo(inst, "VisibleVoxels", pos_ssbo);

        // rawkit_gpu_ssbo_transition(pos_ssbo, (
        //   VK_ACCESS_SHADER_WRITE_BIT
        // ));

        rawkit_shader_instance_dispatch_compute(
          inst,
          visible_count,
          6,
          1
        );

        // rawkit_gpu_ssbo_transition(pos_ssbo, (
        //   VK_ACCESS_SHADER_READ_BIT
        // ));

        rawkit_shader_instance_end(inst);
      }
    }

    // render the visible voxels as cubes
    if (0) {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/per-voxel-world.vert"),
        rawkit_file("shader/per-voxel-world.frag")
      );
      // render the world shader
      rawkit_shader_instance_t *inst = rawkit_shader_instance_create(shader);
      if (inst) {
        rawkit_shader_instance_begin(inst);
        rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);
        rawkit_shader_instance_param_ssbo(inst, "VisibleVoxels", pos_ssbo);
        rawkit_shader_instance_param_texture(inst, "world_texture", world_texture, nearest_sampler);
        rawkit_shader_instance_param_texture(inst, "world_occlusion_texture", world_occlusion_texture, nearest_sampler);

        VkViewport viewport = {
          .x = 0.0f,
          .y = 0.0f,
          .width = window_dims.x,
          .height = window_dims.y,
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
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

        vkCmdDraw(inst->command_buffer, 6, count, 0, 0);

        rawkit_shader_instance_end(inst);
      }
    }

    // render the visible voxels as quads
    if (1) {
      rawkit_shader_t *shader = rawkit_shader(
        rawkit_file("shader/per-voxel-world-quad.vert"),
        rawkit_file("shader/per-voxel-world.frag")
      );
      // render the world shader
      rawkit_shader_instance_t *inst = rawkit_shader_instance_create(shader);
      if (inst) {
        rawkit_shader_instance_begin(inst);
        rawkit_shader_instance_param_ubo(inst, "UBO", &world_ubo);
        rawkit_shader_instance_param_ssbo(inst, "VisibleVoxels", pos_ssbo);
        rawkit_shader_instance_param_texture(inst, "world_texture", world_texture, nearest_sampler);
        rawkit_shader_instance_param_texture(inst, "world_occlusion_texture", world_occlusion_texture, nearest_sampler);

        VkViewport viewport = {
          .x = 0.0f,
          .y = 0.0f,
          .width = window_dims.x,
          .height = window_dims.y,
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
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

        vkCmdDraw(inst->command_buffer, 6, count, 0, 0);

        rawkit_shader_instance_end(inst);
      }
    }
  }
}