#pragma once

#include <stddef.h>

#include <public/V8JsiRuntime.h>

namespace v8 {
class Platform;
template <typename T>
class Local;
class Value;
class Message;
} // namespace v8

namespace inspector {

class AgentImpl;

class Agent {
 public:
  // TODO :: safe access to platform
  // Note :: Currently we do support one platform/isolate/context per agent..
  // This is enough for our scenarios.
  explicit Agent(
      v8::Platform &platform,
      v8::Isolate *isolate,
      v8::Local<v8::Context> context,
      const char *context_name,
      int port);
  ~Agent();
  
  void waitForDebugger();

  void start();
  void stop();

  bool IsStarted();
  bool IsConnected();
  void WaitForDisconnect();
  void FatalException(
      v8::Local<v8::Value> error,
      v8::Local<v8::Message> message);

 private:
  AgentImpl *impl;
};

} // namespace inspector
