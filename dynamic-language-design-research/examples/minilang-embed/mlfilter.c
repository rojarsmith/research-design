/* mlfilter.c: a scripted data filter, the example host for the embedding API.
 *
 * Reads a CSV file, hands every data row to a minilang script's keep(row),
 * and calls the script's report(rows) with the rows it kept. The script
 * decides what to keep and how to print it; this program parses the file and
 * provides three host functions the language lacks: field(), sort_by(),
 * which calls back into minilang, and fmt(). A row whose keep() throws is
 * reported and skipped, and the run goes on.
 *
 *     gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlfilter mlfilter.c minilang.c -lm
 *     python3 ../minilang-vm/minilang_vm.py --emit report.mlc programs/report.ml
 *     ./mlfilter report.mlc data/orders.csv
 *     ./mlfilter --gc-stress report.mlc data/orders.csv     same output, collecting on every allocation
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minilang.h"

#define MAX_COLUMNS 32

typedef struct {
    int n;
    char *names[MAX_COLUMNS];
} Header;

/* ------------------------------------------------------- host functions */

/* field(row, name): the column's position comes from the CSV header, which
 * this function receives through its user data pointer. */
static ml_status field(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    const Header *header = ud;
    const char *name;
    (void)nargs;
    ml_status st = ml_get_string(vm, args[1], &name, NULL); /* a non-string fails, catchably */
    if (st != ML_OK) return st;
    for (int i = 0; i < header->n; i++)
        if (strcmp(header->names[i], name) == 0) return ml_list_get(vm, args[0], (size_t)i, result);
    return ml_throwf(vm, "no column named '%s'", name);
}

/* fmt(number, decimals): fixed-point text. */
static ml_status fmt(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    double x;
    int64_t decimals;
    char text[64];
    (void)nargs, (void)ud;
    ml_status st = ml_get_float(vm, args[0], &x);
    if (st == ML_OK) st = ml_get_int(vm, args[1], &decimals);
    if (st != ML_OK) return st;
    if (decimals < 0 || decimals > 20) return ml_throwf(vm, "fmt(): decimals must be 0 to 20");
    int n = snprintf(text, sizeof text, "%.*f", (int)decimals, x);
    if (n < 0 || n >= (int)sizeof text) return ml_throwf(vm, "fmt(): number too large");
    return ml_new_string(vm, text, (size_t)n, result);
}

/* sort_by(list, key_fn): a new list, stably sorted by keys that are all
 * numbers or all strings. Every key comes from calling key_fn, so minilang
 * runs inside this C function; if key_fn throws, the status is returned as
 * it is and the exception continues in the script. All handles made here
 * belong to this call's scope and are released when it returns; the string
 * keys' text stays valid until then because their handles are live. */
typedef struct {
    double num;
    const char *str;
    size_t index;
} Key;
static int by_number(const void *pa, const void *pb) {
    const Key *a = pa, *b = pb;
    if (a->num != b->num) return a->num < b->num ? -1 : 1;
    return a->index < b->index ? -1 : a->index > b->index;
}
static int by_string(const void *pa, const void *pb) {
    const Key *a = pa, *b = pb;
    int c = strcmp(a->str, b->str);
    if (c) return c;
    return a->index < b->index ? -1 : a->index > b->index;
}
static ml_status sort_by(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    size_t n;
    (void)nargs, (void)ud;
    ml_status st = ml_list_len(vm, args[0], &n);
    if (st != ML_OK) return st;
    Key *keys = calloc(n ? n : 1, sizeof *keys);
    ml_value *items = calloc(n ? n : 1, sizeof *items);
    if (!keys || !items) {
        free(keys);
        free(items);
        return ml_throwf(vm, "sort_by(): out of memory");
    }
    int strings = 0;
    for (size_t i = 0; i < n && st == ML_OK; i++) {
        ml_value key;
        st = ml_list_get(vm, args[0], i, &items[i]);
        if (st == ML_OK) st = ml_call(vm, args[1], 1, &items[i], &key); /* minilang runs here */
        if (st != ML_OK) break;
        ml_type t = ml_typeof(vm, key);
        if (i == 0) strings = t == ML_TSTRING;
        keys[i].index = i;
        if (strings && t == ML_TSTRING) st = ml_get_string(vm, key, &keys[i].str, NULL);
        else if (!strings && (t == ML_TINT || t == ML_TFLOAT)) st = ml_get_float(vm, key, &keys[i].num);
        else st = ml_throwf(vm, "sort_by(): keys must be all numbers or all strings");
        if (st == ML_OK && !strings && keys[i].num != keys[i].num) st = ml_throwf(vm, "sort_by(): a key is nan");
    }
    if (st == ML_OK) {
        qsort(keys, n, sizeof *keys, strings ? by_string : by_number);
        st = ml_new_list(vm, result);
        for (size_t i = 0; i < n && st == ML_OK; i++) st = ml_list_push(vm, *result, items[keys[i].index]);
    }
    free(keys);
    free(items);
    return st;
}

/* ------------------------------------------------------------- the file */

static int split(char *line, char **cells, int max) { /* no quoting: enough for the example */
    int n = 0;
    line[strcspn(line, "\r\n")] = 0;
    for (char *p = line;; p++) {
        if (n == max) return -1;
        cells[n++] = p;
        p = strchr(p, ',');
        if (!p) return n;
        *p = 0;
    }
}

