/* minilang.c: the embedding API (minilang.h) over the stage 5 VM.
 *
 * One translation unit: this file includes ../minilang-c/mlvm.c with its
 * command-line driver left out, so every VM internal stays static and the
 * object file exports only the ml_* functions. The VM side of embedding is a
 * handful of hooks in mlvm.c (marked "embedding" there): host function
 * objects, a mark_host() root hook, a soft heap limit, and run(), which
 * starts above the frames of a suspended minilang call.
 *
 * Handles. A handle is (slot id, generation). Scoped handles live on a stack
 * of slots; a scope is a mark on that stack. Pinned handles live in a second
 * table with a free list; bit 31 of the id selects it. A slot's generation is
 * odd while the slot is in use and even when free, and it changes on every
 * release, so an old handle never matches a reused slot. mark_handles() makes
 * every slot a root, which is the whole rooting rule: the host can only reach
 * values through slots.
 *
 * Errors. The pending error is the VM's own error state (vm->err, errmsg,
 * errval, errline, host_type). API calls that fail set it; a host function
 * that returns an error status hands it to the interpreter unchanged, which
 * then unwinds exactly as if a builtin had failed. Nothing here uses
 * setjmp/longjmp, so no host frame is ever skipped.
 */
#define MLVM_LIBRARY
#include "../minilang-c/mlvm.c"

#include "minilang.h"

#define MAX_HOST_DEPTH 200 /* host functions active at once (each nesting uses C stack) */
#define PIN_BIT 0x80000000u

typedef struct {
    Value v;      /* nil when free */
    uint32_t gen; /* odd: in use */
} Slot;

struct ml_vm {
    VM *core;
    ml_options opt;
    Slot *locals; /* scoped handles: a stack */
    uint32_t nlocals, cap_locals;
    Slot *pins; /* pinned handles */
    uint32_t npins, cap_pins;
    uint32_t *free_pins;
    uint32_t nfree_pins, cap_free_pins;
    uint32_t *scopes; /* each open scope's mark in locals */
    int nscopes, cap_scopes;
    int scope_floor; /* scopes below this belong to a running host call */
    int host_depth;  /* host functions running now */
    int in_output;  /* inside the print callback */
    ml_output out;
    void *out_ud;
};

/* ------------------------------------------------------------- misuse */

static ml_status misuse(ml_vm *api, const char *fmt, ...) {
    VM *vm = api->core;
    va_list ap;
    buf_clear(&vm->errmsg);
    va_start(ap, fmt);
    buf_vprintf(&vm->errmsg, fmt, ap);
    va_end(ap);
    vm->err = E_HOST;
    vm->host_type = "SystemError";
    vm->has_errval = 0;
    if (!api->opt.misuse_returns) {
        fprintf(stderr, "minilang: API misuse: %s\n", vm->errmsg.p);
        fflush(stderr);
        abort();
    }
    return ML_MISUSE;
}

static int entry_ok(ml_vm *api, const char *fn) {
    if (!api) {
        fprintf(stderr, "minilang: API misuse: %s called with a NULL VM\n", fn);
        abort();
    }
    if (api->in_output) { /* the VM is inside print(): leave its error state alone */
        if (!api->opt.misuse_returns) {
            fprintf(stderr, "minilang: API misuse: %s called from the print callback\n", fn);
            fflush(stderr);
            abort();
        }
        return 0;
    }
    return 1;
}
#define ENTER(api)                                                              \
    do {                                                                        \
        if (!entry_ok(api, __func__)) return ML_MISUSE;                         \
    } while (0)

/* ------------------------------------------------------------ handles */

static void mark_handles(VM *vm) {
    ml_vm *api = vm->host;
    for (uint32_t i = 0; i < api->nlocals; i++) mark_value(vm, api->locals[i].v);
    for (uint32_t i = 0; i < api->npins; i++) mark_value(vm, api->pins[i].v);
}

static void grow_slots(Slot **slots, uint32_t *cap) {
    uint32_t n = *cap ? *cap * 2 : 64;
    *slots = xrealloc(*slots, n * sizeof(Slot));
    for (uint32_t i = *cap; i < n; i++) {
        (*slots)[i].v = v_nil();
        (*slots)[i].gen = 0;
    }
    *cap = n;
}

/* A new handle in the innermost scope. Uses malloc only, never the GC heap,
 * so it cannot collect: v needs no other root while this runs. */
static ml_value new_local(ml_vm *api, Value v) {
    if (api->nlocals == api->cap_locals) grow_slots(&api->locals, &api->cap_locals);
    uint32_t i = api->nlocals++;
    Slot *s = &api->locals[i];
    s->gen++; /* even (free) -> odd (in use) */
    s->v = v;
    ml_value h = {i, s->gen};
    return h;
}

