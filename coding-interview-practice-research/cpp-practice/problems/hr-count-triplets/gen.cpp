// 產生 tests/big.in：n = 100000、r = 1、全部是 1。答案是 C(100000, 3)，超過 int。
#include <cstdio>
int main() {
    const int n = 100000;
    std::printf("%d 1\n", n);
    for (int i = 0; i < n; ++i) std::printf("1%c", i + 1 < n ? ' ' : '\n');
}
