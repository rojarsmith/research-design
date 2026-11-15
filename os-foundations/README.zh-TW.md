[English](README.md) | **繁體中文** | [實作建置計畫](SYLLABUS.zh-TW.md)

# 作業系統基礎：從核心到合成器

> **草稿，待審閱（2026-11-15）。** 本文是課程規劃，不是完成的課程，目前沒有任何實驗。階段順序、時數估算與工具選擇都屬提案。技術敘述已在當天對照[來源](SYLLABUS.zh-TW.md#來源)列出的官方頁面查核，實作前請重新確認版本。

這是一門「先動手做」的作業系統課程，終點只有一個：能自己打造桌面圖形系統，也就是能執行真實應用程式的視窗系統與合成器（compositor）。第一部分建立作業系統基礎：修改一個小型教學核心，並用 Linux 與 Windows 的真實介面寫程式。第二部分把這些機制帶到顯示端，處理緩衝區如何在用戶端行程、核心與顯示硬體之間移動。課程最後在 Linux DRM/KMS 上完成一個最小的 Wayland 合成器。

## 與圖學課程的分工

桌面圖形這個主題依層次切開：

| 層次 | 要回答的問題 | 放在哪裡 |
| --- | --- | --- |
| 像素 | 視窗或最終影格的每個像素怎麼算出來：2D 繪圖、文字、混色、色彩 | [`computer-graphics-practice/`](../computer-graphics-practice/README.zh-TW.md) 規劃中的第二部分「2D、文字與合成渲染」主線。可在 Windows 上完成。 |
| 緩衝區與時間 | 緩衝區歸誰所有、如何送到另一個行程與螢幕、何時顯示 | 本課程第二部分。從階段 13 起需要在實體機器上執行 Linux。 |
| 機制 | 行程、記憶體、陷阱（trap）、行程間通訊、同步、排程、驅動程式 | 本課程第一部分。在 Windows 11 上透過 WSL 2 與 QEMU 完成。 |

總結專題把兩邊接起來：輸入是各個 surface，輸出是合成後的影格。本課程提供用戶端緩衝區、fence 與顯示時序，圖學課程提供把 surface 合成為影格的模組。兩者的[介面](SYLLABUS.zh-TW.md#與圖學課程的介面)也列在規劃中。

## 學習路線

| 階段 | 動手製作 | 核心概念 | 執行環境 |
| --- | --- | --- | --- |
| **第一部分：作業系統基礎** | | | |
| 00 工作環境 | 在 QEMU 開機 xv6，用 GDB 逐步追蹤 | 不經韌體的開機、特權模式 | WSL 2 |
| 01 系統呼叫 | 新增系統呼叫，追蹤一次呼叫跨越邊界 | 陷阱、trap frame、不可信的使用者指標 | WSL 2、Windows |
| 02 行程與執行緒 | fork/exec/pipe 管線、使用者層級執行緒 | 行程狀態、情境切換、呼叫慣例 | WSL 2、Windows |
| 03 虛擬記憶體 | 寫入時複製的 fork、共享與私有映射 | 分頁表、分頁錯誤、mmap、SIGBUS | WSL 2、Windows |
| 04 中斷與驅動程式 | 追蹤裝置中斷、寫一個小型 Linux 字元裝置模組 | 中斷情境、延後處理、裝置模型 | WSL 2、Linux 虛擬機器 |
| 05 同步 | 降低鎖競爭、以 futex 實作 mutex | 原子操作、自旋鎖與睡眠鎖、喚醒遺失 | WSL 2、Windows |
| 06 排程與延遲 | 新的 xv6 排程器、喚醒延遲分布圖 | 搶占、公平與延遲、即時排程類別 | WSL 2、Windows |
| 07 檔案系統 | 擴充 xv6 檔案系統、檔案映射 mmap | 日誌、inode、fd 與開啟檔案描述 | WSL 2 |
| 08 行程間通訊與事件迴圈 | 傳遞 fd 與封存的共享記憶體、單一 epoll 迴圈 | 權能（capability）、封存、就緒與完成模型 | WSL 2、Windows |
| 09 第一部分專題 | 自訂協定的無螢幕顯示伺服器 | 不可信用戶端、背壓、影格節奏 | WSL 2 |
| **第二部分：視窗與顯示系統** | | | |
| 10 視窗系統架構 | 用 PresentMon 量測 DWM 呈現路徑 | X11、Wayland、DWM/WDDM；為何 DWM 無法取代 | Windows |
| 11 Wayland 用戶端 | 不用工具套件的 Wayland 用戶端 | 物件、全域物件、雙重緩衝狀態、影格回呼 | WSL 2（WSLg） |
| 12 DRM/KMS | 用 atomic commit 點亮螢幕 | KMS 物件、屬性、DRM master、render node | Linux 虛擬機器，之後 Linux 實體機 |
| 13 平面與時序 | 翻頁時序、平面、VRR、熱插拔復原 | vblank、page flip、前廊、uevent、連結狀態 | Linux 實體機 |
| 14 緩衝區共享與同步 | GPU 緩衝區跨兩個行程送到掃描輸出 | dma-buf、modifier、dma-fence、sync_file、syncobj | Linux 實體機、Windows |
| 15 輸入 | evdev → libinput → xkbcommon，加上焦點分派 | 事件封包、裝置特性修正、鍵盤配置 | Linux 實體機 |
| 16 座位與工作階段 | 以一般權限執行並撐過 VT 切換 | logind/libseat、DRM master 交接、權限撤銷 | Linux 實體機 |
| 17 隔離與安全 | 攻擊並防禦自己的伺服器 | 不可信緩衝區、擷取政策、GPU 重置 | Linux 實體機、Windows |
| 18 總結專題 | 能執行真實應用程式的最小 Wayland 合成器 | 以上全部，端到端 | Linux 實體機 |

各階段的里程碑、先備條件與檢核請見 [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md)。

## 時數與先備條件

估計約需 260 至 450 小時的專注時間：第一部分約 110 至 190 小時，第二部分約 150 至 260 小時，其中總結專題占 60 至 120 小時。每週投入 6 至 8 小時，大約需要八個月到一年半。這是給已會寫 C 的人的規劃估算，不是保證；進度以通過的檢核計算，不以週數計算。

起點條件：C 語言（指標、結構、位元操作、連結）、Linux shell、git 與 make，以及讀懂編譯器輸出所需的 RISC-V 組合語言，程度相當於 [`c-compiler-foundations` 學習路線](../c-compiler-foundations/research/04-integration-path.zh-TW.md)第 1 關。第二部分另需完成圖學課程到階段 05（GPU 管線），以及其規劃中「2D、文字與合成渲染」主線的合成模組。

## 實驗載具：為什麼選 xv6-riscv

[xv6-riscv](https://github.com/mit-pdos/xv6-riscv) 是 MIT 以 ANSI C 重新實作的 Unix 第 6 版，執行於現代多處理器 RISC-V，也是 MIT 6.1810 課程的核心。2025 年公開的實驗依序為 util、syscall、pgtbl、traps、cow、net、lock、fs、mmap（[課表](https://pdos.csail.mit.edu/6.1810/2025/schedule.html)）。2026-11-15 查核時，2026 秋季網站仍在建置中。選它的原因：

- 小到可以完整讀完，卻具備真正的行程、Sv39 分頁、陷阱、PLIC、多處理器上的鎖、排程器、具日誌的檔案系統、pipe 與 virtio 裝置。
- 在 QEMU 的 `virt` 機器上以 `-bios none` 開機，從重置到 shell 提示字元的每一道指令都在原始碼中。
- 銜接 [`c-compiler-foundations/`](../c-compiler-foundations/README.zh-TW.md) 的 RISC-V。該目錄以 RV32IM 為目標；xv6 是 64 位元（`-march=rv64gc`），並加上特權架構：CSR、特權模式、陷阱與分頁。
- MIT 的實驗與 xv6 課本提供經過驗證的練習順序；本課程再加上 Linux 與 NT 的對照，以及通往顯示端的路線。

評估過的其他方案：

| 方案 | 結論 |
| --- | --- |
| 擴充 `c-compiler-foundations/` 的 `rv32sim` | 不作為主要載具。它執行的是組合語言文字而非機器碼，也沒有 CSR、特權模式、陷阱或 MMU。要讓它開機跑核心本身就是另一個專案，因此列為選修支線。 |
| 從零寫 x86-64 玩具核心 | 在碰到核心概念前，要先花更多時間處理開機與舊式 PC 細節。適合在 xv6 之後延伸。 |
| 直接修改 Linux | 作為要修改的核心太大。本課程全程從使用者空間（WSL 2）使用 Linux，並在虛擬機器中寫一個核心模組。 |
| Windows NT | 沒有原始碼。透過官方 API 文件與追蹤工具（WPR/WPA、Process Explorer）對照。撰寫 NT 驅動程式屬選修，且只在虛擬機器中進行。 |

## 各項工作在哪裡執行

| 工作 | Windows 11 | WSL 2 | Linux 虛擬機器（QEMU） | Linux 實體機 |
| --- | --- | --- | --- | --- |
| xv6 實驗 | 不適用 | 可以。MIT 給 Windows 的做法是 WSL 2 搭配 Ubuntu 24.04；WSL 1 無法使用 | 不需要 | 可以 |
| Linux 系統呼叫、記憶體與行程間通訊實驗 | 不適用 | 可以。WSL 2 在輕量虛擬機器中執行真正的 Linux 核心 | 可以 | 可以 |
| 自己的 Linux 核心模組 | 不適用 | 不採用 | 可以 | 可以 |
| NT 對照（Win32、ETW 追蹤、Process Explorer） | 可以 | 不適用 | 不適用 | 不適用 |
| Wayland 用戶端程式 | 不適用 | 可以，透過 WSLg | 可以 | 可以 |
| DRM/KMS API 基礎 | 不適用 | 不行 | 可以，用 virtio-gpu 或 VKMS | 可以 |
| 真實平面、vblank 時序、VRR、熱插拔、GPU 緩衝區共享、輸入裝置、座位 | 不適用 | 不行 | 不行，沒有真實時序與硬體平面 | 可以 |
| 總結專題合成器 | 不適用 | 不行 | 只能對 VKMS 開發 | 可以 |

**第二部分從階段 13 起，需要能直接控制顯示器的 Linux。** 也就是在 RTX 5070 Ti 桌機上安裝雙系統開機，或另備一台電腦。WSL 2 無法替代：它的 GPU 存取是供繪圖用的半虛擬化（para-virtualization），WSLg 則透過 Weston 合成器，以 RDP 把 Linux 視窗轉送到 Windows。WSL 裡沒有任何東西能透過 KMS 驅動你的螢幕。階段 11 與 12 可以先在 Windows 上開始（先用 WSLg，再用 QEMU 虛擬機器），階段 10 本身就是研究 Windows。

虛擬機器方面：QEMU 提供 Windows 版本（Stefan Weil 的安裝檔或 MSYS2 套件），並可使用 Windows Hypervisor Platform（WHPX）加速器。Windows 11 也預設為 WSL 2 開啟巢狀虛擬化（`.wslconfig` 的 `nestedVirtualization`），這是取得 Linux 虛擬機器的另一條路。

## Linux 實體機

RTX 5070 Ti 可以在 Linux 上完成本課程，但有限制。以下取自 NVIDIA 驅動程式 README（620.70.06，2026-11-15 查核）：

- Blackwell GPU 只由 NVIDIA 的開放核心模組支援。
- nvidia-drm 的 KMS 預設開啟。
- 驅動程式附有 GBM 後端，供以 GBM 為基礎的 Wayland 合成器使用。
- 顯式同步使用 DRM syncobj timeline 點，需要 6.8 以上的核心。
- KMS 驅動只註冊主平面與游標平面，沒有 overlay 平面。
- 已知問題還包括 VRR 行為，以及未設定 `fbdev=1` 時與 simpledrm 互相干擾造成的黑畫面。進行階段 13 前請先讀過。
- Mesa 的 NVK Vulkan 驅動列出支援消費級 Blackwell（RTX 5000 系列），可作為日後比較的上游方案。

因此階段 13 的 overlay 平面實驗需要 VKMS（練習 API 用）或另一張 GPU。Intel 或 AMD GPU 都可以，內顯就夠，但要先用 `drm_info` 確認它有提供 overlay 平面。另備一台這類小電腦，也能讓主力桌機免於雙系統開機。這是下方的待決問題 1。

螢幕：ASUS VG289Q 規格列出 Adaptive-Sync，垂直頻率 40–60 Hz，可用於 VRR 實驗。Acer ET322QK 是否支援 VRR 尚未查證。

## 每次實驗的流程

沿用圖學課程的流程，再加上 `mcu-foundations` 的一步：**預測 → 實作 → 觀察 → 解釋 → 刻意弄壞。** 建立自己的 `NOTES.md`，每個實驗都記下所有工具的版本、預期結果、實測結果與弄壞的案例。Linux 或 NT 的對照必須引用文件頁面或附上追蹤紀錄，才算完成。

## 相關目錄

- [`computer-graphics-practice/`](../computer-graphics-practice/README.zh-TW.md) 是第二部分的先修課。它的階段 00–05 涵蓋影格緩衝區、像素格式與 GPU 管線，規劃中的第二部分「2D、文字與合成渲染」主線則提供階段 09 與 18 呼叫的合成模組。該第二部分仍在另一個分支撰寫中，階段名稱可能變動。
- [`c-compiler-foundations/`](../c-compiler-foundations/README.zh-TW.md) 整理了契約堆疊（語言、ABI、ISA），階段 01–02 把它往下延伸：trap frame 與 `swtch` 要搭配 RISC-V 呼叫慣例才看得懂。其中的 `rv32sim` 是選修特權模式支線的起點。
- [`mcu-foundations/`](../mcu-foundations/README.md) 談單一位址空間中的 reset 到 `main`、中斷、DMA 緩衝區所有權與 ISR 規則。本課程在有記憶體保護、多個行程的環境下問同樣的問題。
- [`mcu-embedded-hmi-research/`](../mcu-embedded-hmi-research/README.zh-TW.md) 研究由單一應用程式獨占顯示器、沒有合成器的嵌入式圖形（LVGL、TouchGFX、智慧顯示器）。第二部分處理的是多用戶端的情況。
- [`display-wake-research/`](../display-wake-research/README.zh-TW.md) 研究同一台 RTX 5070 Ti 桌機上的 Windows 顯示路徑，包括休眠中的 DisplayPort 螢幕被當成已中斷連線，以及 Win+Ctrl+Shift+B 重置圖形驅動。階段 10 與 13 會從作業系統端遇到同樣的事件：WDDM 復原與 KMS 熱插拔。

## 檔案

- [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md) / [`SYLLABUS.md`](SYLLABUS.md)：逐階段建置計畫、與圖學課程的介面、選修支線與來源。
- 目前沒有實驗。規劃的結構是 `labs/NN-name/`，每階段一個資料夾，各附練習與檢核的 README，與圖學課程一致。

## 完成目標

你會完成一個自己寫的合成器。它能以一般使用者身分從文字主控台啟動，同時執行終端機、一個 GTK 或 Qt 應用程式與一個 Vulkan 示範程式，並撐過 VT 切換、螢幕熱插拔與用戶端當掉。你還要寫一份設計說明，附實測時間戳記，追蹤一個用戶端影格從 `wl_surface.commit` 到掃描輸出、再回到影格回呼的過程。

只完成第一部分的標準是：能附上追蹤紀錄，說明從系統呼叫、情境切換到分頁錯誤的完整過程，並分別以 xv6、Linux 與 NT 的術語解釋。

## 範圍與限制

- 不在範圍內：GPU 核心驅動、Mesa 驅動、UI 工具套件、Xwayland 內部、Windows 核心驅動（選修，限虛擬機器）、HDR 與色彩管理（選修延伸）。
- 在 Windows 上只能研究與量測 DWM，無法取代它。原因見建置計畫的階段 10。
- WSL 2 在虛擬機器內執行，在那裡量到的延遲只能相對比較，階段 13 會在實體機上重新量測。
- 來源於 2026-11-15 查核。查核期間 gitlab.freedesktop.org 與 git.sr.ht 拒絕自動化請求，因此 wlroots、tinywl 與 seatd 只附連結，未重新閱讀頁面。Wayland 協定的狀態取自 wayland.app 索引，該索引由相同的 XML 檔產生。
- 時數為估算。

## 待決問題

1. 階段 13–18 用哪台 Linux 機器：在 RTX 5070 Ti 桌機上雙系統開機，還是另備一台使用 Intel 或 AMD 顯示晶片的電腦？桌機的 CPU 有內顯嗎？
2. 發行版與核心：Ubuntu 24.04 與 MIT 工具頁一致。NVIDIA 顯式同步需要 6.8 以上核心，nvidia-drm 的 KMS 色彩管線需要 6.19 以上。WSL 2 與 Linux 實體機要用同一個發行版，還是可以不同？
3. 總結專題的路線：用階段 12–16 寫好的程式組成自己的後端（建議，因為那些階段已經寫出來了），還是後端用 wlroots，只自己寫場景與合成？
4. 語言：第一部分與面向核心的程式用 C。合成器用 C 還是 C++，視圖學課程第二部分的核心（C++/Vulkan）而定？
5. NT 的深度：只做對照（目前規劃），還是在 Windows 虛擬機器中加入 KMDF 驅動實驗？
6. 保留還是刪除 `rv32sim` 特權模式支線？
7. Acer ET322QK 是否支援 VRR？只影響階段 13。
