#include "pl_inlined.h"

typedef int (*BeforeCB)(V8Context* ctx);

static int create_EventLoop_object(V8Context* ctx);

static struct {
    const char* file_name;
    BeforeCB    before_cb;
    const char* source;
} js_inlined[] = {
    {
        "c_eventloop.js", /* on my mac, this eval takes 574.697134 us avg */
        create_EventLoop_object,

        "/*\n"
        " *  C eventloop example (c_eventloop.c).\n"
        " *\n"
        " *  Ecmascript code to initialize the exposed API (setTimeout() etc) when\n"
        " *  using the C eventloop.\n"
        " *\n"
        " *  https://developer.mozilla.org/en-US/docs/Web/JavaScript/Timers\n"
        " */\n"
        "\n"
        "/*\n"
        " *  Timer API\n"
        " */\n"
        "\n"
        "function setTimeout(func, delay) {\n"
        "    var cb_func;\n"
        "    var bind_args;\n"
        "    var timer_id;\n"
        "\n"
        "    // Delay can be optional at least in some contexts, so tolerate that.\n"
        "    // https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/setTimeout\n"
        "    if (typeof delay !== 'number') {\n"
        "        if (typeof delay === 'undefined') {\n"
        "            delay = 0;\n"
        "        } else {\n"
        "            throw new TypeError('invalid delay');\n"
        "        }\n"
        "    }\n"
        "\n"
        "    if (typeof func === 'string') {\n"
        "        // Legacy case: callback is a string.\n"
        "        cb_func = eval.bind(this, func);\n"
        "    } else if (typeof func !== 'function') {\n"
        "        throw new TypeError('callback is not a function/string');\n"
        "    } else if (arguments.length > 2) {\n"
        "        // Special case: callback arguments are provided.\n"
        "        bind_args = Array.prototype.slice.call(arguments, 2);  // [ arg1, arg2, ... ]\n"
        "        bind_args.unshift(this);  // [ global(this), arg1, arg2, ... ]\n"
        "        cb_func = func.bind.apply(func, bind_args);\n"
        "    } else {\n"
        "        // Normal case: callback given as a function without arguments.\n"
        "        cb_func = func;\n"
        "    }\n"
        "\n"
        "    timer_id = EventLoop_createTimer(cb_func, delay, true /*oneshot*/);\n"
        "\n"
        "    return timer_id;\n"
        "}\n"
        "\n"
        "function clearTimeout(timer_id) {\n"
        "    if (typeof timer_id !== 'number') {\n"
        "        throw new TypeError('timer ID is not a number');\n"
        "    }\n"
        "    var success = EventLoop_deleteTimer(timer_id);  /* retval ignored */\n"
        "}\n"
        "\n"
        "function setInterval(func, delay) {\n"
        "    var cb_func;\n"
        "    var bind_args;\n"
        "    var timer_id;\n"
        "\n"
        "    if (typeof delay !== 'number') {\n"
        "        if (typeof delay === 'undefined') {\n"
        "            delay = 0;\n"
        "        } else {\n"
        "            throw new TypeError('invalid delay');\n"
        "        }\n"
        "    }\n"
        "\n"
        "    if (typeof func === 'string') {\n"
        "        // Legacy case: callback is a string.\n"
        "        cb_func = eval.bind(this, func);\n"
        "    } else if (typeof func !== 'function') {\n"
        "        throw new TypeError('callback is not a function/string');\n"
        "    } else if (arguments.length > 2) {\n"
        "        // Special case: callback arguments are provided.\n"
        "        bind_args = Array.prototype.slice.call(arguments, 2);  // [ arg1, arg2, ... ]\n"
        "        bind_args.unshift(this);  // [ global(this), arg1, arg2, ... ]\n"
        "        cb_func = func.bind.apply(func, bind_args);\n"
        "    } else {\n"
        "        // Normal case: callback given as a function without arguments.\n"
        "        cb_func = func;\n"
        "    }\n"
        "\n"
        "    timer_id = EventLoop_createTimer(cb_func, delay, false /*oneshot*/);\n"
        "\n"
        "    return timer_id;\n"
        "}\n"
        "\n"
        "function clearInterval(timer_id) {\n"
        "    if (typeof timer_id !== 'number') {\n"
        "        throw new TypeError('timer ID is not a number');\n"
        "    }\n"
        "    EventLoop_deleteTimer(timer_id);\n"
        "}\n"
        "\n"
    },
};

void pl_register_inlined_functions(V8Context* ctx)
{
    size_t j = 0;
    dTHX;
    for (j = 0; j < sizeof(js_inlined) / sizeof(js_inlined[0]); ++j) {
        if (js_inlined[j].before_cb) {
            js_inlined[j].before_cb(ctx);
        }
        pl_eval(aTHX_ ctx, js_inlined[j].source, js_inlined[j].file_name);
    }
}

static int create_EventLoop_object(V8Context* ctx)
{
#if 1
    return 0;
#else
    /*
     * The idea was to create an EventLoop object that could later hold the
     * slots for createTimer and deleteTimer, so that it could be used from the
     * JS code in pl_inline.cc, but I could not make it work.
     */
    HV* empty = newHV();
    SV* ref = newRV_noinc((SV*) empty);
    return pl_set_global_or_property(aTHX_ ctx, "EventLoop", ref);
#endif
}
