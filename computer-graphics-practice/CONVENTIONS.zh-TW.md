# 課程慣例

[English](CONVENTIONS.md) | **繁體中文**

每個渲染器都得選定幾項慣例。把程式從一個圖形 API 移植到另一個時，大部分錯誤都來自其中某項慣例悄悄改變。本頁是課程唯一的慣例清單：列出第一部（CPU 實驗與 WebGL 2）採用的做法、哪些實驗依賴各項慣例，以及第二部換到 Vulkan 或 Direct3D 12 時會有什麼變化。各實驗的 README 會重述它依賴的項目。

## 總覽

| 慣例 | 第一部（CPU 實驗、WebGL 2） | 實驗 | Vulkan | Direct3D 12 |
| --- | --- | --- | --- | --- |
| 座標系手性 | 右手世界與視圖空間，+y 朝上；攝影機朝自己的 −z 看 | 03–06 | API 不規定；由你的矩陣決定 | API 不規定；DirectXMath 同時提供左手與右手版本的函式 |
| 向量慣例 | 行向量，`p' = M · p`，因此 `P · V · M · p` 先套用 M | 02–06 | GLSL：相同（`M * v`） | HLSL：行向量寫成 `mul(M, v)`；DirectXMath 使用列向量（`v · M`） |
| 矩陣儲存 | 實驗 02：3×3 以巢狀列儲存 `m[row][col]`。實驗 03 起：扁平的行優先陣列 `m[col * 4 + row]`，以 `transpose = false` 上傳 | 02–06 | GLSL 預設行優先 | HLSL 常數緩衝區預設以行優先打包；DirectXMath 矩陣在記憶體中為列優先 |
| NDC 的 y | +y 朝上 | 03–06 | **+y 朝下** | +y 朝上 |
| 影格緩衝區原點 | CPU 實驗：像素第 0 列在最上方。WebGL 視窗座標（`gl_FragCoord`、`readPixels`）：左下角 | 01–06 | 左上角 | 左上角 |
| 裁切空間深度 | `−w ≤ z ≤ w`，NDC z 在 [−1, 1]，視窗深度 `d = (z_ndc + 1) / 2` | 03–06 | **`0 ≤ z ≤ w`**，NDC z 在 [0, 1]（可用擴充改成 [−1, 1]） | **`0 ≤ z ≤ w`** |
| 深度測試 | 清除為 1，較小者通過；反轉 Z 是實驗 04 的 B 題 | 04–06 | 可選任何比較方式；常用反轉 Z | 同 Vulkan |
| 正面 | NDC 中（+y 朝上）逆時針為正面；剔除背面 | 01–02（有向面積）、04–06 | `frontFace` 是 pipeline 狀態，依影格緩衝區座標判斷 | `FrontCounterClockwise` 預設為 FALSE：**順時針為正面** |
| 像素中心 | 像素 (x, y) 的取樣點在 (x + 0.5, y + 0.5) | 01–06 | 相同 | 相同 |
| 貼圖列順序 | 影像第 0 列在最上方；在 WebGL 中，除非設定 `UNPACK_FLIP_Y_WEBGL`，它會成為 v = 0 | 06 | 記憶體中的第 0 列是 v = 0 | 記憶體中的第 0 列是 v = 0 |
| 色彩 | 8 位元色彩以 sRGB 編碼；混色、光照、濾波與混合都在線性光中計算，輸出時只編碼一次 | 02、05、06 | `*_SRGB` 格式在取樣時解碼、寫入時編碼；HDR 透過 swapchain 色彩空間 | `*_UNORM_SRGB` 格式同理；HDR 透過 scRGB 或 HDR10 swap chain |
| 角度與單位 | 程式中用弧度，滑桿顯示角度；世界單位未指定（可想成公尺） | 02–06 | — | — |

## 細節與第二部說明

### 座標系手性與攝影機

本課程使用 +y 朝上的右手世界座標。攝影機朝自己的 −z 軸看：[`labs/common/math.js`](labs/common/math.js) 的 `cameraBasis` 回傳 `right`、`up` 與 `forward`，而視圖矩陣的第三列是攝影機的*後方*軸 `−forward`。Vulkan 和 Direct3D 都不規定世界或視圖空間的手性；只有投影與裁切體才是 API 的慣例。到了第二部，保留右手世界，只改投影與視口（見下文）。許多 Direct3D 範例使用左手座標，因為 DirectXMath 的 `…LH` 函式長期是預設選擇。若混用這類範例的程式碼，請只在一個邊界處轉換，並記下位置。

