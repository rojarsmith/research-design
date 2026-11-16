/* mlvm.c: a C11 virtual machine for minilang with a mark-sweep garbage collector.
 *
 * Stage 5 of the research roadmap. It loads the bytecode files written by
 * ../minilang-vm/minilang_vm.py --emit (format 1.0, described in that file and
 * in README.md) and runs them with the same output, error messages and line
 * numbers as the tree-walker in ../minilang. The front end stays in Python;
 * this file is a loader, a verifier, an interpreter and a heap.
 *
 * Build (one line, no other dependencies):
 *     gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlvm mlvm.c -lm
 * Run:
 *     mlvm [--gc-stress] [--gc-min BYTES] [--stats] program.mlc
 *     mlvm --dis program.mlc      same text as minilang_vm.py --dis
 *     mlvm --serve                line protocol used by minilang_c.py and difftest.py
 *
 * Sections: platform, buffers, values and objects, the VM, heap and
 * collector, strings, tables, integers, number text, code object loading,
 * formatting, disassembler, errors, operators, builtins, interpreter, driver.
 *
 * As a library (roadmap stage 6): ../minilang-embed/minilang.c defines
 * MLVM_LIBRARY and includes this file, which leaves out the command-line
 * driver; the embedding API uses the hooks marked "embedding" below (host
 * functions, extra roots, a heap limit, runs that start above a host's
 * call). The command-line VM leaves them all zero.
 *
 * Memory model: every heap object is on one linked list and is freed only by
 * the sweep phase. Roots are listed explicitly in mark_roots(). Any C code that
 * holds an object pointer across an allocation must keep the object reachable
 * from a root (usually by leaving it on the VM stack or pushing it on the temp
 * root stack). --gc-stress collects on every allocation so that a missing root
 * shows up as a wrong result or a sanitizer report instead of a rare crash.
 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------------------------------------------------------------- platform */
/* Everything platform specific is in this section (roadmap stage 7: keep
 * platform dependencies in one layer). */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
static uint64_t now_ns(void) {
    static LARGE_INTEGER freq;
    LARGE_INTEGER count;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    uint64_t c = (uint64_t)count.QuadPart, f = (uint64_t)freq.QuadPart;
    return c / f * 1000000000u + c % f * 1000000000u / f;
}
#ifndef MLVM_LIBRARY
static void binary_stdio(void) {
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stdin), _O_BINARY);
}
#endif
#else
static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}
#ifndef MLVM_LIBRARY
static void binary_stdio(void) {}
#endif
#endif

static double wall_seconds(void) { /* minilang's clock(): Python's time.time() */
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static _Noreturn void fatal(const char *what) {
    fprintf(stderr, "mlvm: fatal: %s\n", what);
    exit(70);
}

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) fatal("out of memory");
    return q;
}

/* ----------------------------------------------------------------- buffers */
/* Growable byte strings for text that is not a minilang value: output lines,
 * error messages, number formatting. They live outside the GC heap. */

typedef struct {
    char *p;
    size_t n, cap;
} Buf;

static void buf_reserve(Buf *b, size_t extra) {
    if (b->n + extra + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 64;
        while (cap < b->n + extra + 1) cap *= 2;
        b->p = xrealloc(b->p, cap);
        b->cap = cap;
    }
}
static void buf_add(Buf *b, const char *s, size_t n) {
    buf_reserve(b, n);
    if (n) memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}
static void buf_str(Buf *b, const char *s) { buf_add(b, s, strlen(s)); }
static void buf_ch(Buf *b, char c) { buf_add(b, &c, 1); }
static void buf_u64(Buf *b, uint64_t v) {
    char t[24];
    int i = 24;
    do {
        t[--i] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    buf_add(b, t + i, (size_t)(24 - i));
}
static void buf_i64(Buf *b, int64_t v) {
    if (v < 0) {
        buf_ch(b, '-');
        buf_u64(b, (uint64_t)0 - (uint64_t)v);
    } else {
        buf_u64(b, (uint64_t)v);
    }
}
static void buf_vprintf(Buf *b, const char *fmt, va_list ap) {
    char small[256];
    va_list copy;
    va_copy(copy, ap);
    int n = vsnprintf(small, sizeof small, fmt, copy);
    va_end(copy);
    if (n < 0) fatal("bad format");
    if ((size_t)n < sizeof small) {
        buf_add(b, small, (size_t)n);
        return;
    }
    buf_reserve(b, (size_t)n);
    vsnprintf(b->p + b->n, (size_t)n + 1, fmt, ap);
    b->n += (size_t)n;
}
static void buf_printf(Buf *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    buf_vprintf(b, fmt, ap);
    va_end(ap);
}
static void buf_clear(Buf *b) {
    b->n = 0;
    if (b->p) b->p[0] = 0;
}
static void buf_free(Buf *b) {
    free(b->p);
    b->p = NULL;
    b->n = b->cap = 0;
}

/* ------------------------------------------------------- values and objects */
/* A Value is a tagged union (the roadmap's default value model): nil, bool,
 * 64-bit int, double and builtins are immediate; strings, integers outside
 * 64 bits, lists, cells, closures and code objects are heap objects. */

typedef enum { V_NIL, V_BOOL, V_INT, V_FLOAT, V_BUILTIN, V_UNDEF, V_OBJ } VKind;
typedef enum { O_STRING, O_BIGINT, O_LIST, O_CELL, O_CLOSURE, O_CODE, O_HOST } OKind;

typedef struct Obj {
    struct Obj *next; /* every object is on vm->objects */
    size_t size;      /* bytes of this allocation (not counting side buffers) */
    uint8_t kind;
    uint8_t marked;
} Obj;

typedef struct {
    VKind k;
    union {
        int b;
        int64_t i;
        double f;
        Obj *o;
    } as;
} Value;

typedef struct {
    Obj obj;
    size_t len;    /* bytes, UTF-8 */
    size_t nchars; /* code points: len() and indexing count these, as Python does */
    uint32_t hash; /* only for interned strings */
    uint8_t ascii;
    uint8_t interned;
    char s[];
} ObjString;

typedef uint32_t limb;
typedef struct {
    Obj obj;
    int neg;
    size_t n; /* limbs, little-endian; never fits in int64 (see int_result) */
    limb d[];
} ObjBig;

typedef struct {
    Obj obj;
    size_t n, cap;
    Value *items;
} ObjList;

typedef struct {
    Obj obj;
    Value v;
} ObjCell;

typedef struct {
    int32_t start, end, target;
} Handler;
typedef struct {
    int32_t start, end;
} Loop;
typedef struct {
    ObjString *name;
    uint32_t nmaybe;
    uint32_t *maybe;  /* upvalues to try, innermost first, skipping UNDEFINED */
    uint8_t is_upval; /* final step: an upvalue, or the global table */
    uint32_t final;
} Chain;
typedef struct {
    uint8_t local; /* from the enclosing frame's slot, or its upvalue */
    uint32_t index;
    ObjString *name;
} UpSpec;

typedef struct ObjCode {
    Obj obj;
    ObjString *name;
    uint32_t nparams;
    ObjString **params;
    uint32_t nslots, max_stack;
    uint32_t ncode;  /* int32 units: opcode, operand, opcode, operand, ... */
    int32_t *code;
    uint32_t nconsts;
    Value *consts;
    uint32_t nlines;
    int32_t *lines; /* pairs: first offset, line */
    uint32_t nhandlers;
    Handler *handlers;
    uint32_t nloops;
    Loop *loops;
    uint32_t nchains;
    Chain *chains;
    uint32_t nup;
    UpSpec *ups;
    ObjString **slot_names; /* nslots entries */
} ObjCode;

typedef struct {
    Obj obj;
    ObjCode *code;
    uint32_t nup;
    ObjCell *up[];
} ObjClosure;

/* Embedding: a function written in C by the host. To minilang it is a
 * builtin (type "function", printed <builtin NAME>, equal only to itself);
 * the command-line VM never creates one. */
typedef struct ObjHost {
    Obj obj;
    ObjString *name;
    int arity;        /* -1: any number of arguments */
    void (*fn)(void); /* the host's function, cast back by the embedding layer */
    void *ud;
} ObjHost;

static inline Value v_nil(void) { Value v; v.k = V_NIL; v.as.i = 0; return v; }
static inline Value v_undef(void) { Value v; v.k = V_UNDEF; v.as.i = 0; return v; }
static inline Value v_bool(int b) { Value v; v.k = V_BOOL; v.as.i = 0; v.as.b = b != 0; return v; }
static inline Value v_int(int64_t i) { Value v; v.k = V_INT; v.as.i = i; return v; }
static inline Value v_float(double f) { Value v; v.k = V_FLOAT; v.as.f = f; return v; }
static inline Value v_builtin(int i) { Value v; v.k = V_BUILTIN; v.as.i = i; return v; }
static inline Value v_obj(void *o) { Value v; v.k = V_OBJ; v.as.o = (Obj *)o; return v; }

static inline int is_kind(Value v, OKind k) { return v.k == V_OBJ && v.as.o->kind == k; }
#define IS_STR(v) is_kind(v, O_STRING)
#define IS_LIST(v) is_kind(v, O_LIST)
#define IS_BIG(v) is_kind(v, O_BIGINT)
#define IS_CLOSURE(v) is_kind(v, O_CLOSURE)
#define AS_STR(v) ((ObjString *)(v).as.o)
#define AS_LIST(v) ((ObjList *)(v).as.o)
#define AS_BIG(v) ((ObjBig *)(v).as.o)
#define AS_CELL(v) ((ObjCell *)(v).as.o)
#define AS_CLOSURE(v) ((ObjClosure *)(v).as.o)
#define AS_CODE(v) ((ObjCode *)(v).as.o)

static inline int is_intval(Value v) { return v.k == V_INT || IS_BIG(v); }
static inline int is_number(Value v) { return v.k == V_INT || v.k == V_FLOAT || IS_BIG(v); }
static inline int falsy(Value v) { return v.k == V_NIL || (v.k == V_BOOL && !v.as.b); }

/* ------------------------------------------------------------------ the VM */

#define MAX_DEPTH 400   /* minilang's call depth limit, reported as stack overflow */
#define MAX_NESTING 1000 /* nesting depth for str()/repr of lists; see README */
#define STACK_SLACK 1 /* ADD_CONST and friends push their constant on the slow path */
#define GC_GROWTH 2               /* collect when the heap has doubled since the last collection */
#define GC_MIN_HEAP (1024 * 1024) /* never collect below this, unless stressing */

typedef enum { E_NONE, E_RUNTIME, E_RECURSION, E_HOST, E_RETURN, E_BREAK } ErrKind;

typedef struct {
    ObjClosure *closure; /* NULL for the script */
    ObjCode *code;
    size_t pc;   /* saved while a callee runs */
    size_t base; /* stack index of slot 0 */
} Frame;

typedef struct {
    ObjString *key; /* NULL: empty (val nil) or tombstone (val true) */
    Value val;
} Entry;
typedef struct {
    size_t count, cap; /* count includes tombstones */
    Entry *e;
} Table;

typedef void (*OutputFn)(void *ud, const char *s, size_t n);

typedef struct {
    int stress;
    size_t min_heap;
} VMOptions;

typedef struct VM {
    /* value stack: every frame's slots, then its operands */
    Value *stack;
    size_t stack_cap, sp;
    Frame frames[MAX_DEPTH + 1];
    int nframes;
    Table globals, strings; /* strings is the intern table, weak */
    /* heap */
    Obj *objects;
    size_t bytes, next_gc, min_heap;
    int stress;
    Obj **gray;
    size_t ngray, gray_cap;
    Value *temps; /* explicit roots for C code that allocates twice; grows, because */
    int ntemps, temps_cap; /* the loader keeps one per level of function nesting */
    /* statistics */
    uint64_t total_bytes, total_objects, freed_objects, freed_bytes, collections;
    uint64_t pause_ns_total, pause_ns_max, mark_ns, sweep_ns, run_ns, load_ns;
    size_t peak_bytes, live_objects, live_after_gc;
    /* the error being raised */
    ErrKind err;
    Buf errmsg;
    const char *host_type; /* E_HOST: Python exception type the tree-walker raises */
    int errline;           /* -1: the faulting instruction's line, filled in by the loop */
    int has_errval;        /* else the caught value is errmsg as a string */
    Value errval;          /* thrown value, or escaped return value */
    /* output */
    OutputFn out;
    void *out_ud;
    Buf line; /* print()'s line, reused */
    /* embedding: all zero in the command-line VM */
    size_t mem_limit; /* heap limit in bytes, 0 for none; see heap_realloc() */
    int over_limit;   /* the heap passed mem_limit even after a collection */
    void (*mark_host)(struct VM *vm);                                    /* marks the host's handles */
    int (*call_host)(struct VM *vm, ObjHost *fn, int nargs, Value *out); /* args are the top nargs values */
    void *host;
} VM;

static void collect(VM *vm);

/* ------------------------------------------------------ heap and collector */

/* All object memory goes through here, so the VM knows how many bytes are
 * live and when to collect. Growing an allocation may collect first; at that
 * moment everything the caller still needs must be reachable from a root. */
static void *heap_realloc(VM *vm, void *p, size_t old, size_t new_size) {
    if (new_size > old) {
        vm->total_bytes += new_size - old;
        if (vm->stress || vm->bytes + (new_size - old) > vm->next_gc) collect(vm);
        /* Embedding: a host-set heap limit is soft. The allocation still
         * succeeds, so no code here needs a failure path; the interpreter
         * raises MemoryError at its next jump or call, and the embedding API
         * checks over_limit after each allocation it makes. */
        if (vm->mem_limit && vm->bytes + (new_size - old) > vm->mem_limit) {
            collect(vm);
            if (vm->bytes + (new_size - old) > vm->mem_limit) vm->over_limit = 1;
        }
    } else {
        vm->freed_bytes += old - new_size;
    }
    vm->bytes = vm->bytes - old + new_size;
    if (vm->bytes > vm->peak_bytes) vm->peak_bytes = vm->bytes;
    if (new_size == 0) {
        free(p);
        return NULL;
    }
    return xrealloc(p, new_size);
}

static Obj *new_obj(VM *vm, OKind kind, size_t size) {
    Obj *o = heap_realloc(vm, NULL, 0, size); /* may collect: o is not linked yet */
    memset(o, 0, size);
    o->kind = (uint8_t)kind;
    o->size = size;
    o->next = vm->objects;
    vm->objects = o;
    vm->total_objects++;
    vm->live_objects++;
    return o;
}

static void push_temp(VM *vm, Value v) { /* malloc, not the GC heap: never collects */
    if (vm->ntemps == vm->temps_cap) {
        vm->temps_cap = vm->temps_cap ? vm->temps_cap * 2 : 64;
        vm->temps = xrealloc(vm->temps, (size_t)vm->temps_cap * sizeof(Value));
    }
    vm->temps[vm->ntemps++] = v;
}
static void pop_temp(VM *vm) { vm->ntemps--; }

static void mark_obj(VM *vm, Obj *o) {
    if (o == NULL || o->marked) return;
    o->marked = 1;
    if (vm->ngray == vm->gray_cap) { /* the gray stack is outside the heap */
        vm->gray_cap = vm->gray_cap ? vm->gray_cap * 2 : 256;
        vm->gray = xrealloc(vm->gray, vm->gray_cap * sizeof *vm->gray);
    }
    vm->gray[vm->ngray++] = o;
}
static void mark_value(VM *vm, Value v) {
    if (v.k == V_OBJ) mark_obj(vm, v.as.o);
}

/* Trace an object's references. Iterative (gray stack), so a list nested a
 * million deep does not overflow the C stack. */
static void blacken(VM *vm, Obj *o) {
    switch ((OKind)o->kind) {
    case O_STRING:
    case O_BIGINT:
        break;
    case O_LIST: {
        ObjList *l = (ObjList *)o;
        for (size_t i = 0; i < l->n; i++) mark_value(vm, l->items[i]);
        break;
    }
    case O_CELL:
        mark_value(vm, ((ObjCell *)o)->v);
        break;
    case O_CLOSURE: {
        ObjClosure *c = (ObjClosure *)o;
        mark_obj(vm, (Obj *)c->code);
        for (uint32_t i = 0; i < c->nup; i++) mark_obj(vm, (Obj *)c->up[i]);
        break;
    }
    case O_CODE: {
        ObjCode *c = (ObjCode *)o;
        mark_obj(vm, (Obj *)c->name);
        for (uint32_t i = 0; i < c->nparams; i++) mark_obj(vm, (Obj *)c->params[i]);
        for (uint32_t i = 0; i < c->nconsts; i++) mark_value(vm, c->consts[i]);
        for (uint32_t i = 0; i < c->nchains; i++) mark_obj(vm, (Obj *)c->chains[i].name);
        for (uint32_t i = 0; i < c->nup; i++) mark_obj(vm, (Obj *)c->ups[i].name);
        if (c->slot_names)
            for (uint32_t i = 0; i < c->nslots; i++) mark_obj(vm, (Obj *)c->slot_names[i]);
        break;
    }
    case O_HOST:
        mark_obj(vm, (Obj *)((ObjHost *)o)->name);
        break;
    }
}

/* The root set. Anything not reachable from here is garbage. */
static void mark_roots(VM *vm) {
    for (size_t i = 0; i < vm->sp; i++) mark_value(vm, vm->stack[i]);
    for (int i = 0; i < vm->nframes; i++) {
        mark_obj(vm, (Obj *)vm->frames[i].closure);
        mark_obj(vm, (Obj *)vm->frames[i].code);
    }
    for (size_t i = 0; i < vm->globals.cap; i++) {
        Entry *e = &vm->globals.e[i];
        if (e->key) {
            mark_obj(vm, (Obj *)e->key);
            mark_value(vm, e->val);
        }
    }
    for (int i = 0; i < vm->ntemps; i++) mark_value(vm, vm->temps[i]);
    if (vm->has_errval) mark_value(vm, vm->errval);
    if (vm->mark_host) vm->mark_host(vm); /* embedding: values the host holds through handles */
    /* vm->strings is weak: interned strings survive only if marked elsewhere */
}

static void free_obj(VM *vm, Obj *o) {
    switch ((OKind)o->kind) {
    case O_LIST: {
        ObjList *l = (ObjList *)o;
        heap_realloc(vm, l->items, l->cap * sizeof(Value), 0);
        break;
    }
    case O_CODE: {
        ObjCode *c = (ObjCode *)o;
        heap_realloc(vm, c->params, c->nparams * sizeof(ObjString *), 0);
        heap_realloc(vm, c->code, c->ncode * sizeof(int32_t), 0);
        heap_realloc(vm, c->consts, c->nconsts * sizeof(Value), 0);
        heap_realloc(vm, c->lines, c->nlines * 2 * sizeof(int32_t), 0);
        heap_realloc(vm, c->handlers, c->nhandlers * sizeof(Handler), 0);
        heap_realloc(vm, c->loops, c->nloops * sizeof(Loop), 0);
        for (uint32_t i = 0; i < c->nchains; i++)
            heap_realloc(vm, c->chains[i].maybe, c->chains[i].nmaybe * sizeof(uint32_t), 0);
        heap_realloc(vm, c->chains, c->nchains * sizeof(Chain), 0);
        heap_realloc(vm, c->ups, c->nup * sizeof(UpSpec), 0);
        heap_realloc(vm, c->slot_names, c->slot_names ? c->nslots * sizeof(ObjString *) : 0, 0);
        break;
    }
    default:
        break;
    }
    size_t size = o->size;
    if (vm->stress) memset(o, 0xDB, size); /* poison: a dangling pointer now reads garbage */
    heap_realloc(vm, o, size, 0);
    vm->freed_objects++;
    vm->live_objects--;
}

static void table_remove_unmarked(Table *t);

static void collect(VM *vm) {
    uint64_t t0 = now_ns();
    mark_roots(vm);
    while (vm->ngray) blacken(vm, vm->gray[--vm->ngray]);
    table_remove_unmarked(&vm->strings);
    uint64_t t1 = now_ns();
    Obj **link = &vm->objects;
    while (*link) {
        Obj *o = *link;
        if (o->marked) {
            o->marked = 0;
            link = &o->next;
        } else {
            *link = o->next;
            free_obj(vm, o);
        }
    }
    vm->live_after_gc = vm->bytes;
    size_t grow = vm->bytes * GC_GROWTH;
    vm->next_gc = grow > vm->min_heap ? grow : vm->min_heap;
    uint64_t t2 = now_ns(), pause = t2 - t0;
    vm->mark_ns += t1 - t0;
    vm->sweep_ns += t2 - t1;
    vm->collections++;
    vm->pause_ns_total += pause;
    if (pause > vm->pause_ns_max) vm->pause_ns_max = pause;
}

/* ----------------------------------------------------------------- strings */

static uint32_t hash_bytes(const char *s, size_t n) { /* FNV-1a */
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h ^= (uint8_t)s[i];
        h *= 16777619u;
    }
    return h;
}

