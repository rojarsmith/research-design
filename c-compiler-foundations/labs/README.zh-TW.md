# 實作練習總覽

兩個實驗室，都不需要安裝額外套件；有主機 C 編譯器與含 riscv32 後端的 clang 時可以多跑兩條驗證路徑。

| 目錄 | 回答的問題 | 入口 |
| --- | --- | --- |
| `tinyc/` | 編譯器內部長什麼樣；C 程式如何變成 RISC-V 指令；不同編譯器與處理器為何得到相同結果 | [tinyc/README.md](tinyc/README.md)（英文技術說明） |
| `probe/` | 一個 C 實作對標準留白的地方做了什麼決定；如何有系統地評估一顆處理器 | [probe/checklist.zh-TW.md](probe/checklist.zh-TW.md) |

## 五分鐘開始

```sh
cd c-compiler-foundations/labs/tinyc
python3 run_tests.py
```

預期看到五個範例在三條路徑上 `PASS`，以及每條路徑的指令數與堆疊深度。接著：

```sh
python3 tinyc.py examples/01_fib.c --ast        # 看語法樹
python3 tinyc.py examples/01_fib.c              # 看產出的 RV32 組合語言
python3 tinyc.py examples/01_fib.c -o fib.s && python3 rv32sim.py fib.s --trace 2>&1 | head -30
```

`--trace` 每執行一條指令印一行，包含 `a0` 與 `sp`，適合對照 [04 融會貫通](../research/04-integration-path.zh-TW.md) 的契約堆疊圖，看函式呼叫時堆疊如何長出來又收回去。

## 探針

```sh
cd ../probe
cc -std=c11 probe.c -o probe && ./probe
```

把輸出貼進 [評估清單](probe/checklist.zh-TW.md) 的對應欄位。若手上有交叉編譯器與模擬器，用同一個檔案對目標處理器再跑一次，兩份輸出的差異就是你要在程式中處理的可攜性邊界。

## 讀程式碼的順序

1. `tinyc.py` 的 `lex()`：約 60 行，字元變 token。
2. `Parser` 類別：每個方法對應一條文法規則；`postfix()` 中可以看到 `a[i]` 被改寫為 `*(a + i)`。
3. `CodeGen.function()`：序曲與尾聲，對照 README 中的框架圖。
4. `CodeGen.expression()` 與 `binary()`：所有值都經過 `a0`，暫存值進堆疊；指標加法在這裡乘以 4。
5. `rv32sim.py` 的 `execute()`：每條指令一行語意，就是 ISA 規格書的可執行版本。
6. `runtime_rv32.s`：真正的程式從 `_start` 開始，`main` 只是被呼叫的函式。
