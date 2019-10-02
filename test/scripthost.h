#pragma once

#include <jsi\jsi.h>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <set>
#include <thread>

#include <queue>

#include <iostream>

#include <jsi/jsi.h>

class MyTask {
 public:
  virtual ~MyTask() = default;
  virtual void run() = 0;
};

class FunctionTask : public MyTask {
 public:
  FunctionTask(std::function<void()> &&func) : func_(std::move(func)) {}
  void run() override {
    func_();
  }

 private:
  std::function<void()> func_;
};

class MyIdleTask {
 public:
  virtual ~MyIdleTask() = default;
  virtual void run(double deadline_in_seconds) = 0;
};

class MyTaskRunner {
 public:
  MyTaskRunner();
  ~MyTaskRunner();

  void PostTask(std::unique_ptr<MyTask> task);

  void PostDelayedTask(std::unique_ptr<MyTask> task, double delay_in_seconds);

  void PostIdleTask(std::unique_ptr<MyIdleTask> task) {
    std::abort();
  }

  bool IdleTasksEnabled() {
    return false;
  };

  // Must be called from an alien thread.
  void WaitTillDone();

 private:
  void WorkerFunc();
  void TimerFunc();

 private:
  using DelayedEntry = std::pair<double, std::unique_ptr<MyTask>>;

  // Define a comparison operator for the delayed_task_queue_ to make sure
  // that the unique_ptr in the DelayedEntry is not accessed in the priority
  // queue. This is necessary because we have to reset the unique_ptr when we
  // remove a DelayedEntry from the priority queue.
  struct DelayedEntryCompare {
    bool operator()(DelayedEntry &left, DelayedEntry &right) {
      return left.first > right.first;
    }
  };

  std::priority_queue<
      DelayedEntry,
      std::vector<DelayedEntry>,
      DelayedEntryCompare>
      delayed_task_queue_;
  std::queue<std::unique_ptr<MyTask>> tasks_queue_;

  std::mutex queue_access_mutex_;
  std::condition_variable tasks_available_cond_;

  std::mutex delayed_queue_access_mutex_;
  std::condition_variable delayed_tasks_available_cond_;

  std::atomic<bool> stop_requested_{false};

  // TODO -- This should become a semaphore when we support more than one worker
  // thread.
  std::mutex worker_stopped_mutex_;
  std::condition_variable worker_stopped_cond_;
  bool worker_stopped_{false};

  std::mutex timer_stopped_mutex_;
  std::condition_variable timer_stopped_cond_;
  bool timer_stopped_{false};
};

class ScriptHost {
 public:
  /*static ScriptHost& instance() {
static ScriptHost inst;
return inst;
  }*/

  std::unique_ptr<facebook::jsi::Runtime> runtime_;

  std::shared_ptr<MyTaskRunner> jsiTaskRunner_;
  // EventLoop jsiEventLoop_;
  // BgEventLoop bgEventLoop_;

  ScriptHost() : ScriptHost(std::make_shared<MyTaskRunner>()) {}
  ScriptHost(std::shared_ptr<MyTaskRunner> jsiTaskRunner);

  void Reset();

  ~ScriptHost() {
    std::cout << "dtor";
  }

  void runScript(std::string &script);

  double setTimeout(facebook::jsi::Function &&jsiFuncCallback, double ms);
  double setImmediate(facebook::jsi::Function &&jsiFuncCallback);
  double setInterval(facebook::jsi::Function &&jsiFuncCallback, double delay);
  void clearTimeout(double timeoutId);
};
