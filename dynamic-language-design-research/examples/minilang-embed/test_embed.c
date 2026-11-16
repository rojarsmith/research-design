/* test_embed.c: tests for the embedding API in minilang.h.
 *
 * Build and run (test_minilang_embed.py compiles the programs in tests/ and
 * runs all of this):
 *     gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_embed test_embed.c minilang.c -lm
 *     python3 ../minilang-vm/minilang_vm.py --emit DIR/api.mlc tests/api.ml   (each program in tests/)
 *     ./test_embed DIR                    plain
 *     ./test_embed --gc-stress DIR        every VM collects on every allocation
 *     ./test_embed --gc-min 4096 DIR      small heap: many collections
 *     ./test_embed --crash KIND DIR       must abort with an "API misuse" message (KIND: stale, close)
 *
 * Every VM is closed with ml_close() == 0: no object escapes the accounting.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minilang.h"

static int checks, failures;
static int stress;
static size_t min_heap;
static const char *dir = ".";

static void check(int ok, const char *what, int line) {
    checks++;
    if (!ok) {
        failures++;
        printf("FAIL line %d: %s\n", line, what);
    }
}
#define CHECK(cond) check((cond) != 0, #cond, __LINE__)
/* A call and the status it must return; on a mismatch, show the pending error. */
#define EXPECT(vm, call, want)                                                                         \
    do {                                                                                               \
        ml_status got_ = (call);                                                                       \
        check(got_ == (want), #call " == " #want, __LINE__);                                           \
        if (got_ != (want))                                                                            \
            printf("    got %s: %s: %s\n", ml_status_name(got_), ml_error_type(vm), ml_error_message(vm)); \
    } while (0)
#define OK(vm, call) EXPECT(vm, call, ML_OK)
#define CHECK_STR(vm, h, want) check_str(vm, h, want, __LINE__)

static void check_str(ml_vm *vm, ml_value h, const char *want, int line) {
    const char *s = NULL;
    size_t n = 0;
    ml_status st = ml_get_string(vm, h, &s, &n);
    int ok = st == ML_OK && n == strlen(want) && memcmp(s, want, n) == 0;
    check(ok, want, line);
    if (!ok) printf("    got %s \"%s\"\n", ml_status_name(st), st == ML_OK ? s : "");
}

static ml_vm *open_vm(int misuse_returns) {
    ml_options o;
    memset(&o, 0, sizeof o);
    o.gc_stress = stress;
    o.gc_min_heap = min_heap;
    o.misuse_returns = misuse_returns;
    return ml_open(&o);
}
static void close_vm(ml_vm *vm) {
    size_t left = ml_close(vm);
    check(left == 0, "ml_close() returns 0 bytes left", __LINE__);
}
static ml_status load_path(ml_vm *vm, const char *name, ml_value *prog) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s.mlc", dir, name);
    return ml_load_file(vm, path, prog);
}
/* Load and run a test program; returns its trailing value. */
static ml_value run_file(ml_vm *vm, const char *name) {
    ml_value prog, result = ML_NO_VALUE;
    ml_status st = load_path(vm, name, &prog);
    if (st != ML_OK) {
        printf("cannot load %s/%s.mlc: %s\n", dir, name, ml_error_message(vm));
        exit(2);
    }
    OK(vm, ml_run(vm, prog, &result));
    return result;
}
static ml_value global(ml_vm *vm, const char *name) {
    ml_value h = ML_NO_VALUE;
    OK(vm, ml_get_global(vm, name, &h));
    return h;
}
static ml_value integer(ml_vm *vm, int64_t i) {
    ml_value h;
    ml_new_int(vm, i, &h);
    return h;
}
static ml_value string(ml_vm *vm, const char *s) {
    ml_value h;
    ml_new_string(vm, s, strlen(s), &h);
    return h;
}
static ml_status call1(ml_vm *vm, const char *fn, ml_value arg, ml_value *result) {
    return ml_call(vm, global(vm, fn), 1, &arg, result);
}
static int64_t get_int(ml_vm *vm, ml_value h) {
    int64_t i = -999;
    OK(vm, ml_get_int(vm, h, &i));
    return i;
}

/* ------------------------------------------------------- host functions */

