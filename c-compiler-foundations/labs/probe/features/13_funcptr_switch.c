// Indirect calls and jump tables need indirect jumps; recursion needs a
// real stack. Look at how the switch is lowered and how deep frames get.
typedef int (*op_t)(int, int);
static int add(int a, int b) { return a + b; }
static int sub(int a, int b) { return a - b; }
int apply(op_t f, int a, int b) { return f(a, b); }
op_t pick(int which) { return which ? add : sub; }

int classify(int x) {
    switch (x) {
    case 0: return 10; case 1: return 11; case 2: return 12; case 3: return 13;
    case 4: return 14; case 5: return 15; case 6: return 16; case 7: return 17;
    case 8: return 18; case 9: return 19; default: return -1;
    }
}

int depth(int n) { return n == 0 ? 0 : 1 + depth(n - 1); }
