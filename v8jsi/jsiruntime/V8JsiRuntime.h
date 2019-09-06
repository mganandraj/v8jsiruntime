#pragma once

#include <jsi/ScriptStore.h>
#include <jsi/jsi.h>
#include <memory>

namespace v8 {
template <class T>
class Local;
class Context;
class Platform;
class Isolate;
class TaskRunner;
} // namespace v8

namespace v8runtime {

struct JSITask {
  virtual ~JSITask() = default;
  virtual void run() = 0;
};

struct JSIIdleTask {
  virtual ~JSIIdleTask() = default;
  virtual void run(double deadline_in_seconds) = 0;
};

struct JSITaskRunner {
  virtual void postTask(std::unique_ptr<JSITask> task) = 0;
  virtual void postDelayedTask(
      std::unique_ptr<JSITask> task,
      double delay_in_seconds) = 0;
  virtual void postIdleTask(std::unique_ptr<JSIIdleTask> task) = 0;
  virtual bool IdleTasksEnabled() = 0;
};

class V8Platform;

// TODO :: This needs to be refactored away from this interface.
struct InspectorInterface {
  // This will start the server and listen for connections .. Don't create
  // instance if debugger is not needed. Creating multiple inspectors in the
  // same process is untested and not supported as of now.
  static std::unique_ptr<InspectorInterface> create(
      v8::Platform *platform,
      int port);

  // We have an overly simplified model to start with. i.e. we support only one
  // isolate+context for debugging. This must be called from inside the isolate
  // and context scope to be debugged.
  virtual void initialize(
      v8::Isolate *isolate,
      v8::Local<v8::Context> context,
      const char *context_name /*must be null terminated*/) = 0;

  virtual void waitForDebugger() = 0;
};

enum class LogLevel {
  Trace = 0,
  Info = 1,
  Warning = 2,
  Error = 3,
  Fatal = 4,
};

using Logger = std::function<void(const char *message, LogLevel logLevel)>;

struct V8RuntimeArgs {
  const v8::Platform *platform;
  std::shared_ptr<Logger> logger;

  std::unique_ptr<JSITaskRunner>
      foreground_task_runner; // foreground === js_thread => sequential

  // Sorry, currently we don't support providing custom background runner. We
  // create a default one shared by all runtimes. std::unique_ptr<TaskRunner>
  // background_task_runner; // background thread pool => non sequential

  std::unique_ptr<InspectorInterface> inspector = nullptr;

  std::unique_ptr<const facebook::jsi::Buffer> custom_snapshot_blob;

  std::unique_ptr<facebook::jsi::ScriptStore> scriptStore;
  std::unique_ptr<facebook::jsi::PreparedScriptStore> preparedScriptStore;

  bool liteMode{false};

  bool trackGCObjectStats{false};

  bool backgroundMode{false};

  bool enableTracing{false};
  bool enableJitTracing{false};
  bool enableMessageTracing{true};
  bool enableLog{true};

  size_t initial_heap_size_in_bytes{0};
  size_t maximum_heap_size_in_bytes{0};
};

__declspec(dllexport)
    std::unique_ptr<facebook::jsi::Runtime> __cdecl makeV8Runtime(
        V8RuntimeArgs &&args);

// TODO :: Following should go to a more private header

constexpr int ISOLATE_DATA_SLOT = 0;

// Platform needs to map every isolate to this data.
struct IsolateData {
  std::shared_ptr<v8::TaskRunner> foreground_task_runner_;

  // Weak reference.
  void *runtime_;
};

} // namespace v8runtime