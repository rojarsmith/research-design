/* minilang.h: the embedding API of the minilang C VM (roadmap stage 6).
 *
 * The library is minilang.c, which includes ../minilang-c/mlvm.c; nothing
 * from mlvm.c is visible here. Build a host program with:
 *     gcc -std=c11 -Wall -Wextra -Werror -O2 -o host host.c minilang.c -lm
 * Programs are bytecode files written by ../minilang-vm/minilang_vm.py --emit.
 *
 * Four rules (README.md explains them):
 *
 * 1. Handles, not pointers. Host code never sees a minilang value; it holds
 *    an ml_value, a handle to a slot that the garbage collector treats as a
 *    root. A handle stays valid until it is released, its scope is closed,
 *    or the VM is closed. Using a handle after that is detected (each slot
 *    has a generation count) and reported as API misuse, never a read of
 *    freed memory. The one raw pointer the API returns is the text of
 *    ml_get_string(), valid while that handle is live.
 *
 * 2. Scopes. Handles are created in the innermost open scope and all of them
 *    are released when it closes (V8's HandleScope, Node-API's handle
 *    scopes). Each call of a host function runs in its own scope, so its
 *    arguments and temporaries need no cleanup. ml_pin() makes a handle that
 *    belongs to no scope, for values a host keeps between calls.
 *
 * 3. Status codes. Every call returns an ml_status. A failed call leaves a
 *    pending error (message, type, line, value) that the ml_error_*()
 *    functions read. A host function reports failure by returning a status
 *    other than ML_OK with the error still pending: returning the status of
 *    a failed API call propagates that error into the calling minilang code,
 *    where an ML_EXCEPTION is catchable by try/catch. Errors never longjmp
 *    through host frames.
 *
 * 4. Misuse is loud. Stale handles, unbalanced scopes and calls from the
 *    print callback are host bugs: by default the library prints the problem
 *    and calls abort(). Set ml_options.misuse_returns to get ML_MISUSE.
 *
 * One VM is used by one thread at a time; separate VMs share no state.
 */
#ifndef MINILANG_H
#define MINILANG_H

#include <stddef.h>
#include <stdint.h>

#define MINILANG_API_VERSION 1 /* compare with ml_api_version() at run time */

typedef struct ml_vm ml_vm; /* opaque: one heap, one global table */

/* A handle. {0, 0} (ML_NO_VALUE) is never valid. Compare values with
 * ml_equal(), not the handle fields. */
typedef struct ml_value {
    uint32_t id;
    uint32_t gen;
} ml_value;
#define ML_NO_VALUE ((ml_value){0, 0})

typedef enum ml_status {
    ML_OK = 0,
    ML_EXCEPTION,  /* a minilang exception (throw, a runtime error, ml_throw): catchable by try/catch */
    ML_ERROR,      /* an error minilang cannot catch: OverflowError, ValueError, RecursionError,
                      BreakSignal, ReturnSignal, SystemError, LoadError (see ml_error_type) */
    ML_NOMEM,      /* the heap passed ml_options.heap_limit (MemoryError, not catchable) */
    ML_TYPE_ERROR, /* a value has the wrong type, is out of range or is not valid UTF-8;
                      pending as a catchable exception */
    ML_NOT_FOUND,  /* no such global; pending as a catchable "undefined variable" exception */
    ML_LOAD_ERROR, /* the loader rejected a bytecode file (LoadError) */
    ML_MISUSE      /* a bug in the host (only with misuse_returns; otherwise abort) */
} ml_status;

typedef enum ml_type {
    ML_TINVALID, /* a stale or invalid handle */
    ML_TNIL,
    ML_TBOOL,
    ML_TINT,     /* any integer; ml_get_int() needs one that fits in int64_t */
    ML_TFLOAT,
    ML_TSTRING,
    ML_TLIST,
    ML_TFUNCTION, /* minilang function, builtin or host function */
    ML_TPROGRAM,  /* a loaded bytecode file */
    ML_TOTHER
} ml_type;

/* A host function. args are handles in the call's own scope; set *result to
 * any live handle (left unset, the result is nil) and return ML_OK, or
 * return an error status with an error pending (ml_throw, ml_throwf, or the
 * status of a failed API call). The VM checks arity before calling. */
typedef ml_status (*ml_cfunction)(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud);

/* print()'s output, one line per call, without the newline. The callback
 * must not call back into the VM. */
typedef void (*ml_output)(void *ud, const char *text, size_t len);

typedef struct ml_options {
    int gc_stress;      /* collect on every allocation and poison freed memory (testing) */
    size_t gc_min_heap; /* never collect below this many bytes; 0: 1 MiB */
    size_t heap_limit;  /* 0: none. A soft limit: the heap may pass it by one instruction's
                           allocations, then minilang code stops with MemoryError and API
                           calls return ML_NOMEM */
    int misuse_returns; /* 0: print API misuse on stderr and abort(); 1: return ML_MISUSE */
} ml_options;