### 向量與儲存順序

行向量代表 `M · p`，由右往左讀：在 `P · V · M` 中，模型矩陣最先作用。實驗 02 以巢狀列儲存 3×3 矩陣，讓程式讀起來像紙上的數學；從實驗 03 起，`labs/common/math.js` 以扁平的行優先陣列（`m[col * 4 + row]`）儲存 4×4 矩陣，並用 `fromRows` 讓你逐列寫出矩陣。WebGL 的 `uniformMatrix4fv(location, false, m)` 可直接讀取這種排列。Vulkan 使用的 GLSL 也預設為行優先矩陣。HLSL 的常數緩衝區預設以行優先打包，但 DirectXMath 使用列向量與列優先儲存，所以它的矩陣通常在上傳時轉置，或在著色器中宣告為 `row_major`。每個程式庫只選一條規則，並用非對稱矩陣測試。

### NDC 的 y 方向與影格緩衝區原點

在 OpenGL 與 WebGL 中，NDC +y 朝上，視窗座標從左下角開始；CPU 實驗則把 NDC +y 朝上映射到向下遞增的像素列（`ndcToPixel`）。Vulkan 的 NDC +y **朝下**，影格緩衝區原點在左上角，所以直接沿用 OpenGL 風格的投影，畫面會上下顛倒。只在一個地方修正：使用負的視口高度（Vulkan 1.1 起允許），或把投影矩陣的 y 列取負。Direct3D 12 的 NDC +y 仍朝上，並映射到原點在左上角的渲染目標，所以 OpenGL 風格的投影在那裡畫面是正的。

### 裁切空間深度

第一部採用 OpenGL／WebGL 的裁切體 `−w ≤ z ≤ w`，因此 NDC z 在 [−1, 1]，視窗深度是 `d = (z_ndc + 1) / 2`。Vulkan 與 Direct3D 12 裁切到 `0 ≤ z ≤ w`，NDC z 本身就是 [0, 1] 的儲存深度。若直接沿用第一部的 `perspective()` 矩陣，所有 NDC z 為負的部分都會被裁掉，實際上就是比兩倍近裁切距離更近的東西。請改用 [0, 1] 的投影：對同樣的右手視圖空間，第三列改為 `[0, 0, far / (near − far), far · near / (near − far)]`。Vulkan 的 `VK_EXT_depth_clip_control` 擴充也能讓 pipeline 改用 [−1, 1] 範圍。第二部使用 [0, 1] 加上反轉 Z（近處存 1、遠處存 0、以「較大」通過測試）搭配浮點深度緩衝區，精度分布比實驗 04 的映射平均得多。

### 正面與繞序

第一部的網格從外面看是逆時針，正面則定義為 NDC（+y 朝上）中的逆時針（`gl.frontFace(gl.CCW)`、實驗 04 的 `ndcArea`）。Vulkan 把正面設為 pipeline 設定，並依影格緩衝區座標判斷方向。若照上文翻轉 y 讓畫面變正，螢幕上看起來逆時針的三角形仍是逆時針，因此保留 `VK_FRONT_FACE_COUNTER_CLOCKWISE` 即可。若沒有翻轉，畫面與所有繞序都會鏡射，剔除就會移除你想保留的面。Direct3D 12 預設把**順時針**三角形視為正面，除非 `FrontCounterClockwise` 為 TRUE；本課程的網格請設為 TRUE。

### 像素中心與貼圖列順序

三種 API 都在像素中心 (x + 0.5, y + 0.5) 取樣，與實驗 01 相同。影像檔與 canvas 先存最上面的一列。WebGL 會把這第一列上傳為貼圖的 v = 0，除非設定 `UNPACK_FLIP_Y_WEBGL`（實驗 06 的 F 題）；Vulkan 與 Direct3D 12 也把記憶體中的第一列視為 v = 0，貼圖座標 (0, 0) 位於影像左上角。

### 色彩：內部用線性光，進出時用 sRGB

