// #include "pl_stats.h"
// #include "pl_util.h"
#include "V8Context.h"
#include "pl_v8.h"

// #define PL_GC_RUNS 2

using namespace v8;

struct FuncData {
    FuncData(V8Context* ctx, SV* func) :
        ctx(ctx), func(func) {}

    V8Context* ctx;
    SV* func;
};

static void perl_caller(const FunctionCallbackInfo<Value>& args)
{
    Local<Function> v8_func = Local<Function>::Cast(args.This());
    // fprintf(stderr, "YES MOTHERFUCKER!\n");
    Isolate* isolate = args.GetIsolate();
    Local<Name> v8_key = String::NewFromUtf8(isolate, "__perl_callback", NewStringType::kNormal).ToLocalChecked();
    Local<External> v8_val = Local<External>::Cast(args.Data());
    FuncData* data = (FuncData*) v8_val->Value();
    // fprintf(stderr, "PTR => %p\n", data);
    // HandleScope handle_scope(data->ctx->isolate);

    SV* ret = 0;

    /* prepare Perl environment for calling the CV */
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);

    /* pass in the stack each of the params we received */
    int nargs = args.Length();
    // fprintf(stderr, "ARGS: %d\n", nargs);
    for (int j = 0; j < nargs; j++) {
        Local<Value> arg = Local<Value>::Cast(args[j]);
        // fprintf(stderr, "GOT ARG %d\n", j);
        Handle<Object> object = Local<Object>::Cast(arg);
        // fprintf(stderr, "GOT HANDLE %d\n", j);
        SV* val = pl_v8_to_perl(aTHX_ data->ctx, object);
        // fprintf(stderr, "GOT PERL %d\n", j);
        mXPUSHs(val);
        // fprintf(stderr, "PUSHED ARG %d\n", j);
    }

    /* call actual Perl CV, passing all params */
    PUTBACK;
    call_sv(data->func, G_SCALAR | G_EVAL);
    SPAGAIN;

    /* get returned value from Perl and return it */
    ret = POPs;
    Handle<Object> object = pl_perl_to_v8(aTHX_ ret, data->ctx);

    args.GetReturnValue().Set(object);

    /* cleanup */
    PUTBACK;
    FREETMPS;
    LEAVE;
}

