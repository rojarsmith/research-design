[English](README.md) | **繁體中文** | [開啟互動課程](index.html)

# 電腦圖形學實作課程

這門課以「先動手做」為主，適合已有程式設計經驗的資工本科生。先補足實作需要的數學，在 CPU 上親手產生像素，再把相同概念搬到著色器，逐步完成小型即時渲染器。

## 從這裡開始

1. 用現代瀏覽器開啟 [`index.html`](index.html)。課程頁面可離線使用，並可切換英文與繁體中文。
2. 從 **01 — 畫出一個三角形** 開始。範例會在 CPU 上建立影格緩衝區，以邊函數判定涵蓋範圍，並用重心座標插值顏色。
3. 調整頂點控制項、完成實驗題，再到 `labs/01-triangle/app.js` 實作變化題。
4. 能回答某個實驗的檢核問題後，再從實驗列選下一個實驗。每個實驗資料夾都有自己的 README，列出練習與檢核問題。
5. 建立自己的 `NOTES.md`，記錄觀察、截圖和每個檢核問題的答案。

前幾個實驗不需要設定顯示卡、遊戲引擎、套件管理器，也不要求修過圖學課。先用軟體渲染器建立脈絡，再介紹 WebGL 2 與 GPU 著色器。課程頁面是學習輔助；每個練習仍須親自修改程式。

## 學習路線

| 階段 | 動手製作 | 核心概念 |
| --- | --- | --- |
| 0. 像素 | 在 CPU 影格緩衝區繪製像素與三角形 | 座標、取樣、光柵化、重心座標 |
| 1. 2D 變換 | 移動、旋轉、縮放形狀 | 向量、矩陣、變換組合、齊次座標 |
| 2. 3D 變換 | 用攝影機渲染旋轉網格 | 模型／視圖／投影、裁切空間、透視除法 |
| 3. 可見性 | 加入深度緩衝區與背面剔除 | 深度測試、頂點繞序、裁切、精度 |
| 4. GPU 管線 | 將網格移植至 WebGL 2 並撰寫著色器 | 頂點處理、插值、片段、緩衝區、uniform |
| 5. 材質與光照 | 加入法線、Lambert／Blinn-Phong 與貼圖 | 座標空間、取樣、色彩空間、BRDF 直覺 |
| 6. 陰影與品質 | 加入陰影貼圖並實驗抗鋸齒 | 多重繪製階段、偏移、混疊與取捨 |
| 7. 期末專題 | 製作精簡互動場景並說明渲染流程 | 效能分析、除錯、設計選擇、可重現展示 |

逐階段的建置計畫請見 [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md)。目前目錄已實作第 01–07 個實驗；後續階段是隨學習逐步完成的課程路線。

**第二部（草稿，待審閱）：** [`SYLLABUS-PART2.zh-TW.md`](SYLLABUS-PART2.zh-TW.md) 規劃瀏覽器之後的內容：在 RTX 5070 Ti 上使用原生 Vulkan 或 Direct3D 12，先修共同核心，再選遊戲引擎渲染器，或 2D、文字與合成渲染其中一條路線。這只是提案，尚未實作任何部分。第一部與第二部都能在 Windows 上完成。

課程也會穿插硬體知識：渲染管線各階段、GPU 平行執行、記憶體與 CPU/GPU 資料搬運，以及以證據診斷效能。瀏覽器實驗方便起步；NVIDIA 實作路線會加入原生 OpenGL 渲染器，並用 Nsight Graphics 擷取和分析 RTX 5070 Ti 上的影格。CUDA 影像處理與 Vulkan 光線追蹤是選修延伸，不會把 CUDA 當成圖形管線本身。