static Slot *slot_of(ml_vm *api, ml_value h) {
    if (!(h.gen & 1)) return NULL;
    Slot *s;
    if (h.id & PIN_BIT) {
        uint32_t i = h.id & ~PIN_BIT;
        if (i >= api->npins) return NULL;
        s = &api->pins[i];
    } else {
        if (h.id >= api->nlocals) return NULL;
        s = &api->locals[h.id];
    }
    return s->gen == h.gen ? s : NULL;
}

/* The value behind a handle, or a misuse report. */
static int get(ml_vm *api, ml_value h, Value *out, const char *fn) {
    Slot *s = slot_of(api, h);
    if (!s) {
        misuse(api, "%s: stale or invalid handle (id %u, generation %u)", fn, (unsigned)h.id, (unsigned)h.gen);
        return 0;
    }
    *out = s->v;
    return 1;
}
#define GET(api, h, out)                                                        \
    do {                                                                        \
        if (!get(api, h, out, __func__)) return ML_MISUSE;                      \
    } while (0)

static void free_slot(Slot *s) {
    s->v = v_nil();
    s->gen++; /* odd -> even */
}

static uint32_t current_mark(ml_vm *api) { return api->nscopes ? api->scopes[api->nscopes - 1] : 0; }

static void pop_dead_locals(ml_vm *api) {
    uint32_t floor = current_mark(api);
    while (api->nlocals > floor && !(api->locals[api->nlocals - 1].gen & 1)) api->nlocals--;
}

static int open_scope(ml_vm *api) {
    if (api->nscopes == api->cap_scopes) {
        api->cap_scopes = api->cap_scopes ? api->cap_scopes * 2 : 16;
        api->scopes = xrealloc(api->scopes, (size_t)api->cap_scopes * sizeof(uint32_t));
    }
    api->scopes[api->nscopes] = api->nlocals;
    return api->nscopes++;
}

static void close_scope(ml_vm *api) { /* the innermost one */
    uint32_t mark = api->scopes[--api->nscopes];
    for (uint32_t i = mark; i < api->nlocals; i++)
        if (api->locals[i].gen & 1) free_slot(&api->locals[i]);
    api->nlocals = mark;
}

int ml_scope_open(ml_vm *api) {
    if (!entry_ok(api, __func__)) return -1;
    return open_scope(api);
}

ml_status ml_scope_close(ml_vm *api, int scope, ml_value *keep) {
    ENTER(api);
    if (scope < api->scope_floor || scope != api->nscopes - 1)
        return misuse(api, "ml_scope_close(%d): the innermost scope the caller may close is %d", scope,
                      api->nscopes - 1 >= api->scope_floor ? api->nscopes - 1 : -1);
    Value v = v_nil();
    if (keep) GET(api, *keep, &v);
    close_scope(api);
    if (keep) *keep = new_local(api, v); /* no allocation between: v needs no root */
    return ML_OK;
}

ml_status ml_pin(ml_vm *api, ml_value h, ml_value *pinned) {
    ENTER(api);
    Value v;
    GET(api, h, &v);
    if (!pinned) return misuse(api, "ml_pin: no output handle");
    uint32_t i;
    if (api->nfree_pins) {
        i = api->free_pins[--api->nfree_pins];
    } else {
        if (api->npins == api->cap_pins) grow_slots(&api->pins, &api->cap_pins);
        i = api->npins++;
    }
    Slot *s = &api->pins[i];
    s->gen++;
    s->v = v;
    pinned->id = i | PIN_BIT;
    pinned->gen = s->gen;
    return ML_OK;
}

ml_status ml_release(ml_vm *api, ml_value h) {
    ENTER(api);
    Slot *s = slot_of(api, h);
    if (!s) return misuse(api, "ml_release: stale or invalid handle (id %u, generation %u)", (unsigned)h.id, (unsigned)h.gen);
    free_slot(s);
    if (h.id & PIN_BIT) {
        if (api->nfree_pins == api->cap_free_pins) {
            api->cap_free_pins = api->cap_free_pins ? api->cap_free_pins * 2 : 16;
            api->free_pins = xrealloc(api->free_pins, api->cap_free_pins * sizeof(uint32_t));
        }
        api->free_pins[api->nfree_pins++] = h.id & ~PIN_BIT;
    } else {
        pop_dead_locals(api);
    }
    return ML_OK;
}

/* -------------------------------------------------------------- errors */

