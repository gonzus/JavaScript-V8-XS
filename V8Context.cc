#include "libplatform/libplatform.h"
#include "V8Context.h"

#define ELEMS 3

using namespace v8;

static uint64_t GetTypeFlags(const v8::Local<v8::Value>& v);

V8Context::V8Context(const char* flags)
{
    // fprintf(stderr, "V8 construct\n");
    V8Context::initialize_v8();

    // Create a new Isolate and make it the current one.
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    // fprintf(stderr, "V8 created allocator\n");
    isolate = v8::Isolate::New(create_params);
    // fprintf(stderr, "V8 created isolate\n");
    // fprintf(stderr, "V8 construct done\n");
}

V8Context::~V8Context()
{
    // fprintf(stderr, "V8 destruct\n");
    delete create_params.array_buffer_allocator;
    V8Context::terminate_v8();
    // fprintf(stderr, "V8 destruct done\n");
}

int V8Context::eval(const char* code, const char* file)
{
    v8::Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, code, v8::NewStringType::kNormal)
        .ToLocalChecked();

    // Compile the source code.
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();

    // Run the script to get the result.
    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

    // Convert the result to an UTF8 string and print it.
    v8::String::Utf8Value utf8(isolate, result);
    fprintf(stderr, "GONZO: [%s]\n", *utf8);

    v8::Handle<v8::Array> a = CreateArray(2);
    fprintf(stderr, "GONZO: GOT ARRAY with %d elements\n", a->Length());
    DumpObject(a);

    v8::Handle<v8::Object> o = CreateObject(2);
    fprintf(stderr, "GONZO: GOT OBJECT\n");
    DumpObject(o);

    return 0;
}

v8::Handle<v8::Array> V8Context::CreateArray(int nested)
{
    // We will be creating temporary handles so we use a handle scope.
    v8::EscapableHandleScope handle_scope(isolate);

    // Create a new empty array.
    v8::Handle<v8::Array> array = v8::Array::New(isolate);

    // Return an empty result if there was an error creating the array.
    if (array.IsEmpty()) {
        fprintf(stderr, "%2d: Returning empty array\n", nested);
        return v8::Handle<v8::Array>();
    }

    // Fill out some values
    int j = 0;
    for (; j < ELEMS; ++j) {
        int v = j*(10+nested);
        array->Set(j, v8::Integer::New(isolate, v));
        fprintf(stderr, "%2d: Added element %2d => %3d\n", nested, j, v);
    }
    array->Set(j++, v8::Boolean::New(isolate, (j%2) == 0));
    array->Set(j++, v8::Boolean::New(isolate, (j%2) == 0));
    switch (nested) {
        case 2: {
            v8::Handle<v8::Array> a = CreateArray(1);
            array->Set(j, a);
            fprintf(stderr, "%2d: Added nested array %2d\n", nested, j);
            break;
        }
        case 1: {
            v8::Handle<v8::Object> o = CreateObject(0);
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

v8::Handle<v8::Object> V8Context::CreateObject(int nested)
{
    // We will be creating temporary handles so we use a handle scope.
    v8::EscapableHandleScope handle_scope(isolate);

    // Create a new empty object.
    v8::Handle<v8::Object> object = v8::Object::New(isolate);

    // Return an empty result if there was an error creating the array.
    if (object.IsEmpty()) {
        fprintf(stderr, "%2d: Returning empty object\n", nested);
        return v8::Handle<v8::Object>();
    }

    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    int j = 0;
    for (j = 0; j < ELEMS; ++j) {
        char k[256];
        int v = j*(10+nested);
        sprintf(k, "key_%d", j);
        v8::Local<v8::Value> key = v8::String::NewFromUtf8(isolate, k, v8::NewStringType::kNormal)
            .ToLocalChecked();
        v8::Local<v8::Value> val = v8::Integer::New(isolate, v);
        object->Set(context, key, val);
        fprintf(stderr, "%2d: Set key [%s] to value %d\n", nested, k, v);
    }
    switch (nested) {
        case 2: {
            v8::Handle<v8::Object> o = CreateObject(1);
            v8::Local<v8::Value> key = v8::String::NewFromUtf8(isolate, "nested", v8::NewStringType::kNormal)
                .ToLocalChecked();
            object->Set(context, key, o);
            fprintf(stderr, "%2d: Added nested object\n", nested);
            break;
        }
        case 1: {
            v8::Handle<v8::Array> a = CreateArray(0);
            v8::Local<v8::Value> key = v8::String::NewFromUtf8(isolate, "array", v8::NewStringType::kNormal)
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

void V8Context::DumpObject(const v8::Handle<v8::Object>& object, int level)
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
        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(object);
        int size = array->Length();
        // fprintf(stderr, "%2d: ARRAY with %d elements\n", level, size);
        fprintf(stderr, "[");
        for (int j = 0; j < size; ++j) {
            Handle<v8::Object> elem = Local<Object>::Cast(array->Get(j));
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
            Handle<v8::Object> key = Local<Object>::Cast(property_names->Get(j));
            Handle<v8::Object> value = Local<Object>::Cast(object->Get(key));
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
    v8::V8::InitializeICUDefaultLocation(prog);
    v8::V8::InitializeExternalStartupData(prog);
    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    // fprintf(stderr, "V8 initializing done\n");
}

void V8Context::terminate_v8()
{
    // fprintf(stderr, "V8 terminating\n");
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    // fprintf(stderr, "V8 terminating done\n");
}

static uint64_t GetTypeFlags(const v8::Local<v8::Value>& v)
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
