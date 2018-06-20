#include "pl_stats.h"
#include "pl_console.h"
#include "pl_v8.h"

#define PL_GC_RUNS 2

using namespace v8;

struct FuncData {
    FuncData(V8Context* ctx, SV* func) :
        ctx(ctx), func(newSVsv(func)) {}

    V8Context* ctx;
    SV* func;
};

static const char* get_typeof(V8Context* ctx, const Handle<Object>& object);

static void perl_caller(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = args.GetIsolate();
    Local<Function> v8_func = Local<Function>::Cast(args.This());
#if 1
    Local<External> v8_val = Local<External>::Cast(args.Data());
#else
    // If args.This() returned the same bject as GetFunction() on the function
    // template we used to create the function, this would work; alas, it
    // doesn't work, so we have to pass the data we want so that args.Data()
    // can return it.
    Local<Name> v8_key = String::NewFromUtf8(isolate, "__perl_callback", NewStringType::kNormal).ToLocalChecked();
    Local<External> v8_val = Local<External>::Cast(v8_func->Get(v8_key));
#endif
    FuncData* data = (FuncData*) v8_val->Value();

    SV* ret = 0;

    /* prepare Perl environment for calling the CV */
    dTHX;
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);

    /* pass in the stack each of the params we received */
    int nargs = args.Length();
    for (int j = 0; j < nargs; j++) {
        Local<Value> arg = Local<Value>::Cast(args[j]);
        Handle<Object> object = Local<Object>::Cast(arg);
        SV* val = pl_v8_to_perl(aTHX_ data->ctx, object);
        mXPUSHs(val);
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
    }
    else if (object->IsNull()) {
    }
    else if (object->IsBoolean()) {
        bool val = object->BooleanValue();
        ret = newSViv(val);
    }
    else if (object->IsNumber()) {
        double val = object->NumberValue();
        ret = newSVnv(val);  /* JS numbers are always doubles */
    }
    else if (object->IsString()) {
        String::Utf8Value val(ctx->isolate, object);
        ret = newSVpvn(*val, val.length());
        SvUTF8_on(ret); /* yes, always */
    }
    else if (object->IsFunction()) {
        Local<Function> v8_func = Local<Function>::Cast(object);
        Local<Name> v8_key = String::NewFromUtf8(ctx->isolate, "__perl_callback", NewStringType::kNormal).ToLocalChecked();
        Local<External> v8_val = Local<External>::Cast(object->Get(v8_key));
        FuncData* data = (FuncData*) v8_val->Value();
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
    } else if (SvIOK(value)) {
        int val = SvIV(value);
        ret = Local<Object>::Cast(Number::New(ctx->isolate, val));
    } else if (SvNOK(value)) {
        double val = SvNV(value);
        ret = Local<Object>::Cast(Number::New(ctx->isolate, val));
    } else if (SvPOK(value)) {
        STRLEN vlen = 0;
        const char* vstr = SvPV_const(value, vlen);
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
                    Maybe<bool> check = object->Set(context, v8_key, nested);
                    if (check.IsNothing() || !check.FromMaybe(false)) {
                        croak("Could not set JS element for object\n");
                    }
                }
            }
        } else if (SvTYPE(ref) == SVt_PVCV) {
#if 1
            FuncData* data = new FuncData(ctx, value);
            Local<Value> val = External::New(ctx->isolate, data);
            Local<FunctionTemplate> ft = FunctionTemplate::New(ctx->isolate, perl_caller, val);
            Local<Name> v8_key = String::NewFromUtf8(ctx->isolate, "__perl_callback", NewStringType::kNormal).ToLocalChecked();
            Local<Function> v8_func = ft->GetFunction();
            v8_func->Set(v8_key, val);
            ret = Local<Object>::Cast(v8_func);
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

    Local<Object> object;
    bool found = find_object(ctx, name, context, object);
    if (found) {
        ret = pl_v8_to_perl(aTHX_ ctx, object);
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

    Local<Object> object;
    Local<Value> slot;
    bool found = find_parent(ctx, name, context, object, slot);
    if (found) {
        Handle<Object> v8_value = pl_perl_to_v8(aTHX_ value, ctx);
        object->Set(slot, v8_value);
        ret = 1;
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

    Local<Object> object;
    bool found = find_object(ctx, name, context, object);
    if (found) {
        ret = &PL_sv_yes;
    }

    return ret;
}

SV* pl_typeof_global_or_property(pTHX_ V8Context* ctx, const char* name)
{
    const char* cstr = "undefined";

    Isolate::Scope isolate_scope(ctx->isolate);
    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    Local<Object> object;
    bool found = find_object(ctx, name, context, object);
    if (found) {
        cstr = get_typeof(ctx, object);
    }

    STRLEN clen = 0;
    SV* ret = newSVpv(cstr, clen);
    return ret;
}

SV* pl_instanceof_global_or_property(pTHX_ V8Context* ctx, const char* oname, const char* cname)
{
#if 1
    // TODO: hack alert
    // This is here only because the real implementation below doesn't work
    char code[1024];
    sprintf(code, "%s instanceof %s", oname, cname);
    return pl_eval(aTHX_ ctx, code);
#else
    SV* ret = &PL_sv_no; /* return false by default */

    Isolate::Scope isolate_scope(ctx->isolate);
    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    Local<Object> object;
    bool found = find_object(ctx, oname, context, object);
    if (found) {
        Local<FunctionTemplate> ft = FunctionTemplate::New(ctx->isolate);
        Local<String> v8_name = String::NewFromUtf8(ctx->isolate, cname, NewStringType::kNormal).ToLocalChecked();
        ft->SetClassName(v8_name);
        if (ft->HasInstance(object)) {
            ret = &PL_sv_yes;
        }
        else {
            fprintf(stderr, "GONZO [%s]\n", cname);
        }
    }

    return ret;
#endif
}

SV* pl_eval(pTHX_ V8Context* ctx, const char* code, const char* file)
{
    SV* ret = &PL_sv_undef; /* return undef by default */

    Isolate::Scope isolate_scope(ctx->isolate);
    HandleScope handle_scope(ctx->isolate);

    Local<Context> context = Local<Context>::New(ctx->isolate, ctx->persistent_context);
    Context::Scope context_scope(context);

    TryCatch try_catch(ctx->isolate);
    bool ok = true;
    do {
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
        ok = Script::Compile(context, source).ToLocal(&script);
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
        Handle<Object> object = Local<Object>::Cast(result);
        ret = pl_v8_to_perl(aTHX_ ctx, object);
    } while (0);
    if (!ok) {
        String::Utf8Value error(ctx->isolate, try_catch.Exception());
        pl_show_error(ctx, *error);
        return ret;
    }

    return ret;
}

int pl_run_gc(V8Context* ctx)
{
    // Run PL_GC_RUNS GC rounds
    for (int j = 0; j < PL_GC_RUNS; ++j) {
        ctx->isolate->LowMemoryNotification();
    }
    return PL_GC_RUNS;
}

bool find_parent(V8Context* ctx, const char* name, Local<Context>& context, Local<Object>& object, Local<Value>& slot, int create)
{
    int start = 0;
    object = context->Global();
    bool found = false;
    while (1) {
        int pos = start;
        while (name[pos] != '\0' && name[pos] != '.') {
            ++pos;
        }
        int length = pos - start;
        if (length <= 0) {
            // invalid name
            break;
        }
        slot = String::NewFromUtf8(ctx->isolate, name + start, NewStringType::kNormal, length).ToLocalChecked();
        if (name[pos] == '\0') {
            // final element, we are done
            found = true;
            break;
        }
        Local<Value> child;
        if (object->Has(slot)) {
            // object has a slot with that name
            child = object->Get(slot);
        }
        else if (!create) {
            // we must not create the missing slot, we are done
            break;
        }
        else {
            // create the missing slot and go on
            child = Object::New(ctx->isolate);
            object->Set(slot, child);
        }
        object = Local<Object>::Cast(child);
        if (!child->IsObject()) {
            // child in slot is not an object
            break;
        }
        start = pos + 1;
    }

    return found;
}

bool find_object(V8Context* ctx, const char* name, Local<Context>& context, Local<Object>& object)
{
    Local<Value> slot;
    if (!find_parent(ctx, name, context, object, slot)) {
        // could not find parent
        return false;
    }
    if (!object->Has(slot)) {
        // parent doesn't have a slot with that name
        return false;
    }
    Local<Value> child = object->Get(slot);
    object = Local<Object>::Cast(child);
    return true;
}

static const char* get_typeof(V8Context* ctx, const Handle<Object>& object)
{
    const char* label = "undefined";

    if (object->IsUndefined()) {
    }
    else if (object->IsNull()) {
        label = "null";
    }
    else if (object->IsBoolean()) {
        label = "boolean";
    }
    else if (object->IsNumber()) {
        label = "number";
    }
    else if (object->IsString()) {
        label = "string";
    }
    else if (object->IsArray()) {
        label = "array";
    }
    else if (object->IsSymbol()) {
        label = "symbol";
    }
    else if (object->IsExternal()) {
        label = "pointer";
    }
    else if (object->IsFunction()) {
        label = "function";
    }
    else if (object->IsObject()) {
        label = "object";
    }

    return label;

#if 0
    if (v->IsArgumentsObject()  ) result |= 0x0000000000000001;
    if (v->IsArrayBuffer()      ) result |= 0x0000000000000002;
    if (v->IsArrayBufferView()  ) result |= 0x0000000000000004;
    if (v->IsBooleanObject()    ) result |= 0x0000000000000010;
    if (v->IsDataView()         ) result |= 0x0000000000000040;
    if (v->IsDate()             ) result |= 0x0000000000000080;
    if (v->IsFalse()            ) result |= 0x0000000000000200;
    if (v->IsFloat32Array()     ) result |= 0x0000000000000400;
    if (v->IsFloat64Array()     ) result |= 0x0000000000000800;
    if (v->IsGeneratorFunction()) result |= 0x0000000000002000;
    if (v->IsGeneratorObject()  ) result |= 0x0000000000004000;
    if (v->IsInt16Array()       ) result |= 0x0000000000008000;
    if (v->IsInt32Array()       ) result |= 0x0000000000010000;
    if (v->IsInt32()            ) result |= 0x0000000000020000;
    if (v->IsInt8Array()        ) result |= 0x0000000000040000;
    if (v->IsMapIterator()      ) result |= 0x0000000000080000;
    if (v->IsMap()              ) result |= 0x0000000000100000;
    if (v->IsName()             ) result |= 0x0000000000200000;
    if (v->IsNativeError()      ) result |= 0x0000000000400000;
    if (v->IsNumberObject()     ) result |= 0x0000000001000000;
    if (v->IsPromise()          ) result |= 0x0000000008000000;
    if (v->IsRegExp()           ) result |= 0x0000000010000000;
    if (v->IsSetIterator()      ) result |= 0x0000000020000000;
    if (v->IsSet()              ) result |= 0x0000000040000000;
    if (v->IsStringObject()     ) result |= 0x0000000080000000;
    if (v->IsSymbolObject()     ) result |= 0x0000000200000000;
    if (v->IsTrue()             ) result |= 0x0000000800000000;
    if (v->IsTypedArray()       ) result |= 0x0000001000000000;
    if (v->IsUint16Array()      ) result |= 0x0000002000000000;
    if (v->IsUint32Array()      ) result |= 0x0000004000000000;
    if (v->IsUint32()           ) result |= 0x0000008000000000;
    if (v->IsUint8Array()       ) result |= 0x0000010000000000;
    if (v->IsUint8ClampedArray()) result |= 0x0000020000000000;
    if (v->IsWeakMap()          ) result |= 0x0000080000000000;
    if (v->IsWeakSet()          ) result |= 0x0000100000000000;
#endif
}