static ml_status host_double(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)ud;
    int64_t i;
    if (ml_typeof(vm, args[0]) == ML_TINT && ml_get_int(vm, args[0], &i) == ML_OK) return ml_new_int(vm, 2 * i, result);
    double d;
    ml_status st = ml_get_float(vm, args[0], &d); /* a string: ML_TYPE_ERROR, which propagates */
    if (st != ML_OK) return st;
    return ml_new_float(vm, 2 * d, result);
}
static ml_status host_sum(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)ud;
    int64_t total = 0;
    for (int i = 0; i < nargs; i++) {
        int64_t x;
        ml_status st = ml_get_int(vm, args[i], &x);
        if (st != ML_OK) return st;
        total += x;
    }
    return ml_new_int(vm, total, result);
}
static ml_status host_raise(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)result, (void)ud;
    int64_t i = 0;
    ml_get_int(vm, args[0], &i);
    return ml_throwf(vm, "bad input %lld", (long long)i);
}
static ml_status host_throw_value(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)result, (void)ud;
    return ml_throw(vm, args[0]);
}
static ml_status host_no_error(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)vm, (void)nargs, (void)args, (void)result, (void)ud;
    return ML_EXCEPTION; /* a bug: failure without a pending error */
}
static ml_value stash;
static ml_status host_stash(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)vm, (void)nargs, (void)result, (void)ud;
    stash = args[0]; /* a bug: the handle dies when this call returns */
    return ML_OK;
}
static ml_value kept;
static ml_status host_keep(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)result, (void)ud;
    return ml_pin(vm, args[0], &kept); /* the right way */
}
static ml_status host_bad_result(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)ud;
    ml_value tmp;
    ml_new_int(vm, 1, &tmp);
    ml_release(vm, tmp);
    *result = tmp; /* a bug: stale */
    (void)args;
    return ML_OK;
}
static ml_status host_leave_scope(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)args, (void)result, (void)ud;
    ml_scope_open(vm); /* a bug: never closed */
    return ML_OK;
}
static ml_status apply(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)ud;
    return ml_call(vm, args[0], 1, &args[1], result); /* an error propagates as it is */
}
static ml_status apply_or(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)ud;
    if (ml_call(vm, args[0], 1, &args[1], result) == ML_OK) return ML_OK;
    ml_clear_error(vm); /* handled in C */
    *result = args[2];
    return ML_OK;
}
static ml_value side_program;
static ml_status run_side(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)args, (void)ud;
    return ml_run(vm, side_program, result);
}

static void register_all(ml_vm *vm) {
    OK(vm, ml_register(vm, "host_double", host_double, 1, NULL));
    OK(vm, ml_register(vm, "host_sum", host_sum, -1, NULL));
    OK(vm, ml_register(vm, "host_raise", host_raise, 1, NULL));
    OK(vm, ml_register(vm, "host_throw_value", host_throw_value, 1, NULL));
    OK(vm, ml_register(vm, "host_no_error", host_no_error, 1, NULL));
    OK(vm, ml_register(vm, "host_stash", host_stash, 1, NULL));
    OK(vm, ml_register(vm, "host_keep", host_keep, 1, NULL));
    OK(vm, ml_register(vm, "host_bad_result", host_bad_result, 1, NULL));
    OK(vm, ml_register(vm, "host_leave_scope", host_leave_scope, 1, NULL));
    OK(vm, ml_register(vm, "apply", apply, 2, NULL));
    OK(vm, ml_register(vm, "apply_or", apply_or, 3, NULL));
    OK(vm, ml_register(vm, "run_side", run_side, 0, NULL));
}

/* Output captured from print(). */
static char printed[4096];
static void capture(void *ud, const char *s, size_t n) {
    (void)ud;
    size_t used = strlen(printed);
    if (used + n + 2 < sizeof printed) {
        memcpy(printed + used, s, n);
        printed[used + n] = '\n';
        printed[used + n + 1] = 0;
    }
}

/* ---------------------------------------------------------------- tests */

static void test_lifecycle(void) {
    CHECK(ml_api_version() == MINILANG_API_VERSION);
    ml_vm *vm = open_vm(0);
    ml_value r = run_file(vm, "api");
    CHECK_STR(vm, r, "loaded");
    ml_stats st;
    ml_get_stats(vm, &st);
    CHECK(st.live_objects > 100);
    if (stress) CHECK(st.collections > 100); /* loading alone allocates hundreds of objects */
    close_vm(vm);
    vm = ml_open(NULL); /* defaults */
    close_vm(vm);
}

