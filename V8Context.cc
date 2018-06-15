#include "libplatform/libplatform.h"
#include "V8Context.h"

#define ELEMS 3

V8Context::V8Context(const char* flags)
{
    // fprintf(stderr, "V8 construct\n");
    V8Context::initialize_v8();

    // Create a new Isolate and make it the current one.
    create_params.array_buffer_allocator =
        ArrayBuffer::Allocator::NewDefaultAllocator();
    // fprintf(stderr, "V8 created allocator\n");
    isolate = Isolate::New(create_params);
    // fprintf(stderr, "V8 created isolate\n");
    // fprintf(stderr, "V8 construct done\n");

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    // Create a new context.
    Local<Context> context = Context::New(isolate);
    persistent_context.Reset(isolate, context);
}

V8Context::~V8Context()
{
    delete create_params.array_buffer_allocator;
    V8Context::terminate_v8();
}

SV* V8Context::get(const char* name)
{
    return pl_get_global_or_property(aTHX_ this, name);
}

void V8Context::set(const char* name, SV* value)
{
    pl_set_global_or_property(aTHX_ this, name, value);
}

int V8Context::eval(const char* code, const char* file)
{
    // Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);

    fprintf(stderr, "creating copy of context\n");
    Local<Context> context = Local<Context>::New(isolate, persistent_context);
    Context::Scope context_scope(context);
    fprintf(stderr, "created copy of context\n");

#if 1
    // Create a string containing the JavaScript source code.
    Local<String> source =
        String::NewFromUtf8(isolate, code, NewStringType::kNormal)
        .ToLocalChecked();

    // Compile the source code.
    Local<Script> script =
        Script::Compile(context, source).ToLocalChecked();

    // Run the script to get the result.
    Local<Value> result = script->Run(context).ToLocalChecked();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(isolate, result);
    fprintf(stderr, "GONZO: [%s]\n", *utf8);
#endif

#if 0
    Handle<Array> a = CreateArray(2);
    fprintf(stderr, "GONZO: GOT ARRAY with %d elements\n", a->Length());
    DumpObject(a);

    Handle<Object> o = CreateObject(2);
    fprintf(stderr, "GONZO: GOT OBJECT\n");
    DumpObject(o);
#endif

    return 0;
}

#if 0
Handle<Array> V8Context::CreateArray(int nested)
{
    // We will be creating temporary handles so we use a handle scope.
    EscapableHandleScope handle_scope(isolate);

    // Create a new empty array.
    Handle<Array> array = Array::New(isolate);

    // Return an empty result if there was an error creating the array.
    if (array.IsEmpty()) {
        fprintf(stderr, "%2d: Returning empty array\n", nested);
        return Handle<Array>();
    }

    // Fill out some values
    int j = 0;
    for (; j < ELEMS; ++j) {
        int v = j*(10+nested);
        array->Set(j, Integer::New(isolate, v));
        fprintf(stderr, "%2d: Added element %2d => %3d\n", nested, j, v);
    }
    array->Set(j, Boolean::New(isolate, (j%2) == 0)); ++j;
    array->Set(j, Boolean::New(isolate, (j%2) == 0)); ++j;
    switch (nested) {
        case 2: {
            Handle<Array> a = CreateArray(1);
            array->Set(j, a);
            fprintf(stderr, "%2d: Added nested array %2d\n", nested, j);
            break;
        }
        case 1: {
            Handle<Object> o = CreateObject(0);
            array->Set(j, o);
            fprintf(stderr, "%2d: Added nested object %2d\n", nested, j);
            break;
        }
        default:
            break;
    }

    // Return the value through Close.
    return handle_scope.Escape(array);
}

Handle<Object> V8Context::CreateObject(int nested)
{
    // We will be creating temporary handles so we use a handle scope.
    EscapableHandleScope handle_scope(isolate);

    // Create a new empty object.
    Handle<Object> object = Object::New(isolate);

    // Return an empty result if there was an error creating the array.
    if (object.IsEmpty()) {
        fprintf(stderr, "%2d: Returning empty object\n", nested);
        return Handle<Object>();
    }

    Local<Context> context = isolate->GetCurrentContext();

    int j = 0;
    for (j = 0; j < ELEMS; ++j) {
        char k[256];
        int v = j*(10+nested);
        sprintf(k, "key_%d", j);
        Local<Value> key = String::NewFromUtf8(isolate, k, NewStringType::kNormal)
            .ToLocalChecked();
        Local<Value> val = Integer::New(isolate, v);
        object->Set(context, key, val);
        fprintf(stderr, "%2d: Set key [%s] to value %d\n", nested, k, v);
    }
    switch (nested) {
        case 2: {
            Handle<Object> o = CreateObject(1);
            Local<Value> key = String::NewFromUtf8(isolate, "nested", NewStringType::kNormal)
                .ToLocalChecked();
            object->Set(context, key, o);
            fprintf(stderr, "%2d: Added nested object\n", nested);
            break;
        }
        case 1: {
            Handle<Array> a = CreateArray(0);
            Local<Value> key = String::NewFromUtf8(isolate, "array", NewStringType::kNormal)
                .ToLocalChecked();
            object->Set(context, key, a);
            fprintf(stderr, "%2d: Added nested array\n", nested);
            break;
        }
        default:
            break;
    }
    if (nested) {
    }

    // Return the value through Close.
    return handle_scope.Escape(object);
}
#endif

