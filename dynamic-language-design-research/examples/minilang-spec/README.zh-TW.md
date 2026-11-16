# minilang-spec：語言規格與一致性測試

[回主報告](../../README.zh-TW.md)｜[路線圖](../../research/03-design-your-own-dynamic-language.zh-TW.md)｜[階段 2：minilang](../minilang/README.zh-TW.md)｜[階段 4：minilang-vm](../minilang-vm/README.zh-TW.md)｜[階段 5：minilang-c](../minilang-c/README.zh-TW.md)｜[English README](README.md)

這是研究路線圖「階段 8：規格與流程」的可執行範例。minilang 目前有三個引擎，階段 4 與 5 發現它們之所以一致，有一部分是因為照抄樹走訪碰巧的行為。本目錄把語言寫下來，並拿文字檢查引擎：

- [SPEC.zh-TW.md](SPEC.zh-TW.md)（[英文](SPEC.md)）：minilang 1.0 參考手冊。每個實作定義或出人意料的行為都有決定（明定、實作定義，或變更提案並暫列為舊有行為），另有兩個一致性層級、參考引擎的實作定義行為、九項交由負責人決定的變更提案，以及版本、變更流程與發布關卡。
- [tests/](tests/)：201 個一致性測試，規格第 2 到 14 節各一個目錄；每個測試是一支小程式，寫明預期輸出與結束方式，並標明它檢查的節。
- `conformance.py`：在每個引擎上執行每個測試，印出各引擎、各節的矩陣。只用 Python 標準函式庫。
- `test_conformance.py`：測試這套測試本身，包括在樹走訪中刻意植入偏差，確認測試抓得到。

本目錄不修改任何引擎。違反明定規則的引擎列在下方作為回報，不在這裡修正。已在 Windows 11、Python 3.14.6 與 WinLibs GCC 16.2.0 上驗證。

## 執行

```bash
cd dynamic-language-design-research/examples/minilang-spec
gcc -std=c11 -Wall -Wextra -Werror -O2 -o ../minilang-c/mlvm ../minilang-c/mlvm.c -lm   # C VM，同 minilang-c（Windows 上為 mlvm.exe；git 會忽略）
python3 conformance.py                     # tree、vm、c，透過 Python API，約 3 秒
python3 conformance.py --cli               # 同上，透過命令列，約 30 秒
python3 conformance.py --all-configs       # 另加 vm --opt、c --opt 與 c --gc-stress
python3 conformance.py --level core        # Core 層級：只跑 core 測試，不比對訊息文字
python3 conformance.py --strict            # 引擎不再符合文件記載的行為時也算失敗
python3 conformance.py --list              # 列出每個測試的節、層級與狀態
python3 conformance.py --mlvm PATH         # 使用其他位置建置的 C VM（預設：$MLVM，再來是 ../minilang-c/mlvm）
python3 conformance.py --cmd "mine=python3 my_impl.py"   # 任何遵守規格 15.3 節的實作
python3 -m unittest test_conformance -v    # 測試本身的 14 個測試，約 3 秒
```

Windows 上 `python3` 可能是 `python`。結束碼就是規格附錄 D 的發布關卡：所有選定的測試在所有引擎上都通過才是 0。

## 測試格式

測試是一支 minilang 程式，後面接一個以 `# ---` 開頭的註解區塊。區塊放在檔尾，預期結果中的行號才會是程式本身的行號：

```text
print("before");
print(
  1 +
  "a"
);

# ---
# spec: 10.4
# title: an operator error is reported at the operator's line
# end: runtime-error 3
# message: '+' expects two numbers, two strings or two lists, got number and string
#> before
```

| 鍵 | 意義 |
| --- | --- |
| `spec` | 測試檢查的規格節；第一個決定所在目錄與矩陣的列 |
| `level` | `core`（預設）或 `full`；規格 15.1 節 |
| `status` | `specified`（預設）、`legacy P3`（在提案 P3 決定前保留現行行為），或 `impl`（實作定義） |
| `end` | `ok`、`syntax-error N`、`runtime-error N`、`fatal`，或以 `\|` 分隔的多個選項（`impl` 測試的允許範圍） |
| `message` | 錯誤訊息；只在 Full 層級比對 |
| `#>` | 每行一個預期輸出行；`output: unchecked` 表示不比對輸出（部分 `impl` 測試） |
| `doc-tree`、`doc-vm`、`doc-c` | `impl` 測試中各引擎文件化的結果（規格附錄 B）；不同時回報為文件偏移 |

