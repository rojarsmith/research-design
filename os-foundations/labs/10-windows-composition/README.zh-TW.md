[English](README.md) | **繁體中文** | [實作建置計畫，階段 10](../../SYLLABUS.zh-TW.md)

# 實驗 10：視窗系統架構，在 Windows 上量測

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **`flipstats.exe`（練習 A–D）：已建置並執行**，於 2026-11-16 在本機以一般權限執行，編譯器為 WinLibs 的 gcc 16.2.0，使用它附帶的 MinGW-w64 14.0.0 標頭檔與匯入程式庫（不需要 Windows SDK）：`info` 一次；視窗模式與被遮住的視窗模式，各在 vsync 開與關下跑兩次；視窗模式加上最大影格延遲 1，跑兩次。輸出：[`windows/example-output.txt`](windows/example-output.txt)。其中沒有出現帳號名稱、電腦名稱或 SID。
> - **執行的地方不是這個階段預定的地方。** 這台機器是 Intel Core i7-11700，搭配 Intel UHD Graphics 750（驅動程式 31.0.101.5334），沒有其他 GPU，作業系統為 Windows 11 Pro 組建 26300.9711，而且程式是在**遠端桌面工作階段**中執行。這個工作階段唯一的顯示器就是遠端顯示器，每個 API 都回報它是 1440 × 900、32 Hz。每個影格都是為這個遠端顯示器合成的，影格統計中與 vblank 有關的欄位一直是 0。這裡沒有任何數字量測到 RTX 5070 Ti、實體螢幕、Independent Flip 或 overlay 平面。請在你要研究的機器的本機主控台上全部重跑一次。
> - **無邊框全螢幕（練習 E）：未執行。** 它會蓋住正在使用這台機器的人的整個螢幕。它的程式碼和視窗模式只差在建立的視窗，但從來沒有執行過。
> - **影格統計分析中與 vblank 有關的一半**（從 present 到 vblank 的延遲、glitch 計數）也從未執行，因為 vblank 欄位一直是 0。程式中的這幾行沒有經過測試。
> - **PresentMon（練習 F）：未執行**，也沒有下載。命令列依照 2026-11-16 讀到的主控台版 README 撰寫，`PresentMode` 各值的意義也取自該文件。
> - **未執行：** 任何 ETW 或 WPR 追蹤、Win+Ctrl+Shift+B（練習 G）、TDR 或任何讓 GPU 停止回應的實驗（只有描述），以及任何顯示設定的變更。
> - **文件：** 下方連結的 Learn 頁面、DirectX 部落格文章與 PresentMon README 都在 2026-11-16 讀過。

## 目標

第二部分從一個你無法取代的系統開始。Windows 程式呈現（present）一個影格時，要嘛由 DWM 把它合成進桌面，要嘛由顯示硬體直接掃描輸出程式自己的緩衝區，DWM 退到一旁。本實驗建立一個使用 flip-model swapchain 的小型 Direct3D 11 程式，記錄一般 API 對影格去向能說的一切，再指出這些 API 說不出的部分，也就是 PresentMon 的 ETW 擷取所回答的問題。程式以三種設定執行：視窗模式、被另一個視窗遮住一部分的視窗，以及蓋住整個螢幕的無邊框視窗。

```
 你的行程                              dwm.exe（每個工作階段一個）          顯示硬體
 D3D11 + DXGI，flip model
 Present(緩衝區 N) ───────────────────▶ 合成：讀取緩衝區 N（共用，      ──▶ 主平面掃描輸出
   緩衝區與 DWM 共用；                   不複製）到自己的影格                 DWM 的影格
   不複製到重新導向表面                                                      PresentMon："Composed: Flip"

 視窗蓋滿螢幕、上面沒有東西、 ────────── independent flip：DWM 停止  ─────▶ 主平面直接掃描輸出
 大小與格式相符                           合成；緩衝區 N 直接送到平面          緩衝區 N
                                                                              "Hardware: Independent Flip"

 驅動配給一個 overlay 平面 ───────────── DWM 在主平面上合成 ──────────────▶ overlay 平面：緩衝區 N；
 （多平面 overlay，MPO）                  其餘的一切                          主平面：DWM 的影格
                                                                              "Hardware Composed: Independent Flip"
```

階段 13 會從 Linux 這一側遇到同樣的東西：平面、翻頁、vblank 與 VRR。在 Windows 上，由 DWM 決定一個影格走哪一條路；在 Linux 上，由你的合成器決定。

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`windows/flipstats.c`](windows/flipstats.c) | 以 C 寫成的 D3D11 程式：一個清點模式（`info`）與三種視窗設定，各可選 vsync 開或關、`FLIP_DISCARD` 或 `FLIP_SEQUENTIAL`、緩衝區數量與影格延遲；記錄 `QueryPerformanceCounter`、`GetLastPresentCount`、`GetFrameStatistics`、`GetFrameStatisticsMedia`、`DwmGetCompositionTimingInfo` 與 `WaitForVBlank` | 已在此建置並執行（`info`、`windowed`、`overlap`）；`borderless` 未執行 |
| [`windows/example-output.txt`](windows/example-output.txt) | 輸出紀錄，在遠端桌面工作階段中的十一次執行 | 2026-11-16 記錄 |
| PresentMon CSV 檔 | 你在練習 F 的擷取 | 此處沒有記錄 |

