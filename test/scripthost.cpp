#include <string>

#include "scripthost.h"

#include <jsiruntime/V8JsiRuntime.h>

#include "typedjsi.h"

#include <iostream>

#include "logger.h"

using namespace facebook;

class JSILogger {
 public:
  static jsi::HostFunctionType getLogger() {
    std::shared_ptr<JSILogger> jsiLogger(new JSILogger());
    auto func = [jsiLogger](
                    jsi::Runtime &runtime,
                    const jsi::Value &,
                    const jsi::Value *args,
                    size_t count) {
      // jsiLogger->ping();
      Logger::log(args[0].asString(runtime).utf8(runtime).c_str());
      return jsi::Value::undefined();
    };
    return std::move(func);
  }

  ~JSILogger() {
    Logger::log("~JSILogger");
  }

  void ping() {
    Logger::log("ping ping");
  }
};

class ThrowFunc {
 public:
  static jsi::HostFunctionType getFunc() {
    auto func = [](jsi::Runtime &runtime,
                   const jsi::Value &,
                   const jsi::Value *args,
                   size_t count) {
      // throw std::runtime_error("Error");
      // throw facebook::jsi::JSINativeException(std::string("Exc"));
      throw 3;
      return jsi::Value::undefined();
    };
    return std::move(func);
  }

  ~ThrowFunc() {
    Logger::log("~JSILogger");
  }
};

class MyHostObject {
 public:
  class HostObjectImpl : public jsi::HostObject {
   public:
    HostObjectImpl(std::shared_ptr<MyHostObject> hostObject)
        : hostObject_(hostObject) {}

    ~HostObjectImpl() {
      Logger::log("~HostObjectImpl");
    }

    jsi::Value get(jsi::Runtime &rt, const jsi::PropNameID &name) override {
      return jsi::String::createFromUtf8(rt, hostObject_->get(name.utf8(rt)));
    }

    void set(
        jsi::Runtime &rt,
        const jsi::PropNameID &name,
        const jsi::Value &value) override {
      hostObject_->set(name.utf8(rt), value.getString(rt).utf8(rt));
    }

    std::shared_ptr<MyHostObject> hostObject_;
  };

  static std::shared_ptr<jsi::HostObject> getHostObject() {
    return std::make_shared<HostObjectImpl>(std::make_shared<MyHostObject>());
  }

  std::string get(const std::string &propName) {
    if (propName == "name") {
      return name_;
    }

    if (propName == "state") {
      return state_;
    }
  }

  void set(const std::string &propName, const std::string &value) {
    if (propName == "name") {
      name_.assign(value);
    }

    if (propName == "state") {
      state_.assign(value);
    }
  }

  ~MyHostObject() {
    Logger::log("~MyHostObject");
  }

  std::string name_{"MyHostObject"};
  std::string state_{"sleepy"};
};

class StringOpHostObject : public jsi::HostObject {
 public:
  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override {
    auto methodName = name.utf8(runtime);

    if (methodName == "appendString") {
      return jsi::Function::createFromHostFunction(
          runtime,
          name,
          2,
          [this](
              jsi::Runtime &runtime,
              const jsi::Value &thisValue,
              const jsi::Value *arguments,
              size_t count) -> jsi::Value {
            return jsi::String::createFromAscii(
                runtime,
                arguments[0].getString(runtime).utf8(runtime) +
                    arguments[1].getString(runtime).utf8(runtime).c_str());
          });
    } else {
      return jsi::Value::undefined();
    }
  }
};

class v8RuntimeTask : public MyTask {
 public:
  v8RuntimeTask(std::unique_ptr<v8runtime::JSITask> task)
      : task_(std::move(task)) {}
  void run() override {
    task_->run();
  }

 private:
  std::unique_ptr<v8runtime::JSITask> task_;
};

class MyTaskRunnerAdapter : public v8runtime::JSITaskRunner {
 public:
  MyTaskRunnerAdapter(std::shared_ptr<MyTaskRunner> taskRunner)
      : taskRunner_(std::move(taskRunner)) {}

  void postTask(std::unique_ptr<v8runtime::JSITask> task) override {
    taskRunner_->PostTask(std::make_unique<v8RuntimeTask>(std::move(task)));
  }

