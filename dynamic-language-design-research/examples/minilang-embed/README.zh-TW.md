# minilang-embed：minilang C VM 的嵌入 API 範例

[回主報告](../../README.zh-TW.md)｜[路線圖](../../research/03-design-your-own-dynamic-language.zh-TW.md)｜[階段 5：minilang-c](../minilang-c/README.zh-TW.md)｜[階段 4：minilang-vm](../minilang-vm/README.zh-TW.md)｜[階段 2：minilang](../minilang/README.zh-TW.md)｜[English README](README.md)

這是研究路線圖「階段 6：宿主互通」的可執行範例。它把階段 5 的 VM [`../minilang-c/mlvm.c`](../minilang-c/README.zh-TW.md) 變成附公開標頭 [`minilang.h`](minilang.h) 的函式庫：C 程式可以建立 VM、載入位元組碼檔、帶引數呼叫 minilang 函式、註冊讓 minilang 呼叫的 C 函式、雙向傳遞 minilang 的每一種值，並以狀態碼取回錯誤。VM 沒有分叉：`minilang.c` 直接引入 `mlvm.c`，後者的命令列建置與行為都不變。本目錄另有 C 測試程式、一個範例宿主（以 minilang 寫報表規則的 CSV 篩選器），以及一個簡短的 Python ctypes 範例。目的在示範階段 6 的元件與驗收方式，不是生產品質的 API。

已在 Windows 11（WinLibs GCC 16.2.0、Python 3.14.6）與 Ubuntu 24.04 容器（podman；GCC 13.3.0、Python 3.12.3、AddressSanitizer、UndefinedBehaviorSanitizer 與 LeakSanitizer）上執行並通過測試。

## 執行

```bash
cd dynamic-language-design-research/examples/minilang-embed
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_embed test_embed.c minilang.c -lm    # C 測試
gcc -std=c11 -Wall -Wextra -Werror -O2 -o mlfilter mlfilter.c minilang.c -lm       # 範例宿主
python3 -m unittest test_minilang_embed -v                                        # 10 個測試，約 1 秒（也需要 ../minilang-c/mlvm）
python3 ../minilang-vm/minilang_vm.py --emit report.mlc programs/report.ml
./mlfilter report.mlc data/orders.csv                                             # 以 minilang 寫的 CSV 報表
./mlfilter --gc-stress report.mlc data/orders.csv                                 # 同上，但每次配置都回收
gcc -std=c11 -Wall -Wextra -Werror -O2 -shared -o minilang.dll minilang.c -lm      # 選用；Linux：-shared -fPIC -o libminilang.so
python3 embed_ctypes.py                                                           # 從 Python 使用同一個 API，只用標準函式庫
```

Windows 上 `python3` 可能要寫成 `python`。除了階段 5 已用到的 GCC（WinLibs）與 Python 之外不需要安裝任何東西。宿主程式只要把 `minilang.c` 一起編譯。`gcc -c minilang.c` 產生的目的檔只匯出 `minilang.h` 的 `ml_` 函式（兩個平台都用 `nm` 確認過），因為來自 `mlvm.c` 的一切都維持 `static`。

10 個測試的組成：C 測試程式（13 個測試函式、2,526 項檢查）分別以一般模式、`--gc-stress` 與 4 KiB 堆積執行；1 個確認 API 誤用預設會中止程式的測試；範例報表在一般模式與 `--gc-stress` 下各一次；1 個缺少腳本所需欄位的 CSV 檔；ctypes 範例，未建置共享函式庫時略過；以及 2 個針對 `mlvm.c` 本身的回歸測試，執行階段 5 的命令列 VM 與它的 serve 模式（見下方「兩項修正」）。

## 管線

```text
宿主程式（C）→ minilang.h：ml_open、ml_load、ml_run、ml_call、ml_register、handle、狀態碼
            → minilang.c：handle 槽位與 scope、宿主函式的跳板、錯誤對應
              （#define MLVM_LIBRARY、#include "../minilang-c/mlvm.c"）
            → mlvm.c：載入器、驗證器、直譯器、回收器（階段 5），加上五個嵌入用掛鉤
位元組碼檔   ← python3 ../minilang-vm/minilang_vm.py --emit（階段 4 的前端，未修改）
```

## mlvm.c 改了什麼