本課程的 8 位元色彩以 sRGB 編碼：色彩選擇器、影像、canvas 與 WebGL 預設影格緩衝區都使用 sRGB 傳輸函數。任何把光相加或平均的運算都必須在線性光中進行：頂點間插值、光照、貼圖濾波與 mipmap，以及混合。輸入時解碼、計算，輸出時**只編碼一次**。公式放在 `labs/common/color.js`。

- 實驗 01 刻意直接混合編碼後的數值；實驗 02 加上切換，方便比較。
- 實驗 03 與 04 使用單色面、沒有混色，因此不涉及這個問題。
- 實驗 05 在頂點著色器解碼頂點色、在片段著色器編碼（可切換）。
- 實驗 06 使用 `SRGB8_ALPHA8` 貼圖，讓 GPU 在濾波前解碼，並在著色器中為光照結果編碼。
- 除錯檢視（深度、法線、UV）直接輸出數值：它們是量測值，不是光。

到了第二部，`*_SRGB` 影像格式會讓硬體在取樣時解碼、在寫入（包含混合）時編碼，而 sRGB 的 swapchain 格式則取代手動編碼。HDR 輸出使用擴展範圍的線性格式（scRGB）或 PQ 編碼格式（HDR10），第二部的色彩科學階段會涵蓋兩者。

## 移植檢查清單

把任何實驗移到新的 API 時，用刻意不對稱的場景（例如實驗 02 的 F，或實驗 06 的紅色貼圖格）逐項記錄並測試：

1. 畫面是正的嗎？（NDC 的 y、視口、影格緩衝區原點）
2. 被剔除的是正確的面嗎？（任何 y 翻轉之後的正面設定）
3. 攝影機前方有沒有東西被誤裁？（投影的深度範圍）
4. 非對稱矩陣傳進著色器後保持不變嗎？（儲存順序、transpose 參數、`row_major`／`column_major`）
5. 紅色貼圖格在預期的角落嗎？（貼圖列順序）
6. 50 % 灰階漸層看起來正確嗎？黑白棋盤格在遠處平均出的灰色正確嗎？（sRGB 解碼與編碼各只發生一次）

## 參考資料

於 2026-11-15 依官方頁面核對。

- WebGL 2／OpenGL ES 3.0：[裁切體與多邊形方向](https://registry.khronos.org/OpenGL/specs/es/3.0/es_spec_3.0.pdf)（§2.18、§3.6.1）、[`glFrontFace`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glFrontFace.xhtml)（初始值為 CCW）、[`glDepthRangef`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glDepthRangef.xhtml)、[`glReadPixels`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glReadPixels.xhtml)（原點在左下角）、[`glUniform`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glUniform.xhtml)（transpose）。
- Vulkan：[視口變換](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-viewport)（原點在左上角）、[裁切體](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-clipping)（0 ≤ z ≤ w）、[多邊形方向](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-polygons-basic)（在影格緩衝區座標中計算面積，公式前有負號）、[`VK_KHR_maintenance1`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_maintenance1.html)（負的視口高度，1.1 起為核心）、[`VK_EXT_depth_clip_control`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_depth_clip_control.html)、[Vulkan 用 GLSL 的預設值](https://github.com/KhronosGroup/GLSL/blob/main/extensions/khr/GL_KHR_vulkan_glsl.txt)（block 預設行優先）。
- Direct3D：[`D3D12_RASTERIZER_DESC`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_rasterizer_desc)（FrontCounterClockwise 預設為 FALSE）、[像素座標](https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-coordinates)（原點在左上角）、[視口與裁切](https://learn.microsoft.com/en-us/windows/win32/direct3d9/viewports-and-clipping)（裁切空間 y 從 1 到 −1，由視口翻轉）、[HLSL 矩陣打包](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math)、[DirectXMath 慣例](https://learn.microsoft.com/en-us/windows/win32/dxmath/pg-xnamath-getting-started)。
- 色彩：[sRGB，IEC 61966-2-1](https://webstore.iec.ch/en/publication/6169)，傳輸函數亦見 [CSS Color 4](https://www.w3.org/TR/css-color-4/)；[DirectX 的 Advanced Color 與 HDR](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range)；[Vulkan swapchain 色彩空間](https://docs.vulkan.org/refpages/latest/refpages/source/VkColorSpaceKHR.html)。
