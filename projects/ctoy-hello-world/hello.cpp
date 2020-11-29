/*
Much of this file has been adapted from the excellent hello world sample
from CToy (see: https://github.com/anael-seghezzi/CToy/blob/master/ressources/src/sample/hello_world.c)

CTOY
------------------------------------------------------------------------
 Copyright (c) 2015-2020 Anael Seghezzi <www.maratis3d.com>

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would
    be appreciated but is not required.

 2. Altered source versions must be plainly marked as such, and must not
    be misrepresented as being the original software.

 3. This notice may not be removed or altered from any source
    distribution.

*/

#include <stdio.h>

#include <cimgui.h>
#include <rawkit/rawkit.h>
#include <rawkit/hot.h>

#include <vulkan/vulkan.h>

typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3;

#define STAR_COUNT 32
vec3 stars[STAR_COUNT];

void star_update(void) {
  float speed = 0.14;
  int i;
  for (i = 0; i < STAR_COUNT; i++) {
    stars[i].z -= speed;
    if (stars[i].z < 0.1) {
      stars[i].x = rawkit_randf();
      stars[i].y = rawkit_randf();
      stars[i].z = rawkit_randf() * 4;
    }
  }
}

void star_draw(float *buffer, float width, float height) {
  for (int i = 0; i < STAR_COUNT; i++) {
    float fx = (stars[i].x - 0.5f) / stars[i].z;
    float fy = (stars[i].y - 0.5f) / stars[i].z;
    int ix = (fx + 0.5f) * width;
    int iy = (fy + 0.5f) * height;
    if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
      float *pixel = &buffer[(uint64_t)(iy * width + ix) * 4];
      if (pixel[0] < 0.5 && pixel[0] < 0.5 && pixel[0] < 0.5) {
        pixel[0] += 0.5;
        pixel[1] += 0.5;
        pixel[2] += 0.5;
        pixel[3] += 0.5;
      }
    }
  }
}

void image_draw(float *dest, const rawkit_image_t *src, float width, float height) {
  double t = rawkit_now() * 10;

  for (uint32_t x=0; x<(uint32_t)width; x++) {
    float fy = sinf(t + (float)x / 10.0) * 0.05;
    for (uint32_t y=0; y<(uint32_t)height; y++) {
      int32_t ly = y + fy * height;
      if (ly < 0 || ly >= (int32_t)height) {
        continue;
      }
      dest[(x + y * (uint32_t)width) * 4 + 0] = (float)src->data[(uint64_t)(x *4 + ly * width * 4) + 0] / 255.0f;
      dest[(x + y * (uint32_t)width) * 4 + 1] = (float)src->data[(uint64_t)(x *4 + ly * width * 4) + 1] / 255.0f;
      dest[(x + y * (uint32_t)width) * 4 + 2] = (float)src->data[(uint64_t)(x *4 + ly * width * 4) + 2] / 255.0f;
      dest[(x + y * (uint32_t)width) * 4 + 3] = (float)src->data[(uint64_t)(x *4 + ly * width * 4) + 3] / 255.0f;
    }
  }
}

void setup() {
  // init stars
  {
    for (int i = 0; i < STAR_COUNT; i++) {
      stars[i].x = rawkit_randf();
      stars[i].y = rawkit_randf();
      stars[i].z = rawkit_randf() * 4;
    }
  }
}

void clear(rawkit_cpu_buffer_t *buffer) {
  float *data = (float *)buffer->data;
  uint64_t l = buffer->size / 4;
  for (uint64_t i=0; i<l; i+=4) {
    data[i+0] = 0.0f;
    data[i+1] = 0.0f;
    data[i+2] = 0.0f;
    data[i+3] = 1.0f;
  }
}

void loop() {
  const rawkit_image_t *image = rawkit_image("data/hello_world.png");
  rawkit_texture_t *texture = rawkit_texture_mem(
    "output",
    (uint32_t)image->width,
    (uint32_t)image->height,
    VK_FORMAT_R32G32B32A32_SFLOAT
  );

  rawkit_cpu_buffer_t *pixel_buffer = rawkit_cpu_buffer("pixels", texture->options.size);
  float *pixels = (float *)pixel_buffer->data;

  star_update();

  // dynamically update a slab of pixels
  clear(pixel_buffer);
  image_draw(pixels, image, image->width, image->height);
  star_draw(pixels, image->width, image->height);
  rawkit_texture_update_buffer(texture, pixel_buffer);

  ImTextureID imgui_texture = rawkit_imgui_texture(
    texture,
    texture->default_sampler
  );

  if (!imgui_texture) {
    return;
  }

  bool show_demo_window = true;
  igShowDemoWindow(NULL);
  igBegin("simple window", 0, 0);
   igTextUnformatted("hello!", NULL);

   igImage(
    imgui_texture,
    { (float)texture->options.width * 4, (float)texture->options.height * 4 },
    { 0.0f, 0.0f }, // uv0
    { 1.0f, 1.0f }, // uv1
    {1.0f, 1.0f, 1.0f, 1.0f}, // tint color
    {1.0f, 1.0f, 1.0f, 1.0f} // border color
   );

  igEnd();
}