static ObjString *alloc_string(VM *vm, size_t len) {
    ObjString *s = (ObjString *)new_obj(vm, O_STRING, sizeof(ObjString) + len + 1);
    s->len = len;
    s->s[len] = 0;
    return s;
}
static void string_meta(ObjString *s) {
    size_t chars = 0;
    int ascii = 1;
    for (size_t i = 0; i < s->len; i++) {
        uint8_t c = (uint8_t)s->s[i];
        if (c >= 0x80) ascii = 0;
        if ((c & 0xC0) != 0x80) chars++;
    }
    s->nchars = chars;
    s->ascii = (uint8_t)ascii;
}
static ObjString *new_string(VM *vm, const char *text, size_t len) {
    ObjString *s = alloc_string(vm, len);
    if (len) memcpy(s->s, text, len);
    string_meta(s);
    return s;
}

static int str_equal(const ObjString *a, const ObjString *b) {
    return a == b || (a->len == b->len && memcmp(a->s, b->s, a->len) == 0);
}
static int str_compare(const ObjString *a, const ObjString *b) {
    /* Byte order of UTF-8 is code point order, which is how Python compares. */
    size_t n = a->len < b->len ? a->len : b->len;
    int c = memcmp(a->s, b->s, n);
    if (c) return c < 0 ? -1 : 1;
    return a->len < b->len ? -1 : a->len > b->len;
}

/* Decode one UTF-8 sequence (the loader only accepts valid UTF-8). */
static uint32_t utf8_decode(const char *s, size_t *len) {
    uint8_t c = (uint8_t)s[0];
    if (c < 0x80) { *len = 1; return c; }
    if (c < 0xE0) { *len = 2; return (uint32_t)(c & 0x1F) << 6 | (s[1] & 0x3F); }
    if (c < 0xF0) {
        *len = 3;
        return (uint32_t)(c & 0x0F) << 12 | (uint32_t)(s[1] & 0x3F) << 6 | (s[2] & 0x3F);
    }
    *len = 4;
    return (uint32_t)(c & 0x07) << 18 | (uint32_t)(s[1] & 0x3F) << 12 |
           (uint32_t)(s[2] & 0x3F) << 6 | (s[3] & 0x3F);
}
static int utf8_valid(const uint8_t *s, size_t n) {
    size_t i = 0;
    while (i < n) {
        uint8_t c = s[i];
        size_t len;
        uint32_t cp, min;
        if (c < 0x80) { i++; continue; }
        if (c >= 0xC2 && c < 0xE0) { len = 2; cp = c & 0x1F; min = 0x80; }
        else if (c >= 0xE0 && c < 0xF0) { len = 3; cp = c & 0x0F; min = 0x800; }
        else if (c >= 0xF0 && c < 0xF5) { len = 4; cp = c & 0x07; min = 0x10000; }
        else return 0;
        if (i + len > n) return 0;
        for (size_t k = 1; k < len; k++) {
            if ((s[i + k] & 0xC0) != 0x80) return 0;
            cp = cp << 6 | (s[i + k] & 0x3F);
        }
        if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp < 0xE000)) return 0;
        i += len;
    }
    return 1;
}

/* ------------------------------------------------------------------ tables */
/* Open addressing with linear probing. Table arrays use malloc, not the GC
 * heap, so inserting never collects (clox has to push the key on the stack
 * first because its tables do). */

static Entry *table_slot(Entry *e, size_t cap, ObjString *key) {
    size_t i = key->hash & (cap - 1);
    Entry *tomb = NULL;
    for (;;) {
        Entry *x = &e[i];
        if (x->key == NULL) {
            if (x->val.k == V_NIL) return tomb ? tomb : x;
            if (!tomb) tomb = x;
        } else if (x->key == key) {
            return x;
        }
        i = (i + 1) & (cap - 1);
    }
}
static void table_grow(Table *t) {
    size_t cap = t->cap ? t->cap * 2 : 16;
    Entry *e = xrealloc(NULL, cap * sizeof(Entry));
    for (size_t i = 0; i < cap; i++) {
        e[i].key = NULL;
        e[i].val = v_nil();
    }
    t->count = 0;
    for (size_t i = 0; i < t->cap; i++) {
        Entry *old = &t->e[i];
        if (!old->key) continue;
        Entry *x = table_slot(e, cap, old->key);
        *x = *old;
        t->count++;
    }
    free(t->e);
    t->e = e;
    t->cap = cap;
}
/* Keys are interned strings, so they compare by pointer. */
static int table_get(Table *t, ObjString *key, Value *out) {
    if (!t->cap) return 0;
    Entry *x = table_slot(t->e, t->cap, key);
    if (!x->key) return 0;
    *out = x->val;
    return 1;
}
static Entry *table_find(Table *t, ObjString *key) {
    if (!t->cap) return NULL;
    Entry *x = table_slot(t->e, t->cap, key);
    return x->key ? x : NULL;
}
static void table_set(Table *t, ObjString *key, Value val) {
    if ((t->count + 1) * 4 > t->cap * 3) table_grow(t);
    Entry *x = table_slot(t->e, t->cap, key);
    if (!x->key && x->val.k == V_NIL) t->count++;
    x->key = key;
    x->val = val;
}
static ObjString *table_find_text(Table *t, const char *s, size_t n, uint32_t hash) {
    if (!t->cap) return NULL;
    size_t i = hash & (t->cap - 1);
    for (;;) {
        Entry *x = &t->e[i];
        if (x->key == NULL) {
            if (x->val.k == V_NIL) return NULL;
        } else if (x->key->hash == hash && x->key->len == n && memcmp(x->key->s, s, n) == 0) {
            return x->key;
        }
        i = (i + 1) & (t->cap - 1);
    }
}
static void table_remove_unmarked(Table *t) {
    for (size_t i = 0; i < t->cap; i++) {
        Entry *x = &t->e[i];
        if (x->key && !x->key->obj.marked) {
            x->key = NULL;
            x->val = v_bool(1); /* tombstone */
        }
    }
}
static void table_free(Table *t) {
    free(t->e);
    t->e = NULL;
    t->cap = t->count = 0;
}

/* Constants, global names and builtin names are interned; strings built at
 * run time are not, so building a string costs no table lookup. */
static ObjString *intern(VM *vm, const char *s, size_t n) {
    uint32_t h = hash_bytes(s, n);
    ObjString *found = table_find_text(&vm->strings, s, n, h);
    if (found) return found;
    ObjString *str = new_string(vm, s, n);
    str->hash = h;
    str->interned = 1;
    table_set(&vm->strings, str, v_nil());
    return str;
}

/* ---------------------------------------------------------------- integers */
/* minilang inherits Python's integers, which have no size limit, so the VM
 * keeps an int64 fast path and falls back to sign-magnitude integers with
 * 32-bit limbs. Results that fit in int64 are always stored as V_INT. */

typedef struct {
    int neg;
    size_t n;
    limb *d;
    limb tmp[2];
    limb *own;
} Big;

static void big_view(Value v, Big *b) { /* no allocation; v must stay rooted */
    b->own = NULL;
    if (v.k == V_INT) {
        uint64_t m = v.as.i < 0 ? (uint64_t)0 - (uint64_t)v.as.i : (uint64_t)v.as.i;
        b->neg = v.as.i < 0;
        b->tmp[0] = (limb)m;
        b->tmp[1] = (limb)(m >> 32);
        b->n = m == 0 ? 0 : (m >> 32) ? 2 : 1;
        b->d = b->tmp;
    } else {
        ObjBig *o = AS_BIG(v);
        b->neg = o->neg;
        b->n = o->n;
        b->d = o->d;
    }
}
static void big_alloc(Big *b, size_t n) {
    b->own = calloc(n ? n : 1, sizeof(limb));
    if (!b->own) fatal("out of memory");
    b->d = b->own;
    b->n = n;
    b->neg = 0;
}
static void big_release(Big *b) {
    free(b->own);
    b->own = NULL;
}
static void big_trim(Big *b) {
    while (b->n && b->d[b->n - 1] == 0) b->n--;
    if (b->n == 0) b->neg = 0;
}
static Value int_result(VM *vm, Big *b) {
    big_trim(b);
    if (b->n <= 2) {
        uint64_t m = b->n == 0 ? 0 : b->n == 1 ? b->d[0] : ((uint64_t)b->d[1] << 32 | b->d[0]);
        if (!b->neg && m <= (uint64_t)INT64_MAX) return v_int((int64_t)m);
        if (b->neg && m <= (uint64_t)INT64_MAX) return v_int(-(int64_t)m);
        if (b->neg && m == (uint64_t)INT64_MAX + 1) return v_int(INT64_MIN);
    }
    ObjBig *o = (ObjBig *)new_obj(vm, O_BIGINT, sizeof(ObjBig) + b->n * sizeof(limb));
    o->neg = b->neg;
    o->n = b->n;
    memcpy(o->d, b->d, b->n * sizeof(limb));
    return v_obj(o);
}

static int mag_cmp(const limb *a, size_t an, const limb *b, size_t bn) {
    if (an != bn) return an < bn ? -1 : 1;
    for (size_t i = an; i-- > 0;)
        if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}
static size_t mag_add(const limb *a, size_t an, const limb *b, size_t bn, limb *r) {
    if (an < bn) {
        const limb *t = a; a = b; b = t;
        size_t tn = an; an = bn; bn = tn;
    }
    uint64_t c = 0;
    size_t i = 0;
    for (; i < bn; i++) { c += (uint64_t)a[i] + b[i]; r[i] = (limb)c; c >>= 32; }
    for (; i < an; i++) { c += a[i]; r[i] = (limb)c; c >>= 32; }
    r[i] = (limb)c;
    return an + 1;
}
static size_t mag_sub(const limb *a, size_t an, const limb *b, size_t bn, limb *r) { /* a >= b */
    uint64_t borrow = 0;
    size_t i = 0;
    for (; i < bn; i++) {
        uint64_t t = (uint64_t)a[i] - b[i] - borrow;
        r[i] = (limb)t;
        borrow = (t >> 32) & 1;
    }
    for (; i < an; i++) {
        uint64_t t = (uint64_t)a[i] - borrow;
        r[i] = (limb)t;
        borrow = (t >> 32) & 1;
    }
    return an;
}
static void mag_mul(const limb *a, size_t an, const limb *b, size_t bn, limb *r) { /* r zeroed */
    for (size_t i = 0; i < an; i++) {
        uint64_t c = 0;
        for (size_t j = 0; j < bn; j++) {
            c += (uint64_t)a[i] * b[j] + r[i + j];
            r[i + j] = (limb)c;
            c >>= 32;
        }
        r[i + bn] = (limb)c;
    }
}
static int nlz32(limb x) {
    int n = 0;
    if (x == 0) return 32;
    while (!(x & 0x80000000u)) { x <<= 1; n++; }
    return n;
}
static size_t mag_bitlen(const limb *d, size_t n) {
    return n == 0 ? 0 : (n - 1) * 32 + (size_t)(32 - nlz32(d[n - 1]));
}
/* q = u / v, r = u % v for m >= n >= 1, v[n-1] != 0; q has m-n+1 limbs, r has
 * n limbs. Knuth's algorithm D as written in Hacker's Delight (divmnu). */
static void mag_divmod(const limb *u, size_t m, const limb *v, size_t n, limb *q, limb *r) {
    if (n == 1) {
        uint64_t k = 0;
        for (size_t j = m; j-- > 0;) {
            uint64_t cur = k << 32 | u[j];
            q[j] = (limb)(cur / v[0]);
            k = cur % v[0];
        }
        if (r) r[0] = (limb)k;
        return;
    }
    int s = nlz32(v[n - 1]);
    limb *vn = xrealloc(NULL, n * sizeof(limb));
    limb *un = xrealloc(NULL, (m + 1) * sizeof(limb));
    for (size_t i = n - 1; i > 0; i--)
        vn[i] = (limb)(v[i] << s | (s ? (limb)((uint64_t)v[i - 1] >> (32 - s)) : 0));
    vn[0] = v[0] << s;
    un[m] = s ? (limb)((uint64_t)u[m - 1] >> (32 - s)) : 0;
    for (size_t i = m - 1; i > 0; i--)
        un[i] = (limb)(u[i] << s | (s ? (limb)((uint64_t)u[i - 1] >> (32 - s)) : 0));
    un[0] = u[0] << s;
    for (size_t j = m - n + 1; j-- > 0;) {
        uint64_t num = (uint64_t)un[j + n] << 32 | un[j + n - 1];
        uint64_t qhat = num / vn[n - 1];
        uint64_t rhat = num % vn[n - 1];
        while (qhat >> 32 || qhat * vn[n - 2] > (rhat << 32 | un[j + n - 2])) {
            qhat--;
            rhat += vn[n - 1];
            if (rhat >> 32) break;
        }
        uint64_t borrow = 0, carry = 0;
        for (size_t i = 0; i < n; i++) {
            uint64_t p = qhat * vn[i] + carry;
            carry = p >> 32;
            uint64_t t = (uint64_t)un[i + j] - (p & 0xFFFFFFFFu) - borrow;
            un[i + j] = (limb)t;
            borrow = (t >> 32) & 1;
        }
        uint64_t t = (uint64_t)un[j + n] - carry - borrow;
        un[j + n] = (limb)t;
        q[j] = (limb)qhat;
        if (t >> 32) { /* subtracted too much: add back */
            q[j]--;
            uint64_t c = 0;
            for (size_t i = 0; i < n; i++) {
                c += (uint64_t)un[i + j] + vn[i];
                un[i + j] = (limb)c;
                c >>= 32;
            }
            un[j + n] = (limb)(un[j + n] + c);
        }
    }
    if (r)
        for (size_t i = 0; i < n; i++)
            r[i] = (limb)(un[i] >> s | (s ? (limb)((uint64_t)un[i + 1] << (32 - s)) : 0));
    free(vn);
    free(un);
}
static limb mag_divsmall(limb *d, size_t n, limb v) { /* in place; returns remainder */
    uint64_t k = 0;
    for (size_t j = n; j-- > 0;) {
        uint64_t cur = k << 32 | d[j];
        d[j] = (limb)(cur / v);
        k = cur % v;
    }
    return (limb)k;
}
static void mag_shl(const limb *a, size_t an, size_t bits, Big *out) {
    size_t w = bits / 32, s = bits % 32;
    big_alloc(out, an + w + 1);
    for (size_t i = 0; i < an; i++) {
        uint64_t x = (uint64_t)a[i] << s;
        out->d[i + w] |= (limb)x;
        out->d[i + w + 1] |= (limb)(x >> 32);
    }
}
static int mag_shr(const limb *a, size_t an, size_t bits, Big *out) { /* returns 1 if bits were lost */
    size_t w = bits / 32, s = bits % 32;
    int lost = 0;
    for (size_t i = 0; i < w && i < an; i++) lost |= a[i] != 0;
    if (w < an && s) lost |= (a[w] & (((limb)1 << s) - 1)) != 0;
    big_alloc(out, w < an ? an - w : 0);
    for (size_t i = w; i < an; i++) {
        uint64_t x = a[i] >> s;
        if (s && i + 1 < an) x |= (uint64_t)a[i + 1] << (32 - s);
        out->d[i - w] = (limb)x;
    }
    return lost;
}
static uint64_t mag_u64(const limb *d, size_t n) { /* n <= 2 */
    return n == 0 ? 0 : n == 1 ? d[0] : ((uint64_t)d[1] << 32 | d[0]);
}