static void test_values(void) {
    ml_vm *vm = open_vm(0);
    run_file(vm, "api");
    ml_value echo = global(vm, "echo"), v, r;
    int b;
    int64_t i;
    double d;

    OK(vm, ml_new_nil(vm, &v));
    OK(vm, ml_call(vm, echo, 1, &v, &r));
    CHECK(ml_typeof(vm, r) == ML_TNIL);
    CHECK(!ml_truthy(vm, r));
    OK(vm, ml_new_bool(vm, 7, &v));
    OK(vm, ml_call(vm, echo, 1, &v, &r));
    OK(vm, ml_get_bool(vm, r, &b));
    CHECK(b == 1 && ml_typeof(vm, r) == ML_TBOOL);
    const int64_t ints[] = {0, -1, 42, INT64_MAX, INT64_MIN};
    for (int k = 0; k < 5; k++) {
        OK(vm, ml_call(vm, echo, 1, (v = integer(vm, ints[k]), &v), &r));
        CHECK(ml_typeof(vm, r) == ML_TINT && get_int(vm, r) == ints[k]);
    }
    const double floats[] = {1.5, -0.0, 1e300, INFINITY};
    for (int k = 0; k < 4; k++) {
        OK(vm, ml_new_float(vm, floats[k], &v));
        OK(vm, ml_call(vm, echo, 1, &v, &r));
        OK(vm, ml_get_float(vm, r, &d));
        CHECK(ml_typeof(vm, r) == ML_TFLOAT && d == floats[k] && signbit(d) == signbit(floats[k]));
    }
    OK(vm, ml_new_float(vm, NAN, &v));
    OK(vm, ml_call(vm, echo, 1, &v, &r));
    OK(vm, ml_get_float(vm, r, &d));
    CHECK(isnan(d));
    CHECK(ml_equal(vm, integer(vm, 3), (ml_new_float(vm, 3.0, &v), v))); /* 3 == 3.0 in minilang */
    CHECK(!ml_equal(vm, integer(vm, 1), (ml_new_bool(vm, 1, &v), v)));  /* 1 != true */

    /* minilang's view of host values: type() and str() */
    struct { int kind; const char *want; } cases[] = {
        {0, "nil:nil"}, {1, "bool:true"}, {2, "number:-7"}, {3, "number:2.5"}, {4, "number:3"},
        {5, "string:h\xc3\xa9llo \"q\""}, {6, "list:[1, x, [nil]]"}};
    for (unsigned k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        switch (cases[k].kind) {
        case 0: ml_new_nil(vm, &v); break;
        case 1: ml_new_bool(vm, 1, &v); break;
        case 2: v = integer(vm, -7); break;
        case 3: ml_new_float(vm, 2.5, &v); break;
        case 4: ml_new_float(vm, 3.0, &v); break; /* integral floats print as integers */
        case 5: v = string(vm, "h\xc3\xa9llo \"q\""); break;
        default: {
            ml_value inner, nil;
            ml_new_list(vm, &v);
            ml_list_push(vm, v, integer(vm, 1));
            ml_list_push(vm, v, string(vm, "x"));
            ml_new_list(vm, &inner);
            ml_new_nil(vm, &nil);
            ml_list_push(vm, inner, nil);
            ml_list_push(vm, v, inner);
        }
        }
        OK(vm, call1(vm, "describe", v, &r));
        CHECK_STR(vm, r, cases[k].want);
    }

    /* strings: UTF-8 is checked, length is in bytes, NUL bytes are kept */
    EXPECT(vm, ml_new_string(vm, "\xff", 1, &v), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "string is not valid UTF-8") == 0);
    OK(vm, ml_new_string(vm, "a\0b", 3, &v));
    const char *s;
    size_t n;
    OK(vm, ml_get_string(vm, v, &s, &n));
    CHECK(n == 3 && s[1] == 0 && s[2] == 'b' && s[3] == 0);
    ml_value lenfn = global(vm, "len");
    OK(vm, ml_call(vm, lenfn, 1, &v, &r));
    CHECK(get_int(vm, r) == 3);
    OK(vm, ml_call(vm, lenfn, 1, (v = string(vm, "h\xc3\xa9"), &v), &r));
    CHECK(get_int(vm, r) == 2); /* code points */

    /* wrong types are ML_TYPE_ERROR with a pending exception */
    EXPECT(vm, ml_get_int(vm, string(vm, "7"), &i), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "expected an integer, got string") == 0);
    CHECK(strcmp(ml_error_type(vm), "RuntimeErr") == 0);
    EXPECT(vm, ml_get_int(vm, (ml_new_float(vm, 2.0, &v), v), &i), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "expected an integer, got float") == 0);
    EXPECT(vm, ml_get_bool(vm, integer(vm, 1), &b), ML_TYPE_ERROR);
    EXPECT(vm, ml_get_string(vm, integer(vm, 1), &s, &n), ML_TYPE_ERROR);
    EXPECT(vm, ml_get_float(vm, string(vm, "x"), &d), ML_TYPE_ERROR);

    /* integers beyond 64 bits */
    OK(vm, call1(vm, "pow2", integer(vm, 70), &r));
    CHECK(ml_typeof(vm, r) == ML_TINT);
    EXPECT(vm, ml_get_int(vm, r, &i), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "integer does not fit in 64 bits") == 0);
    OK(vm, ml_get_float(vm, r, &d));
    CHECK(d == 1180591620717411303424.0);
    OK(vm, ml_tostring(vm, r, &v));
    CHECK_STR(vm, v, "1180591620717411303424");
    OK(vm, call1(vm, "pow2", integer(vm, 1100), &r));
    EXPECT(vm, ml_get_float(vm, r, &d), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "OverflowError") == 0);
    OK(vm, call1(vm, "pow2", integer(vm, 63), &r));
    EXPECT(vm, ml_get_int(vm, r, &i), ML_TYPE_ERROR); /* 2**63 is one past INT64_MAX */
    close_vm(vm);
}

static void test_lists(void) {
    ml_vm *vm = open_vm(0);
    run_file(vm, "api");
    ml_value list, r, v;
    size_t n;
    OK(vm, ml_new_list(vm, &list));
    for (int k = 1; k <= 100; k++) OK(vm, ml_list_push(vm, list, integer(vm, k)));
    OK(vm, ml_list_len(vm, list, &n));
    CHECK(n == 100);
    OK(vm, call1(vm, "sum", list, &r));
    CHECK(get_int(vm, r) == 5050);
    OK(vm, ml_list_set(vm, list, 0, integer(vm, 1001)));
    OK(vm, ml_list_get(vm, list, 0, &v));
    CHECK(get_int(vm, v) == 1001);
    EXPECT(vm, ml_list_get(vm, list, 100, &v), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "index 100 out of range for length 100") == 0);
    EXPECT(vm, ml_list_set(vm, list, 100, v), ML_TYPE_ERROR);
    EXPECT(vm, ml_list_push(vm, integer(vm, 1), v), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "expected a list, got integer") == 0);
    /* a list made by minilang, read in C; the same object both ways */
    OK(vm, call1(vm, "echo", list, &r));
    CHECK(ml_equal(vm, r, list));
    OK(vm, ml_list_push(vm, r, string(vm, "shared")));
    OK(vm, ml_list_len(vm, list, &n));
    CHECK(n == 101);
    close_vm(vm);
}

