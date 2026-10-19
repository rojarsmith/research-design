# 移植性與執行環境評估

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-19。本文提供一套評估方法：先看官方「支援政策」而不是看網路上「有人跑過」，再逐層檢查實作語言、系統相依、擴充 ABI、散布方式與受限環境，最後把執行模型納入，因為它決定同一程式在不同機器上的行為是否一致。

## 一、先讀支援政策：Tier 的意義

兩個主流專案都用層級定義承諾，重點不是「能不能編」，而是「失敗會不會擋住發布、誰負責修」。

| 層級 | CPython（PEP 11） | Node.js（BUILDING.md） |
| --- | --- | --- |
| Tier 1 | CI 失敗阻擋發布；破壞 main 的變更立即修或撤回；全體核心開發者共同負責 | 多數使用者的平台；Build WG 維持完整測試；失敗阻擋發布 |
| Tier 2 | 需穩定 buildbot 與至少兩名核心開發者；24 小時內修或撤回；失敗阻擋發布 | 較小使用者群；完整測試；失敗阻擋發布，但基礎設施問題可能延後二進位發布 |
| Tier 3 / Experimental | 需穩定 buildbot 與至少一名核心開發者；無回應時限；失敗不阻擋發布 | 可能無法編譯或通過測試；不出官方版本；失敗不阻擋發布 |
| 未列出 | 視為不支援；程式碼只在維護負擔小且受益者多時保留，可能不經棄用流程移除 | 廠商已 EoL 的版本一律不支援；生產環境只應用 Tier 1 或 2 |