static Value int_addsub(VM *vm, Value x, Value y, int subtract) {
    Big a, b, r;
    big_view(x, &a);
    big_view(y, &b);
    int bneg = subtract ? (b.n ? !b.neg : 0) : b.neg;
    big_alloc(&r, (a.n > b.n ? a.n : b.n) + 1);
    if (a.neg == bneg) {
        r.n = mag_add(a.d, a.n, b.d, b.n, r.d);
        r.neg = a.neg;
    } else if (mag_cmp(a.d, a.n, b.d, b.n) >= 0) {
        r.n = mag_sub(a.d, a.n, b.d, b.n, r.d);
        r.neg = a.neg;
    } else {
        r.n = mag_sub(b.d, b.n, a.d, a.n, r.d);
        r.neg = bneg;
    }
    Value v = int_result(vm, &r);
    big_release(&r);
    return v;
}
static Value int_mul(VM *vm, Value x, Value y) {
    Big a, b, r;
    big_view(x, &a);
    big_view(y, &b);
    if (a.n == 0 || b.n == 0) return v_int(0);
    big_alloc(&r, a.n + b.n);
    mag_mul(a.d, a.n, b.d, b.n, r.d);
    r.neg = a.neg != b.neg;
    Value v = int_result(vm, &r);
    big_release(&r);
    return v;
}
static Value int_neg(VM *vm, Value x) {
    if (x.k == V_INT && x.as.i != INT64_MIN) return v_int(-x.as.i);
    Big a, r;
    big_view(x, &a);
    big_alloc(&r, a.n);
    memcpy(r.d, a.d, a.n * sizeof(limb));
    r.neg = !a.neg;
    Value v = int_result(vm, &r);
    big_release(&r);
    return v;
}
/* Python's %: the result takes the sign of the divisor. y != 0. */
static Value int_mod(VM *vm, Value x, Value y) {
    if (x.k == V_INT && y.k == V_INT) {
        if (y.as.i == -1) return v_int(0);
        int64_t r = x.as.i % y.as.i;
        if (r != 0 && ((r < 0) != (y.as.i < 0))) r += y.as.i;
        return v_int(r);
    }
    Big a, b, q, r;
    big_view(x, &a);
    big_view(y, &b);
    if (mag_cmp(a.d, a.n, b.d, b.n) < 0) {
        big_alloc(&r, a.n);
        memcpy(r.d, a.d, a.n * sizeof(limb));
    } else {
        big_alloc(&q, a.n - b.n + 1);
        big_alloc(&r, b.n);
        mag_divmod(a.d, a.n, b.d, b.n, q.d, r.d);
        big_release(&q);
    }
    big_trim(&r);
    if (r.n && a.neg != b.neg) {
        Big t;
        big_alloc(&t, b.n);
        t.n = mag_sub(b.d, b.n, r.d, r.n, t.d);
        big_release(&r);
        r = t;
        r.d = r.own;
    }
    r.neg = r.n ? b.neg : 0;
    Value v = int_result(vm, &r);
    big_release(&r);
    return v;
}
static int int_cmp(Value x, Value y) {
    if (x.k == V_INT && y.k == V_INT) return x.as.i < y.as.i ? -1 : x.as.i > y.as.i;
    Big a, b;
    big_view(x, &a);
    big_view(y, &b);
    if (a.neg != b.neg) return a.neg ? -1 : 1;
    int c = mag_cmp(a.d, a.n, b.d, b.n);
    return a.neg ? -c : c;
}
/* Exact value of an integral, finite double. */
static void big_from_double(double x, Big *b) {
    int neg = x < 0;
    x = fabs(x);
    if (x < 18446744073709551616.0) {
        uint64_t m = (uint64_t)x;
        big_alloc(b, 2);
        b->d[0] = (limb)m;
        b->d[1] = (limb)(m >> 32);
    } else {
        int e;
        double frac = frexp(x, &e); /* x = frac * 2^e, frac in [0.5, 1) */
        uint64_t mant = (uint64_t)ldexp(frac, 53);
        size_t shift = (size_t)(e - 53);
        big_alloc(b, (size_t)e / 32 + 2);
        for (int i = 0; i < 53; i++)
            if (mant >> i & 1) b->d[(shift + (size_t)i) / 32] |= (limb)1 << ((shift + (size_t)i) % 32);
    }
    b->neg = neg;
    big_trim(b);
}
static Value int_from_double(VM *vm, double x) { /* x integral and finite */
    if (x > -9.2e18 && x < 9.2e18) return v_int((int64_t)x);
    Big b;
    big_from_double(x, &b);
    Value v = int_result(vm, &b);
    big_release(&b);
    return v;
}
/* Python's float(int): correctly rounded (half to even); 0, or -1 when too large. */
static int int_to_double(Value x, double *out) {
    if (x.k == V_INT) {
        *out = (double)x.as.i; /* conversion rounds to nearest, as Python does */
        return 0;
    }
    Big a;
    big_view(x, &a);
    size_t nbits = mag_bitlen(a.d, a.n);
    if (nbits > DBL_MAX_EXP) return -1;
    size_t shift = nbits - (DBL_MANT_DIG + 2);
    uint64_t top = 0;
    for (int i = DBL_MANT_DIG + 1; i >= 0; i--) {
        size_t bit = shift + (size_t)i;
        top = top << 1 | ((a.d[bit / 32] >> (bit % 32)) & 1);
    }
    int sticky = 0;
    for (size_t i = 0; i < shift / 32; i++) sticky |= a.d[i] != 0;
    if (shift % 32) sticky |= (a.d[shift / 32] & (((limb)1 << (shift % 32)) - 1)) != 0;
    static const int half_even[8] = {0, -1, -2, 1, 0, -1, 2, 1}; /* CPython's _PyLong_Frexp */
    top |= (uint64_t)sticky;
    top = (uint64_t)((int64_t)top + half_even[top & 7]);
    double r = ldexp((double)top, (int)shift);
    if (isinf(r)) return -1;
    *out = a.neg ? -r : r;
    return 0;
}
/* Exact comparison of an integer with a double: -1, 0, 1, or 2 if d is NaN. */
static int int_cmp_double(Value x, double d) {
    if (isnan(d)) return 2;
    if (isinf(d)) return d > 0 ? -1 : 1;
    if (x.k == V_INT && x.as.i >= -(INT64_C(1) << 53) && x.as.i <= (INT64_C(1) << 53)) {
        double a = (double)x.as.i;
        return a < d ? -1 : a > d;
    }
    double fl = floor(d);
    Big a, f;
    big_view(x, &a);
    big_from_double(fl, &f);
    int c;
    if (a.neg != f.neg) c = a.neg ? -1 : 1;
    else {
        c = mag_cmp(a.d, a.n, f.d, f.n);
        if (a.neg) c = -c;
    }
    big_release(&f);
    if (c) return c;
    return d - fl > 0 ? -1 : 0;
}
/* Python's int / int: the correctly rounded quotient (CPython's
 * long_true_divide, ported). 0, or -1 when the result is too large. */
static int int_true_div(Value x, Value y, double *out) {
    Big a, b;
    big_view(x, &a);
    big_view(y, &b);
    int neg = a.neg != b.neg;
    if (a.n == 0) {
        *out = neg ? -0.0 : 0.0;
        return 0;
    }
    int64_t la = (int64_t)mag_bitlen(a.d, a.n), lb = (int64_t)mag_bitlen(b.d, b.n);
    if (la <= DBL_MANT_DIG && lb <= DBL_MANT_DIG) {
        double r = (double)mag_u64(a.d, a.n) / (double)mag_u64(b.d, b.n);
        *out = neg ? -r : r;
        return 0;
    }
    int64_t diff = la - lb;
    if (diff > DBL_MAX_EXP) return -1;
    if (diff < DBL_MIN_EXP - DBL_MANT_DIG - 1) {
        *out = neg ? -0.0 : 0.0;
        return 0;
    }
    int64_t shift = (diff > DBL_MIN_EXP ? diff : DBL_MIN_EXP) - DBL_MANT_DIG - 2;
    Big xs, q, r;
    int inexact = 0;
    if (shift <= 0) mag_shl(a.d, a.n, (size_t)-shift, &xs);
    else inexact = mag_shr(a.d, a.n, (size_t)shift, &xs);
    big_trim(&xs);
    big_alloc(&q, xs.n - b.n + 1);
    big_alloc(&r, b.n);
    mag_divmod(xs.d, xs.n, b.d, b.n, q.d, r.d);
    big_trim(&r);
    if (r.n) inexact = 1;
    big_trim(&q);
    uint64_t xq = mag_u64(q.d, q.n); /* at most 56 bits */
    big_release(&xs);
    big_release(&q);
    big_release(&r);
    int x_bits = 64 - (xq >> 32 ? nlz32((limb)(xq >> 32)) : 32 + nlz32((limb)xq));
    int64_t min_bits = DBL_MIN_EXP - shift;
    int extra = (int)((x_bits > min_bits ? x_bits : min_bits) - DBL_MANT_DIG);
    uint64_t mask = (uint64_t)1 << (extra - 1);
    uint64_t low = xq | (uint64_t)inexact;
    if ((low & mask) && (low & (3 * mask - 1))) low += mask;
    low &= ~(2 * mask - 1);
    double dx = (double)low;
    if (shift + x_bits >= DBL_MAX_EXP && shift + x_bits > DBL_MAX_EXP + (dx == ldexp(1.0, x_bits)))
        return -1;
    double res = ldexp(dx, (int)shift);
    *out = neg ? -res : res;
    return 0;
}

/* Decimal text of an integer. Python 3.11+ refuses to convert integers of
 * more than 4300 digits to text (sys.get_int_max_str_digits()); so does this. */
#define MAX_STR_DIGITS 4300
static int big_text(const Big *b, Buf *out);
static int int_to_text(Value x, Buf *out) {
    if (x.k == V_INT) {
        buf_i64(out, x.as.i);
        return 0;
    }
    Big a;
    big_view(x, &a);
    return big_text(&a, out);
}
static int big_text(const Big *bp, Buf *out) {
    Big a = *bp;
    if (a.n == 0) {
        buf_ch(out, '0');
        return 0;
    }
    limb *t = xrealloc(NULL, a.n * sizeof(limb));
    memcpy(t, a.d, a.n * sizeof(limb));
    size_t n = a.n, nchunks = 0, cap = a.n * 10 / 9 + 2;
    limb *chunks = xrealloc(NULL, cap * sizeof(limb));
    while (n) {
        chunks[nchunks++] = mag_divsmall(t, n, 1000000000u);
        while (n && t[n - 1] == 0) n--;
    }
    size_t digits = 9 * (nchunks - 1);
    for (limb top = chunks[nchunks - 1]; top; top /= 10) digits++;
    int ok = digits <= MAX_STR_DIGITS;
    if (ok) {
        if (a.neg) buf_ch(out, '-');
        buf_u64(out, chunks[nchunks - 1]);
        for (size_t i = nchunks - 1; i-- > 0;) {
            char piece[10];
            limb c = chunks[i];
            for (int k = 8; k >= 0; k--) { piece[k] = (char)('0' + c % 10); c /= 10; }
            buf_add(out, piece, 9);
        }
    }
    free(t);
    free(chunks);
    return ok ? 0 : -1;
}
static Value int_from_text(VM *vm, const char *s, size_t n, int neg) { /* n ASCII digits */
    while (n > 1 && *s == '0') { s++; n--; }
    if (n <= 18) {
        int64_t v = 0;
        for (size_t i = 0; i < n; i++) v = v * 10 + (s[i] - '0');
        return v_int(neg ? -v : v);
    }
    Big b;
    big_alloc(&b, n / 9 + 2);
    b.n = 0;
    size_t i = 0;
    while (i < n) {
        size_t take = i == 0 && n % 9 ? n % 9 : 9; /* first chunk short, then 9 digits */
        uint64_t mul = 1, add = 0;
        for (size_t k = 0; k < take; k++) { mul *= 10; add = add * 10 + (uint64_t)(s[i + k] - '0'); }
        uint64_t c = add;
        for (size_t k = 0; k < b.n; k++) {
            c += (uint64_t)b.d[k] * mul;
            b.d[k] = (limb)c;
            c >>= 32;
        }
        if (c) b.d[b.n++] = (limb)c;
        i += take;
    }
    b.neg = neg;
    Value v = int_result(vm, &b);
    big_release(&b);
    return v;
}

/* ------------------------------------------------------------- number text */

static int is_pow2(double x) {
    int e;
    return frexp(fabs(x), &e) == 0.5;
}
/* Python's repr(float): the shortest digit string that reads back as x, in
 * fixed notation when the decimal exponent is in [-4, 16), else d.ddde+XX. */
static void float_repr(double x, char *out /* >= 40 bytes */) {
    if (isnan(x)) { strcpy(out, "nan"); return; }
    if (isinf(x)) { strcpy(out, x > 0 ? "inf" : "-inf"); return; }
    if (x == 0) { strcpy(out, signbit(x) ? "-0.0" : "0.0"); return; }
    char tmp[48], digits[24];
    int nd = 0, exp10 = 0;
    for (int p = 1; p <= 17; p++) {
        snprintf(tmp, sizeof tmp, "%.*e", p - 1, x);
        int ok = strtod(tmp, NULL) == x;
        char *e = strchr(tmp, 'e');
        nd = 0;
        for (char *c = tmp; c < e; c++)
            if (*c >= '0' && *c <= '9') digits[nd++] = *c;
        exp10 = atoi(e + 1);
        if (!ok && is_pow2(x) && p < 17) {
            /* The interval of values that read back as a power of two is
             * narrower below it, so the next p-digit string up may fit. */
            char up[24];
            int k = nd - 1, upexp = exp10;
            memcpy(up, digits, (size_t)nd);
            while (k >= 0 && up[k] == '9') up[k--] = '0';
            if (k < 0) { up[0] = '1'; upexp++; }
            else up[k]++;
            char cand[48];
            int len = snprintf(cand, sizeof cand, "%s%c.", x < 0 ? "-" : "", up[0]);
            memcpy(cand + len, up + 1, (size_t)(nd - 1));
            snprintf(cand + len + nd - 1, sizeof cand - (size_t)(len + nd - 1), "e%d", upexp);
            if (strtod(cand, NULL) == x) {
                memcpy(digits, up, (size_t)nd);
                exp10 = upexp;
                ok = 1;
            }
        }
        if (ok) break;
    }
    while (nd > 1 && digits[nd - 1] == '0') nd--;
    int decpt = exp10 + 1;
    char *o = out;
    if (x < 0) *o++ = '-';
    if (decpt <= -4 || decpt > 16) {
        *o++ = digits[0];
        if (nd > 1) {
            *o++ = '.';
            memcpy(o, digits + 1, (size_t)(nd - 1));
            o += nd - 1;
        }
        sprintf(o, "e%c%02d", exp10 < 0 ? '-' : '+', exp10 < 0 ? -exp10 : exp10);
    } else if (decpt <= 0) {
        *o++ = '0';
        *o++ = '.';
        for (int i = 0; i < -decpt; i++) *o++ = '0';
        memcpy(o, digits, (size_t)nd);
        o[nd] = 0;
    } else if (decpt >= nd) {
        memcpy(o, digits, (size_t)nd);
        o += nd;
        for (int i = nd; i < decpt; i++) *o++ = '0';
        strcpy(o, ".0");
    } else {
        memcpy(o, digits, (size_t)decpt);
        o += decpt;
        *o++ = '.';
        memcpy(o, digits + decpt, (size_t)(nd - decpt));
        o[nd - decpt] = 0;
    }
}
/* minilang's stringify() of a float: an integral value prints as an integer. */
static void float_text(double x, Buf *b) {
    if (isfinite(x) && floor(x) == x) {
        if (x > -9.2e18 && x < 9.2e18) {
            buf_i64(b, (int64_t)x);
        } else {
            Big big;
            big_from_double(x, &big);
            big_text(&big, b); /* at most 309 digits */
            big_release(&big);
        }
        return;
    }
    char t[48];
    float_repr(x, t);
    buf_str(b, t);
}

