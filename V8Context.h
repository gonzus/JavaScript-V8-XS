#ifndef V8CONTEXT_H_
#define V8CONTEXT_H_

#include <v8.h>
#include "pl_config.h"
#include "pl_v8.h"

using namespace v8;

class V8Context {
    public:
        V8Context(const char* flags = NULL);
        ~V8Context();

        SV* get(const char* name);
        void set(const char* name, SV* value);

        int eval(const char* code, const char* file = 0);

        Isolate* isolate;
        Persistent<Context> persistent_context;

        static uint64_t GetTypeFlags(const Local<Value>& v);
    private:
        void initialize_v8();
        void terminate_v8();
        Handle<Array> CreateArray(int nested);
        Handle<Object> CreateObject(int nested);
        void DumpObject(const Handle<Object>& object, int level = 0);

        std::unique_ptr<Platform> platform;
        Isolate::CreateParams create_params;
};

#endif
