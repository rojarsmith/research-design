// 64-bit arithmetic on a 32-, 16- or 8-bit core: register pairs or libcalls?
long long add64(long long a, long long b) { return a + b; }
long long mul64(long long a, long long b) { return a * b; }
long long div64(long long a, long long b) { return a / b; }
long long shl64(long long a, int n) { return a << n; }