新增 144 行、修改 36 行（從 3,158 行變成 3,266 行）。命令列、輸出與一行建置指令都不變；階段 2 的 13 個測試、階段 4 的 76 個與階段 5 的 94 個不改一行全部通過，階段 5 的隨機程式比對（1,008 個程式、6,048 次 C VM 執行）沒有任何差異。

| 修改 | 原因 |
| --- | --- |
| 在驅動程式（`main`、`--serve`、統計文字）、反組譯器與二進位 stdio 外加 `#ifndef MLVM_LIBRARY` | 函式庫沒有 `main`，沒用到的 `static` 函式會讓 `-Werror` 失敗 |
| 物件種類 `O_HOST`（`ObjHost`：名稱、參數個數、C 函式指標、使用者資料），追蹤、印出、型別都比照內建函式 | 宿主函式是 minilang 的值：可以存起來、傳遞、比較 |
| `OP_CALL` 透過 `call_host` 掛鉤呼叫 `ObjHost`，之後重新載入快取的 `stack`、`regs`、`sp` | 宿主函式可能執行 minilang 程式碼，而那會讓值堆疊成長並因此搬家 |
| `mark_roots()` 呼叫 `mark_host` 掛鉤 | 宿主持有的 handle 是根 |
| `execute()` 拆成建立框架與 `run(vm, floor, result)`；`RETURN`、`BREAK_OUT`、例外展開與遞迴錯誤的轉換都停在 `floor`，而不是框架 0 | 一次執行可以從一個暫停中的 minilang 呼叫之上開始，任何展開都不會進入在 C 函式另一側等待的框架 |
| `mem_limit` 與 `over_limit`：`heap_realloc()` 設旗標，`OP_JUMP` 與 `OP_CALL` 丟出 `MemoryError` | 把記憶體不足變成狀態碼，而不必在每個配置點加失敗路徑 |
| 暫時根堆疊改為可成長（`malloc`，從不用 GC 堆積），不再固定 64 格 | 錯誤修正，見下 |
| `LOAD_CELL`、`STORE_CELL` 或 `MAKE_CLOSURE` 遇到不是 cell 的槽位時，以接不到的 `SystemError` 結束該次執行，不再呼叫 `fatal()` | 錯誤修正，見下 |

命令列 VM 的所有掛鉤都是零。`OP_JUMP` 與 `OP_CALL` 多出的 `over_limit` 判斷量不出成本：階段 4 的三個基準交錯各跑 9 次、共量兩輪，最終版 `mlvm` 的執行時間是舊版的 0.95 至 1.03 倍（中位數；每個基準兩版的最小至最大範圍都重疊），輸出完全相同。

### 兩項修正

階段 8 的一致性測試發現：函式字面值巢狀 64 層時，`mlvm` 會以 `mlvm: fatal: temp root stack overflow` 停止（結束狀態 70），在 `--serve` 下同一程序裡的其他 session 也一起結束；63 層則正常。載入器把正在填寫的每個程式碼物件放在暫時根堆疊上，每層巢狀一格，但這個堆疊固定只有 64 格，所以載入器自己的 200 層上限（「functions nested too deeply」）永遠到不了。現在堆疊會成長；成長用 `malloc`，所以推入根仍然不會觸發回收。同樣的理由也適用於另一個輸入能觸及的 `fatal()`：通過驗證器、卻把不含 cell 的槽位當 cell 讀的位元組碼（驗證器無法靜態排除）會結束整個程序；現在改為以 `SystemError` 結束那一次執行。嵌入的宿主不能因為載入一個檔案就被殺掉。

回歸測試：函式字面值巢狀 63、64、200、201 層的程式，以及一個把一道 `LOAD_LOCAL` 改成 `LOAD_CELL` 的階段 4 位元組碼檔，在函式庫上（`test_embed`，一般、`--gc-stress` 與 4 KiB 堆積）、在命令列 VM 上（一般與 `--gc-stress`）、以及在同一個 serve 程序的一般與壓力 session 中執行。最多 200 層都能載入並執行，201 層是載入錯誤「functions nested too deeply」，壞檔以 `SystemError: bytecode reads a slot that holds no cell` 結束，serve 程序與其他 session 繼續運作。在修正前的階段 5 執行檔上，64、200、201 層與壞檔都以結束狀態 70 收場。仍會呼叫 `fatal()` 的只剩：`malloc` 失敗、未知的 opcode（驗證器已排除），以及 `vsnprintf` 處理 VM 自己的格式字串時失敗。

