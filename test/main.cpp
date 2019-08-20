#include "jsiruntime/V8JsiRuntime.h"

#include "scripthost.h"

int main() {
	v8runtime::V8RuntimeArgs args{};
	
	// std::string script("var promise = getDataAsync('xyz'); print('Hoy');promise.then(function(value) { print(value);}); //# sourceURL=filename.js");
	std::string script("print('Hoy'); //# sourceURL=filename.js");
	ScriptHost::instance().runScript(script);

	getchar();

	ScriptHost::instance().jsiTaskRunner_->WaitTillDone();
	// std::unique_ptr<facebook::jsi::Runtime> runtime = v8runtime::makeV8Runtime(std::move(args)); 
	// runtime->evaluateJavaScript()
	// facebook::jsi::Object global = runtime->global();
}