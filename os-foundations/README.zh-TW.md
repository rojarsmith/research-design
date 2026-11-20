[English](README.md) | **繁體中文** | [實作建置計畫](SYLLABUS.zh-TW.md)

# 作業系統基礎：從核心到合成器

> **草稿，待審閱（2026-11-15）。** 本文是課程規劃，不是完成的課程。目前階段 00–10 有實驗草稿，放在 [`labs/`](labs/)；階段 09 是第一部分的專題。撰寫時已建置並執行其中的 Windows 程式；xv6 與 Linux 的步驟已於 2026-11-16 在 podman 下的 Ubuntu 24.04 容器中執行，尚未在 WSL 2 本身執行，階段 04 的核心模組也尚未建置或載入。階段順序、時數估算與工具選擇都屬提案。技術敘述已在當天對照[來源](SYLLABUS.zh-TW.md#來源)列出的官方頁面查核，實作前請重新確認版本。

這是一門「先動手做」的作業系統課程，終點只有一個：能自己打造桌面圖形系統，也就是能執行真實應用程式的視窗系統與合成器（compositor）。第一部分建立作業系統基礎：修改一個小型教學核心，並用 Linux 與 Windows 的真實介面寫程式。第二部分把這些機制帶到顯示端，處理緩衝區如何在用戶端行程、核心與顯示硬體之間移動。課程最後在 Linux DRM/KMS 上完成一個最小的 Wayland 合成器。

## 與圖學課程的分工

桌面圖形這個主題依層次切開：

| 層次 | 要回答的問題 | 放在哪裡 |
| --- | --- | --- |
| 像素 | 視窗或最終影格的每個像素怎麼算出來：2D 繪圖、文字、混色、色彩 | [`computer-graphics-practice/`](../computer-graphics-practice/README.zh-TW.md) [第二部草稿](../computer-graphics-practice/SYLLABUS-PART2.zh-TW.md)中的「2D、文字與合成渲染路線」。可在 Windows 上完成。 |
| 緩衝區與時間 | 緩衝區歸誰所有、如何送到另一個行程與螢幕、何時顯示 | 本課程第二部分。在 Linux 虛擬機器中進行，最後一組硬體檢核需要 Linux 實體機。 |
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
| 12 DRM/KMS | 用 atomic commit 點亮螢幕 | KMS 物件、屬性、DRM master、render node | Linux 虛擬機器 |
| 13 平面與時序 | 翻頁事件、平面、熱插拔復原 | vblank、page flip、前廊、uevent、連結狀態 | Linux 虛擬機器 |
| 14 緩衝區共享與同步 | 緩衝區跨兩個行程送到掃描輸出 | dma-buf、modifier、dma-fence、sync_file、syncobj | Linux 虛擬機器、Windows |
| 15 輸入 | evdev → libinput → xkbcommon，加上焦點分派 | 事件封包、裝置特性修正、鍵盤配置 | Linux 虛擬機器 |
| 16 座位與工作階段 | 以一般權限執行並撐過 VT 切換 | logind/libseat、DRM master 交接、權限撤銷 | Linux 虛擬機器 |
| 17 隔離與安全 | 攻擊並防禦自己的伺服器 | 不可信緩衝區、擷取政策、GPU 重置 | Linux 虛擬機器、Windows |
| 18 總結專題 | 能執行真實應用程式的最小 Wayland 合成器 | 以上全部，端到端 | Linux 虛擬機器 |
| 硬體驗證 | 在真實螢幕上重做時序、平面、VRR、熱插拔與 GPU 檢核 | 虛擬機器看不到的部分 | Linux 實體機 |

各階段的里程碑、先備條件與檢核請見 [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md)。

## 時數與先備條件

估計約需 270 至 480 小時的專注時間：第一部分約 110 至 190 小時，第二部分約 160 至 290 小時，其中包含 60 至 120 小時的總結專題，以及 15 至 30 小時的硬體驗證。每週投入 6 至 8 小時，大約需要八個月到一年半。這是給已會寫 C 的人的規劃估算，不是保證；進度以通過的檢核計算，不以週數計算。

起點條件：C 語言（指標、結構、位元操作、連結）、Linux shell、git 與 make，以及讀懂編譯器輸出所需的 RISC-V 組合語言，程度相當於 [`c-compiler-foundations` 學習路線](../c-compiler-foundations/research/04-integration-path.zh-TW.md)第 1 關。第二部分另需完成圖學課程第一部到實驗 05（GPU 管線）。總結專題還需要圖學課程第二部「2D、文字與合成渲染路線」的 T4（合成）。T4 建立在 T1、T3 與共同核心之上，這部分本身就有數百小時，可以和本課程第一部分同時進行。

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
| KMS atomic commit、翻頁事件、overlay 平面（VKMS）、由主機顯示變更觸發的熱插拔（virtio-gpu） | 不適用 | 不行 | 可以 | 可以 |
| 緩衝區共享、顯式同步、輸入裝置、座位與 VT 切換 | 不適用 | 不行 | 可以，以軟體繪製 | 可以 |
| 能執行真實應用程式的總結專題合成器 | 不適用 | 不行 | 可以，以軟體繪製 | 可以 |
| 真實翻頁時序、VRR、真實 GPU 的平面、拔插纜線與連結狀態、GPU 加速合成、輸入到畫面的延遲 | 不適用 | 不行 | 不行 | 可以 |

**Linux 虛擬機器可以撐起整個第二部分，包括總結專題。只有最後的硬體驗證，才需要能直接控制真實顯示器的 Linux。** 硬體驗證仍需要在 RTX 5070 Ti 桌機上雙系統開機，或另備一台電腦，但可以留到最後再處理。

虛擬機器能提供的功能（2026-11-15 對照客體驅動原始碼查核）：

- **建議的虛擬機器顯示是 QEMU 的 virtio-gpu。** 它是 atomic KMS 驅動，具備 render node 與含 timeline 的 DRM syncobj（[virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c)）。每個輸出有一個主平面與一個游標平面（[virtgpu_display.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_display.c)）。主機端的顯示變更會以熱插拔事件傳給客體（[virtgpu_kms.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_kms.c)）。
- **VKMS 補上 virtio-gpu 缺少的部分。** 它可透過 configfs 提供 overlay 平面與 writeback connector，但目前還不能在執行中熱插拔（[VKMS](https://docs.kernel.org/gpu/vkms.html)）。
- **VMware Workstation 可作為備案。** 它自 2024 年 11 月起對所有使用者免費（[VMware](https://blogs.vmware.com/cloud-foundation/2024/11/11/vmware-fusion-and-workstation-are-now-free-for-all-users/)）。它的客體驅動 vmwgfx 是 atomic 驅動，具備 render node，有主平面與游標平面，但沒有 syncobj（[vmwgfx_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vmwgfx/vmwgfx_drv.c)）。
- **Hyper-V 的功能太少。** 它的 hyperv_drm 只有一個主平面，也沒有 render node，不夠本課程使用（[hyperv_drm_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/hyperv/hyperv_drm_drv.c)）。
- **客體預計以軟體繪製。** Windows 主機上的 QEMU 客體能否使用 3D 加速，官方文件沒有記載：QEMU 對 virgl 與 Venus 的需求說明只涵蓋 Linux 主機（[virtio-gpu](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html)）。

虛擬機器做不到的事：

- 真實的掃描輸出時序與 VRR；
- 真實 GPU 的平面；
- 實際拔插纜線與連結訓練；
- 在 RTX 5070 Ti 上做 GPU 加速合成；
- 輸入到畫面的延遲。

GPU 也無法直接交給 Linux 虛擬機器：Hyper-V 的 Discrete Device Assignment 文件只針對 Windows Server 與伺服器等級硬體，列出的客體也只有 Windows（[Learn](https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/plan/plan-for-deploying-devices-using-discrete-device-assignment)）。

WSL 2 不適合第二部分的顯示實驗：它的 GPU 存取是供繪圖用的半虛擬化（para-virtualization），WSLg 則透過 Weston 合成器，以 RDP 把 Linux 視窗轉送到 Windows。階段 10 本身就是研究 Windows，階段 11 在 WSLg 中進行。

QEMU 的執行方式：QEMU 提供 Windows 版本（Stefan Weil 的安裝檔或 MSYS2 套件），並可使用 Windows Hypervisor Platform（WHPX）加速器。Windows 11 也預設為 WSL 2 開啟巢狀虛擬化（`.wslconfig` 的 `nestedVirtualization`），這是取得 Linux 虛擬機器的另一條路。Learn 提醒，Hyper-V hypervisor 執行時，VMware Workstation 與 VirtualBox 可能不穩定（[Learn](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/quick-start/enable-hyper-v)）；若持續啟用 WSL 2，請留意這一點。

## 最後硬體驗證用的實體機

進行硬體驗證時，RTX 5070 Ti 可以在 Linux 上完成，但有限制。以下取自 NVIDIA 驅動程式 README（620.70.06，2026-11-15 查核）：

- Blackwell GPU 只由 NVIDIA 的開放核心模組支援。
- nvidia-drm 的 KMS 預設開啟。
- 驅動程式附有 GBM 後端，供以 GBM 為基礎的 Wayland 合成器使用。
- 顯式同步使用 DRM syncobj timeline 點，需要 6.8 以上的核心。
- KMS 驅動只註冊主平面與游標平面，沒有 overlay 平面。
- 已知問題還包括 VRR 行為，以及未設定 `fbdev=1` 時與 simpledrm 互相干擾造成的黑畫面。進行階段 13 前請先讀過。
- Mesa 的 NVK Vulkan 驅動列出支援消費級 Blackwell（RTX 5000 系列），可作為日後比較的上游方案。

因此在實體機上，overlay 平面需要另一張 GPU。Intel 或 AMD GPU 都可以，內顯就夠，但要先用 `drm_info` 確認它有提供 overlay 平面。走虛擬機器路線時，VKMS 可用來練習 overlay 平面的 API。另備一台這類小電腦，也能讓主力桌機免於雙系統開機。要用哪台機器是下方的待決問題 1，可以等總結專題在虛擬機器中跑起來後再決定。

螢幕：ASUS VG289Q 規格列出 Adaptive-Sync，垂直頻率 40–60 Hz，可用於 VRR 檢核。Acer ET322QK 是否支援 VRR 尚未查證。

## 每次實驗的流程

沿用圖學課程的流程，再加上 `mcu-foundations` 的一步：**預測 → 實作 → 觀察 → 解釋 → 刻意弄壞。** 建立自己的 `NOTES.md`，每個實驗都記下所有工具的版本、預期結果、實測結果與弄壞的案例。Linux 或 NT 的對照必須引用文件頁面或附上追蹤紀錄，才算完成。

## 相關目錄

- [`computer-graphics-practice/`](../computer-graphics-practice/README.zh-TW.md) 是第二部分的先修課。它的階段 00–05 涵蓋影格緩衝區、像素格式與 GPU 管線，它的[第二部草稿](../computer-graphics-practice/SYLLABUS-PART2.zh-TW.md)有一條「2D、文字與合成渲染路線」，其中 T3（文字）與 T4（合成）提供階段 09 與 18 呼叫的模組。該第二部仍是待審閱的草稿，階段名稱可能變動。
- [`c-compiler-foundations/`](../c-compiler-foundations/README.zh-TW.md) 整理了契約堆疊（語言、ABI、ISA），階段 01–02 把它往下延伸：trap frame 與 `swtch` 要搭配 RISC-V 呼叫慣例才看得懂。其中的 `rv32sim` 是選修特權模式支線的起點。
- [`mcu-foundations/`](../mcu-foundations/README.md) 談單一位址空間中的 reset 到 `main`、中斷、DMA 緩衝區所有權與 ISR 規則。本課程在有記憶體保護、多個行程的環境下問同樣的問題。
- [`mcu-embedded-hmi-research/`](../mcu-embedded-hmi-research/README.zh-TW.md) 研究由單一應用程式獨占顯示器、沒有合成器的嵌入式圖形（LVGL、TouchGFX、智慧顯示器）。第二部分處理的是多用戶端的情況。
- [`pc-troubleshooting/cases/2026-10-14-display-wake/`](../pc-troubleshooting/cases/2026-10-14-display-wake/README.zh-TW.md) 研究同一台 RTX 5070 Ti 桌機上的 Windows 顯示路徑，包括休眠中的 DisplayPort 螢幕被當成已中斷連線，以及 Win+Ctrl+Shift+B 重置圖形驅動。階段 10 與 13 會從作業系統端遇到同樣的事件：WDDM 復原與 KMS 熱插拔。

## 檔案

- [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md) / [`SYLLABUS.md`](SYLLABUS.md)：逐階段建置計畫、與圖學課程的介面、選修支線與來源。
- [`labs/`](labs/)：每階段一個資料夾 `labs/NN-name/`，各附一份 README，內含練習、預測、檢核與常見失敗情況，與圖學課程一致。每份 README 都註明哪些步驟有執行、哪些沒有。目前的草稿：
  - [`labs/00-workbench/`](labs/00-workbench/README.zh-TW.md)：在 QEMU 中開機 xv6，用 GDB 從重置向量逐步追到 `main()`。所有練習已於 2026-11-16 在 Ubuntu 24.04 容器中執行（Windows 11 上的 podman，不是 WSL 2 Ubuntu 本身），附輸出紀錄；WSL 2 的安裝步驟未執行。
  - [`labs/01-system-calls/`](labs/01-system-calls/README.zh-TW.md)：追蹤一次系統呼叫跨越邊界、新增一個系統呼叫，並把錯誤指標交給 xv6、Linux 與 NT。Windows 程式於 2026-11-15 建置並執行；xv6 與 Linux 部分於 2026-11-16 在 Ubuntu 24.04 容器中執行。兩者都附輸出紀錄。
  - [`labs/02-processes-threads/`](labs/02-processes-threads/README.zh-TW.md)：`fork`／`exec`／`pipe` 管線、追蹤 `fork()` 與 `swtch()`、使用者層級執行緒，以及 Linux 與 NT 上行程、執行緒與切換的成本。Windows 程式（包括手寫的 x64 情境切換）於 2026-11-15 建置並執行；xv6 與 Linux 部分於 2026-11-16 在 Ubuntu 24.04 容器中執行。兩者都附輸出紀錄。
  - [`labs/03-virtual-memory/`](labs/03-virtual-memory/README.zh-TW.md)：印出 xv6 分頁表、寫入時複製的 `fork` 及前後的分頁數、在 GDB 中確認手算的位址轉換、Linux 上的 `MAP_SHARED` 與 `MAP_PRIVATE` 及 `SIGBUS`，以及 NT 上的保留、提交與共享 section。xv6 與 Linux 部分已在 podman（WSL 2 虛擬機器）中的 Ubuntu 24.04 容器裡，針對 xv6-riscv commit `fa2ddcb7` 建置並執行，xv6 部分搭配參考解答（參考解答沒有放進專案），寫入時複製版本通過 `usertests`；Windows 程式已建置並執行。全部附輸出紀錄。
  - [`labs/04-interrupts-drivers/`](labs/04-interrupts-drivers/README.zh-TW.md)：在 xv6 中追蹤一次 UART 中斷從 PLIC 到主控台，以及強迫搶占的計時器中斷；一個帶週期性計時器的 Linux 字元裝置模組；在不需系統管理員權限下觀察 NT 的計時器解析度與中斷、DPC 速率。xv6 部分（搭配沒有放進專案的參考解答）、以 `timerfd` 實作的阻塞讀取版本與 Windows 程式已建置並執行，附輸出紀錄；核心模組沒有建置或載入。
  - [`labs/05-synchronization/`](labs/05-synchronization/README.zh-TW.md)：xv6 的鎖統計與每 CPU 分頁配置器、刻意弄壞 `acquire()` 與 `push_off()`、xv6 以及條件變數、futex 與 `WaitOnAddress` 的遺失喚醒、futex mutex 對上 `pthread_mutex`、競爭下的 SRW lock、critical section 與 `WaitOnAddress`，以及 Windows 上跨行程的鎖為何需要核心物件。全部已建置並執行，附輸出紀錄：xv6 與 Linux 部分在 podman 下的 Ubuntu 24.04 容器（虛擬機器）中，Windows 程式在本機。ThreadSanitizer 與 GDB 步驟尚未執行。不附參考解答。
  - [`labs/06-scheduling-latency/`](labs/06-scheduling-latency/README.zh-TW.md)：xv6 的優先權排程器與輪轉式的比較，附喚醒延遲分布圖；以及有負載時 Linux（`timerfd`、`SCHED_OTHER` 對 `SCHED_FIFO`）與 Windows（高解析度可等待計時器、四種執行緒優先權，其中一種另測停用優先權提升）上的 60 Hz 喚醒。xv6、Linux 與 Windows 部分已建置並執行，附輸出紀錄；xv6 與 Linux 部分在 podman 下的 Ubuntu 24.04 容器（虛擬機器）中。WPR／WPA 錄製與選做的彩票排程器尚未執行。不附參考解答。
  - [`labs/07-file-systems/`](labs/07-file-systems/README.zh-TW.md)：為 xv6 加上雙重間接區塊、符號連結與以檔案為後盾的 `mmap`；在寫入途中強制結束 QEMU，逐個區塊說明日誌的復原（並刻意弄壞它）；Linux 的檔案描述子與開啟檔案描述、NT 的 handle 與檔案物件，以及各系統由誰寫回被映射的髒頁。xv6 與 Linux 部分已在 podman 下的 Ubuntu 24.04 容器（虛擬機器）中建置並執行，xv6 的擴充搭配參考解答（不隨附），並通過 `usertests -q`；Windows 程式已建置並執行。全部附輸出紀錄。負責寫回的執行緒本身沒有追蹤。
  - [`labs/08-ipc-event-loops/`](labs/08-ipc-event-loops/README.zh-TW.md)：以 `SCM_RIGHTS` 傳遞 fd、以 `SO_PEERCRED` 檢查對方身分、接收端只接受無法縮小的封存 `memfd` 緩衝區、一個同時服務 socket、`timerfd`、`signalfd` 與 `eventfd` 的 `epoll` 迴圈、就緒與完成兩種模型，以及 Windows 上的具名管道伺服器、以 `DuplicateHandle` 交出的 section、`MsgWaitForMultipleObjectsEx` 與 I/O completion port。Linux 部分在 podman 下的 Ubuntu 24.04 容器（虛擬機器）中建置並執行，Windows 程式在本機，全部附輸出紀錄。`io_uring` 被容器的 seccomp 設定檔拒絕；Process Explorer 步驟與 libwayland 原始碼閱讀尚未執行。不附參考解答。
  - [`labs/09-headless-display-server/`](labs/09-headless-display-server/README.zh-TW.md)：第一部分專題。無螢幕顯示伺服器的題目與檢查清單、一份固定的線上協定（[`PROTOCOL.zh-TW.md`](labs/09-headless-display-server/PROTOCOL.zh-TW.md)），以及測試框架：兩個正常的用戶端；在影格中途當掉、永遠不讀自己的 socket、送出超出緩衝區的 damage、或送出未封存或太小緩衝區的問題用戶端；以及檢查存活、60 Hz 影格節奏、外洩的 fd 與映射、影格像素的檢查程式。測試框架已於 2026-11-16 在 podman 下的 Ubuntu 24.04 容器（虛擬機器）中建置並執行，對象是一個不隨附的參考伺服器（三次通過，附輸出紀錄），以及它刻意加入錯誤的 14 個建置，每一個都失敗。設計說明、Wayland 比較與延伸練習由學習者撰寫，尚未執行。
  - [`labs/10-windows-composition/`](labs/10-windows-composition/README.zh-TW.md)：一個使用 flip-model swapchain 的 Direct3D 11 程式，在視窗、被部分遮住與無邊框三種情況下，記錄不需特殊權限的 API 對其影格的描述（`Present` 時間、`GetFrameStatistics`、`GetFrameStatisticsMedia`、`DwmGetCompositionTimingInfo`、`WaitForVBlank`）；一張說明每個來源能證明與不能證明什麼的表；用來讀取 `PresentMode` 的 PresentMon 命令列；以及 Win+Ctrl+Shift+B 與 TDR 的比較。視窗與被遮住兩種情況已於 2026-11-16 建置並執行，vsync 開與關各兩次，附輸出紀錄；但執行環境是一台 Intel UHD Graphics 750 機器上的遠端桌面工作階段，不是 RTX 5070 Ti 桌機的本機主控台：每個影格都為 32 Hz 的遠端顯示器合成，vblank 欄位一直是 0。無邊框執行、PresentMon、任何追蹤與 Win+Ctrl+Shift+B 都沒有執行。
  - 階段 11–18 還沒有實驗資料夾。
- [`readings/`](readings/)：把幾個實驗串起來的讀物。
  - [`readings/threads-gpos-vs-rtos.zh-TW.md`](readings/threads-gpos-vs-rtos.zh-TW.md)：執行緒由什麼組成、情境切換如何進行（xv6 與 FreeRTOS 在 Cortex-M 上的 PendSV），以及 RTOS 的執行緒和 Linux、Windows 的執行緒在排程、隔離、堆疊與中斷邊界上的差異。2026-11-20 對照官方文件查核；沒有新執行程式，數字取自實驗 02、06 與 `mcu-foundations` 第七課。

## 完成目標

你會完成一個自己寫的合成器。它能以一般使用者身分從文字主控台啟動，同時執行終端機、一個 GTK 或 Qt 應用程式與一個 Vulkan 示範程式，並撐過 VT 切換、螢幕熱插拔與用戶端當掉。你還要寫一份設計說明，附實測時間戳記，追蹤一個用戶端影格從 `wl_surface.commit` 到掃描輸出、再回到影格回呼的過程。

以上都能在虛擬機器中完成。硬體驗證再到真實螢幕上重做一次；在那裡，時間戳記才有意義，也才有 VRR 與真實平面。只有後者能證明影格時序。

只完成第一部分的標準是：能附上追蹤紀錄，說明從系統呼叫、情境切換到分頁錯誤的完整過程，並分別以 xv6、Linux 與 NT 的術語解釋。

## 範圍與限制

- 不在範圍內：GPU 核心驅動、Mesa 驅動、UI 工具套件、Xwayland 內部、Windows 核心驅動（選修，限虛擬機器）、HDR 與色彩管理（選修延伸）。
- 在 Windows 上只能研究與量測 DWM，無法取代它。原因見建置計畫的階段 10。
- WSL 2 與 Linux 虛擬機器都在 hypervisor 下執行，在那裡量到的延遲只能相對比較；硬體驗證會在真實螢幕上重新量測。
- 來源於 2026-11-15 查核。查核期間 gitlab.freedesktop.org 與 git.sr.ht 拒絕自動化請求，因此 wlroots、tinywl 與 seatd 只附連結，未重新閱讀頁面。Wayland 協定的狀態取自 wayland.app 索引，該索引由相同的 XML 檔產生。
- 時數為估算。

## 審閱決定（2026-11-15）

- **第二部分的執行環境：** 預設使用 Linux 虛擬機器（QEMU 搭配 virtio-gpu，另加 VKMS），實體機只用於最後的硬體驗證。
- **總結專題的路線：** 把階段 12–16 的程式組成自己的後端，建立在 libwayland-server 之上。tinywl 只當地圖閱讀，後端不使用 wlroots。
- **語言：** 第一部分與面向核心的程式用 C；合成器可用 C++，以配合圖學課程第二部分的核心。
- **選修支線：** 全部保留，包括 rv32sim 特權模式、Windows 虛擬機器中的 KMDF 驅動、HDR 與色彩、Xwayland 與 NVK。

## 待決問題

1. 最後硬體驗證用哪台機器：在 RTX 5070 Ti 桌機上雙系統開機，還是另備一台使用 Intel 或 AMD 顯示晶片的電腦？桌機的 CPU 有內顯嗎？這可以等總結專題在虛擬機器中跑起來再決定。
2. 虛擬機器的主機端：在 Windows 上以 WHPX 執行 QEMU，還是在 WSL 2 內執行 QEMU？兩者都有官方文件支持，但都沒有記載客體的 3D 加速。建議先用 Windows 上的 QEMU，因為它的視窗是原生的，不必經過 WSLg 轉送。
3. 發行版與核心：Ubuntu 24.04 與 MIT 工具頁一致。NVIDIA 顯式同步需要 6.8 以上核心，nvidia-drm 的 KMS 色彩管線需要 6.19 以上。WSL 2、虛擬機器與 Linux 實體機要不要都用同一個發行版？
4. Acer ET322QK 是否支援 VRR？只影響硬體驗證。
5. 重用模組的圖形 API：圖學課程第二部讓你在 Vulkan 與 Direct3D 12 之間選一個並沿用到底，但只有 Vulkan 或 CPU 路徑能在本課程的 Linux 合成器中執行。圖學課程是否因此選 Vulkan？
