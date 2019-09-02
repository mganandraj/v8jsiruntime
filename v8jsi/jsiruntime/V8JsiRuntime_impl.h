#pragma once

#include "jsiruntime/V8JsiRuntime.h"

#include "v8.h"
#include "libplatform/libplatform.h"

#include "platform/V8Platform.h"

#include <cstdlib>
#include <iostream>
#include <mutex>
#include <atomic>
#include <list>
#include <sstream>
#include <unordered_map>

#include <cstdlib>

using namespace facebook;

#if defined(_MSC_VER)
#define CDECL __cdecl
#else
#define CDECL
#endif

#define _ISOLATE_CONTEXT_ENTER v8::Isolate *isolate = v8::Isolate::GetCurrent(); \
    v8::Isolate::Scope isolate_scope(isolate); \
    v8::HandleScope handle_scope(isolate); \
    v8::Context::Scope context_scope(context_.Get(isolate));

namespace v8runtime {

// Note : Counter implementation based on d8
// A single counter in a counter collection.
class Counter {
public:
	static const int kMaxNameSize = 64;
	int32_t* Bind(const char* name, bool histogram);
	int32_t* ptr() { return &count_; }
	int32_t count() { return count_; }
	int32_t sample_total() { return sample_total_; }
	bool is_histogram() { return is_histogram_; }
	void AddSample(int32_t sample);

private:
	int32_t count_;
	int32_t sample_total_;
	bool is_histogram_;
	uint8_t name_[kMaxNameSize];
};

// A set of counters and associated information.  An instance of this
// class is stored directly in the memory-mapped counters file if
// the --map-counters options is used
class CounterCollection {
public:
	CounterCollection();
	Counter* GetNextCounter();

private:
	static const unsigned kMaxCounters = 512;
	uint32_t magic_number_;
	uint32_t max_counters_;
	uint32_t max_name_size_;
	uint32_t counters_in_use_;
	Counter counters_[kMaxCounters];
};

using CounterMap = std::unordered_map<std::string, Counter*>;

enum class CacheType {
  NoCache,
  CodeCache,
  FullCodeCache
};

class V8Runtime : public facebook::jsi::Runtime {
public:
  V8Runtime(V8RuntimeArgs&& args);
  ~V8Runtime();

  jsi::Value evaluateJavaScript(const std::shared_ptr<const jsi::Buffer>& buffer, const std::string& sourceURL) override;

  facebook::jsi::Object global() override;

  std::string description() override;

  bool isInspectable() override;

private:

  struct IHostProxy {
    virtual void destroy() = 0;
  };

  class HostObjectLifetimeTracker {
  public:
    void ResetHostObject(bool isGC /*whether the call is coming from GC*/) {
      assert(!isGC || !isReset_);
      if (!isReset_) {
        isReset_ = true;
        hostProxy_->destroy();
        objectTracker_.Reset();
      }
    }

    HostObjectLifetimeTracker(V8Runtime& runtime, v8::Local<v8::Object> obj, IHostProxy* hostProxy) : hostProxy_(hostProxy) {
      objectTracker_.Reset(runtime.GetIsolate(), obj);
      objectTracker_.SetWeak(this, HostObjectLifetimeTracker::Destroyed, v8::WeakCallbackType::kParameter);
    }

    // Useful for debugging.
    ~HostObjectLifetimeTracker() {
      assert(isReset_);
      std::cout << "~HostObjectLifetimeTracker" << std::endl;
    }

  private:
    v8::Global<v8::Object> objectTracker_;
    std::atomic<bool> isReset_{ false };
    IHostProxy* hostProxy_;

    static void CDECL Destroyed(const v8::WeakCallbackInfo<HostObjectLifetimeTracker>& data) {
      v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
      data.GetParameter()->ResetHostObject(true /*isGC*/);
    }
  };

