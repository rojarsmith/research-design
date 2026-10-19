# Python 與 Node.js 如何從 0 到有

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-19。本文把三條歷史線放在一起看：CPython（1989 年起）、V8 與 Node.js（2008 至 2009 年起）、Lua（1993 年起，當作「刻意追求可移植與可嵌入」的對照）。每條線都回答同一組問題：起點是什麼問題、借了什麼現成基礎、第一版長什麼樣、後來怎麼制度化。

## 一、CPython：一個人的假期專案如何變成語言

### 起點與借用

Guido van Rossum 在 CWI 的 ABC 語言小組有實作直譯語言的經驗，之後到 Amoeba 分散式作業系統小組工作。Amoeba 的系統呼叫介面無法從 Bourne shell 使用，寫 C 程式又太慢，他判斷「一個語法像 ABC、但能存取 Amoeba 系統呼叫的腳本語言」可以填補需求。因為只做 Amoeba 專用語言太浪費，他選擇做一個「通用可擴充」的語言。他明確指出 ABC 最大的問題之一是不可擴充。[Python 官方 FAQ](https://docs.python.org/3/faq/general.html)

從既有語言借來的東西：ABC 的縮排分組與高階資料型別；Modula-3 的例外語法與語意；Amoeba 工作經驗讓他重視例外處理。[Python 官方 FAQ](https://docs.python.org/3/faq/general.html)

### 時間線

| 時間 | 事件 |
| --- | --- |
| 1989 年 12 月 | 聖誕假期開始實作，之後一年多主要用自己的時間開發 |
| 1990 年 | CWI 內部版本 |
| 1991 年 2 月 20 日 | 0.9.0 發布到 Usenet 的 alt.sources |
| 1994 年 1 月 26 日 | 1.0.0 |
| 2000 年 10 月 16 日 | 2.0 |
| 2008 年 12 月 3 日 | 3.0 |

來源：[Guido van Rossum 的 Python 歷史時間線](https://python-history.blogspot.com/2009/01/brief-timeline-of-python.html)、[Python 官方 FAQ](https://docs.python.org/3/faq/general.html)。

### 第一版的技術形態

官方 FAQ 描述 Python 為直譯式、互動式、物件導向，可用 C 或 C++ 擴充；原始碼發行版是完整的 C 原始碼，在多數 Unix 平台可直接編譯執行。[Python 官方 FAQ](https://docs.python.org/3/faq/general.html) 語言參考手冊說明 CPython 是以 C 寫成的原始實作，新語言功能通常先出現在這裡；Jython（Java）、IronPython（.NET）、PyPy（以 Python 寫的 Python，含 JIT）是替代實作，手冊刻意描述「語言」而不是某個實作。[語言參考簡介](https://docs.python.org/3/reference/introduction.html)

這裡有一個對自製語言很重要的事實：**Python 的規格書是在實作之後才寫的，而且至今沒有 ISO 標準**。語言由參考手冊加 PEP 流程定義。PEP 1 定義 Standards Track、Informational、Process 三種提案，狀態從 Draft 到 Accepted、Final，Steering Council 是最終決定者，Standards Track PEP 必須有 Motivation、Specification、Rationale、Backwards Compatibility，且要有參考實作才能 Final。[PEP 1](https://peps.python.org/pep-0001/)

### 今天的 CPython 編譯與執行管線

CPython 維護者文件描述從原始碼到 `PyCodeObject` 的階段。以下檔案名稱以查閱日的 main 分支為準，版本間會變動。[InternalDocs: compiler](https://github.com/python/cpython/blob/main/InternalDocs/compiler.md)

| 階段 | 做什麼 | 主要檔案 |
| --- | --- | --- |
| 詞法分析 | 原始碼切成 token | `Parser/lexer/`、`Parser/tokenizer/` |
| PEG 語法分析 | 3.9 起由 PEG 取代 LL(1)；文法在 `Grammar/python.gram`，分析器由文法產生 | `Parser/parser.c` |
| AST | 節點型別以 ASDL 定義，記憶體用 arena 一次釋放 | `Parser/Python.asdl`、`Python/Python-ast.c` |
| 符號表 | 走訪 AST 決定變數作用域 | `Python/symtable.c` |
| 偽指令 | 走訪 AST 產生接近位元組碼的偽指令 | `Python/compile.c`、`codegen.c` |
| 控制流程圖最佳化 | 基本區塊、peephole 最佳化 | `Python/flowgraph.c` |
| 組譯 | 解析跳躍目標、例外表、位置表，建立 code object | `Python/assemble.c` |
| 執行 | 評估迴圈 | `Python/ceval.c` |

PEG 換掉 LL(1) 的原因是 LL(1) 逼得文法與 AST 建構程式繞路，有些語法（例如括號多行 `with`）無法表達；新分析器速度與記憶體控制在舊分析器的 10% 內。[PEP 617](https://peps.python.org/pep-0617/)

直譯器本體是堆疊機：每個指令是 16 位元 code unit，前 8 位元為 opcode、後 8 位元為 oparg；指令以自訂 DSL 寫在 `Python/bytecodes.c`，由 `make regen-cases` 產生 `generated_cases.c.h`；3.11 起呼叫框架是輕量的 `_PyInterpreterFrame`，Python 對 Python 的呼叫不再遞迴 C 堆疊；分派方式有 switch、computed goto（預設）與需要特定編譯器支援的 tail-calling 直譯器。[InternalDocs: interpreter](https://github.com/python/cpython/blob/main/InternalDocs/interpreter.md)

### 效能與並行的演進

| 版本 | 變更 | 來源 |
| --- | --- | --- |
| 3.11 | 特化自適應直譯器：指令在執行時依觀察到的型別替換成特化版本，使用 inline cache；PEP 稱實驗中最高可達 50% 加速，依基準不同約 10% 至 60% | [PEP 659](https://peps.python.org/pep-0659/) |
| 3.13 | copy-and-patch JIT：實驗性，預設關閉，建置時需 LLVM，執行時不需；畢業條件之一是在至少一個主流平台有約 5% 的有意義加速 | [PEP 744](https://peps.python.org/pep-0744/) |
| 3.13 | free-threaded 建置（`--disable-gil`，ABI 標籤加 `t`）：偏置參考計數、永生物件、mimalloc 取代 pymalloc、容器的逐物件鎖、非分代的 stop-the-world GC；與標準建置 ABI 不相容 | [PEP 703](https://peps.python.org/pep-0703/) |
| 3.13 以後 | 官方 HOWTO 指出單執行緒額外負擔依平台約 1% 至 8%；未標示支援 free-threading 的擴充會讓 GIL 重新啟用 | [Free-threading HOWTO](https://docs.python.org/3/howto/free-threading-python.html) |

記憶體管理以參考計數為主，循環垃圾回收為輔。預設建置把容器物件分成三代，`gc.get_threshold()` 預設 `(2000, 10, 10)`；free-threaded 建置移除 `PyGC_Head`、改掃描 mimalloc 堆積、不分代、每次回收停止所有執行緒。[InternalDocs: garbage collector](https://github.com/python/cpython/blob/main/InternalDocs/garbage_collector.md)

### 制度化：發布週期與平台政策

- 每年 10 月發布一個功能版本；開發期約 17 個月；前兩年提供 bugfix，再三年只提供安全修正，共五年支援。[PEP 602](https://peps.python.org/pep-0602/)
- 3.11 起 C 程式碼使用不含選用功能的 C11，公開 C API 需與 C99 及 C++ 相容；禁止 GCC 或 MSVC 專屬擴充；主要編譯器必須零警告。[PEP 7](https://peps.python.org/pep-0007/)
- 平台支援分層，見[研究二](02-portability-and-runtime-evaluation.zh-TW.md)。[PEP 11](https://peps.python.org/pep-0011/)

## 二、V8 與 Node.js：先有引擎，再有執行環境

### V8（2008 年）

2008 年 9 月 2 日 Chromium 官方部落格由 Lars Bak 介紹 V8：為效能從頭設計的開源 JavaScript 引擎，把 JavaScript 直接編譯成原生機器碼，並用 hidden class 與 inline cache 加速屬性存取；同日 Google 宣布 Chrome、Chromium 與 V8 以 BSD 授權開源。[Chromium Blog: Google Chrome's Need for Speed](https://blog.chromium.org/2008/09/google-chromes-need-for-speed_02.html)、[Google Developers Blog](https://developers.googleblog.com/2008/09/google-chrome-chromium-and-v8-launch.html)

今天的 V8 以 C++ 寫成，實作 ECMAScript 與 WebAssembly，官方支援 Windows、macOS、Linux 上的 x64、IA-32、ARM；IBM i、AIX、MIPS、ppcle64、s390x 由外部維護。它可以嵌入任何 C++ 應用；DOM 不屬於 V8，由瀏覽器提供。垃圾回收器是 stop-the-world、分代、精確式。[V8 docs](https://v8.dev/docs)

執行層級（2023 年 12 月 Maglev 文章）：

| 層 | 角色 |
| --- | --- |
| Ignition | 暫存器式位元組碼直譯器；所有 JavaScript 先編成位元組碼執行，並記錄物件形狀與型別回饋 |
| Sparkplug | 2021 年加入的基線編譯器，幾乎立即把位元組碼變成機器碼，加速有限但穩定 |
| Maglev | Chrome M117 加入的中層最佳化編譯器，以 SSA IR 與執行回饋產生特化碼，假設失敗可 deoptimize |
| TurboFan | 最上層最佳化編譯器，產生高度最佳化的推測性機器碼，只對熱點划算 |

多層存在的理由是編譯成本與程式碼品質的取捨；真實網頁大量時間花在永遠不夠熱、進不了 TurboFan 的函式上，Maglev 填補這段，並讓 TurboFan 可以等更久再出手。文章報告 JetStream 能耗降 3.5%、Speedometer 降 10%。[V8 blog: Maglev](https://v8.dev/blog/maglev)、[V8 docs: Ignition](https://v8.dev/docs/ignition)

V8 版本號 `x.y.z.w` 的 `x.y` 是 Chromium 里程碑除以 10，嵌入者應追隨最新穩定版 Chrome 對應的分支頭，而不是數字最大的 tag。[V8 docs: version numbers](https://v8.dev/docs/version-numbers)

### Node.js（2009 年）

Node.js 官方介紹的定義：開源、跨平台的 JavaScript 執行環境，在瀏覽器之外執行 Chrome 核心的 V8 引擎；應用在單一程序中執行，不為每個請求開執行緒；標準函式庫提供非同步 I/O 原語，I/O 不阻塞執行緒，回應回來時再恢復操作，因此單一伺服器可處理數千個並行連線。[Introduction to Node.js](https://nodejs.org/en/learn/getting-started/introduction-to-nodejs)、[About Node.js](https://nodejs.org/en/about)

歷史日期的官方頁面沒有記載。二手來源一致指出 Ryan Dahl 在 2009 年 11 月於柏林的第一屆 JSConf EU 展示 Node.js，早期版本在 2009 年 5 至 6 月已公開，原名 web.js。這些細節來自 [Wikipedia](https://en.wikipedia.org/wiki/Node.js) 與 [The Register](https://www.theregister.com/2011/03/01/the_rise_and_rise_of_node_dot_js?page=4)，本研究僅視為二手紀錄。

Node.js 的技術堆疊是「借用加黏合」：

| 層 | 內容 | 來源 |
| --- | --- | --- |
| 語言引擎 | V8，Node.js 自己不實作 JavaScript | [V8 docs](https://v8.dev/docs) |
| 事件迴圈與 I/O | libuv，原本就是為 Node.js 寫的 C 函式庫；在 Linux 用 epoll、macOS 與 BSD 用 kqueue、SunOS 用 event ports、Windows 用 IOCP；網路 I/O 在迴圈執行緒，檔案 I/O 與 `getaddrinfo` 走執行緒池 | [libuv design overview](https://docs.libuv.org/en/v1.x/design.html) |
| 原生擴充 ABI | Node-API：與 V8 無關的 C API，跨主版本 ABI 穩定；直接用 `v8.h`、`node.h`、`uv.h` 則不保證 | [Node-API](https://nodejs.org/api/n-api.html) |
| 語言規格 | ECMA-262，1997 年 6 月第一版，現行 2026 年 6 月第 17 版，由 TC39 維護 | [ECMA-262](https://ecma-international.org/publications-and-standards/standards/ecma-262/) |

libuv 的事件迴圈每輪依序：更新時間、跑到期計時器、檢查迴圈是否存活、跑 pending 回呼、idle、prepare、計算 poll 逾時、阻塞等待 I/O 並觸發回呼、check、close 回呼。迴圈本身不是執行緒安全的；多個迴圈必須各在自己的執行緒。[libuv design overview](https://docs.libuv.org/en/v1.x/design.html)

### 制度化：規格流程與發布週期

TC39 以 Stage 0 到 4 推進提案。Stage 1 要有 champion 與問題描述，Stage 2 要有初步規格文字，Stage 3 設計完成只接受實作回饋，Stage 4 要求兩個相容實作通過 Test262、有實際上線經驗，並向 ecma262 送出整合規格的 PR。[TC39 process](https://tc39.es/process-document/) Test262 是官方一致性測試套件，2025 年 5 月時超過 50,000 個測試檔，涵蓋 ECMA-262、ECMA-402、ECMA-404。[test262](https://github.com/tc39/test262)

Node.js 的大版本在 Current 階段六個月，LTS 共約 30 個月；Node.js 27 起改為每年一個大版本、每個大版本都進 LTS，並在 Current 前多六個月 Alpha。查閱日 v26 為 Current，v24 與 v22 為 LTS。[Node.js previous releases](https://nodejs.org/en/about/previous-releases)

## 三、Lua：刻意為可移植與可嵌入而設計的對照組

Lua 1993 年生於巴西 PUC-Rio 的 Tecgraf 實驗室。前身是兩個為 Petrobras 石油公司做的小語言：DEL（資料輸入語言）與 SOL（簡單物件語言）。SOL 團隊在 1993 年 3 月完成初版但從未交付；1993 年年中三位作者討論 DEL 與 SOL 後決定做一個通用的設定語言，並因為 Tecgraf 客戶的電腦種類非常雜，要求以 C 函式庫形式提供並且可移植。1993 年 7 月 Waldemar Celes 完成第一版實作，詞法與語法分析用 lex 與 yacc 這類傳統 Unix 工具；同年 10 月在巴西軟體工程研討會發表。[The Evolution of Lua（HOPL III）](https://www.lua.org/doc/hopl.pdf)

作者在 2005 年論文列出五個實作目標，對自製語言很有參考價值：簡單、效率、可移植（嚴格 ANSI C、避免 C 的陰暗角落、也能當 C++ 編譯、零警告）、可嵌入（簡單有力的 C API，主要只用 C 內建型別）、低嵌入成本。為了可移植，Lua 不用 direct threaded code 這類技巧，只用標準 while-switch 分派；3.0 之後改用手寫遞迴下降分析器，因為更小、更快、更可移植、可重入、錯誤訊息更好；編譯器沒有中間表示，邊解析邊直接產生 VM 指令。Lua 5.0 是第一個廣泛使用的暫存器式 VM；閉包的區域變數留在堆疊，只在離開作用域且仍被巢狀函式引用時才搬到堆積。[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)

今天的 Lua 約 32,000 行 C，64 位元 Linux 上完整直譯器 293K，MIT 授權（自 5.0 起），可在 Unix、Windows、行動平台、嵌入式微處理器與 IBM 大型主機執行。[Lua about](https://www.lua.org/about.html)

## 四、跨語言歸納

| 觀察 | CPython | V8 / Node.js | Lua |
| --- | --- | --- | --- |
| 第一版花多久 | 約 14 個月到公開 | V8 隨 Chrome 發布；Node.js 數月內公開 | 約 2 個月到可用，4 個月到發表 |
| 第一版分析器 | 手寫（LL(1) 時代） | 不適用（V8 自有） | yacc 產生，後改手寫 |
| 執行形態 | 堆疊式位元組碼 | 位元組碼加多層 JIT | 暫存器式位元組碼 |
| 記憶體 | 參考計數加循環 GC | 分代追蹤式 GC | 追蹤式 GC（5.1 起增量） |
| 可擴充方式 | C 擴充模組；後有穩定 ABI | Node-API；V8 模板嵌入 API | C API 是核心設計 |
| 規格 | 參考手冊加 PEP；無 ISO | ECMA-262 與 TC39 | 參考手冊 |
| 平台政策 | PEP 11 三層 | BUILDING.md 三層 | 「有 ANSI C 編譯器就能跑」 |

其他動態語言的簡短對照，供延伸閱讀：Ruby 由 Matz 設計、1995 年首次公開，參考實作 MRI 以 C 寫成，並有 ISO/IEC 30170:2012 標準（2012 年 4 月，313 頁，JTC 1/SC 22）。[Ruby about](https://www.ruby-lang.org/en/about/)、[ISO/IEC 30170:2012](https://www.iso.org/standard/59579.html) PyPy 用 RPython 寫直譯器再翻譯成 C，JIT 追蹤的是直譯器而非使用者程式，因此可套用到任何以 RPython 寫的直譯器。[PyPy architecture](https://doc.pypy.org/en/latest/architecture.html) QuickJS 以少數 C 檔實作 ES2025，hello world 約 367 KiB x86 碼，runtime 完整生命週期低於 300 微秒，MIT 授權，用參考計數加循環移除。[QuickJS](https://bellard.org/quickjs/) MicroPython 是 Python 3 的精簡實作，可在 256K 程式空間與 16K RAM 內執行。[MicroPython](https://micropython.org/)
