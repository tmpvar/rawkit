#include <rawkit/rawkit.h>
#include <cglm/vec2.h>
#include <math.h>
// #include <stdint.h>

inline double fract(double v) {  return v - floor(v); }
// inline double min(double a, double b) { return a <= b ? a : b; }
// inline double max(double a, double b) { return a >= b ? a : b; }

typedef struct runner_t {
  vec2 pos;
  float health;
  int on_ground;
  float vely;
} runner_t;

runner_t runner = {
  .pos = {0.0, 24.0},
  .health = 1000.0,
  .on_ground = false,
  .vely = 0.0,
};

float animFrame = 0.0;
double last = 0.0;
double pos = 0.0;

vec2 world_res = { 320, 200 };
vec2 screen_res = { 1920, 600 };
float scale = 1.0;
float speed = 30.0f;
double startTime = 0.0f;
uint32_t seed = 2023;
float time_scale = 0.125;

const float cloud_starting_height = -100.0f;

const rawkit_texture_sampler_t *linear_sampler = NULL;
const rawkit_texture_sampler_t *nearest_sampler = NULL;
const rawkit_texture_sampler_t *nearest_repeating_sampler = NULL;

void setup() {
  startTime = rawkit_now();
  screen_res[0] = (double)rawkit_window_width();
  screen_res[1] = (double)rawkit_window_height();

  linear_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_LINEAR,
    VK_FILTER_LINEAR,
    VK_SAMPLER_MIPMAP_MODE_LINEAR,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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

  nearest_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
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


  nearest_repeating_sampler = rawkit_texture_sampler(
    rawkit_default_gpu(),
    VK_FILTER_NEAREST,
    VK_FILTER_NEAREST,
    VK_SAMPLER_MIPMAP_MODE_NEAREST,
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
}

void clear_screen(rawkit_vg_t *vg, uint32_t hex) {
  uint8_t *c = (uint8_t *)&hex;
  rawkit_vg_save(vg);
  rawkit_vg_reset_transform(vg);
  rawkit_vg_begin_path(vg);
    rawkit_vg_rect(vg, 0, 0, screen_res[0], screen_res[1]);
    rawkit_vg_fill_color(vg, rawkit_vg_RGBA(
      c[3],
      c[2],
      c[1],
      c[0]
    ));
    rawkit_vg_fill(vg);
  rawkit_vg_restore(vg);
}

float get_blue_noise(const rawkit_image_t *bn, uint32_t loc) {
  if (!bn || !bn->resource_version) {
    return 0.0;
  }
  return (float)bn->data[loc % bn->len] / 255.0f;
}

static float histogram[4096] = {0};

void render_clouds_from_noise(
  rawkit_vg_t *vg,
  rawkit_texture_t *tex,
  const rawkit_image_t *noise,
  float pos,
  float divisions,
  uint32_t noise_sample_stride,
  float height_range,
  float height_offset
) {
  if (divisions <= 0.0 || !tex->resource_version || !noise->resource_version) {
    return;
  }

  float incr = floor(world_res[0] / divisions);

  uint32_t i = 0;

  float texture_width = (float)tex->options.width;
  float texture_height = (float)tex->options.height;

  float lower = -texture_width;
  float upper = world_res[0] + texture_width;

  for (float x = lower; x <= upper; x+=incr) {
    float p = floor(pos) + x;
    float v = get_blue_noise(noise, (uint32_t)((float)i + pos) * 4  * noise_sample_stride);
    float v2 = get_blue_noise(noise, (uint32_t)((float)i + pos) * 3  * noise_sample_stride);
    float smoothX = x + fract(p) * world_res[0];

    histogram[i++] = v;//(float)i/100;

    if (v > 0.95f) {
      rawkit_vg_save(vg);
      rawkit_vg_translate(vg,
        x,
        cloud_starting_height - (texture_height * v2 - v * texture_height * 0.05) * height_range + texture_height * height_offset
      );
      rawkit_vg_scale(vg, v, v);

      rawkit_vg_draw_texture_rect(
        vg,
        0.0f,
        0.0f,
        texture_width,
        texture_height,

        0.0f,
        0.0f,
        tex,
        nearest_sampler
      );
      rawkit_vg_restore(vg);
    }
  }
  igText("noise end: %u", i);
  igPlotHistogramFloatPtr(
    "##cloud blue noise",
    histogram,
    i-1,
    0,
    "cloud blue noise",
    .0f,
    1.0f,
    (ImVec2){400.0, 50.0f},
    sizeof(float)
  );
}


void loop() {
  igText("runner: pos(%f,%f) health: %f", runner.pos[0], runner.pos[1], runner.health);
  {
    float dmin = 0.01f;
    float dmax = 2.0f;
    igSliderScalar("##time_scale", ImGuiDataType_Float, &time_scale, &dmin, &dmax, "time scale: %f", 1.0f);
  }

  // frame setup
  screen_res[0] = (double)rawkit_window_width();
  screen_res[1] = (double)rawkit_window_height();
  double now = rawkit_now();
  double deltaTime = now - last;
  last = now;
  // TODO: move this into hot state, unless we want it to restart the runner every save.
  pos += speed * deltaTime * time_scale;

  rawkit_texture_target_t *target = rawkit_texture_target_begin(
    rawkit_default_gpu(),
    "low res",
    world_res[0],
    world_res[1],
    false
  );

  // render to texture
  {
    rawkit_vg_t *vg = rawkit_default_vg();
    // rawkit_vg_t *vg = rawkit_vg_from_texture_target(target);
    // rawkit_vg_begin_frame(
    //   vg,
    //   target->command_buffer,
    //   (float)world_res[0],
    //   (float)world_res[1],
    //   1.0f
    // );
    clear_screen(vg, 0x111122FF);
    rawkit_vg_translate(vg, 0.0f, world_res[0] / 4.0f);

    // load up our hot reloadable textures
    rawkit_texture_t *level_tiles = rawkit_texture("terrain.png");
    rawkit_texture_t *knight_run = rawkit_texture("knight_run.png");
    rawkit_texture_t *tree = rawkit_texture("tree.png");
    rawkit_texture_t *tree2 = rawkit_texture("tree2.png");
    rawkit_texture_t *sky = rawkit_texture("sky.png");
    rawkit_texture_t *cloud = rawkit_texture("cloud.png");
    rawkit_texture_t *cloud2 = rawkit_texture("cloud2.png");
    rawkit_texture_t *cloud3 = rawkit_texture("cloud3.png");
    rawkit_texture_t *cloud4 = rawkit_texture("cloud4.png");

    const rawkit_image_t *blue_noise = rawkit_image("blue-noise-ldr.png");

    // draw the sky
    {
      clear_screen(vg, 0x41f6fcff);

      rawkit_vg_draw_texture_rect(
        vg,
        0.0,
        0.0f,
        (float)sky->options.width,
        (float)sky->options.height,
        0,
        -(float)sky->options.height * .8,
        sky,
        nearest_sampler
      );

      render_clouds_from_noise(
        vg,
        cloud2,
        blue_noise,
        pos / 100.0,
        100.0f,
        10.0f,
        1.0,
        -0.25
      );

      render_clouds_from_noise(
        vg,
        cloud,
        blue_noise,
        pos / 100.0f,
        100.0f,
        1.0f,
        0.2,
        0.0
      );

      render_clouds_from_noise(
        vg,
        cloud3,
        blue_noise,
        pos / 30.0f,
        100.0f,
        1.0f,
        0.2,
        0.0
      );

      render_clouds_from_noise(
        vg,
        cloud4,
        blue_noise,
        pos / 130.0f,
        100.0f,
        1.0f,
        0.1,
        0.0
      );
    }

    // draw some trees
    {
      // pine trees
      float step_size = 0.1;
      for (float vt = 0.0; vt <= 1.0; vt+=step_size) {
        uint32_t noise_loc = 0;
        for (float x = -1.0; x <= 35; x+=1) {
          double p = pos / 20.0;
          double v = get_blue_noise(blue_noise, floor(p) + (noise_loc++));
          double smoothX = floor(x*16 - fract(p) * 16);
          if (v > vt && v < vt + step_size) {
            rawkit_vg_save(vg);
            float scale = max(1.0f, v * 1.05f);
            rawkit_vg_scale(vg, scale, scale);
            rawkit_vg_translate(vg, smoothX, -76 + v * 16);
            rawkit_vg_draw_texture_rect(
              vg,
              0,
              0,
              16,
              64,
              0,
              0,
              tree2,
              nearest_sampler
            );
            rawkit_vg_restore(vg);
          }
        }
      }

      // tree
      for (float x = -1.0; x <= 20; x++) {
        double p = pos / 4.0;
        double v = get_blue_noise(blue_noise, floor(p) + x);
        double smoothX = floor(x*16 - fract(p) * 16);

        if (v < 0.1) {
          rawkit_vg_save(vg);
          float scale = max(1.0, v * .005f);
          rawkit_vg_scale(vg, scale, scale);
          rawkit_vg_draw_texture_rect(
            vg,
            0,
            0,
            32,
            64,
            smoothX,
            -32,
            tree,
            nearest_sampler
          );
          rawkit_vg_restore(vg);
        }
      }
    }

    // draw the ground
    {
      rawkit_vg_save(vg);
      rawkit_vg_translate(vg, 0.0, 16);

      // rawkit_vg_fill_color(vg, rawkit_vg_RGBA(98, 53, 48, 255));
      // rawkit_vg_begin_path(vg);
      // rawkit_vg_rect(vg, 0, 0, world_res[0], 600);
      // rawkit_vg_fill(vg);

      uint32_t i=0;
      for (float x = -8.0; x <= world_res[0]; x+=8) {

        double p = pos;
        double pv = get_blue_noise(blue_noise, (floor(pos) + (x - 1.0)/8.0) / 4.0);
        double v = get_blue_noise(blue_noise, (floor(pos) + x/8.0) / 4.0);
        double nv = get_blue_noise(blue_noise, (floor(pos) + (x + 1.0)/8.0) / 4.0);
        double smoothX = x - fract(pos) * 8;

        v /= 4;
        pv /= 4;
        nv /= 4;
        double rawv = v;

        double avg = ((
          min(max(pv, 0.1), 0.4) +
          min(max(v, 0.1), 0.4) +
          min(max(nv, 0.1), 0.4)
        ) / 3.0);

        histogram[i++] = avg;

        v = floor(v * 10.0) * 8.0;
        pv = floor(pv * 10.0) * 8.0;
        nv = floor(nv * 10.0) * 8.0;

        avg = floor(avg * 10.0) * 8.0;
        if (x == 8.0) {
          float expected_y = avg + 8.0f;

          if (expected_y > runner.pos[1]) {
            runner.health -= 0.1;
          }

          if (expected_y < runner.pos[1]) {
            runner.vely -= 0.125;
            runner.pos[1] += runner.vely;
          }

          if (expected_y >= runner.pos[1]) {
            runner.vely = 0.0;
            runner.pos[1] = expected_y;
          }

        }

        for (double y = -16.0; y < avg; y+=8.0) {
          double tileY = y;
          if (pv>y+8.0 && nv>y+8) {
            tileY = 16.0;
          }

          rawkit_vg_draw_texture_rect(
            vg,
            fmodf(floor(pos) * 8.0 + x, level_tiles->options.width),
            tileY,
            8,
            8,
            floor(smoothX+8),
            -y,
            level_tiles,
            nearest_repeating_sampler
          );
        }
      }

      igPlotHistogramFloatPtr(
        "##ground avg",
        histogram,
        i-1,
        0,
        "ground height avg",
        .0f,
        1.0f,
        (ImVec2){400.0, 50.0f},
        sizeof(float)
      );
      rawkit_vg_restore(vg);
    }

    // draw the runner
    {
      rawkit_vg_save(vg);
      rawkit_vg_translate(vg, 0.0, 16);
      float frame = floorf(pos * time_scale * speed);
      igText("runner frame: %f (%f, %f)", frame, runner.pos[0], runner.pos[1]);
      rawkit_vg_draw_texture_rect(
        vg,
        fmodf(frame * 32.0f, (float)knight_run->options.width),
        0.0f,
        32.0f,
        16.0f,
        runner.pos[0],
        -runner.pos[1],
        knight_run,
        nearest_sampler
      );
      rawkit_vg_restore(vg);
    }

    // rawkit_vg_end_frame(vg);
    // rawkit_texture_target_end(target);
  }

  // render to screen
  if (false){
    rawkit_vg_t *vg = rawkit_default_vg();

    rawkit_vg_save(vg);

    float dx = screen_res[0] - world_res[0];
    float dy = screen_res[1] - world_res[1];
    float scale = 4.0f;
    if (dx < dy) {

    } else {

    }

    rawkit_vg_scale(vg, scale, scale);

    target->color->default_sampler = nearest_sampler;
    rawkit_vg_draw_texture(
      vg,
      0,
      0,
      world_res[0],
      world_res[1],
      target->color
    );
    rawkit_vg_restore(vg);
  }
}


// RenderTexture2D target;

// typedef struct BlueNoise {
//   Image img;
//   Color *data;
// } BlueNoise;

// BlueNoise blue_noise = {0};


// int setup() {
//   InitWindow((int)screen_res.x, (int)screen_res.y, "pixel runner");

//   blue_noise.img = LoadImage("blue-noise-ldr.png");
//   blue_noise.data = GetImageData(blue_noise.img);
//   target = LoadRenderTexture(world_res.x, world_res.y);
//   return 0;
// }

// Rectangle tile(float x, float y) {
//   return (Rectangle){
//     .x = x*8.0f,
//     .y = y*8.0f,
//     .width = 8.0f,
//     .height = -8.0f,
//   };
// }

// double get_blue_noise(BlueNoise bn, uint32_t loc) {
//   uint32_t len = bn.img.width * bn.img.height;
//   double val = (double)bn.data[(loc * seed) % len].r / 255.0;

//   return val;
// }



// void loop() {

//   if (runner.health < 0.0) {
//     ClearBackground(GetColor(0x111122ff));
//     DrawText("You perished!", screen_res.x / 2.0 - 200, screen_res.y / 2.0, 50, RED);
//     DrawText("press space to try again", screen_res.x / 2.0 - 160, screen_res.y / 2.0 + 50, 20, GetColor(0xCCCCCCFF));
//     if (IsKeyDown(KEY_SPACE)) {
//       runner.health = 1.0;
//       runner.pos[0] = 40.0;
//       startTime = rawkit_now();
//     } else {
//       return;
//     }
//   }

//   last = now;
//   if (IsKeyDown(KEY_SPACE) && (vely == 0.0 || runner.on_ground == true)) {
//     vely = 1.5;
//     runner.pos[0] += 4.0;
//   }
//   runner.on_ground = 0;

//   vely -= 8.0 * GetFrameTime();

//   runner.pos[0] += vely;


//       // cloud
//       for (float x = 0.0; x <= 10; x++) {
//         double p = pos / 960.0 + 32.0;
//         double v = get_blue_noise(blue_noise, floor(p) + x);
//         double smoothX = x*100 - fract(p) * 100;

//         if (v < .9) {
//           DrawTexturePro(
//             cloud2,
//             (Rectangle){0, 0, 64, -64},
//             (Rectangle){
//               .x = smoothX,
//               .y = 30 + 10 * v,
//               .width = 64,
//               .height = (double)64.0
//             },
//             (Vector2){0, 0},
//             0,
//             WHITE
//           );
//         }
//       }

//       // cloud
//       for (float x = 0.0; x <= 10; x++) {
//         double p = pos / 510.0;
//         double v = get_blue_noise(blue_noise, floor(p) + x);
//         double smoothX = x*100 - fract(p) * 100;

//         if (v < 0.5) {
//           DrawTexturePro(
//             cloud3,
//             (Rectangle){0, 0, 32, -23},
//             (Rectangle){
//               .x = smoothX,
//               .y = 50 + 10 * v,
//               .width = 32,
//               .height = (double)32.0
//             },
//             (Vector2){0, 0},
//             0,
//             WHITE
//           );
//         }
//       }

//       // tree
//       for (float x = -1.0; x <= 20; x++) {
//         double p = pos / 4.0;
//         double v = get_blue_noise(blue_noise, floor(p) + x);
//         double smoothX = x*16 - fract(p) * 16;

//         if (v > 0.5) {
//           DrawTexturePro(
//             tree,
//             (Rectangle){0, 0, 32, -64},
//             (Rectangle){
//               .x = smoothX,
//               .y = 7,
//               .width = v > 0.8 ? 32 : -32,
//               .height = (double)48.0 * v
//             },
//             (Vector2){0, 0},
//             0,
//             // WHITE
//             GetColor(0xAAAAAAFF)
//           );
//         }
//       }

//       // tree
//       for (float x = -1.0; x <= 20; x++) {
//         double p = pos / 4.0;
//         double v = get_blue_noise(blue_noise, floor(p) + x);
//         double smoothX = x*16 - fract(p) * 16;

//         if (v > 0.5) {
//           DrawTexturePro(
//             tree2,
//             (Rectangle){0, 0, 16, -64},
//             (Rectangle){
//               .x = smoothX,
//               .y = 7,
//               .width = 16,
//               .height = 64*v,
//             },
//             (Vector2){0, 0},
//             0,
//             GetColor(0xAAAAAAFF)

//           );
//         }
//       }


//       // tree
//       for (float x = -1.0; x <= 20; x++) {
//         double p = pos / 4.0;
//         double v = get_blue_noise(blue_noise, floor(p) + x);
//         double smoothX = x*16 - fract(p) * 16;

//         if (v <= 0.2) {
//           DrawTexturePro(
//             tree,
//             (Rectangle){0, 0, 32, -64},
//             (Rectangle){
//               .x = smoothX,
//               .y = 2,
//               .width = 32,
//               .height = (double)64.0
//             },
//             (Vector2){0, 0},
//             0,
//             WHITE
//           );
//         }
//       }

//       // tree
//       for (float x = -1.0; x <= 20; x++) {
//         double p = pos / 4.0;
//         double v = get_blue_noise(blue_noise, floor(p) + x);
//         double smoothX = x*16 - fract(p) * 16;

//         if (v < 0.2) {
//           DrawTexturePro(
//             tree2,
//             (Rectangle){0, 0, 16, -64},
//             (Rectangle){
//               .x = smoothX,
//               .y = 7,
//               .width = 16,
//               .height = 64+32*v,
//             },
//             (Vector2){0, 0},
//             0,
//             WHITE

//           );
//         }
//       }



//   EndTextureMode();

//   Vector2 offset = {0, -(world_res.y + 18) * scale * 0.5};
//   //offset.y = -screen_res.y * 0.5 / scale + 14 * scale;// / scale;
//   offset.x = 0;
//   offset.y = 0;
//   DrawTextureEx(target.texture, offset, 0.0f, scale, WHITE);

//   DrawText("health", 10, 10, 20, WHITE);
//   DrawRectangleLines(79, 8, 102, 22, RED);
//   DrawRectangle(80, 9, 100.0 * runner.health, 20, GetColor(0xe38dd6FF));
//   DrawRectangleLines(80, 9, 100.0 * runner.health, 20, GetColor(0xe38dd6FF));



// }

