#pragma once

#include <stddef.h>

#include <jsiruntime/V8Runtime.h>

namespace v8 {
	class Platform;
	template<typename T>
	class Local;
	class Value;
	class Message;
}  // namespace v8

namespace inspector {

class AgentImpl;

class Agent : public v8runtime::InspectorInterface {
public:
  // TODO :: safe access to platform
	explicit Agent(v8::Platform* platform, int port);
	~Agent();

  void initialize(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* context_name /*must be null terminated*/) override;
  void waitForDebugger() override;

  void start();
  void stop();

	bool IsStarted();
	bool IsConnected();
	void WaitForDisconnect();
	void FatalException(v8::Local<v8::Value> error, v8::Local<v8::Message> message);

private:
	AgentImpl* impl;
};

}  // namespace inspector
