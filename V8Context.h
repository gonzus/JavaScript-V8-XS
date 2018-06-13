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

class V8Context {
    public:
        V8Context(const char* flags = NULL);
        ~V8Context();

        void set_flags_from_string(const char *str);
    private:
        static int v8_initialized;
        static void initialize_v8();
};

#endif