執行器把整個程式（含註解）交給引擎。一次執行以 `ok`、`syntax-error`、`runtime-error`、`fatal`（`try` 接不到的錯誤，規格 10.7 節）或 `crash`（引擎本身失敗）結束，`crash` 永遠不算通過。

執行引擎有兩種方式：

- **Python API**（預設）：三個引擎共用的建構子與 `run()` 約定（`minilang.Interpreter`、`minilang_vm.VM`、`minilang_c.CVM`），和它們自己的測試用法相同。執行器會先把 Python 遞迴上限提高到 20,000，與引擎的命令列一致。C VM 只開一個 `mlvm --serve` 行程並擷取它的標準錯誤，當掉時可以附上 VM 自己的訊息。
- **命令列**（`--cli`）：`python3 minilang.py FILE`、`minilang_vm.py`、`minilang_c.py`，依規格 15.3 節的約定分類。命令列分不出語法錯誤與執行期錯誤（兩者都印 `error: [line N] ...`），所以兩種都算符合。執行器會設定 `PYTHONIOENCODING=utf-8`（見下方發現 5）。

## 結果

`python3 conformance.py --all-configs`，2026-11-16，Windows 11 Pro、Python 3.14.6、以 WinLibs GCC 16.2.0 `-O2` 建置的 `mlvm`：

| 規格節 | 測試數 | tree | vm | vm --opt | c | c --opt | c --gc-stress |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2 詞法結構 | 24 | 24 | 24 | 24 | 24 | 24 | 24 |
| 3 語法 | 18 | 18 | 18 | 18 | 18 | 18 | 18 |
| 4 值與型別 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| 5 真假值、相等與順序 | 13 | 13 | 13 | 13 | 13 | 13 | 13 |
| 6 運算式 | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 7 作用域與閉包 | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 8 控制流程 | 13 | 13 | 13 | 13 | 13 | 13 | 13 |
| 9 函式與呼叫 | 10 | 10 | 10 | 10 | 10 | 10 | 10 |
| 10 錯誤與例外 | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 11 標準函式 | 20 | 20 | 20 | 20 | 20 | 20 | 20 |
| 12 數值 | 17 | 17 | 17 | 17 | 17 | 17 | 17 |
| 13 字串與文字 | 6 | 6 | 6 | 6 | 6 | 6 | 6 |
| 14 限制 | 14 | 14 | 14 | 14 | **13** | **13** | **13** |
| **Core 層級** | 160 | 160 | 160 | 160 | **159** | **159** | **159** |
| **Full 層級** | 201 | 201 | 201 | 201 | **200** | **200** | **200** |

201 個測試中，明定 168 個、舊有行為 23 個、實作定義 10 個。`--cli --all-configs` 得到相同的矩陣與相同的失敗。Python 引擎在同一行程中各約 0.2 秒；透過命令列、8 個行程平行時，每個引擎 5 到 14 秒。

實作定義的測試，依執行器的輸出整理（D 編號見規格附錄 A）：

| 測試 | tree | vm、vm --opt | c、c --opt、c --gc-stress |
| --- | --- | --- | --- |
| `02-lexical/unicode-identifier`、`unicode-whitespace`、`literal-unicode-decimal-digit`（D2、D3、D5） | ok | ok | ok |
| `02-lexical/literal-superscript-digit`（D5） | fatal ValueError | fatal ValueError | fatal ValueError |
| `02-lexical/literal-4301-digits`（D6） | fatal ValueError | fatal ValueError | fatal ValueError |
| `11-builtins/num-unicode-digits`（D34） | ok，印出 `3 12` | ok，印出 `3 12` | runtime-error 1 |
| `14-limits/cyclic-list-text`（D49） | fatal RecursionError | fatal RecursionError | fatal RecursionError |
| `14-limits/cyclic-list-text-in-function`（D49） | ok，印出 `caught: stack overflow (host recursion limit)` | **fatal RecursionError** | ok，同 tree |
| `14-limits/nesting-list-text-1100`（D48） | ok | ok | fatal RecursionError |
| `14-limits/nesting-functions-200`（D48） | ok | ok | **crash** |

