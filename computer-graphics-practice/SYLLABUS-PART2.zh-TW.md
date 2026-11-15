# 第二部建置計畫 — 原生 GPU 渲染（草稿）

[English](SYLLABUS-PART2.md) | **繁體中文**

> **狀態：待審閱的草稿。** 本計畫尚未經過審閱或核准，各階段都還沒有實作。審閱後，階段名稱、順序與時數估計都可能調整。

第一部（[`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md) 的階段 00–08）在瀏覽器中打造渲染器：先寫 CPU 光柵器，再換成 WebGL 2。第二部把同樣的概念帶到原生程式，在 GeForce RTX 5070 Ti 上使用 Vulkan（或 Direct3D 12），目標是以下兩種成果之一：

- **引擎路線：** 遊戲引擎核心的渲染器。
- **2D、文字與合成渲染路線：** 桌面使用者介面底下的繪製與合成層，包括向量圖形、文字，以及 GPU 合成器。

兩條路線都從**共同核心**開始，內容涵蓋畫面呈現、同步、計算著色器、量測與色彩。先完成共同核心，再選一條路線；另一條路線列為選修。

## 課程在哪裡進行

**整門圖學課程，第一部與第二部，都能在 Windows 加上 RTX 5070 Ti 完成。** 本計畫的任何階段與期末專題都不需要 Linux 電腦；只有文末所述的作業系統研究才需要 Linux。

需要自行安裝的工具：

- C++ 工具鏈：MSVC（Visual Studio）或 clang，以及 CMake。
- LunarG Vulkan SDK（驗證層、著色器編譯器、`vulkaninfo`）；撰寫本草稿時 Windows 版最新為 1.4.363.1。若偏好 Direct3D 12，則需要 Windows SDK，並可選用 DirectX 12 Agility SDK。
- 擷取影格用的 NVIDIA Nsight Graphics 與 RenderDoc，以及量測影格時間與延遲用的 PresentMon。所有擷取都由你自己在 RTX 5070 Ti 上執行。

開始時在筆記中記下你選的 API（Vulkan 或 Direct3D 12），整個第二部都沿用同一個。以下各階段先寫 Vulkan 的概念，括號內是 Direct3D 12 的對應項目。

## 開始之前

- 完成第一部實驗 01–06 並保留筆記；建議也完成 07–08（陰影、期末場景）。
- 能熟練使用 C 或 C++ 和除錯器。更進一步的 C++（所有權、RAII、建置系統）屬於階段 C0。
- 讀懂 [`CONVENTIONS.zh-TW.md`](CONVENTIONS.zh-TW.md)。第二部會改變 NDC 的 y 方向、深度範圍與正面設定；每次移植都從那份檢查清單開始。

## 進度與評量方式

每個階段都沿用第一部的流程：**預測 → 實作 → 觀察 → 解釋**，並寫下檢核答案。另外：

- 每個階段結束時，正常使用下**驗證層不能有任何錯誤**（或 D3D12 除錯層不能有任何錯誤）。
- 每個效能說法都要有**證據**：GPU 時間戳、PresentMon 記錄，或 Nsight Graphics／RenderDoc 擷取。只看使用率百分比不算數。
- 時數是給已完成第一部的學生的粗略估計。最初幾個原生階段花的時間通常會比預期多。

---

## 共同核心

### C0 — 原生渲染器骨架

**先備：** 第一部實驗 01–06。**時數：** 30–50 小時。

**製作：** 用 CMake 建立專案，開啟視窗，建立啟用驗證層的 Vulkan 1.3 或 1.4 instance 與 device，每個影格透過 swapchain 清除畫面。（Direct3D 12：device、command queue、flip model swap chain、除錯層。）

**需要時再學：** 以 C++ 所有權與 RAII 管理 GPU 物件、處理 API 回傳的錯誤、CMake target、使用除錯器、驗證層如何回報誤用。

**檢核：** 列出你建立的每個物件、由誰擁有、何時銷毀。示範沒有任何驗證訊息的乾淨關閉流程。刻意違反一條規則（例如銷毀仍在使用中的緩衝區），解釋驗證層給出的訊息。

### C1 — 以明確同步繪製

**先備：** C0。**時數：** 50–80 小時。

**製作：** 移植實驗 05–06 有光照與貼圖的立方體。配置緩衝區與影像（使用 Vulkan Memory Allocator，或 D3D12 Memory Allocator），透過 staging buffer 上傳，使用 dynamic rendering，以 push constant 或 descriptor set 傳遞每影格資料，並同時進行兩到三個 **in-flight 影格**。

**需要時再學：** command buffer 與佇列、pipeline 物件、descriptor set（root signature 與 descriptor heap）、影像 layout、`synchronization2` barrier（resource barrier 或 enhanced barrier）、fence 與 semaphore，以及 `CONVENTIONS.zh-TW.md` 說明的 [0, 1] 深度範圍與 y 翻轉。

**檢核：** 解釋一個影格中的每個 barrier：它等待什麼、讓什麼變得可見、為什麼需要。刻意移除一個 barrier，用同步驗證層抓到它，並描述在真實硬體上可能出什麼錯。

### C2 — 畫面呈現與影格節奏（應用程式端）

**先備：** C1。**時數：** 20–30 小時。

**製作：** 一個設定面板，可切換 present mode（FIFO、MAILBOX，以及支援時的 IMMEDIATE；Direct3D 12：flip model 的 sync interval 與 tearing 支援）、in-flight 影格數，以及人為加上的 CPU 或 GPU 負載。記錄影格時間與從 present 到顯示的延遲。

**需要時再學：** swapchain 影像與 present mode（只有 FIFO 保證存在）、從應用程式角度看的垂直同步、畫面撕裂、影格佇列與延遲、在開始下一個影格前等待 present 完成（present wait 或 frame latency waitable object），以及 PresentMon 的 `DisplayLatency`、`MsClickToPhotonLatency` 等指標。

**檢核：** 在相同負載下，畫出三種設定的影格時間與延遲。解釋流暢度、撕裂與延遲之間的取捨，並分別為遊戲和桌面工具選出預設值。

### C3 — 計算著色器與 GPU 的執行模型

**先備：** C1。**時數：** 40–60 小時。

**製作：** 對影像或緩衝區實作模糊、直方圖與前綴和三個計算核心，每個都有簡單版與最佳化版（shared memory、subgroup 運算、更好的記憶體存取）。

**需要時再學：** workgroup 與 dispatch、subgroup（wave／warp）、shared memory、佔用率與延遲隱藏、合併記憶體存取、atomic 運算、計算與繪圖之間的同步。

**檢核：** 為每個核心提供兩個版本的時間戳量測，並用 Nsight Graphics GPU Trace 以證據（例如記憶體吞吐量或佔用率）解釋差異，而不是猜測。

### C4 — 量測 GPU

**先備：** C1、C3。**時數：** 20–30 小時。

**製作：** 在每個 pass 前後加上 GPU 時間戳查詢、畫面上的影格時間圖，並寫下分別用 Nsight Graphics 與 RenderDoc 擷取並逐步解讀一個影格的說明。

**需要時再學：** 時間戳查詢（`vkCmdWriteTimestamp2` 與 `timestampPeriod`；Direct3D 12 的 timestamp query heap 與 `GetTimestampFrequency`）、CPU 時間與 GPU 時間、Nsight Graphics 的 Graphics Capture 與 GPU Trace 的差別（舊的 Frame Debugger 對 Direct3D 12 與 Vulkan 已列為舊版功能）、從擷取中讀取 pipeline 狀態與資源內容、瓶頸假設（頂點、片段、頻寬、CPU 提交、同步）。

**檢核：** 為目前的渲染器列出影格時間預算（例如 60 Hz 的 16.6 ms 依 pass 分配），並用一次擷取和一個控制變因的實驗，為一個瓶頸判斷辯護。

### C5 — 色彩科學：線性光、sRGB、HDR 與色彩管理

**先備：** C1、第一部實驗 06。**時數：** 30–50 小時。

**製作：** 一個色彩測試場景，包含灰階漸層、棋盤格、高飽和原色與高亮區域。分別輸出到 sRGB swapchain 與 HDR swapchain：擴展範圍線性（scRGB，FP16）或 PQ 編碼（HDR10）。加上簡單的色調映射與 SDR 白色亮度控制。

**需要時再學：** 傳輸函數與色域原色的差別、sRGB 格式以及硬體在哪裡解碼與編碼、浮點渲染目標、scRGB（線性、Rec. 709 原色；在 HDR 顯示器上 1.0 代表 80 nits，在 SDR 顯示器上則代表顯示器的參考白）與 HDR10（PQ，SMPTE ST 2084，搭配 Rec. 2020 原色）、色調映射，以及 Windows Advanced Color 與 Auto Color Management 如何處理 SDR 與 HDR 顯示器。

**檢核：** 追蹤一個像素值從貼圖、光照、色調映射、編碼一路到顯示訊號，說出每一步的色彩空間。示範一個刻意的錯誤（重複編碼，或在編碼空間中混色）及其可見結果。

---

## 引擎路線

目標：遊戲引擎的渲染核心，包括資料驅動的 pass、物理式材質、陰影、後製、GPU 驅動的剔除與光線追蹤，並附效能量測。

### E1 — 渲染圖與資源生命週期

**先備：** 共同核心。**時數：** 40–60 小時。

**製作：** 小型渲染圖（render graph）：每個 pass 宣告讀寫哪些資源，由渲染圖排定 pass 順序、插入 barrier，並建立或重用（aliasing）暫時影像。

**需要時再學：** frame graph、資源生命週期、自動放置 barrier、記憶體 aliasing、剔除未使用的 pass。

**檢核：** 展示一個影格的渲染圖、它產生的 barrier，以及 aliasing 省下的記憶體。在不手動修改任何 barrier 的情況下加入一個新 pass。

### E2 — 材質與著色器系統

**先備：** E1。**時數：** 40–60 小時。

**製作：** 離線把著色器編譯成 SPIR-V（或 DXIL），用反射資訊建立 descriptor layout，設計材質描述格式，為少數功能產生著色器變體，並加上 pipeline cache。

**需要時再學：** 從 GLSL 或 HLSL 到 SPIR-V、著色器反射、specialization constant、建立 pipeline 的成本與快取、熱重載。

**檢核：** 從頭到尾加入一個新的材質功能，回報目前有多少 pipeline、建立它們花多少時間，並示範熱重載。

### E3 — 物理式材質與影像式光照

**先備：** E2、C5。**時數：** 50–80 小時。

**製作：** glTF 採用的 metallic–roughness 材質模型，搭配直接光源與來自環境貼圖的影像式光照（預先濾波的高光、輻照度，以及 BRDF 查找表）。

**需要時再學：** 微表面 BRDF（GGX、Smith、Schlick Fresnel）、能量守恆、split-sum 近似、立方體貼圖與以 mip 表示粗糙度、線性光 HDR 渲染。

**檢核：** 渲染一組粗糙度／金屬度球體陣列，並與參考檢視器比較，解釋一個看得到的差異。

### E4 — 級聯陰影貼圖

**先備：** E1、第一部實驗 07。**時數：** 30–50 小時。

**製作：** 平行光的級聯陰影，包含 PCF 濾波、級聯之間的混合，以及避免閃爍的穩定化。

**需要時再學：** 級聯切分、texel 對齊、深度偏移與斜率偏移、陰影貼圖解析度與涵蓋範圍的取捨。

**檢核：** 視覺化各級聯，量測每個級聯的成本，並展示一個瑕疵（陰影痤瘡、陰影分離或閃爍）及其修正。

### E5 — 後製：色調映射、泛光與時間性抗鋸齒

**先備：** E3。**時數：** 40–60 小時。

**製作：** HDR 後製鏈：泛光（bloom）、曝光與色調映射，以及時間性抗鋸齒（TAA），包含投影抖動、移動向量、歷史重投影與歷史拒絕。

**需要時再學：** HDR 渲染目標、以 mip 鏈做泛光、次像素抖動、移動向量、鄰域限制（neighborhood clamping）、殘影與模糊之間的取捨。

**檢核：** 比較無抗鋸齒、TAA，以及移動向量故意算錯的 TAA，用放大擷取解釋你看到的每個瑕疵。

### E6 — GPU 驅動的剔除與間接繪製

**先備：** E1、C3。**時數：** 50–80 小時。

**製作：** 用計算著色器做視錐剔除與遮擋剔除（階層式 Z，Hi-Z），寫出間接繪製指令，再用一次 multi-draw-indirect-count 呼叫（Direct3D 12：`ExecuteIndirect`）畫出上千個物件。

**需要時再學：** 間接繪製參數、繪製數量緩衝區、Hi-Z 金字塔、兩階段遮擋剔除、CPU 與 GPU 提交成本。mesh shader 列為選修延伸。

**檢核：** 比較 10,000 個物件在 CPU 剔除與 GPU 剔除下的 CPU 時間與 GPU 時間，解釋兩種版本分別把時間花在哪裡。

### E7 — 硬體光線追蹤

**先備：** E3、C3。**時數：** 50–80 小時。

**製作：** 為場景建立底層與頂層加速結構，再從計算或片段著色器用 ray query 做光線追蹤陰影或環境光遮蔽（Direct3D 12：DXR inline ray tracing）。完整的光線追蹤 pipeline 列為選修。

**需要時再學：** 加速結構的建立與更新、instancing、ray query 與光線追蹤 pipeline 的差別、降噪基礎、每條光線的成本。用 Nsight Graphics 檢視光線追蹤工作：RenderDoc 能擷取，但無法除錯。

**檢核：** 從畫質與量測成本兩方面，比較 E4 的陰影貼圖和光線追蹤陰影，並說明你會選哪一種上線。

### E8 — 多執行緒錄製、素材管線與除錯介面

**先備：** E1、E2。**時數：** 40–60 小時。

**製作：** 多執行緒錄製指令；glTF 2.0 載入器；離線產生 KTX2 或 DDS 容器中區塊壓縮（BCn）貼圖的處理步驟；以及引擎內的除錯介面（例如 Dear ImGui），用來切換功能與顯示 GPU 時間。

**需要時再學：** 每個執行緒各自的 command pool、secondary command buffer 或 bundle、工作系統、glTF 的場景與材質、BC1／BC5／BC7／BC6H 格式與各自適用的情況、一次載入與串流的成本差異。

**檢核：** 展示壓縮與多執行緒前後的載入時間與影格時間數據，以及除錯介面至少控制三項渲染器功能。

### 引擎路線期末專題 — 迷你引擎渲染器

**先備：** E1–E8。**時數：** 80–150 小時。

**製作：** 以你在第二部寫的程式碼為基礎，從零完成一個渲染器：載入 glTF 場景，使用 PBR 材質、陰影、後製與 TAA、GPU 驅動的剔除，以及至少一項光線追蹤效果；每個主要 pass 都有除錯檢視。

**交付：** 原始碼、簡短影片，以及一份**影格時間預算報告**：在註明的解析度與 present mode 下，於 RTX 5070 Ti 量測每個 pass 的 GPU 時間，並附一次 Nsight Graphics 擷取，支持你最重要的最佳化主張。

---

## 2D、文字與合成渲染路線

目標：桌面使用者介面底下的渲染層：向量路徑、文字，以及能正確又有效率地合成各個表面的合成器。讓每個模組的介面保持乾淨：**輸入表面，輸出合成後的影格**。之後規劃中的作業系統研究會重用這些模組（見最後一節）。

### T1 — 在 CPU 上繪製向量路徑

**先備：** 共同核心（至少完成 C5）、第一部實驗 01。**時數：** 50–80 小時。

**製作：** CPU 路徑光柵器：直線與二次／三次 Bézier 曲線、曲線細分、nonzero 與 even-odd 填充規則、以精確面積計算涵蓋率的抗鋸齒，以及含接合、端點與虛線的描邊。

**需要時再學：** Bézier 數學與自適應細分、有向面積累加、填充規則、描邊輪廓與其失敗情況，以及涵蓋率為何必須在線性光中混合。

**檢核：** 渲染一張路徑與描邊的測試圖，在 400 % 放大下與參考渲染器比較，解釋每一處差異。

### T2 — 在 GPU 上繪製向量路徑

**先備：** T1、C3。**時數：** 60–100 小時。

**製作：** 用計算著色器把路徑渲染移到 GPU：把路徑線段分配到圖塊、逐圖塊計算涵蓋率，再合成。研究 Vello 系列開源渲染器並與你的 CPU 版本比較：以計算著色器為中心的研究版渲染器（alpha 階段），以及較新的「sparse strips」渲染器 `vello_cpu` 與 `vello_gpu`。

**需要時再學：** 圖塊化與分箱、GPU 前綴和、稀疏與密集涵蓋率、conflation 瑕疵、場景大小不受限時的記憶體預算。

**檢核：** 量測包含上千條路徑的場景在 CPU 與 GPU 上的時間，並各舉一個兩種做法分別勝出的例子。

### T3 — 文字渲染

**先備：** T1、C5。**時數：** 60–100 小時。

**製作：** 文字管線：用 HarfBuzz 做文字塑形，光柵化字形（FreeType 或你在 T1 寫的光柵器），把字形快取到字形圖集，以斷行規則排版段落，並在 GPU 上繪製。比較灰階抗鋸齒、hinting 與次像素定位，並嘗試次像素（LCD）渲染。選修比較：多通道有向距離場圖集（msdfgen），以及 HarfBuzz 較新的光柵與 GPU 函式庫（這個專案起初只做文字塑形）。

**需要時再學：** 文字塑形在做什麼（連字、字距、複雜文字）、雙向文字與斷行的概要、字形度量、hinting（FreeType 會做，HarfBuzz 不做）、文字的 gamma 與對比、次像素抗鋸齒為何在透明與旋轉時失效，以及 Microsoft 為何現在建議 Windows 上大多數文字使用灰階抗鋸齒。

**檢核：** 以 100 % 與 150 % 縮放渲染同一段拉丁文字、繁體中文與一種複雜文字，解釋你的渲染選擇與一個瑕疵。

### T4 — 合成

**先備：** T3、C2。**時數：** 50–80 小時。

**製作：** 一個合成器函式庫：輸入一組表面（位置、大小、不透明度、變換、含圓角的裁切），輸出一個影格。使用預乘 alpha 並在線性光中混色，追蹤受損區域，只重繪有變動的部分，略過被不透明表面完全遮住的表面，並在非整數縮放倍率下正確渲染。

**需要時再學：** 預乘 alpha 與 Porter–Duff 運算子、圖層樹、受損區域、不透明區域的遮擋剔除、Windows 的 HiDPI 與每台螢幕各自的 DPI 感知（v2）、非整數縮放時的像素對齊。

**平台說明：** 受損區域追蹤一定能省下渲染工作，因為只把受損區域重繪進合成影像。告訴*呈現引擎*哪些區域改變，則是另一回事。Direct3D 12 可透過 DXGI 的 `Present1` 加上 dirty rectangle 做到，但需要 `FLIP_SEQUENTIAL` 而非 `FLIP_DISCARD`。我們查到的公開資料庫中，RTX 5070 Ti 的 Windows 驅動沒有回報 Vulkan 的 `VK_KHR_incremental_present` 擴充，所以 Vulkan 版本會呈現完整影格。請在自己的電腦上用 `vulkaninfo` 確認並記錄結果。

**檢核：** 在畫面上顯示受損區域與每個影格重繪的像素數，並展示一個場景，讓在 sRGB 編碼值中混色和在線性光中混色看起來明顯不同。說明你的呈現是部分更新還是完整影格，以及原因。

### T5 — 桌面內容的色彩管理與 HDR

**先備：** T4、C5。**時數：** 30–50 小時。

**製作：** 把 SDR（sRGB）表面與 HDR 表面合成到 HDR 輸出，提供可調整的 SDR 白色亮度，並讓 SDR 內容看起來保持不變。

**需要時再學：** 混合 SDR 與 HDR 內容、參考白（在 Windows 上以 scRGB 合成時，SDR 內容要乘上顯示器的 SDR 白色亮度除以 80 nits）、介面用的色調映射、表面的色彩空間、ICC 色彩描述檔概要。

**檢核：** 用每一步的數值，說明一個 sRGB 白色像素如何在你的 HDR 輸出中變成顯示訊號。

### 2D、文字與合成路線期末專題 — 單一行程的 GPU 合成器

**先備：** T1–T5。**時數：** 80–150 小時。

**製作：** 一個 Windows 應用程式，在 RTX 5070 Ti 上使用 Vulkan 或 Direct3D 12，合成數個表面，例如以你在 T1–T3 寫的模組繪製向量圖形與文字的模擬視窗。它要做受損區域追蹤、遮擋剔除與正確的色彩處理，包含 HDR 模式。記下你選的 API 與理由。

**交付：** 原始碼、簡短影片，以及一份量測報告：在閒置桌面、一個有動畫的視窗與最壞情況下，分別列出影格時間、每個影格重繪的像素數，以及用 PresentMon（例如 `DisplayLatency` 與 `MsClickToPhotonLatency`）與自己的時間戳量測的輸入到顯示延遲；並註明是否使用部分呈現。

---

## 第二部之後：規劃中的作業系統研究

視窗系統架構、核心的顯示介面、硬體顯示平面、跨行程緩衝區共享、輸入堆疊，以及真正的系統合成器，**都不屬於**本課程。這些主題將在本儲存庫規劃中的另一項作業系統研究繼續，目前還沒有對應的資料夾。那項研究需要 Linux；第一部與第二部都不需要。

本路線的 2D、文字與合成模組預計會在那裡重用。讓它們的介面保持精簡（輸入表面，輸出合成後的影格），之後的系統合成器就能直接驅動，不必修改。

## 完成第二部代表什麼

- **代表完成：** 你從零做出所選路線的期末專題，有量測數據支持設計選擇，並能讀懂、解釋相應開源專案的對應部分（例如引擎路線的 Godot 渲染器，或 2D 與文字路線的 Skia 與 Vello）。更高的目標是在這類專案中有 patch 被接受。
- **不代表：** 資深或正式產品等級的能力。那仍需要在真實團隊中，面對真實內容與各式硬體累積經驗。本計畫幫你為那份工作做準備，但無法取代它。

## 粗略時數

| 部分 | 階段 | 時數 |
| --- | --- | --- |
| 共同核心 | C0–C5 | 190–300 |
| 引擎路線 | E1–E8 與期末專題 | 420–680 |
| 2D、文字與合成路線 | T1–T5 與期末專題 | 330–560 |
| **共同核心加一條路線** | | **約 520–980** |

這些是規劃用的估計，不是保證。記錄自己在每個階段實際花的時數，再據此調整。

## 參考資料

於 2026-11-15 依官方頁面核對。版本更新很快，安裝前請先確認最新版本。

**API、慣例與工具（共同核心）**

- Vulkan 規格：[視口變換](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-viewport)、[裁切](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-clipping)、[多邊形方向](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-polygons-basic)、[各版本提升為核心的擴充](https://docs.vulkan.org/spec/latest/appendices/versions.html#versions-1.4-promotions)；[Khronos 的 Vulkan 1.4 公告](https://www.khronos.org/news/press/khronos-streamlines-development-and-deployment-of-gpu-accelerated-applications-with-vulkan-1.4)（2024 年 12 月）。
- [LunarG Vulkan SDK](https://www.lunarg.com/products/vulkan-sdk/) 與 [Windows 入門說明](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html)；[Vulkan Memory Allocator](https://gpuopen.com/vulkan-memory-allocator/) 與 [D3D12 Memory Allocator](https://gpuopen.com/d3d12-memory-allocator/)。
- 畫面呈現：[Vulkan present mode](https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html)（只有 FIFO 保證支援）、[`VK_KHR_present_wait2`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_wait2.html)；DXGI 的 [swap effect](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect)、[畫面撕裂支援](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays)、[frame latency waitable object](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-getframelatencywaitableobject)。
- Direct3D 12：[光柵器狀態與正面預設值](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_rasterizer_desc)、[Agility SDK](https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/)、[enhanced barrier](https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html)、[GPU 計時](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing)。
- 量測：[Vulkan `timestampPeriod`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceLimits.html)；[Nsight Graphics](https://developer.nvidia.com/nsight-graphics/get-started)（Direct3D 12 與 Vulkan 使用 Graphics Capture 與 GPU Trace；支援 RTX 50 系列）；[RenderDoc](https://renderdoc.org/) 及其[光線追蹤限制](https://renderdoc.org/docs/behind_scenes/raytracing.html)（能擷取，但不能除錯）；[PresentMon 指標](https://raw.githubusercontent.com/GameTechDev/PresentMon/main/README-ConsoleApplication.md)。
- 硬體：[GeForce RTX 5070 系列規格](https://www.nvidia.com/zh-tw/geforce/graphics-cards/50-series/rtx-5070-family/)（Vulkan 1.4、OpenGL 4.6、DirectX 12 Ultimate）。
- 色彩：[sRGB，IEC 61966-2-1](https://webstore.iec.ch/en/publication/6169)（公式亦見 [CSS Color 4](https://www.w3.org/TR/css-color-4/)）；[DirectX 的 Advanced Color 與 HDR](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)；[Auto Color Management](https://devblogs.microsoft.com/directx/auto-color-management)；Vulkan 的 [swapchain 色彩空間](https://docs.vulkan.org/refpages/latest/refpages/source/VkColorSpaceKHR.html)與 [HDR metadata](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_hdr_metadata.html)。

**引擎路線**

- 渲染圖：Yuriy O'Donnell，[FrameGraph: Extensible Rendering Architecture in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in)（GDC 2017）。
- PBR 與 IBL：Brian Karis，[Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)（SIGGRAPH 2013）；[Physically Based Rendering in Filament](https://google.github.io/filament/Filament.html)。
- 陰影與抗鋸齒：[Cascaded Shadow Maps](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps)；Brian Karis，[High-Quality Temporal Supersampling](https://advances.realtimerendering.com/s2014/)（SIGGRAPH 2014）。
- GPU 驅動渲染：[`vkCmdDrawIndexedIndirectCount`](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdDrawIndexedIndirectCount.html)（1.2 起為核心）、[`ExecuteIndirect`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect)、[`VK_EXT_mesh_shader`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html)（選修）。
- 光線追蹤：[Vulkan 光線追蹤正式規格](https://www.khronos.org/blog/vulkan-ray-tracing-final-specification-release)；[DXR 1.1 與 inline ray tracing](https://devblogs.microsoft.com/directx/dxr-1-1/)。
- 素材與工具：[glTF](https://www.khronos.org/gltf/)（亦為 ISO/IEC 12113:2022）；[KTX 2.0](https://www.khronos.org/ktx/) 與 [`ktx create`](https://github.khronos.org/KTX-Software/ktxtools/ktx_create.html)；[texconv](https://github.com/microsoft/DirectXTex/wiki/Texconv)；[區塊壓縮格式](https://learn.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11)；[Dear ImGui backend](https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md)；[Godot Engine 原始碼](https://github.com/godotengine/godot)。

**2D、文字與合成路線**

- 向量渲染：[Vello](https://github.com/linebender/vello) 與其[架構說明](https://github.com/linebender/vello/blob/main/ARCHITECTURE.md)；Raph Levien，[Fast 2D rendering on GPU](https://raphlinus.github.io/rust/graphics/gpu/2020/06/13/fast-2d-rendering.html)；[Inside the fastest font renderer in the world](https://medium.com/@raphlinus/inside-the-fastest-font-renderer-in-the-world-75ae5270c445)（有向面積累加）；Diego Nehab，[Converting Stroked Primitives to Filled Primitives](https://w3.impa.br/~diego/projects/Neh20/index.html)（SIGGRAPH 2020）；[Skia](https://skia.org/)。
- 文字：HarfBuzz 的 [What is text shaping?](https://harfbuzz.github.io/what-is-harfbuzz.html#what-is-text-shaping) 與 [What HarfBuzz doesn't do](https://harfbuzz.github.io/what-harfbuzz-doesnt-do.html)；[FreeType](https://freetype.org/)；[DirectWrite](https://learn.microsoft.com/en-us/windows/win32/directwrite/introducing-directwrite)；[msdfgen](https://github.com/Chlumsky/msdfgen)；[UAX #9 雙向文字](https://www.unicode.org/reports/tr9/)；[UAX #14 斷行](https://www.unicode.org/reports/tr14/)；[OpenType 規格](https://learn.microsoft.com/en-us/typography/opentype/spec/)。
- 合成：Porter 與 Duff，[Compositing Digital Images](https://history.siggraph.org/?p=107468)（SIGGRAPH 1984）；[`DXGI_ALPHA_MODE`](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/ne-dxgi1_2-dxgi_alpha_mode)；[DXGI dirty rectangle](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-2-presentation-improvements)；[`VK_KHR_incremental_present` 支援資料](https://vulkan.gpuinfo.org/displayextensiondetail.php?extension=VK_KHR_incremental_present)；[DPI awareness context](https://learn.microsoft.com/en-us/windows/win32/hidpi/dpi-awareness-context) 與 [`WM_DPICHANGED`](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged)。
