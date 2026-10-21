[English overview](README.md) | **繁體中文**

# WebAssembly：原理、移植實務與採用評估

研究與查核日期：2026-10-21（Asia/Taipei）。範圍涵蓋瀏覽器與瀏覽器外執行，重點放在理解技術、彙整文章，以及讓第一次驗證有明確路線。本文為公開資料研究，未實測效能；工具安裝與大型函式庫移植尚未執行。

## 先掌握的結論

1. **Wasm 是可攜的編譯目標。** C/C++、Rust 等程式經工具鏈產生 `.wasm`，由宿主驗證、編譯並執行；外部能力則由宿主介面供應。[MDN 概念文章](https://developer.mozilla.org/en-US/docs/WebAssembly/Guides/Concepts)
2. **價值常在重用既有程式碼。** 對成熟 C/C++ 函式庫而言，Emscripten 提供的不只是編譯器，還包括與 Web 整合的執行期支援。[C 函式庫移植案例](https://web.dev/articles/emscripting-a-c-library)
3. **移植成功取決於完整建置鏈。** 設定、相依庫、編譯、連結都要符合目標。只替換最外層編譯器，無法保證相依產物相容。[Emscripten 建置文件](https://emscripten.org/docs/compiling/Building-Projects.html)
4. **運算速度與畫面流暢是兩個問題。** Wasm 同步工作仍會占用所在執行緒；Worker 可把重工作移出 UI 執行緒，但有啟動與通訊成本。[Web 應用效能模式](https://web.dev/articles/webassembly-performance-patterns-for-web-apps)
5. **WASI 與瀏覽器工具鏈分開評估。** Component Model 處理元件介面與組合；WASI 定義系統能力介面。WASI 官方目前記載 0.3.0 已於 2026-06-11 發布，舊文中的「Preview 3 尚未發布」已不能直接沿用。[Component Model](https://component-model.bytecodealliance.org/introduction.html)、[WASI 0.3](https://wasi.dev/releases/wasi-p3)

## 文件地圖

| 文件 | 回答的問題 |
| --- | --- |
| [01 原理與技術邊界](research/01-foundations.zh-TW.md) | Wasm 如何執行？Module、Instance、Memory 是什麼？與 JavaScript、WASI 有何關係？ |
| [02 Emscripten 與指定文章彙整](research/02-emscripten-guide.zh-TW.md) | Claire 文章有哪些重點與需要補充的地方？如何做最小 C 到 Web 實驗？ |
| [03 效能、部署與安全](research/03-performance-deployment-security.zh-TW.md) | 如何量測？執行緒、SIMD、MIME、CSP 與記憶體有哪些限制？ |
| [04 使用情境與研究路線](research/04-adoption-roadmap.zh-TW.md) | 何時值得試？如何比較替代方案並定義驗收？ |
| [05 文章導讀、來源與查證紀錄](research/05-sources.zh-TW.md) | 延伸文章各自解決什麼問題？資料可信度與未驗證項目為何？ |

## 建議閱讀方式

先讀 01 建立模型，再讀 02 對照指定文章。準備導入產品時，使用 03 的量測清單和 04 的驗收表。05 保留逐項來源，方便回查，不能把文章中的版本與範例直接當成已在本環境驗證的配置。

本研究以整體技術生態為範圍，未預設任何公司的產品需求。採用建議屬工程推論，必須由目標裝置、實際資料與交付條件驗證。
