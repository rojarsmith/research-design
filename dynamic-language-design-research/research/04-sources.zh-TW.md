# 來源與查證範圍

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-19。以下來源在查閱日實際開啟。官方文件與作者本人的論文、部落格用於歷史與技術事實；評估表、階段計畫、決策建議與規模推估是本研究提出的方法，不是任何社群的官方標準。

## 官方與第一手來源

| 編號 | 來源 | 支持的內容 |
| --- | --- | --- |
| 1 | [Python 官方 FAQ：為何創造 Python](https://docs.python.org/3/faq/general.html) | ABC、Modula-3、Amoeba 的影響；1989 年聖誕假期開工；1991 年 2 月公開；可擴充性的動機 |
| 2 | [Guido van Rossum：Python 簡要時間線](https://python-history.blogspot.com/2009/01/brief-timeline-of-python.html) | 0.9.0 於 1991 年 2 月 20 日發布到 alt.sources；1.0、2.0、3.0 日期 |
| 3 | [Python 語言參考：簡介](https://docs.python.org/3/reference/introduction.html) | CPython、Jython、IronPython、PyPy；手冊描述語言而非實作 |
| 4 | [PEP 1](https://peps.python.org/pep-0001/) | PEP 類型、狀態、Steering Council、必要章節 |
| 5 | [PEP 7](https://peps.python.org/pep-0007/) | C11 無選用功能；禁編譯器專屬擴充；零警告 |
| 6 | [PEP 11](https://peps.python.org/pep-0011/) | 平台支援三層、各層平台清單、未列出即不支援 |
| 7 | [PEP 384](https://peps.python.org/pep-0384/) | 穩定 ABI 的動機與限制 API 設計 |
| 8 | [PEP 600](https://peps.python.org/pep-0600/) | manylinux 以 glibc 版本命名的 wheel 標籤 |
| 9 | [PEP 602](https://peps.python.org/pep-0602/) | 每年 10 月發布、五年支援 |
| 10 | [PEP 617](https://peps.python.org/pep-0617/) | 3.9 改用 PEG 的原因與效能數字 |
| 11 | [PEP 659](https://peps.python.org/pep-0659/) | 3.11 特化自適應直譯器與 10% 至 60% 加速範圍 |
| 12 | [PEP 703](https://peps.python.org/pep-0703/) | 3.13 free-threaded 建置的機制、ABI 不相容、效能代價 |
| 13 | [PEP 730](https://peps.python.org/pep-0730/) | iOS Tier 3；禁 subprocess；無 REPL；不能執行時下載程式碼 |
| 14 | [PEP 744](https://peps.python.org/pep-0744/) | 3.13 copy-and-patch JIT 實驗狀態、LLVM 建置相依、記憶體代價 |
| 15 | [Python C API 穩定性](https://docs.python.org/3/c-api/stable.html) | abi3 與 abi3t；ABI 不等於行為穩定；`Py_mod_abi` |
| 16 | [Python free-threading HOWTO](https://docs.python.org/3/howto/free-threading-python.html) | 單執行緒 1% 至 8% 額外負擔；GIL 自動重新啟用 |
| 17 | [sys 模組文件](https://docs.python.org/3/library/sys.html) | `sys.platform` 值、`sys.implementation` |
| 18 | [CPython InternalDocs：compiler](https://github.com/python/cpython/blob/main/InternalDocs/compiler.md) | 編譯管線各階段與檔案 |
| 19 | [CPython InternalDocs：interpreter](https://github.com/python/cpython/blob/main/InternalDocs/interpreter.md) | 16 位元 code unit、指令 DSL、框架、分派方式 |
| 20 | [CPython InternalDocs：garbage collector](https://github.com/python/cpython/blob/main/InternalDocs/garbage_collector.md) | 參考計數加循環 GC、三代與閾值、free-threaded 差異 |
| 21 | [pyperformance](https://github.com/python/pyperformance) | Python 基準套件定位 |
| 22 | [cibuildwheel](https://cibuildwheel.pypa.io/en/stable/) | 跨平台 wheel 建置涵蓋範圍 |
| 23 | [Pyodide](https://pyodide.org/en/stable/) | CPython 經 Emscripten 在瀏覽器執行、FFI |
| 24 | [PyPy 架構](https://doc.pypy.org/en/latest/architecture.html) | RPython 翻譯、meta-tracing JIT、incminimark |
| 25 | [MicroPython](https://micropython.org/) | 256K 程式空間與 16K RAM |
| 26 | [MicroPython 移植指南](https://docs.micropython.org/en/latest/develop/porting.html) | 最小 port 五個檔案與功能巨集 |
| 27 | [Chromium Blog：Google Chrome's Need for Speed（2008-09-02）](https://blog.chromium.org/2008/09/google-chromes-need-for-speed_02.html) | V8 開源、編譯到機器碼、hidden class 與 inline cache |
| 28 | [Google Developers Blog：Chrome、Chromium、V8 發布](https://developers.googleblog.com/2008/09/google-chrome-chromium-and-v8-launch.html) | BSD 授權開源 |
| 29 | [V8 docs](https://v8.dev/docs) | C++、支援平台、外部維護的架構、GC 性質、可嵌入 |
| 30 | [V8 docs：Ignition](https://v8.dev/docs/ignition) | 暫存器式直譯器 |
| 31 | [V8 blog：Maglev（2023-12-05）](https://v8.dev/blog/maglev) | 四層執行管線與能耗數字 |
| 32 | [V8 docs：embed](https://v8.dev/docs/embed) | Isolate、Context、Handle、模板 |
| 33 | [V8 docs：version numbers](https://v8.dev/docs/version-numbers) | 版本號與 Chromium 里程碑關係 |
| 34 | [Introduction to Node.js](https://nodejs.org/en/learn/getting-started/introduction-to-nodejs) | 在瀏覽器外執行 V8、單程序非阻塞 I/O |
| 35 | [About Node.js](https://nodejs.org/en/about) | 事件驅動模型、OpenJS Foundation |
| 36 | [Node.js BUILDING.md](https://github.com/nodejs/node/blob/main/BUILDING.md) | 平台層級、最低版本、工具鏈、官方二進位環境、libatomic |
| 37 | [Node-API](https://nodejs.org/api/n-api.html) | ABI 穩定、與引擎無關、版本機制 |
| 38 | [Node.js previous releases](https://nodejs.org/en/about/previous-releases) | Current 六個月、LTS 30 個月、27 起年度週期 |
| 39 | [Node.js Single Executable Applications](https://nodejs.org/api/single-executable-applications.html) | 單檔散布、VFS、限制 |
| 40 | [node:wasi](https://nodejs.org/api/wasi.html) | 實驗狀態、非安全沙箱 |
| 41 | [libuv design overview](https://docs.libuv.org/en/v1.x/design.html) | 各平台輪詢機制、迴圈階段、執行緒池 |
| 42 | [ECMA-262](https://ecma-international.org/publications-and-standards/standards/ecma-262/) | 1997 年第一版、2026 年第 17 版、TC39 |
| 43 | [TC39 process](https://tc39.es/process-document/) | Stage 0 到 4 與進入條件 |
| 44 | [test262](https://github.com/tc39/test262) | 一致性測試套件規模與用途 |
| 45 | [Lua about](https://www.lua.org/about.html) | 1993 年、PUC-Rio、約 32,000 行 C、大小、MIT |
| 46 | [The Evolution of Lua（HOPL III，2007）](https://www.lua.org/doc/hopl.pdf) | DEL、SOL、1993 年 7 月第一版、yacc、ANSI C、可移植動機 |
| 47 | [The Implementation of Lua 5.0（JUCS，2005）](https://www.lua.org/doc/jucs05.pdf) | 五個實作目標、手寫分析器、無中間表示、while-switch、暫存器 VM、upvalue |
| 48 | [QuickJS](https://bellard.org/quickjs/) | ES2025、大小、啟動時間、參考計數加循環移除 |
| 49 | [Ruby about](https://www.ruby-lang.org/en/about/) | Matz、1995 年、MRI |
| 50 | [ISO/IEC 30170:2012](https://www.iso.org/standard/59579.html) | Ruby 標準（ISO 頁面查閱時回 403，資料以 ISO 搜尋摘要與各國標準機構轉載為準） |
| 51 | [webassembly.org](https://webassembly.org/) | 設計目標、非網頁嵌入 |
| 52 | [wasi.dev](https://wasi.dev/) | 0.1、0.2、0.3、Component Model、執行時列表 |
| 53 | [JetStream](https://browserbench.org/JetStream/) | JavaScript 與 WebAssembly 基準 |
| 54 | [Crafting Interpreters](https://craftinginterpreters.com/contents.html) | 樹走訪與位元組碼兩部曲章節結構 |
| 55 | [Writing An Interpreter In Go](https://interpreterbook.com/) | Monkey 語言、Pratt 解析、閉包 |

## 二手來源（僅用於歷史日期）

| 來源 | 內容 | 處理方式 |
| --- | --- | --- |
| [Wikipedia：Node.js](https://en.wikipedia.org/wiki/Node.js) | 2009 年 11 月 8 日 JSConf EU 展示；5 月 27 日首次發布 | 文中標示為二手，官方頁面未記載 |
| [The Register（2011）](https://www.theregister.com/2011/03/01/the_rise_and_rise_of_node_dot_js?page=4) | 2009 年 6 月早期版本；原名 web.js | 同上 |

## 證據如何解讀

- 歷史事實優先採作者本人敘述（Guido van Rossum 的 FAQ 與部落格、Lua 作者論文、Lars Bak 的 Chromium 部落格）。Node.js 的誕生日期只有二手來源，文中已標示。
- 平台層級、工具鏈版本、發布週期是查閱日的快照。PEP 11、BUILDING.md 與 Node.js 發布頁會隨版本改變，實作前必須重查。
- 效能數字（PEP 659 的 10% 至 60%、PEP 744 的記憶體 10% 至 20%、Maglev 的能耗、free-threading 的 1% 至 8%）都是各專案自述的基準結果，不同工作負載差異大，本研究未重新測量。
- 階段規模與時間是推估，依據為教材章節與參考實作的行數；minilang 範例已實際執行並通過測試，其他數字未驗證。
- CPython InternalDocs 標明為維護者文件，檔案名稱與細節會隨版本變動。

## 導入前待確認

| 問題 | 影響的決策 |
| --- | --- |
| 目標是嵌入式函式庫還是獨立直譯器 | API 形狀、全域狀態、是否帶編譯器 |
| 目標平台清單與工具鏈 | C 標準選擇、分派方式、是否允許 JIT |
| 是否需要多執行緒共享記憶體 | 值模型、參考計數與 GC 設計、ABI |
| 誰會寫原生擴充 | 是否需要穩定 ABI 與版本巨集 |
| 是否會有第二個實作 | 參考手冊與一致性測試的投入時機 |
