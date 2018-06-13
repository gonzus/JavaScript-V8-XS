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

class V8Context {
    public:
        V8Context(const char* flags = NULL);
        ~V8Context();

        int run(const char* code);

        void set_flags_from_string(const char *str);
    private:
        void initialize_v8();
        void terminate_v8();

        std::unique_ptr<v8::Platform> platform;
        v8::Isolate::CreateParams create_params;
        v8::Isolate* isolate;
};

#endif