## 發現

1. **C VM 錯誤：64 層巢狀函式字面值會讓行程當掉。** `mlvm.c` 的 `load_code()` 每多一層函式巢狀就推入一個暫時根，而暫時根堆疊只能放 64 個（`TEMP_ROOTS`）；載入器自己的上限 `MAX_CODE_NESTING`（訊息 `functions nested too deeply`）是 200，永遠到不了。VM 以 `mlvm: fatal: temp root stack overflow` 與結束碼 70 停止；serve 模式下同一行程的其他工作階段全部遺失。63 層可以運作。規格 14.3 節要求超過實作上限時要有診斷過的錯誤，所以這在每種 C 設定下都讓 `14-limits/nesting-functions-200` 失敗，也是整套測試唯一的失敗。本目錄不修正。
2. **在函式內印出包含自己的列表時，引擎不一致。** 樹走訪在最內層呼叫處把宿主的 `RecursionError` 轉成可捕捉的 `stack overflow (host recursion limit)`，C VM 照抄；Python VM 沒有可以轉換的宿主呼叫，錯誤以致命的 `RecursionError` 逸出。minilang-vm 的說明寫這個訊息只會來自極深的巢狀運算式；印出循環或很深的列表是第二種、也很普通的觸發方式。規格把它定為實作定義（D49），提案 P5 會改成印出 `[...]`。
3. **共用前端遇到某些數字會當掉。** 數字字面值中的 `²`（以及其他 Python `str.isdigit()` 為真但 `int()` 失敗的字元）會讓詞法分析器丟出 Python 的 `ValueError`，4,301 位數的整數字面值也一樣。三個引擎共用詞法分析器，所以三者都如此；提案 P7 會把兩者都變成語法錯誤或一般字面值。
4. **整除的整數除法不精確。** `9007199254740993 / 1` 是 `9007199254740992`，因為商先捨入成浮點數再轉回整數。三個引擎一致（C VM 移植了 CPython 的捨入），所以對引擎而言不是錯誤；規格把它保留為舊有行為，提案 P9 會讓整除商精確。這是撰寫規格 12.3 節時發現的，既有測試都沒有涵蓋。
5. **Python 命令列在這台機器上無法印出非 ASCII 文字。** 系統地區設定為 cp950、標準輸出接到管線時，`minilang.py` 與 `minilang_vm.py` 遇到第一個 `é` 就以 `UnicodeEncodeError` 失敗；`minilang_c.py` 正常，因為 `mlvm` 直接寫 UTF-8 位元組。規格 15.3 節把 UTF-8 列入命令列約定，執行器會設定 `PYTHONIOENCODING=utf-8`。
6. **語法分析在提高遞迴上限之前執行。** `Interpreter.run()` 與 `VM.run()` 在語法分析之後才提高 Python 的遞迴上限，所以透過 Python API、遞迴上限為預設的 1,000 時，70 層巢狀括號就以致命的 `RecursionError` 失敗（60 層可以）；命令列沒問題，因為它們事先把上限提高到 20,000。執行器也這樣做，規格 14.3 節則訂下巢狀深度的下限。
7. **三個引擎，獨立實作卻沒有三個。** 詞法與語法分析器只有一份，Python VM 重用樹走訪的標準函式、文字格式與運算子慢速路徑。規格第 2、3 節的一致只是同一個實作和自己一致；只有 C VM 獨立實作運算子、數值與文字。規格附錄 D.2 以路線圖引用的 TC39「兩個獨立實作」標準記錄這一點。

## 規格做了哪些決定

