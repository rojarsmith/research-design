// 產生 tests/big.in：n = 10^7、m = 2*10^5，每個操作都蓋住整個陣列並加 10^9。答案 2*10^14，超過 int。
#include <cstdio>
int main() {
    const int n = 10000000, m = 200000;
    std::printf("%d %d\n", n, m);
    for (int i = 0; i < m; ++i) std::printf("1 %d 1000000000\n", n);
}