static void clear_error(VM *vm) {
    vm->err = E_NONE;
    vm->has_errval = 0;
    vm->errval = v_nil();
    buf_clear(&vm->errmsg);
}

static ml_status host_errorf(VM *vm, ml_status st, const char *type, const char *fmt, ...) {
    va_list ap;
    buf_clear(&vm->errmsg);
    va_start(ap, fmt);
    buf_vprintf(&vm->errmsg, fmt, ap);
    va_end(ap);
    vm->err = E_HOST;
    vm->host_type = type;
    vm->has_errval = 0;
    return st;
}

/* A catchable exception whose value is the message, like a runtime error. */
static ml_status exceptionf(VM *vm, ml_status st, const char *fmt, ...) {
    va_list ap;
    buf_clear(&vm->errmsg);
    va_start(ap, fmt);
    buf_vprintf(&vm->errmsg, fmt, ap);
    va_end(ap);
    vm->err = E_RUNTIME;
    vm->errline = -1; /* the interpreter fills in the line of the call if it propagates */
    vm->has_errval = 0;
    return st;
}

/* After an allocation: did it pass the heap limit? */
static int over_limit(VM *vm) {
    if (!vm->over_limit) return 0;
    vm->over_limit = 0;
    host_errorf(vm, ML_NOMEM, "MemoryError", "heap limit exceeded");
    return 1;
}

/* The status for how a run ended. Signals that reach the host become
 * uncatchable errors of the type the tree-walker would raise, so they stop
 * at a C caller instead of crossing it. */
static ml_status finish(VM *vm, ErrKind k) {
    switch (k) {
    case E_NONE: return ML_OK;
    case E_RUNTIME: return ML_EXCEPTION;
    case E_RECURSION: return ML_ERROR;
    case E_BREAK: return host_errorf(vm, ML_ERROR, "BreakSignal", "break outside a loop");
    case E_RETURN: {
        Value v = vm->errval; /* the returned value, kept as the error's value */
        host_errorf(vm, ML_ERROR, "ReturnSignal", "top-level return");
        vm->errval = v;
        vm->has_errval = 1;
        return ML_ERROR;
    }
    case E_HOST: return strcmp(vm->host_type, "MemoryError") == 0 ? ML_NOMEM : ML_ERROR;
    }
    return ML_ERROR;
}

static const char *kind_name(Value v) { /* for messages: minilang's type(), with ints and floats apart */
    if (v.k == V_FLOAT) return "float";
    if (is_intval(v)) return "integer";
    return type_name(v);
}

ml_status ml_throw(ml_vm *api, ml_value h) {
    ENTER(api);
    VM *vm = api->core;
    Value v;
    GET(api, h, &v);
    buf_clear(&vm->errmsg);
    /* As OP_THROW does: the message is made now, so a value that cannot be
     * printed fails here (an uncatchable error) even if a catch would take it. */
    if (stringify(vm, v, &vm->errmsg, 0)) return finish(vm, vm->err);
    vm->err = E_RUNTIME;
    vm->errline = -1;
    vm->has_errval = v.k != V_NIL; /* throw nil is caught as the string "nil" */
    vm->errval = v;
    return ML_EXCEPTION;
}

ml_status ml_throwf(ml_vm *api, const char *fmt, ...) {
    ENTER(api);
    VM *vm = api->core;
    va_list ap;
    buf_clear(&vm->errmsg);
    va_start(ap, fmt);
    buf_vprintf(&vm->errmsg, fmt, ap);
    va_end(ap);
    vm->err = E_RUNTIME;
    vm->errline = -1;
    vm->has_errval = 0;
    return ML_EXCEPTION;
}

const char *ml_error_message(ml_vm *api) {
    if (!api) return "";
    VM *vm = api->core;
    return vm->err == E_NONE || !vm->errmsg.p ? "" : vm->errmsg.p;
}

const char *ml_error_type(ml_vm *api) {
    if (!api) return "";
    VM *vm = api->core;
    switch (vm->err) {
    case E_NONE: return "";
    case E_RUNTIME: return "RuntimeErr";
    case E_BREAK: return "BreakSignal";
    case E_RETURN: return "ReturnSignal";
    case E_RECURSION: case E_HOST: return vm->host_type;
    }
    return "";
}

int ml_error_line(ml_vm *api) {
    if (!api) return 0;
    VM *vm = api->core;
    return vm->err == E_RUNTIME && vm->errline > 0 ? vm->errline : 0;
}