  class HostObjectProxy : public IHostProxy {
  public:
    static void Get(v8::Local<v8::Name> v8PropName, const v8::PropertyCallbackInfo<v8::Value>& info)
    {
      v8::Local<v8::External> data = v8::Local<v8::External>::Cast(info.This()->GetInternalField(0));
      HostObjectProxy* hostObjectProxy = reinterpret_cast<HostObjectProxy*>(data->Value());

      if (hostObjectProxy == nullptr)
        std::abort();

      V8Runtime& runtime = hostObjectProxy->runtime_;
      std::shared_ptr<facebook::jsi::HostObject> hostObject = hostObjectProxy->hostObject_;

      v8::Local<v8::String> propNameStr = v8::Local<v8::String>::Cast(v8PropName);
      
	  char buffer[512];
	  propNameStr->WriteUtf8(info.GetIsolate(), buffer);

	//std::string propName;
	//propName.resize(propNameStr->Utf8Length(info.GetIsolate()));
	//propNameStr->WriteUtf8(info.GetIsolate(), &propName[0]);

		// facebook::jsi::PropNameID propNameId = runtime.createPropNameIDFromUtf8(reinterpret_cast<uint8_t*>(&propName[0]), propName.length());
	  facebook::jsi::PropNameID propNameId = runtime.createPropNameIDFromUtf8(reinterpret_cast<uint8_t*>(buffer), propNameStr->Utf8Length(info.GetIsolate()));
	  
	  v8::Local<v8::Value> retValue;
	  {
		  retValue = runtime.valueRef(hostObject->get(runtime, propNameId));
	  }

	  info.GetReturnValue().Set(retValue);
    
	}

    static void Set(v8::Local<v8::Name> v8PropName, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
    {
      v8::Local<v8::External> data = v8::Local<v8::External>::Cast(info.This()->GetInternalField(0));
      HostObjectProxy* hostObjectProxy = reinterpret_cast<HostObjectProxy*>(data->Value());

      if (hostObjectProxy == nullptr)
        std::abort();

      V8Runtime& runtime = hostObjectProxy->runtime_;
      std::shared_ptr<facebook::jsi::HostObject> hostObject = hostObjectProxy->hostObject_;

      v8::Local<v8::String> propNameStr = v8::Local<v8::String>::Cast(v8PropName);

      std::string propName;
      propName.resize(propNameStr->Utf8Length(info.GetIsolate()));
      propNameStr->WriteUtf8(info.GetIsolate(), &propName[0]);

      hostObject->set(runtime, runtime.createPropNameIDFromUtf8(reinterpret_cast<uint8_t*>(&propName[0]), propName.length()), runtime.createValue(value));
    }

    static void Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
    {
      v8::Local<v8::External> data = v8::Local<v8::External>::Cast(info.Data());
      HostObjectProxy* hostObjectProxy = reinterpret_cast<HostObjectProxy*>(data->Value());

      if (hostObjectProxy != nullptr) {

        V8Runtime& runtime = hostObjectProxy->runtime_;
        std::shared_ptr<facebook::jsi::HostObject> hostObject = hostObjectProxy->hostObject_;

        std::vector<facebook::jsi::PropNameID> propIds = hostObject->getPropertyNames(runtime);

        v8::Local<v8::Array> result = v8::Array::New(info.GetIsolate(), static_cast<int>(propIds.size()));
        v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

        for (uint32_t i = 0; i < result->Length(); i++)
        {
          v8::Local<v8::Value> propIdValue = runtime.valueRef(propIds[i]);
          if (!result->Set(context, i, propIdValue).FromJust()) { std::terminate(); };
        }

        info.GetReturnValue().Set(result);
      }
      else {
        info.GetReturnValue().Set(v8::Array::New(info.GetIsolate()));
      }
    }

    HostObjectProxy(V8Runtime& rt, const std::shared_ptr<facebook::jsi::HostObject>& hostObject) : runtime_(rt), hostObject_(hostObject) {}
    std::shared_ptr<facebook::jsi::HostObject> getHostObject() { return hostObject_; }
  private:
    friend class HostObjectLifetimeTracker;
    void destroy() override {
      hostObject_.reset();
    }

    V8Runtime& runtime_;
    std::shared_ptr<facebook::jsi::HostObject> hostObject_;
  };

