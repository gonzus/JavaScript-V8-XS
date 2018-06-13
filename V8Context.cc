#include "libplatform/libplatform.h"
#include "V8Context.h"

using namespace v8;

int V8Context::v8_initialized = 0;

V8Context::V8Context(const char* flags)
{
    fprintf(stderr, "V8 construct\n");
    V8Context::initialize_v8();
    fprintf(stderr, "V8 construct done\n");
}

V8Context::~V8Context()
{
    fprintf(stderr, "V8 destruct\n");
    fprintf(stderr, "V8 destruct done\n");
}

void V8Context::set_flags_from_string(const char *str)
{
    V8::SetFlagsFromString(str, strlen(str));
}

void V8Context::initialize_v8()
{
    if (v8_initialized) {
        return;
    }
    v8_initialized = 1;

    fprintf(stderr, "V8 initializing\n");
    const char* prog = "foo";
    v8::V8::InitializeICUDefaultLocation(prog);
    v8::V8::InitializeExternalStartupData(prog);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    fprintf(stderr, "V8 initializing done\n");
}
