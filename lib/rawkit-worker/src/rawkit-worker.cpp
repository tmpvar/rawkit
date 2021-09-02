#include <rawkit/worker.h>
#include <rawkit-jit-internal.h>

#define RAWKIT_EXPORT_FILTER_WORKER
#include <hot/host/hot.h>

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <concurrentqeueue.h>

#include <thread>
#include <atomic>
#include <chrono>
using namespace std;

struct WorkerState {
  using PayloadType = void *;

  struct ThreadWrap {
    atomic<bool> complete;
    thread *t = nullptr;
    ThreadWrap(WorkerState *state) {
      complete.store(false);
      t = new thread(WorkerState::thread_tick, state, this);
    }
    ~ThreadWrap() {
      if (t) {
        if (t->joinable()) {
          complete.store(true);
          t->join();
        }
        delete t;
        t = nullptr;
      }
    }
  };

  moodycamel::ConcurrentQueue<PayloadType> host_rx;
  moodycamel::ConcurrentQueue<PayloadType> host_tx;

  static void thread_tick(WorkerState *worker_state, ThreadWrap *thread_wrap) {
    while(!thread_wrap->complete.load()) {
      rawkit_jit_tick_status result = rawkit_jit_tick(worker_state->jit);

      if (result == RAWKIT_JIT_TICK_ERROR) {
        uint32_t i = 0;
        rawkit_jit_message_t msg;
        while (rawkit_jit_get_message(worker_state->jit, i, &msg)) {
          if (i == 0) {
            fprintf(stderr, "program compilation issues\n");
          }

          const char *level_str = "<null>";
          switch (msg.level) {
            case RAWKIT_JIT_MESSAGE_LEVEL_NOTE: level_str = "note"; break;
            case RAWKIT_JIT_MESSAGE_LEVEL_WARNING: level_str = "warning"; break;
            case RAWKIT_JIT_MESSAGE_LEVEL_REMARK: level_str = "remark"; break;
            case RAWKIT_JIT_MESSAGE_LEVEL_ERROR: level_str = "error"; break;
            case RAWKIT_JIT_MESSAGE_LEVEL_FATAL: level_str = "fatal"; break;
            default:
              level_str = "none";
          }

          fprintf(stderr, "%s %s:%u:%u %s\n",
            level_str,
            msg.filename,
            msg.line,
            msg.column,
            msg.str
          );

          i++;
        }
      }

      if (result == RAWKIT_JIT_TICK_BUILT) {
        rawkit_jit_call_setup(worker_state->jit);
      }

      rawkit_jit_call_loop(worker_state->jit);

      std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
  }

  ThreadWrap *thread_wrap = nullptr;
  rawkit_jit_t *jit = nullptr;
  const char *full_path = nullptr;
  rawkit_worker_t *worker = nullptr;

  using GetHostWorkerFn = function<rawkit_worker_t *()>;
  GetHostWorkerFn get_host_worker_fn = nullptr;

  rawkit_worker_t *get_worker() {
    return this->worker;
  }

  char worker_host_address_define[512] = "";

  static rawkit_worker_t *static_get_host_worker() {
    printf("static works fine.....\n");
    return nullptr;
  }

  WorkerState(rawkit_worker_t *w, const char *full_path) {
    worker = w;
    this->full_path = full_path;
    jit = rawkit_jit_create(full_path);
    worker_hot_init(jit);
    rawkit_jit_add_define(jit, "-DRAWKIT_WORKER=1");



    snprintf(
      worker_host_address_define,
      sizeof(worker_host_address_define) - 1,
      "-DRAWKIT_WORKER_HOST_ADDRESS=((rawkit_worker_t *)(uintptr_t(%p)))",
      w
    );

    rawkit_jit_add_define(jit, worker_host_address_define);

    // rawkit_jit_add_export(
    //   jit,
    //   "rawkit_worker_host_2",
    //   static_get_host_worker
    // );

    // get_host_worker_fn = [this]() -> rawkit_worker_t * {
    //   return this->worker;
    // };

    // rawkit_jit_add_export(
    //   jit,
    //   "rawkit_worker_host",
    //   get_host_worker_fn
    // );

    // jit->job->addExport(
    //   "rawkit_worker_host_2",
    //   llvm::pointerToJITTargetAddress(static_get_host_worker)
    // );


    thread_wrap = new ThreadWrap(this);
  }

};

rawkit_worker_t *rawkit_worker_create_ex(const char *name, const char *file, const char *from_file) {
  fs::path full_path(file);
  if (!fs::exists(full_path)) {
    fs::path rel_dir = fs::path(from_file).remove_filename();
    full_path = fs::absolute(rel_dir / full_path);
  }

  char tmp[2048] = "\0";
  snprintf(tmp, sizeof(tmp)-1, "rawkit-worker/%s-%s", name, full_path.string().c_str());
  rawkit_worker_t *worker = rawkit_hot_resource(tmp, rawkit_worker_t);

  strncpy(
    worker->full_path,
    full_path.string().c_str(),
    sizeof(worker->full_path) - 1
  );

  if (!worker->_state) {
    WorkerState *state = new WorkerState(worker, worker->full_path);
    worker->_state = (void *)state;
  }
  return worker;
}

void rawkit_worker_send_ex(rawkit_worker_t *worker, void *data, bool worker_to_host) {
  if (!worker || !worker->_state || !data) {
    printf("rawkit_worker_send_ex worker or worker->_state or was null\n");
    return;
  }

  auto state = (WorkerState *)worker->_state;

  auto &q = worker_to_host
    ? state->host_rx
    : state->host_tx;

  q.enqueue(data);
}

void *rawkit_worker_recv_ex(rawkit_worker_t *worker, bool worker_to_host) {
  if (!worker || !worker->_state) {
    printf("rawkit_worker_recv_ex worker or worker->_state was null\n");
    return nullptr;
  }

  auto state = (WorkerState *)worker->_state;

  auto &q = worker_to_host
    ? state->host_tx
    : state->host_rx;


  void *msg = nullptr;
  if (!q.try_dequeue(msg)) {
    return nullptr;
  }

  return msg;
}


rawkit_worker_queue_status_t rawkit_worker_queue_status(rawkit_worker_t *worker) {
  if (!worker || !worker->_state) {
    return {};
  }

  auto state = (WorkerState *)worker->_state;

  rawkit_worker_queue_status_t ret = {};
  ret.rx_count = state->host_rx.size_approx();
  ret.tx_count = state->host_tx.size_approx();
  return ret;
}