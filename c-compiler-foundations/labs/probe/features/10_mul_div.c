// Does the target have hardware multiply and divide, or do these become
// library calls such as __mulsi3 / __divsi3 / __aeabi_idiv?
int mul(int a, int b) { return a * b; }
int div(int a, int b) { return a / b; }
int rem(int a, int b) { return a % b; }
unsigned udiv(unsigned a, unsigned b) { return a / b; }