依 NVIDIA 的 [GeForce RTX 5070 系列規格](https://www.nvidia.com/zh-tw/geforce/graphics-cards/50-series/rtx-5070-family/)，RTX 5070 Ti 支援 Vulkan 1.4 和 OpenGL 4.6。NVIDIA 的 [Nsight Graphics 支援清單](https://developer.nvidia.com/nsight-graphics/get-started)列出 RTX 50 系列與 OpenGL／Vulkan；安裝前請查看最新系統需求。

建議每階段投入 2–4 小時。每次以修改程式和觀察結果為主，再用概念筆記解釋觀察到的現象。

座標系手性、矩陣儲存順序、NDC 的 y 方向、深度範圍、正面繞序，以及色彩計算在哪個空間進行等慣例，統一列在 [`CONVENTIONS.zh-TW.md`](CONVENTIONS.zh-TW.md)，並說明換到 Vulkan 與 Direct3D 12 時各自要怎麼改。

## 每次實驗的流程

依序進行：**預測 → 實作 → 觀察 → 解釋**。一次只改一個因素，記下具體觀察（例如共用邊上的像素究竟由哪個三角形涵蓋），再把觀察連回程式中的規則。能解釋像素為何出現，再進入下一項。

## 檔案

- [`index.html`](index.html) — 雙語課程總覽與互動實驗；從實驗列選擇要進行的實驗。直接從磁碟開啟即可執行。
- [`CONVENTIONS.zh-TW.md`](CONVENTIONS.zh-TW.md) — 課程採用的座標、矩陣、深度、繞序與色彩慣例，以及換到 Vulkan 與 Direct3D 12 時的變化。
- [`labs/common/math.js`](labs/common/math.js)、[`labs/common/color.js`](labs/common/color.js) — 實驗 02–07 共用的向量／矩陣／攝影機／投影函式與 sRGB 轉換；以傳統 script 掛在 `window.cg` 上，不需要伺服器。
- [`labs/common/mesh.js`](labs/common/mesh.js)、[`labs/common/webgl.js`](labs/common/webgl.js) — 實驗 07 起使用：依實驗 06 頂點格式建立的球體、方塊與平面網格，以及 WebGL 2 輔助函式（程式、網格、貼圖、陰影貼圖與色彩渲染目標、GPU 計時查詢）。
- [`SYLLABUS-PART2.zh-TW.md`](SYLLABUS-PART2.zh-TW.md) — 第二部計畫**草稿**（原生 GPU 渲染、引擎路線、2D／文字／合成路線），待審閱。
- [`labs/01-triangle/app.js`](labs/01-triangle/app.js) — 可閱讀、可擴充的 CPU 三角形光柵器。
- [`labs/01-triangle/README.md`](labs/01-triangle/README.md) — 實驗說明與實作檢核點。
- [`labs/02-transforms/app.js`](labs/02-transforms/app.js) — 3×3 齊次變換、視口矩陣，以及疊在實驗 01 光柵器上的線框除錯覆層。
- [`labs/02-transforms/README.md`](labs/02-transforms/README.md) — 變換順序與混色實驗、A–H 練習與檢核問題。
- [`labs/03-first-3d/app.js`](labs/03-first-3d/app.js) — 4×4 矩陣、環繞攝影機、透視與正交投影、頂點追蹤與旁觀者檢視，仍在 CPU 上執行。
- [`labs/03-first-3d/README.md`](labs/03-first-3d/README.md) — 座標慣例、A–G 練習與檢核問題。
- [`labs/04-visibility/app.js`](labs/04-visibility/app.js) — 近裁切面裁切、背面剔除、可切換精度的深度緩衝區、作為對照的畫家演算法，以及深度與過度繪製除錯檢視。
- [`labs/04-visibility/README.md`](labs/04-visibility/README.md) — 可見性場景、A–G 練習與檢核問題。
- [`labs/05-webgl/app.js`](labs/05-webgl/app.js) — 以 WebGL 2 繪製同一個立方體（緩衝區、頂點陣列、著色器、uniform），附 JavaScript 版著色器的 CPU 對照版、像素差異檢視與即時片段著色器編輯器。
- [`labs/05-webgl/README.md`](labs/05-webgl/README.md) — CPU／GPU 比較、A–H 練習（含選修的原生 OpenGL／Nsight 路線）與檢核問題。
- [`labs/06-materials/app.js`](labs/06-materials/app.js) — 以 WebGL 2 實作法線與法線矩陣、逐頂點或逐片段的 Lambert 與 Blinn–Phong 光照、含 mipmap 的貼圖濾波，以及線性光與 sRGB 色彩計算的比較。
- [`labs/06-materials/README.md`](labs/06-materials/README.md) — 光照與色彩空間實驗、A–H 練習與檢核問題。
- [`labs/07-shadows/app.js`](labs/07-shadows/app.js) — 繪製至深度貼圖的雙階段陰影貼圖、固定與斜率偏移、PCF，以及用並排放大鏡和有鋸齒的對照版比較 MSAA 與 SSAA，另有除錯檢視與 GPU 計時。
- [`labs/07-shadows/README.zh-TW.md`](labs/07-shadows/README.zh-TW.md) — 陰影痤瘡、陰影分離與抗鋸齒實驗、A–H 練習與檢核問題。

## 完成目標

完成一個自己撰寫的渲染器、一個互動期末場景，以及一篇簡短技術說明：追蹤頂點從物件座標經光柵化到畫面像素的流程，並說明座標慣例、深度行為、採用的光照模型，以及至少一項畫質／效能取捨。
