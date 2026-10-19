# 動態語言從 0 到 1：Python 與 Node.js 的誕生、移植評估與自製語言路線

[English overview](README.md)

研究日期：2026-10-19。對象是想理解 Python、Node.js 這類動態語言如何「從無到有」，並評估一門語言的移植性與執行環境，最後自己設計一門動態語言的工程師。歷史與技術事實附官方或作者本人來源；評估表、階段計畫與範例是本研究提出的建議，不是任何社群的官方標準。

一句話結論：**動態語言不是先有「語言」再有「平台」，而是先有一個解決具體問題的小直譯器，用 C 寫成、依附既有作業系統與 C 編譯器，然後靠擴充機制、規格流程與平台支援政策一層層長成生態。** 自製語言照同樣順序走：先有一個能跑的樹走訪直譯器，再決定要不要位元組碼、垃圾回收、JIT 與多平台。

## 閱讀入口

| 文件 | 解決的問題 |
| --- | --- |
| 本頁 | 三個問題的總答案、跨語言對照、取捨原則 |
| [Python 與 Node.js 如何從 0 到有](research/01-how-python-and-nodejs-came-to-be.zh-TW.md) | CPython、V8、libuv、Node.js、Lua 的誕生順序、技術堆疊與演進里程碑 |
| [移植性與執行環境評估](research/02-portability-and-runtime-evaluation.zh-TW.md) | 平台支援層級、ABI、相依層、嵌入式與 WebAssembly、評估檢查表 |
| [自製動態語言路線圖](research/03-design-your-own-dynamic-language.zh-TW.md) | 從規格到樹走訪直譯器、位元組碼 VM、GC、FFI、工具鏈的階段計畫與決策表 |
| [可執行範例 minilang](examples/minilang/README.zh-TW.md) | 單檔樹走訪直譯器，示範詞法、語法、閉包、例外與測試 |
| [來源與查證範圍](research/04-sources.zh-TW.md) | 官方文件、論文與本研究的證據邊界 |

## 問題一：Python 與 Node.js 是怎麼從 0 到有的

兩者的起點都不是「設計一門偉大的語言」，而是一個人用 C 解決手邊的問題，再借用現成的執行基礎。

| 項目 | Python（CPython） | Node.js | Lua（對照組） |
| --- | --- | --- | --- |
| 起點問題 | 在 CWI 的 Amoeba 作業系統上做系統管理，Bourne shell 與 C 都不順手 | 用非阻塞 I/O 寫可擴展的網路伺服器 | Tecgraf 為 Petrobras 做資料輸入與設定描述，需要可嵌入的設定語言 |
| 開工時間 | 1989 年聖誕假期開始實作 | 2009 年；同年 11 月在柏林 JSConf EU 公開展示 | 1993 年中設計，同年 7 月完成第一版實作 |
| 首次公開 | 1991 年 2 月 20 日以 0.9.0 發布到 alt.sources | 2009 年 | 1993 年 10 月於巴西軟體工程研討會發表 |
| 實作語言 | C | 自身 C++ 膠合層；語言引擎 V8 為 C++；I/O 層 libuv 為 C | C（嚴格 ANSI C，可當 C++ 編譯） |
| 借用的基礎 | ABC 的語法與高階資料型別、Modula-3 的例外語法、Unix C 函式庫 | Google 2008 年隨 Chrome 開源的 V8 引擎、作業系統的 epoll/kqueue/IOCP | yacc 產生的語法分析器（3.0 前）、C 標準函式庫 |
| 語言規格 | Language Reference 加 PEP 流程；無 ISO 標準 | ECMA-262（1997 年第一版，TC39 維護，現行 2026 年第 17 版） | 參考手冊；無外部標準 |
| 執行模型 | 位元組碼堆疊機，參考計數加循環 GC，3.11 起特化直譯器，3.13 起實驗性 JIT | V8 的 Ignition 位元組碼直譯器加 Sparkplug、Maglev、TurboFan 多層編譯；單執行緒事件迴圈 | 暫存器式位元組碼 VM，5.0 起成為第一個廣泛使用的暫存器 VM |