## API 一覽

| 類別 | 函式 |
| --- | --- |
| VM | `ml_open(options)`、`ml_close`（回傳堆積仍記帳的位元組數，必須是 0）、`ml_set_output`、`ml_get_stats`、`ml_collect`、`ml_api_version` |
| 程式與呼叫 | `ml_load`、`ml_load_file`、`ml_run`、`ml_call` |
| 全域變數與宿主函式 | `ml_get_global`、`ml_set_global`、`ml_new_function`、`ml_register` |
| Handle | `ml_scope_open`、`ml_scope_close`（可選擇帶一個值出去）、`ml_pin`、`ml_release` |
| 值 | `ml_new_nil`、`_bool`、`_int`、`_float`、`_string`、`_list`；`ml_typeof`、`ml_truthy`、`ml_equal`；`ml_get_bool`、`_int`、`_float`、`_string`；`ml_tostring`；`ml_list_len`、`_get`、`_set`、`_push` |
| 錯誤 | `ml_throw`、`ml_throwf`、`ml_error_message`、`ml_error_type`、`ml_error_line`、`ml_error_value`、`ml_clear_error`、`ml_status_name` |

宿主函式只有一種簽章：`ml_status fn(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud)`；VM 呼叫前會像內建函式一樣檢查參數個數（`-1` 表示不限）。`minilang.h` 遵守路線圖的 C API 原則：標頭裡只有不透明型別與 C 內建型別（`ml_vm` 不透明，`ml_value` 是兩個整數）、每個呼叫回傳狀態碼且錯誤細節另外取、內部結構不進標頭，以及版本巨集 `MINILANG_API_VERSION`，執行時可用 `ml_api_version()` 比對。

```c
ml_vm *vm = ml_open(NULL);                        /* NULL：預設選項 */
ml_value prog, fn, arg, result;
if (ml_load_file(vm, "app.mlc", &prog) != ML_OK || ml_run(vm, prog, NULL) != ML_OK)
    fprintf(stderr, "%s: %s\n", ml_error_type(vm), ml_error_message(vm));
ml_get_global(vm, "double", &fn);
ml_new_int(vm, 21, &arg);
if (ml_call(vm, fn, 1, &arg, &result) == ML_EXCEPTION)
    fprintf(stderr, "line %d: %s\n", ml_error_line(vm), ml_error_message(vm));
ml_close(vm);                                     /* 釋放一切，包括 handle */
```

## 根集合：scope 裡的 handle

### 規則

宿主程式碼從不直接持有 minilang 的值。每個產生值的 API 函式給出一個 `ml_value` handle：VM 所擁有的槽位表中的索引，加上一個世代計數。回收器標記每一個槽位（`mark_roots()` 呼叫 `mark_handles()`），所以只要還有 handle 指向某個值，它就活著，反之亦然。Handle 由 `ml_release`、關閉建立它的 scope、或關閉 VM 釋放。

- **Scope。** Handle 建立在最內層開啟的 scope；`ml_scope_close` 一次釋放其中全部，並可把一個值帶到外層 scope（V8 的 `EscapableHandleScope`）。VM 在每次呼叫宿主函式時替它開一個 scope，所以宿主函式的引數與暫存值不必清理，宿主函式也無法關閉呼叫端的 scope。每輪都會產生值的宿主迴圈應該每輪開一個 scope，`mlfilter.c` 就是每一列 CSV 一個。
- **Pin。** `ml_pin` 產生不屬於任何 scope 的 handle，給宿主在多次呼叫之間保留的值，例如回呼函式或載入的程式；只有 `ml_release` 會釋放它。
- **偵測。** 槽位使用中時世代是奇數，每次釋放都會改變，所以舊 handle 永遠對不上重複使用的槽位，過期 handle 背後的值也從不會被讀取。以下都算誤用：使用已釋放的 handle、已關閉 scope 的 handle、宿主函式從引數留下來的 handle；不按順序關閉 scope；宿主函式回傳時還有 scope 沒關；在 print 回呼裡呼叫 API；關閉執行中的 VM。函式庫預設會印出問題並呼叫 `abort()`；設定 `ml_options.misuse_returns` 則改為回傳 `ML_MISUSE`。測試兩種都用。
- **唯一的原生指標。** `ml_get_string` 不複製，直接回傳堆積裡字串的位元組，在該 handle 有效期間有效。Lua 對字串指標也有同樣的承諾：值留在堆疊上時有效。物件從不搬家，所以 handle 活著指標就有效；`mlfilter.c` 的 `sort_by` 排序時就依賴這一點。

