#ifndef C_EVENTLOOP_H_
#define C_EVENTLOOP_H_

#include "V8Context.h"

int pl_register_eventloop_functions(V8Context* ctx, Local<ObjectTemplate>& object_template);
SV* pl_run_function_in_event_loop(pTHX_ V8Context* ctx, const char* func);

#endif