  void postDelayedTask(
      std::unique_ptr<v8runtime::JSITask> task,
      double delay_in_seconds) override {
    taskRunner_->PostDelayedTask(
        std::make_unique<v8RuntimeTask>(std::move(task)), delay_in_seconds);
  }

  void postIdleTask(std::unique_ptr<v8runtime::JSIIdleTask> task) override {
    std::abort();
  }

  bool IdleTasksEnabled() override {
    return true;
  }

 private:
  MyTaskRunnerAdapter(const MyTaskRunnerAdapter &) = delete;
  MyTaskRunnerAdapter &operator=(const MyTaskRunnerAdapter &) = delete;

  std::shared_ptr<MyTaskRunner> taskRunner_;
};

void ScriptHost::Reset() {
  jsiTaskRunner_->PostTask(
      std::make_unique<FunctionTask>([this]() { runtime_ = nullptr; }));
}

ScriptHost::ScriptHost(std::shared_ptr<MyTaskRunner> jsiTaskRunner) {
  // jsiTaskRunner_ = std::make_shared<MyTaskRunner>();
  jsiTaskRunner_ = jsiTaskRunner;

  jsiTaskRunner_->PostTask(std::make_unique<FunctionTask>([this]() {
    v8runtime::V8RuntimeArgs args{};
    args.liteMode = false;
    args.trackGCObjectStats = false;
    args.backgroundMode = false;

    args.maximum_heap_size_in_bytes = 10 * 1024 * 1024;

    // auto tr = new MyTaskRunnerAdapter(jsiTaskRunner_);
    args.foreground_task_runner =
        std::make_unique<MyTaskRunnerAdapter>(jsiTaskRunner_);
    runtime_ = v8runtime::makeV8Runtime(std::move(args));

    // facebook::jsi::chakraruntime::ChakraRuntimeArgs args;
    // args.loggingCallback = [](const char* message,
    // facebook::jsi::chakraruntime::LogLevel level) { std::cout << message <<
    // std::endl; }; runtime_ =
    // facebook::jsi::chakraruntime::makeChakraRuntime(std::move(args));
  }));

  jsiTaskRunner_->PostTask(std::make_unique<FunctionTask>([this]() {
    runtime_->global().setProperty(
        *runtime_,
        "mylogger",
        jsi::Function::createFromHostFunction(
            *runtime_,
            jsi::PropNameID::forAscii(*runtime_, "mylogger"),
            1,
            JSILogger::getLogger()));

    runtime_->global().setProperty(
        *runtime_,
        "throwfunc",
        jsi::Function::createFromHostFunction(
            *runtime_,
            jsi::PropNameID::forAscii(*runtime_, "throwfunc"),
            1,
            ThrowFunc::getFunc()));

    /*runtime_->global().setProperty(
            *runtime_,
            "getTimeAsync",
            jsi::Function::createFromAsyncHostFunction(
                    *runtime_,
                    jsi::PropNameID::forAscii(*runtime_, "getTimeAsync"),
                    1,
                    [](jsi::Runtime& runtime, const jsi::Value&, const
    jsi::Value* args, size_t count) { std::unique_ptr<AsyncEvent> event =
    std::make_unique<AsyncEvent>(); event->func_ = []() {return
    jsi::Value::undefined(); }; return event->promise_.get_future();

    }));*/

    /*runtime_->global().setProperty(
            *runtime_,
            "myobject",
            jsi::Object::createFromHostObject(*runtime_,
    MyHostObject::getHostObject())
    );*/

    /*runtime_->global().setProperty(
      *runtime_,
      "stringOps",
      jsi::Object::createFromHostObject(*runtime_,
    std::make_shared<StringOpHostObject>())
    );*/

    runtime_->global().setProperty(
        *runtime_,
        "setTimeout",
        facebook::jsi::Function::createFromHostFunction(
            *runtime_,
            facebook::jsi::PropNameID::forAscii(*runtime_, "settimeout"),
            2,
            [this](
                facebook::jsi::Runtime &runtime,
                const facebook::jsi::Value &,
                const facebook::jsi::Value *args,
                size_t count) {
              if (count != 2) {
                throw std::invalid_argument(
                    "Function setTimeout expects 2 arguments");
              }

              double returnvalue = this->setTimeout(
                  args[0].getObject(runtime).asFunction(runtime),
                  typedjsi::get<double>(runtime, args[1]) /*ms*/);
              return facebook::jsi::detail::toValue(runtime, returnvalue);
            }));

    runtime_->global().setProperty(
        *runtime_,
        "setImmediate",
        facebook::jsi::Function::createFromHostFunction(
            *runtime_,
            facebook::jsi::PropNameID::forAscii(*runtime_, "setImmediate"),
            1,
            [this](
                facebook::jsi::Runtime &runtime,
                const facebook::jsi::Value &,
                const facebook::jsi::Value *args,
                size_t count) {
              if (count != 1) {
                throw std::invalid_argument(
                    "Function setImmediate expects 1 arguments");
              }

              double returnValue = this->setImmediate(
                  args[0].getObject(runtime).asFunction(runtime));
              return facebook::jsi::detail::toValue(runtime, returnValue);
            }));

    runtime_->global().setProperty(
        *runtime_,
        "setInterval",
        facebook::jsi::Function::createFromHostFunction(
            *runtime_,
            facebook::jsi::PropNameID::forAscii(*runtime_, "setInterval"),
            2,
            [this](
                facebook::jsi::Runtime &runtime,
                const facebook::jsi::Value &,
                const facebook::jsi::Value *args,
                size_t count) {
              if (count != 2) {
                throw std::invalid_argument(
                    "Function setInterval expects 2 arguments");
              }

              double returnValue = this->setInterval(
                  args[0].getObject(runtime).asFunction(runtime),
                  typedjsi::get<double>(runtime, args[1]) /*delay*/);
              return facebook::jsi::detail::toValue(runtime, returnValue);
            }));

    runtime_->global().setProperty(
        *runtime_,
        "clearTimeout",
        facebook::jsi::Function::createFromHostFunction(
            *runtime_,
            facebook::jsi::PropNameID::forAscii(*runtime_, "clearTimeout"),
            1,
            [this](
                facebook::jsi::Runtime &runtime,
                const facebook::jsi::Value &,
                const facebook::jsi::Value *args,
                size_t count) {
              if (count != 1) {
                throw std::invalid_argument(
                    "Function clearTimeout expects 1 arguments");
              }

              this->clearTimeout(
                  typedjsi::get<double>(runtime, args[0]) /*timeoutId*/);

              return facebook::jsi::Value::undefined();
            }));

    /*jsi::Value stringOpsObj = runtime_->global().getProperty(
      *runtime_,
      "stringOps");

    std::shared_ptr<jsi::HostObject> ho =
    stringOpsObj.getObject(*runtime_).getHostObject(*runtime_);
*/
  }));
}