ml_status ml_error_value(ml_vm *api, ml_value *out) {
    ENTER(api);
    VM *vm = api->core;
    if (!out) return misuse(api, "ml_error_value: no output handle");
    if (vm->err == E_NONE) return ML_NOT_FOUND; /* nothing pending; the state is unchanged */
    if (vm->has_errval) {
        *out = new_local(api, vm->errval);
        return ML_OK;
    }
    /* No value: the message as a string, as a catch would see a runtime error. */
    *out = new_local(api, v_obj(new_string(vm, vm->errmsg.p ? vm->errmsg.p : "", vm->errmsg.n)));
    return ML_OK;
}

void ml_clear_error(ml_vm *api) {
    if (api) clear_error(api->core);
}

const char *ml_status_name(ml_status st) {
    static const char *const names[] = {"ML_OK", "ML_EXCEPTION", "ML_ERROR", "ML_NOMEM",
                                        "ML_TYPE_ERROR", "ML_NOT_FOUND", "ML_LOAD_ERROR", "ML_MISUSE"};
    return (unsigned)st < sizeof names / sizeof names[0] ? names[st] : "ML_(unknown)";
}

/* ---------------------------------------------------- host functions */

/* The interpreter calls this for an ObjHost (vm->call_host). The function and
 * its arguments are the top nargs + 1 values of the VM stack, so they stay
 * rooted; the host sees them through handles in a fresh scope. Returns 0
 * with *out set, or -1 with the VM's error set. */
static int host_call(VM *vm, ObjHost *h, int nargs, Value *out) {
    ml_vm *api = vm->host;
    if (h->arity >= 0 && nargs != h->arity) {
        rt_error(vm, "%s() expects %d argument(s), got %d", h->name->s, h->arity, nargs);
        return -1;
    }
    if (api->host_depth >= MAX_HOST_DEPTH) {
        rt_error(vm, "stack overflow (max host call depth %d)", MAX_HOST_DEPTH);
        return -1;
    }
    int scope = open_scope(api), saved_floor = api->scope_floor;
    api->scope_floor = scope + 1; /* the host function cannot close this scope */
    ml_value small[8];
    ml_value *args = nargs <= 8 ? small : xrealloc(NULL, (size_t)nargs * sizeof(ml_value));
    size_t first = vm->sp - (size_t)nargs;
    for (int i = 0; i < nargs; i++) args[i] = new_local(api, vm->stack[first + i]);
    ml_value result = ML_NO_VALUE;
    clear_error(vm);
    api->host_depth++;
    ml_status st = ((ml_cfunction)h->fn)(api, nargs, args, &result, h->ud);
    api->host_depth--;
    if (args != small) free(args);

    int failed = 0;
    if (api->nscopes != scope + 1) {
        int open = api->nscopes - scope - 1;
        while (api->nscopes > scope + 1) close_scope(api);
        misuse(api, "host function '%s' returned with %d handle scope(s) still open", h->name->s, open);
        failed = 1;
    } else if (st == ML_OK) {
        Value v = v_nil();
        if ((result.id || result.gen) && !get(api, result, &v, h->name->s)) failed = 1;
        else {
            clear_error(vm); /* the host handled any error it saw */
            *out = v;        /* no allocation until the interpreter stores it */
        }
    } else {
        if (vm->err == E_NONE)
            host_errorf(vm, ML_ERROR, "SystemError", "host function '%s' returned %s without setting an error",
                        h->name->s, ml_status_name(st));
        else if (vm->err == E_BREAK || vm->err == E_RETURN)
            finish(vm, vm->err);
        failed = 1;
    }
    close_scope(api);
    api->scope_floor = saved_floor;
    return failed ? -1 : 0;
}

ml_status ml_new_function(ml_vm *api, const char *name, ml_cfunction fn, int arity, void *ud, ml_value *out) {
    ENTER(api);
    VM *vm = api->core;
    if (!name || !fn || !out || arity < -1) return misuse(api, "ml_new_function: bad arguments");
    size_t n = strlen(name);
    if (!utf8_valid((const uint8_t *)name, n)) return exceptionf(vm, ML_TYPE_ERROR, "name is not valid UTF-8");
    ObjString *s = intern(vm, name, n);
    push_temp(vm, v_obj(s)); /* the next allocation may collect */
    ObjHost *h = (ObjHost *)new_obj(vm, O_HOST, sizeof(ObjHost));
    pop_temp(vm);
    h->name = s;
    h->arity = arity;
    h->fn = (void (*)(void))fn;
    h->ud = ud;
    *out = new_local(api, v_obj(h));
    if (over_limit(vm)) {
        ml_release(api, *out);
        return ML_NOMEM;
    }
    return ML_OK;
}

/* ------------------------------------------------------------- globals */