  class HostFunctionProxy : public IHostProxy {
  public:
    static void call(HostFunctionProxy& hostFunctionProxy, const v8::FunctionCallbackInfo<v8::Value>& callbackInfo) {
      V8Runtime& runtime = const_cast<V8Runtime&>(hostFunctionProxy.runtime_);
      v8::Isolate* isolate = callbackInfo.GetIsolate();

      std::vector<facebook::jsi::Value> argsVector;
      for (int i = 0; i < callbackInfo.Length(); i++)
      {
        argsVector.push_back(hostFunctionProxy.runtime_.createValue(callbackInfo[i]));
      }

      const facebook::jsi::Value& thisVal = runtime.createValue(callbackInfo.This());

      facebook::jsi::Value result;
      try {
        result = hostFunctionProxy.func_(runtime, thisVal, argsVector.data(), callbackInfo.Length());
      }
      catch (const facebook::jsi::JSError& error) {
        callbackInfo.GetReturnValue().Set(v8::Undefined(isolate));

        // Schedule to throw the exception back to JS.
        isolate->ThrowException(runtime.valueRef(error.value()));
        return;
      }
      catch (const std::exception& ex) {
        callbackInfo.GetReturnValue().Set(v8::Undefined(isolate));

        // Schedule to throw the exception back to JS.
        v8::Local<v8::String> message = v8::String::NewFromUtf8(isolate, ex.what(), v8::NewStringType::kNormal).ToLocalChecked();
        isolate->ThrowException(v8::Exception::Error(message));
        return;
      }
      catch (...) {
        callbackInfo.GetReturnValue().Set(v8::Undefined(isolate));

        // Schedule to throw the exception back to JS.
        v8::Local<v8::String> message = v8::String::NewFromOneByte(isolate, reinterpret_cast<const uint8_t*>("<Unknown exception in host function callback>"), v8::NewStringType::kNormal).ToLocalChecked();
        isolate->ThrowException(v8::Exception::Error(message));
        return;
      }

      callbackInfo.GetReturnValue().Set(runtime.valueRef(result));
    }

  public:
    static void CDECL HostFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
      v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
      v8::Local<v8::External> data = v8::Local<v8::External>::Cast(info.Data());
      HostFunctionProxy* hostFunctionProxy = reinterpret_cast<HostFunctionProxy*> (data->Value());
      hostFunctionProxy->call(*hostFunctionProxy, info);
    }

    HostFunctionProxy(V8Runtime& runtime, facebook::jsi::HostFunctionType func)
      : func_(std::move(func)), runtime_(runtime) {};

  private:
    friend class HostObjectLifetimeTracker;
    void destroy() override {
      func_ = [](Runtime& rt, const facebook::jsi::Value& thisVal, const facebook::jsi::Value* args, size_t count) {return facebook::jsi::Value::undefined(); };
    }

    facebook::jsi::HostFunctionType func_;
    V8Runtime& runtime_;
  };

  class V8StringValue final : public PointerValue {
    V8StringValue(v8::Local<v8::String> str);
    ~V8StringValue();

    void invalidate() override;

    v8::Persistent<v8::String> v8String_;
  protected:
    friend class V8Runtime;
  };

  class V8ObjectValue final : public PointerValue {
    V8ObjectValue(v8::Local<v8::Object> obj);

    ~V8ObjectValue();

    void invalidate() override;

    v8::Persistent<v8::Object> v8Object_;

  protected:
    friend class V8Runtime;
  };

  class ExternalOwningOneByteStringResource
    : public v8::String::ExternalOneByteStringResource {
  public:
	explicit ExternalOwningOneByteStringResource(const std::shared_ptr<const jsi::Buffer>& buffer)
		: buffer_(buffer) /*create a copy of shared_ptr*/ {}
    const char* data() const override { return reinterpret_cast<const char*>(buffer_->data()); }
    size_t length() const override { return buffer_->size(); }

  private:
	std::shared_ptr<const jsi::Buffer> buffer_;
  };

  std::shared_ptr<const facebook::jsi::PreparedJavaScript> prepareJavaScript(const std::shared_ptr<const facebook::jsi::Buffer>&, std::string) override;
  facebook::jsi::Value evaluatePreparedJavaScript(const std::shared_ptr<const facebook::jsi::PreparedJavaScript>&) override;

  std::string symbolToString(const facebook::jsi::Symbol&) override;

  PointerValue* cloneString(const Runtime::PointerValue* pv) override;
  PointerValue* cloneObject(const Runtime::PointerValue* pv) override;
  PointerValue* clonePropNameID(const Runtime::PointerValue* pv) override;
  PointerValue* cloneSymbol(const PointerValue*) override;