typedef struct ml_stats {
    uint64_t collections;
    size_t heap_bytes, peak_bytes, live_objects;
    size_t handles, pinned; /* live handles in scopes, and pinned handles */
} ml_stats;

int ml_api_version(void);

/* ---- VMs */
ml_vm *ml_open(const ml_options *options); /* NULL: defaults */
size_t ml_close(ml_vm *vm); /* frees everything; returns the bytes the heap still counted (0) */
void ml_set_output(ml_vm *vm, ml_output fn, void *ud); /* NULL fn: stdout */
void ml_get_stats(ml_vm *vm, ml_stats *out);
ml_status ml_collect(ml_vm *vm);

/* ---- programs and calls */
ml_status ml_load(ml_vm *vm, const void *bytes, size_t len, ml_value *program);
ml_status ml_load_file(ml_vm *vm, const char *path, ml_value *program);
/* Runs a program's top level; *result (may be NULL) is its trailing expression's value. */
ml_status ml_run(ml_vm *vm, ml_value program, ml_value *result);
/* Calls a minilang function, builtin or host function. Allowed inside host
 * functions (minilang -> C -> minilang); exceptions, break and return never
 * unwind past the C caller. */
ml_status ml_call(ml_vm *vm, ml_value fn, int nargs, const ml_value *args, ml_value *result);

/* ---- globals and host functions; names are UTF-8 */
ml_status ml_get_global(ml_vm *vm, const char *name, ml_value *out);
ml_status ml_set_global(ml_vm *vm, const char *name, ml_value value);
/* arity -1 accepts any number of arguments */
ml_status ml_new_function(ml_vm *vm, const char *name, ml_cfunction fn, int arity, void *ud, ml_value *out);
ml_status ml_register(ml_vm *vm, const char *name, ml_cfunction fn, int arity, void *ud);

/* ---- handles */
int ml_scope_open(ml_vm *vm); /* returns the scope's number, -1 on misuse */
/* Closes the innermost scope, which must be `scope`. If keep is not NULL,
 * *keep is replaced by a handle to the same value in the enclosing scope
 * (V8's EscapableHandleScope). */
ml_status ml_scope_close(ml_vm *vm, int scope, ml_value *keep);
ml_status ml_pin(ml_vm *vm, ml_value v, ml_value *pinned); /* released only by ml_release */
ml_status ml_release(ml_vm *vm, ml_value v);

/* ---- making values (new handles in the innermost scope) */
ml_status ml_new_nil(ml_vm *vm, ml_value *out);
ml_status ml_new_bool(ml_vm *vm, int b, ml_value *out);
ml_status ml_new_int(ml_vm *vm, int64_t i, ml_value *out);
ml_status ml_new_float(ml_vm *vm, double d, ml_value *out);
ml_status ml_new_string(ml_vm *vm, const char *s, size_t len, ml_value *out); /* UTF-8, copied */
ml_status ml_new_list(ml_vm *vm, ml_value *out);

/* ---- reading values */
ml_type ml_typeof(ml_vm *vm, ml_value v);
int ml_truthy(ml_vm *vm, ml_value v);           /* only nil and false are falsy */
int ml_equal(ml_vm *vm, ml_value a, ml_value b); /* minilang's == */
ml_status ml_get_bool(ml_vm *vm, ml_value v, int *out);
ml_status ml_get_int(ml_vm *vm, ml_value v, int64_t *out);
ml_status ml_get_float(ml_vm *vm, ml_value v, double *out); /* any number */
/* *s points into the heap, NUL-terminated, valid while v is live. */
ml_status ml_get_string(ml_vm *vm, ml_value v, const char **s, size_t *len);
ml_status ml_tostring(ml_vm *vm, ml_value v, ml_value *out); /* minilang's str() */

/* ---- lists */
ml_status ml_list_len(ml_vm *vm, ml_value list, size_t *len);
ml_status ml_list_get(ml_vm *vm, ml_value list, size_t i, ml_value *out);
ml_status ml_list_set(ml_vm *vm, ml_value list, size_t i, ml_value v);
ml_status ml_list_push(ml_vm *vm, ml_value list, ml_value v);

/* ---- errors */
ml_status ml_throw(ml_vm *vm, ml_value v);              /* returns ML_EXCEPTION */
ml_status ml_throwf(ml_vm *vm, const char *fmt, ...);   /* the message string is the thrown value */
const char *ml_error_message(ml_vm *vm); /* "" if none; valid until the next failing call */
const char *ml_error_type(ml_vm *vm);    /* "RuntimeErr" for exceptions, else the uncatchable type */
int ml_error_line(ml_vm *vm);            /* the minilang line, 0 if none */
/* The thrown value (ReturnSignal: the returned value); for other errors, the message. */
ml_status ml_error_value(ml_vm *vm, ml_value *out);
void ml_clear_error(ml_vm *vm);
const char *ml_status_name(ml_status status);

#endif
