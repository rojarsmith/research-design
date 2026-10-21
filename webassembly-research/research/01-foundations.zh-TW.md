# 01 原理與技術邊界

[回主報告](../README.zh-TW.md) · 查核日期：2026-10-21

## 從原始碼到執行

```text
C / C++ / Rust 等原始碼
          ↓ 編譯與連結（目標、ABI、執行期必須一致）
       .wasm 二進位
          ↓ 宿主驗證、編譯、建立執行個體
瀏覽器的 Wasm 引擎，或瀏覽器外的 Wasm runtime
          ↕ imports / exports
JavaScript、WASI 或應用程式自訂的宿主功能
```

Wasm 的指令格式獨立於實體 CPU，實際執行由引擎處理。WAT 是方便閱讀的文字表示，不是另一種執行平台。核心模型含 Module（程式模組）、Instance（有狀態的執行個體）、Memory（線性位元組記憶體）與 Table（可供間接呼叫等用途的表）。同一模組可建立多個執行個體；是否共享記憶體由配置決定。[MDN 概念](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Concepts)、[WAT 導讀](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Understanding_the_text_format)

## 三組常被混在一起的概念

| 名稱 | 角色 | 要確認的邊界 |
| --- | --- | --- |
| WebAssembly Core | 指令、型別、模組與執行模型 | 引擎必須支援實際產物使用的功能 |
| Emscripten | C/C++ 到 Web 的工具鏈及支援層 | 瀏覽器 API、非同步與原生系統介面仍須調整 |
| JavaScript glue | 載入、初始化及語言互通 | `.wasm` 通常不能脫離其預期 imports 直接使用 |
| Component Model / WIT | 元件組合及型別化介面描述 | 核心模組與元件有不同的封裝、工具與互通契約 |
| WASI | 宿主提供的標準系統能力介面 | 版本、world、runtime 支援與授權必須一致 |

表格整理依據：[Emscripten 建置](https://emscripten.org/docs/compiling/Building-Projects.html)、[Component Model 入門](https://component-model.bytecodealliance.org/introduction.html)、[WASI 0.3](https://wasi.dev/releases/wasi-p3)。選到同一個 `.wasm` 副檔名，並不等於選到相同的執行環境。

## 與 JavaScript 的分工

在 Web 應用中，可讓 JavaScript/TypeScript 處理 UI、事件、網路與排程，讓 Wasm 處理既有函式庫或運算核心。C/C++ 的函式、類別與記憶體可透過 Emscripten 的直接匯出、`ccall/cwrap`、Embind 等方式接到 JavaScript。跨邊界時要另訂字串編碼、陣列長度、記憶體所有權及釋放規則。[互通文件](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html)

工程推論：優先暴露「一次處理一塊資料」的 API，通常比讓每個小運算都跨語言呼叫更容易管理；是否較快仍需實測。

## 瀏覽器外與版本現況

Component Model 讓不同語言元件以明確介面組合，WASI 再提供檔案、命令列或 HTTP 等系統介面。依 WASI 官方發布頁，0.3 把非同步機制移入 Component Model，包含 `async func`、`stream<T>`、`future<T>`；不能把 0.2 的 `wasi:io` 用法原封不動視為 0.3 API。[WASI 0.3 發布說明](https://wasi.dev/releases/wasi-p3)

採用推論：以交付所需 world 與語言工具的相容性選版本，比只選最新版本更可靠。瀏覽器整合還要單獨驗證，不能從某個 runtime 的 WASI 支援推論所有瀏覽器都能直接執行同一元件。