## 環境設定

- **編譯器：** 和前面幾個實驗一樣，使用 WinLibs（MinGW-w64）的 gcc。它的標頭檔包含 `d3d11.h`、`d3d11_1.h`、`dxgi1_2.h` 到 `dxgi1_6.h` 與 `dwmapi.h`，程式庫包含 `libd3d11.a`、`libdxgi.a` 與 `libdwmapi.a`，所以不需要 Windows SDK。程式只用清除（clear）來繪圖，所以也不需要著色器編譯器。（WinLibs 也附有 `d3d12.h`、`libd3d12.a` 與 `libd3dcompiler.a`，想用 D3D12 也可以；兩者的呈現路徑都屬於 DXGI。）

  ```powershell
  cd windows
  gcc -O2 -Wall -o flipstats.exe flipstats.c -ld3d11 -ldxgi -ldwmapi -luser32 -lgdi32 -ladvapi32
  .\flipstats.exe info
  ```

- **在哪裡執行：** 在你要研究的機器（本課程是 RTX 5070 Ti 桌機）的本機主控台上、以本機登入執行，不要透過遠端桌面。程式會印出 `GetSystemMetrics(SM_REMOTESESSION)`，它必須是 0。把其他視窗關掉，或移離右下角與你要測試的螢幕。
- **PresentMon**（練習 F），從它的 [GitHub releases](https://github.com/GameTechDev/PresentMon/releases) 取得。2026-11-16 時最新的版本是 v2.6.0，主控台程式就是單一檔案 `PresentMon-2.6.0-x64.exe`（957 KB）。請到 releases 頁面確認有沒有更新的版本。它透過 ETW 工作階段記錄；它的 README 提供 `--restart_as_admin`，用於沒有提升權限的情況，所以請在提升權限的命令提示字元中執行。

## 先讀

- Learn：[DXGI flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)（flip 與 bitblt、視窗模式下的影格統計、偵測 glitch 的步驟）、[IDXGISwapChain::Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)（flip model 的同步間隔、VRR 需要允許畫面撕裂）、[DXGI_FRAME_STATISTICS](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_frame_statistics)、[GetFrameStatistics](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-getframestatistics)、[DwmGetCompositionTimingInfo](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetcompositiontiminginfo)。
- Shawn Hargreaves，[For best performance, use DXGI flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/)（DirectX 部落格，2018-04-09）：DirectFlip、Independent Flip、MPO，以及 `FLIP_DISCARD` 帶來的「反向合成」（reverse composition）。
- Learn，驅動程式這一側：[WDDM 架構](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)、[MPO](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support)、[TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery)。
- PresentMon 的[主控台程式 README](https://github.com/GameTechDev/PresentMon/blob/main/README-ConsoleApplication.md)：選項、CSV 欄位與 `PresentMode` 的值。
- 課綱的[階段 10](../../SYLLABUS.zh-TW.md)，關於 X11、Wayland，以及 DWM 為何無法取代。

## 程式

`flipstats.exe` 在擁有視窗所在螢幕的轉接器上建立 D3D11 裝置，並以 `CreateSwapChainForHwnd` 建立 swapchain：`DXGI_FORMAT_B8G8R8A8_UNORM`、兩個緩衝區、`DXGI_SWAP_EFFECT_FLIP_DISCARD`（加 `--sequential` 改為 `FLIP_SEQUENTIAL`），當 `IDXGIFactory5::CheckFeatureSupport` 回報支援畫面撕裂時，再加上 `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING`。每個影格它把緩衝區清成新的顏色，畫一條每個影格移動 4 像素的白色長條（撕裂時長條會斷開），然後呼叫 `Present(1, 0)`；加上 `--vsync 0` 時改為 `Present(0, DXGI_PRESENT_ALLOW_TEARING)`。它宣告支援每個螢幕各自的 DPI，讓 Windows 不會在有縮放的螢幕上縮放它的視窗（那會迫使視窗被合成），並以 `DXGI_MWA_NO_ALT_ENTER` 停用 Alt+Enter。

| 設定 | 視窗 | 安全措施 |
| --- | --- | --- |
| `windowed` | 320 × 240 的工作區，靠近工作區域的右下角 | `WS_EX_NOACTIVATE`、`SW_SHOWNOACTIVATE`：絕不成為前景視窗，沒有工作列按鈕；不是最上層視窗 |
| `overlap` | 同上，再加上一個屬於它的 160 × 120 不透明彈出視窗，蓋住工作區的右下部分 | 同上；被擁有的視窗永遠位於擁有者之上 |
| `borderless` | 剛好蓋住主螢幕的 `WS_POPUP`，正常顯示並帶到前景 | 整段執行期間都會蓋住你的螢幕；按 Esc 可提早結束 |

每種設定都會自行結束（預設 4 s，最多 30 s），另有一個看門狗執行緒，會在執行應該結束後 5 s 終止行程。

## 證據，以及每一項能證明什麼

應用程式能呼叫的東西裡，沒有一個能逐影格、有文件依據地告訴它 DWM 有沒有合成那個影格。以下是記錄中每個來源實際說了什麼：

| 來源 | 回報什麼 | 能顯示什麼 | 不能顯示什麼 | 在記錄的執行中 |
| --- | --- | --- | --- | --- |
| `Present` 前後的 `QueryPerformanceCounter` | 何時送出、`Present` 阻塞多久 | 反壓：呈現佇列滿了或 vsync 節奏會讓 `Present` 等待 | 送出之後的任何事 | vsync 開時阻塞約一個 32 Hz 週期，關時約 0.1 ms |
| `GetLastPresentCount` | DXGI 給你最近一次 present 的編號 | 把你的時間戳記和統計資料接起來 | 單獨看沒有意義 | 編號 1、2、3、… |
| `GetFrameStatistics` | `PresentCount`：DXGI 認定已顯示的最新一次 present；`PresentRefreshCount`：它在哪一次 vblank 顯示；`SyncRefreshCount` 與 `SyncQPCTime`：某次 vblank 及其時間 | 哪些 present 到達螢幕、在第幾次 vblank；依照 Learn 的 [glitch 步驟](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model#avoiding-detecting-and-recovering-from-glitches)偵測 glitch | 影格是由 DWM 合成還是由硬體翻頁；Learn 警告這些統計在許多多螢幕情境下並不可靠 | `PresentCount` 有前進；三個 vblank 欄位一直是 0 |
| `IDXGISwapChainMedia::GetFrameStatisticsMedia` | `CompositionMode`：`COMPOSED`、`OVERLAY`、`NONE` | DXGI 本身對合成方式的提示 | Learn 把它寫給決定是否繼續使用解碼 swapchain 的媒體程式，而不是一般的 swapchain，也沒有說 `OVERLAY` 是否涵蓋 Independent Flip | 每次 present 都是 `COMPOSED`（第一次 present 之前有一次 `NONE`） |
| `DwmGetCompositionTimingInfo(NULL, …)` | DWM 的更新率，以及它對更新與已合成影格的計數器 | DWM 自己跑得多快，針對整個工作階段 | 關於單一視窗的任何事：從 Windows 8.1 起視窗代碼必須是 `NULL` | 32 Hz；DXGI 回報每秒 32 個新影格時，計數器每秒只前進 8–10 |
| `IDXGIOutput::WaitForVBlank` | 在輸出的下一次 vblank 返回 | DXGI 看到的輸出節奏 | 哪個平面顯示了什麼 | 平均 31.0–32.0 Hz，中位數 31.2–31.4 ms |
| `SupportsOverlays`、`CheckOverlaySupport`、`CheckHardwareCompositionSupport`、撕裂支援 | 輸出與驅動「能」做什麼 | MPO 或硬體合成究竟可不可能 | 實際上有沒有發生 | 沒有 overlay、沒有硬體合成旗標、支援撕裂 |
| PresentMon（ETW） | 每次 present 的 `PresentMode`、`AllowsTearing` 與顯示時間 | 每次 present 走哪一條路、何時到達螢幕 | 光子，那需要攝影機或延遲測試器 | 未執行 |

所以程式能讓你看到節奏、排隊，以及哪些 present 被顯示了。只有追蹤能說出它們是「怎麼」被顯示的。

## 練習

執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. API 怎麼描述這台機器（已在此執行）

```powershell
.\flipstats.exe info
```

它印出 Windows 組建、是否為遠端桌面工作階段、每個 DXGI 轉接器（描述、LUID、廠商與裝置 ID、以 `CheckInterfaceSupport` 取得的使用者模式驅動版本）及其輸出（桌面矩形、`EnumDisplaySettings` 的更新率、overlay 與硬體合成支援）、`QueryDisplayConfig` 回報的作用中顯示路徑（以分數表示的精確更新率、連接類型）、DWM 的時序，以及程式什麼都沒顯示時 DWM 的計數器在 2 s 內前進多少。

**預測**你的機器：DXGI 列出幾個轉接器（軟體轉接器也算）、哪一個擁有你的螢幕，以及以分數表示的更新率。

在此處的遠端桌面工作階段中觀察到：

- 三個 DXGI 轉接器：兩個 LUID 不同的「Intel(R) UHD Graphics 750」，以及軟體的「Microsoft Basic Render Driver」。唯一的輸出 `\\.\DISPLAY353`（1440 × 900、32 Hz）掛在第一個 Intel 轉接器下。`QueryDisplayConfig` 卻把這個顯示器的路徑放在*第二個* LUID 上，而 DXGI 列出那個轉接器時沒有任何輸出；連接類型回報為「other」，也沒有螢幕名稱。`Win32_VideoController` 列出一個 1440 × 900、32 Hz 的「Microsoft Remote Display Adapter」，以及 1920 × 1080、60 Hz 的 Intel GPU：後者是主控台的顯示器，這個工作階段看不到。遠端顯示轉接器是怎麼建構的，沒有調查。
- 沒有 overlay（`SupportsOverlays` 為 FALSE，`CheckOverlaySupport` 旗標為 0）、沒有硬體合成（`CheckHardwareCompositionSupport` 旗標為 0）、支援撕裂。
- DWM：更新率 32 Hz（`qpcRefreshPeriod` 31.250 ms）；畫面上沒有我們的東西時，它的計數器在 2 s 內只前進了一。這符合「DWM 只在有東西改變時才合成，而不是每次 vblank 都合成」。
- 每個工作階段有一個 `dwm.exe`（`Get-Process dwm` 在工作階段 1 與 2 各顯示一個）。

### B. 視窗模式的 flip-model swapchain，vsync 開與關（已在此執行）

```powershell
.\flipstats.exe windowed 4              # Present(1, 0)
.\flipstats.exe windowed 4 --vsync 0    # Present(0, DXGI_PRESENT_ALLOW_TEARING)
```

**預測：** vsync 開時，`Present` 阻塞多久？一次 `Present` 呼叫之後，多久才第一次出現在 `GetFrameStatistics` 中？vsync 關時，每秒有多少次 present，其中有多少曾被顯示？

觀察結果，每種各兩次（每次執行：320 × 240、不是前景視窗、上面沒有其他視窗）：

| 設定 | 每秒 present 次數 | `Present` 中位數 | 從未被回報為最新顯示的 present | 從 `Present` 到第一次回報它的讀取，中位數（p99） | `CompositionMode` | DWM `cFrame` 每秒 |
| --- | --- | --- | --- | --- | --- | --- |
| 視窗，vsync 開 | 32.6／32.6 | 30.93／30.96 ms | 131 中 0／131 中 0 | 187.2（218.8）／187.2（218.4）ms | `COMPOSED` 131／131 | 8.1／8.0 |
| 視窗，vsync 關 | 5,864／5,686 | 0.110／0.117 ms | 23,458 中 23,329／22,744 中 22,615 | 34.7（40.2）／35.1（40.1）ms | `COMPOSED` 129／129 | 9.9／9.9 |

這些執行顯示了什麼、沒有顯示什麼：

- **輸出決定程式的節奏。** 使用 `Present(1, 0)` 時，`Present` 阻塞了大約一個 32 Hz 輸出週期（中位數 30.9 ms；`WaitForVBlank` 的中位數為 31.2–31.4 ms）。程式只能以影格離開佇列的速度繪製。
- **這裡的佇列很長。** 一次 present 在呼叫後約 187 ms 才第一次出現在 `GetFrameStatistics` 中：六個 31.25 ms 的週期。統計每次 `Present` 只讀一次，所以每個數值最多晚一個週期。練習 D 拿掉了這六個週期中的兩個；其餘四個花在哪裡，從這些 API 看不出來。
- **vsync 關時，幾乎每個影格都被丟掉了。** `Present` 約 0.1 ms 就返回，每秒約 5,800 次，但每秒只有約 32 次 present 曾是被顯示的最新一個：99.4 % 的影格白畫了。這正是 Learn 對 flip model 同步間隔 0 的規則（[Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)）：取消前一個影格剩下的時間，如果已有更新的影格排隊，就丟棄這一個。延遲降到大約一個週期（35 ms），因為沒有東西在佇列中等待。
- **允許撕裂，但不可能撕裂。** `DXGI_PRESENT_ALLOW_TEARING` 被接受了，但 DWM 合成了每一個影格（`COMPOSED`），而合成出來的影格是完整的。撕裂需要緩衝區直接上平面（練習 E）。
- **每次 vsync 關的執行中，每秒 present 次數都在下降**，例如第 1 次從每秒 8,717 降到 3,679。原因沒有調查（時脈或溫度限制、遠端顯示路徑）。
- **DWM 的計數器和 DXGI 的不一致。** DXGI 回報每秒 32 個新影格；DWM 的 `cFrame` 與 `cRefresh` 在 vsync 開時每秒前進 8，關時 10。兩者各自在數什麼，沒有查明。這個問題需要的是追蹤，而不是更多 API 呼叫。
- **沒有顯示：** PresentMon 會不會把這些 present 稱為 `Composed: Flip`、到螢幕發光的延遲，以及任何關於 RTX 5070 Ti 的事。

### C. 上面有另一個視窗（已在此執行）

```powershell
.\flipstats.exe overlap 4
.\flipstats.exe overlap 4 --vsync 0
```

程式建立自己的第二個不透明視窗（160 × 120），蓋住工作區的右下部分。記錄中的 `windows above ours that intersect its client area: 1 of this program, 0 others` 證實了這一點，而且只計數（不讀取其他視窗的標題或類別）。

**預測：** 在你的主控台上，這個視窗會得到哪一種 `PresentMode`？和練習 B、練習 E 比起來呢？

觀察結果，每種各兩次：

| 設定 | 每秒 present 次數 | `Present` 中位數 | 從未被回報為最新顯示的 present | 從 `Present` 到第一次回報它的讀取，中位數（p99） | `CompositionMode` | DWM `cFrame` 每秒 |
| --- | --- | --- | --- | --- | --- | --- |
| 被遮住，vsync 開 | 32.6／32.8 | 30.96／30.97 ms | 131 中 0／132 中 0 | 187.2（218.7）／187.3（188.3）ms | `COMPOSED` 131／132 | 8.1／8.1 |
| 被遮住，vsync 關 | 5,973／5,891 | 0.109／0.106 ms | 23,894 中 23,765／23,565 中 23,436 | 34.7（39.8）／35.0（47.1）ms | `COMPOSED` 129／129 | 9.9／9.9 |

在這裡，遮住與否沒有改變任何東西；在一個每個影格本來就要合成的工作階段中，這是預期的結果。只有在視窗原本有機會離開合成的地方，也就是本機顯示器上的全螢幕視窗，這個比較才有意義。部落格文章描述了 DWM 處理「直接翻頁的 swapchain 上面有內容」的三種方式：退回合成、把上面的內容「反向合成」進程式的緩衝區，或使用 MPO。在上面有東西與沒有東西的情況下各跑一次練習 E，找出你的機器用的是哪一種。

### D. 呈現佇列：最大影格延遲 1（已在此執行）

```powershell
.\flipstats.exe windowed 4 --latency 1     # IDXGIDevice1::SetMaximumFrameLatency(1)
```

裝置預設回報的最大影格延遲是 3。改成 1 時：

| 設定 | 每秒 present 次數 | `Present` 中位數 | 從 `Present` 到第一次回報它的讀取，中位數（p99） |
| --- | --- | --- | --- |
| 視窗，vsync 開，延遲 3（練習 B） | 32.6／32.6 | 30.93／30.96 ms | 187.2（218.8）／187.2（218.4）ms |
| 視窗，vsync 開，延遲 1 | 32.4／32.3 | 31.17／31.18 ms | 124.9（126.2）／125.0（126.3）ms |

中位數少了 62.3 ms，也就是兩個 31.25 ms 的週期。這符合「預設延遲 3 允許在佇列中等待的兩個影格」。`Present` 仍阻塞一個週期，所以影格率沒有改變。第二次執行中，130 次 present 有 3 次從未被看到是最新的一個。由於每次 `Present` 只讀一次統計，若兩次 present 在同一個讀取間隔內被顯示，看起來就會是這樣，所以這不能證明它們被丟掉了。**解釋**為何降低延遲沒有降低影格率，以及可等待的 swapchain（`DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT`，此處沒有使用）會改變什麼。

### E. 無邊框全螢幕（此處未執行）

```powershell
.\flipstats.exe borderless 10
.\flipstats.exe borderless 10 --vsync 0
```

這會蓋住你的主螢幕 10 s 並取得前景；按 Esc 可提早結束。本草稿沒有執行它，因為它會蓋住正在使用這台機器的人的螢幕。

在你的主控台上、那個螢幕沒有開啟其他東西時，要注意：

- 部落格文章中 DirectFlip 的條件是：swapchain 的緩衝區符合螢幕的尺寸，且視窗的工作區蓋住整個螢幕。檢查 `window:` 那一行：工作區大小必須等於螢幕的模式，並且是在螢幕實際的 DPI 下。
- 在實體顯示器上，`GetFrameStatistics` 應該會填入 `PresentRefreshCount`、`SyncRefreshCount` 與 `SyncQPCTime`。這時程式會印出由它們推得的 vblank 週期、影格在螢幕上停留超過同步間隔的步數，以及從 `Present` 到 DXGI 回報該影格顯示的那次 vblank 的時間。這段程式碼從未執行過（見狀態區塊）：在相信它之前，先拿它的數字和 `WaitForVBlank` 以及顯示路徑的更新率對照。
- `CompositionMode`：記錄它是否從 `COMPOSED` 改變，並和同一次執行中 PresentMon 的 `PresentMode` 比較（練習 F）。對一般 swapchain 而言 `OVERLAY` 代表什麼，Learn 沒有說，本實驗也不知道。
- 使用 `--vsync 0` 且處於 Independent Flip 時，白色長條應該會撕裂，而「從未被回報為最新顯示的 present」仍會是絕大多數。

### F. 用 PresentMon 擷取（此處未執行）

本草稿沒有下載 PresentMon，下面的命令也都沒有執行。先在提升權限的 PowerShell 中開始擷取，再在一般的 PowerShell 中、於 3 s 的延遲內啟動程式：

```powershell
# 提升權限：等 3 s，記錄 8 s，然後結束
.\PresentMon-2.6.0-x64.exe --process_name flipstats.exe --delay 3 --timed 8 --terminate_after_timed --output_file windowed-vsync1.csv
# 一般的命令提示字元，立刻執行：
.\flipstats.exe windowed 12 > windowed-vsync1-flipstats.txt
```

對六種執行重複這些步驟：`windowed`、`overlap` 與 `borderless`，各搭配 `--vsync 1` 與 `--vsync 0`，每個 CSV 取不同的名字。把同一次執行的程式記錄放在對應的 CSV 旁邊。所用選項依 README 的說明：`--process_name` 只記錄該執行檔名稱的行程，`--delay` 在開始記錄前等待，`--timed` 在指定秒數後停止記錄，`--terminate_after_timed` 讓 PresentMon 在計時擷取結束後離開。`--v1_metrics` 會改為輸出 PresentMon 1.x 的欄位，而不是預設的欄位組；請看 CSV 的標題列確認你拿到的欄位。

讀 `PresentMode` 欄位。README 定義了這些值：

| `PresentMode` | 意義，依 README |
| --- | --- |
| `Composed: Flip` | 視窗模式、flip-model swapchain，表面與 DWM 共用以進行合成 |
| `Hardware: Independent Flip` | 程式不擁有螢幕，但它的表面每個影格仍被直接換上螢幕 |
| `Hardware Composed: Independent Flip` | 被配給了硬體 overlay 平面的 flip-model swapchain |
| `Hardware: Legacy Flip`、`Hardware: Legacy Copy to front buffer` | 程式擁有螢幕（舊式的獨占全螢幕） |
| `Composed: Copy with GPU GDI`、`Composed: Copy with CPU GDI` | 透過與 GDI 共用的表面或 DirectX 重新導向表面的 bitblt 模型呈現 |

只用 PowerShell 統計各模式並找出切換點：

```powershell
Import-Csv windowed-vsync1.csv | Group-Object PresentMode | Select-Object Count, Name
$i = 0; $prev = ''
foreach ($r in Import-Csv borderless-vsync0.csv) {
    if ($r.PresentMode -ne $prev) { "{0,5}  {1}  AllowsTearing={2}" -f $i, $r.PresentMode, $r.AllowsTearing; $prev = $r.PresentMode }
    $i++
}
```

也看看 `AllowsTearing`、`SyncInterval`、`PresentFlags`，以及顯示時間的欄位（`DisplayedTime`、`DisplayLatency`，或在 `--v1_metrics` 下的 `MsBetweenDisplayChange` 與 `MsUntilDisplayed`）。把 `PresentMode` 和同一次執行中程式的 `CompositionMode` 比較，把顯示時間和程式的「從 `Present` 到回報」時間比較。預期無邊框執行的最初幾個影格會先被合成，然後才切換；記錄切換發生在第幾個影格之後。

### G. 讓它切換，以及按下 Win+Ctrl+Shift+B（此處未執行）

在一次無邊框擷取（`--vsync 0`、較長的 `--timed`）期間，讓某個東西出現在程式上方：按音量鍵讓音量面板出現、打開開始功能表，或按 Alt+Tab。在 CSV 中找出每次切換，並對應到你做的動作。

接著是 [`display-wake-research`](../../../display-wake-research/README.zh-TW.md) 的問題：Win+Ctrl+Shift+B 的圖形重置重新啟動了什麼？該資料夾引用的 Microsoft 支援頁面，把它描述為要求 Windows 重置圖形驅動程式。在主控台上執行 `.\flipstats.exe windowed 20`，在執行期間按下 Win+Ctrl+Shift+B，並記錄：

1. `Present` 有沒有回傳錯誤？程式會停下來並印出錯誤；若是 `DXGI_ERROR_DEVICE_REMOVED` 或 `DXGI_ERROR_DEVICE_RESET`，還會印出 `GetDeviceRemovedReason`。
2. 如果執行繼續：`GetFrameStatistics` 有沒有再次回傳 `DXGI_ERROR_FRAME_STATISTICS_DISJOINT`？DWM 的計數器有沒有重新開始（比較 `before:` 與 `after:`）？
3. 視窗有沒有繼續更新？螢幕有沒有閃一下？

和 Learn 描述的 TDR 比較：`Dxgkrnl` 中的 GPU 排程器發現某個工作在逾時（預設兩秒）內無法完成或被搶占，miniport 的 `DxgkDdiResetFromTimeout` 重置 GPU，視訊記憶體管理員清除所有配置，應用程式必須釋放並重新建立它們的 Direct3D 裝置。Learn 也說，若一分鐘內發生五次以上的停止回應與復原，作業系統會在第六次時觸發 bug check。**不要在這台或任何你正在工作的機器上製造 TDR。** 階段 17 會刻意送出一個永遠不結束的 GPU 工作；TDR 的登錄機碼（`TdrDelay` 等）是給驅動程式測試用的，本課程不會去動它們。

### H. 畫出三條呈現路徑

並排畫出 X11、Wayland 與 Windows，各一個影格，從用戶端繪製到掃描輸出。每一個都標出：誰擁有顯示硬體（X 伺服器、透過 KMS 的合成器、`Dxgkrnl` 與 DWM）、合成發生在哪裡、緩衝區在哪裡被複製或共用，以及用戶端在哪裡可以不經合成直接到達螢幕（未重新導向的全螢幕 X 用戶端、Wayland 合成器對用戶端緩衝區的直接掃描輸出、Independent Flip）。X11 與 Wayland 的部分用課綱的[階段 10](../../SYLLABUS.zh-TW.md)，Windows 的部分用你的 PresentMon 結果。

## 需要時再學

- **WDDM 的分層。** Direct3D runtime 與廠商的使用者模式驅動（UMD）在你的行程內執行。它們透過 `gdi32` 的 thunk 進入 `Dxgkrnl`，其中包含 GPU 排程器（VidSch）與視訊記憶體管理員（VidMm，WDDM 2.0 以後位於 `dxgmms2.sys`）。廠商的核心模式 miniport 驅動（KMD）負責驅動硬體（[WDDM 架構](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)）。DWM 在使用者模式執行，每個工作階段一個行程（練習 A）。
- **Flip model。** 在 bitblt 模型中，runtime 每次 present 都把你的後緩衝區複製到一個重新導向表面，DWM 再從這份複本合成。在 flip model 中，所有後緩衝區都與 DWM 共用，DWM 直接從它們合成（[flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)）。同一頁要求 2–16 個緩衝區與三種格式之一，並說明不支援 MSAA。
- **Direct Flip、Independent Flip、MPO。** 當 flip-model swapchain 的緩衝區填滿螢幕時，DWM 可以掃描輸出它們，而不是自己的緩衝區（DirectFlip），也可能由掃描輸出硬體縮放，或放在 overlay 平面上（MPO）。swapchain 一旦被 direct flip，DWM 可以停止合成，直到有其他東西改變，程式的影格就「獨立地」送到螢幕（Independent Flip）（[部落格](https://devblogs.microsoft.com/directx/dxgi-flip-model/)）。
- **多平面 overlay。** 從 Windows 8.1 的 WDDM 1.3 開始提供，讓顯示硬體把多個平面（影片、程式的視窗、桌面）組合成一張影像，而不是用軟體混合（[MPO](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support)）。影格有沒有得到 overlay 平面，就是 `Hardware Composed: Independent Flip` 所回報的。
- **同步間隔與撕裂。** 對 flip model 而言，同步間隔 0 會取消前一個影格剩下的時間，並丟棄被更新影格取代的影格；1–4 則讓影格至少停留那麼多次 vblank。可變更新率顯示器要求允許撕裂（[Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)）。階段 13 會以 KMS 屬性 `VRR_ENABLED` 遇到 VRR。
- **呈現佇列。** `IDXGIDevice1::SetMaximumFrameLatency` 限制 CPU 最多可以預先排入幾個影格（此處預設為 3）；佇列滿時 `Present` 會阻塞（練習 D）。
- **TDR。** 由 GPU 排程器偵測，預設逾時兩秒，透過 miniport 重置，應用程式必須重新建立裝置（[TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery)）。練習 G 把它和 Win+Ctrl+Shift+B 比較。
- **為何 DWM 無法取代。** 從 Windows 8 起，合成無法再以程式關閉（[DwmEnableComposition](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenablecomposition)）。自訂合成器唯一有文件的途徑 `Windows.Devices.Display.Core`，只涵蓋 DWM 忽略的顯示器：頭戴式與特殊用途的顯示器，它們會離開桌面。間接顯示驅動是加入螢幕、並從桌面合成器接收影格，而不是取代它。來源見課綱。Windows 程式可以選擇自己怎麼呈現，但不能選擇由自己來合成桌面。

## Linux／NT 對照

| Windows | Linux（階段 13 以後） |
| --- | --- |
| DWM 合成桌面，並決定 swapchain 何時可以繞過它 | Wayland 合成器負責合成，也由它決定直接掃描輸出 |
| Independent Flip：不經合成直接掃描輸出的全螢幕 flip-model 緩衝區 | 直接掃描輸出：合成器把用戶端的 dma-buf 放上平面（總結專題的里程碑 4） |
| MPO：`Hardware Composed: Independent Flip` | KMS overlay 平面，以只測試的 atomic commit 試探 |
| `GetFrameStatistics`：present 計數與 vblank 計數 | 翻頁事件：序號與單調時鐘時間戳記 |
| `IDXGIOutput::WaitForVBlank` | `drmWaitVBlank` |
| `DXGI_PRESENT_ALLOW_TEARING` 與 VRR | `DRM_MODE_PAGE_FLIP_ASYNC`、`tearing-control-v1`、`VRR_ENABLED` |
| TDR 重置 GPU 並移除裝置 | GPU 驅動的重置；合成器必須撐過去（階段 17） |

推動本課程的差異是：在 Linux 上，你可以自己寫合成器，而階段 18 就是在做這件事。

## 檢核

在 `NOTES.md` 中回答。

1. 根據你的 PresentMon 紀錄：哪些影格由 DWM 合成、哪些未經合成就到達螢幕，以及每次切換的原因，附上 CSV 中的影格編號與造成切換的動作。
2. 對[證據表](#證據以及每一項能證明什麼)中的每個來源，用一句話說明你的執行顯示了什麼，再用一句話說明它不能顯示什麼。每次執行中 `CompositionMode` 和 `PresentMode` 是否一致？
3. 列出 Windows 開發者在這條路徑上能改與不能改的部分。能改：swap effect、緩衝區數量、格式、同步間隔、撕裂旗標、影格延遲、視窗的樣式、大小與 DPI 感知。不能改：某個影格是否被 DWM 合成、是否配給 overlay 平面、DWM 本身、vblank。再加上你的執行教會你的部分。
4. 你在練習 H 畫的圖，每個系統附一句話，說明由誰決定一個影格可以跳過合成。
5. Win+Ctrl+Shift+B 重新啟動了什麼？根據練習 G 回答。把 Microsoft 有文件記載的、你觀察到的與你推論的分開，並和 TDR 以及階段 13 的 KMS 熱插拔事件比較。
6. 用三個有文件依據的事實，說明為何在 Windows 上不可能有你自己的桌面合成器，以及這對本課程的總結專題在哪裡執行代表什麼。

## 常見失敗情況

- **`undefined reference to IID_IDXGIFactory2`**（或其他 `IID_`）。在恰好一個檔案中、於 DXGI 標頭檔之前引入 `<initguid.h>`，就像 `flipstats.c` 那樣；或連結 `-ldxguid`。
- **`SM_REMOTESESSION = 1`、32 Hz、每個影格都是 `COMPOSED`、vblank 欄位為 0。** 你在遠端桌面中，和記錄的執行一樣。請在主控台登入。
- **無邊框模式始終沒有離開合成。** 上面有東西（看 `windows above ours … others`）、工作區大小不等於螢幕的模式、行程沒有 DPI 感知而被 Windows 縮放（程式已設定每個螢幕各自的 DPI 感知；檢查 `DPI` 的值），或驅動在這個輸出上不提供。與其猜測，不如查看 `CheckHardwareCompositionSupport` 與 PresentMon。
- **使用 `--vsync 0` 時 `Present` 回傳 `DXGI_ERROR_INVALID_CALL`。** `DXGI_PRESENT_ALLOW_TEARING` 需要以 `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` 建立的 swapchain。程式只在系統回報支援撕裂時才同時設定兩者。
- **Alt+Enter 切換成獨占全螢幕。** 這個程式不會：它呼叫了 `MakeWindowAssociation(..., DXGI_MWA_NO_ALT_ENTER)`。
- **PresentMon 什麼都沒記錄到。** 沒有提升權限、行程名稱錯誤，或程式在延遲結束前就已經結束。
- **數字看起來像是某一次執行的偶然。** 每種設定至少跑兩次，並像這裡的表格一樣回報範圍。

## 後續會用到的地方

- 階段 13 以翻頁事件取代 `GetFrameStatistics`，以 VKMS 平面上的只測試 commit 取代 `CheckOverlaySupport`。硬體驗證會在真正的顯示器上重問本實驗的時序問題。
- 階段 14 在兩個行程之間共用 D3D12 資源與 fence：一個跨越行程邊界交出去的緩衝區，就像你的 swapchain 緩衝區交給 DWM 那樣。
- 階段 17 送出永遠不結束的 GPU 工作並觀察 TDR。它的 `WDA_EXCLUDEFROMCAPTURE` 只在 DWM 合成時有效，也就是本實驗量測的那種合成。
- 階段 18 的里程碑 4 是 Independent Flip 的 Linux 版本：全螢幕用戶端緩衝區的直接掃描輸出。

## 下一步

[建置計畫](../../SYLLABUS.zh-TW.md)的階段 11 在 WSLg 中寫一個不用工具套件的 Wayland 用戶端。
