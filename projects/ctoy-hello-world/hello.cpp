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

#include "shared-texture.h"

typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3;

#define STAR_COUNT 32
vec3 stars[STAR_COUNT];

struct rawkit_image image;
void star_init(void) {
  for (int i = 0; i < STAR_COUNT; i++) {
    stars[i].x = rawkit_randf();
    stars[i].y = rawkit_randf();
    stars[i].z = rawkit_randf() * 4;
  }
}

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

void m_image_sub_pixel(const struct rawkit_image *src, float x, float y, float *result)
{
   float *colors0, *colors1, *colors2, *colors3;
   float *src_data = (float *)src->pixels;
   int width = src->width;
   int height = src->height;
   int comp = 4;
  //  int comp = src->comp;
   int c;
   float fx, fy;
   int wm = width - 1;
   int hm = height - 1;
   int ix, iy, ix2, iy2;

   ix = (int)x;
   iy = (int)y;
   fx = x - (float)ix;
   fy = y - (float)iy;
   fx = fmax(fx, 0);
   fy = fmax(fy, 0);

   ix = fmax(fmin(ix, wm), 0);
   iy = fmax(fmin(iy, hm), 0);
   ix2 = ix + 1;
   iy2 = iy + 1;
   ix2 = fmin(ix2, wm);
   iy2 = fmin(iy2, hm);

   colors0 = src_data + (width * iy  + ix)  * comp;
   colors1 = src_data + (width * iy  + ix2) * comp;
   colors2 = src_data + (width * iy2 + ix)  * comp;
   colors3 = src_data + (width * iy2 + ix2) * comp;

   for(c = 0; c < comp; c++) {
      float A = colors0[c] + (colors2[c] - colors0[c]) * fy;
      float B = colors1[c] + (colors3[c] - colors1[c]) * fy;
      result[c] = A + (B - A) * fx;
   }
}

void image_draw(float *pixels, float width, float height) {
  float *pixel = pixels;
  double t = rawkit_now() * 100;

  float sint = sin(t * 0.05) * 0.5 + 0.5;

  for (float y = 0; y < height; y++) {
    for (float x = 0; x < width; x++) {

      float im_pixel[4];
      float r = (0.15 + sint * 0.15) * rawkit_randf();
      float fx = x;
      float fy = y + sin((t + x) * 0.1) * 4.5;

      m_image_sub_pixel(&image, fx, fy, im_pixel);

      pixel[0] = im_pixel[0] + r * 0.05;
      pixel[1] = im_pixel[1] + r * 0.05;
      pixel[2] = im_pixel[2] + r * 0.05;
      pixel[3] = im_pixel[3] + r * 0.05;

      pixel += 4;
    }
  }
}

void setup() {
  image = rawkit_load_image("data/hello_world.png");
  star_init();
}

void loop() {
  VkImage *pImage = rawkit_hot_state("pImage", VkImage);
  SharedTexture *tex = rawkit_hot_state("shared texture", SharedTexture);

  tex->init(
    (uint32_t)image.width,
    (uint32_t)image.height,
    image.channels,
    TEXTURE_COMPONENT_TYPE_F32
  );

  // dynamically update a slab of pixels
  float *pixels = (float *)tex->map();
    star_update();
    image_draw(pixels, tex->width, tex->height);
    star_draw(pixels, tex->width, tex->height);
  tex->unmap();

  bool show_demo_window = true;
  igShowDemoWindow(NULL);
  igBegin("simple window", 0, 0);
   igTextUnformatted("hello!", NULL);

   igImage(
    tex->imgui_texture,
    { (float)tex->width * 4, (float)tex->height * 4 },
    { 0.0f, 0.0f }, // uv0
    { 1.0f, 1.0f }, // uv1
    {1.0f, 1.0f, 1.0f, 1.0f}, // tint color
    {1.0f, 1.0f, 1.0f, 1.0f} // border color
   );

  igEnd();
}