static ml_status set_global(ml_vm *api, const char *name, Value v) { /* v must be rooted */
    VM *vm = api->core;
    size_t n = strlen(name);
    if (!utf8_valid((const uint8_t *)name, n)) return exceptionf(vm, ML_TYPE_ERROR, "name is not valid UTF-8");
    ObjString *key = intern(vm, name, n);
    table_set(&vm->globals, key, v); /* malloc'ed table: no collection */
    return over_limit(vm) ? ML_NOMEM : ML_OK;
}

ml_status ml_set_global(ml_vm *api, const char *name, ml_value h) {
    ENTER(api);
    Value v;
    GET(api, h, &v);
    if (!name) return misuse(api, "ml_set_global: NULL name");
    return set_global(api, name, v);
}

ml_status ml_get_global(ml_vm *api, const char *name, ml_value *out) {
    ENTER(api);
    VM *vm = api->core;
    if (!name || !out) return misuse(api, "ml_get_global: bad arguments");
    size_t n = strlen(name);
    /* Global names are interned, so a name that is not interned is not a
     * global; looking it up this way allocates nothing. */
    ObjString *key = table_find_text(&vm->strings, name, n, hash_bytes(name, n));
    Value v;
    if (!key || !table_get(&vm->globals, key, &v))
        return exceptionf(vm, ML_NOT_FOUND, "undefined variable '%s'", name);
    *out = new_local(api, v);
    return ML_OK;
}

ml_status ml_register(ml_vm *api, const char *name, ml_cfunction fn, int arity, void *ud) {
    ENTER(api);
    ml_value h;
    ml_status st = ml_new_function(api, name, fn, arity, ud, &h);
    if (st != ML_OK) return st;
    st = set_global(api, name, api->locals[h.id].v);
    ml_release(api, h);
    return st;
}

/* --------------------------------------------------- programs and calls */

ml_status ml_load(ml_vm *api, const void *bytes, size_t len, ml_value *program) {
    ENTER(api);
    VM *vm = api->core;
    if ((!bytes && len) || !program) return misuse(api, "ml_load: bad arguments");
    clear_error(vm);
    char err[256];
    int temps = vm->ntemps;
    ObjCode *script = load_program(vm, bytes ? bytes : (const void *)"", len, err, sizeof err);
    if (!script) return host_errorf(vm, ML_LOAD_ERROR, "LoadError", "%s", err);
    *program = new_local(api, v_obj(script));
    vm->ntemps = temps; /* the loader left the script on the temp roots */
    if (over_limit(vm)) {
        ml_release(api, *program);
        return ML_NOMEM;
    }
    return ML_OK;
}

ml_status ml_load_file(ml_vm *api, const char *path, ml_value *program) {
    ENTER(api);
    uint8_t *data;
    size_t n;
    if (!path) return misuse(api, "ml_load_file: NULL path");
    if (read_file(path, &data, &n)) return host_errorf(api->core, ML_LOAD_ERROR, "LoadError", "cannot read %s", path);
    ml_status st = ml_load(api, data, n, program);
    free(data);
    return st;
}

ml_status ml_run(ml_vm *api, ml_value program, ml_value *result) {
    ENTER(api);
    VM *vm = api->core;
    Value p;
    GET(api, program, &p);
    clear_error(vm);
    vm->over_limit = 0;
    if (!is_kind(p, O_CODE) || AS_CODE(p)->nparams || AS_CODE(p)->nup)
        return exceptionf(vm, ML_TYPE_ERROR, "ml_run expects a program, got %s", type_name(p));
    if (vm->nframes > MAX_DEPTH) return exceptionf(vm, ML_EXCEPTION, "stack overflow (max call depth %d)", MAX_DEPTH);
    size_t entry_sp = vm->sp;
    int entry_frames = vm->nframes;
    Value r = v_nil();
    ErrKind k = execute(vm, AS_CODE(p), &r);
    vm->sp = entry_sp; /* drop this run's frames, also after an uncaught error */
    vm->nframes = entry_frames;
    if (k == E_NONE && result) *result = new_local(api, r);
    return finish(vm, k);
}