/* Python's str.isspace() for one code point (used by strip, int() and float()). */
static int py_space(uint32_t c) {
    return (c >= 0x09 && c <= 0x0D) || (c >= 0x1C && c <= 0x20) || c == 0x85 || c == 0xA0 ||
           c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
           c == 0x202F || c == 0x205F || c == 0x3000;
}
static void py_strip(const char *s, size_t n, size_t *start, size_t *end) {
    size_t i = 0, j = n, len;
    while (i < n && py_space(utf8_decode(s + i, &len))) i += len;
    while (j > i) {
        size_t k = j - 1;
        while (k > i && ((uint8_t)s[k] & 0xC0) == 0x80) k--;
        if (!py_space(utf8_decode(s + k, &len))) break;
        j = k;
    }
    *start = i;
    *end = j;
}
static int ieq(const char *s, size_t n, const char *word) {
    if (strlen(word) != n) return 0;
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c != word[i]) return 0;
    }
    return 1;
}
/* Python's float(str) after stripping: [sign] (digits [. digits] | . digits)
 * [e [sign] digits] | inf | infinity | nan, with single underscores allowed
 * between digits. Returns 1 if valid. */
static int parse_py_float(const char *s, size_t n, double *out) {
    char *clean = xrealloc(NULL, n + 1);
    size_t m = 0;
    char prev = 0;
    int ok = 1;
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (c == '_') {
            if (!(prev >= '0' && prev <= '9')) ok = 0;
        } else {
            if (prev == '_' && !(c >= '0' && c <= '9')) ok = 0;
            clean[m++] = c;
        }
        prev = c;
    }
    if (prev == '_') ok = 0;
    clean[m] = 0;
    size_t i = 0;
    int neg = 0;
    if (ok && i < m && (clean[i] == '+' || clean[i] == '-')) neg = clean[i++] == '-';
    if (ok && (ieq(clean + i, m - i, "inf") || ieq(clean + i, m - i, "infinity"))) {
        *out = neg ? -HUGE_VAL : HUGE_VAL;
    } else if (ok && ieq(clean + i, m - i, "nan")) {
        *out = NAN;
    } else if (ok) {
        size_t mant = 0, j = i;
        while (j < m && clean[j] >= '0' && clean[j] <= '9') { j++; mant++; }
        if (j < m && clean[j] == '.') {
            j++;
            while (j < m && clean[j] >= '0' && clean[j] <= '9') { j++; mant++; }
        }
        if (mant == 0) ok = 0;
        if (ok && j < m && (clean[j] == 'e' || clean[j] == 'E')) {
            j++;
            if (j < m && (clean[j] == '+' || clean[j] == '-')) j++;
            size_t ed = 0;
            while (j < m && clean[j] >= '0' && clean[j] <= '9') { j++; ed++; }
            if (ed == 0) ok = 0;
        }
        if (j != m) ok = 0;
        if (ok) *out = strtod(clean, NULL); /* the text is plain decimal now */
    }
    free(clean);
    return ok;
}

/* ----------------------------------------------------- code object loading */
/* Bytecode file format 1.0; the writer and the layout are in
 * ../minilang-vm/minilang_vm.py (dump_program). The loader checks the header,
 * bounds-checks every read, accepts only valid UTF-8, and then verifies each
 * code object before the VM may run it: operands in range, jump targets on
 * instruction boundaries, and a stack-depth walk over every path (like the
 * Python compiler's stack_depth(), plus underflow checks per instruction) so
 * that the frame size taken from the file is never too small. */

enum {
    OP_LOAD_CONST, OP_LOAD_LOCAL, OP_STORE_LOCAL, OP_LOAD_CELL, OP_STORE_CELL, OP_MAKE_CELL,
    OP_CELL_ARG, OP_LOAD_UPVAL, OP_STORE_UPVAL, OP_LOAD_GLOBAL, OP_STORE_GLOBAL,
    OP_DEFINE_GLOBAL, OP_LOAD_CHAIN, OP_STORE_CHAIN, OP_POP, OP_DUP, OP_ADD, OP_SUB, OP_MUL,
    OP_DIV, OP_MOD, OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE, OP_NEG, OP_NOT, OP_INDEX,
    OP_SET_INDEX, OP_BUILD_LIST, OP_MAKE_CLOSURE, OP_CALL, OP_JUMP, OP_JUMP_IF_FALSE,
    OP_JUMP_IF_FALSE_OR_POP, OP_JUMP_IF_TRUE_OR_POP, OP_RETURN, OP_THROW, OP_BREAK_OUT,
    OP_HALT, OP_ADD_CONST, OP_SUB_CONST, OP_LT_CONST, OP_JUMP_IF_NOT_LT, NUM_OPCODES
};
static const char *const OPNAMES[NUM_OPCODES] = {
    "LOAD_CONST", "LOAD_LOCAL", "STORE_LOCAL", "LOAD_CELL", "STORE_CELL", "MAKE_CELL",
    "CELL_ARG", "LOAD_UPVAL", "STORE_UPVAL", "LOAD_GLOBAL", "STORE_GLOBAL", "DEFINE_GLOBAL",
    "LOAD_CHAIN", "STORE_CHAIN", "POP", "DUP", "ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "NE",
    "LT", "LE", "GT", "GE", "NEG", "NOT", "INDEX", "SET_INDEX", "BUILD_LIST", "MAKE_CLOSURE",
    "CALL", "JUMP", "JUMP_IF_FALSE", "JUMP_IF_FALSE_OR_POP", "JUMP_IF_TRUE_OR_POP", "RETURN",
    "THROW", "BREAK_OUT", "HALT", "ADD_CONST", "SUB_CONST", "LT_CONST", "JUMP_IF_NOT_LT"};
/* Values each instruction needs on the operand stack, and its net effect;
 * -100 marks "depends on the operand". */
static const int8_t OP_POPS[NUM_OPCODES] = {
    0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 1, 1, 2, 3, -100, 0, -100, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 2};
static const int8_t OP_EFFECT[NUM_OPCODES] = {
    1, 1, -1, 1, -1, 0, 0, 1, -1, 1, -1, -1, 1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0, 0, -1, -2, -100, 1, -100, 0, -1, -100, -100, -1, -1, 0, -1, 0, 0, 0, -2};

enum { K_NIL, K_FALSE, K_TRUE, K_INT, K_BIGINT, K_FLOAT, K_STR, K_CODE };
#define MLC_MAJOR 1
#define MLC_MINOR 0
#define MAX_CODE_NESTING 200

typedef struct {
    const uint8_t *p, *end;
    char err[160];
} Reader;

static int rd_fail(Reader *r, const char *msg) {
    if (!r->err[0]) snprintf(r->err, sizeof r->err, "%s", msg);
    return 0;
}
static int rd_need(Reader *r, size_t n) {
    if ((size_t)(r->end - r->p) < n) return rd_fail(r, "truncated bytecode file");
    return 1;
}
static uint32_t rd_u32(Reader *r) {
    if (!rd_need(r, 4)) return 0;
    uint32_t v = (uint32_t)r->p[0] | (uint32_t)r->p[1] << 8 | (uint32_t)r->p[2] << 16 |
                 (uint32_t)r->p[3] << 24;
    r->p += 4;
    return v;
}
static uint16_t rd_u16(Reader *r) {
    if (!rd_need(r, 2)) return 0;
    uint16_t v = (uint16_t)(r->p[0] | r->p[1] << 8);
    r->p += 2;
    return v;
}
static uint8_t rd_u8(Reader *r) {
    if (!rd_need(r, 1)) return 0;
    return *r->p++;
}
static uint64_t rd_u64(Reader *r) {
    uint64_t lo = rd_u32(r);
    uint64_t hi = rd_u32(r);
    return hi << 32 | lo;
}
/* A count that must leave at least `each` bytes per item in the file, so a
 * corrupt count cannot make the loader allocate gigabytes. */
static uint32_t rd_count(Reader *r, size_t each) {
    uint32_t n = rd_u32(r);
    if (r->err[0]) return 0;
    if (each && (size_t)(r->end - r->p) / each < n) {
        rd_fail(r, "count larger than the file");
        return 0;
    }
    return n;
}
static ObjString *rd_str(VM *vm, Reader *r) {
    uint32_t n = rd_count(r, 1);
    if (r->err[0] || !rd_need(r, n)) return NULL;
    if (!utf8_valid(r->p, n)) {
        rd_fail(r, "string is not valid UTF-8");
        return NULL;
    }
    ObjString *s = intern(vm, (const char *)r->p, n);
    r->p += n;
    return s;
}

static void *heap_array(VM *vm, size_t n, size_t each) {
    void *p = heap_realloc(vm, NULL, 0, n * each);
    if (p) memset(p, 0, n * each);
    return p;
}

static int verify_code(ObjCode *co, Reader *r);

static ObjCode *load_code(VM *vm, Reader *r, int depth) {
    if (depth > MAX_CODE_NESTING) {
        rd_fail(r, "functions nested too deeply");
        return NULL;
    }
    /* Every object made here is reachable from co, and co from the temp
     * roots, before the next allocation; arrays are zeroed before their
     * counts are set, so a collection in the middle traces only valid data. */
    ObjCode *co = (ObjCode *)new_obj(vm, O_CODE, sizeof(ObjCode));
    push_temp(vm, v_obj(co));
    uint32_t n;
    co->name = rd_str(vm, r);
    n = rd_count(r, 4);
    co->params = heap_array(vm, n, sizeof(ObjString *));
    co->nparams = n;
    for (uint32_t i = 0; i < n && !r->err[0]; i++) co->params[i] = rd_str(vm, r);
    co->nslots = rd_u32(r);
    co->max_stack = rd_u32(r);
    n = rd_count(r, 5);
    if (n > (UINT32_MAX - 1) / 2) rd_fail(r, "code too long");
    if (!r->err[0]) {
        co->code = heap_array(vm, (size_t)n * 2, sizeof(int32_t));
        co->ncode = n * 2;
        for (uint32_t i = 0; i < n; i++) {
            co->code[2 * i] = rd_u8(r);
            uint32_t arg = rd_u32(r);
            if (arg > INT32_MAX) rd_fail(r, "operand out of range");
            co->code[2 * i + 1] = (int32_t)arg;
        }
    }
    n = rd_count(r, 1);
    co->consts = heap_array(vm, n, sizeof(Value));
    for (uint32_t i = 0; i < n; i++) co->consts[i] = v_nil();
    co->nconsts = n;
    for (uint32_t i = 0; i < n && !r->err[0]; i++) {
        uint8_t tag = rd_u8(r);
        switch (tag) {
        case K_NIL: co->consts[i] = v_nil(); break;
        case K_FALSE: co->consts[i] = v_bool(0); break;
        case K_TRUE: co->consts[i] = v_bool(1); break;
        case K_INT: co->consts[i] = v_int((int64_t)rd_u64(r)); break;
        case K_FLOAT: {
            uint64_t bits = rd_u64(r);
            double d;
            memcpy(&d, &bits, sizeof d);
            co->consts[i] = v_float(d);
            break;
        }
        case K_STR: {
            ObjString *s = rd_str(vm, r);
            if (s) co->consts[i] = v_obj(s);
            break;
        }
        case K_BIGINT: {
            ObjString *s = rd_str(vm, r);
            if (!s) break;
            const char *t = s->s;
            size_t len = s->len;
            int neg = len && t[0] == '-';
            if (neg) { t++; len--; }
            int digits = len > 0;
            for (size_t k = 0; k < len; k++) digits &= t[k] >= '0' && t[k] <= '9';
            if (!digits) { rd_fail(r, "bad integer constant"); break; }
            push_temp(vm, v_obj(s));
            co->consts[i] = int_from_text(vm, t, len, neg);
            pop_temp(vm);
            break;
        }
        case K_CODE: {
            ObjCode *inner = load_code(vm, r, depth + 1);
            if (inner) {
                co->consts[i] = v_obj(inner);
                pop_temp(vm); /* inner is now reachable from co */
            }
            break;
        }
        default:
            rd_fail(r, "unknown constant tag");
        }
    }
    n = rd_count(r, 8);
    co->lines = heap_array(vm, (size_t)n * 2, sizeof(int32_t));
    co->nlines = n;
    for (uint32_t i = 0; i < 2 * n && !r->err[0]; i++) co->lines[i] = (int32_t)rd_u32(r);
    n = rd_count(r, 12);
    co->handlers = heap_array(vm, n, sizeof(Handler));
    co->nhandlers = n;
    for (uint32_t i = 0; i < n && !r->err[0]; i++) {
        co->handlers[i].start = (int32_t)rd_u32(r);
        co->handlers[i].end = (int32_t)rd_u32(r);
        co->handlers[i].target = (int32_t)rd_u32(r);
    }
    n = rd_count(r, 8);
    co->loops = heap_array(vm, n, sizeof(Loop));
    co->nloops = n;
    for (uint32_t i = 0; i < n && !r->err[0]; i++) {
        co->loops[i].start = (int32_t)rd_u32(r);
        co->loops[i].end = (int32_t)rd_u32(r);
    }
    n = rd_count(r, 13);
    co->chains = heap_array(vm, n, sizeof(Chain));
    co->nchains = n;
    for (uint32_t i = 0; i < n && !r->err[0]; i++) {
        Chain *ch = &co->chains[i];
        ch->name = rd_str(vm, r);
        uint32_t m = rd_count(r, 4);
        ch->maybe = heap_array(vm, m, sizeof(uint32_t));
        ch->nmaybe = m;
        for (uint32_t k = 0; k < m; k++) ch->maybe[k] = rd_u32(r);
        ch->is_upval = rd_u8(r);
        ch->final = rd_u32(r);
    }
    n = rd_count(r, 9);
    co->ups = heap_array(vm, n, sizeof(UpSpec));
    co->nup = n;
    for (uint32_t i = 0; i < n && !r->err[0]; i++) {
        co->ups[i].local = rd_u8(r);
        co->ups[i].index = rd_u32(r);
        co->ups[i].name = rd_str(vm, r);
    }
    if (!r->err[0] && co->nslots > (size_t)(r->end - r->p) / 4) rd_fail(r, "truncated bytecode file (slot names)");
    if (!r->err[0]) {
        co->slot_names = heap_array(vm, co->nslots, sizeof(ObjString *));
        for (uint32_t i = 0; i < co->nslots && !r->err[0]; i++) co->slot_names[i] = rd_str(vm, r);
    }
    if (!r->err[0] && !verify_code(co, r)) return NULL;
    if (r->err[0]) return NULL;
    return co; /* still on the temp stack; the caller pops it */
}