規格附錄 A 列出 50 項決定。簡言之：詞法規則、值、相等、使用時才解析的作用域、閉包、錯誤行號、標準函式與 Python 的數值語意，都依引擎現況**明定**。識別字與空白中的 Unicode、`num()` 讀非 ASCII 數字、巢狀上限、循環列表的文字、`clock()`、致命錯誤的回報與輸出編碼是**實作定義**，參考引擎的行為記錄在附錄 B。九項行為是**變更提案**，在負責人決定前保留為舊有行為（Full 層級必須照做，Core 層級不規定）：

| # | 提案 | 取代現行的 |
| --- | --- | --- |
| P1 | `throw nil` 繫結 `nil` | 被接成字串 `"nil"` |
| P2 | 靜態區塊作用域 | 之後的 `let` 會改變閉包中名稱指向的變數 |
| P3 | `break` 只能在同一函式的迴圈內，否則是語法錯誤 | `break` 跨越函式呼叫；多餘的 `break` 是致命錯誤 |
| P4 | 函式外的 `return` 是語法錯誤 | 執行時致命 |
| P5 | 一般運算不產生致命錯誤；循環列表印成 `[...]` | 宿主的 `OverflowError`、`ValueError`、`RecursionError` 逸出 |
| P6 | 去除 `num()` 的 Python 遺留 | `num() cannot convert None`、`num(true)` 是 1 |
| P7 | 字面值只用 ASCII 數字；前端永不當掉 | 發現 3 |
| P8 | 呼叫深度改為至少 400 而非恰好 400 | 恰好 400 |
| P9 | 精確的整數商 | 發現 4 |

## 測試本身如何驗證

- **植入偏差**（`test_conformance.py`）：只改一條規則的樹走訪，必須讓該規則的測試失敗。共檢查八種變體：呼叫深度 399、`throw nil` 繫結 `nil`、截斷式 `%`、布林等於數值、六位數的浮點數文字、`break` 停在函式邊界、改寫過的除以零訊息，以及所有執行期錯誤都回報在第 1 行。每一種都被抓到。改寫訊息與詞法 `break` 兩種只在 Full 層級失敗、Core 層級通過，這正是分層的用意。測試的第一版在詞法 `break` 變體上卡住，因為有個測試用了 `while (true)`；現在所有測試中的迴圈都有上限，錯誤的引擎會失敗而不是卡住。
- **節的引用**：測試引用的每一節都是兩份規格中都存在的標題。
- **兩種執行方式**：Python API 與命令列得到相同的矩陣，所以分類結果不取決於引擎的驅動方式。

## 限制與未驗證事項

- 只在一台機器、一套工具鏈上執行：Windows 11、Python 3.14.6、WinLibs GCC 16.2.0。沒有在 Linux 或其他編譯器上跑。
- 路線圖階段 8 的驗收「他人能依手冊實作語言子集並通過一致性測試」尚未達成：目前沒有任何只依 SPEC 寫成的實作。Core 層級的設計讓實作不需要 Python 的數值格式，但這一點未經測試。
- 測試是對每條規則取樣，沒有窮舉每種訊息形式（例如所有 `expected X but found K`）、每個 Unicode 類別或 REPL。致命錯誤回報的措辭刻意不測。
- C 的轉接層驅動 `minilang_c.CVM`，並讀取 `minilang_c` 的兩個內部名稱來擷取當掉時的訊息；若階段 6 重構該模組，執行器會改為不擷取的方式執行，命令列模式則不依賴它們。
- `clock()` 只檢查是正數，以及兩次呼叫之間不會倒退。

## 下一步

1. 負責人決定 P1 到 P9；採納的提案依規格附錄 D.2 進行，成為 minilang 2.0，若 VM 指令的意義改變，MLBC 也要升主版本。
2. 修正 C VM 的巢狀當掉問題（發現 1），讓發布關卡通過；之後把 `--strict` 納入關卡。
3. 不讀引擎、只依 SPEC 寫一個 Core 實作，檢驗手冊是否足夠。

詳細決策見[路線圖](../../research/03-design-your-own-dynamic-language.zh-TW.md)。