共同模式整理成四步：

1. **先有宿主，再有語言。** CPython 依賴 C 編譯器與 Unix 系統呼叫；Node.js 直接拿已經存在的 V8，自己只做事件迴圈與系統 API 綁定；Lua 刻意只用 ANSI C。沒有人從組合語言或自製 CPU 開始。
2. **可擴充性是第一個設計決策。** Guido van Rossum 明確說 ABC 最大的問題是不可擴充，所以 Python 一開始就能用 C 擴充；Lua 自稱「可擴充的擴充語言」，C API 是核心而非附件；Node.js 的價值是把 V8 接上作業系統。
3. **規格與流程在語言成功之後才制度化。** PEP 1 在 2000 年才定義提案流程；ECMAScript 在 JavaScript 於瀏覽器流行後才於 1997 年標準化；TC39 的 Stage 0 到 4 流程要求兩個相容實作通過 Test262 才能進標準。
4. **平台支援從「作者的機器」變成「層級政策」。** CPython 的 PEP 11 與 Node.js 的 BUILDING.md 都用 Tier 1、Tier 2、Tier 3 或 Experimental 定義哪些平台的失敗會擋住發布，而不是宣稱「到處都能跑」。

細節與來源見[研究一](research/01-how-python-and-nodejs-came-to-be.zh-TW.md)。

## 問題二：如何評估一門語言的移植性與執行環境

移植性不是單一分數，建議拆成六層分別評估，每層都有可查的證據。

| 層 | 要問的問題 | Python 的證據 | Node.js 的證據 |
| --- | --- | --- | --- |
| 平台支援政策 | 哪些 OS、CPU、libc 組合在哪一層？失敗會不會擋發布？ | PEP 11 列出 Tier 1 到 3 的 target triple，未列者視為不支援 | BUILDING.md 列出 OS、架構、最低 kernel 與 glibc 版本及 Tier |
| 實作語言與工具鏈 | 需要哪個版本的 C/C++ 編譯器？有無平台專用擴充？ | PEP 7 要求 C11 無選用功能，禁用 GCC 或 MSVC 專屬擴充 | Tier 1 要求 GCC 13.2 以上或 Clang 19.1 以上、VS 2022 以上、Xcode 16.4 以上 |
| 系統相依層 | I/O、執行緒、動態載入靠什麼抽象？ | `posixmodule` 直接包 POSIX 與 Win32；`sys.platform` 區分平台 | libuv 抽象 epoll、kqueue、event ports、IOCP，檔案 I/O 走執行緒池 |
| 擴充 ABI | 原生擴充是否需每版重編？ | 限制 API 與穩定 ABI（abi3，3.15 新增 abi3t）；free-threaded 版 ABI 不相容 | Node-API 跨主版本 ABI 穩定；直接用 V8 或 libuv 標頭則不保證 |
| 二進位散布 | 怎麼把編好的東西交給使用者？ | manylinux 以 glibc 版本命名的 wheel 標籤、cibuildwheel | 官方二進位、Single Executable Applications、prebuilt 原生模組 |
| 受限環境 | 無檔案系統、無子程序、無 JIT 權限時能不能跑？ | WASI 為 Tier 2、Emscripten 與 iOS、Android 為 Tier 3；iOS 禁 subprocess | `node:wasi` 為實驗；SEA 的 VFS 唯讀；MicroPython 示範 16 KB RAM 級別的另一條路 |

執行環境的評估再加三個維度：效能層級（純直譯、特化直譯、基線 JIT、最佳化 JIT）、記憶體模型（參考計數、追蹤式 GC、是否 stop-the-world）、並行模型（GIL、事件迴圈、free-threading）。這些都會改變「同一份程式在不同機器上的行為」，所以屬於移植性的一部分。評估檢查表與評分方式見[研究二](research/02-portability-and-runtime-evaluation.zh-TW.md)。