struct StringBuffer : public jsi::Buffer {
  size_t size() const override {
    return string_.size();
  }

  const uint8_t *data() const override {
    return reinterpret_cast<const uint8_t *>(string_.c_str());
  }

  StringBuffer(const std::string &str) : string_(str) {}

  const std::string string_;
};

void ScriptHost::runScript(std::string &script) {
  jsiTaskRunner_->PostTask(std::make_unique<FunctionTask>([this, script]() {
    try {
      std::string sourceUrl("MyJS");
      runtime_->evaluateJavaScript(
          std::make_unique<StringBuffer>(script), sourceUrl);
    } catch (std::exception &exc) {
      std::cout << exc.what();
    }
  }));
}

double ScriptHost::setTimeout(jsi::Function &&jsiFuncCallback, double ms) {
  std::abort();
  // auto timeout = std::chrono::milliseconds(static_cast<long long>(ms));
  // auto timerId = jsiEventLoop_.add(timeout,
  // std::make_unique<JSIFunctionProxy>(*runtime_, std::move(jsiFuncCallback)));
  // return static_cast<double>(timerId);
}

double ScriptHost::setInterval(jsi::Function &&jsiFuncCallback, double delay) {
  std::abort();
  /*auto timeout = std::chrono::milliseconds(static_cast<long long>(delay));
  auto timerId = jsiEventLoop_.addPeriodic(timeout,
  std::make_unique<JSIFunctionProxy>(*runtime_, std::move(jsiFuncCallback)));
  return static_cast<double>(timerId);*/
}

double ScriptHost::setImmediate(jsi::Function &&jsiFuncCallback) {
  std::abort();
  /*try
  {
    auto timerId = jsiEventLoop_.add(std::chrono::milliseconds(0),
  std::make_unique<JSIFunctionProxy>(*runtime_, std::move(jsiFuncCallback)));
    return static_cast<double>(timerId);
  }
  catch (...)
  {
    std::throw_with_nested(facebook::jsi::JSError(*runtime_, "Failed to register
  native callback."));
  }*/
}

