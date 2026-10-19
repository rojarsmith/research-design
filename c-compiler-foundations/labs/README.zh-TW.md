# 實作練習總覽

兩個實驗室，都不需要安裝額外套件；有主機 C 編譯器與含 riscv32 後端的 clang 時可以多跑兩條驗證路徑。

| 目錄 | 回答的問題 | 入口 |
| --- | --- | --- |
| `tinyc/` | 編譯器內部長什麼樣；同一個前端如何接上 RV32 與 x86-64 兩個後端；不同編譯器與處理器為何得到相同結果 | [tinyc/README.md](tinyc/README.md)（英文技術說明） |
| `probe/` | 一個 C 實作對標準留白的地方做了什麼決定；如何有系統地評估一顆處理器；同一組程式在 19 種目標上的大小與硬體缺口 | [probe/checklist.zh-TW.md](probe/checklist.zh-TW.md)、[compare_targets.py](probe/compare_targets.py) |

## 五分鐘開始

```sh
cd c-compiler-foundations/labs/tinyc
python3 run_tests.py
```

預期看到五個範例在四條路徑上 `PASS`：tinyc → RV32 模擬器、主機 gcc 原生、tinyc 的 x86-64 輸出在主機 CPU 原生執行、clang 的 riscv32 輸出在模擬器執行。接著：

```sh
python3 tinyc.py examples/01_fib.c --ast        # 看語法樹
python3 tinyc.py examples/01_fib.c              # 看產出的 RV32 組合語言
python3 tinyc.py examples/01_fib.c --target x86_64   # 同一棵語法樹換成 x86-64
python3 tinyc.py examples/01_fib.c -o fib.s && python3 rv32sim.py fib.s --trace 2>&1 | head -30
```

`--trace` 每執行一條指令印一行，包含 `a0` 與 `sp`，適合對照 [04 融會貫通](../research/04-integration-path.zh-TW.md) 的契約堆疊圖，看函式呼叫時堆疊如何長出來又收回去。

## 探針

```sh
cd ../probe
cc -std=c11 probe.c -o probe && ./probe
```

把輸出貼進 [評估清單](probe/checklist.zh-TW.md) 的對應欄位。若手上有交叉編譯器與模擬器，用同一個檔案對目標處理器再跑一次，兩份輸出的差異就是你要在程式中處理的可攜性邊界。

```sh
python3 compare_targets.py --targets avr,cortex-m0,rv32i,rv32im,x86-64
```

`compare_targets.py` 不需要交叉函式庫或硬體：它用 clang 內建後端把 `tinyc/examples/` 與 `probe/features/` 的程式編成目的檔，列出每個目標的 `int`／指標大小、`.text` 大小，以及硬體沒有而必須呼叫函式庫的運算。本環境 19 個目標的完整輸出在 [results/](probe/results/)，解讀見 [06 實測](../research/06-target-comparison.zh-TW.md)。

## 讀程式碼的順序

1. `tinyc.py` 的 `lex()`：約 60 行，字元變 token。
2. `Parser` 類別：每個方法對應一條文法規則；`postfix()` 中可以看到 `a[i]` 被改寫為 `*(a + i)`。
3. `CodeGen.function()`：序曲與尾聲，對照 README 中的框架圖。
4. `CodeGen.expression()` 與 `binary()`：所有值都經過累加器，暫存值進堆疊；指標加法在這裡乘以 4。這一層完全不知道目標是哪顆處理器。
5. `Machine`、`RV32`、`X86_64` 三個類別：後端的全部內容，各約 120 行。並排閱讀，對照 [tinyc README](tinyc/README.md) 的後端差異表。
6. `rv32sim.py` 的 `execute()`：每條指令一行語意，就是 ISA 規格書的可執行版本。
7. `runtime_rv32.s`：真正的程式從 `_start` 開始，`main` 只是被呼叫的函式；x86-64 後端則借用主機 libc 的啟動碼。
