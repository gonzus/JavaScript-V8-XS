#ifndef V8CONTEXT_H_
#define V8CONTEXT_H_

#include <v8.h>
#include "pl_config.h"
#include "pl_v8.h"

using namespace v8;

class V8Context {
    public:
        V8Context(const char* program = 0);
        ~V8Context();

        SV* get(const char* name);
        SV* exists(const char* name);
        SV* typeof(const char* name);

        void set(const char* name, SV* value);
        SV* eval(const char* code, const char* file = 0);

        int run_gc();

        Isolate* isolate;
        Persistent<Context> persistent_context;
        Persistent<ObjectTemplate> persistent_template;

        static uint64_t GetTypeFlags(const Local<Value>& v);
    private:
        static void initialize_v8(V8Context* self);
        static void terminate_v8(V8Context* self);
        static int instance_count;
        static std::unique_ptr<Platform> platform;

        Handle<Array> CreateArray(int nested);
        Handle<Object> CreateObject(int nested);
        void DumpObject(const Handle<Object>& object, int level = 0);

        char* program;
        Isolate::CreateParams create_params;
};

#endif