static int verify_code(ObjCode *co, Reader *r) {
    char *msg = r->err;
    size_t cap = sizeof r->err;
    const char *name = co->name ? co->name->s : "?";
#define BAD(...) (snprintf(msg, cap, __VA_ARGS__), 0)
    if (co->nparams > co->nslots) return BAD("%s: more parameters than slots", name);
    if (co->ncode == 0) return BAD("%s: empty code", name);
    if (co->nlines == 0 || co->lines[0] != 0) return BAD("%s: line table must start at offset 0", name);
    for (uint32_t i = 1; i < co->nlines; i++)
        if (co->lines[2 * i] <= co->lines[2 * i - 2] || co->lines[2 * i] >= (int32_t)co->ncode)
            return BAD("%s: line table out of order", name);
    for (uint32_t i = 0; i < co->nconsts; i++) {
        if (!is_kind(co->consts[i], O_CODE)) continue;
        ObjCode *inner = AS_CODE(co->consts[i]);
        for (uint32_t k = 0; k < inner->nup; k++) {
            UpSpec *u = &inner->ups[k];
            if (u->local ? u->index >= co->nslots : u->index >= co->nup)
                return BAD("%s: upvalue %u of %s out of range", name, (unsigned)k, inner->name->s);
        }
    }
    for (uint32_t i = 0; i < co->nchains; i++) {
        Chain *ch = &co->chains[i];
        for (uint32_t k = 0; k < ch->nmaybe; k++)
            if (ch->maybe[k] >= co->nup) return BAD("%s: chain upvalue out of range", name);
        if (ch->is_upval > 1 || (ch->is_upval && ch->final >= co->nup))
            return BAD("%s: chain target out of range", name);
    }
    for (uint32_t pc = 0; pc < co->ncode; pc += 2) {
        int32_t op = co->code[pc], arg = co->code[pc + 1];
        uint32_t ua = (uint32_t)arg;
        if (op < 0 || op >= NUM_OPCODES) return BAD("%s: unknown opcode %d at %u", name, (int)op, (unsigned)pc);
        switch (op) {
        case OP_LOAD_CONST:
            if (ua >= co->nconsts) return BAD("%s: constant index out of range at %u", name, (unsigned)pc);
            break;
        case OP_ADD_CONST: case OP_SUB_CONST: case OP_LT_CONST:
            if (ua >= co->nconsts || !is_number(co->consts[ua]))
                return BAD("%s: operand at %u is not a numeric constant", name, (unsigned)pc);
            break;
        case OP_LOAD_GLOBAL: case OP_STORE_GLOBAL: case OP_DEFINE_GLOBAL:
            if (ua >= co->nconsts || !IS_STR(co->consts[ua]))
                return BAD("%s: global name at %u is not a string constant", name, (unsigned)pc);
            break;
        case OP_MAKE_CLOSURE:
            if (ua >= co->nconsts || !is_kind(co->consts[ua], O_CODE))
                return BAD("%s: closure operand at %u is not a code constant", name, (unsigned)pc);
            break;
        case OP_LOAD_LOCAL: case OP_STORE_LOCAL: case OP_LOAD_CELL: case OP_STORE_CELL:
        case OP_MAKE_CELL: case OP_CELL_ARG:
            if (ua >= co->nslots) return BAD("%s: slot out of range at %u", name, (unsigned)pc);
            break;
        case OP_LOAD_UPVAL: case OP_STORE_UPVAL:
            if (ua >= co->nup) return BAD("%s: upvalue out of range at %u", name, (unsigned)pc);
            break;
        case OP_LOAD_CHAIN: case OP_STORE_CHAIN:
            if (ua >= co->nchains) return BAD("%s: chain out of range at %u", name, (unsigned)pc);
            break;
        case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_JUMP_IF_FALSE_OR_POP:
        case OP_JUMP_IF_TRUE_OR_POP: case OP_JUMP_IF_NOT_LT:
            if (ua >= co->ncode || ua % 2) return BAD("%s: bad jump target at %u", name, (unsigned)pc);
            break;
        default:
            break;
        }
    }
    for (uint32_t i = 0; i < co->nhandlers; i++) {
        Handler *h = &co->handlers[i];
        if (h->start < 0 || h->start > h->end || h->end > (int32_t)co->ncode || h->target < 0 ||
            h->target >= (int32_t)co->ncode || h->target % 2)
            return BAD("%s: bad exception table entry", name);
    }
    for (uint32_t i = 0; i < co->nloops; i++) {
        Loop *l = &co->loops[i];
        if (l->start < 0 || l->start > l->end || l->end >= (int32_t)co->ncode || l->end % 2)
            return BAD("%s: bad loop table entry", name);
    }
    /* Stack depth over every path. Entry points: the start (depth 0), each
     * exception handler (the VM pushes the thrown value: depth 1) and each
     * loop exit (BREAK_OUT empties the operand stack: depth 0). */
    uint32_t ninstr = co->ncode / 2;
    int64_t *seen = xrealloc(NULL, ninstr * sizeof(int64_t));
    for (uint32_t i = 0; i < ninstr; i++) seen[i] = -1;
    size_t nwork = 0, capw = 16 + co->nhandlers + co->nloops + ninstr;
    int64_t *work = xrealloc(NULL, capw * 2 * sizeof(int64_t));
#define PUSH_WORK(pc_, d_) do { if (nwork == capw) { capw *= 2; work = xrealloc(work, capw * 2 * sizeof(int64_t)); } \
        work[2 * nwork] = (pc_); work[2 * nwork + 1] = (d_); nwork++; } while (0)
    PUSH_WORK(0, 0);
    for (uint32_t i = 0; i < co->nhandlers; i++) PUSH_WORK(co->handlers[i].target, 1);
    for (uint32_t i = 0; i < co->nloops; i++) PUSH_WORK(co->loops[i].end, 0);
    int64_t deepest = 0;
    int ok = 1;
    while (ok && nwork) {
        nwork--;
        int64_t pc = work[2 * nwork], depth = work[2 * nwork + 1];
        for (;;) {
            if (pc < 0 || pc >= (int64_t)co->ncode) { ok = BAD("%s: execution runs off the end", name); break; }
            if (seen[pc / 2] >= 0) {
                if (seen[pc / 2] != depth) ok = BAD("%s: stack depth %lld vs %lld at offset %lld", name,
                                                    (long long)seen[pc / 2], (long long)depth, (long long)pc);
                break;
            }
            seen[pc / 2] = depth;
            int32_t op = co->code[pc], arg = co->code[pc + 1];
            int64_t pops = OP_POPS[op] == -100 ? (op == OP_CALL ? (int64_t)arg + 1 : arg) : OP_POPS[op];
            if (depth < pops) { ok = BAD("%s: stack underflow at offset %lld", name, (long long)pc); break; }
            if (op == OP_CALL) depth -= arg;
            else if (op == OP_BUILD_LIST) depth += 1 - (int64_t)arg;
            else if (op == OP_JUMP_IF_FALSE_OR_POP || op == OP_JUMP_IF_TRUE_OR_POP) {
                PUSH_WORK(arg, depth);
                depth -= 1;
            } else depth += OP_EFFECT[op];
            if (depth > deepest) deepest = depth;
            if (op == OP_JUMP_IF_FALSE || op == OP_JUMP_IF_NOT_LT) PUSH_WORK(arg, depth);
            if (op == OP_JUMP) pc = arg;
            else if (op == OP_RETURN || op == OP_THROW || op == OP_BREAK_OUT || op == OP_HALT) break;
            else pc += 2;
        }
    }
    free(seen);
    free(work);
#undef PUSH_WORK
    if (ok && deepest > (int64_t)co->max_stack)
        ok = BAD("%s: max_stack %u is smaller than the computed %lld", name, (unsigned)co->max_stack,
                 (long long)deepest);
    return ok;
#undef BAD
}

/* Load a whole file; on success the script is on the temp root stack. */
static ObjCode *load_program(VM *vm, const uint8_t *data, size_t n, char *err, size_t errcap) {
    Reader r;
    r.p = data;
    r.end = data + n;
    r.err[0] = 0;
    int temps = vm->ntemps;
    ObjCode *script = NULL;
    if (n < 4 || memcmp(data, "MLBC", 4) != 0) {
        rd_fail(&r, "not a minilang bytecode file");
    } else {
        r.p += 4;
        unsigned major = rd_u16(&r), minor = rd_u16(&r), nops = rd_u16(&r);
        rd_u16(&r); /* flags: informational */
        if (!r.err[0] && (major != MLC_MAJOR || minor > MLC_MINOR))
            snprintf(r.err, sizeof r.err, "unsupported bytecode version %u.%u (this VM reads %d.%d)", major,
                     minor, MLC_MAJOR, MLC_MINOR);
        else if (!r.err[0] && nops != NUM_OPCODES)
            snprintf(r.err, sizeof r.err, "bytecode has %u opcodes, this VM has %d", nops, NUM_OPCODES);
        if (!r.err[0]) script = load_code(vm, &r, 0);
        if (script && (script->nparams || script->nup)) rd_fail(&r, "the script takes no parameters or upvalues");
        if (script && r.p != r.end) rd_fail(&r, "trailing bytes after the script");
    }
    if (r.err[0]) {
        vm->ntemps = temps; /* partial objects become garbage */
        snprintf(err, errcap, "%s", r.err);
        return NULL;
    }
    return script;
}

/* -------------------------------------------------------------- formatting */

static const char *const BUILTIN_NAMES[] = {"print", "len", "push", "pop", "str", "num", "type", "clock"};
static const int BUILTIN_ARITY[] = {-1, 1, 2, 1, 1, 1, 1, 0};
#define NUM_BUILTINS 8

static void raise_recursion(VM *vm);
static int host_error(VM *vm, const char *type, const char *msg);
static const char INT_STR_LIMIT[] =
    "Exceeds the limit (4300 digits) for integer string conversion; use sys.set_int_max_str_digits() to "
    "increase the limit";

/* minilang's stringify(). Returns -1 with a host error set (integer too long
 * for text, or nesting beyond MAX_NESTING, which the tree-walker reports as a
 * RecursionError from the Python stack). */
static int stringify(VM *vm, Value v, Buf *b, int depth) {
    switch (v.k) {
    case V_NIL: buf_str(b, "nil"); return 0;
    case V_BOOL: buf_str(b, v.as.b ? "true" : "false"); return 0;
    case V_INT: buf_i64(b, v.as.i); return 0;
    case V_FLOAT: float_text(v.as.f, b); return 0;
    case V_BUILTIN: buf_printf(b, "<builtin %s>", BUILTIN_NAMES[v.as.i]); return 0;
    case V_UNDEF: buf_str(b, "<undefined>"); return 0;
    case V_OBJ: break;
    }
    switch ((OKind)v.as.o->kind) {
    case O_STRING: buf_add(b, AS_STR(v)->s, AS_STR(v)->len); return 0;
    case O_BIGINT:
        if (int_to_text(v, b)) return host_error(vm, "ValueError", INT_STR_LIMIT);
        return 0;
    case O_LIST: {
        if (depth >= MAX_NESTING) { raise_recursion(vm); return -1; }
        ObjList *l = AS_LIST(v);
        buf_ch(b, '[');
        for (size_t i = 0; i < l->n; i++) {
            if (i) buf_str(b, ", ");
            if (stringify(vm, l->items[i], b, depth + 1)) return -1;
        }
        buf_ch(b, ']');
        return 0;
    }
    case O_CLOSURE: buf_printf(b, "<fn %s>", AS_CLOSURE(v)->code->name->s); return 0;
    case O_CODE: buf_printf(b, "<code %s>", AS_CODE(v)->name->s); return 0;
    case O_CELL: buf_str(b, "<cell>"); return 0;
    case O_HOST: buf_printf(b, "<builtin %s>", ((ObjHost *)v.as.o)->name->s); return 0;
    }
    return 0;
}

/* Python's repr() of a str. Non-ASCII characters are kept unless they are
 * among the non-printable ranges below; Python consults the Unicode database,
 * so unassigned code points may differ (see README). */
static int unicode_printable(uint32_t c) {
    if (c < 0x20 || c == 0x7F) return 0;
    if (c < 0x7F) return 1;
    if (c <= 0xA0 || c == 0xAD || c == 0x061C || c == 0x1680 || c == 0x180E) return 0;
    if ((c >= 0x2000 && c <= 0x200F) || (c >= 0x2028 && c <= 0x202F) || (c >= 0x205F && c <= 0x206F)) return 0;
    if (c == 0x3000 || c == 0xFEFF || (c >= 0xD800 && c <= 0xF8FF) || (c >= 0xFFF9 && c <= 0xFFFB)) return 0;
    if (c == 0xFFFE || c == 0xFFFF || c >= 0xF0000 || (c >= 0xE0000 && c <= 0xE0FFF)) return 0;
    return 1;
}
static void repr_string(const ObjString *s, Buf *b) {
    int single = memchr(s->s, '\'', s->len) != NULL, dbl = memchr(s->s, '"', s->len) != NULL;
    char quote = single && !dbl ? '"' : '\'';
    buf_ch(b, quote);
    for (size_t i = 0; i < s->len;) {
        size_t len;
        uint32_t c = utf8_decode(s->s + i, &len);
        if (c == (uint32_t)quote || c == '\\') { buf_ch(b, '\\'); buf_ch(b, (char)c); }
        else if (c == '\t') buf_str(b, "\\t");
        else if (c == '\n') buf_str(b, "\\n");
        else if (c == '\r') buf_str(b, "\\r");
        else if (unicode_printable(c)) buf_add(b, s->s + i, len);
        else if (c < 0x100) buf_printf(b, "\\x%02x", (unsigned)c);
        else if (c < 0x10000) buf_printf(b, "\\u%04x", (unsigned)c);
        else buf_printf(b, "\\U%08x", (unsigned)c);
        i += len;
    }
    buf_ch(b, quote);
}
/* Python's repr() of a minilang value, used by num()'s error message and by
 * the disassembler. A list inside itself prints as [...], as in Python. */
typedef struct ReprStack {
    const ObjList *list;
    const struct ReprStack *up;
} ReprStack;
static int repr_value(VM *vm, Value v, Buf *b, const ReprStack *active, int depth) {
    switch (v.k) {
    case V_NIL: buf_str(b, "None"); return 0;
    case V_BOOL: buf_str(b, v.as.b ? "True" : "False"); return 0;
    case V_INT: buf_i64(b, v.as.i); return 0;
    case V_FLOAT: {
        char t[48];
        float_repr(v.as.f, t);
        buf_str(b, t);
        return 0;
    }
    default: break;
    }
    if (IS_STR(v)) { repr_string(AS_STR(v), b); return 0; }
    if (IS_LIST(v)) {
        for (const ReprStack *a = active; a; a = a->up)
            if (a->list == AS_LIST(v)) { buf_str(b, "[...]"); return 0; }
        if (depth >= MAX_NESTING) { raise_recursion(vm); return -1; }
        ReprStack here = {AS_LIST(v), active};
        ObjList *l = AS_LIST(v);
        buf_ch(b, '[');
        for (size_t i = 0; i < l->n; i++) {
            if (i) buf_str(b, ", ");
            if (repr_value(vm, l->items[i], b, &here, depth + 1)) return -1;
        }
        buf_ch(b, ']');
        return 0;
    }
    return stringify(vm, v, b, depth); /* functions, builtins, big integers */
}

static const char *type_name(Value v) {
    switch (v.k) {
    case V_NIL: return "nil";
    case V_BOOL: return "bool";
    case V_INT: case V_FLOAT: return "number";
    case V_BUILTIN: return "function";
    case V_UNDEF: return "_Undefined";
    case V_OBJ: break;
    }
    switch ((OKind)v.as.o->kind) {
    case O_STRING: return "string";
    case O_BIGINT: return "number";
    case O_LIST: return "list";
    case O_CLOSURE: return "function";
    case O_CODE: return "Code";
    case O_CELL: return "cell"; /* never a minilang value */
    case O_HOST: return "function";
    }
    return "?";
}

/* ----------------------------------------------------------- disassembler */
/* Same text as minilang_vm.disassemble(), which makes the loader testable:
 * a file that disassembles identically was read back field by field. */

static int line_at(const ObjCode *co, size_t offset) {
    uint32_t lo = 0, hi = co->nlines; /* last run whose start <= offset */
    while (hi - lo > 1) {
        uint32_t mid = (lo + hi) / 2;
        if ((size_t)co->lines[2 * mid] <= offset) lo = mid;
        else hi = mid;
    }
    return co->lines[2 * lo + 1];
}

