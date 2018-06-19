#include "libplatform/libplatform.h"
#include "pl_util.h"
#include "V8Context.h"

int V8Context::instance_count = 0;
std::unique_ptr<Platform> V8Context::platform = 0;

// Extracts a C string from a V8 Utf8Value.
static const char* ToCString(const String::Utf8Value& value)
{
    return *value ? *value : "<string conversion failed>";
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
static void Print(const FunctionCallbackInfo<Value>& args)
{
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        HandleScope handle_scope(args.GetIsolate());
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        String::Utf8Value str(args.GetIsolate(), args[i]);
        const char* cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}

static void Version(const FunctionCallbackInfo<Value>& args)
{
    args.GetReturnValue().Set(
            String::NewFromUtf8(args.GetIsolate(), V8::GetVersion(),
                NewStringType::kNormal).ToLocalChecked());
}

static void TimeStamp_ms(const FunctionCallbackInfo<Value>& args)
{
    double now = now_us() / 1000.0;
    args.GetReturnValue().Set(Local<Object>::Cast(Number::New(args.GetIsolate(), now)));
}

V8Context::V8Context(const char* program_)
{
    // fprintf(stderr, "V8 constructing\n");
    program = new char[256];
    if (program_) {
        strcpy(program, program_);
    }
    else {
        sprintf(program, "program_%05d", instance_count);
    }

    V8Context::initialize_v8(this);

    // Create a new Isolate and make it the current one.
    create_params.array_buffer_allocator =
        ArrayBuffer::Allocator::NewDefaultAllocator();
    // fprintf(stderr, "V8 created allocator\n");
    isolate = Isolate::New(create_params);
    // fprintf(stderr, "V8 created isolate\n");
    // fprintf(stderr, "V8 construct done\n");

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    // Create a template for the global object.
    Local<ObjectTemplate> object_template = ObjectTemplate::New(isolate);

    // Bind the global 'print' function to the C++ Print callback.
    Local<FunctionTemplate> ft = FunctionTemplate::New(isolate, Print);
    Local<Name> v8_key = String::NewFromUtf8(isolate, "func_property", NewStringType::kNormal).ToLocalChecked();
    Local<Value> val = Integer::New(isolate, 11);
    ft->Set(v8_key, val);
    // ft->Set("func_property", val);
    object_template->Set(
            String::NewFromUtf8(isolate, "print", NewStringType::kNormal).ToLocalChecked(),
            ft);

    // Bind the 'version' function
    object_template->Set(
            String::NewFromUtf8(isolate, "version", NewStringType::kNormal).ToLocalChecked(),
            FunctionTemplate::New(isolate, Version));

    // Bind the 'timestamp_ms' function
    object_template->Set(
            String::NewFromUtf8(isolate, "timestamp_ms", NewStringType::kNormal).ToLocalChecked(),
            FunctionTemplate::New(isolate, TimeStamp_ms));

    // Create a new context.
    Local<Context> context = Context::New(isolate, 0, object_template);
    persistent_context.Reset(isolate, context);
    persistent_template.Reset(isolate, object_template);
    // fprintf(stderr, "V8 constructing done\n");
}

V8Context::~V8Context()
{
    isolate->Dispose();
    delete create_params.array_buffer_allocator;
    delete[] program;

#if 0
    // We should terminate v8 at some point.  However, because the calling code
    // may create multiple instances of V8Context, whether "nested" or
    // "sequential", we cannot just assume we should do this.  For now, we just
    // *never* terminate v8.
    V8Context::terminate_v8(this);
#endif
}

SV* V8Context::get(const char* name)
{
    return pl_get_global_or_property(aTHX_ this, name);
}

SV* V8Context::exists(const char* name)
{
    return pl_exists_global_or_property(aTHX_ this, name);
}

SV* V8Context::typeof(const char* name)
{
    return pl_typeof_global_or_property(aTHX_ this, name);
}

void V8Context::set(const char* name, SV* value)
{
    pl_set_global_or_property(aTHX_ this, name, value);
}

SV* V8Context::eval(const char* code, const char* file)
{
    return pl_eval(aTHX_ this, code, file);
}

int V8Context::run_gc()
{
    return pl_run_gc(this);
}

void V8Context::initialize_v8(V8Context* self)
{
    if (instance_count++) {
        return;
    }
    V8::InitializeICUDefaultLocation(self->program);
    V8::InitializeExternalStartupData(self->program);
    platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();
    // fprintf(stderr, "V8 initializing done\n");
}

void V8Context::terminate_v8(V8Context* self)
{
    if (--instance_count) {
        return;
    }
    V8::Dispose();
    V8::ShutdownPlatform();
    // fprintf(stderr, "V8 terminating done\n");
}

uint64_t V8Context::GetTypeFlags(const Local<Value>& v)
{
    uint64_t result = 0;
    if (v->IsArgumentsObject()  ) result |= 0x0000000000000001;
    if (v->IsArrayBuffer()      ) result |= 0x0000000000000002;
    if (v->IsArrayBufferView()  ) result |= 0x0000000000000004;
    if (v->IsArray()            ) result |= 0x0000000000000008;
    if (v->IsBooleanObject()    ) result |= 0x0000000000000010;
    if (v->IsBoolean()          ) result |= 0x0000000000000020;
    if (v->IsDataView()         ) result |= 0x0000000000000040;
    if (v->IsDate()             ) result |= 0x0000000000000080;
    if (v->IsExternal()         ) result |= 0x0000000000000100;
    if (v->IsFalse()            ) result |= 0x0000000000000200;
    if (v->IsFloat32Array()     ) result |= 0x0000000000000400;
    if (v->IsFloat64Array()     ) result |= 0x0000000000000800;
    if (v->IsFunction()         ) result |= 0x0000000000001000;
    if (v->IsGeneratorFunction()) result |= 0x0000000000002000;
    if (v->IsGeneratorObject()  ) result |= 0x0000000000004000;
    if (v->IsInt16Array()       ) result |= 0x0000000000008000;
    if (v->IsInt32Array()       ) result |= 0x0000000000010000;
    if (v->IsInt32()            ) result |= 0x0000000000020000;
    if (v->IsInt8Array()        ) result |= 0x0000000000040000;
    if (v->IsMapIterator()      ) result |= 0x0000000000080000;
    if (v->IsMap()              ) result |= 0x0000000000100000;
    if (v->IsName()             ) result |= 0x0000000000200000;
    if (v->IsNativeError()      ) result |= 0x0000000000400000;
    if (v->IsNull()             ) result |= 0x0000000000800000;
    if (v->IsNumberObject()     ) result |= 0x0000000001000000;
    if (v->IsNumber()           ) result |= 0x0000000002000000;
    if (v->IsObject()           ) result |= 0x0000000004000000;
    if (v->IsPromise()          ) result |= 0x0000000008000000;
    if (v->IsRegExp()           ) result |= 0x0000000010000000;
    if (v->IsSetIterator()      ) result |= 0x0000000020000000;
    if (v->IsSet()              ) result |= 0x0000000040000000;
    if (v->IsStringObject()     ) result |= 0x0000000080000000;
    if (v->IsString()           ) result |= 0x0000000100000000;
    if (v->IsSymbolObject()     ) result |= 0x0000000200000000;
    if (v->IsSymbol()           ) result |= 0x0000000400000000;
    if (v->IsTrue()             ) result |= 0x0000000800000000;
    if (v->IsTypedArray()       ) result |= 0x0000001000000000;
    if (v->IsUint16Array()      ) result |= 0x0000002000000000;
    if (v->IsUint32Array()      ) result |= 0x0000004000000000;
    if (v->IsUint32()           ) result |= 0x0000008000000000;
    if (v->IsUint8Array()       ) result |= 0x0000010000000000;
    if (v->IsUint8ClampedArray()) result |= 0x0000020000000000;
    if (v->IsUndefined()        ) result |= 0x0000040000000000;
    if (v->IsWeakMap()          ) result |= 0x0000080000000000;
    if (v->IsWeakSet()          ) result |= 0x0000100000000000;
    return result;
}