static void test_globals_and_vms(void) {
    ml_vm *a = open_vm(0), *b = open_vm(0);
    run_file(a, "api");
    run_file(b, "api");
    ml_value v, r;
    EXPECT(a, ml_get_global(a, "no_such_global", &v), ML_NOT_FOUND);
    CHECK(strcmp(ml_error_message(a), "undefined variable 'no_such_global'") == 0);
    OK(a, ml_set_global(a, "setting", string(a, "from C")));
    OK(a, ml_call(a, global(a, "read_setting"), 0, NULL, &r));
    CHECK_STR(a, r, "from C");
    OK(b, ml_call(b, global(b, "read_setting"), 0, NULL, &r)); /* VMs share nothing */
    CHECK_STR(b, r, "default");
    OK(a, ml_set_global(a, "brand_new", integer(a, 5)));
    OK(a, ml_get_global(a, "brand_new", &v));
    CHECK(get_int(a, v) == 5);
    EXPECT(b, ml_get_global(b, "brand_new", &v), ML_NOT_FOUND);
    EXPECT(a, ml_set_global(a, "\xc0", v), ML_TYPE_ERROR);
    /* running a program again keeps the globals of the first run */
    ml_value prog;
    OK(a, load_path(a, "api", &prog));
    OK(a, ml_run(a, prog, NULL));
    OK(a, ml_call(a, global(a, "read_setting"), 0, NULL, &r));
    CHECK_STR(a, r, "default"); /* the script's let ran again */
    close_vm(a);
    close_vm(b);
}

static void test_calls(void) {
    ml_vm *vm = open_vm(0);
    run_file(vm, "api");
    ml_value args[2], r, counter;
    args[0] = integer(vm, 2);
    args[1] = integer(vm, 3);
    OK(vm, ml_call(vm, global(vm, "add"), 2, args, &r));
    CHECK(get_int(vm, r) == 5);
    EXPECT(vm, ml_call(vm, global(vm, "add"), 1, args, &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "<fn add> expects 2 argument(s), got 1") == 0);
    CHECK(ml_error_line(vm) == 0); /* raised before any minilang line ran */
    EXPECT(vm, ml_call(vm, args[0], 0, NULL, &r), ML_TYPE_ERROR);
    CHECK(strcmp(ml_error_message(vm), "cannot call a number") == 0);
    /* builtins, including print with the output captured */
    printed[0] = 0;
    ml_set_output(vm, capture, NULL);
    args[0] = string(vm, "hi");
    ml_new_float(vm, 0.1, &args[1]);
    OK(vm, ml_call(vm, global(vm, "print"), 2, args, &r));
    CHECK(strcmp(printed, "hi 0.1\n") == 0);
    EXPECT(vm, ml_call(vm, global(vm, "len"), 2, args, &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "len() expects 1 argument(s), got 2") == 0);
    /* closures keep their state between calls from C */
    OK(vm, ml_call(vm, global(vm, "make_counter"), 0, NULL, &counter));
    CHECK(ml_typeof(vm, counter) == ML_TFUNCTION);
    for (int k = 1; k <= 3; k++) {
        OK(vm, ml_call(vm, counter, 0, NULL, &r));
        CHECK(get_int(vm, r) == k);
    }
    close_vm(vm);
}

