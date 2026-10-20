// Floating point without an FPU is still C, just slower: look for __addsf3,
// __mulsf3, __divdf3, __aeabi_fadd and friends in the undefined symbols.
float fadd(float a, float b) { return a + b; }
float fmul(float a, float b) { return a * b; }
double ddiv(double a, double b) { return a / b; }
int f2i(float a) { return (int)a; }
double i2d(int a) { return (double)a; }
