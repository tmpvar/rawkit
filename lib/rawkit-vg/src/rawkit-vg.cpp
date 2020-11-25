#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <rawkit/vg.h>

#include "nanovg-binding.h"

#include <string>
using namespace std;


rawkit_vg_t *rawkit_vg(
  rawkit_gpu_t *gpu,
  VkRenderPass render_pass,
  const char *name,
  rawkit_resource_t *parent
) {
  string vg_id = string("rawkit-vg://") + name;

  rawkit_vg_t* vg = nullptr;
  bool dirty = false;
  if (parent) {
    uint64_t id = rawkit_hash_resources(vg_id.c_str(), 1, (const rawkit_resource_t **)&parent);
    vg = rawkit_hot_resource_id(vg_id.c_str(), id, rawkit_vg_t);
    dirty = rawkit_resource_sources(vg, parent);
  } else {
    vg = rawkit_hot_resource(vg_id.c_str(), rawkit_vg_t);
    dirty = vg->_state == nullptr;
  }

  if (!dirty) {
    return vg;
  }

  VGState *state = new VGState();

  VKNVGCreateInfo create_info = {0};
  create_info.device = gpu->device;
  create_info.gpu = gpu->physical_device;
  create_info.renderpass = render_pass;

  state->ctx = nvgCreateVk(
    create_info,
    NVG_ANTIALIAS | NVG_STENCIL_STROKES
  );

  // setup 64x64 error image
  nvgErrorImage(state->ctx);

  vg->_state = (void *)state;

  return vg;
}


NVGpaint rawkit_vg_texture(rawkit_vg_t *vg, float cx, float cy, float w, float h, float angle, rawkit_texture_t *tex, float alpha) {
  NVGpaint p = {0};
  nvgTransformIdentity(p.xform);
  nvgTransformRotate(p.xform, angle);
  p.xform[4] = cx;
  p.xform[5] = cy;

  p.extent[0] = w;
  p.extent[1] = h;
  p.texture = tex;
  p.innerColor = p.outerColor = nvgRGBAf(1,1,1,alpha);
  return p;
}

void rawkit_vg_draw_texture(rawkit_vg_t *vg, float x, float y, float w, float h, rawkit_texture_t *tex) {
  NVGpaint paint = rawkit_vg_texture(vg, x, y, w, h, 0.0f, tex, 1.0f);

  rawkit_vg_begin_path(vg);
    rawkit_vg_rect(vg, x, y, w, h);
    rawkit_vg_fill_paint(vg, paint);
    rawkit_vg_fill(vg);
}

void rawkit_vg_draw_texture_rect(
  rawkit_vg_t *vg,
  float src_x,
  float src_y,
  float src_w,
  float src_h,

  float dest_x,
  float dest_y,

  rawkit_texture_t *tex,
  const rawkit_texture_sampler_t *sampler
) {
  const rawkit_texture_options_t *opts = &tex->options;

  NVGpaint p = {0};
  {
    float alpha = 1.0f;
    float angle = 0.0f;
    rawkit_vg_transform_identity(p.xform);
    rawkit_vg_transform_rotate(p.xform, angle);
    p.xform[4] = dest_x - src_x;
    p.xform[5] = dest_y - src_y;

    p.extent[0] = (float)opts->width;
    p.extent[1] = (float)opts->height;
    p.texture = tex;
    p.sampler = sampler;
    p.innerColor = p.outerColor = rawkit_vg_RGBAf(1,1,1,alpha);
  }


  // NVGpaint paint = rawkit_vg_texture(vg, dx, dy, src_w, src_h, 0.0f, tex, 1.0f);
  rawkit_vg_begin_path(vg);
    rawkit_vg_rect(vg, dest_x, dest_y, src_w, src_h);
    rawkit_vg_fill_paint(vg, p);
    rawkit_vg_fill(vg);
}

#include "nanovg/img/error-64x64.h"
static int g_MissingImageId = -1;
int nvgErrorImage(NVGcontext *ctx) {
  if (g_MissingImageId > -1) {
    return g_MissingImageId;
  }

  if (!ctx) {
    printf("ERROR: invalid ctx while creating error image\n");
    return -1;
  }

  g_MissingImageId = nvgCreateImageRGBA(
    ctx,
    64,
    64,
    NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY | NVG_IMAGE_NEAREST,
    error_64x64_data
  );
  return g_MissingImageId;
}