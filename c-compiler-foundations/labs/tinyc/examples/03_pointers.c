// Pointers and arrays: addresses are just integers the hardware can load from.
int swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
    return 0;
}

int sum(int *xs, int n) {
    int total = 0;
    for (int i = 0; i < n; i = i + 1)
        total = total + xs[i];
    return total;
}

int bubble_sort(int *xs, int n) {
    for (int i = 0; i < n - 1; i = i + 1)
        for (int j = 0; j < n - 1 - i; j = j + 1)
            if (xs[j] > xs[j + 1])
                swap(&xs[j], &xs[j + 1]);
    return 0;
}

int main() {
    int data[8];
    int i;
    for (i = 0; i < 8; i = i + 1)
        data[i] = (i * 7 + 3) % 10;
    bubble_sort(data, 8);
    for (i = 0; i < 8; i = i + 1) { print_int(data[i]); print_char(' '); }
    print_char('\n');
    print_int(sum(data, 8)); print_char('\n');

    int x = 1;
    int y = 2;
    swap(&x, &y);
    print_int(x); print_char(' '); print_int(y); print_char('\n');

    int *p = data;
    int *q = p + 5;
    print_int(*q); print_char(' ');
    print_int(q - p); print_char(' ');
    print_int(*(p + 2) == data[2]); print_char('\n');
    return 0;
}