static void test_minilang_errors(void) {
    ml_vm *vm = open_vm(0);
    run_file(vm, "api");
    ml_value r = ML_NO_VALUE, v;
    size_t n;
    EXPECT(vm, call1(vm, "fail", string(vm, "boom"), &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_type(vm), "RuntimeErr") == 0);
    CHECK(strcmp(ml_error_message(vm), "boom") == 0);
    CHECK(ml_error_line(vm) == 5);
    OK(vm, ml_error_value(vm, &v));
    CHECK_STR(vm, v, "boom");
    EXPECT(vm, ml_call(vm, global(vm, "fail_value"), 0, NULL, &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "[1, 2]") == 0);
    OK(vm, ml_error_value(vm, &v));
    OK(vm, ml_list_len(vm, v, &n)); /* the thrown list itself, not its text */
    CHECK(n == 2);
    ml_value args[2] = {integer(vm, 1), integer(vm, 0)};
    EXPECT(vm, ml_call(vm, global(vm, "divide"), 2, args, &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "division by zero") == 0);
    CHECK(ml_error_line(vm) == 9);
    OK(vm, ml_error_value(vm, &v)); /* a runtime error is caught as its message */
    CHECK_STR(vm, v, "division by zero");
    EXPECT(vm, ml_call(vm, global(vm, "huge_plus_half"), 0, NULL, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "OverflowError") == 0);
    CHECK(strcmp(ml_error_message(vm), "int too large to convert to float") == 0);
    EXPECT(vm, ml_call(vm, global(vm, "breaker"), 0, NULL, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "BreakSignal") == 0);
    EXPECT(vm, call1(vm, "deep", integer(vm, 0), &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "stack overflow (max call depth 400)") == 0);
    /* a script's top-level return reaches the host as ReturnSignal, with the value */
    ml_value prog;
    OK(vm, load_path(vm, "toplevel_return", &prog));
    EXPECT(vm, ml_run(vm, prog, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "ReturnSignal") == 0);
    OK(vm, ml_error_value(vm, &v));
    OK(vm, ml_list_len(vm, v, &n));
    CHECK(n == 3);
    ml_clear_error(vm);
    CHECK(strcmp(ml_error_message(vm), "") == 0 && strcmp(ml_error_type(vm), "") == 0);
    EXPECT(vm, ml_error_value(vm, &v), ML_NOT_FOUND);
    /* the VM keeps working after every one of these */
    OK(vm, ml_call(vm, global(vm, "add"), 2, (args[0] = integer(vm, 1), args), &r));
    CHECK(get_int(vm, r) == 1);
    /* load errors */
    EXPECT(vm, ml_load(vm, "not bytecode", 12, &prog), ML_LOAD_ERROR);
    CHECK(strcmp(ml_error_type(vm), "LoadError") == 0);
    CHECK(strcmp(ml_error_message(vm), "not a minilang bytecode file") == 0);
    EXPECT(vm, ml_load(vm, "MLBC\x01\x00", 6, &prog), ML_LOAD_ERROR);
    CHECK(strcmp(ml_error_message(vm), "truncated bytecode file") == 0);
    EXPECT(vm, load_path(vm, "no_such_file", &prog), ML_LOAD_ERROR);
    CHECK(strncmp(ml_error_message(vm), "cannot read ", 12) == 0);
    EXPECT(vm, ml_run(vm, integer(vm, 1), &r), ML_TYPE_ERROR);
    close_vm(vm);
}

static void test_host_functions(void) {
    ml_vm *vm = open_vm(1);
    register_all(vm);
    run_file(vm, "api");
    ml_value r, args[3];
    OK(vm, call1(vm, "call_host_double", integer(vm, 20), &r));
    CHECK(get_int(vm, r) == 41);
    ml_value hd = global(vm, "host_double");
    CHECK(ml_typeof(vm, hd) == ML_TFUNCTION);
    OK(vm, call1(vm, "describe", hd, &r));
    CHECK_STR(vm, r, "function:<builtin host_double>");
    OK(vm, ml_call(vm, hd, 1, (args[0] = integer(vm, 4), args), &r)); /* C calls C through the VM */
    CHECK(get_int(vm, r) == 8);
    args[0] = integer(vm, 1);
    args[1] = integer(vm, 2);
    args[2] = integer(vm, 3);
    OK(vm, ml_call(vm, global(vm, "host_sum"), 3, args, &r));
    CHECK(get_int(vm, r) == 6);
    OK(vm, ml_call(vm, global(vm, "host_sum"), 0, NULL, &r));
    CHECK(get_int(vm, r) == 0);
    /* arity is checked like a builtin's */
    EXPECT(vm, ml_call(vm, hd, 2, args, &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "host_double() expects 1 argument(s), got 2") == 0);

    /* errors raised in C are minilang exceptions: catchable, with the line of the call */
    args[0] = global(vm, "host_raise");
    args[1] = integer(vm, 7);
    OK(vm, ml_call(vm, global(vm, "catch_call"), 2, args, &r));
    CHECK_STR(vm, r, "caught: bad input 7");
    args[0] = hd;
    args[1] = string(vm, "seven");
    OK(vm, ml_call(vm, global(vm, "catch_call"), 2, args, &r)); /* a failed conversion, propagated */
    CHECK_STR(vm, r, "caught: expected a number, got string");
    EXPECT(vm, call1(vm, "call_host_double", string(vm, "x"), &r), ML_EXCEPTION);
    CHECK(strcmp(ml_error_message(vm), "expected a number, got string") == 0);
    CHECK(ml_error_line(vm) == 33);
    ml_new_list(vm, &args[1]);
    args[0] = global(vm, "host_throw_value");
    OK(vm, ml_call(vm, global(vm, "catch_identity"), 2, args, &r)); /* ml_throw: the same list arrives */
    CHECK(ml_typeof(vm, r) == ML_TBOOL && ml_truthy(vm, r));
    args[1] = integer(vm, 1);
    args[0] = global(vm, "host_raise");
    OK(vm, ml_call(vm, global(vm, "catch_call"), 2, args, &r));
    CHECK_STR(vm, r, "caught: bad input 1");

    /* host bugs become SystemError, which minilang cannot catch */
    args[0] = global(vm, "host_no_error");
    EXPECT(vm, ml_call(vm, global(vm, "catch_call"), 2, args, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "SystemError") == 0);
    CHECK(strcmp(ml_error_message(vm),
                 "host function 'host_no_error' returned ML_EXCEPTION without setting an error") == 0);
    args[0] = global(vm, "host_bad_result");
    EXPECT(vm, ml_call(vm, global(vm, "catch_call"), 2, args, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "SystemError") == 0);
    CHECK(strstr(ml_error_message(vm), "stale or invalid handle") != NULL);
    args[0] = global(vm, "host_leave_scope");
    EXPECT(vm, ml_call(vm, global(vm, "catch_call"), 2, args, &r), ML_ERROR);
    CHECK(strcmp(ml_error_message(vm),
                 "host function 'host_leave_scope' returned with 1 handle scope(s) still open") == 0);
    /* a host function cannot close its caller's scope */
    int s = ml_scope_open(vm);
    OK(vm, ml_scope_close(vm, s, NULL));
    EXPECT(vm, ml_scope_close(vm, s, NULL), ML_MISUSE);
    close_vm(vm);
}

