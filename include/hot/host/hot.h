
#include <hot/jitjob.h>

#include <hot/guest/hot/state.h>

void host_hot_init_state(JitJob *job) {
  job->addExport("hotState", hotState);
}

void host_hot_init(JitJob *job) {
  host_hot_init_state(job);
}