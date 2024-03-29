
#pragma once
#include <rawkit/jit.h>

#include <hot/guest/hot/state.h>
#if !defined(__linux__)
  #include <hot/host/stdlib.h>
#endif
#include <hot/host/stdio.h>
#include <hot/host/string.h>
#include <hot/host/cimgui.h>
#include <hot/host/croaring.h>
#include <hot/host/pull-stream.h>
#include <hot/host/rawkit-core.h>
#include <hot/host/rawkit-glsl.h>
#include <hot/host/rawkit-gpu.h>
#include <hot/host/rawkit-hash.h>
#include <hot/host/rawkit-hot.h>
#include <hot/host/rawkit-file.h>
#include <hot/host/rawkit-diskwatcher.h>
#include <hot/host/rawkit-shader.h>
#include <hot/host/rawkit-texture.h>
#include <hot/host/rawkit-image.h>
#include <hot/host/rawkit-jit.h>
#include <hot/host/rawkit-mesh.h>
#include <hot/host/lz4.h>
#include <hot/host/hidapi.h>
#include <hot/host/uv.h>
#include <hot/host/rawkit-worker.h>

static void host_hot_init_state(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "hotState", (void *)&hotState);
}

#if !defined(RAWKIT_EXPORT_FILTER_WORKER)
  #include <hot/host/tinyfiledialogs.h>
  #include <hot/host/glfw.h>
  #include <hot/host/rawkit-vg.h>
  #include <hot/host/rawkit-window.h>

  void host_rawkit_serial_init(rawkit_jit_t *jit);

  void host_hot_init(rawkit_jit_t *jit) {
    host_hot_init_state(jit);
    // host_init_string(jit);

    // host_init_stdio(jit);
    host_init_tinyfiledialogs(jit);
    host_cimgui_init(jit);

    host_rawkit_serial_init(jit);

    //#if !defined(_WIN32)
    host_croaring_init(jit);
    //#endif

    // #if !defined(__linux__)
    //   host_init_stdlib(jit);
    // #endif
    host_glfw_init(jit);
    host_init_pull_stream(jit);

    host_init_rawkit_core(jit);
    host_init_rawkit_glsl(jit);
    host_init_rawkit_gpu(jit);
    host_init_rawkit_hash(jit);
    host_init_rawkit_hot(jit);
    host_init_rawkit_file(jit);
    host_init_rawkit_diskwatcher(jit);
    host_init_rawkit_shader(jit);
    host_init_rawkit_texture(jit);
    host_init_rawkit_image(jit);
    host_init_rawkit_jit(jit);
    host_init_rawkit_mesh(jit);
    host_init_rawkit_vg(jit);
    host_init_rawkit_window(jit);
    host_init_rawkit_worker(jit);
    host_init_uv(jit);
    host_init_lz4(jit);
    host_init_hidapi(jit);
  }
#endif

void worker_hot_init(rawkit_jit_t *jit) {

  host_hot_init_state(jit);
  host_rawkit_serial_init(jit);
  host_croaring_init(jit);
  host_init_pull_stream(jit);

  host_init_rawkit_core(jit);
  host_init_rawkit_glsl(jit);
  host_init_rawkit_gpu(jit);
  host_init_rawkit_hash(jit);
  host_init_rawkit_hot(jit);
  host_init_rawkit_file(jit);
  host_init_rawkit_diskwatcher(jit);
  host_init_rawkit_shader(jit);
  host_init_rawkit_texture(jit);
  host_init_rawkit_image(jit);
  host_init_rawkit_jit(jit);
  host_init_rawkit_mesh(jit);

  host_init_rawkit_worker(jit);

  host_init_uv(jit);
  host_init_lz4(jit);
  host_init_hidapi(jit);
}