static void test_reentrancy(void) {
    ml_vm *vm = open_vm(1);
    register_all(vm);
    run_file(vm, "api");
    ml_value r, prog;
    /* minilang -> C -> minilang -> C -> minilang */
    OK(vm, call1(vm, "nested", integer(vm, 5), &r));
    CHECK(get_int(vm, r) == 11);
    /* an exception thrown three levels down crosses two C frames to a minilang catch */
    OK(vm, call1(vm, "nested_throw", integer(vm, 4), &r));
    CHECK_STR(vm, r, "outer caught: inner 4");
    /* C handles an error itself and returns a fallback */
    OK(vm, call1(vm, "fallback", integer(vm, 0), &r));
    CHECK_STR(vm, r, "fallback");
    OK(vm, call1(vm, "fallback", integer(vm, 5), &r));
    CHECK(get_int(vm, r) == 2);
    /* recursion through C stops at the host depth limit, catchably */
    OK(vm, ml_call(vm, global(vm, "catch_rec"), 0, NULL, &r));
    CHECK_STR(vm, r, "stack overflow (max host call depth 200)");
    /* the inner call grows (moves) the stack; the waiting frame's locals survive */
    OK(vm, ml_call(vm, global(vm, "keeps_locals"), 0, NULL, &r));
    CHECK_STR(vm, r, "kept 300");
    /* break does not cross C: it is an uncatchable BreakSignal from there on */
    EXPECT(vm, ml_call(vm, global(vm, "break_through_c"), 0, NULL, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "BreakSignal") == 0);
    /* a program run from inside a host function, inside a minilang call */
    OK(vm, load_path(vm, "side", &side_program));
    OK(vm, ml_pin(vm, side_program, &side_program));
    OK(vm, ml_set_global(vm, "side_runs", integer(vm, 0)));
    OK(vm, ml_call(vm, global(vm, "run_side_from_minilang"), 0, NULL, &r)); /* minilang -> C -> ml_run */
    CHECK(get_int(vm, r) == 101);
    OK(vm, ml_call(vm, global(vm, "run_side"), 0, NULL, &r)); /* C -> C -> ml_run */
    CHECK(get_int(vm, r) == 200);
    ml_value runs;
    OK(vm, ml_get_global(vm, "side_runs", &runs));
    CHECK(get_int(vm, runs) == 2);
    ml_release(vm, side_program);
    /* the VM state is clean afterwards */
    OK(vm, load_path(vm, "api", &prog));
    OK(vm, ml_run(vm, prog, &r));
    CHECK_STR(vm, r, "loaded");
    close_vm(vm);
}

