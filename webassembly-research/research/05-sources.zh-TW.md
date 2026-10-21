# 05 文章導讀、來源與查證紀錄

[回主報告](../README.zh-TW.md) · 全部來源查核日期：2026-10-21（Asia/Taipei）

## 指定文章與延伸閱讀

以下為摘要與閱讀定位，非全文轉載。官方文件用於確認機制；工程文章提供具體流程，不能單靠作者案例推導通用效能結論。

| 來源 | 摘要與閱讀用途 | 查證界線 |
| --- | --- | --- |
| [Claire Chang：Emscripten 編譯實作筆記](https://claire-chang.com/post/emscripten-webassembly-compile-guide/) | 從 Windows/MSYS2 環境到 C/C++ 建置，透過 libde265.js 與 AV1/AOM 說明工具鏈設定問題 | 原 archive 與 post URL 的直接擷取均失敗；搜尋工具回傳 post 內文。頁面標示 2024-09-12，但內文含 2026 資料，應視為目前可見修訂內容，不能當成 2024 原稿 |
| [MDN：WebAssembly concepts](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Concepts) | 建立 Wasm、宿主與 JavaScript 的概念模型 | 概念解說，不能取代實際功能相容性檢查 |
| [Surma：Emscripting a C library to Wasm](https://web.dev/articles/emscripting-a-c-library) | 用 libwebp 移植串起工具鏈、C 介面與 JavaScript 呼叫，適合接在入門之後閱讀 | 歷史教學中的選項須以目前 Emscripten 文件重新核對 |
| [Thomas Steiner：WebAssembly performance patterns for web apps](https://web.dev/articles/webassembly-performance-patterns-for-web-apps) | 逐步比較載入、編譯、執行個體與 Worker 生命週期的安排 | 例子用於說明模式，不代表真實產品的加速比例 |
| [Ingvar Stepanyan：Using WebAssembly threads from C, C++ and Rust](https://web.dev/articles/webassembly-threads) | 將 Web Workers、共享 Memory 與同步連成多執行緒模型 | 部署與旗標以目前 pthreads 文件為準 |
| [Using asynchronous web APIs from WebAssembly](https://web.dev/articles/asyncify) | 解釋同步程式與 Web 非同步事件迴圈之間的落差 | 屬歷史背景；文章對當時功能缺口的描述，不能用來判定 2026 所有非同步方案的現況 |
| [Component Model 入門](https://component-model.bytecodealliance.org/introduction.html) | 認識以型別介面組合不同語言元件的方向 | 與瀏覽器直接載入核心模組分開理解 |
| [WASI 0.3 發布頁](https://wasi.dev/releases/wasi-p3) | 查核正式發布日期，以及非同步介面的變化 | 具體語言與 runtime 組合仍須鎖版驗證；發布規格不表示所有宿主已支援 |

## 按工程問題回查官方資料

| 問題 | 來源 | 本研究採用的資訊 |
| --- | --- | --- |
| 如何安裝與啟用 SDK？ | [Download and install](https://emscripten.org/docs/getting_started/downloads.html) | emsdk 與 Windows 指令入口 |
| 原生專案怎麼交叉編譯？ | [Building Projects](https://emscripten.org/docs/compiling/Building-Projects.html) | wrapper、CMake toolchain、host 工具與 target 產物問題 |
| JS 如何呼叫 C/C++？ | [Interacting with code](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html) | 匯出、直接呼叫、ccall/cwrap 與綁定方式 |
| 如何封裝非同步初始化？ | [Modularized Output](https://emscripten.org/docs/compiling/Modularized-Output.html) | 模組 factory 與 ES module 輸出 |
| Wasm 如何載入？ | [MDN Loading and running](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Loading_and_running) | streaming 與 ArrayBuffer 路徑 |
| 串流載入有哪些伺服器要求？ | [MDN instantiateStreaming](https://developer.mozilla.org/en-US/docs/WebAssembly/Reference/JavaScript_interface/instantiateStreaming_static) | MIME 與 CSP 注意事項 |
| pthreads 的實際限制？ | [Pthreads support](https://emscripten.org/docs/porting/pthreads.html) | 編譯/連結旗標、跨來源隔離、分開建置回退版本 |
| 如何處理向量運算？ | [Using SIMD](https://emscripten.org/docs/porting/simd.html) | Wasm SIMD 工具鏈與移植入口 |
| 沙箱保護到哪裡？ | [WebAssembly Security](https://webassembly.org/docs/security/) | 宿主隔離與線性記憶體內部破壞的差別 |
| 如何閱讀文字表示？ | [MDN WAT](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Understanding_the_text_format) | 指令、函式與匯出的可讀表示 |

## 證據使用方式

- **文件事實**：以連結支持工具、介面與限制；不把「接近原生」當成已證實的效能保證。
- **作者經驗**：保留指定文章的情境與問題，不將其套件清單普遍化。
- **本研究推論**：量測模型、採用條件與階段驗收是整理後的工程建議。
- **未知事項**：實際裝置效能、SDK 鎖定版本、特定函式庫移植狀態與交付相容性，需要另行實驗。

## 本機查證與未執行事項

本次在 PowerShell 查詢目前可見的 `emcc`、`clang`、`node`、`python`，僅找到 Node 與 Python。這表示目前 PATH 未提供前兩項，不代表整台電腦絕對沒有安裝。未安裝 emsdk、未重建 libde265/AOM、未執行 02 的編譯配方，也未作瀏覽器或效能測試。

研究檔案完成後檢查相對連結、檔案編碼與 Git diff；這些文件檢查不等於 Wasm 執行驗證。網頁是可變動資料，未存放第三方全文快照，後續重查可能遇到內容或網址變更。
