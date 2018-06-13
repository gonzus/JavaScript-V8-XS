#include "libplatform/libplatform.h"
#include "V8Context.h"

V8Context::V8Context(const char* flags)
{
    fprintf(stderr, "V8 construct\n");
    V8Context::initialize_v8();

    // Create a new Isolate and make it the current one.
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    fprintf(stderr, "V8 created allocator\n");
    isolate = v8::Isolate::New(create_params);
    fprintf(stderr, "V8 created isolate\n");
    fprintf(stderr, "V8 construct done\n");
}

V8Context::~V8Context()
{
    fprintf(stderr, "V8 destruct\n");
    delete create_params.array_buffer_allocator;
    V8Context::terminate_v8();
    fprintf(stderr, "V8 destruct done\n");
}

int V8Context::run(const char* code)
{
    v8::Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, code, v8::NewStringType::kNormal)
        .ToLocalChecked();

    // Compile the source code.
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();

    // Run the script to get the result.
    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

    // Convert the result to an UTF8 string and print it.
    v8::String::Utf8Value utf8(isolate, result);
    fprintf(stderr, "GONZO: [%s]\n", *utf8);
    return 0;
}

void V8Context::set_flags_from_string(const char *str)
{
    v8::V8::SetFlagsFromString(str, strlen(str));
}

void V8Context::initialize_v8()
{
    fprintf(stderr, "V8 initializing\n");
    const char* prog = "foo";
    v8::V8::InitializeICUDefaultLocation(prog);
    v8::V8::InitializeExternalStartupData(prog);
    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    fprintf(stderr, "V8 initializing done\n");
}

void V8Context::terminate_v8()
{
    fprintf(stderr, "V8 terminating\n");
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    fprintf(stderr, "V8 terminating done\n");
}