void V8Context::DumpObject(const Handle<Object>& object, int level)
{
    // uint64_t mask = GetTypeFlags(object);
    // fprintf(stderr, "%2d: OBJECT MASK: %llx\n", level, mask);

    if (object->IsUndefined()) {
        fprintf(stderr, "UNDEFINED");
    }
    else if (object->IsNull()) {
        fprintf(stderr, "NULL");
    }
    else if (object->IsBoolean()) {
        bool value = object->BooleanValue();
        fprintf(stderr, "%s", value ? "true" : "false");
    }
    else if (object->IsNumber()) {
        int64_t value = object->IntegerValue();
        fprintf(stderr, "%lld", value);
    }
    else if (object->IsString()) {
        String::Utf8Value value(object);
        fprintf(stderr, "<%s>", *value);
    }
    else if (object->IsArray()) {
        Handle<Array> array = Handle<Array>::Cast(object);
        int size = array->Length();
        // fprintf(stderr, "%2d: ARRAY with %d elements\n", level, size);
        fprintf(stderr, "[");
        for (int j = 0; j < size; ++j) {
            Handle<Object> elem = Local<Object>::Cast(array->Get(j));
            // fprintf(stderr, "%2d:  elem %2d: ", level, j);
            if (j > 0) fprintf(stderr, ", ");
            DumpObject(elem, level+1);
        }
        fprintf(stderr, "]");
    }
    else if (object->IsObject()) {
        Local<Array> property_names = object->GetOwnPropertyNames();
        // fprintf(stderr, "%2d: OBJECT with %d properties\n", level, property_names->Length());
        fprintf(stderr, "{");
        for (int j = 0; j < property_names->Length(); ++j) {
            Handle<Object> key = Local<Object>::Cast(property_names->Get(j));
            Handle<Object> value = Local<Object>::Cast(object->Get(key));
            // fprintf(stderr, "%2d:  slot %d: ", level, j);
            if (j > 0) fprintf(stderr, ", ");
            DumpObject(key, level+1);
            fprintf(stderr, ": ");
            DumpObject(value, level+1);
        }
        fprintf(stderr, "}");
    }

    if (level == 0) {
        fprintf(stderr, "\n");
    }
}

void V8Context::initialize_v8()
{
    // fprintf(stderr, "V8 initializing\n");
    const char* prog = "foo";
    V8::InitializeICUDefaultLocation(prog);
    V8::InitializeExternalStartupData(prog);
    platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();
    // fprintf(stderr, "V8 initializing done\n");
}

void V8Context::terminate_v8()
{
    // fprintf(stderr, "V8 terminating\n");
    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    // fprintf(stderr, "V8 terminating done\n");
}

uint64_t V8Context::GetTypeFlags(const Local<Value>& v)
{
    uint64_t result = 0;
    if (v->IsArgumentsObject()  ) result |= 0x0000000000000001;
    if (v->IsArrayBuffer()      ) result |= 0x0000000000000002;
    if (v->IsArrayBufferView()  ) result |= 0x0000000000000004;
    if (v->IsArray()            ) result |= 0x0000000000000008;
    if (v->IsBooleanObject()    ) result |= 0x0000000000000010;
    if (v->IsBoolean()          ) result |= 0x0000000000000020;
    if (v->IsDataView()         ) result |= 0x0000000000000040;
    if (v->IsDate()             ) result |= 0x0000000000000080;
    if (v->IsExternal()         ) result |= 0x0000000000000100;
    if (v->IsFalse()            ) result |= 0x0000000000000200;
    if (v->IsFloat32Array()     ) result |= 0x0000000000000400;
    if (v->IsFloat64Array()     ) result |= 0x0000000000000800;
    if (v->IsFunction()         ) result |= 0x0000000000001000;
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
    if (v->IsNull()             ) result |= 0x0000000000800000;
    if (v->IsNumberObject()     ) result |= 0x0000000001000000;
    if (v->IsNumber()           ) result |= 0x0000000002000000;
    if (v->IsObject()           ) result |= 0x0000000004000000;
    if (v->IsPromise()          ) result |= 0x0000000008000000;
    if (v->IsRegExp()           ) result |= 0x0000000010000000;
    if (v->IsSetIterator()      ) result |= 0x0000000020000000;
    if (v->IsSet()              ) result |= 0x0000000040000000;
    if (v->IsStringObject()     ) result |= 0x0000000080000000;
    if (v->IsString()           ) result |= 0x0000000100000000;
    if (v->IsSymbolObject()     ) result |= 0x0000000200000000;
    if (v->IsSymbol()           ) result |= 0x0000000400000000;
    if (v->IsTrue()             ) result |= 0x0000000800000000;
    if (v->IsTypedArray()       ) result |= 0x0000001000000000;
    if (v->IsUint16Array()      ) result |= 0x0000002000000000;
    if (v->IsUint32Array()      ) result |= 0x0000004000000000;
    if (v->IsUint32()           ) result |= 0x0000008000000000;
    if (v->IsUint8Array()       ) result |= 0x0000010000000000;
    if (v->IsUint8ClampedArray()) result |= 0x0000020000000000;
    if (v->IsUndefined()        ) result |= 0x0000040000000000;
    if (v->IsWeakMap()          ) result |= 0x0000080000000000;
    if (v->IsWeakSet()          ) result |= 0x0000100000000000;
    return result;
}