來源：[PEP 11](https://peps.python.org/pep-0011/)、[Node.js BUILDING.md](https://github.com/nodejs/node/blob/main/BUILDING.md)。

查閱日的平台清單摘要（實作前請重查）：

| 專案 | Tier 1 | Tier 2 | Tier 3 / Experimental |
| --- | --- | --- | --- |
| CPython | aarch64 與 x86_64 的 Linux glibc（gcc）、aarch64 macOS、i686 與 x86_64 Windows MSVC | aarch64 Windows、aarch64 與 x86_64 Linux（clang）、x86_64 macOS、wasm32-wasip1 | Android（aarch64、x86_64）、iOS 與模擬器、32 位元 Raspberry Pi OS、ppc64le、riscv64、s390x、wasm32 Emscripten、FreeBSD x86_64 |
| Node.js | Linux x64 與 arm64（glibc，kernel ≥ 5.14、glibc ≥ 2.34）、Windows x64、macOS arm64 13.5 以上 | Linux x64 musl、ppc64le、s390x；Windows arm64；SmartOS x64；AIX ppc64be | Linux x86、armv7、loong64、riscv64；macOS x64；FreeBSD x64；OpenHarmony arm64 |

兩份清單透露的評估原則：

1. **用 target triple 或「OS + 架構 + libc 版本」描述平台，不用 OS 名稱。** Node.js 直接寫最低 kernel 與 glibc 版本；CPython 區分 glibc 與 gcc、clang 組合。
2. **工具鏈版本是支援條件的一部分。** Node.js Tier 1 要求 GCC 13.2 以上或 Clang 19.1 以上、Visual Studio 2022 或 2026 搭配 Windows 11 SDK、Xcode 16.4 以上；riscv64 原生建置需 GCC 14 或 Clang 19 以上。[BUILDING.md](https://github.com/nodejs/node/blob/main/BUILDING.md)
3. **官方二進位的建置環境也要公開。** Node.js 列出每個官方二進位在哪個 OS 與編譯器上建出，Node.js 25 起 Linux 官方二進位連結 `libatomic`，執行環境必須安裝。[BUILDING.md](https://github.com/nodejs/node/blob/main/BUILDING.md)

## 二、六層評估表

| 層 | 問題 | 證據來源 | 評估方式 |
| --- | --- | --- | --- |
| 1. 支援政策 | 目標平台在哪一層？由誰維護？ | PEP 11、BUILDING.md 或對應文件 | 不在 Tier 1 或 2 就要自備 buildbot 與人力 |
| 2. 實作語言與工具鏈 | 用哪個 C/C++ 標準？禁用哪些擴充？ | PEP 7：C11 無選用功能，禁 GCC/MSVC 專屬擴充，主要編譯器零警告；Lua：嚴格 ANSI C，可當 C++ 編譯 | 目標平台的編譯器是否達標；若語言核心依賴 computed goto 或 `musttail`，是否有回退路徑 |
| 3. 系統相依層 | I/O、時間、執行緒、動態載入、訊號怎麼抽象？ | libuv 把 epoll、kqueue、event ports、IOCP 收成一個模型；CPython 用 `sys.platform` 區分 `linux`、`darwin`、`win32`、`ios`、`android`、`emscripten`、`wasi` 等 | 列出語言核心直接呼叫的系統 API，確認目標平台都有；沒有的要有替代或明確禁用 |
| 4. 擴充 ABI | 原生擴充是否跨版本相容？ | CPython 限制 API 與穩定 ABI（`abi3`，3.15 新增 free-threaded 用的 `abi3t`）；Node-API 跨主版本穩定，直接用 V8 或 libuv 標頭不保證 | 生態系中有多少套件依賴不穩定 ABI；每次升版要重編多少 |
| 5. 二進位散布 | 編好的東西如何交付？ | manylinux 以 `manylinux_X_Y_ARCH` 標記 glibc 版本；cibuildwheel 覆蓋 macOS、Windows、manylinux、musllinux、iOS、Android、Pyodide；Node.js 官方二進位、SEA 單檔 | 每個目標平台是否有官方或可重現的建置流程 |
| 6. 受限環境 | 無檔案系統、無子程序、無執行緒、無 JIT 權限時能否運作？ | iOS 禁用 subprocess 與 fork、無 TTY、無 REPL、不能執行時下載程式碼；WASI 與 Emscripten 同樣禁 subprocess；`node:wasi` 為實驗且非安全沙箱；SEA 的 VFS 唯讀、原生模組要先寫到暫存檔 | 語言核心是否把這些能力設計成可關閉的功能而不是前提 |

來源：[PEP 7](https://peps.python.org/pep-0007/)、[libuv design](https://docs.libuv.org/en/v1.x/design.html)、[sys.platform](https://docs.python.org/3/library/sys.html)、[C API stability](https://docs.python.org/3/c-api/stable.html)、[Node-API](https://nodejs.org/api/n-api.html)、[PEP 600](https://peps.python.org/pep-0600/)、[cibuildwheel](https://cibuildwheel.pypa.io/en/stable/)、[PEP 730](https://peps.python.org/pep-0730/)、[node:wasi](https://nodejs.org/api/wasi.html)、[Node.js SEA](https://nodejs.org/api/single-executable-applications.html)。

## 三、ABI 穩定性為什麼是移植性的核心

CPython 在 3.2 定義穩定 ABI 的理由說明了問題：內部記憶體配置（字串駐留方式、物件大小型別）在版本間改變，直接讀取欄位的擴充升版後會讀到錯的資料；Windows 上每個功能版本有不同的 `pythonXY.dll`，擴充作者針對特定版本編譯，二進位散布讓問題更嚴重。解法是限制 API（`Py_LIMITED_API`）：只開放一組承諾不變的函式，結構欄位大多不可直接存取，型別在執行時以 `PyType_FromSpec` 建立，Windows 連結 `python3.dll` 轉發到版本專用 DLL。[PEP 384](https://peps.python.org/pep-0384/)

現行文件補充：穩定 ABI 防止符號缺失與結構配置改變，但不防行為改變；同一小版本內 ABI 前後相容，不同小版本需分別建置；3.15 加入 `Py_mod_abi` 槽讓不相容的擴充拋出例外而不是崩潰。[C API stability](https://docs.python.org/3/c-api/stable.html) free-threaded 建置因物件標頭改變而與標準建置及穩定 ABI 不相容，擴充必須重編並標示 `Py_mod_gil`，否則載入時 GIL 會被重新啟用。[PEP 703](https://peps.python.org/pep-0703/)

Node-API 的做法是把 ABI 與引擎脫鉤：操作對應 ECMA-262 概念而非 V8 概念，每個呼叫回傳 `napi_status`，JavaScript 值是不透明的 `napi_value`；版本 9 以後 Node-API 版本與 Node.js 版本獨立，預設 `NAPI_VERSION` 為 8；node-addon-api 是 header-only 的 C++ 包裝，仍只呼叫匯出的 C 函式所以保留 ABI 保證。[Node-API](https://nodejs.org/api/n-api.html)

對自製語言的含意：**從第一天就區分「內部標頭」與「公開 API」，公開 API 只用不透明指標與 C 內建型別，就能把日後的 ABI 問題控制在可管理範圍。** Lua 的 C API 從一開始就是這樣設計。[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)

## 四、執行環境的三個維度

同一份程式在不同環境的行為差異，常來自執行模型而不是 OS。

| 維度 | 選項 | 對行為與移植的影響 |
| --- | --- | --- |
| 效能層級 | 純直譯（Lua 的 while-switch）、特化直譯（CPython 3.11）、基線 JIT（Sparkplug）、最佳化 JIT（Maglev、TurboFan、CPython 3.13 實驗 JIT） | JIT 需要可寫可執行的記憶體與建置時工具（CPython JIT 需 LLVM）；iOS 與部分沙箱禁止 JIT；JIT 版記憶體較高（PEP 744 報告 10% 至 20%） |
| 記憶體模型 | 參考計數（CPython、QuickJS）、追蹤式 GC（V8 分代 stop-the-world、Lua、PyPy incminimark）、兩者混合 | 參考計數釋放時機確定、易與 C 互通，但要處理循環；追蹤式 GC 需要精確根集合與移動物件時的 handle 機制（V8 的 HandleScope） |
| 並行模型 | GIL（CPython 預設）、單執行緒事件迴圈（Node.js）、free-threading（CPython 3.13t）、協程（Lua） | 事件迴圈把 I/O 可移植性集中到一個函式庫（libuv）；free-threading 改變 ABI 與擴充寫法；GIL 讓擴充可以假設互斥 |

來源：[V8 docs](https://v8.dev/docs)、[V8 blog: Maglev](https://v8.dev/blog/maglev)、[PEP 744](https://peps.python.org/pep-0744/)、[PEP 703](https://peps.python.org/pep-0703/)、[V8 embed](https://v8.dev/docs/embed)、[PyPy architecture](https://doc.pypy.org/en/latest/architecture.html)、[QuickJS](https://bellard.org/quickjs/)。

## 五、WebAssembly 與微控制器：兩個極端的移植目標

WebAssembly 是堆疊式虛擬機的二進位指令格式，設計目標為可移植、高效、安全的沙箱，可嵌入瀏覽器也支援非網頁嵌入。[webassembly.org](https://webassembly.org/) WASI 提供 POSIX 風格的系統介面，有 0.1（Preview 1）、0.2、0.3 三個里程碑，0.2 起建立在 Component Model 上；執行時包括 Wasmtime、WAMR、WasmEdge、wazero 等，各有側重（WAMR 偏 IoT 與嵌入式，Wasmtime 偏伺服器端）。[wasi.dev](https://wasi.dev/)

CPython 把 `wasm32-unknown-wasip1` 列為 Tier 2、`wasm32-unknown-emscripten` 列為 Tier 3。[PEP 11](https://peps.python.org/pep-0011/) Pyodide 以 Emscripten 把 CPython 編成 WebAssembly 在瀏覽器與 Node.js 執行，可用 micropip 安裝純 Python wheel，並移植了 NumPy、pandas、SciPy 等含 C、C++、Rust 擴充的套件，JavaScript 與 Python 可互相呼叫。[Pyodide](https://pyodide.org/en/stable/) Node.js 的 `node:wasi` 可執行 WASI Preview 1 模組，但文件明言不提供完整檔案系統安全性，不應用來跑不受信任的程式碼。[node:wasi](https://nodejs.org/api/wasi.html)

另一端是微控制器。MicroPython 可在 256K 程式空間與 16K RAM 內執行，官方移植指南說明最小 port 只需五個檔案：`main.c`（設定 GC 堆積、初始化 runtime、啟動 REPL）、`Makefile`、`mpconfigport.h`（以 `#define` 開關功能，例如 `MICROPY_ENABLE_GC`、`MICROPY_PY_MATH`、浮點實作方式）、`mphalport.c` 與 `mphalport.h`（字元輸入輸出等硬體抽象）。這是「把平台相依收進一個薄層，功能全部可組態」的教科書範例。[MicroPython](https://micropython.org/)、[MicroPython porting guide](https://docs.micropython.org/en/latest/develop/porting.html)

Lua 的做法更簡單：核心只依賴 ANSI C，編譯器約佔核心 30%，記憶體受限時可以不帶編譯器、離線預編譯再由小模組載入。[The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)

## 六、評估檢查表

以下是本研究建議的檢查表，用於評估既有語言或自製語言的某個目標平台。每題答「是」記 1 分，建議 Tier 1 平台全數為「是」，Tier 2 至少前 8 題為「是」。

| 編號 | 檢查項目 |
| --- | --- |
| 1 | 官方文件明確列出此平台的 OS、架構、libc 或 SDK 最低版本與支援層級 |
| 2 | 有公開的 CI 或 buildbot 在此平台持續建置與測試 |
| 3 | 核心只需標準 C/C++ 子集，或對編譯器擴充有回退路徑 |
| 4 | 系統相依集中在單一抽象層（如 libuv、`mphalport`），而非散布在核心各處 |
| 5 | 無法提供的系統能力（子程序、執行緒、JIT、檔案系統）可在建置時關閉，且語言層有明確錯誤 |
| 6 | 公開 C API 只用不透明指標與 C 內建型別，並與內部標頭分離 |
| 7 | 原生擴充有跨版本 ABI 承諾，或至少有版本檢查避免崩潰 |
| 8 | 有可重現的二進位建置流程，並記錄建置環境（OS、編譯器、連結的執行時函式庫） |
| 9 | 有一致性測試套件（如 Test262、pyperformance 式基準）可在此平台執行並比對結果 |
| 10 | 發布週期與支援期限公開，使用者知道此平台何時會被降級或移除 |

測量效能時建議使用專案自己的基準：pyperformance 以真實工作負載為主，自述目標是所有 Python 實作的權威基準，但不針對 PyPy 調校。[pyperformance](https://github.com/python/pyperformance) JetStream 3.0 測 JavaScript 與 WebAssembly 工作負載的啟動、執行速度與流暢度。[JetStream](https://browserbench.org/JetStream/) 跨語言比較的數字只能當方向，不能當結論。

## 七、評估既有語言時的常見誤判

- **把「有人在某平台編過」當成支援。** PEP 11 明言未列出者不支援，相關程式碼可能不經棄用流程移除。
- **只看 OS 不看 libc。** Node.js 對 glibc 與 musl 分開列層；manylinux 標籤直接以 glibc 版本命名。
- **忽略工具鏈版本。** Node.js riscv64 需 GCC 14 或 Clang 19 以上；CPython JIT 建置需 LLVM。
- **把 ABI 穩定當成行為穩定。** 穩定 ABI 不保證函式行為不變，仍要對每個支援的小版本測試。
- **把沙箱當安全邊界。** `node:wasi` 與 SEA 文件都明確說明不是安全沙箱。
