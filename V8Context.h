#ifndef V8CONTEXT_H_
#define V8CONTEXT_H_

#include <v8.h>

#ifdef __cplusplus
extern "C" {
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include "ppport.h"
}
#endif

#ifdef New
#undef New
#endif
#ifdef Null
#undef Null
#endif
#ifdef do_open
#undef do_open
#endif
#ifdef do_close
#undef do_close
#endif
#ifdef IsSet
#undef IsSet
#endif

class V8Context {
    public:
        V8Context(const char* flags = NULL);
        ~V8Context();

        int eval(const char* code, const char* file = 0);

    private:
        void initialize_v8();
        void terminate_v8();
        v8::Handle<v8::Array> CreateArray(int nested);
        v8::Handle<v8::Object> CreateObject(int nested);
        void DumpObject(const v8::Handle<v8::Object>& object, int level = 0);

        std::unique_ptr<v8::Platform> platform;
        v8::Isolate::CreateParams create_params;
        v8::Isolate* isolate;
};

#endif