/* -?digits is an integer, -?digits.digits a float, anything else a string. */
static ml_status cell_value(ml_vm *vm, const char *text, ml_value *out) {
    const char *p = text + (*text == '-');
    size_t digits = strspn(p, "0123456789");
    if (digits && p[digits] == 0) {
        errno = 0;
        long long i = strtoll(text, NULL, 10);
        if (errno == 0) return ml_new_int(vm, i, out);
    }
    if (digits && p[digits] == '.' && p[digits + 1] && strspn(p + digits + 1, "0123456789") == strlen(p + digits + 1))
        return ml_new_float(vm, strtod(text, NULL), out);
    return ml_new_string(vm, text, strlen(text), out);
}

static int script_failed(ml_vm *vm, const char *what) {
    fprintf(stderr, "mlfilter: %s: %s: %s", what, ml_error_type(vm), ml_error_message(vm));
    if (ml_error_line(vm)) fprintf(stderr, " (script line %d)", ml_error_line(vm));
    fputc('\n', stderr);
    return 1;
}

int main(int argc, char **argv) {
    ml_options opt;
    memset(&opt, 0, sizeof opt);
    int a = 1;
    if (a < argc && strcmp(argv[a], "--gc-stress") == 0) {
        opt.gc_stress = 1;
        a++;
    }
    if (argc - a != 2) {
        fputs("usage: mlfilter [--gc-stress] script.mlc data.csv\n", stderr);
        return 2;
    }
    const char *script = argv[a], *csv = argv[a + 1];
    FILE *in = fopen(csv, "r");
    if (!in) {
        fprintf(stderr, "mlfilter: cannot read %s\n", csv);
        return 2;
    }
    char line[4096], *cells[MAX_COLUMNS];
    Header header = {0, {0}};
    if (!fgets(line, sizeof line, in) || (header.n = split(line, cells, MAX_COLUMNS)) < 1) {
        fprintf(stderr, "mlfilter: %s: no header line\n", csv);
        fclose(in);
        return 2;
    }
    for (int i = 0; i < header.n; i++) header.names[i] = strcpy(malloc(strlen(cells[i]) + 1), cells[i]);

    ml_vm *vm = ml_open(&opt);
    int status = 0, rows = 0, kept_rows = 0, skipped = 0, lineno = 1;
    ml_value program, keep, report, kept, total;
    if (ml_register(vm, "field", field, 2, &header) || ml_register(vm, "sort_by", sort_by, 2, NULL) ||
        ml_register(vm, "fmt", fmt, 2, NULL)) {
        status = script_failed(vm, "register");
    } else if (ml_load_file(vm, script, &program) || ml_run(vm, program, NULL)) {
        status = script_failed(vm, script);
    } else if (ml_get_global(vm, "keep", &keep) || ml_get_global(vm, "report", &report)) {
        status = script_failed(vm, script);
    } else if (ml_new_list(vm, &kept)) {
        status = script_failed(vm, "list");
    }
    while (!status && fgets(line, sizeof line, in)) {
        lineno++;
        int n = split(line, cells, MAX_COLUMNS);
        if (n == 1 && cells[0][0] == 0) continue; /* blank line */
        rows++;
        if (n != header.n) {
            fprintf(stderr, "%s:%d: row skipped: %d fields, the header has %d\n", csv, lineno, n, header.n);
            skipped++;
            continue;
        }
        int scope = ml_scope_open(vm); /* this row's handles go when the scope closes */
        ml_value row, cell, verdict;
        ml_status st = ml_new_list(vm, &row);
        for (int i = 0; i < n && st == ML_OK; i++) {
            st = cell_value(vm, cells[i], &cell);
            if (st == ML_OK) st = ml_list_push(vm, row, cell);
        }
        if (st == ML_OK) st = ml_call(vm, keep, 1, &row, &verdict);
        if (st == ML_OK && ml_truthy(vm, verdict)) {
            st = ml_list_push(vm, kept, row); /* the list keeps the row alive after the scope */
            kept_rows++;
        } else if (st == ML_EXCEPTION) { /* the script rejected this row: report it and go on */
            fprintf(stderr, "%s:%d: row skipped: %s", csv, lineno, ml_error_message(vm));
            if (ml_error_line(vm)) fprintf(stderr, " (script line %d)", ml_error_line(vm));
            fputc('\n', stderr);
            skipped++;
            st = ML_OK;
        }
        if (st != ML_OK) status = script_failed(vm, "keep");
        ml_scope_close(vm, scope, NULL);
    }
    fclose(in);
    if (!status) {
        if (ml_call(vm, report, 1, &kept, &total) != ML_OK) status = script_failed(vm, "report");
        else fprintf(stderr, "mlfilter: %d rows, %d kept, %d skipped\n", rows, kept_rows, skipped);
    }
    fflush(stdout);
    size_t left = ml_close(vm);
    for (int i = 0; i < header.n; i++) free(header.names[i]);
    if (left) {
        fprintf(stderr, "mlfilter: heap accounting error: %lu bytes left\n", (unsigned long)left);
        return 3;
    }
    return status;
}
