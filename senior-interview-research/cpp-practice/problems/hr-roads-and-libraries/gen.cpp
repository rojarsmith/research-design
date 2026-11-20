// 產生 tests/big.in：10 組測資，每組 n = 10^5，路是一條鏈 1-2-3-…-n，第 j 組每隔 10^4·(j+1) 條斷開一次。
// 修路比蓋圖書館便宜一點，答案約 10^10，超過 int；最後一組是一整條 10^5 個城市的鏈，遞迴 DFS 會深達 10^5 層。
// 第一組之後的每一組都要重設狀態，否則上一組的邊會留下來。
#include <cstdio>
int main() {
    const int q = 10, n = 100000, lib = 100000;
    std::printf("%d\n", q);
    for (int j = 0; j < q; ++j) {
        int gap = 10000 * (j + 1), m = 0;
        for (int i = 1; i < n; ++i)
            if (i % gap != 0) ++m;
        std::printf("%d %d %d %d\n", n, m, lib, lib - 1 - j);
        for (int i = n - 1; i >= 1; --i)
            if (i % gap != 0) std::printf("%d %d\n", i, i + 1);
    }
}
