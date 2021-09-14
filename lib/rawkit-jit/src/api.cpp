#include <stdlib.h>
#include <rawkit/jit.h>

#include <rawkit-jit-internal.h>

rawkit_jit_t *rawkit_jit_create(const char *file) {
  if (!file) {
    return NULL;
  }

  const char *argv[] = {
    file,
    NULL
  };

  JitJob *job = JitJob::create(1, (const char **)argv);

  if (!job) {
    return NULL;
  }

  rawkit_jit_t *jit = (rawkit_jit_t *)calloc(sizeof(rawkit_jit_t), 1);
  if (!jit) {
    return NULL;
  }

  jit->job = job;
  return jit;
}

const char *rawkit_jit_program_path(rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return "";
  }

  return jit->job->program_file.c_str();
}

rawkit_jit_status rawkit_jit_get_status(const rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return RAWKIT_JIT_STATUS_ERR;
  }

  if (!jit->job->active_runnable) {
    return RAWKIT_JIT_STATUS_ERR;
  }

  return RAWKIT_JIT_STATUS_OK;
}

void _rawkit_jit_destroy(rawkit_jit_t **jit) {
  if (!jit) {
    return;
  }

  free(*jit);
  *jit = NULL;
}

void _rawkit_jit_add_export(rawkit_jit_t *jit, const char *name, void *address) {
  if (!jit || !jit->job || !name || !address) {
    return;
  }

  jit->job->addExport(name, address);
}

void rawkit_jit_add_define(rawkit_jit_t *jit, const char *value) {
  if (!jit || !jit->job || !value) {
    return;
  }

  jit->job->addCompilerArg(value);
}

const char *rawkit_jit_get_program_path(const rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return "<invalid jit>";
  }

  return jit->job->program_file.c_str();
}

static rawkit_jit_status_callback_t global_status_callback = nullptr;
void rawkit_jit_set_global_status_callback(rawkit_jit_status_callback_t cb) {
  global_status_callback = cb;
}

rawkit_jit_tick_status rawkit_jit_tick(rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return RAWKIT_JIT_TICK_INVALID;
  }

  rawkit_jit_tick_status status = jit->job->tick();

  if (global_status_callback) {
    global_status_callback(jit, jit->job->last_build_status);
  }

  if (status == RAWKIT_JIT_TICK_BUILT) {
    jit->version++;
  }
  return status;
}

bool rawkit_jit_get_message(const rawkit_jit_t *jit, uint32_t index, rawkit_jit_message_t *ret) {
  if (!jit || !jit->job) {
    return false;
  }

  if (jit->job->messages.size() <= index) {
    return false;
  }

  const DiagnosticEntry &e = jit->job->messages[index];

  switch (e.level) {
    case clang::DiagnosticsEngine::Note: ret->level = RAWKIT_JIT_MESSAGE_LEVEL_NOTE; break;
    case clang::DiagnosticsEngine::Warning: ret->level = RAWKIT_JIT_MESSAGE_LEVEL_WARNING; break;
    case clang::DiagnosticsEngine::Remark: ret->level = RAWKIT_JIT_MESSAGE_LEVEL_REMARK; break;
    case clang::DiagnosticsEngine::Error: ret->level = RAWKIT_JIT_MESSAGE_LEVEL_ERROR; break;
    case clang::DiagnosticsEngine::Fatal: ret->level = RAWKIT_JIT_MESSAGE_LEVEL_FATAL; break;
    default:
      ret->level = RAWKIT_JIT_MESSAGE_LEVEL_NONE;
  }

  ret->line = e.line;
  ret->column = e.column;
  ret->filename = e.filename.c_str();
  ret->str = e.message.c_str();
  return true;
}


void rawkit_jit_call_setup(rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return;
  }

  jit->job->setup();
}

void rawkit_jit_call_loop(rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return;
  }

  jit->job->loop();
}

uint64_t rawkit_jit_version(const rawkit_jit_t *jit) {
  if (!jit) {
    return 0;
  }

  return jit->version;
}

static rawkit_jit_t *default_rawkit_jit = nullptr;
void rawkit_set_default_jit(rawkit_jit_t *jit) {
  default_rawkit_jit = jit;
}

rawkit_jit_t *rawkit_default_jit() {
  return default_rawkit_jit;
}

void rawkit_teardown_fn_ex(rawkit_jit_t *jit, void *user_data, rawkit_teardown_fn_t fn) {
  if (!jit || !jit->job || !fn) {
    return;
  }

  JitJob::TeardownFnWrap wrap = {};
  wrap.ptr = user_data;
  wrap.fn = fn;

  jit->job->teardown_functions.push(std::move(wrap));
}

void rawkit_jit_set_debug(rawkit_jit_t *jit, bool v) {
  if (!jit || !jit->job) {
    return;
  }

  printf("rawkit_jit_set_debug(%s)\n", v ? "true" : "false");
  jit->job->debug_build = v;

  if (v) {
    jit->job->addCompilerArg("-O0");
    jit->job->addCompilerArg("-v");
    jit->job->addCompilerArg("-g");
    jit->job->addCompilerArg("-DRAWKIT_JIT_DEBUG=1");
  } else {
    jit->job->addCompilerArg("-O3");
    jit->job->addCompilerArg("-DRAWKIT_JIT_DEBUG=0");
  }
}

uint32_t rawkit_jit_get_version(rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return 0;
  }

  return jit->job->version;
}
