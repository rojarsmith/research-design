// Short-circuit evaluation and precedence, checked against a host compiler.
int calls;

int side(int v) {
    calls = calls + 1;
    return v;
}

int main() {
    calls = 0;
    int a = side(0) && side(1);     // right side must not run
    int b = side(1) || side(0);     // right side must not run
    int c = side(1) && side(2);     // both run, result is 1 not 2
    print_int(a); print_int(b); print_int(c); print_char(' ');
    print_int(calls); print_char('\n');

    print_int(2 + 3 * 4);  print_char(' ');
    print_int((2 + 3) * 4); print_char(' ');
    print_int(10 - 4 - 3); print_char(' ');
    print_int(!0 + !5);    print_char(' ');
    print_int(1 < 2 == 1); print_char(' ');
    print_int(-(-8) / 3);  print_char('\n');

    int n = 0;
    int fizz = 0;
    while (n < 100) {
        n = n + 1;
        if (n % 3 == 0 || n % 5 == 0)
            fizz = fizz + 1;
    }
    print_int(fizz); print_char('\n');
    return 0;
}