  facebook::jsi::PropNameID createPropNameIDFromAscii(const char* str, size_t length)
    override;
  facebook::jsi::PropNameID createPropNameIDFromUtf8(const uint8_t* utf8, size_t length)
    override;
  facebook::jsi::PropNameID createPropNameIDFromString(const facebook::jsi::String& str) override;
  std::string utf8(const facebook::jsi::PropNameID&) override;
  bool compare(const facebook::jsi::PropNameID&, const facebook::jsi::PropNameID&) override;

  facebook::jsi::String createStringFromAscii(const char* str, size_t length) override;
  facebook::jsi::String createStringFromUtf8(const uint8_t* utf8, size_t length) override;
  std::string utf8(const facebook::jsi::String&) override;

  facebook::jsi::Object createObject() override;
  facebook::jsi::Object createObject(std::shared_ptr<facebook::jsi::HostObject> ho) override;
  virtual std::shared_ptr<facebook::jsi::HostObject> getHostObject(
    const facebook::jsi::Object&) override;
  facebook::jsi::HostFunctionType& getHostFunction(const facebook::jsi::Function&) override;

  facebook::jsi::Value getProperty(const facebook::jsi::Object&, const facebook::jsi::String& name) override;
  facebook::jsi::Value getProperty(const facebook::jsi::Object&, const facebook::jsi::PropNameID& name)
    override;
  bool hasProperty(const facebook::jsi::Object&, const facebook::jsi::String& name) override;
  bool hasProperty(const facebook::jsi::Object&, const facebook::jsi::PropNameID& name) override;
  void setPropertyValue(
    facebook::jsi::Object&,
    const facebook::jsi::String& name,
    const facebook::jsi::Value& value) override;
  void setPropertyValue(
    facebook::jsi::Object&,
    const facebook::jsi::PropNameID& name,
    const facebook::jsi::Value& value) override;
  bool isArray(const facebook::jsi::Object&) const override;
  bool isArrayBuffer(const facebook::jsi::Object&) const override;
  bool isFunction(const facebook::jsi::Object&) const override;
  bool isHostObject(const facebook::jsi::Object&) const override;
  bool isHostFunction(const facebook::jsi::Function&) const override;
  facebook::jsi::Array getPropertyNames(const facebook::jsi::Object&) override;

  facebook::jsi::WeakObject createWeakObject(const facebook::jsi::Object&) override;
  facebook::jsi::Value lockWeakObject(const facebook::jsi::WeakObject&) override;

  facebook::jsi::Array createArray(size_t length) override;
  size_t size(const facebook::jsi::Array&) override;
  size_t size(const facebook::jsi::ArrayBuffer&) override;
  uint8_t* data(const facebook::jsi::ArrayBuffer&) override;
  facebook::jsi::Value getValueAtIndex(const facebook::jsi::Array&, size_t i) override;
  void setValueAtIndexImpl(facebook::jsi::Array&, size_t i, const facebook::jsi::Value& value)
    override;

  facebook::jsi::Function createFunctionFromHostFunction(
    const facebook::jsi::PropNameID& name,
    unsigned int paramCount,
    facebook::jsi::HostFunctionType func) override;
  facebook::jsi::Value call(
    const facebook::jsi::Function&,
    const facebook::jsi::Value& jsThis,
    const facebook::jsi::Value* args,
    size_t count) override;
  facebook::jsi::Value callAsConstructor(
    const facebook::jsi::Function&,
    const facebook::jsi::Value* args,
    size_t count) override;

  bool strictEquals(const facebook::jsi::String& a, const facebook::jsi::String& b) const override;
  bool strictEquals(const facebook::jsi::Object& a, const facebook::jsi::Object& b) const override;
  bool strictEquals(const jsi::Symbol& a, const jsi::Symbol& b) const override;

  bool instanceOf(const facebook::jsi::Object& o, const facebook::jsi::Function& f) override;

void AddHostObjectLifetimeTracker(std::shared_ptr<HostObjectLifetimeTracker> hostObjectLifetimeTracker);

static void CDECL OnMessage(v8::Local<v8::Message> message, v8::Local<v8::Value> error);

V8RuntimeArgs& runtimeArgs() {
  return args_;
}

private:

  V8RuntimeArgs args_;

  v8::Local<v8::Context> CreateContext(v8::Isolate* isolate);

  // Methods to compile and execute JS script.
  v8::ScriptCompiler::CachedData* TryLoadCachedData(const std::string& path);
  void PersistCachedData(std::unique_ptr<v8::ScriptCompiler::CachedData> cachedData, const std::string& path);
  
  v8::Local<v8::Script> GetCompiledScriptFromCache(const v8::Local<v8::String> &source, const std::string& sourceURL);
  v8::Local<v8::Script> GetCompiledScript(const v8::Local<v8::String> &source, const std::string& sourceURL);

  jsi::Value ExecuteString(v8::Local<v8::String> source, const facebook::jsi::Buffer* cache, v8::Local<v8::Value> name, bool report_exceptions);
  jsi::Value ExecuteString(const v8::Local<v8::String>& source, const std::string& sourceURL);
  
  void ReportException(v8::TryCatch* try_catch);

  v8::Isolate* GetIsolate() const { return isolate_; }

  // Basically convenience casts
  static v8::Local<v8::String> stringRef(const facebook::jsi::String& str);
  static v8::Local<v8::Value> valueRef(const facebook::jsi::PropNameID& sym);
  static v8::Local<v8::Object> objectRef(const facebook::jsi::Object& obj);

  v8::Local<v8::Value> valueRef(const facebook::jsi::Value& value);
  facebook::jsi::Value createValue(v8::Local<v8::Value> value) const;

  // Factory methods for creating String/Object
  facebook::jsi::String createString(v8::Local<v8::String> stringRef) const;
  facebook::jsi::PropNameID createPropNameID(v8::Local<v8::Value> propValRef);
  facebook::jsi::Object createObject(v8::Local<v8::Object> objectRef) const;

  // Used by factory methods and clone methods
  facebook::jsi::Runtime::PointerValue* makeStringValue(v8::Local<v8::String> str) const;
  facebook::jsi::Runtime::PointerValue* makeObjectValue(v8::Local<v8::Object> obj) const;

  v8::Isolate* isolate_;
  std::unique_ptr<v8runtime::IsolateData> isolate_data_;

  v8::Global<v8::Context> context_;

  v8::StartupData startup_data_;
  v8::Isolate::CreateParams create_params_;

  v8::Persistent<v8::FunctionTemplate> hostFunctionTemplate_;
  v8::Persistent<v8::Function> hostObjectConstructor_;

  std::list<std::shared_ptr<HostObjectLifetimeTracker>> hostObjectLifetimeTrackerList_;

  // These are a few configuration parameter used only on Android now.
  bool isCacheEnabled_ {false};
  bool shouldProduceFullCache_ {false};
  bool shouldSetNoLazyFlag_ {false};
  std::string cacheDirectory_;
  CacheType cacheType_;

  bool reportException_{ true };
  bool printResult_{ false };
  std::string desc_;


  static void MapCounters(v8::Isolate* isolate, const char* name);
  static Counter* GetCounter(const char* name, bool is_histogram);
  static int* LookupCounter(const char* name);
  static void* CreateHistogram(const char* name, int min, int max,
	  size_t buckets);
  static void AddHistogramSample(void* histogram, int sample);

  static void DumpCounters(const char* when);

  static CounterMap* counter_map_;
  // We statically allocate a set of local counters to be used if we
  // don't want to store the stats in a memory-mapped file
  static CounterCollection local_counters_;
  static CounterCollection* counters_;
  static char counters_file_[sizeof(CounterCollection)];


  const v8::Platform* platform_;
  v8::StartupData custom_snapshot_startup_data_;

  std::vector<std::unique_ptr<ExternalOwningOneByteStringResource>> owned_external_string_resources_;

  // v8::Platform is shared between isolates. It will be kept alive till there is an isolate using it.
  // It will be shutdown when isolate count drops to zero.
  // Following statics are used to manage the liftime of the v8::Platform.
  static std::mutex sMutex_;
  static bool sIsPlatformCreated_;
  static unsigned int sCurrentIsolateCount_;
};
} // namespace v8runtime