static SV* pl_v8_to_perl_impl(pTHX_ V8Context* ctx, const Handle<Object>& object, HV* seen)
{
    SV* ret = &PL_sv_undef; /* return undef by default */
    if (object->IsUndefined()) {
        // fprintf(stderr, "V8 UNDEFINED\n");
    }
    else if (object->IsNull()) {
        // fprintf(stderr, "V8 NULL\n");
    }
    else if (object->IsBoolean()) {
        bool val = object->BooleanValue();
        // fprintf(stderr, "V8 BOOLEAN %d\n", (int) val);
        ret = newSViv(val);
    }
    else if (object->IsNumber()) {
        double val = object->NumberValue();
        // fprintf(stderr, "V8 NUMBER %f\n", val);
        ret = newSVnv(val);  /* JS numbers are always doubles */
    }
    else if (object->IsString()) {
        // fprintf(stderr, "STRING BABY\n");
        String::Utf8Value val(ctx->isolate, object);
        // fprintf(stderr, "V8 STRING [%s]\n", *val);
        ret = newSVpvn(*val, val.length());
        SvUTF8_on(ret); /* yes, always */
    }
    else if (object->IsFunction()) {
        // fprintf(stderr, "V8 FUNCTION\n");
        Local<Function> v8_func = Local<Function>::Cast(object);
        Local<Name> v8_key = String::NewFromUtf8(ctx->isolate, "__perl_callback", NewStringType::kNormal).ToLocalChecked();
        Local<External> v8_val = Local<External>::Cast(object->Get(v8_key));
        FuncData* data = (FuncData*) v8_val->Value();
        // fprintf(stderr, "PTR => %p\n", data);
        ret = data->func;
#if 0
        /* if the JS function has a slot with the Perl callback, */
        /* then we know we created it, so we return that */
        if (duk_get_prop_lstring(ctx, pos, PL_SLOT_GENERIC_CALLBACK, sizeof(PL_SLOT_GENERIC_CALLBACK) - 1)) {
            ret = (SV*) duk_get_pointer(ctx, pos);
        }
        duk_pop(ctx); /* pop function / null pointer */
#endif
    }
    else if (object->IsArray()) {
        SV** answer = 0;
#if 0
        void* ptr = duk_get_heapptr(ctx, pos);
        char kstr[100];
        int klen = sprintf(kstr, "%p", ptr);
        answer = hv_fetch(seen, kstr, klen, 0);
#endif
        if (answer) {
            /* TODO: weaken reference? */
            ret = newRV(*answer);
        } else {
            AV* values_array = newAV();
            SV* values = sv_2mortal((SV*) values_array);
#if 0
            if (hv_store(seen, kstr, klen, values, 0)) {
                SvREFCNT_inc(values);
            }
#endif
            ret = newRV(values);

            Handle<Array> array = Handle<Array>::Cast(object);
            int array_top = array->Length();
            // fprintf(stderr, "V8 ARRAY %d\n", array_top);
            for (int j = 0; j < array_top; ++j) {
                Handle<Object> elem = Local<Object>::Cast(array->Get(j));
                // TODO: check we got a valid element
                SV* nested = sv_2mortal(pl_v8_to_perl_impl(aTHX_ ctx, elem, seen));
                if (!nested) {
                    croak("Could not create Perl SV for array\n");
                }
                if (av_store(values_array, j, nested)) {
                    SvREFCNT_inc(nested);
                }
            }
        }
    }
    else if (object->IsObject()) {
        SV** answer = 0;
#if 0
        void* ptr = duk_get_heapptr(ctx, pos);
        char kstr[100];
        int klen = sprintf(kstr, "%p", ptr);
        answer = hv_fetch(seen, kstr, klen, 0);
#endif
        if (answer) {
            /* TODO: weaken reference? */
            ret = newRV(*answer);
        } else {
            HV* values_hash = newHV();
            SV* values = sv_2mortal((SV*) values_hash);
#if 0
            if (hv_store(seen, kstr, klen, values, 0)) {
                SvREFCNT_inc(values);
            }
#endif
            ret = newRV(values);

            Local<Array> property_names = object->GetOwnPropertyNames();
            int hash_top = property_names->Length();
            // fprintf(stderr, "V8 HASH %d\n", hash_top);
            for (int j = 0; j < hash_top; ++j) {
                Local<Value> v8_key = property_names->Get(j);
                // TODO: check we got a valid key
                String::Utf8Value key(ctx->isolate, v8_key->ToString());

                Handle<Object> val = Local<Object>::Cast(object->Get(v8_key));
                // TODO: check we got a valid value

                SV* nested = sv_2mortal(pl_v8_to_perl_impl(aTHX_ ctx, val, seen));
                if (!nested) {
                    croak("Could not create Perl SV for hash\n");
                }
                SV* pkey = newSVpvn(*key, key.length());
                SvUTF8_on(pkey); /* yes, always */
                STRLEN klen = 0;
                const char* kstr = SvPV_const(pkey, klen);
                if (hv_store(values_hash, kstr, -klen, nested, 0)) {
                    SvREFCNT_inc(nested);
                }
            }
        }
    }
    else {
        croak("Don't know how to deal with this thing\n");
    }
    return ret;
}

