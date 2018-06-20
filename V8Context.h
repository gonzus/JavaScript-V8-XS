#ifndef V8CONTEXT_H_
#define V8CONTEXT_H_

#include <v8.h>
#include "pl_config.h"
#include "pl_v8.h"

#define V8_OPT_NAME_GATHER_STATS      "gather_stats"
#define V8_OPT_NAME_SAVE_MESSAGES     "save_messages"
#define V8_OPT_NAME_MAX_MEMORY_BYTES  "max_memory_bytes"
#define V8_OPT_NAME_MAX_TIMEOUT_US    "max_timeout_us"

#define V8_OPT_FLAG_GATHER_STATS      0x01
#define V8_OPT_FLAG_SAVE_MESSAGES     0x02
#define V8_OPT_FLAG_MAX_MEMORY_BYTES  0x04
#define V8_OPT_FLAG_MAX_TIMEOUT_US    0x08

using namespace v8;

class V8Context {
    public:
        V8Context(HV* opt);
        ~V8Context();

        SV* get(const char* name);
        SV* exists(const char* name);
        SV* typeof(const char* name);

        void set(const char* name, SV* value);
        SV* eval(const char* code, const char* file = 0);

        SV* dispatch_function_in_event_loop(const char* func);

        int run_gc();

        HV* get_stats();
        void reset_stats();

        HV* get_msgs();
        void reset_msgs();

        Isolate* isolate;
        Persistent<Context> persistent_context;
        Persistent<ObjectTemplate> persistent_template;

        uint64_t flags;
        HV* stats;
        HV* msgs;
        long pagesize_bytes;

        static uint64_t GetTypeFlags(const Local<Value>& v);
    private:
        void register_functions();

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