#ifndef MLVM_LIBRARY /* the disassembler is part of the command-line tool */
static void disassemble(VM *vm, ObjCode *co, Buf *b) {
    buf_printf(b, "== %s  params=", co->name->s);
    for (uint32_t i = 0; i < co->nparams; i++) buf_printf(b, "%s%s", i ? "," : "", co->params[i]->s);
    if (!co->nparams) buf_ch(b, '-');
    buf_printf(b, " slots=%u upvalues=%u max_stack=%u ==\nconstants: ", (unsigned)co->nslots,
               (unsigned)co->nup, (unsigned)co->max_stack);
    for (uint32_t i = 0; i < co->nconsts; i++) {
        buf_printf(b, "%s%u=", i ? ", " : "", (unsigned)i);
        repr_value(vm, co->consts[i], b, NULL, 0);
    }
    if (!co->nconsts) buf_ch(b, '-');
    if (co->nup) {
        buf_str(b, "\nupvalues: ");
        for (uint32_t i = 0; i < co->nup; i++)
            buf_printf(b, "%s%u=%s(%s %u)", i ? ", " : "", (unsigned)i, co->ups[i].name->s,
                       co->ups[i].local ? "slot" : "upvalue", (unsigned)co->ups[i].index);
    }
    int last = -1;
    for (uint32_t pc = 0; pc < co->ncode; pc += 2) {
        int32_t op = co->code[pc], arg = co->code[pc + 1];
        int line = line_at(co, pc);
        char shown[16];
        if (line != last) snprintf(shown, sizeof shown, "%4d", line);
        else strcpy(shown, "   |");
        last = line;
        Buf operand = {0};
        buf_str(&operand, "");
        switch (op) {
        case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_JUMP_IF_FALSE_OR_POP: case OP_JUMP_IF_TRUE_OR_POP:
        case OP_JUMP_IF_NOT_LT:
            buf_printf(&operand, "-> %d", (int)arg);
            break;
        case OP_LOAD_CONST: case OP_LOAD_GLOBAL: case OP_STORE_GLOBAL: case OP_DEFINE_GLOBAL:
        case OP_ADD_CONST: case OP_SUB_CONST: case OP_LT_CONST: case OP_MAKE_CLOSURE:
            buf_printf(&operand, "%d (", (int)arg);
            repr_value(vm, co->consts[arg], &operand, NULL, 0);
            buf_ch(&operand, ')');
            break;
        case OP_LOAD_LOCAL: case OP_STORE_LOCAL: case OP_LOAD_CELL: case OP_STORE_CELL: case OP_MAKE_CELL:
        case OP_CELL_ARG:
            buf_printf(&operand, "%d (%s)", (int)arg, co->slot_names[arg]->s);
            break;
        case OP_LOAD_UPVAL: case OP_STORE_UPVAL:
            buf_printf(&operand, "%d (%s)", (int)arg, co->ups[arg].name->s);
            break;
        case OP_LOAD_CHAIN: case OP_STORE_CHAIN: {
            Chain *ch = &co->chains[arg];
            buf_printf(&operand, "%d (%s: ", (int)arg, ch->name->s);
            for (uint32_t k = 0; k < ch->nmaybe; k++) buf_printf(&operand, "upvalue %u if defined, then ", (unsigned)ch->maybe[k]);
            if (ch->is_upval) buf_printf(&operand, "upvalue %u)", (unsigned)ch->final);
            else buf_str(&operand, "global)");
            break;
        }
        case OP_CALL: case OP_BUILD_LIST:
            buf_printf(&operand, "%d", (int)arg);
            break;
        default:
            break;
        }
        buf_printf(b, "\n%5u %s  %-20s %s", (unsigned)pc, shown, OPNAMES[op], operand.p);
        buf_free(&operand);
    }
    for (uint32_t i = 0; i < co->nhandlers; i++)
        buf_printf(b, "\nexception: %d-%d -> %d", (int)co->handlers[i].start, (int)co->handlers[i].end,
                   (int)co->handlers[i].target);
    for (uint32_t i = 0; i < co->nloops; i++)
        buf_printf(b, "\nloop: %d-%d", (int)co->loops[i].start, (int)co->loops[i].end);
    buf_str(b, "\nline table:");
    for (uint32_t i = 0; i < co->nlines; i++)
        buf_printf(b, " %d:%d", (int)co->lines[2 * i], (int)co->lines[2 * i + 1]);
    for (uint32_t i = 0; i < co->nconsts; i++) {
        if (!is_kind(co->consts[i], O_CODE)) continue;
        buf_str(b, "\n\n");
        disassemble(vm, AS_CODE(co->consts[i]), b);
    }
}
#endif

/* ------------------------------------------------------------------ errors */
/* A raised error is recorded in the VM and the function returns -1; the
 * interpreter loop then unwinds. Kinds mirror how the tree-walker ends:
 *   E_RUNTIME    minilang RuntimeErr, catchable by try/catch
 *   E_RECURSION  Python RecursionError; becomes "stack overflow (host recursion
 *                limit)" at the innermost minilang call, as in Interpreter.call()
 *   E_HOST       another Python exception (OverflowError, ValueError) that
 *                no minilang code can catch
 *   E_RETURN, E_BREAK  ReturnSignal / BreakSignal escaping the script */

static int rt_error(VM *vm, const char *fmt, ...) {
    va_list ap;
    buf_clear(&vm->errmsg);
    va_start(ap, fmt);
    buf_vprintf(&vm->errmsg, fmt, ap);
    va_end(ap);
    vm->err = E_RUNTIME;
    vm->errline = -1;
    vm->has_errval = 0;
    return -1;
}
static int host_error(VM *vm, const char *type, const char *msg) {
    buf_clear(&vm->errmsg);
    buf_str(&vm->errmsg, msg);
    vm->err = E_HOST;
    vm->host_type = type;
    vm->has_errval = 0;
    return -1;
}
static void raise_recursion(VM *vm) {
    buf_clear(&vm->errmsg);
    buf_str(&vm->errmsg, "maximum recursion depth exceeded");
    vm->err = E_RECURSION;
    vm->host_type = "RecursionError";
    vm->has_errval = 0;
}
static int overflow_to_float(VM *vm) { return host_error(vm, "OverflowError", "int too large to convert to float"); }

/* -------------------------------------------------------------- operators */
/* The generic paths, used when the interpreter's int and float fast paths do
 * not apply. They follow minilang.Interpreter.binary() rule by rule. */

static ObjList *new_list(VM *vm, size_t cap) {
    ObjList *l = (ObjList *)new_obj(vm, O_LIST, sizeof(ObjList));
    if (cap) {
        push_temp(vm, v_obj(l)); /* the second allocation may collect */
        l->items = heap_realloc(vm, NULL, 0, cap * sizeof(Value));
        l->cap = cap;
        pop_temp(vm);
    }
    return l;
}
static void list_push(VM *vm, ObjList *l, Value v) { /* l and v must be rooted */
    if (l->n == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 4;
        l->items = heap_realloc(vm, l->items, l->cap * sizeof(Value), cap * sizeof(Value));
        l->cap = cap;
    }
    l->items[l->n++] = v;
}

static int to_double(VM *vm, Value v, double *out) {
    if (v.k == V_FLOAT) { *out = v.as.f; return 0; }
    if (int_to_double(v, out)) return overflow_to_float(vm);
    return 0;
}
/* -1, 0, 1, or 2 when unordered (NaN) */
static int num_cmp(Value a, Value b) {
    if (a.k == V_FLOAT && b.k == V_FLOAT) {
        if (isnan(a.as.f) || isnan(b.as.f)) return 2;
        return a.as.f < b.as.f ? -1 : a.as.f > b.as.f;
    }
    if (a.k == V_FLOAT) {
        int c = int_cmp_double(b, a.as.f);
        return c == 2 ? 2 : -c;
    }
    if (b.k == V_FLOAT) return int_cmp_double(a, b.as.f);
    return int_cmp(a, b);
}
static int values_equal(Value a, Value b) {
    if (a.k == V_BOOL || b.k == V_BOOL) return a.k == b.k && a.as.b == b.as.b;
    if (is_number(a) && is_number(b)) return num_cmp(a, b) == 0;
    if (IS_STR(a) && IS_STR(b)) return str_equal(AS_STR(a), AS_STR(b));
    if (a.k != b.k) return 0;
    switch (a.k) {
    case V_NIL: case V_UNDEF: return 1;
    case V_BUILTIN: return a.as.i == b.as.i;
    case V_OBJ: return a.as.o == b.as.o; /* lists and functions compare by identity */
    default: return 0;
    }
}
static int is_zero(Value v) {
    return (v.k == V_INT && v.as.i == 0) || (v.k == V_FLOAT && v.as.f == 0);
}
static double py_fmod(double x, double y) { /* CPython's float_rem */
    double mod = fmod(x, y);
    if (mod) {
        if ((y < 0) != (mod < 0)) mod += y;
    } else {
        mod = copysign(0.0, y);
    }
    return mod;
}

static const char *const OP_TEXT[] = {[OP_ADD] = "+", [OP_SUB] = "-", [OP_MUL] = "*", [OP_DIV] = "/",
                                      [OP_MOD] = "%", [OP_EQ] = "==", [OP_NE] = "!=", [OP_LT] = "<",
                                      [OP_LE] = "<=", [OP_GT] = ">", [OP_GE] = ">="};

/* a and b must stay rooted (the interpreter leaves them on the stack). */
static int binary_op(VM *vm, int op, Value a, Value b, Value *out) {
    if (op == OP_EQ || op == OP_NE) {
        int eq = values_equal(a, b);
        *out = v_bool(op == OP_EQ ? eq : !eq);
        return 0;
    }
    int nums = is_number(a) && is_number(b);
    if (op == OP_ADD && !nums) {
        if (IS_STR(a) && IS_STR(b)) {
            ObjString *x = AS_STR(a), *y = AS_STR(b);
            ObjString *s = alloc_string(vm, x->len + y->len);
            memcpy(s->s, x->s, x->len);
            memcpy(s->s + x->len, y->s, y->len);
            s->nchars = x->nchars + y->nchars;
            s->ascii = x->ascii && y->ascii;
            *out = v_obj(s);
            return 0;
        }
        if (IS_LIST(a) && IS_LIST(b)) {
            ObjList *x = AS_LIST(a), *y = AS_LIST(b);
            ObjList *l = new_list(vm, x->n + y->n);
            for (size_t i = 0; i < x->n; i++) l->items[l->n++] = x->items[i];
            for (size_t i = 0; i < y->n; i++) l->items[l->n++] = y->items[i];
            *out = v_obj(l);
            return 0;
        }
        return rt_error(vm, "'+' expects two numbers, two strings or two lists, got %s and %s", type_name(a),
                        type_name(b));
    }
    if (op >= OP_LT && op <= OP_GE) {
        int c;
        if (nums) c = num_cmp(a, b);
        else if (IS_STR(a) && IS_STR(b)) c = str_compare(AS_STR(a), AS_STR(b));
        else
            return rt_error(vm, "'%s' expects two numbers or two strings, got %s and %s", OP_TEXT[op],
                            type_name(a), type_name(b));
        int r = c == 2 ? 0 : op == OP_LT ? c < 0 : op == OP_LE ? c <= 0 : op == OP_GT ? c > 0 : c >= 0;
        *out = v_bool(r);
        return 0;
    }
    if (!nums) return rt_error(vm, "'%s' expects two numbers, got %s and %s", OP_TEXT[op], type_name(a), type_name(b));
    int ints = is_intval(a) && is_intval(b);
    if ((op == OP_DIV || op == OP_MOD) && is_zero(b)) return rt_error(vm, "division by zero");
    if (ints) {
        switch (op) {
        case OP_ADD: *out = int_addsub(vm, a, b, 0); return 0;
        case OP_SUB: *out = int_addsub(vm, a, b, 1); return 0;
        case OP_MUL: *out = int_mul(vm, a, b); return 0;
        case OP_MOD: *out = int_mod(vm, a, b); return 0;
        case OP_DIV: {
            double q;
            if (int_true_div(a, b, &q)) return host_error(vm, "OverflowError", "integer division result too large for a float");
            *out = floor(q) == q ? int_from_double(vm, q) : v_float(q); /* exact: an int, as in minilang */
            return 0;
        }
        }
    }
    double x, y;
    if (to_double(vm, a, &x) || to_double(vm, b, &y)) return -1;
    switch (op) {
    case OP_ADD: *out = v_float(x + y); break;
    case OP_SUB: *out = v_float(x - y); break;
    case OP_MUL: *out = v_float(x * y); break;
    case OP_DIV: *out = v_float(x / y); break;
    default: *out = v_float(py_fmod(x, y)); break;
    }
    return 0;
}

/* Interpreter.check_index(): returns the position, or -1 with an error set. */
static int64_t check_index(VM *vm, Value index, size_t len) {
    if (!is_number(index)) return rt_error(vm, "index must be an integer, got %s", type_name(index));
    Value iv = index;
    if (index.k == V_FLOAT) {
        double f = index.as.f;
        if (isnan(f)) return host_error(vm, "ValueError", "cannot convert float NaN to integer");
        if (isinf(f)) return host_error(vm, "OverflowError", "cannot convert float infinity to integer");
        if (floor(f) != f) return rt_error(vm, "index must be an integer, got number");
        if (f > -9.2e18 && f < 9.2e18) iv = v_int((int64_t)f);
        else iv = int_from_double(vm, f); /* only for the message below; not stored */
    }
    if (iv.k == V_INT && iv.as.i >= -(int64_t)len && iv.as.i < (int64_t)len)
        return iv.as.i < 0 ? iv.as.i + (int64_t)len : iv.as.i;
    Buf t = {0};
    if (int_to_text(iv, &t)) {
        buf_free(&t);
        return host_error(vm, "ValueError", INT_STR_LIMIT);
    }
    rt_error(vm, "index %s out of range for length %llu", t.p, (unsigned long long)len);
    buf_free(&t);
    return -1;
}

static Value char_at(VM *vm, ObjString *s, int64_t pos) { /* s rooted */
    size_t i = 0, len;
    if (s->ascii) {
        i = (size_t)pos;
        len = 1;
    } else {
        for (int64_t k = 0; k < pos; k++) {
            utf8_decode(s->s + i, &len);
            i += len;
        }
        utf8_decode(s->s + i, &len);
    }
    return v_obj(new_string(vm, s->s + i, len));
}

static int index_op(VM *vm, Value obj, Value index, Value *out) {
    if (IS_LIST(obj)) {
        int64_t pos = check_index(vm, index, AS_LIST(obj)->n);
        if (pos < 0) return -1;
        *out = AS_LIST(obj)->items[pos];
        return 0;
    }
    if (IS_STR(obj)) {
        int64_t pos = check_index(vm, index, AS_STR(obj)->nchars);
        if (pos < 0) return -1;
        *out = char_at(vm, AS_STR(obj), pos);
        return 0;
    }
    return rt_error(vm, "cannot index a %s", type_name(obj));
}

/* ---------------------------------------------------------------- builtins */

static int b_print(VM *vm, Value *args, int n, Value *out) {
    Buf *line = &vm->line;
    buf_clear(line);
    buf_str(line, "");
    for (int i = 0; i < n; i++) {
        if (i) buf_ch(line, ' ');
        if (stringify(vm, args[i], line, 0)) return -1;
    }
    vm->out(vm->out_ud, line->p, line->n);
    *out = v_nil();
    return 0;
}
static int b_len(VM *vm, Value *args, int n, Value *out) {
    (void)n;
    if (IS_STR(args[0])) *out = v_int((int64_t)AS_STR(args[0])->nchars);
    else if (IS_LIST(args[0])) *out = v_int((int64_t)AS_LIST(args[0])->n);
    else return rt_error(vm, "len() expects a string or list, got %s", type_name(args[0]));
    return 0;
}
static int b_push(VM *vm, Value *args, int n, Value *out) {
    (void)n;
    if (!IS_LIST(args[0])) return rt_error(vm, "push() expects a list");
    list_push(vm, AS_LIST(args[0]), args[1]);
    *out = args[0];
    return 0;
}
static int b_pop(VM *vm, Value *args, int n, Value *out) {
    (void)n;
    if (!IS_LIST(args[0]) || AS_LIST(args[0])->n == 0) return rt_error(vm, "pop() expects a non-empty list");
    ObjList *l = AS_LIST(args[0]);
    *out = l->items[--l->n];
    return 0;
}
static int b_str(VM *vm, Value *args, int n, Value *out) {
    (void)n;
    if (IS_STR(args[0])) { *out = args[0]; return 0; } /* str() of a string is itself, as in Python */
    Buf t = {0};
    buf_str(&t, "");
    if (stringify(vm, args[0], &t, 0)) { buf_free(&t); return -1; }
    *out = v_obj(new_string(vm, t.p, t.n));
    buf_free(&t);
    return 0;
}
/* num(v): int(v) if v is a string of digits after strip() and lstrip("-"),
 * else float(v); TypeError and ValueError become "num() cannot convert %r". */
static int b_num(VM *vm, Value *args, int n, Value *out) {
    (void)n;
    Value v = args[0];
    if (IS_STR(v)) {
        ObjString *s = AS_STR(v);
        size_t a, z;
        py_strip(s->s, s->len, &a, &z);
        size_t minus = 0;
        while (a + minus < z && s->s[a + minus] == '-') minus++;
        size_t digits = 0;
        while (a + minus + digits < z && s->s[a + minus + digits] >= '0' && s->s[a + minus + digits] <= '9') digits++;
        if (digits && a + minus + digits == z) { /* the int() branch */
            if (minus <= 1 && digits <= MAX_STR_DIGITS) {
                *out = int_from_text(vm, s->s + a + minus, digits, minus == 1);
                return 0;
            }
        } else {
            double d;
            if (parse_py_float(s->s + a, z - a, &d)) {
                *out = v_float(d);
                return 0;
            }
        }
    } else if (v.k == V_BOOL) {
        *out = v_float(v.as.b ? 1.0 : 0.0);
        return 0;
    } else if (v.k == V_FLOAT) {
        *out = v;
        return 0;
    } else if (is_intval(v)) {
        double d;
        if (to_double(vm, v, &d)) return -1; /* OverflowError is not caught by num() */
        *out = v_float(d);
        return 0;
    }
    Buf t = {0};
    buf_str(&t, "");
    if (repr_value(vm, v, &t, NULL, 0)) { buf_free(&t); return -1; }
    rt_error(vm, "num() cannot convert %s", t.p);
    buf_free(&t);
    return -1;
}
static int b_type(VM *vm, Value *args, int n, Value *out) {
    (void)n;
    const char *name = type_name(args[0]);
    *out = v_obj(intern(vm, name, strlen(name)));
    return 0;
}
static int b_clock(VM *vm, Value *args, int n, Value *out) {
    (void)vm; (void)args; (void)n;
    *out = v_float(wall_seconds());
    return 0;
}
typedef int (*BuiltinFn)(VM *, Value *, int, Value *);
static const BuiltinFn BUILTIN_FNS[NUM_BUILTINS] = {b_print, b_len, b_push, b_pop, b_str, b_num, b_type, b_clock};

/* ------------------------------------------------------------- interpreter */

