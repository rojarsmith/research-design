// Loops and integer arithmetic: gcd, primes, and C's truncating division.
int gcd(int a, int b) {
    while (b != 0) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a;
}

int is_prime(int n) {
    if (n < 2) return 0;
    for (int d = 2; d * d <= n; d = d + 1)
        if (n % d == 0) return 0;
    return 1;
}

int main() {
    print_int(gcd(1071, 462)); print_char('\n');
    for (int n = 1; n < 30; n = n + 1)
        if (is_prime(n)) { print_int(n); print_char(' '); }
    print_char('\n');
    print_int(-7 / 2); print_char(' ');
    print_int(-7 % 2); print_char(' ');
    print_int(7 / -2); print_char(' ');
    print_int(7 % -2); print_char('\n');
    return 0;
}