static void test_handles(void) {
    ml_vm *vm = open_vm(1); /* misuse returns ML_MISUSE instead of aborting */
    register_all(vm);
    run_file(vm, "api");
    ml_stats st;
    ml_value v, w, r;
    int64_t i;
    ml_get_stats(vm, &st);
    size_t base = st.handles;

    /* released and scoped-out handles are detected, never read */
    v = integer(vm, 1);
    OK(vm, ml_release(vm, v));
    EXPECT(vm, ml_get_int(vm, v, &i), ML_MISUSE);
    CHECK(strcmp(ml_error_type(vm), "SystemError") == 0);
    CHECK(ml_typeof(vm, v) == ML_TINVALID);
    EXPECT(vm, ml_release(vm, v), ML_MISUSE); /* twice */
    w = integer(vm, 2);                        /* reuses the slot with a new generation */
    CHECK(w.id == v.id && w.gen != v.gen);
    EXPECT(vm, ml_get_int(vm, v, &i), ML_MISUSE);
    CHECK(get_int(vm, w) == 2);
    CHECK(ml_typeof(vm, ML_NO_VALUE) == ML_TINVALID);
    int s = ml_scope_open(vm);
    for (int k = 0; k < 50; k++) v = string(vm, "temporary");
    ml_get_stats(vm, &st);
    CHECK(st.handles == base + 1 + 50);
    OK(vm, ml_scope_close(vm, s, &v)); /* keep the last one */
    ml_get_stats(vm, &st);
    CHECK(st.handles == base + 2);
    CHECK_STR(vm, v, "temporary");
    s = ml_scope_open(vm);
    w = string(vm, "gone");
    OK(vm, ml_scope_close(vm, s, NULL));
    EXPECT(vm, ml_get_string(vm, w, NULL, NULL), ML_MISUSE);
    int outer = ml_scope_open(vm), inner = ml_scope_open(vm);
    EXPECT(vm, ml_scope_close(vm, outer, NULL), ML_MISUSE); /* not the innermost */
    OK(vm, ml_scope_close(vm, inner, NULL));
    OK(vm, ml_scope_close(vm, outer, NULL));
    /* a handle kept from a host call's arguments dies with the call */
    OK(vm, call1(vm, "host_stash", string(vm, "argument"), &r));
    EXPECT(vm, ml_get_string(vm, stash, NULL, NULL), ML_MISUSE);
    /* a pinned handle survives scopes and collections */
    s = ml_scope_open(vm);
    OK(vm, call1(vm, "host_keep", string(vm, "pinned value"), &r));
    OK(vm, ml_scope_close(vm, s, NULL));
    OK(vm, ml_collect(vm));
    for (int k = 0; k < 20; k++) OK(vm, call1(vm, "pow2", integer(vm, 200), &r)); /* make garbage */
    OK(vm, ml_collect(vm));
    CHECK_STR(vm, kept, "pinned value");
    ml_get_stats(vm, &st);
    CHECK(st.pinned == 1);
    /* releasing the last handle frees the object at the next collection */
    ml_value big;
    s = ml_scope_open(vm);
    OK(vm, ml_new_list(vm, &big));
    for (int k = 0; k < 1000; k++) OK(vm, ml_list_push(vm, big, string(vm, "x")));
    OK(vm, ml_scope_close(vm, s, &big));
    OK(vm, ml_collect(vm));
    ml_stats with;
    ml_get_stats(vm, &with);
    OK(vm, ml_release(vm, kept));
    OK(vm, ml_release(vm, big));
    OK(vm, ml_collect(vm));
    ml_get_stats(vm, &st);
    CHECK(st.pinned == 0);
    CHECK(with.live_objects - st.live_objects == 1002); /* the list, its 1,000 strings, the pinned string */
    close_vm(vm);
}

/* Values held only through handles survive many collections and garbage. */
static void test_rooting(void) {
    ml_vm *vm = open_vm(0);
    run_file(vm, "api");
    enum { N = 300 };
    ml_value strings[N], list, r;
    char text[32];
    OK(vm, ml_new_list(vm, &list));
    for (int k = 0; k < N; k++) {
        snprintf(text, sizeof text, "value %d", k);
        strings[k] = string(vm, text);
        int s = ml_scope_open(vm);
        ml_value tmp;
        OK(vm, ml_tostring(vm, integer(vm, k), &tmp));
        OK(vm, ml_list_push(vm, list, tmp)); /* tmp's handle dies; the list keeps it */
        OK(vm, ml_scope_close(vm, s, NULL));
        if (k % 50 == 0) {
            OK(vm, call1(vm, "pow2", integer(vm, 300), &r));
            OK(vm, ml_collect(vm));
        }
    }
    OK(vm, ml_collect(vm));
    int ok = 1;
    for (int k = 0; k < N; k++) {
        const char *s;
        size_t n;
        ml_value item, str;
        snprintf(text, sizeof text, "value %d", k);
        ok &= ml_get_string(vm, strings[k], &s, &n) == ML_OK && strcmp(s, text) == 0;
        snprintf(text, sizeof text, "%d", k);
        ok &= ml_list_get(vm, list, (size_t)k, &item) == ML_OK && ml_get_string(vm, item, &s, &n) == ML_OK &&
              strcmp(s, text) == 0;
        ok &= ml_tostring(vm, item, &str) == ML_OK;
    }
    CHECK(ok);
    close_vm(vm);
}

static void test_heap_limit(void) {
    ml_options o;
    memset(&o, 0, sizeof o);
    o.gc_stress = stress;
    o.heap_limit = 256 * 1024;
    o.misuse_returns = 1;
    ml_vm *vm = ml_open(&o);
    run_file(vm, "api");
    ml_value r, v, args[2];
    ml_stats st;
    EXPECT(vm, ml_call(vm, global(vm, "hog"), 0, NULL, &r), ML_NOMEM);
    CHECK(strcmp(ml_error_type(vm), "MemoryError") == 0);
    ml_get_stats(vm, &st);
    CHECK(st.peak_bytes < 2 * o.heap_limit);
    EXPECT(vm, ml_call(vm, global(vm, "try_hog"), 0, NULL, &r), ML_NOMEM); /* try/catch cannot catch it */
    ml_value empty;
    OK(vm, ml_new_list(vm, &empty));
    EXPECT(vm, call1(vm, "hog_recursive", empty, &r), ML_NOMEM); /* no loop: caught at a call */
    /* the VM is usable afterwards, and the garbage goes */
    args[0] = integer(vm, 2);
    args[1] = integer(vm, 3);
    OK(vm, ml_call(vm, global(vm, "add"), 2, args, &r));
    CHECK(get_int(vm, r) == 5);
    OK(vm, ml_collect(vm));
    ml_get_stats(vm, &st);
    CHECK(st.heap_bytes < o.heap_limit);
    /* the host's own allocations are limited too */
    size_t big = 512 * 1024;
    char *text = malloc(big);
    memset(text, 'a', big);
    EXPECT(vm, ml_new_string(vm, text, big, &v), ML_NOMEM);
    free(text);
    OK(vm, ml_new_string(vm, "small", 5, &v));
    close_vm(vm);
}

