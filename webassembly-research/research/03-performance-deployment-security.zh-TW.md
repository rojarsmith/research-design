# 03 效能、部署與安全

[回主報告](../README.zh-TW.md) · 查核日期：2026-10-21

## 效能應拆成完整路徑

本研究建議使用以下量測模型，而非只計算函式本體時間：

```text
首次工作 = 下載 + 編譯/初始化 + 資料轉換與傳遞 + 運算 + 結果回傳
後續工作 = 排程/傳遞 + 資料轉換 + 運算 + 結果回傳
```

這是評估框架，不是引擎保證每一步都依序發生。串流編譯可重疊部分下載與編譯；快取與常駐 Worker 也會改變後續成本。web.dev 的效能模式文章示範了串流載入、傳遞已編譯模組給 Worker，以及常駐或按需建立 Worker 的取捨。[文章](https://web.dev/articles/webassembly-performance-patterns-for-web-apps)

工程建議：用相同輸入比較 JavaScript、JavaScript + Worker、Wasm、Wasm + Worker。先檢查輸出一致，再量測冷啟動、暖機後時間、p50/p95、傳輸大小、峰值記憶體和主執行緒阻塞。大型資料要把複製與配置算進去；不能以一個微型算術例子推估影像處理效益。

## 執行緒與 SIMD

| 技術 | 能改善什麼 | 需要另外確認什麼 |
| --- | --- | --- |
| 單一 Worker | 讓運算離開 UI 執行緒 | 啟動、資料傳遞與取消任務設計；不代表演算法自動平行 |
| Emscripten pthreads | 以共享記憶體實作多執行緒工作 | 編譯與連結均需 `-pthread`；瀏覽器部署需要跨來源隔離 |
| Wasm SIMD | 對適合的資料運算使用向量指令 | 編譯器選項、程式形狀、瀏覽器支援及實際效益 |

來源：[執行緒原理](https://web.dev/articles/webassembly-threads)、[Emscripten pthreads](https://emscripten.org/docs/porting/pthreads.html)、[SIMD 文件](https://emscripten.org/docs/porting/simd.html)。SIMD 與 pthreads 是分開的能力，不應假設開啟其中一項就取得另一項。

Emscripten 文件指出，共享記憶體的 pthread 版本不能用同一 binary 自動退回無執行緒版本；有此需求應提供兩套建置。Worker 本身可在不共享記憶體下使用，不能把一般 Worker 與 pthreads 的部署要求混為一談。[pthreads 限制](https://emscripten.org/docs/porting/pthreads.html)

## 部署檢查

1. **載入方式**：使用 HTTP 服務驗證；Emscripten 產物優先使用對應 loader，直接 `instantiateStreaming` 的核心模組則必須提供正確 imports。[MDN 載入指南](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Loading_and_running)
2. **MIME 與 CSP**：串流編譯需要伺服器正確回傳 `application/wasm`；嚴格 CSP 可能阻擋 Wasm 編譯，需對照實際政策調整。[MDN instantiateStreaming](https://developer.mozilla.org/en-US/docs/WebAssembly/Reference/JavaScript_interface/instantiateStreaming_static)
3. **共享記憶體版本**：驗證 `crossOriginIsolated`，以及 COOP/COEP 與第三方資源的相容性。常見配置為 `Cross-Origin-Opener-Policy: same-origin`、`Cross-Origin-Embedder-Policy: require-corp`，仍需在部署站點確認。[pthreads 文件](https://emscripten.org/docs/porting/pthreads.html)
4. **產物一致性**（工程建議）：JS loader、Wasm、Worker 與資料檔採相同發布版本；測試 CDN 快取及回滾，避免新 loader 載到舊 Wasm。
5. **失敗處理**（工程建議）：提供載入失敗、記憶體不足、功能不支援、逾時與取消的使用者流程；視產品需求選擇替代路徑。

## 安全邊界

Wasm 沙箱限制模組存取宿主，但不是把 C/C++ 的所有錯誤變不可能。線性記憶體內的越界寫入仍可能破壞鄰近物件；整段線性記憶體的邊界檢查不等於逐物件記憶體安全。宿主提供的 API 也決定模組能做哪些事。[WebAssembly 安全模型](https://webassembly.org/docs/security/)

工程推論：不可信輸入仍需長度與資源限制，宿主 imports 採最小能力配置，並設置運算時間與記憶體上限。瀏覽器端下載的程式也不適合作為秘密金鑰的保護機制；發布二進位格式不等於加密原始邏輯。
