//  Copyright (c) Facebook, Inc. and its affiliates.
#include "V8Runtime.h"
#include "V8Runtime_impl.h"

using namespace facebook;

namespace v8runtime {

V8Runtime::V8Runtime() {
  // Not to be called on Windows.
  std::abort();
}

V8Runtime::V8Runtime(const folly::dynamic& v8Config) : V8Runtime() {
  // Not to be called on Windows.
  std::abort();
}

bool V8Runtime::ExecuteString(const v8::Local<v8::String>& source, const std::string& sourceURL) {
  // Not to be called on windows.
  std::abort();
}

} // namespace v8runtime
