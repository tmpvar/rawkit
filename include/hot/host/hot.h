
#include <rawkit/jit.h>

#include <hot/guest/hot/state.h>
#if !defined(__linux__)
  #include <hot/host/stdlib.h>
#endif
#include <hot/host/stdio.h>
#include <hot/host/string.h>
#include <hot/host/cimgui.h>
#include <hot/host/croaring.h>
#include <hot/host/tinyfiledialogs.h>
#include <hot/host/rawkit-glsl.h>
#include <hot/host/rawkit-hash.h>
#include <hot/host/rawkit-hot.h>

void host_hot_init_state(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "hotState", (void *)&hotState);
}

void host_rawkit_serial_init(rawkit_jit_t *jit);

double rawkit_now() {
  return glfwGetTime();
}

void host_hot_init(rawkit_jit_t *jit) {
  host_hot_init_state(jit);
  host_init_string(jit);

  host_init_stdio(jit);
  host_init_tinyfiledialogs(jit);
  host_cimgui_init(jit);

  host_rawkit_serial_init(jit);

  //#if !defined(_WIN32)
  host_croaring_init(jit);
  //#endif

  #if !defined(__linux__)
    host_init_stdlib(jit);
  #endif

  host_init_rawkit_glsl(jit);
  host_init_rawkit_hash(jit);
  host_init_rawkit_hot(jit);

  rawkit_jit_add_export(jit, "rawkit_now", (void *)&rawkit_now);
}