void ScriptHost::clearTimeout(double timerId) {
  // jsiEventLoop_.cancel(static_cast<size_t>(timerId));
}

MyTaskRunner::~MyTaskRunner() {
  stop_requested_ = true;
  tasks_available_cond_.notify_all();
  delayed_tasks_available_cond_.notify_all();

  WaitTillDone();
}

void MyTaskRunner::WaitTillDone() {
  // Wait until both worker and timer threads are dranined.
  std::unique_lock<std::mutex> worker_stopped_lock(worker_stopped_mutex_);
  worker_stopped_cond_.wait(
      worker_stopped_lock, [this]() { return worker_stopped_; });

  std::unique_lock<std::mutex> timer_stopped_lock(timer_stopped_mutex_);
  timer_stopped_cond_.wait(
      timer_stopped_lock, [this]() { return timer_stopped_; });
}

MyTaskRunner::MyTaskRunner() {
  std::thread(&MyTaskRunner::WorkerFunc, this).detach();
  std::thread(&MyTaskRunner::TimerFunc, this).detach();
}

void MyTaskRunner::PostTask(std::unique_ptr<MyTask> task) {
  {
    std::lock_guard<std::mutex> lock(queue_access_mutex_);
    tasks_queue_.push(std::move(task));
  }

  tasks_available_cond_.notify_all();
}

void MyTaskRunner::PostDelayedTask(
    std::unique_ptr<MyTask> task,
    double delay_in_seconds) {
  {
    if (delay_in_seconds == 0) {
      PostTask(std::move(task));
      return;
    }

    double deadline =
        std::chrono::steady_clock::now().time_since_epoch().count() +
        delay_in_seconds *
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::seconds(1))
                .count();

    std::lock_guard<std::mutex> lock(delayed_queue_access_mutex_);
    delayed_task_queue_.push(std::make_pair(deadline, std::move(task)));
    delayed_tasks_available_cond_.notify_all();
  }
}

void MyTaskRunner::WorkerFunc() {
  while (true) {
    std::unique_lock<std::mutex> lock(queue_access_mutex_);
    tasks_available_cond_.wait(
        lock, [this]() { return !tasks_queue_.empty() || stop_requested_; });

    if (stop_requested_)
      break;
    if (tasks_queue_.empty())
      continue;

    std::unique_ptr<MyTask> nexttask = std::move(tasks_queue_.front());
    tasks_queue_.pop();

    lock.unlock();

    nexttask->run();
  }

  worker_stopped_ = true;
  worker_stopped_cond_.notify_all();
}

void MyTaskRunner::TimerFunc() {
  while (true) {
    std::unique_lock<std::mutex> delayed_lock(delayed_queue_access_mutex_);
    delayed_tasks_available_cond_.wait(delayed_lock, [this]() {
      return !delayed_task_queue_.empty() || stop_requested_;
    });

    if (stop_requested_)
      break;

    if (delayed_task_queue_.empty())
      continue; // Loop back and block the thread.

    std::queue<std::unique_ptr<MyTask>> new_ready_tasks;

    do {
      const DelayedEntry &delayed_entry = delayed_task_queue_.top();
      if (delayed_entry.first <
          std::chrono::steady_clock::now().time_since_epoch().count()) {
        new_ready_tasks.push(
            std::move(const_cast<DelayedEntry &>(delayed_entry).second));
        delayed_task_queue_.pop();
      } else {
        // The rest are not ready ..
        break;
      }

    } while (!delayed_task_queue_.empty());

    delayed_lock.unlock();

    if (!new_ready_tasks.empty()) {
      std::lock_guard<std::mutex> lock(queue_access_mutex_);

      do {
        tasks_queue_.push(std::move(new_ready_tasks.front()));
        new_ready_tasks.pop();
      } while (!new_ready_tasks.empty());

      tasks_available_cond_.notify_all();
    }

    if (!delayed_task_queue_.empty()) {
      std::this_thread::sleep_for(std::chrono::seconds(
          1)); // Wait for a second and loop back and recompute again.
    } // else loop back and block the thread.
  }

  timer_stopped_ = true;
  timer_stopped_cond_.notify_all();
}
