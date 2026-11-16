# 自製動態語言路線圖

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-19。本文把「從 0 設計一門動態語言」拆成九個階段，每階段說明要做什麼、怎麼驗收、歷史上的語言怎麼選、常見陷阱。階段順序與 Python、Lua 的實際歷史以及《Crafting Interpreters》的兩部結構一致：**先做會跑的樹走訪直譯器，再做位元組碼 VM，最後才談 JIT 與多平台**。行數與時間是依公開教材與參考實作推估，不是量測。

可執行範例 [minilang](../examples/minilang/README.zh-TW.md) 對應階段 2，用 Python 寫成單檔直譯器，約 900 行含測試；[minilang-vm](../examples/minilang-vm/README.zh-TW.md) 對應階段 4，沿用 minilang 的詞法與語法分析器，加上位元組碼編譯器與堆疊式 VM，並量測相對樹走訪的加速倍數；[minilang-c](../examples/minilang-c/README.zh-TW.md) 對應階段 5，用 C11 寫成載入 minilang-vm 位元組碼檔的 VM 與精確標記清除 GC，並補上 C 版的加速倍數。

## 階段 0：寫下問題、範圍與宿主

Python 起於 Amoeba 系統管理，Lua 起於 Petrobras 的設定檔與資料輸入，Node.js 起於非阻塞網路伺服器。每個成功的動態語言都先有一個具體問題。[Python 官方 FAQ](https://docs.python.org/3/faq/general.html)、[The Evolution of Lua](https://www.lua.org/doc/hopl.pdf)、[Introduction to Node.js](https://nodejs.org/en/learn/getting-started/introduction-to-nodejs)

一頁設計文件至少回答：

| 問題 | 選項與歷史案例 |
| --- | --- |
| 要解決什麼問題 | 設定與腳本（Lua）、系統膠合（Python）、事件驅動 I/O（Node.js）、教學（Lox、Monkey） |
| 嵌入還是獨立 | Lua 第一天就是 C 函式庫；Python 是獨立直譯器後補嵌入 API；V8 是引擎，Node.js 是宿主 |
| 宿主語言 | C（CPython、Lua、QuickJS、MicroPython）、C++（V8）、RPython（PyPy）、Java 或 Go（教材用）。原型階段可用任何高階語言，正式實作選 C 子集最容易移植 |
| 授權 | Lua 與 QuickJS 用 MIT；V8 用 BSD；這會影響能否被嵌入商業產品 |
| 明確不做什麼 | Lua 不提供陣列型別只給表；Node.js 不提供多執行緒共享記憶體模型 |

驗收：能用十行範例程式說明語言的核心價值，而且範例裡沒有「之後再說」的功能。

## 階段 1：語法與語意草案

需要四份東西：文法、值模型、作用域規則、錯誤模型。先寫人讀的版本，不急著形式化。

### 文法

用 EBNF 或 PEG 寫。minilang 的文法示例：

```text
program     := statement* EOF
statement   := "let" IDENT "=" expression ";"
             | "fn" IDENT "(" params? ")" block
             | "if" "(" expression ")" block ( "else" ( block | ifStmt ) )?
             | "while" "(" expression ")" block
             | "return" expression? ";" | "break" ";" | "throw" expression ";"
             | "try" block "catch" "(" IDENT ")" block
             | block | expression ";"
block       := "{" statement* "}"
expression  := assignment
assignment  := ( IDENT | postfix "[" expression "]" ) "=" assignment | logicOr
logicOr     := logicAnd ( "or" logicAnd )*
logicAnd    := equality ( "and" equality )*
equality    := comparison ( ( "==" | "!=" ) comparison )*
comparison  := term ( ( "<" | "<=" | ">" | ">=" ) term )*
term        := factor ( ( "+" | "-" ) factor )*
factor      := unary ( ( "*" | "/" | "%" ) unary )*
unary       := ( "-" | "not" ) unary | postfix
postfix     := primary ( "(" args? ")" | "[" expression "]" )*
primary     := NUMBER | STRING | "true" | "false" | "nil" | IDENT
             | "(" expression ")" | "[" args? "]" | "fn" "(" params? ")" block
```

CPython 從 LL(1) 換成 PEG 的教訓：文法工具的限制會反過來扭曲語言設計，例如括號多行 `with` 在 LL(1) 下無法表達。[PEP 617](https://peps.python.org/pep-0617/) Lua 則從 yacc 換成手寫遞迴下降，因為更小、更可移植、可重入、錯誤訊息更好。[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf) 建議原型用手寫遞迴下降加 Pratt 運算式分析，不依賴產生器。

### 值模型

| 選項 | 代表 | 代價 |
| --- | --- | --- |
| 一切皆物件 | Python、Ruby | 整數與布林也是堆積物件，需要小整數快取或永生物件；CPython 3.13 把 `None`、`True`、小整數設為永生以避免參考計數競爭 |
| 少數原生型別加一種容器 | Lua（表）、JavaScript（物件加原型） | 核心小、FFI 簡單；表要同時處理陣列與雜湊兩種用法 |
| 標籤聯集（tagged union） | Lua 的 `TValue`、clox 的 `Value` | C 實作最直接；要決定 NaN-boxing 或 struct |

來源：[PEP 703](https://peps.python.org/pep-0703/)、[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)、[Crafting Interpreters](https://craftinginterpreters.com/contents.html)。

### 作用域與閉包

詞法作用域加一級函式與閉包是 Lua、Python、JavaScript 的共同終點；Lua 在演進中逐步加入完整詞法作用域與匿名函式，受 Scheme 影響。[The Evolution of Lua](https://www.lua.org/doc/hopl.pdf) 建議第一版就做詞法作用域，不做動態作用域，否則之後改不了。

### 錯誤模型

| 選項 | 代表 | 設計時要決定 |
| --- | --- | --- |
| 例外 | Python、JavaScript | 可拋任意值還是只能拋特定型別；跨宿主邊界如何傳遞 |
| 錯誤值加保護呼叫 | Lua 的 `error` 與 `pcall` | 錯誤如何帶位置資訊 |
| 狀態碼 | Node-API 的 `napi_status` | 適合 C API，不適合語言層 |

minilang 選例外（`throw` 與 `try/catch`），執行期錯誤同時帶行號。

驗收：寫 20 個合法程式與 10 個非法程式，人工標記預期結果，之後變成測試。

## 階段 2：樹走訪直譯器

這是第一個「會跑」的里程碑，《Crafting Interpreters》第二部用 10 章完成：掃描、表示程式、解析運算式、求值、陳述與狀態、控制流程、函式、解析與繫結、類別、繼承。[Crafting Interpreters](https://craftinginterpreters.com/contents.html) 《Writing An Interpreter In Go》用同樣順序實作 Monkey 語言，含 Pratt 解析與閉包。[interpreterbook.com](https://interpreterbook.com/)

元件與建議：

| 元件 | 建議做法 | 陷阱 |
| --- | --- | --- |
| 詞法分析 | 手寫狀態機，每個 token 記行號 | 字串逸出、數字格式、註解要在第一版就定 |
| 語法分析 | 遞迴下降加 Pratt 運算式 | 錯誤訊息要指出行號與期望的 token；考慮錯誤恢復但第一版可略 |
| AST | 每種節點一個小結構；Python 原型可用 dataclass 或 kind 加欄位 | 不要過早最佳化成位元組碼 |
| 環境 | 鏈結的字典；函式物件保存定義時的環境 | 閉包捕捉的是變數不是值；迴圈內建立閉包要測 |
| 控制流程 | `return`、`break` 用宿主語言的例外或訊號傳遞 | 遞迴深度受宿主限制，要把宿主的堆疊溢位轉成語言層錯誤 |
| 內建函式 | 以宿主函式包裝，與使用者函式共用呼叫介面 | 輸出要可注入，否則無法測試 |
| REPL | 逐行解析，運算式陳述顯示結果 | 多行輸入需判斷「尚未結束」 |

驗收：測試套件全綠（minilang 用 Python `unittest`）、REPL 可互動、能跑費氏數列與閉包計數器。

## 階段 3：標準函式庫與模組

Lua 第一版只有 I/O、字串與數學函式庫；作業系統、表、協程、模組是後來依需求加的。[The Evolution of Lua](https://www.lua.org/doc/hopl.pdf) 建議只做階段 0 的問題需要的函式庫，並從一開始就區分「核心」與「可選函式庫」，MicroPython 用 `mpconfigport.h` 的 `#define` 逐項開關就是這個原則。[MicroPython porting guide](https://docs.micropython.org/en/latest/develop/porting.html)

模組系統要決定：路徑解析規則、快取、循環載入行為、是否允許執行時從網路載入（iOS 禁止）。[PEP 730](https://peps.python.org/pep-0730/)

驗收：用這門語言寫出一個解決階段 0 問題的小工具。

## 階段 4：位元組碼編譯器與 VM

為什麼要離開樹走訪：AST 走訪每個節點都要分派與指標追蹤；AST 不能序列化成快取；嵌入式環境希望離線預編譯。Lua 為此允許不帶編譯器、只帶載入器。[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)

| 決策 | 選項 | 案例 |
| --- | --- | --- |
| 堆疊式或暫存器式 | 堆疊式指令短、編譯簡單；暫存器式指令數少、要做暫存器分配 | CPython 堆疊式；Lua 5.0 與 V8 Ignition 暫存器式 |
| 編譯路徑 | AST 到位元組碼（可做符號表與 CFG 最佳化）；單趟邊解析邊產生 | CPython 走 AST、符號表、偽指令、CFG、組譯；Lua 無中間表示 |
| 指令編碼 | 固定寬度（CPython 16 位元 code unit 加 `EXTENDED_ARG`）；可變寬度 | 固定寬度易做 inline cache |
| 分派 | `switch`（最可移植）；computed goto；tail call | Lua 為可移植只用 while-switch；CPython 預設 computed goto 並保留 switch 版本 |
| 閉包 | upvalue：區域變數留在堆疊，逃逸時才搬到堆積 | Lua 5.0 的做法，clox 採同一設計 |
| 呼叫框架 | 在 VM 內自管框架，不遞迴宿主堆疊 | CPython 3.11 起 Python 對 Python 呼叫不遞迴 C 堆疊 |
| 程式碼物件 | 常數池、行號表、例外表、最大堆疊深度 | CPython 的 `PyCodeObject` 由組譯階段建立 |

來源：[InternalDocs: compiler](https://github.com/python/cpython/blob/main/InternalDocs/compiler.md)、[InternalDocs: interpreter](https://github.com/python/cpython/blob/main/InternalDocs/interpreter.md)、[V8 docs: Ignition](https://v8.dev/docs/ignition)、[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)。

《Crafting Interpreters》第三部用 17 章完成 C 位元組碼 VM：chunk、VM、掃描、編譯運算式、值型別、字串、雜湊表、全域與區域變數、跳躍、呼叫、閉包、GC、類別、方法、超類別、最佳化。[Crafting Interpreters](https://craftinginterpreters.com/contents.html)

驗收：VM 跑階段 2 的同一套測試結果一致；用固定基準（例如遞迴費氏、迴圈、字串拼接）記錄相對樹走訪的加速倍數。

可執行範例 [minilang-vm](../examples/minilang-vm/README.zh-TW.md) 依這個驗收做了一次，結果是量測值，不是推估：

- **同一套測試。** minilang 的 13 個測試原封不動在 VM 上執行（未改寫也未複製），編譯器最佳化關閉與開啟各跑一次，全部通過；另有差異測試逐行比對兩種引擎的輸出與錯誤訊息（含行號）。範例是單檔約 1,200 行 Python 加約 600 行測試，含常數池、行號表、例外表、計算出的最大堆疊深度、跳躍回填、upvalue 閉包與反組譯器。
- **加速倍數。** 2026-11-16 在 Python 3.14.6、Intel Core i7-11700、Windows 11 上，各引擎交錯執行 5 次取中位數，共量兩次：VM 相對樹走訪在遞迴費氏快 2.64 至 2.80 倍、計數迴圈 1.90 至 1.92 倍、字串拼接 1.45 至 1.50 倍；開啟編譯器最佳化後迴圈到 3.01 至 3.19 倍、字串拼接到 1.90 至 2.00 倍，費氏不變。增益幾乎都來自「指派陳述不產生 DUP 與 POP」；兩個超指令（常數運算元、比較加跳躍）在雜訊範圍內。
- **加速從哪裡來。** VM 執行的指令數和樹走訪拜訪的節點數差不多（每次費氏呼叫 11 道指令對 11.5 個節點），差別在樹走訪每個節點都是一次以上的 Python 函式呼叫、`return` 靠宿主例外傳遞、每個區塊建立一個字典環境；VM 把這些換成一個迴圈裡的串列索引。在 Python 裡每道指令仍約 140 至 200 奈秒（以中位數除以計數推算），所以這個倍數是宿主語言的性質，不能拿來預估 C 版本；C 版本在階段 5 量測：minilang-c 執行同一份位元組碼，在呼叫與迴圈上比 Python VM 快 81 至 154 倍。
- **對階段順序的意涵。** 先做樹走訪的建議成立，而且理由比效能更實際：VM 是對著一個會跑的參考實作與它的測試寫的，差異測試與隨機程式比對才有仲裁依據。以 Python 原型做階段 4，價值在驗證解析器、程式碼物件、閉包與例外表的設計，不在速度；要拿到位元組碼的效能，仍要照「關鍵決策一覽」把正式版移到 C 子集。
- **被迫寫下來的語意。** 做 VM 時發現 minilang 有幾個只由樹走訪實作決定的行為：閉包看得到外層之後才 `let` 的變數、函式內的 `break` 會中斷呼叫端的迴圈、頂層 `return` 以宿主例外逸出、`throw nil` 被接成字串 `"nil"`。VM 全部照做以維持一致，但這些應該在階段 1 寫進語意草案，或在階段 8 的規格裡明定。

## 階段 5：記憶體管理

| 方案 | 優點 | 代價 | 案例 |
| --- | --- | --- | --- |
| 參考計數 | 釋放時機確定；與 C 互通簡單；實作直接 | 循環要另外處理；多執行緒下計數更新成本高 | CPython（加循環 GC）、QuickJS（加循環移除） |
| 標記清除 | 處理循環；核心小 | 需要精確根集合；停頓時間 | Lua、clox |
| 分代或增量 | 短停頓 | 需要寫屏障或 handle 機制 | V8（分代）、Lua 5.1（增量）、PyPy incminimark |

CPython 的經驗說明參考計數在多執行緒下的代價：為了拿掉 GIL，3.13 引入偏置參考計數、永生物件、延遲參考計數，GC 改為非分代 stop-the-world。[PEP 703](https://peps.python.org/pep-0703/) V8 的經驗說明移動式 GC 的 API 代價：所有物件透過 handle 存取，`HandleScope` 結束時釋放，持久 handle 另外管理。[V8 embed](https://v8.dev/docs/embed)

建議：原型用宿主語言的 GC（minilang 用 Python 的）；C 實作第一版用標記清除，根集合只包含 VM 堆疊、全域表與開放 upvalue；在測試中加入「配置時強制 GC」模式找出漏掉的根。

驗收：長時間執行的測試記憶體不增長；強制 GC 模式下測試全綠。

可執行範例 [minilang-c](../examples/minilang-c/README.zh-TW.md) 依這個驗收做了一次，結果是量測值，不是推估：

- **前端不重寫。** minilang-vm 新增 `--emit`，把程式碼物件寫成有版本號、有文件的位元組碼檔（常數、指令、行號表、例外表、迴圈表、最大堆疊深度、upvalue 描述）；C VM 只做載入、驗證、執行與記憶體管理。這同時驗證了階段 4「可離線預編譯、只帶載入器」的動機。載入器在執行前檢查每個運算元與表格，並重算每條路徑的堆疊深度，損壞與截斷的檔案都被拒絕。
- **同一套測試，強制 GC。** minilang 的測試與 minilang-vm 的差異測試不改一行在 C VM 上執行，含每次配置都回收的 `--gc-stress`，共 94 個測試全部通過；10,000 個隨機程式在 6 種設定（一般與最佳化位元組碼，各配預設堆積、4 KiB 堆積與壓力模式）下與樹走訪逐行比對，60,048 次執行全部一致。在 Ubuntu 容器以 AddressSanitizer 與 UBSan 建置，再跑測試與另外 2,000 個程式，沒有任何報告。
- **長時間執行記憶體不增長。** 每輪產生字串、串列、閉包與 cell 的程式，跑 20,000 輪與 2,000 輪的峰值相差不到 25%；2 萬個包含自己的串列與 2 萬個呼叫自己的區域函式全部被回收；每個 VM 拆除後記帳剛好歸零。
- **為什麼是標記清除。** minilang 裡每個會呼叫自己的區域函式都是循環（閉包持有 cell，cell 持有閉包），只靠參考計數一定洩漏；標記清除不移動物件，C 程式碼可以直接持有已列入根的指標，不必像 V8 那樣全面使用 handle。回收器本身約 160 行，落在上表「數百至 1 千行」的下緣以下；整個 C VM 約 3,150 行，其中約 700 行只為了重現 Python 的數值與文字格式（見下）。
- **根集合與建議不同。** 除了 VM 堆疊與全域表，還需要框架的程式碼物件、C 程式碼連續配置兩次之間的暫時根、正被丟出的值；intern 表必須是弱引用。開放 upvalue 則不存在：階段 4 的位元組碼在作用域進入時就建立 cell，代價是每次進入作用域配置一次。
- **強制 GC 模式找得到漏根，但不是全部。** 一次性植入 7 個根集合錯誤，其中 6 個是真的錯誤。預設堆積下只有 1 個現形；壓力模式下的隨機程式抓到 4 個；ASan 下的測試套件抓到 5 個，其中 1 個要靠隨機程式漏掉後補寫的針對性測試，另 1 個（intern 表不是弱引用）只會讀到已釋放的記憶體，只有 ASan 看得出。三種方法合起來才抓到全部。第 7 個（不標記框架的閉包）不是錯誤，因為閉包本來就在堆疊上。
- **C 版的加速倍數，補上階段 4 的缺口。** 同一台機器上，C VM（-O2）執行同一份位元組碼：遞迴費氏 5.8 毫秒、計數迴圈 21.8 毫秒、字串拼接 107 毫秒。比 Python VM 快 81 至 154 倍（費氏與迴圈）與 8 至 12 倍（拼接），比樹走訪快 207 至 312 倍與 12 至 19 倍；範圍來自 Python 端的計時雜訊與兩種基準算法，條件見範例說明。以階段 4 的指令數推算，每道指令約 1.3 至 2.2 奈秒。
- **GC 的成本在清除，不在標記。** 拼接配置 600,021 個物件、回收 66 次，回收佔執行時間 29% 至 30%，最長停頓約 1 毫秒；標記合計只有 0.03 毫秒，時間幾乎都花在逐一釋放死字串。對這類程式，下一步是配置器或新生代，不是指令特化。
- **被迫寫下來的語意。** 樹走訪的數值與文字格式就是 Python 的：沒有上限的整數、整除時才得整數的 `/`、整數與浮點數的精確比較、`repr()` 的浮點數格式、`int()` 與 `float()` 的文法（含底線、`inf`、Unicode 空白）、4,300 位數的轉換上限、以 `repr()` 顯示的錯誤訊息、以碼位計數的字串，以及極大整數轉浮點數時 `try` 接不到的 `OverflowError`。C VM 全部照做，但這些應該在階段 1 寫進語意草案，或在階段 8 的規格裡明定，否則每個實作都得重現某個特定的宿主。

## 階段 6：宿主互通

C API 設計原則，從三個成功案例歸納：

| 原則 | 案例 |
| --- | --- |
| 公開 API 只用不透明指標與 C 內建型別 | Lua 的 C API 主要依賴內建 C 型別；Node-API 的 `napi_value` 不透明 |
| 每個呼叫回傳狀態碼，錯誤資訊另外取 | Node-API 每個函式回傳 `napi_status` |
| 內部結構不進公開標頭 | CPython 限制 API 隱藏 `PyTypeObject` 等結構，型別用 `PyType_FromSpec` 建立 |
| 版本化 | Node-API 以 `NAPI_VERSION` 選版本；CPython 以 `Py_LIMITED_API` 指定最低版本 |
| 錯誤跨邊界有定義 | Pyodide 的 FFI 支援錯誤在 JavaScript 與 Python 間傳遞 |

來源：[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)、[Node-API](https://nodejs.org/api/n-api.html)、[PEP 384](https://peps.python.org/pep-0384/)、[Pyodide](https://pyodide.org/en/stable/)。

嵌入式語言還要決定全域狀態：V8 用 `Isolate`（獨立堆積）與 `Context`（獨立全域物件）讓多個不相干的腳本共存。[V8 embed](https://v8.dev/docs/embed) 建議從第一天就把所有狀態放在一個 `State` 結構並由每個 API 函式接收，不要用 C 全域變數。

驗收：用 C 寫一個擴充函式並從語言呼叫；用 C 宿主程式嵌入並執行一段腳本。

## 階段 7：平台與散布

依[研究二](02-portability-and-runtime-evaluation.zh-TW.md)的六層逐項落實：

1. **先寫支援政策再加平台。** 仿 PEP 11 或 BUILDING.md 列出 Tier、負責人與阻擋發布的規則。[PEP 11](https://peps.python.org/pep-0011/)、[BUILDING.md](https://github.com/nodejs/node/blob/main/BUILDING.md)
2. **限制 C 子集。** 仿 PEP 7：選定一個 C 標準、禁用編譯器專屬擴充、零警告；需要的非標準能力用自家巨集包裝以便替換。[PEP 7](https://peps.python.org/pep-0007/)
3. **平台相依收進一層。** 仿 libuv 或 MicroPython 的 `mphalport`。[libuv design](https://docs.libuv.org/en/v1.x/design.html)、[MicroPython porting guide](https://docs.micropython.org/en/latest/develop/porting.html)
4. **功能可在建置時關閉。** 子程序、執行緒、檔案系統、JIT 都要能關，關掉後語言層給明確錯誤，CPython 在 iOS 與 WASI 就是這樣處理 `subprocess`。[PEP 730](https://peps.python.org/pep-0730/)
5. **CI 矩陣對應 Tier。** Tier 1 平台失敗擋合併。
6. **散布方式。** 單一可執行檔（Node.js SEA 的作法是把腳本 blob 注入二進位）、二進位套件標籤（manylinux 以 glibc 版本命名）、或只發原始碼。[Node.js SEA](https://nodejs.org/api/single-executable-applications.html)、[PEP 600](https://peps.python.org/pep-0600/)

驗收：每個 Tier 1 平台 CI 綠燈；至少一個受限目標（WASI 或微控制器）能跑核心測試子集。

## 階段 8：規格與流程

語言成功後才需要，但要預留：

- **參考手冊**：描述語言而非實作，必要處加「實作註記」，這是 Python 語言參考的作法。[語言參考簡介](https://docs.python.org/3/reference/introduction.html)
- **相容性政策**：Python 的 PEP 387 與五年支援期；Node.js 的 Current 與 LTS。[PEP 602](https://peps.python.org/pep-0602/)、[Node.js releases](https://nodejs.org/en/about/previous-releases)
- **提案流程**：PEP 1 的 Motivation、Specification、Rationale、Backwards Compatibility、Reference Implementation；TC39 的 Stage 0 到 4 與「兩個相容實作通過 Test262」。[PEP 1](https://peps.python.org/pep-0001/)、[TC39 process](https://tc39.es/process-document/)
- **一致性測試**：Test262 模式，測試描述可觀察行為而非實作細節。[test262](https://github.com/tc39/test262)

驗收：他人能依手冊實作語言子集並通過一致性測試。

## 效能演進的順序

不要從 JIT 開始。V8 與 CPython 的路線都是：直譯器先正確，再做特化或基線層，最後才是最佳化 JIT。

| 步驟 | 技術 | 案例與數字 |
| --- | --- | --- |
| 1 | 正確的位元組碼直譯器 | Lua while-switch；V8 Ignition |
| 2 | 指令特化加 inline cache | CPython 3.11 依基準約 10% 至 60%；V8 2008 年的 hidden class 與 inline cache |
| 3 | 基線 JIT | V8 Sparkplug，幾乎立即把位元組碼變機器碼 |
| 4 | 中層最佳化 JIT | V8 Maglev，填補基線與頂層的空隙 |
| 5 | 頂層最佳化 JIT | V8 TurboFan；CPython 3.13 copy-and-patch JIT 仍在實驗，記憶體高 10% 至 20% |

來源：[PEP 659](https://peps.python.org/pep-0659/)、[Chromium Blog 2008](https://blog.chromium.org/2008/09/google-chromes-need-for-speed_02.html)、[V8 blog: Maglev](https://v8.dev/blog/maglev)、[PEP 744](https://peps.python.org/pep-0744/)。

minilang-c 的量測給這張表兩個補充：步驟 1 本身就是最大的一步，把同一份位元組碼從 Python 移到 C 就快了兩個數量級，C 編譯器的 -O2 對 -O0 又是 5.6 至 8.4 倍；而在配置密集的程式裡，移到 C 之後剩下的成本是配置與回收，不是分派，步驟 2 之後的技術對它幫助有限，要先處理記憶體管理。

## 關鍵決策一覽

| 決策 | 建議預設 | 何時改變 |
| --- | --- | --- |
| 宿主語言 | 原型用高階語言；正式版用 C 子集 | 團隊熟 Rust 或 C++ 且目標平台工具鏈齊全 |
| 值模型 | 標籤聯集加少數堆積型別 | 需要一切皆物件的反射能力 |
| 作用域 | 詞法作用域加閉包 | 幾乎不改 |
| 錯誤 | 例外，帶行號 | 嵌入式宿主偏好錯誤值 |
| 並行 | 單執行緒加協程或事件迴圈 | 確定需要共享記憶體多執行緒，且願意承擔 ABI 與 GC 複雜度 |
| 記憶體 | 標記清除 | 需要確定性釋放時用參考計數加循環處理 |
| 執行 | 樹走訪到位元組碼 | 嵌入式或啟動時間敏感時可考慮離線預編譯 |
| 嵌入 | 所有狀態放在 `State` 結構 | 幾乎不改 |
| API | 不透明指標、狀態碼、版本巨集 | 幾乎不改 |

## 常見陷阱

- **先做 JIT 或先做多執行緒。** 兩者都要在值模型與 GC 固定後才做，CPython 的 free-threading 花了多個版本且仍有 1% 至 8% 單執行緒代價。[Free-threading HOWTO](https://docs.python.org/3/howto/free-threading-python.html)
- **文法工具限制反過來決定語法。** PEP 617 的教訓。
- **內部結構外露成公開 API。** PEP 384 的教訓：之後每次改記憶體配置都破壞擴充。
- **沒有可注入的輸出與時鐘。** 直譯器無法做決定性測試。
- **把「某平台編得過」當支援。** 沒有 CI 與負責人的平台在 PEP 11 下就是不支援。
- **沒有一致性測試就出現第二個實作。** 之後兩邊行為分歧沒有仲裁依據。minilang-vm 以 minilang 的測試加差異測試當仲裁，仍要照抄樹走訪實作的巧合行為，見階段 4。
- **用高階宿主語言的原型量 VM 加速倍數，再推論正式版。** minilang-vm 在 Python 上比樹走訪快 1.45 至 2.80 倍，來源是省掉宿主的函式呼叫與例外；在 Python 的 if 鏈分派裡，連多加幾個指令都會拖慢其他指令（範例說明有量測）。C 實作的分派成本結構不同，要另外量：minilang-c 執行同一份位元組碼比 Python VM 快 81 至 154 倍，而階段 4 的三項最佳化在 C 上只有 0.94 至 1.27 倍。
- **只靠強制 GC 模式找漏掉的根。** 這個模式只抓得到測試程式實際走過的路徑。minilang-c 植入的 6 個真錯誤中，壓力模式下的隨機程式抓到 4 個；其餘要靠針對性測試或 AddressSanitizer。
- **以為 GC 的成本在標記。** 標記清除的清除階段與整個堆積成正比；minilang-c 的字串拼接有 30% 時間在回收器，其中標記不到回收時間的 0.1%，其餘都在釋放死物件。

## 規模與時間推估的假設

階段 2 的 1 千至 3 千行依 minilang（Python 約 900 行含測試）與 jlox 規模推估；階段 4 的 3 千至 8 千行依 clox 與 Lua 核心約 32,000 行 C 中編譯器約佔 30% 推估；都不含標準函式庫。實際量到的一個點：minilang-c 的 C VM 不含前端約 3,150 行，其中回收器約 160 行、任意精度整數與數值文字約 660 行。[Lua about](https://www.lua.org/about.html)、[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf) 時間以一人熟悉宿主語言為前提；Python 從開工到公開約 14 個月，Lua 約 2 個月到可用，差異來自目標範圍。
