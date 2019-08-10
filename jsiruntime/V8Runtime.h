#pragma once

#include <memory>
#include <jsi/jsi.h>

namespace facebook { namespace react {
class MessageQueueThread;
}}

namespace v8 {
  template <class T> class Local;
  class Context;
  class Platform;
  class Isolate;
  class TaskRunner;
}

namespace v8runtime {

class Task {
public:
  virtual ~Task() = default;
  virtual void run() = 0;
};

class IdleTask {
public:
  virtual ~IdleTask() = default;
  virtual void run(double deadline_in_seconds) = 0;
};

class TaskRunner {
public:
  virtual void postTask(std::unique_ptr<Task> task) = 0;
  virtual void postDelayedTask(std::unique_ptr<Task> task, double delay_in_seconds) = 0;
  virtual void postIdleTask(std::unique_ptr<IdleTask> task) = 0;

  virtual bool IdleTasksEnabled() = 0;

  TaskRunner() = default;
  virtual ~TaskRunner() = default;

private:
  TaskRunner(const TaskRunner&) = delete;
  TaskRunner& operator=(const TaskRunner&) = delete;
};

class V8Platform;

struct InspectorInterface {

  // This will start the server and listen for connections .. Don't create instance if debugger is not needed.
  // Creating multiple inspectors in the same process is untested and not supported as of now.
  static std::unique_ptr<InspectorInterface> create(v8::Platform* platform, int port);

  // We have an overly simplified model to start with. i.e. we support only one isolate+context for debugging.
  // This must be called from inside the isolate and context scope to be debugged.
  virtual void initialize(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* context_name /*must be null terminated*/) = 0;

  virtual void waitForDebugger() = 0;

};

enum class LogLevel
{
  Trace = 0,
  Info = 1,
  Warning = 2,
  Error = 3,
  Fatal = 4,
};

using Logger = std::function<void(const char* message, LogLevel logLevel)>;

struct V8RuntimeArgs {
  const v8::Platform* platform;
  std::shared_ptr<Logger> logger;
    
  std::unique_ptr<TaskRunner> foreground_task_runner; // foreground === js_thread => sequential
  
  // Sorry, currently we don't support providing custom background runner. We create a default one shared by all runtimes.
  // std::unique_ptr<TaskRunner> background_task_runner; // background thread pool => non sequential

  std::unique_ptr<InspectorInterface> inspector = nullptr;

  std::unique_ptr<const facebook::jsi::Buffer> custom_snapshot_blob;
};

std::unique_ptr<facebook::jsi::Runtime> makeV8Runtime(V8RuntimeArgs&& args);

// TODO :: Following should go to a more private header

constexpr int ISOLATE_DATA_SLOT = 0;

// Platform needs to map every isolate to this data.
struct IsolateData {
  std::shared_ptr<v8::TaskRunner> foreground_task_runner_;
  
  // Weak reference.
  void* runtime_;
};


} // namespace v8runtime