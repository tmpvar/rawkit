
#include <hot/jitjob.h>

#include <hot/guest/hot/state.h>
#include <hot/host/stdlib.h>
#include <hot/host/stdio.h>
#include <hot/host/string.h>
void host_hot_init_state(JitJob *job) {
  job->addExport("hotState", hotState);
}

void host_rawkit_serial_init(JitJob *job);

double rawkit_now() {
  return glfwGetTime();
}

void host_hot_init(JitJob *job) {
  host_hot_init_state(job);
  host_init_string(job);
  host_init_stdlib(job);
  host_init_stdio(job);

  host_rawkit_serial_init(job);

  job->addExport("rawkit_now", rawkit_now);
}