static const Handle<Object> pl_perl_to_v8_impl(pTHX_ SV* value, V8Context* ctx, HV* seen)
{
    Handle<Object> ret = Local<Object>::Cast(Null(ctx->isolate));
    if (!SvOK(value)) {
        // fprintf(stderr, "PL UNDEF\n");
    } else if (SvIOK(value)) {
        int val = SvIV(value);
        // fprintf(stderr, "PL INT %d\n", val);
        ret = Local<Object>::Cast(Number::New(ctx->isolate, val));
    } else if (SvNOK(value)) {
        double val = SvNV(value);
        // fprintf(stderr, "PL DOUBLE %f\n", val);
        ret = Local<Object>::Cast(Number::New(ctx->isolate, val));
    } else if (SvPOK(value)) {
        STRLEN vlen = 0;
        const char* vstr = SvPV_const(value, vlen);
        // fprintf(stderr, "PL STRING [%*.*s]\n", vlen, vlen, vstr);
        ret = Local<Object>::Cast(String::NewFromUtf8(ctx->isolate, vstr, NewStringType::kNormal).ToLocalChecked());
    } else if (SvROK(value)) {
        SV* ref = SvRV(value);
        if (SvTYPE(ref) == SVt_PVAV) {
            SV** answer = 0;
            AV* values = (AV*) ref;
#if 0
            char kstr[100];
            int klen = sprintf(kstr, "%p", values);
            answer = hv_fetch(seen, kstr, klen, 0);
#endif
            if (answer) {
                void* ptr = (void*) SvUV(*answer);
                // duk_push_heapptr(ctx, ptr);
            } else {
                int array_top = av_top_index(values) + 1;
                // fprintf(stderr, "PL ARRAY %d\n", array_top);
                Handle<Array> array = Array::New(ctx->isolate);
                ret = Local<Object>::Cast(array);
#if 0
                void* ptr = duk_get_heapptr(ctx, array_pos);
                SV* uptr = sv_2mortal(newSVuv(PTR2UV(ptr)));
                if (hv_store(seen, kstr, klen, uptr, 0)) {
                    SvREFCNT_inc(uptr);
                }
#endif
                for (int j = 0; j < array_top; ++j) {
                    SV** elem = av_fetch(values, j, 0);
                    if (!elem || !*elem) {
                        break; /* could not get element */
                    }
                    const Handle<Object> nested = pl_perl_to_v8_impl(aTHX_ *elem, ctx, seen);
                    // TODO: check for validity
                    //  croak("Could not create JS element for array\n");
                    array->Set(j, nested);
                }
            }
        } else if (SvTYPE(ref) == SVt_PVHV) {
            SV** answer = 0;
            HV* values = (HV*) ref;
#if 0
            char kstr[100];
            int klen = sprintf(kstr, "%p", values);
            answer = hv_fetch(seen, kstr, klen, 0);
#endif
            if (answer) {
                void* ptr = (void*) SvUV(*answer);
                // duk_push_heapptr(ctx, ptr);
            } else {
                // fprintf(stderr, "PL HASH\n");
                Local<Context> context = ctx->isolate->GetCurrentContext();
                Handle<Object> object = Object::New(ctx->isolate);
                ret = Local<Object>::Cast(object);
#if 0
                void* ptr = duk_get_heapptr(ctx, hash_pos);
                SV* uptr = sv_2mortal(newSVuv(PTR2UV(ptr)));
                if (hv_store(seen, kstr, klen, uptr, 0)) {
                    SvREFCNT_inc(uptr);
                }
#endif
                hv_iterinit(values);
                while (1) {
                    SV* key = 0;
                    SV* value = 0;
                    char* kstr = 0;
                    STRLEN klen = 0;
                    HE* entry = hv_iternext(values);
                    if (!entry) {
                        break; /* no more hash keys */
                    }
                    key = hv_iterkeysv(entry);
                    if (!key) {
                        continue; /* invalid key */
                    }
                    SvUTF8_on(key); /* yes, always */
                    kstr = SvPV(key, klen);
                    if (!kstr) {
                        continue; /* invalid key */
                    }

                    value = hv_iterval(values, entry);
                    if (!value) {
                        continue; /* invalid value */
                    }
                    SvUTF8_on(value); /* yes, always */ // TODO: only for strings?

                    const Handle<Object> nested = pl_perl_to_v8_impl(aTHX_ value, ctx, seen);
                    // TODO: check for validity
                    //  croak("Could not create JS element for hash\n");

                    Local<Value> v8_key = String::NewFromUtf8(ctx->isolate, kstr, NewStringType::kNormal).ToLocalChecked();
                    object->Set(context, v8_key, nested);
                }
            }
        } else if (SvTYPE(ref) == SVt_PVCV) {
#if 1
            // fprintf(stderr, "PL SUB\n");
            SV* func = newSVsv(value);
            FuncData* data = new FuncData(ctx, func);
            Local<Value> val = External::New(ctx->isolate, data);
            // fprintf(stderr, "CREATED callback value => %p\n", data);

            // Local<ObjectTemplate> object_template = Local<ObjectTemplate>::New(ctx->isolate, ctx->persistent_template);
            Local<FunctionTemplate> ft = FunctionTemplate::New(ctx->isolate, perl_caller, val);
            // fprintf(stderr, "CREATED function template\n");
            Local<Name> v8_key = String::NewFromUtf8(ctx->isolate, "__perl_callback", NewStringType::kNormal).ToLocalChecked();
            // fprintf(stderr, "CREATED callback key\n");
            Local<Function> v8_func = ft->GetFunction();
            // fprintf(stderr, "GOT function\n");
            v8_func->Set(v8_key, val);
            // fprintf(stderr, "SET callback slot\n");
            ret = Local<Object>::Cast(v8_func);
            // fprintf(stderr, "DONE?!?\n");
            // object_template->Set( String::NewFromUtf8(isolate, "print", NewStringType::kNormal).ToLocalChecked(), ft);
#else
            croak("Don't know yet how to deal with a Perl sub\n");
            /* use perl_caller as generic handler, but store the real callback */
            /* in a slot, from where we can later retrieve it */
            SV* func = newSVsv(value);
            duk_push_c_function(ctx, perl_caller, DUK_VARARGS);
            if (!func) {
                croak("Could not create copy of Perl callback\n");
            }
            duk_push_pointer(ctx, func);
            if (! duk_put_prop_lstring(ctx, -2, PL_SLOT_GENERIC_CALLBACK, sizeof(PL_SLOT_GENERIC_CALLBACK) - 1)) {
                croak("Could not associate C dispatcher and Perl callback\n");
            }
#endif
        } else {
            croak("Don't know how to deal with an undetermined Perl reference\n");
        }
    } else {
        croak("Don't know how to deal with an undetermined Perl object\n");
    }
    return ret;
}

