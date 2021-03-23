#include <rawkit/core.h>

#include <string>
using namespace std;

#include "stb_sb.h"
#include <stdarg.h>

bool _rawkit_resource_sources(rawkit_resource_t *res, uint32_t source_count, ...) {
  if (!res) {
    return false;
  }

  rawkit_resource_t *sources[256] = {};

  va_list vl;
  va_start(vl, source_count);
  for (uint32_t i=0; i<source_count; i++) {
    sources[i] = va_arg(vl, rawkit_resource_t *);
  }

  va_end(vl);

  return rawkit_resource_sources_array(res, source_count, sources);
}

bool rawkit_resource_sources_array(rawkit_resource_t *res, uint32_t source_count, rawkit_resource_t **sources) {
  bool dirty = false;

  uint32_t l = sb_count(res->resource_source_refs);
  // Dirty when the source counts do not match
  if (source_count != l) {
    if (l) {
      stb__sbraw(res->resource_source_refs)[1] = 0;
    }

    for (uint32_t i=0; i<source_count; i++) {
      rawkit_resource_t *source = sources[i];
      rawkit_resource_ref_t ref = {};

      if (!source || source == res) {
        sb_push(res->resource_source_refs, ref);
        continue;
      }

      if (source->resource_version != 0) {
        dirty = true;
      }

      ref.id = source->resource_id;
      ref.version = source->resource_version;
      sb_push(res->resource_source_refs, ref);
    }

    return dirty;
  }


  for (uint32_t i=0; i<source_count; i++) {
    const rawkit_resource_t *source = sources[i];
    if (!source) {
      continue;
    }

    rawkit_resource_ref_t *ref = &res->resource_source_refs[i];

    if (source->resource_id != ref->id) {
      dirty = true;
    }

    if (source->resource_version != 0 && source->resource_version != ref->version) {
      dirty = true;
    }

    // immediately clean the resource so we can avoid making the caller hoopjump
    ref->version = source->resource_version;
    ref->id  = source->resource_id;
  }

  return dirty;
}

#ifndef _WIN32
  #include <unistd.h>
  #include <errno.h>
  #include <string.h>
  #include <sys/types.h>
  #include <sys/ptrace.h>
  #include <sys/wait.h>

  #if !defined(PTRACE_ATTACH) && defined(PT_ATTACH)
    #define PTRACE_ATTACH PT_ATTACH
  #endif

  #if !defined(PTRACE_DETACH) && defined(PT_DETACH)
    #define PTRACE_DETACH PT_DETACH
  #endif

  #ifdef __linux__
    #define _PTRACE(_x, _y) ptrace(_x, _y, NULL, NULL)
  #else
    #define _PTRACE(_x, _y) ptrace(_x, _y, NULL, 0)
  #endif

  bool rawkit_is_debugger_attached() {
    int pid;
    int from_child[2] = {-1, -1};
    if (pipe(from_child) < 0) {
      fprintf(stderr, "Debugger check failed: Error opening internal pipe: %s", strerror(errno));
      return -1;
    }

    pid = fork();
    if (pid == -1) {
      fprintf(stderr, "Debugger check failed: Error forking: %s", strerror(errno));
      return -1;
    }

    if (pid == 0) {
      uint8_t ret = 0;
      int ppid = getppid();
      close(from_child[0]);

      if (_PTRACE(PTRACE_ATTACH, ppid) == 0) {
        waitpid(ppid, NULL, 0);
        write(from_child[1], &ret, sizeof(ret));
        _PTRACE(PTRACE_DETACH, ppid);
        exit(0);
      }

      ret = 1;
      write(from_child[1], &ret, sizeof(ret));
      exit(0);
    } else {
      uint8_t ret = -1;
      while ((read(from_child[0], &ret, sizeof(ret)) < 0) && (errno == EINTR));
      if (ret < 0) {
        fprintf(stderr, "Debugger check failed: Error getting status from child: %s", strerror(errno));
      }

      close(from_child[1]);
      close(from_child[0]);
      waitpid(pid, NULL, 0);
      return ret;
    }
  }
#else
  #include <windows.h>
  #include <debugapi.h>
  bool rawkit_is_debugger_attached() {
    return IsDebuggerPresent();
  }
#endif