/* Calls the value below the top nargs values of the stack. */
static ErrKind call_value(VM *vm, int nargs, Value *result) {
    Value fn = vm->stack[vm->sp - (size_t)nargs - 1];
    if (IS_CLOSURE(fn)) { /* what OP_CALL does, then run until this frame returns */
        ObjClosure *c = AS_CLOSURE(fn);
        ObjCode *callee = c->code;
        if ((uint32_t)nargs != callee->nparams) {
            rt_error(vm, "<fn %s> expects %u argument(s), got %d", callee->name->s, (unsigned)callee->nparams, nargs);
            return E_RUNTIME;
        }
        if (vm->nframes > MAX_DEPTH) {
            rt_error(vm, "stack overflow (max call depth %d)", MAX_DEPTH);
            return E_RUNTIME;
        }
        size_t base = vm->sp - (size_t)nargs;
        ensure_stack(vm, base + callee->nslots + callee->max_stack + STACK_SLACK);
        for (uint32_t i = (uint32_t)nargs; i < callee->nslots; i++) vm->stack[base + i] = v_nil();
        Frame *f = &vm->frames[vm->nframes++];
        f->closure = c;
        f->code = callee;
        f->base = base;
        f->pc = 0;
        vm->sp = base + callee->nslots;
        return run(vm, vm->nframes - 1, result);
    }
    if (fn.k == V_BUILTIN) {
        int which = (int)fn.as.i;
        if (BUILTIN_ARITY[which] >= 0 && nargs != BUILTIN_ARITY[which]) {
            rt_error(vm, "%s() expects %d argument(s), got %d", BUILTIN_NAMES[which], BUILTIN_ARITY[which], nargs);
            return E_RUNTIME;
        }
        return BUILTIN_FNS[which](vm, vm->stack + vm->sp - nargs, nargs, result) ? vm->err : E_NONE;
    }
    return host_call(vm, (ObjHost *)fn.as.o, nargs, result) ? vm->err : E_NONE;
}

ml_status ml_call(ml_vm *api, ml_value fn, int nargs, const ml_value *args, ml_value *result) {
    ENTER(api);
    VM *vm = api->core;
    Value f;
    GET(api, fn, &f);
    if (nargs < 0 || (nargs > 0 && !args)) return misuse(api, "ml_call: bad argument count or array");
    for (int i = 0; i < nargs; i++)
        if (!slot_of(api, args[i])) return misuse(api, "ml_call: argument %d is a stale or invalid handle", i);
    clear_error(vm);
    vm->over_limit = 0;
    if (!IS_CLOSURE(f) && f.k != V_BUILTIN && !is_kind(f, O_HOST))
        return exceptionf(vm, ML_TYPE_ERROR, "cannot call a %s", type_name(f));
    size_t entry_sp = vm->sp;
    int entry_frames = vm->nframes;
    ensure_stack(vm, entry_sp + (size_t)nargs + 1 + STACK_SLACK);
    vm->stack[vm->sp++] = f;
    for (int i = 0; i < nargs; i++) vm->stack[vm->sp++] = slot_of(api, args[i])->v;
    Value r = v_nil();
    ErrKind k = call_value(vm, nargs, &r);
    vm->sp = entry_sp;
    vm->nframes = entry_frames;
    if (k == E_NONE && result) *result = new_local(api, r); /* malloc only: r needs no root */
    return finish(vm, k);
}

/* ------------------------------------------------------------- values */

ml_status ml_new_nil(ml_vm *api, ml_value *out) {
    ENTER(api);
    if (!out) return misuse(api, "ml_new_nil: no output handle");
    *out = new_local(api, v_nil());
    return ML_OK;
}
ml_status ml_new_bool(ml_vm *api, int b, ml_value *out) {
    ENTER(api);
    if (!out) return misuse(api, "ml_new_bool: no output handle");
    *out = new_local(api, v_bool(b));
    return ML_OK;
}
ml_status ml_new_int(ml_vm *api, int64_t i, ml_value *out) {
    ENTER(api);
    if (!out) return misuse(api, "ml_new_int: no output handle");
    *out = new_local(api, v_int(i));
    return ML_OK;
}
ml_status ml_new_float(ml_vm *api, double d, ml_value *out) {
    ENTER(api);
    if (!out) return misuse(api, "ml_new_float: no output handle");
    *out = new_local(api, v_float(d));
    return ML_OK;
}

static ml_status new_object_handle(ml_vm *api, Value v, ml_value *out) {
    *out = new_local(api, v);
    if (over_limit(api->core)) {
        ml_release(api, *out);
        return ML_NOMEM;
    }
    return ML_OK;
}

ml_status ml_new_string(ml_vm *api, const char *s, size_t len, ml_value *out) {
    ENTER(api);
    if ((!s && len) || !out) return misuse(api, "ml_new_string: bad arguments");
    if (len && !utf8_valid((const uint8_t *)s, len))
        return exceptionf(api->core, ML_TYPE_ERROR, "string is not valid UTF-8");
    return new_object_handle(api, v_obj(new_string(api->core, s ? s : "", len)), out);
}