這套做法便宜，是因為階段 5 的回收器從不搬動物件：handle 是回收器讀取的索引，不是需要它更新的指標。若換成會搬動物件的回收器，API 本身不必改，但函式庫內部的每個指標都要同樣小心，這正是 V8 全面使用 handle 的原因。

### 為什麼是 handle，不是其他做法

| 設計 | 誰在用 | 放在這裡代表什麼 |
| --- | --- | --- |
| 值放在虛擬堆疊上，以索引存取 | Lua：「Lua uses a virtual stack to pass values to and from C」 | 對回收器同樣安全，但每個呼叫都是索引運算（`-1`、`-2`），推入一個暫存值就讓其他值的索引全部位移 |
| 參考計數的指標，分擁有與借用 | CPython：`Py_INCREF` 與 `Py_DECREF`；「using a borrowed reference after the owner has disposed of it risks using freed memory」 | 需要這個 VM 沒有的計數，而借用的指標正是這個 API 想排除的「未列入根的值」；錯誤只在記憶體被重複使用時才現形 |
| Scope 裡的 handle，加上可活過 scope 的 handle | V8：`HandleScope` 結束時「all handles created within that scope are removed from the stack」；Node-API：`napi_open_handle_scope` 與 `napi_ref` | 採用。Scope 讓清理自動化；世代計數把錯誤變成立即、有名字的錯誤，而不是偶發的當機 |

