#include <stdlib.h>
#include <rawkit/jit.h>

#include <rawkit-jit-internal.h>

typedef struct rawkit_jit_t {
  JitJob *job;
  uint64_t version;
} rawkit_jit_t;

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

rawkit_jit_status rawkit_jit_get_status(rawkit_jit_t *jit) {
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

bool rawkit_jit_tick(rawkit_jit_t *jit) {
  if (!jit || !jit->job) {
    return false;
  }

  if (jit->job->tick()) {
    jit->version++;
    return true;
  }
  return false;
}

bool rawkit_jit_get_message(const rawkit_jit_t *jit, uint32_t index, rawkit_jit_message_t *ret) {
  if (!jit || !jit->job) {
    return false;
  }

  if (jit->job->messages.size() <= index) {
    return false;
  }

  DiagnosticEntry e = jit->job->messages[index];

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