ml_status ml_new_list(ml_vm *api, ml_value *out) {
    ENTER(api);
    if (!out) return misuse(api, "ml_new_list: no output handle");
    return new_object_handle(api, v_obj(new_list(api->core, 0)), out);
}

ml_type ml_typeof(ml_vm *api, ml_value h) {
    if (!entry_ok(api, __func__)) return ML_TINVALID;
    Value v;
    if (!get(api, h, &v, __func__)) return ML_TINVALID;
    switch (v.k) {
    case V_NIL: return ML_TNIL;
    case V_BOOL: return ML_TBOOL;
    case V_INT: return ML_TINT;
    case V_FLOAT: return ML_TFLOAT;
    case V_BUILTIN: return ML_TFUNCTION;
    case V_UNDEF: return ML_TOTHER;
    case V_OBJ: break;
    }
    switch ((OKind)v.as.o->kind) {
    case O_STRING: return ML_TSTRING;
    case O_BIGINT: return ML_TINT;
    case O_LIST: return ML_TLIST;
    case O_CLOSURE: case O_HOST: return ML_TFUNCTION;
    case O_CODE: return ML_TPROGRAM;
    case O_CELL: return ML_TOTHER;
    }
    return ML_TOTHER;
}

int ml_truthy(ml_vm *api, ml_value h) {
    if (!entry_ok(api, __func__)) return 0;
    Value v;
    return get(api, h, &v, __func__) ? !falsy(v) : 0;
}

int ml_equal(ml_vm *api, ml_value a, ml_value b) {
    if (!entry_ok(api, __func__)) return 0;
    Value x, y;
    if (!get(api, a, &x, __func__) || !get(api, b, &y, __func__)) return 0;
    return values_equal(x, y);
}

ml_status ml_get_bool(ml_vm *api, ml_value h, int *out) {
    ENTER(api);
    Value v;
    GET(api, h, &v);
    if (!out) return misuse(api, "ml_get_bool: no output");
    if (v.k != V_BOOL) return exceptionf(api->core, ML_TYPE_ERROR, "expected a bool, got %s", kind_name(v));
    *out = v.as.b;
    return ML_OK;
}

ml_status ml_get_int(ml_vm *api, ml_value h, int64_t *out) {
    ENTER(api);
    Value v;
    GET(api, h, &v);
    if (!out) return misuse(api, "ml_get_int: no output");
    if (IS_BIG(v)) return exceptionf(api->core, ML_TYPE_ERROR, "integer does not fit in 64 bits");
    if (v.k != V_INT) return exceptionf(api->core, ML_TYPE_ERROR, "expected an integer, got %s", kind_name(v));
    *out = v.as.i;
    return ML_OK;
}

ml_status ml_get_float(ml_vm *api, ml_value h, double *out) {
    ENTER(api);
    VM *vm = api->core;
    Value v;
    GET(api, h, &v);
    if (!out) return misuse(api, "ml_get_float: no output");
    if (!is_number(v)) return exceptionf(vm, ML_TYPE_ERROR, "expected a number, got %s", kind_name(v));
    if (to_double(vm, v, out)) return finish(vm, vm->err); /* OverflowError, as minilang raises */
    return ML_OK;
}

ml_status ml_get_string(ml_vm *api, ml_value h, const char **s, size_t *len) {
    ENTER(api);
    Value v;
    GET(api, h, &v);
    if (!s) return misuse(api, "ml_get_string: no output");
    if (!IS_STR(v)) return exceptionf(api->core, ML_TYPE_ERROR, "expected a string, got %s", kind_name(v));
    *s = AS_STR(v)->s; /* stays valid while h keeps the string alive; objects never move */
    if (len) *len = AS_STR(v)->len;
    return ML_OK;
}

ml_status ml_tostring(ml_vm *api, ml_value h, ml_value *out) {
    ENTER(api);
    VM *vm = api->core;
    Value v;
    GET(api, h, &v);
    if (!out) return misuse(api, "ml_tostring: no output handle");
    if (IS_STR(v)) {
        *out = new_local(api, v);
        return ML_OK;
    }
    Buf t = {0};
    buf_str(&t, "");
    if (stringify(vm, v, &t, 0)) {
        buf_free(&t);
        return finish(vm, vm->err); /* ValueError (over 4,300 digits) or RecursionError */
    }
    ObjString *s = new_string(vm, t.p, t.n); /* v is rooted by h */
    buf_free(&t);
    return new_object_handle(api, v_obj(s), out);
}

/* --------------------------------------------------------------- lists */