SV* pl_v8_to_perl(pTHX_ V8Context* ctx, const Handle<Object>& object)
{
    HV* seen = newHV();
    SV* ret = pl_v8_to_perl_impl(aTHX_ ctx, object, seen);
    hv_undef(seen);
    return ret;
}

const Handle<Object> pl_perl_to_v8(pTHX_ SV* value, V8Context* ctx)
{
    HV* seen = newHV();
    Handle<Object> ret = pl_perl_to_v8_impl(aTHX_ value, ctx, seen);
    hv_undef(seen);
    return ret;
}

static int find_last_dot(const char* name, int* len)
{
    int last_dot = -1;
    *len = 0;
    for (; name[*len] != '\0'; ++*len) {
        if (name[*len] == '.') {
            last_dot = *len;
        }
    }
    return last_dot;
}

SV* pl_get_global_or_property(pTHX_ V8Context* ctx, const char* name)
{
    SV* ret = &PL_sv_undef; /* return undef by default */

    Isolate::Scope isolate_scope(ctx->isolate);
    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    int len = 0;
    int last_dot = find_last_dot(name, &len);
    if (last_dot < 0) {
        Local<Value> v8_name = String::NewFromUtf8(ctx->isolate, name, NewStringType::kNormal).ToLocalChecked();
        Local<Value> value = context->Global()->Get(v8_name);
        Handle<Object> object = Local<Object>::Cast(value);
        ret = pl_v8_to_perl(aTHX_ ctx, object);
    } else {
        // TODO
    }

    return ret;
}

int pl_set_global_or_property(pTHX_ V8Context* ctx, const char* name, SV* value)
{
    int ret = 0;

    Isolate::Scope isolate_scope(ctx->isolate);
    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    int len = 0;
    int last_dot = find_last_dot(name, &len);
    if (last_dot < 0) {
        Local<Value> v8_name = String::NewFromUtf8(ctx->isolate, name, NewStringType::kNormal).ToLocalChecked();
        Handle<Object> object = pl_perl_to_v8(aTHX_ value, ctx);
        context->Global()->Set(v8_name, object);
        ret = 1;
    } else {
        // TODO
    }

    return ret;
}

SV* pl_exists_global_or_property(pTHX_ V8Context* ctx, const char* name)
{
    SV* ret = &PL_sv_no; /* return false by default */

    Isolate::Scope isolate_scope(ctx->isolate);
    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    int len = 0;
    int last_dot = find_last_dot(name, &len);
    if (last_dot < 0) {
        Local<Value> v8_name = String::NewFromUtf8(ctx->isolate, name, NewStringType::kNormal).ToLocalChecked();
        if (context->Global()->Has(v8_name)) {
            ret = &PL_sv_yes;
        }
    } else {
        // TODO
    }
    return ret;
}

