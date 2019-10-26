#include "public/V8JsiRuntime.h"

#include "scripthost.h"

#include <fstream>
#include <streambuf>

int main() {
  v8runtime::V8RuntimeArgs args{};

  // std::string script("var promise = getDataAsync('xyz');
  // print('Hoy');promise.then(function(value) { print(value);}); //#
  // sourceURL=filename.js"); std::string script("print('Hoy'); //#
  // sourceURL=filename.js");

  //std::string script(
  //    "function fib(n) { if (n < 2) { return 1; } return fib(n - 1) + fib(n - 2) } print(fib(20));");

  std::string script(
	 "print(new Date())");

  /*std::ifstream t("D:\\work\\moment_test\\dist\\index.js");
  std::string script(
      (std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());*/

  //std::string script2(
  //    "function fib(n) { if (n < 2) { return 1; } return fib(n - 1) + fib(n - 2) } print(fib(20));");

  {
    auto taskRunner = std::make_shared<MyTaskRunner>();
    // auto taskRunner2 = std::make_shared<MyTaskRunner>();

    ScriptHost host(taskRunner);
    // ScriptHost host2(taskRunner2);

    host.runScript(script);
    // host2.runScript(script);

    getchar();

	getchar();

    host.Reset();
    // host2.Reset();

	getchar();
  }

  // ScriptHost host2;
  // host2.runScript(script2);

  // ScriptHost::instance().runScript(script);

  getchar();

  // ScriptHost::instance().jsiTaskRunner_->WaitTillDone();
  // std::unique_ptr<facebook::jsi::Runtime> runtime =
  // v8runtime::makeV8Runtime(std::move(args)); runtime->evaluateJavaScript()
  // facebook::jsi::Object global = runtime->global();
}