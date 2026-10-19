// Global data lives in .data, not on the stack; it survives across calls.
int counter;
int table[16];

int bump() {
    counter = counter + 1;
    return counter;
}

int fill() {
    for (int i = 0; i < 16; i = i + 1)
        table[i] = i * i;
    return 0;
}

int main() {
    bump(); bump(); bump();
    print_int(counter); print_char('\n');
    fill();
    print_int(table[0] + table[15]); print_char('\n');
    int *p = &table[3];
    *p = 1000;
    print_int(table[3]); print_char('\n');
    return counter + 4;   // exit status 7: observable from the shell
}
