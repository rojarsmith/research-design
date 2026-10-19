# 自製動態語言路線圖

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-19。本文把「從 0 設計一門動態語言」拆成九個階段，每階段說明要做什麼、怎麼驗收、歷史上的語言怎麼選、常見陷阱。階段順序與 Python、Lua 的實際歷史以及《Crafting Interpreters》的兩部結構一致：**先做會跑的樹走訪直譯器，再做位元組碼 VM，最後才談 JIT 與多平台**。行數與時間是依公開教材與參考實作推估，不是量測。

可執行範例 [minilang](../examples/minilang/README.zh-TW.md) 對應階段 2，用 Python 寫成單檔直譯器，約 900 行含測試。

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

## 階段 5：記憶體管理

| 方案 | 優點 | 代價 | 案例 |
| --- | --- | --- | --- |
| 參考計數 | 釋放時機確定；與 C 互通簡單；實作直接 | 循環要另外處理；多執行緒下計數更新成本高 | CPython（加循環 GC）、QuickJS（加循環移除） |
| 標記清除 | 處理循環；核心小 | 需要精確根集合；停頓時間 | Lua、clox |
| 分代或增量 | 短停頓 | 需要寫屏障或 handle 機制 | V8（分代）、Lua 5.1（增量）、PyPy incminimark |

CPython 的經驗說明參考計數在多執行緒下的代價：為了拿掉 GIL，3.13 引入偏置參考計數、永生物件、延遲參考計數，GC 改為非分代 stop-the-world。[PEP 703](https://peps.python.org/pep-0703/) V8 的經驗說明移動式 GC 的 API 代價：所有物件透過 handle 存取，`HandleScope` 結束時釋放，持久 handle 另外管理。[V8 embed](https://v8.dev/docs/embed)

建議：原型用宿主語言的 GC（minilang 用 Python 的）；C 實作第一版用標記清除，根集合只包含 VM 堆疊、全域表與開放 upvalue；在測試中加入「配置時強制 GC」模式找出漏掉的根。

驗收：長時間執行的測試記憶體不增長；強制 GC 模式下測試全綠。

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
- **沒有一致性測試就出現第二個實作。** 之後兩邊行為分歧沒有仲裁依據。

## 規模與時間推估的假設

階段 2 的 1 千至 3 千行依 minilang（Python 約 900 行含測試）與 jlox 規模推估；階段 4 的 3 千至 8 千行依 clox 與 Lua 核心約 32,000 行 C 中編譯器約佔 30% 推估；都不含標準函式庫。[Lua about](https://www.lua.org/about.html)、[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf) 時間以一人熟悉宿主語言為前提；Python 從開工到公開約 14 個月，Lua 約 2 個月到可用，差異來自目標範圍。