static ml_vm *output_vm;
static ml_status output_status;
static void reentrant_output(void *ud, const char *s, size_t n) {
    (void)ud, (void)s, (void)n;
    ml_value v;
    output_status = ml_new_int(output_vm, 1, &v); /* not allowed here */
}
static void test_output_callback(void) {
    ml_vm *vm = open_vm(1);
    output_vm = vm;
    run_file(vm, "api");
    ml_set_output(vm, reentrant_output, NULL);
    ml_value r, v = string(vm, "x");
    output_status = ML_OK;
    OK(vm, ml_call(vm, global(vm, "print"), 1, &v, &r));
    CHECK(output_status == ML_MISUSE);
    ml_set_output(vm, NULL, NULL);
    close_vm(vm);
}

/* Function literals nested 63, 64, 200 and 201 deep (test_minilang_embed.py
 * writes nestN.mlc): the loader keeps one temporary root per level, which
 * once overflowed a fixed stack of 64 and killed the process. Up to 200
 * levels load and run; 201 is a load error. A file that reads a non-cell
 * slot as a cell (badcell.mlc) ends the run, not the process. */
static void test_nesting_and_bad_bytecode(void) {
    ml_vm *vm = open_vm(0);
    ml_set_output(vm, capture, NULL);
    const int levels[] = {63, 64, 200};
    ml_value prog, r;
    for (int k = 0; k < 3; k++) {
        char name[16];
        snprintf(name, sizeof name, "nest%d", levels[k]);
        OK(vm, load_path(vm, name, &prog));
        OK(vm, ml_run(vm, prog, &r));
        CHECK(get_int(vm, r) == levels[k]);
    }
    EXPECT(vm, load_path(vm, "nest201", &prog), ML_LOAD_ERROR);
    CHECK(strcmp(ml_error_message(vm), "functions nested too deeply") == 0);
    OK(vm, load_path(vm, "badcell", &prog));
    EXPECT(vm, ml_run(vm, prog, &r), ML_ERROR);
    CHECK(strcmp(ml_error_type(vm), "SystemError") == 0);
    CHECK(strcmp(ml_error_message(vm), "bytecode reads a slot that holds no cell") == 0);
    OK(vm, load_path(vm, "nest64", &prog)); /* the VM is fine afterwards */
    OK(vm, ml_run(vm, prog, &r));
    CHECK(get_int(vm, r) == 64);
    close_vm(vm);
}

/* --crash: misuse with the default options must stop the program loudly. */
static ml_status host_close(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    (void)nargs, (void)args, (void)result, (void)ud;
    ml_close(vm);
    return ML_OK;
}
static int crash(const char *kind) {
    ml_vm *vm = open_vm(0);
    run_file(vm, "api");
    if (strcmp(kind, "stale") == 0) {
        ml_value v = integer(vm, 1);
        ml_release(vm, v);
        int64_t i;
        ml_get_int(vm, v, &i);
    } else if (strcmp(kind, "close") == 0) {
        ml_register(vm, "host_close", host_close, 0, NULL);
        ml_value r;
        ml_call(vm, global(vm, "host_close"), 0, NULL, &r);
    }
    printf("no abort\n");
    return 0;
}

int main(int argc, char **argv) {
    const char *crash_kind = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gc-stress") == 0) stress = 1;
        else if (strcmp(argv[i], "--gc-min") == 0 && i + 1 < argc) min_heap = (size_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--crash") == 0 && i + 1 < argc) crash_kind = argv[++i];
        else dir = argv[i];
    }
    if (crash_kind) return crash(crash_kind);
    struct { const char *name; void (*fn)(void); } tests[] = {
        {"lifecycle", test_lifecycle}, {"values", test_values}, {"lists", test_lists},
        {"globals and VMs", test_globals_and_vms}, {"calls", test_calls},
        {"minilang errors", test_minilang_errors}, {"host functions", test_host_functions},
        {"reentrancy", test_reentrancy}, {"handles", test_handles}, {"rooting", test_rooting},
        {"heap limit", test_heap_limit}, {"output callback", test_output_callback},
        {"nesting", test_nesting_and_bad_bytecode}};
    int ntests = (int)(sizeof tests / sizeof tests[0]), failed_tests = 0;
    for (int i = 0; i < ntests; i++) {
        int before = failures;
        tests[i].fn();
        if (failures != before) failed_tests++;
        printf("%-16s %s\n", tests[i].name, failures == before ? "ok" : "FAILED");
    }
    printf("%d tests, %d checks, %d failed checks%s\n", ntests, checks, failures,
           stress ? " (--gc-stress)" : min_heap ? " (small heap)" : "");
    return failed_tests ? 1 : 0;
}