static ml_status get_list(ml_vm *api, ml_value h, ObjList **out) {
    Value v;
    if (!get(api, h, &v, "list function")) return ML_MISUSE;
    if (!IS_LIST(v)) return exceptionf(api->core, ML_TYPE_ERROR, "expected a list, got %s", kind_name(v));
    *out = AS_LIST(v);
    return ML_OK;
}

ml_status ml_list_len(ml_vm *api, ml_value list, size_t *len) {
    ENTER(api);
    ObjList *l;
    ml_status st = get_list(api, list, &l);
    if (st != ML_OK) return st;
    if (!len) return misuse(api, "ml_list_len: no output");
    *len = l->n;
    return ML_OK;
}

ml_status ml_list_get(ml_vm *api, ml_value list, size_t i, ml_value *out) {
    ENTER(api);
    ObjList *l;
    ml_status st = get_list(api, list, &l);
    if (st != ML_OK) return st;
    if (!out) return misuse(api, "ml_list_get: no output handle");
    if (i >= l->n)
        return exceptionf(api->core, ML_TYPE_ERROR, "index %llu out of range for length %llu", (unsigned long long)i,
                          (unsigned long long)l->n);
    *out = new_local(api, l->items[i]);
    return ML_OK;
}

ml_status ml_list_set(ml_vm *api, ml_value list, size_t i, ml_value h) {
    ENTER(api);
    ObjList *l;
    Value v;
    ml_status st = get_list(api, list, &l);
    if (st != ML_OK) return st;
    GET(api, h, &v);
    if (i >= l->n)
        return exceptionf(api->core, ML_TYPE_ERROR, "index %llu out of range for length %llu", (unsigned long long)i,
                          (unsigned long long)l->n);
    l->items[i] = v;
    return ML_OK;
}

ml_status ml_list_push(ml_vm *api, ml_value list, ml_value h) {
    ENTER(api);
    ObjList *l;
    Value v;
    ml_status st = get_list(api, list, &l);
    if (st != ML_OK) return st;
    GET(api, h, &v);
    list_push(api->core, l, v); /* both rooted by their handles */
    return over_limit(api->core) ? ML_NOMEM : ML_OK;
}

/* ----------------------------------------------------------------- VMs */

int ml_api_version(void) { return MINILANG_API_VERSION; }

static void output_line(void *ud, const char *s, size_t n) {
    ml_vm *api = ud;
    api->in_output = 1;
    api->out(api->out_ud, s, n);
    api->in_output = 0;
}

ml_vm *ml_open(const ml_options *options) {
    ml_options opt;
    memset(&opt, 0, sizeof opt);
    if (options) opt = *options;
    VMOptions vo;
    vo.stress = opt.gc_stress;
    vo.min_heap = opt.gc_min_heap;
    ml_vm *api = xrealloc(NULL, sizeof *api);
    memset(api, 0, sizeof *api);
    api->opt = opt;
    api->core = vm_new(&vo);
    api->core->host = api;
    api->core->mark_host = mark_handles;
    api->core->call_host = host_call;
    api->core->mem_limit = opt.heap_limit;
    return api;
}

void ml_set_output(ml_vm *api, ml_output fn, void *ud) {
    if (!entry_ok(api, __func__)) return;
    api->out = fn;
    api->out_ud = ud;
    api->core->out = fn ? output_line : stdout_line;
    api->core->out_ud = fn ? api : NULL;
}

size_t ml_close(ml_vm *api) {
    if (!api) return 0;
    if (api->core->nframes > 0 || api->host_depth > 0 || api->in_output) {
        fprintf(stderr, "minilang: API misuse: ml_close called while the VM is running\n");
        fflush(stderr);
        abort();
    }
    api->nlocals = api->npins = 0;
    size_t left = vm_free(api->core);
    free(api->locals);
    free(api->pins);
    free(api->free_pins);
    free(api->scopes);
    free(api);
    return left;
}

void ml_get_stats(ml_vm *api, ml_stats *out) {
    if (!api || !out) return;
    VM *vm = api->core;
    memset(out, 0, sizeof *out);
    out->collections = vm->collections;
    out->heap_bytes = vm->bytes;
    out->peak_bytes = vm->peak_bytes;
    out->live_objects = vm->live_objects;
    for (uint32_t i = 0; i < api->nlocals; i++) out->handles += api->locals[i].gen & 1;
    for (uint32_t i = 0; i < api->npins; i++) out->pinned += api->pins[i].gen & 1;
}

ml_status ml_collect(ml_vm *api) {
    ENTER(api);
    VM *vm = api->core;
    collect(vm);
    if (!vm->mem_limit || vm->bytes <= vm->mem_limit) vm->over_limit = 0;
    return ML_OK;
}
