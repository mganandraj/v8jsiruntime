#pragma once

#include "V8JsiRuntime.h"

#include "v8.h"
#include "libplatform/libplatform.h"

namespace v8runtime {

class TaskAdapter : public v8runtime::JSITask {
public:
  TaskAdapter(std::unique_ptr<v8::Task>&& task)
    : task_(std::move(task)) {}

  void run() override {
    task_->Run();
  }

private:
  std::unique_ptr<v8::Task> task_;
};

class IdleTaskAdapter : public v8runtime::JSIIdleTask {
public:
  IdleTaskAdapter(std::unique_ptr<v8::IdleTask>&& task)
    : task_(std::move(task)) {}

  void run(double deadline_in_seconds) override {
    task_->Run(deadline_in_seconds);
  }

private:
  std::unique_ptr<v8::IdleTask> task_;
};
  
class TaskRunnerAdapter : public v8::TaskRunner {
public:
  TaskRunnerAdapter(std::unique_ptr<v8runtime::JSITaskRunner>&& taskRunner)
    : taskRunner_(std::move(taskRunner)) {}

  void PostTask(std::unique_ptr<v8::Task> task) override {
    taskRunner_->postTask(std::make_unique<TaskAdapter>(std::move(task)));
  }

  void PostDelayedTask(std::unique_ptr<v8::Task> task, double delay_in_seconds) override {
    taskRunner_->postDelayedTask(std::make_unique<TaskAdapter>(std::move(task)), delay_in_seconds);
  }

  void PostIdleTask(std::unique_ptr<v8::IdleTask> task) override {
    taskRunner_->postIdleTask(std::make_unique<IdleTaskAdapter>(std::move(task)));
  }

  bool IdleTasksEnabled() override {
    return taskRunner_->IdleTasksEnabled();
  }

private:
  std::unique_ptr<v8runtime::JSITaskRunner> taskRunner_;
};

};