static void ensure_stack(VM *vm, size_t need) {
    if (need <= vm->stack_cap) return;
    size_t cap = vm->stack_cap ? vm->stack_cap : 256;
    while (cap < need) cap *= 2;
    vm->stack = xrealloc(vm->stack, cap * sizeof(Value));
    vm->stack_cap = cap;
}

static int32_t handler_at(const ObjCode *co, size_t offset) {
    for (uint32_t i = 0; i < co->nhandlers; i++)
        if ((size_t)co->handlers[i].start <= offset && offset < (size_t)co->handlers[i].end)
            return co->handlers[i].target;
    return -1;
}
static int32_t loop_exit_at(const ObjCode *co, size_t offset) {
    for (uint32_t i = 0; i < co->nloops; i++)
        if ((size_t)co->loops[i].start <= offset && offset < (size_t)co->loops[i].end) return co->loops[i].end;
    return -1;
}

/* NULL if the slot holds no cell: the one fault the verifier cannot rule out
 * statically. It ends the run with an error instead of the process, which
 * would take every other session (or the embedding host) with it. */
static ObjCell *as_cell(Value v) { return is_kind(v, O_CELL) ? AS_CELL(v) : NULL; }

static ErrKind run(VM *vm, int floor, Value *result);

/* Run a loaded script. Returns E_NONE with *result set (the trailing
 * expression's value, for the REPL), or the kind of error that ended it.
 * The script's frame starts at vm->sp, which is 0 in the command-line VM;
 * an embedding host may run a script from inside a host function. */
static ErrKind execute(VM *vm, ObjCode *script, Value *result) {
    vm->err = E_NONE;
    size_t base = vm->sp;
    ensure_stack(vm, base + script->nslots + script->max_stack + STACK_SLACK);
    for (uint32_t i = 0; i < script->nslots; i++) vm->stack[base + i] = v_nil();
    Frame *f = &vm->frames[vm->nframes++];
    f->closure = NULL;
    f->code = script;
    f->base = base;
    f->pc = 0;
    vm->sp = base + script->nslots;
    return run(vm, vm->nframes - 1, result);
}

/* Run from the top frame until the frame at index `floor` finishes. floor is
 * 0 in the command-line VM; when a host calls minilang from inside a host
 * function, the new frames start above the suspended ones, and exceptions,
 * break and return never unwind below floor (embedding).
 *
 * The loop keeps sp, regs and pc in locals. Before anything that can
 * allocate, SYNC() publishes sp to the VM, because the collector marks
 * exactly stack[0 .. vm->sp). Leaving a SYNC() out is a rooting bug of the
 * kind --gc-stress is there to find. */
static ErrKind run(VM *vm, int floor, Value *result) {
    Frame *f = &vm->frames[vm->nframes - 1];
    ObjCode *co = f->code;
    ObjClosure *clo = f->closure;
    const int32_t *code = co->code;
    Value *consts = co->consts;
    Value *stack = vm->stack;
    Value *regs = stack + f->base;
    Value *sp = stack + vm->sp;
    size_t pc = f->pc;
    Value r;
#define SYNC() (vm->sp = (size_t)(sp - stack))
#define LOAD_FRAME()                                                                                       \
    do {                                                                                                   \
        f = &vm->frames[vm->nframes - 1];                                                                  \
        co = f->code; clo = f->closure; code = co->code; consts = co->consts;                             \
        regs = stack + f->base;                                                                            \
    } while (0)
#define FAIL() goto error

    for (;;) {
        int32_t op = code[pc], arg = code[pc + 1];
        pc += 2;
        switch (op) {
        case OP_LOAD_LOCAL: *sp++ = regs[arg]; break;
        case OP_LOAD_CONST: *sp++ = consts[arg]; break;
        case OP_STORE_LOCAL: regs[arg] = *--sp; break;
        case OP_LOAD_CELL: {
            ObjCell *c = as_cell(regs[arg]);
            if (!c) goto no_cell;
            *sp++ = c->v;
            break;
        }
        case OP_STORE_CELL: {
            ObjCell *c = as_cell(regs[arg]);
            if (!c) goto no_cell;
            c->v = *--sp;
            break;
        }
        case OP_MAKE_CELL: {
            SYNC();
            ObjCell *c = (ObjCell *)new_obj(vm, O_CELL, sizeof(ObjCell));
            c->v = v_undef();
            regs[arg] = v_obj(c);
            break;
        }
        case OP_CELL_ARG: {
            SYNC();
            ObjCell *c = (ObjCell *)new_obj(vm, O_CELL, sizeof(ObjCell)); /* the argument is still in its slot */
            c->v = regs[arg];
            regs[arg] = v_obj(c);
            break;
        }
        case OP_LOAD_UPVAL: *sp++ = clo->up[arg]->v; break;
        case OP_STORE_UPVAL: clo->up[arg]->v = *--sp; break;
        case OP_LOAD_GLOBAL:
            if (!table_get(&vm->globals, AS_STR(consts[arg]), sp)) {
                SYNC();
                rt_error(vm, "undefined variable '%s'", AS_STR(consts[arg])->s);
                FAIL();
            }
            sp++;
            break;
        case OP_STORE_GLOBAL: {
            Entry *e = table_find(&vm->globals, AS_STR(consts[arg]));
            if (!e) {
                SYNC();
                rt_error(vm, "assignment to undefined variable '%s'", AS_STR(consts[arg])->s);
                FAIL();
            }
            e->val = *--sp;
            break;
        }
        case OP_DEFINE_GLOBAL:
            --sp;
            table_set(&vm->globals, AS_STR(consts[arg]), *sp); /* malloc'ed table: no collection */
            break;
        case OP_LOAD_CHAIN: {
            Chain *ch = &co->chains[arg];
            uint32_t k = 0;
            for (; k < ch->nmaybe; k++) {
                r = clo->up[ch->maybe[k]]->v;
                if (r.k != V_UNDEF) break;
            }
            if (k == ch->nmaybe) {
                if (ch->is_upval) r = clo->up[ch->final]->v;
                else if (!table_get(&vm->globals, ch->name, &r)) {
                    SYNC();
                    rt_error(vm, "undefined variable '%s'", ch->name->s);
                    FAIL();
                }
            }
            *sp++ = r;
            break;
        }
        case OP_STORE_CHAIN: {
            Chain *ch = &co->chains[arg];
            Value v = sp[-1];
            uint32_t k = 0;
            for (; k < ch->nmaybe; k++) {
                ObjCell *c = clo->up[ch->maybe[k]];
                if (c->v.k != V_UNDEF) { c->v = v; break; }
            }
            if (k == ch->nmaybe) {
                if (ch->is_upval) clo->up[ch->final]->v = v;
                else {
                    Entry *e = table_find(&vm->globals, ch->name);
                    if (!e) {
                        SYNC();
                        rt_error(vm, "assignment to undefined variable '%s'", ch->name->s);
                        FAIL();
                    }
                    e->val = v;
                }
            }
            --sp;
            break;
        }
        case OP_POP: --sp; break;
        case OP_DUP: *sp = sp[-1]; sp++; break;
        case OP_ADD: case OP_SUB: case OP_MUL: {
            Value a = sp[-2], b = sp[-1];
            if (a.k == V_INT && b.k == V_INT) {
                int64_t x = a.as.i, y = b.as.i, z;
                if (op == OP_ADD && !((y > 0 && x > INT64_MAX - y) || (y < 0 && x < INT64_MIN - y))) z = x + y;
                else if (op == OP_SUB && !((y < 0 && x > INT64_MAX + y) || (y > 0 && x < INT64_MIN + y))) z = x - y;
                else if (op == OP_MUL && x > -3037000499 && x < 3037000499 && y > -3037000499 && y < 3037000499) z = x * y;
                else goto slow_binary; /* overflow: arbitrary precision */
                sp[-2] = v_int(z);
                sp--;
                break;
            }
            if (a.k == V_FLOAT && b.k == V_FLOAT) {
                sp[-2] = v_float(op == OP_ADD ? a.as.f + b.as.f : op == OP_SUB ? a.as.f - b.as.f : a.as.f * b.as.f);
                sp--;
                break;
            }
            goto slow_binary;
        }
        case OP_LT: case OP_LE: case OP_GT: case OP_GE: {
            Value a = sp[-2], b = sp[-1];
            if (a.k == V_INT && b.k == V_INT) {
                int64_t x = a.as.i, y = b.as.i;
                sp[-2] = v_bool(op == OP_LT ? x < y : op == OP_LE ? x <= y : op == OP_GT ? x > y : x >= y);
                sp--;
                break;
            }
            goto slow_binary;
        }
        case OP_MOD:
            if (sp[-2].k == V_INT && sp[-1].k == V_INT && sp[-1].as.i != 0) {
                sp[-2] = int_mod(vm, sp[-2], sp[-1]); /* int64 path: no allocation */
                sp--;
                break;
            }
            goto slow_binary;
        case OP_EQ: case OP_NE: {
            int eq = values_equal(sp[-2], sp[-1]);
            sp[-2] = v_bool(op == OP_EQ ? eq : !eq);
            sp--;
            break;
        }
        case OP_DIV:
        slow_binary:
            SYNC(); /* operands stay on the stack while the result is allocated */
            if (binary_op(vm, op, sp[-2], sp[-1], &r)) FAIL();
            sp[-2] = r;
            sp--;
            break;
        case OP_NEG: {
            Value a = sp[-1];
            if (a.k == V_FLOAT) sp[-1] = v_float(-a.as.f);
            else if (is_intval(a)) {
                SYNC();
                sp[-1] = int_neg(vm, a);
            } else {
                SYNC();
                rt_error(vm, "unary '-' expects a number, got %s", type_name(a));
                FAIL();
            }
            break;
        }
        case OP_NOT: sp[-1] = v_bool(falsy(sp[-1])); break;
        case OP_INDEX: {
            Value a = sp[-2], b = sp[-1];
            if (b.k == V_INT && IS_LIST(a) && b.as.i >= 0 && (uint64_t)b.as.i < AS_LIST(a)->n) {
                sp[-2] = AS_LIST(a)->items[b.as.i];
                sp--;
                break;
            }
            SYNC();
            if (index_op(vm, a, b, &r)) FAIL();
            sp[-2] = r;
            sp--;
            break;
        }
        case OP_SET_INDEX: {
            Value a = sp[-3], b = sp[-2], v = sp[-1];
            SYNC();
            if (!IS_LIST(a)) {
                rt_error(vm, "can only assign into a list, got %s", type_name(a));
                FAIL();
            }
            int64_t pos = check_index(vm, b, AS_LIST(a)->n);
            if (pos < 0) FAIL();
            AS_LIST(a)->items[pos] = v;
            sp[-3] = v;
            sp -= 2;
            break;
        }
        case OP_BUILD_LIST: {
            SYNC();
            ObjList *l = new_list(vm, (size_t)arg); /* the items are still on the stack */
            for (int32_t i = 0; i < arg; i++) l->items[i] = sp[i - arg];
            l->n = (size_t)arg;
            sp -= arg;
            *sp++ = v_obj(l);
            break;
        }
        case OP_MAKE_CLOSURE: {
            ObjCode *fc = AS_CODE(consts[arg]);
            SYNC();
            ObjClosure *c = (ObjClosure *)new_obj(vm, O_CLOSURE, sizeof(ObjClosure) + fc->nup * sizeof(ObjCell *));
            c->code = fc;
            c->nup = fc->nup;
            for (uint32_t i = 0; i < fc->nup; i++) {
                c->up[i] = fc->ups[i].local ? as_cell(regs[fc->ups[i].index]) : clo->up[fc->ups[i].index];
                if (!c->up[i]) goto no_cell; /* c is unreachable garbage; NULL upvalues are not traced */
            }
            *sp++ = v_obj(c);
            break;
        }
        case OP_CALL: {
            if (vm->over_limit) goto out_of_memory;
            Value fn = sp[-arg - 1];
            if (IS_CLOSURE(fn)) {
                ObjClosure *c = AS_CLOSURE(fn);
                ObjCode *callee = c->code;
                SYNC();
                if ((uint32_t)arg != callee->nparams) {
                    rt_error(vm, "<fn %s> expects %u argument(s), got %d", callee->name->s, (unsigned)callee->nparams, (int)arg);
                    FAIL();
                }
                if (vm->nframes - 1 >= MAX_DEPTH) {
                    rt_error(vm, "stack overflow (max call depth %d)", MAX_DEPTH);
                    FAIL();
                }
                f->pc = pc;
                size_t base = (size_t)(sp - stack) - (size_t)arg;
                size_t need = base + callee->nslots + callee->max_stack + STACK_SLACK;
                if (need > vm->stack_cap) {
                    ensure_stack(vm, need);
                    sp = vm->stack + (sp - stack);
                    stack = vm->stack;
                }
                for (uint32_t i = (uint32_t)arg; i < callee->nslots; i++) stack[base + i] = v_nil();
                f = &vm->frames[vm->nframes++];
                f->closure = c;
                f->code = callee;
                f->base = base;
                f->pc = 0;
                co = callee;
                clo = c;
                code = co->code;
                consts = co->consts;
                regs = stack + base;
                sp = regs + co->nslots;
                pc = 0;
            } else if (fn.k == V_BUILTIN) {
                int which = (int)fn.as.i;
                SYNC();
                if (BUILTIN_ARITY[which] >= 0 && arg != BUILTIN_ARITY[which]) {
                    rt_error(vm, "%s() expects %d argument(s), got %d", BUILTIN_NAMES[which], BUILTIN_ARITY[which], (int)arg);
                    FAIL();
                }
                if (BUILTIN_FNS[which](vm, sp - arg, arg, &r)) FAIL();
                sp -= arg;
                sp[-1] = r;
            } else if (is_kind(fn, O_HOST) && vm->call_host) {
                /* Embedding: a host function may run minilang code, which can
                 * move the stack, so the cached pointers are reloaded. */
                size_t at = (size_t)(sp - stack) - (size_t)arg - 1;
                SYNC();
                int failed = vm->call_host(vm, (ObjHost *)fn.as.o, arg, &r);
                stack = vm->stack;
                regs = stack + f->base;
                sp = stack + at;
                if (failed) {
                    sp += arg + 1;
                    FAIL();
                }
                *sp++ = r;
            } else {
                SYNC();
                rt_error(vm, "cannot call a %s", type_name(fn));
                FAIL();
            }
            break;
        }
        case OP_JUMP:
            if (vm->over_limit) goto out_of_memory; /* every loop's back edge is a JUMP */
            pc = (size_t)arg;
            break;
        case OP_JUMP_IF_FALSE:
            if (falsy(*--sp)) pc = (size_t)arg;
            break;
        case OP_JUMP_IF_FALSE_OR_POP:
            if (falsy(sp[-1])) pc = (size_t)arg;
            else sp--;
            break;
        case OP_JUMP_IF_TRUE_OR_POP:
            if (!falsy(sp[-1])) pc = (size_t)arg;
            else sp--;
            break;
        case OP_RETURN: {
            r = sp[-1];
            if (vm->nframes - 1 == floor) {
                if (!clo) { /* top-level return: ReturnSignal escapes, as in the tree-walker */
                    SYNC();
                    vm->err = E_RETURN;
                    vm->errval = r;
                    vm->has_errval = 1;
                    return E_RETURN;
                }
                /* embedding: a function the host called returns to the host */
                sp = stack + f->base - 1;
                *sp++ = r;
                SYNC();
                vm->nframes--;
                *result = r;
                return E_NONE;
            }
            size_t at = f->base - 1;
            vm->nframes--;
            LOAD_FRAME();
            pc = f->pc;
            stack[at] = r;
            sp = stack + at + 1;
            break;
        }
        case OP_THROW: {
            Value v = sp[-1];
            SYNC();
            buf_clear(&vm->errmsg);
            /* The tree-walker builds the message when it throws, so an
             * unprintable value fails here even if a catch would take it. */
            if (stringify(vm, v, &vm->errmsg, 0)) FAIL();
            vm->err = E_RUNTIME;
            vm->errline = -1;
            vm->has_errval = v.k != V_NIL; /* throw nil is caught as the string "nil" */
            vm->errval = v;
            FAIL();
        }
        case OP_BREAK_OUT: {
            int32_t target;
            for (;;) { /* the nearest caller whose CALL sits inside one of its loops */
                if (vm->nframes - 1 == floor) {
                    SYNC();
                    vm->err = E_BREAK;
                    return E_BREAK;
                }
                vm->nframes--;
                LOAD_FRAME();
                target = loop_exit_at(co, f->pc - 2);
                if (target >= 0) break;
            }
            sp = regs + co->nslots;
            pc = (size_t)target;
            break;
        }
        case OP_HALT:
            *result = sp[-1];
            SYNC();
            return E_NONE;
        case OP_ADD_CONST: case OP_SUB_CONST: {
            Value a = sp[-1], b = consts[arg];
            if (a.k == V_INT && b.k == V_INT) {
                int64_t x = a.as.i, y = b.as.i;
                if (op == OP_ADD_CONST && !((y > 0 && x > INT64_MAX - y) || (y < 0 && x < INT64_MIN - y))) {
                    sp[-1] = v_int(x + y);
                    break;
                }
                if (op == OP_SUB_CONST && !((y < 0 && x > INT64_MAX + y) || (y > 0 && x < INT64_MIN + y))) {
                    sp[-1] = v_int(x - y);
                    break;
                }
            }
            *sp++ = b; /* the generic path wants both operands rooted on the stack */
            op = op == OP_ADD_CONST ? OP_ADD : OP_SUB;
            goto slow_binary;
        }
        case OP_LT_CONST: {
            Value a = sp[-1], b = consts[arg];
            if (a.k == V_INT && b.k == V_INT) {
                sp[-1] = v_bool(a.as.i < b.as.i);
                break;
            }
            *sp++ = b;
            op = OP_LT;
            goto slow_binary;
        }
        case OP_JUMP_IF_NOT_LT: {
            Value a = sp[-2], b = sp[-1];
            int lt;
            if (a.k == V_INT && b.k == V_INT) lt = a.as.i < b.as.i;
            else {
                SYNC();
                if (binary_op(vm, OP_LT, a, b, &r)) FAIL();
                lt = r.as.b;
            }
            sp -= 2;
            if (!lt) pc = (size_t)arg;
            break;
        }
        default:
            fatal("unknown opcode"); /* the verifier rules this out */
        }
        continue;

    no_cell: /* malformed bytecode; not catchable, since no minilang program can cause it */
        SYNC();
        host_error(vm, "SystemError", "bytecode reads a slot that holds no cell");
        goto error;
    out_of_memory: /* embedding: the heap passed the host's limit; not catchable, like Python's MemoryError */
        SYNC();
        vm->over_limit = 0;
        host_error(vm, "MemoryError", "heap limit exceeded");
    error:
        /* Unwind to the innermost handler covering the faulting instruction,
         * first in this frame, then at each caller's CALL instruction. */
        if (vm->err == E_RUNTIME && vm->errline < 0) vm->errline = line_at(co, pc - 2);
        if (vm->err == E_RECURSION && vm->nframes - 1 > floor) {
            /* Interpreter.call() turns RecursionError into a minilang error
             * at the call site of the innermost minilang function. */
            vm->nframes--;
            LOAD_FRAME();
            pc = f->pc;
            rt_error(vm, "stack overflow (host recursion limit)");
            vm->errline = line_at(co, pc - 2);
        }
        if (vm->err != E_RUNTIME) {
            SYNC();
            return vm->err;
        }
        {
            int32_t target;
            for (;;) {
                target = handler_at(co, pc - 2);
                if (target >= 0) break;
                if (vm->nframes - 1 == floor) {
                    SYNC();
                    return E_RUNTIME;
                }
                vm->nframes--;
                LOAD_FRAME();
                pc = f->pc;
            }
            sp = regs + co->nslots; /* statements start with an empty operand stack */
            SYNC();
            Value caught = vm->has_errval ? vm->errval : v_obj(new_string(vm, vm->errmsg.p, vm->errmsg.n));
            *sp++ = caught;
            vm->has_errval = 0;
            vm->err = E_NONE;
            pc = (size_t)target;
        }
    }
#undef SYNC
#undef LOAD_FRAME
#undef FAIL
}