## 問題三：自己從 0 設計一門動態語言該怎麼做

建議順序與 Python、Lua 的歷史一致，也與《Crafting Interpreters》兩部曲的結構一致：先樹走訪直譯器，再位元組碼 VM。每階段都有可驗收的產出。

| 階段 | 產出 | 驗收方式 | 大約規模 |
| --- | --- | --- | --- |
| 0. 問題與範圍 | 一頁設計文件：要解決什麼、不做什麼、宿主語言、授權 | 能用十行範例程式說明語言的核心價值 | 1 至 2 天 |
| 1. 語法與語意草案 | EBNF 文法、值模型、作用域規則、錯誤模型 | 寫 20 個合法與 10 個非法範例，人工判定結果 | 數天 |
| 2. 樹走訪直譯器 | 詞法、語法分析、AST、環境與閉包、例外 | 測試套件全綠；REPL 可互動 | 1 千至 3 千行 |
| 3. 標準函式庫與模組 | 字串、集合、I/O、模組載入 | 能寫一個小工具解決階段 0 的問題 | 視需求 |
| 4. 位元組碼編譯器與 VM | 指令集、常數池、呼叫框架、upvalue 閉包 | 與階段 2 跑同一套測試結果一致；效能測量 | 3 千至 8 千行 C |
| 5. 記憶體管理 | 參考計數或標記清除 GC、根集合、終結器 | 長時間執行測試記憶體不增長 | 數百至 1 千行 |
| 6. 宿主互通 | C API 或 FFI、嵌入 API、錯誤跨邊界 | 用宿主語言寫一個擴充並載入 | 視設計 |
| 7. 平台與散布 | 支援層級文件、CI 矩陣、二進位或單檔散布 | 每個 Tier 1 平台 CI 綠燈 | 持續 |
| 8. 規格與流程 | 語言參考手冊、相容性政策、提案流程 | 第二個實作或他人能照手冊實作子集 | 持續 |

關鍵決策先在階段 0 到 1 做掉，因為之後很難改：

- **值模型**：一切皆物件（Python、Ruby）還是少數原生型別加表（Lua）？影響 GC、FFI 與效能。
- **作用域**：詞法作用域加閉包幾乎是現代共識；Lua 與 Python 都走到這裡。
- **錯誤模型**：例外（Python、JavaScript）或錯誤值（Lua 的 `error`/`pcall`）。
- **並行模型**：先選單執行緒加事件迴圈或協程；多執行緒共享堆積是 Python 花了二十年才在 3.13 做出 free-threaded 實驗版的題目。
- **嵌入或獨立**：Lua 從第一天就是嵌入式函式庫；Python 是獨立直譯器後來才補嵌入 API。這決定 API 形狀與全域狀態的處理。

完整決策表、每階段的細節與常見陷阱見[研究三](research/03-design-your-own-dynamic-language.zh-TW.md)。可執行的最小範例在 [examples/minilang](examples/minilang/README.zh-TW.md)，用 Python 寫成的單檔樹走訪直譯器，含測試，對應階段 2。

## 本研究的邊界

- 歷史事實來自 Python 官方 FAQ 與 Guido van Rossum 的 Python History 部落格、Lua 作者的 HOPL III 論文與 JUCS 論文、Chromium 官方部落格、Node.js 與 V8 官方文件。Node.js 首次展示的日期只有 Wikipedia 等二手來源，文中已標示。
- 平台層級、版本與工具鏈要求以查閱日當天的官方文件為準；PEP 與 BUILDING.md 會隨版本更新，實作前請重查。
- 階段規模是依公開教材與參考實作的行數推估，不是量測結果。
- 本研究沒有宣稱哪種語言設計較優；表格是對照，不是排名。
