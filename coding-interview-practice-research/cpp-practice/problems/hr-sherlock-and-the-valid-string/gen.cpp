// 產生 tests/big.in：26 個字母各 3845 次，再多一個 m，打亂順序，長度 99971。
// 刪掉一個 m 就全部相同，答案是 YES。每刪一個位置就重新計數一次的做法是 O(n²)，會超過時限。
#include <cstdio>
#include <random>
#include <string>
int main() {
    std::string s;
    for (char c = 'a'; c <= 'z'; ++c) s.append(3845, c);
    s += 'm';
    std::mt19937 g(20261115);
    for (size_t i = s.size() - 1; i > 0; --i) {
        size_t j = g() % (i + 1);
        char t = s[i];
        s[i] = s[j];
        s[j] = t;
    }
    std::printf("%s\n", s.c_str());
}
