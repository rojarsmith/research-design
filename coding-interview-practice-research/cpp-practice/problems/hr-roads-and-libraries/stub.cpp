// HackerRank: Roads and Libraries
// https://www.hackerrank.com/challenges/torque-and-development/problem
/* 解題紀錄：pp done 會檢查 [觸發] 與 [關鍵洞見]；重做時整份封存。一行一句，寫給下次的自己看。
[複述]
[限制→複雜度]
[暴力解]
[瓶頸]
[觸發]
[關鍵洞見]
[不變量]
[複雜度]
[卡在哪]
*/
// 輸入：第一行 q（測資組數）。每組第一行 n m c_lib c_road，接著 m 行 u v：城市 u、v 之間可以修一條路（從 1 起算）。
// 每個城市可以蓋圖書館（花 c_lib），或經由修好的路（每條花 c_road）連到一個有圖書館的城市。
// 每組輸出一行：讓每個城市都能到達圖書館的最小總花費。
#include <iostream>
#include <vector>
using namespace std;

int main(int argc, char** argv) {
    if (argc > 1 && !freopen(argv[1], "r", stdin)) return 2;  // 本機除錯（F5）時從檔案讀；HackerRank 不會傳參數
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // 讀入 q 組測資，每組輸出一行答案

    return 0;
}