/* ------------------------------------------------------------------ driver */

static void stdout_line(void *ud, const char *s, size_t n) {
    (void)ud;
    fwrite(s, 1, n, stdout);
    fputc('\n', stdout);
}

static VM *vm_new(const VMOptions *opt) {
    VM *vm = xrealloc(NULL, sizeof(VM));
    memset(vm, 0, sizeof *vm);
    vm->stress = opt->stress;
    vm->min_heap = opt->min_heap ? opt->min_heap : GC_MIN_HEAP;
    vm->next_gc = vm->min_heap;
    vm->out = stdout_line;
    vm->errval = v_nil();
    for (int i = 0; i < NUM_BUILTINS; i++) {
        ObjString *name = intern(vm, BUILTIN_NAMES[i], strlen(BUILTIN_NAMES[i]));
        table_set(&vm->globals, name, v_builtin(i)); /* reachable from now on */
    }
    return vm;
}

/* Free everything; returns the bytes the heap still counted afterwards, which
 * must be 0 (the tests check it: no object or buffer escapes the accounting). */
static size_t vm_free(VM *vm) {
    vm->sp = 0;
    vm->nframes = 0;
    vm->ntemps = 0;
    vm->has_errval = 0;
    table_free(&vm->globals);
    while (vm->objects) {
        Obj *o = vm->objects;
        vm->objects = o->next;
        free_obj(vm, o);
    }
    size_t left = vm->bytes;
    table_free(&vm->strings);
    free(vm->stack);
    free(vm->gray);
    free(vm->temps);
    buf_free(&vm->errmsg);
    buf_free(&vm->line);
    free(vm);
    return left;
}

static int read_file(const char *path, uint8_t **data, size_t *n) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    size_t cap = 4096, len = 0;
    uint8_t *p = xrealloc(NULL, cap);
    for (;;) {
        size_t got = fread(p + len, 1, cap - len, fp);
        len += got;
        if (len < cap) break;
        cap *= 2;
        p = xrealloc(p, cap);
    }
    fclose(fp);
    *data = p;
    *n = len;
    return 0;
}

#ifndef MLVM_LIBRARY /* the rest is the command-line driver; the embedding API replaces it */

static void stats_text(VM *vm, Buf *b) {
    buf_str(b, "collections=");
    buf_u64(b, vm->collections);
    buf_printf(b, " pause_total_us=%.1f pause_max_us=%.1f mark_us=%.1f sweep_us=%.1f",
               (double)vm->pause_ns_total / 1e3, (double)vm->pause_ns_max / 1e3, (double)vm->mark_ns / 1e3,
               (double)vm->sweep_ns / 1e3);
    buf_str(b, " allocated_bytes=");
    buf_u64(b, vm->total_bytes);
    buf_str(b, " allocated_objects=");
    buf_u64(b, vm->total_objects);
    buf_str(b, " freed_objects=");
    buf_u64(b, vm->freed_objects);
    buf_str(b, " peak_bytes=");
    buf_u64(b, vm->peak_bytes);
    buf_str(b, " live_bytes=");
    buf_u64(b, vm->bytes);
    buf_str(b, " live_objects=");
    buf_u64(b, vm->live_objects);
    buf_str(b, " next_gc=");
    buf_u64(b, vm->next_gc);
    buf_printf(b, " stress=%d load_us=%.1f run_us=%.1f", vm->stress, (double)vm->load_ns / 1e3,
               (double)vm->run_ns / 1e3);
}

/* Load and run one file in vm. Fills `ending` with how the run ended, in the
 * serve protocol's words; returns 0 for a normal end. */
static void escape_into(Buf *b, const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\\') buf_str(b, "\\\\");
        else if (s[i] == '\n') buf_str(b, "\\n");
        else if (s[i] == '\r') buf_str(b, "\\r");
        else buf_ch(b, s[i]);
    }
}
static int typed_value(VM *vm, Value v, Buf *b) { /* serve protocol: "<type> <text>" */
    Buf t = {0};
    buf_str(&t, "");
    int rc = 0;
    if (v.k == V_NIL) buf_str(b, "nil");
    else if (v.k == V_BOOL) buf_str(b, v.as.b ? "true" : "false");
    else if (is_intval(v)) {
        buf_str(b, "int ");
        if (v.k == V_INT) buf_i64(b, v.as.i);
        else rc = int_to_text(v, b) ? host_error(vm, "ValueError", INT_STR_LIMIT) : 0;
    } else if (v.k == V_FLOAT) {
        char r[48];
        float_repr(v.as.f, r);
        buf_printf(b, "float %s", r);
    } else if (IS_STR(v)) {
        buf_str(b, "str ");
        escape_into(b, AS_STR(v)->s, AS_STR(v)->len);
    } else {
        rc = stringify(vm, v, &t, 0);
        if (!rc) {
            buf_str(b, "other ");
            escape_into(b, t.p, t.n);
        }
    }
    buf_free(&t);
    return rc;
}
static void ending_text(VM *vm, ErrKind kind, Value result, Buf *b) {
    if (kind == E_NONE || kind == E_RETURN) { /* the value itself may fail to print */
        Buf t = {0};
        buf_str(&t, kind == E_NONE ? "v " : "r ");
        if (typed_value(vm, kind == E_NONE ? result : vm->errval, &t)) kind = vm->err;
        else buf_add(b, t.p, t.n);
        buf_free(&t);
    }
    switch (kind) {
    case E_NONE: case E_RETURN: break;
    case E_RUNTIME:
        buf_printf(b, "e %d ", vm->errline);
        escape_into(b, vm->errmsg.p, vm->errmsg.n);
        break;
    case E_BREAK: buf_str(b, "b"); break;
    case E_RECURSION: case E_HOST:
        buf_printf(b, "h %s ", vm->host_type);
        escape_into(b, vm->errmsg.p, vm->errmsg.n);
        break;
    }
}
static ErrKind run_path(VM *vm, const char *path, Value *result, char *err, size_t errcap) {
    uint8_t *data;
    size_t n;
    if (read_file(path, &data, &n)) {
        snprintf(err, errcap, "cannot read %s", path);
        return E_HOST;
    }
    uint64_t t0 = now_ns();
    int temps = vm->ntemps;
    ObjCode *script = load_program(vm, data, n, err, errcap);
    free(data);
    uint64_t t1 = now_ns();
    vm->load_ns += t1 - t0;
    if (!script) return E_HOST;
    ErrKind kind = execute(vm, script, result);
    vm->run_ns += now_ns() - t1;
    vm->ntemps = temps;
    vm->nframes = 0; /* the next collection no longer sees this run's stack */
    if (kind != E_NONE && kind != E_RETURN) vm->sp = 0;
    return kind;
}

/* --serve: one command per line on stdin, replies on stdout.
 *   new [stress] [min=BYTES]  -> "k ID"               a fresh VM (own heap and globals)
 *   run ID PATH               -> "o TEXT"* then one ending line:
 *        "v TYPE TEXT" value | "r TYPE TEXT" escaped return | "b" escaped break
 *        "e LINE MESSAGE" uncaught error | "h TYPE MESSAGE" Python-level error | "l MESSAGE" load error
 *   stats ID                  -> "s key=value ..."
 *   free ID                   -> "k leaked=BYTES"
 * TEXT escapes backslash, newline and CR. Globals persist across runs in one VM. */
#define MAX_SESSIONS 256
static void serve_line(void *ud, const char *s, size_t n) {
    Buf *b = ud;
    buf_clear(b);
    buf_str(b, "o ");
    escape_into(b, s, n);
    buf_ch(b, '\n');
    fwrite(b->p, 1, b->n, stdout);
}
static int serve(void) {
    binary_stdio();
    VM *vms[MAX_SESSIONS] = {0};
    Buf out = {0}, reply = {0};
    char line[8192];
    while (fgets(line, sizeof line, stdin)) {
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        buf_clear(&reply);
        buf_str(&reply, "");
        if (strncmp(line, "new", 3) == 0) {
            VMOptions opt = {0, 0};
            if (strstr(line, " stress")) opt.stress = 1;
            const char *m = strstr(line, " min=");
            if (m) opt.min_heap = (size_t)strtoull(m + 5, NULL, 10);
            int id = 0;
            while (id < MAX_SESSIONS && vms[id]) id++;
            if (id == MAX_SESSIONS) buf_str(&reply, "l too many sessions");
            else {
                vms[id] = vm_new(&opt);
                vms[id]->out = serve_line;
                vms[id]->out_ud = &out;
                buf_printf(&reply, "k %d", id);
            }
        } else if (strncmp(line, "run ", 4) == 0 || strncmp(line, "stats ", 6) == 0 || strncmp(line, "free ", 5) == 0) {
            char *rest = strchr(line, ' ') + 1;
            char *endp;
            long id = strtol(rest, &endp, 10);
            if (id < 0 || id >= MAX_SESSIONS || !vms[id]) {
                buf_str(&reply, "l no such session");
            } else if (line[0] == 'r') {
                const char *path = *endp == ' ' ? endp + 1 : endp;
                char err[256];
                Value result = v_nil();
                err[0] = 0;
                ErrKind kind = run_path(vms[id], path, &result, err, sizeof err);
                if (err[0]) {
                    buf_str(&reply, "l ");
                    escape_into(&reply, err, strlen(err));
                } else {
                    ending_text(vms[id], kind, result, &reply);
                }
                vms[id]->sp = 0;
                vms[id]->has_errval = 0;
            } else if (line[0] == 's') {
                buf_str(&reply, "s ");
                stats_text(vms[id], &reply);
            } else {
                size_t left = vm_free(vms[id]);
                vms[id] = NULL;
                buf_str(&reply, "k leaked=");
                buf_u64(&reply, left);
            }
        } else if (strcmp(line, "quit") == 0) {
            break;
        } else {
            buf_str(&reply, "l unknown command");
        }
        buf_ch(&reply, '\n');
        fwrite(reply.p, 1, reply.n, stdout);
        fflush(stdout);
    }
    for (int i = 0; i < MAX_SESSIONS; i++)
        if (vms[i]) vm_free(vms[i]);
    buf_free(&out);
    buf_free(&reply);
    return 0;
}

/* --selftest-float: for each line of 16 hex digits (the bits of a double),
 * print repr() and minilang's str(); tests compare them with Python. */
static int selftest_float(void) {
    binary_stdio();
    char line[64];
    Buf b = {0};
    while (fgets(line, sizeof line, stdin)) {
        uint64_t bits = strtoull(line, NULL, 16);
        double d;
        memcpy(&d, &bits, sizeof d);
        char r[48];
        float_repr(d, r);
        buf_clear(&b);
        buf_str(&b, r);
        buf_ch(&b, ' ');
        float_text(d, &b);
        buf_ch(&b, '\n');
        fwrite(b.p, 1, b.n, stdout);
    }
    buf_free(&b);
    return 0;
}

static int usage(void) {
    fputs("usage: mlvm [--gc-stress] [--gc-min BYTES] [--stats] program.mlc\n"
          "       mlvm --dis program.mlc\n"
          "       mlvm --serve\n"
          "Compile programs with: python3 ../minilang-vm/minilang_vm.py --emit program.mlc program.ml\n",
          stderr);
    return 2;
}

int main(int argc, char **argv) {
    VMOptions opt = {0, 0};
    int stats = 0, dis = 0;
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--serve") == 0) return serve();
        if (strcmp(argv[i], "--selftest-float") == 0) return selftest_float();
        if (strcmp(argv[i], "--gc-stress") == 0) opt.stress = 1;
        else if (strcmp(argv[i], "--stats") == 0) stats = 1;
        else if (strcmp(argv[i], "--dis") == 0) dis = 1;
        else if (strcmp(argv[i], "--gc-min") == 0 && i + 1 < argc) opt.min_heap = (size_t)strtoull(argv[++i], NULL, 10);
        else if (argv[i][0] == '-' || path) return usage();
        else path = argv[i];
    }
    if (!path) return usage();
    static char outbuf[1 << 16];
    setvbuf(stdout, outbuf, _IOFBF, sizeof outbuf);
    VM *vm = vm_new(&opt);
    int status = 0;
    if (dis) {
        uint8_t *data;
        size_t n;
        char err[256];
        if (read_file(path, &data, &n)) {
            fprintf(stderr, "mlvm: cannot read %s\n", path);
            vm_free(vm);
            return 2;
        }
        ObjCode *script = load_program(vm, data, n, err, sizeof err);
        free(data);
        if (!script) {
            fprintf(stderr, "mlvm: %s: %s\n", path, err);
            vm_free(vm);
            return 2;
        }
        Buf b = {0};
        disassemble(vm, script, &b);
        fwrite(b.p, 1, b.n, stdout);
        fputc('\n', stdout);
        buf_free(&b);
    } else {
        char err[256];
        Value result = v_nil();
        err[0] = 0;
        ErrKind kind = run_path(vm, path, &result, err, sizeof err);
        fflush(stdout);
        if (err[0]) {
            fprintf(stderr, "mlvm: %s: %s\n", path, err);
            status = 2;
        } else if (kind == E_RUNTIME) {
            fprintf(stderr, "error: [line %d] %s\n", vm->errline, vm->errmsg.p);
            status = 1;
        } else if (kind == E_RETURN) {
            fprintf(stderr, "error: top-level return (the tree-walker's ReturnSignal escapes)\n");
            status = 1;
        } else if (kind == E_BREAK) {
            fprintf(stderr, "error: break outside a loop (the tree-walker's BreakSignal escapes)\n");
            status = 1;
        } else if (kind != E_NONE) {
            fprintf(stderr, "error: %s: %s (a Python exception in the tree-walker)\n", vm->host_type, vm->errmsg.p);
            status = 1;
        }
        if (stats) {
            Buf b = {0};
            buf_str(&b, "mlvm stats: ");
            stats_text(vm, &b);
            fprintf(stderr, "%s\n", b.p);
            buf_free(&b);
        }
    }
    fflush(stdout);
    size_t left = vm_free(vm);
    if (left) {
        fprintf(stderr, "mlvm: heap accounting error: %lu bytes left after teardown\n", (unsigned long)left);
        return 3;
    }
    return status;
}
#endif /* MLVM_LIBRARY */