來源：[V8 docs：embed](https://v8.dev/docs/embed)、[Node-API](https://nodejs.org/api/n-api.html)；不在本主題[來源清單](../../research/04-sources.zh-TW.md)內的：[Lua 5.4 參考手冊 §4](https://www.lua.org/manual/5.4/manual.html#4)、[Python：Extending and Embedding 的參考計數一節](https://docs.python.org/3/extending/extending.html#reference-counts)。

### 強制 GC 模式能證明它嗎？

C 測試以一般模式、`--gc-stress`（每次配置都回收，並把釋放的物件填成 `0xDB`）與 4 KiB 堆積執行；在 Windows 上以 -O0 與 -O2 建置，在容器內以 AddressSanitizer、UBSan 與 LeakSanitizer 建置。每種設定下每項檢查都通過，每個 VM 結束時記帳都是 0 位元組。「rooting」測試讓 300 個字串與一個 300 項的串列只靠 handle 存活，中間穿插強制回收與製造垃圾的呼叫。

與階段 5 相同，做了一次性的檢查（不在本目錄）：在程式碼副本中一次植入一個錯誤，共五個，每份副本都執行 `test_embed`。

| 植入的錯誤 | Windows：預設堆積 / 4 KiB / `--gc-stress` | ASan：預設堆積 / `--gc-stress` |
| --- | --- | --- |
| 1. `mark_roots()` 不呼叫 handle 掛鉤 | 302 項檢查失敗 / 當機 / 當機 | 使用已釋放記憶體 / 同左 |
| 2. 不標記 pin 住的 handle | 每種都有 2 項檢查失敗 | 使用已釋放記憶體 / 同左 |
| 3. `ml_new_function` 在兩次配置之間沒有把名稱列入根 | 通過 / 通過 / 下一次載入因字串損壞而失敗 | 通過 / 使用已釋放記憶體 |
| 4. `OP_CALL` 在宿主呼叫後不重新載入堆疊指標 | 三種都通過 | 使用已釋放記憶體 / 同左 |
| 5. `ml_call` 在執行結束與建立結果 handle 之間讓回收發生 | 當機 / 通過 / 32 項檢查失敗 | 使用已釋放記憶體 / 同左 |

- 錯誤 1、2、5 在預設堆積下就現形，只是因為測試自己呼叫了 `ml_collect()`。錯誤 3 只有壓力模式抓得到，因為只有它會剛好在那兩次配置之間回收。
- **錯誤 4 不是根集合錯誤，壓力模式也抓不到。** 值堆疊在巢狀呼叫期間搬家了（`realloc`），舊記憶體仍存著正確的值；只有 ASan 看得出來。重入帶來第二類錯誤：指向已搬走記憶體的過期指標。階段 5 的結論再次成立：壓力模式與 sanitizer 要一起用，單靠任何一種都不夠。

## 錯誤跨邊界

### 狀態碼與待處理錯誤

| 狀態 | 意義 | 待處理錯誤（`ml_error_type`） |
| --- | --- | --- |
| `ML_OK` | 成功 | 無 |
| `ML_EXCEPTION` | minilang 例外：`throw`、執行期錯誤，或 C 裡的 `ml_throw`、`ml_throwf`；`try`/`catch` 接得到 | `RuntimeErr`，附訊息、行號與值 |
| `ML_ERROR` | minilang 接不到的錯誤 | `OverflowError`、`ValueError`、`RecursionError`（階段 5 的 Python 層錯誤）、`BreakSignal`、`ReturnSignal`、`SystemError`（宿主錯誤）、`LoadError` |
| `ML_NOMEM` | 堆積超過 `heap_limit` | `MemoryError`，接不到 |
| `ML_TYPE_ERROR` | 型別錯誤、超出範圍或不是 UTF-8 | 可接的例外，例如 `expected an integer, got string` |
| `ML_NOT_FOUND` | 沒有這個全域變數 | 可接的 `undefined variable 'x'` |
| `ML_LOAD_ERROR` | 載入器拒絕位元組碼檔 | `LoadError`，附載入器的訊息 |
| `ML_MISUSE` | 宿主錯誤，在設定 `misuse_returns` 時 | `SystemError` |

型別名稱就是樹走訪會丟出的 Python 例外名稱，與階段 5 的 serve 協定相同。失敗的呼叫留下待處理錯誤，由 `ml_error_message`、`ml_error_line` 與 `ml_error_value`（被丟出的值本身，否則是訊息）讀取；它保留到下一次失敗的呼叫、`ml_clear_error`，或 `ml_run`、`ml_call` 開始時。Node-API 也是這樣：每個呼叫回傳 `napi_status`，`napi_get_last_error_info` 描述上一個呼叫。

### 宿主函式的規則

宿主函式要表示失敗，就在有待處理錯誤時回傳 `ML_OK` 以外的狀態。最簡單的做法是直接回傳失敗的 API 呼叫的狀態：

```c
static ml_status fmt(ml_vm *vm, int nargs, const ml_value *args, ml_value *result, void *ud) {
    double x;
    ml_status st = ml_get_float(vm, args[0], &x); /* 字串：ML_TYPE_ERROR，現在是待處理錯誤 */
    if (st != ML_OK) return st;                   /* minilang 在呼叫處看到可接的例外 */
    if (x > 1e300) return ml_throwf(vm, "fmt(): number too large"); /* 或直接丟出一個 */
    ...
}
```

VM 接著的展開方式與內建函式失敗完全相同：`ML_EXCEPTION` 由呼叫外最近的 `try` 接住，本身沒有行號時取呼叫所在的行；`ML_ERROR` 或 `ML_NOMEM` 直接傳回啟動這次執行的宿主。宿主函式自己處理了錯誤就回傳 `ML_OK`，錯誤隨之清除。回傳錯誤狀態卻沒有待處理錯誤是宿主的錯，會變成 `SystemError`。

### 不以 longjmp 跨越宿主框架

Lua 以 `longjmp` 處理錯誤：「Internally, Lua uses the C `longjmp` facility to handle errors」，而 `lua_error`「does a long jump, and therefore never returns」。C 函式所做的 API 呼叫裡發生錯誤時，會跳過該函式的框架直到最近的 `lua_pcall`，所以當時手上有 `malloc` 緩衝區的 C 函式必須為此特別撰寫。這裡什麼都不跳。VM 內部的錯誤本來就是回傳值（階段 5 的 `rt_error()` 回傳 -1，迴圈再依例外表展開）；API 把它們當狀態碼回傳，宿主函式的狀態也以同樣方式回到迴圈。兩個 minilang 框架之間的每個 C 框架都正常返回，`mlfilter.c` 的 `sort_by` 在錯誤路徑上像一般 C 函式一樣釋放陣列。

代價是 VM 不能在配置到一半時停下，這決定了記憶體上限的形狀：

- **堆積上限是軟性的。** 設定 `ml_options.heap_limit` 後，會超過上限的配置先回收；回收後仍超過，配置照樣成功，但設一個旗標。直譯器在每個 `JUMP`（每個迴圈的回邊）與 `CALL` 檢查旗標，所有無界的計算都會經過這兩處，在那裡丟出 `MemoryError`；API 呼叫則在配置後立刻檢查，回傳 `ML_NOMEM`。堆積最多超過上限一輪迴圈或一段直線程式碼的配置量。測試中在 256 KiB 上限下，無限擴大串列的迴圈、包在 `try`/`catch` 裡的同一個迴圈（接不到）、以及完全沒有迴圈、靠遞迴建立串列的程式，全都以 `ML_NOMEM` 結束，峰值低於上限的兩倍，之後 VM 一切正常。
- **`malloc` 失敗仍是致命的。** C 函式庫回傳 `NULL` 時，VM 印出訊息並結束程序，與階段 5 相同。要讓它可恢復，得在 `mlvm.c` 數百個配置點都加上狀態檢查，正是本範例刻意避免的重寫。
- **C 堆疊有上限。** 每一層 minilang → C → minilang 都會用到 C 堆疊（直譯器迴圈、跳板、宿主函式）。同時最多 200 個宿主函式在執行，再下一次呼叫是可接的 `stack overflow (max host call depth 200)`，另外仍有 minilang 本身 400 個框架的上限。經由 C 遞迴的測試在 Windows（-O0 與 -O2）與 ASan 下都碰到這個上限。

### 訊號停在 C

在樹走訪裡，迴圈外的 `break` 與頂層 `return` 不是錯誤，而是展開到呼叫端迴圈或離開腳本的訊號（階段 4「被迫寫下來的語意」）。C 呼叫端沒有迴圈，所以它們停在那裡：從 C 呼叫的函式執行這樣的 `break`，以 `ML_ERROR`、型別 `BreakSignal` 結束；腳本的頂層 `return` 讓 `ml_run` 以 `ReturnSignal` 結束，被回傳的值成為錯誤的值。宿主函式若把它們傳下去，仍是接不到的錯誤，不會中斷呼叫外面的迴圈。這是宿主函式唯一與樹走訪裡的內建函式行為不同之處：在樹走訪裡，訊號會穿過內建函式。

## 重入

```text
宿主：ml_call(nested_throw, 4)
  minilang nested_throw(x)        框架 0，run(floor 0)，在 try 之內
    C apply(f, y)                 自己的 handle scope；宿主深度 1
      minilang f(y)               框架 1，run(floor 1)
        C apply(g, z)             宿主深度 2
          minilang g(z)           框架 2，run(floor 2)：throw "inner 4"
          run(floor 2) 在 floor 以上找不到處理器而返回；ml_call 回傳 ML_EXCEPTION
        apply 回傳這個狀態；錯誤仍待處理；f 裡的 OP_CALL 失敗
      run(floor 1) 在 f 裡找不到處理器而返回；ml_call 回傳 ML_EXCEPTION
    apply 回傳它；nested_throw 裡的 OP_CALL 失敗
  框架 0 的處理器接到 "inner 4"；所有 C 框架都正常返回
```

- 所有框架共用一個值堆疊與一個框架陣列。巢狀執行壓在暫停的框架之上，不論怎麼結束，返回時框架與堆疊都恢復成進入時的樣子。
- 測試涵蓋：三層巢狀（`nested`）、上面這個跨越兩個 C 框架的例外（`nested_throw`）、C 接住內層錯誤並回傳替代值、經由 C 遞迴直到深度上限、抵達 C 的 `break`、從 minilang 呼叫的宿主函式裡以 `ml_run` 執行另一個程式，以及等待中的框架在內層呼叫讓堆疊搬家後區域變數仍完好（`keeps_locals`，300 層深）。
- 各 VM 互相獨立，如同 V8 的 isolate（「a VM instance with its own heap」）；測試交錯使用兩個 VM。一個 VM 同一時間只供一個執行緒使用。

### 跨邊界的成本

一次性量測，-O2，與階段 5 同一台機器，每種呼叫一百萬次、重複三次：minilang 呼叫 minilang 函式 9 至 12 奈秒；minilang 呼叫讀兩個整數、回傳一個整數的宿主函式 25 至 29 奈秒；從 C 呼叫（開 scope、建兩個整數、`ml_call`、讀結果、關 scope）36 至 45 奈秒。跨一次邊界約等於二到五次 minilang 呼叫，所以宿主應該一次傳一個串列，而不是每個元素呼叫一次。

## 範例宿主：mlfilter

[`mlfilter.c`](mlfilter.c) 讀取 CSV 檔，由 [`programs/report.ml`](programs/report.ml) 決定要保留哪些列以及怎麼印出。C 這一側解析檔案，在每列一個的 scope 裡把每列建成 minilang 串列（數字已解析），呼叫 `keep(row)`，把保留的列收進一個串列，最後呼叫 `report(rows)`。它註冊三個 minilang 沒有的宿主函式：`field(row, name)`，透過以使用者資料傳入的 CSV 標頭找到欄位；`fmt(number, decimals)`；以及 `sort_by(list, key)`，它對每個元素呼叫 minilang 的鍵函式，於是 minilang 在 C 裡面執行、而那個 C 又在 minilang 裡面。`keep()` 丟出例外的列會連同腳本行號一起回報並略過。

```text
$ ./mlfilter report.mlc data/orders.csv
data/orders.csv:6: row skipped: negative amount -15 (script line 13)      （stderr）
mlfilter: 8 rows, 4 kept, 1 skipped                                      （stderr）
Chen & Sons     TW  1200.00
Garnet Labs     TW  860.75
Acme Trading    TW  250.50
Fjord Tools     NO  100.00
total 2411.25 in 4 orders of at least 100
```

在 `--gc-stress` 下輸出相同。

[`embed_ctypes.py`](embed_ctypes.py) 透過 `ctypes` 從 Python 使用同一個 API。這時 Python 是宿主：它載入共享函式庫、把一個 Python 函式註冊成宿主函式，再呼叫會回頭呼叫它的 minilang 函式。Python 函式丟出例外時（用 `ml_throw`），minilang 接得到，或者狀態碼一路回到 Python。WinLibs 建出的 DLL 只連結 Windows 本身的 C 執行環境，所以不需要其他東西；在 Linux 上則搭配 `libminilang.so` 執行。

## 與 Lua、CPython、V8 的設計取捨

| 問題 | minilang-embed | Lua | CPython | V8 / Node-API |
| --- | --- | --- | --- | --- |
| C 怎麼指稱一個值 | Handle：槽位索引加世代 | 虛擬堆疊上的索引 | `PyObject *`，擁有或借用 | `Local` handle；`napi_value` |
| 怎麼釋放 | 所屬 scope 關閉，或 `ml_release` | 從堆疊彈出 | `Py_DECREF` | `HandleScope` 結束；`napi_close_handle_scope` |
| 在多次呼叫之間保留 | `ml_pin` | `luaL_ref` 存進 registry | `Py_INCREF` | 持久 handle；`napi_ref` |
| 錯誤怎麼回到 C | 狀態碼加待處理錯誤；沒有跳躍 | `longjmp` 到最近的 `lua_pcall` | `NULL` 或 `-1`，加上直譯器的例外狀態 | Node-API：`napi_status`；待處理的例外在原生呼叫返回時於 JavaScript 丟出 |
| 回收器 | 不搬動的標記清除；handle 是根 | 堆疊上的值是根 | 參考計數加循環回收 | 會搬動；必須用 handle |

- **這個設計付出的代價。** 每次存取值都要經過槽位與世代檢查，宿主函式的引數要複製進 handle，成本就是上面量到的數字。錯誤碼讓 C 程式碼變長，和 Node-API 一樣；Lua 的 `longjmp` 讓 C 程式碼簡短，但在 API 呼叫之間持有資源就不安全。
- **它保留的彈性。** 宿主只看得到 handle，所以階段 5 的回收器將來換成會搬動物件或分代的回收器時，`minilang.h` 不必改，只改 `minilang.c` 與 `mlvm.c`。CPython 的經驗是反例：`PyObject *` 一旦公開，改記憶體管理就會破壞擴充（路線圖：PEP 384 的教訓）。

來源：[V8 docs：embed](https://v8.dev/docs/embed)、[Node-API](https://nodejs.org/api/n-api.html)、[PEP 384](https://peps.python.org/pep-0384/)；不在本主題來源清單內的：[Lua 5.4 參考手冊 §4](https://www.lua.org/manual/5.4/manual.html#4)、[Python：Extending and Embedding](https://docs.python.org/3/extending/extending.html)。

## 驗收：路線圖對階段 6 的測試

| 路線圖標準 | 測試 | 結果 |
| --- | --- | --- |
| 用 C 寫一個擴充函式並從語言呼叫 | `test_embed.c` 的「host functions」：參數個數、每一種值、C 丟出的錯誤由 minilang 在呼叫行接住；`mlfilter.c` 的 `field`、`fmt`、`sort_by` | 通過 |
| 用 C 宿主程式嵌入並執行一段腳本 | `mlfilter.c`；`test_embed.c` 的「lifecycle」、「calls」、「reentrancy」 | 通過 |
| （新增）錯誤雙向跨邊界，不用 `longjmp` | 「minilang errors」、「host functions」、「reentrancy」 | 通過 |
| （新增）無法持有未列入根的值，或使用時會被偵測 | 「handles」、「rooting」、`--crash stale`；所有測試都在 `--gc-stress` 下執行；上面的植入錯誤 | 通過 |
| （新增）記憶體不足是一個狀態碼，VM 之後仍可用 | 「heap limit」 | 通過 |
| （新增）階段 5 的執行檔與測試不變 | 階段 2、4、5 的測試、階段 5 的隨機程式、基準比值 | 通過 |
| （新增）沒有輸入能殺掉程序：最多 200 層函式巢狀可載入，201 層是載入錯誤，非 cell 槽位是錯誤 | `test_embed.c` 的「nesting」；`test_minilang_embed.py` 的 `Nesting`（命令列 VM 與 serve 模式） | 通過；在階段 5 的執行檔上失敗 |

## 檔案結構

| 檔案 | 內容 | 行數 |
| --- | --- | --- |
| `minilang.h` | 公開 API，規則寫在註解裡 | 180 |
| `minilang.c` | handle 與 scope、錯誤、宿主函式跳板、呼叫、值；引入 `../minilang-c/mlvm.c` | 900 |
| `test_embed.c` | C 測試：13 個測試函式 | 820 |
| `mlfilter.c`、`programs/report.ml`、`data/orders.csv` | 範例宿主、它的腳本與資料 | 280 |
| `embed_ctypes.py`、`programs/shout.ml` | ctypes 範例 | 110 |
| `test_minilang_embed.py` | 編譯程式、寫出巢狀與壞 cell 檔案、執行上述所有測試 | 210 |
| `tests/*.ml` | C 測試呼叫的程式 | 90 |

## 限制與未驗證

- 只用 GCC 建置（Windows 上 WinLibs 16.2.0、容器內 13.3.0）；沒有試 Clang 與 MSVC。
- 沒有測試多執行緒。一個 VM 同一時間只供一個執行緒使用；各 VM 之間除了 Windows 上快取的計時器頻率外，沒有共用的可變狀態。
- Handle 不記錄所屬的 VM，所以拿到另一個 VM 使用不會被偵測。
- `ml_get_string` 回傳的指標不受檢查：在 handle 釋放後使用是未定義行為，沒有測試這樣做。
- 堆積上限只計物件記憶體，與階段 5 的統計相同：不含值堆疊、handle 表與 `malloc` 的額外開銷，所以程序實際用量可以更多。`malloc` 失敗仍會結束程序，與階段 5 相同。
- 與階段 5 相同，沒有用隨機位元組對位元組碼檔做模糊測試，所以對任意輸入的記憶體安全並未證明；已知由輸入造成的兩種當機已在上面修正。
- 抵達 C 的 `break` 或頂層 `return` 不會繼續作用到宿主函式外面的 minilang 程式碼。
- print 回呼不能再呼叫 VM。
- 除了 `MINILANG_API_VERSION` 1 之外沒有 ABI 承諾；`ml_value` 的欄位公開只是為了能以值傳遞，不是讓人讀的。
- 宿主只能載入位元組碼；原始碼仍需要 Python 前端（階段 7）。
- 植入錯誤的檢查與呼叫成本的量測是一次性的；那些副本與程式不在本目錄。

## 若要往下推進

1. 階段 7：寫一個 C 前端，讓宿主能載入原始碼；再為這裡測過的兩個平台建 CI。
2. 在每個 handle 裡記錄所屬 VM（借用世代的幾個位元），以偵測拿錯 VM 的 handle。
3. 若呼叫成本開始變得重要，在同一個 VM 上比較這套 API 與值堆疊設計（Lua 的做法）。

詳細決策見[路線圖](../../research/03-design-your-own-dynamic-language.zh-TW.md)。
