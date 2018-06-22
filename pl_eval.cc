#include <map>
#include "pl_stats.h"
#include "pl_console.h"
#include "pl_v8.h"

#define PL_GC_RUNS 2

using namespace v8;

static const char* ToCString(const String::Utf8Value& value)
{
  return *value ? *value : "<string conversion failed>";
}

static void ReportException(Isolate* isolate, TryCatch* try_catch)
{
    HandleScope handle_scope(isolate);
    String::Utf8Value exception(isolate, try_catch->Exception());
    const char* exception_string = ToCString(exception);
    Local<Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        fprintf(stderr, "%s\n", exception_string);
    } else {
        // Print (filename):(line number): (message).
        String::Utf8Value filename(isolate,
                message->GetScriptOrigin().ResourceName());
        Local<Context> context(isolate->GetCurrentContext());
        const char* filename_string = ToCString(filename);
        int linenum = message->GetLineNumber(context).FromJust();
        fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
        // Print line of source code.
        String::Utf8Value sourceline(
                isolate, message->GetSourceLine(context).ToLocalChecked());
        const char* sourceline_string = ToCString(sourceline);
        fprintf(stderr, "%s\n", sourceline_string);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }
        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\n");
        Local<Value> stack_trace_string;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
                stack_trace_string->IsString() &&
                Local<String>::Cast(stack_trace_string)->Length() > 0) {
            String::Utf8Value stack_trace(isolate, stack_trace_string);
            const char* stack_trace_string = ToCString(stack_trace);
            fprintf(stderr, "%s\n", stack_trace_string);
        }
    }
}

SV* pl_eval(pTHX_ V8Context* ctx, const char* code, const char* file)
{
    SV* ret = &PL_sv_undef; /* return undef by default */

    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);
    ScriptOrigin* origin = 0;

    TryCatch try_catch(ctx->isolate);
    bool ok = true;
    do {
        if (file) {
            // Create a string containing the file name.
            Local<String> name;
            ok = String::NewFromUtf8(ctx->isolate, file, NewStringType::kNormal).ToLocal(&name);
            if (!ok) {
                break;
            }
            origin = new ScriptOrigin(name);
            if (!origin) {
                break;
            }
        }

        // Create a string containing the JavaScript source code.
        Local<String> source;
        ok = String::NewFromUtf8(ctx->isolate, code, NewStringType::kNormal).ToLocal(&source);
        if (!ok) {
            break;
        }

        Perf perf;

        // Compile the source code.
        pl_stats_start(aTHX_ ctx, &perf);
        Local<Script> script;
        ok = Script::Compile(context, source, origin).ToLocal(&script);
        pl_stats_stop(aTHX_ ctx, &perf, "compile");
        if (!ok) {
            break;
        }

        // Run the script to get the result.
        pl_stats_start(aTHX_ ctx, &perf);
        Local<Value> result;
        ok = script->Run(context).ToLocal(&result);
        pl_stats_stop(aTHX_ ctx, &perf, "run");
        if (!ok) {
            break;
        }

        // Convert the result into Perl data
        Local<Object> object = Local<Object>::Cast(result);
        ret = pl_v8_to_perl(aTHX_ ctx, object);
    } while (0);
    if (!ok) {
        if (try_catch.HasCaught()) {
            fprintf(stderr, "REPORT exception\n");
            ReportException(ctx->isolate, &try_catch);

            fprintf(stderr, "SHOW error\n");
            String::Utf8Value error(ctx->isolate, try_catch.Exception());
            pl_show_error(ctx, *error);
        }
    }
    delete origin;
    return ret;
}

int pl_run_function(V8Context* ctx, Persistent<Function>& func)
{
    HandleScope handle_scope(ctx->isolate);
    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    TryCatch try_catch(ctx->isolate);
    Local<Value> result;
    bool ok = true;
    do {
        Local<Value> global = context->Global();
        Local<Function> v8_func = Local<Function>::New(ctx->isolate, func);

        ok = v8_func->Call(context, global, 0, 0).ToLocal(&result);
        if (!ok) {
            break;
        }
    } while (0);
    if (!ok) {
        if (try_catch.HasCaught()) {
            fprintf(stderr, "REPORT exception\n");
            ReportException(ctx->isolate, &try_catch);

            fprintf(stderr, "SHOW error\n");
            String::Utf8Value error(ctx->isolate, try_catch.Exception());
            pl_show_error(ctx, *error);
        }
    }
    return ok;
}