#if 0
static const char* get_typeof(duk_context* ctx, int pos)
{
    const char* label = "undefined";
    switch (duk_get_type(ctx, pos)) {
        case DUK_TYPE_NONE:
        case DUK_TYPE_UNDEFINED:
            break;
        case DUK_TYPE_NULL:
            label = "null";
            break;
        case DUK_TYPE_BOOLEAN:
            label = "boolean";
            break;
        case DUK_TYPE_NUMBER:
            label = "number";
            break;
        case DUK_TYPE_STRING:
            label = "string";
            break;
        case DUK_TYPE_OBJECT:
            if (duk_is_array(ctx, pos)) {
                label = "array";
            }
            else if (duk_is_symbol(ctx, pos)) {
                label = "symbol";
            }
            else if (duk_is_pointer(ctx, pos)) {
                label = "pointer";
            }
            else if (duk_is_function(ctx, pos)) {
                label = "function";
            }
            else if (duk_is_c_function(ctx, pos)) {
                label = "c_function";
            }
            else if (duk_is_thread(ctx, pos)) {
                label = "thread";
            }
            else {
                label = "object";
            }
            break;
        case DUK_TYPE_POINTER:
            label = "pointer";
            break;
        case DUK_TYPE_BUFFER:
            label = "buffer";
            break;
        case DUK_TYPE_LIGHTFUNC:
            label = "lightfunc";
            break;
        default:
            croak("Don't know how to deal with an undetermined JS object\n");
            break;
    }
    return label;
}

SV* pl_typeof_global_or_property(pTHX_ duk_context* ctx, const char* name)
{
    const char* cstr = "undefined";
    STRLEN clen = 0;
    SV* ret = 0;
    if (find_global_or_property(ctx, name)) {
        cstr = get_typeof(ctx, -1);
        duk_pop(ctx); /* pop value */
    }
    ret = newSVpv(cstr, clen);
    return ret;
}

SV* pl_instanceof_global_or_property(pTHX_ duk_context* ctx, const char* object, const char* class)
{
    SV* ret = &PL_sv_no; /* return false by default */
    if (find_global_or_property(ctx, object)) {
        if (find_global_or_property(ctx, class)) {
            if (duk_instanceof(ctx, -2, -1)) {
                ret = &PL_sv_yes;
            }
            duk_pop(ctx);
        }
        duk_pop(ctx);
    }
    return ret;
}

SV* pl_eval(pTHX_ Duk* duk, const char* js, const char* file)
{
    SV* ret = &PL_sv_undef; /* return undef by default */
    duk_context* ctx = duk->ctx;
    Stats stats;
    duk_uint_t flags = 0;
    duk_int_t rc = 0;

    /* flags |= DUK_COMPILE_STRICT; */

    pl_stats_start(aTHX_ duk, &stats);
    if (!file) {
        rc = duk_pcompile_string(ctx, flags, js);
    }
    else {
        duk_push_string(ctx, file);
        rc = duk_pcompile_string_filename(ctx, flags, js);
    }
    pl_stats_stop(aTHX_ duk, &stats, "compile");

    if (rc != DUK_EXEC_SUCCESS) {
        croak("JS could not compile code: %s\n", duk_safe_to_string(ctx, -1));
    }

    pl_stats_start(aTHX_ duk, &stats);
    rc = duk_pcall(ctx, 0);
    pl_stats_stop(aTHX_ duk, &stats, "run");
    check_duktape_call_for_errors(rc, ctx);

    ret = pl_duk_to_perl(aTHX_ ctx, -1);
    duk_pop(ctx);
    return ret;
}

int pl_run_gc(Duk* duk)
{
    int j = 0;

    /*
     * From docs in http://duktape.org/api.html#duk_gc
     *
     * You may want to call this function twice to ensure even objects with
     * finalizers are collected.  Currently it takes two mark-and-sweep rounds
     * to collect such objects.  First round marks the object as finalizable
     * and runs the finalizer.  Second round ensures the object is still
     * unreachable after finalization and then frees the object.
     */
    duk_context* ctx = duk->ctx;
    for (j = 0; j < PL_GC_RUNS; ++j) {
        /* DUK_GC_COMPACT: Force object property table compaction */
        duk_gc(ctx, DUK_GC_COMPACT);
    }
    return PL_GC_RUNS;
}
#endif
