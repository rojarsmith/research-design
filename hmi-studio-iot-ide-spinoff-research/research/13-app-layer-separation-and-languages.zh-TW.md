# 硬體與應用分離：MicroEJ 的做法，以及上層語言的選擇

查核日期：2026 年 11 月 19 日。本文回答三個問題：

1. MicroEJ 怎麼把底層硬體控制和上層 App 拆開？
2. 如果上層語言換成 Python 或 TypeScript，會怎樣？
3. 如果還是寫 C，只是改造成可以動態載入的 C，有沒有這種東西？

本研究沒有實際試用任何產品，內容依據官方文件、晶片商開源專案、論文與報導。數字都要在原型上實測。

## 結論

1. **MicroEJ 拆開的關鍵是邊界，不是 Java。** App 只呼叫固定的 API，硬體實作固定的 C 函式原型，模擬器在同一條邊界上換成 mock。這套設計 EHS 可以直接借用。
2. **上層換成 Python 或 TypeScript，架構照樣成立。** 改變的是記憶體占用、即時性、多 App 隔離和模擬器的難易度。TypeScript 比 Python 適合 EHS。
3. **「動態的 C」最可行的做法是把 C 編譯成 WebAssembly。** 原始碼是標準 C，可以熱載入，野指標只會弄壞自己。EHS 的模擬器本來就是 WASM，同一個 `.wasm` 能在 PC 模擬器和 MCU 上執行。
4. **建議分層：** 驅動與匯流排用原生 C；使用者自訂的單元與進階邏輯，用 C 編譯成 Wasm；Low Code 規則用 EHS 自己的格式。TypeScript 留作選配的腳本層。

## 一、MicroEJ 怎麼拆開硬體與 App

### 分層

```
┌─────────────────────────────────────────┐
│ Application（Java）                       │ App 工程師只寫這層
├─────────────────────────────────────────┤
│ Foundation Libraries（Java API）           │ 例如 MicroUI（畫面與輸入）、NET、FS
├─────────────────────────────────────────┤
│ Core Engine（MEJ32 VM）                   │ 排程、垃圾回收、SNI、Kernel & Features
├─────────────────────────────────────────┤
│ Abstraction Layer（LL* C 函式原型）          │ 顯示、觸控、網路各有一組 LL 函式
├─────────────────────────────────────────┤
│ BSP（C）：RTOS、HAL、驅動                   │ 晶片商或移植工程師實作
└─────────────────────────────────────────┘
```

把 Architecture（Core Engine，依 CPU 與編譯器而定）、Foundation Libraries 與其 Abstraction Layer、BSP 包在一起，就是一個 VEE Port（見 [MicroEJ 架構](https://docs.microej.com/en/latest/VEEPortingGuide/architecture.html)）。

### 四個關鍵機制

| 機制 | 做法 | 效果 |
| --- | --- | --- |
| **Abstraction Layer** | 每個 Foundation Library 往下只要求一組固定的 C 函式原型，例如「把 framebuffer 送到螢幕」、「回報一個觸控點」（見 [UI Abstraction Layer](https://docs.microej.com/en/latest/VEEPortingGuide/uiLowLevel.html)） | 換晶片只要重寫 BSP，App 不必改 |
| **SNI**（Simple Native Interface） | App 宣告 `native` Java 方法，由 C 實作；依命名規則對應函式簽名 | 標準函式庫沒有的硬體功能，也有清楚的邊界 |
| **Shielded Plug** | C 與 Java 透過具名的資料區塊交換資料，不互相呼叫 | 時序上互不阻塞；概念近似單元匯流排的「狀態」通道。內部細節未查證 |
| **執行模型** | Core Engine 跑在一個 RTOS Task 裡，所有 Java 執行緒都是這個 Task 內部排程的綠色執行緒；全部睡眠時 Task 也睡眠（見 [Core Engine](https://docs.microej.com/en/latest/VEEPortingGuide/coreEngine.html)） | App 的並行問題被關在 VM 內，不干擾 C 端。代價是：native 呼叫若卡住，會擋住所有 Java 執行緒，所以 native 函式要短 |

### 拆開之後，模擬器才做得出來

- 在 PC 上，App 的 bytecode 原封不動跑在模擬器裡。
- 原本由 C 實作的 native 函式，換成同名的 Java mock，在另一個 HIL process 執行，兩邊用 socket 溝通（見 [Mock](https://docs.microej.com/en/latest/VEEPortingGuide/mock.html)、[HIL Engine](https://docs.microej.com/en/latest/VEEPortingGuide/appendix/tools/hil.html)）。
- [Front Panel](https://docs.microej.com/en/latest/VEEPortingGuide/frontpanel.html) 是一種 mock：在桌面視窗畫出整台設備，提供按鈕、觸控、LED 和螢幕。
- 晶片商的 VEE Port 會附上自己的 mock，例如 [NXP i.MX RT1170](https://www.github.com/MicroEJ/nxp-vee-imxrt1170-evk)、[NXP MCX N947](https://www.github.com/MicroEJ/nxp-vee-mcxn947-frdm)；[Cinterion](https://repository.microej.com/packages/documentation/cinterion-vee/1.19/veeport/index.html) 則把 Abstraction Layer 實作在自家 IoT SDK 與 ThreadX 上。

MicroEJ 的模擬器是**在 API 邊界上模擬**，不是在暫存器層級模擬。這和 [PC 端模擬研究](12-mcu-device-simulation.zh-TW.md)建議 EHS 走的行為層級整機模擬是同一個思路。

### 對應到 EHS

| MicroEJ | EHS 單元匯流排的對應 |
| --- | --- |
| Foundation Library 的 Java API | 單元描述檔裡的狀態、事件與指令 |
| Abstraction Layer（LL 函式原型） | 每種單元要實作的 C 介面，由程式碼產生器產生骨架 |
| BSP | 單元驅動，加上 Zephyr 或 FreeRTOS 移植層 |
| SNI | 使用者自訂單元 |
| Shielded Plug | 匯流排上的「狀態」通道 |
| Java mock 與 Front Panel | 模擬單元：WASM 的 GUI、虛擬按鈕、虛擬 Modbus 從站 |
| VEE Port 由晶片商維護 | 板級支援包由模組商或晶片商提供，EDT 可以是第一家 |

- **要借的**：App 只看得到固定的 API；硬體只實作固定的 C 原型；模擬器在同一條邊界上換成 mock。
- **不必借的**：VM。EHS 可以用程式碼產生在純 C 裡做到同樣的邊界。
- **商業上要借的**：移植工作交給晶片商與合作夥伴，工具商只維護介面規格與驗證套件。

## 二、上層換成 Python 或 TypeScript

已有先例：Python 有 MicroPython；TypeScript 有 Moddable，它的 XS 引擎跑 JavaScript，[建置時會自動呼叫 `tsc` 轉譯 TypeScript](https://moddable.com/blog/typescript/)，[7.0.0 版](https://gh.hhj520.top/Moddable-OpenSource/moddable/releases/tag/7.0.0)起可加 `-tc` 做型別檢查。

### 元件對應

| MicroEJ | Python | TypeScript |
| --- | --- | --- |
| Core Engine | MicroPython 或 PikaPython 直譯器 | XS（Moddable）、JerryScript、QuickJS |
| Foundation Library | Python 模組（`machine`、`lvgl` 綁定等） | 帶 `.d.ts` 型別宣告的 JS 模組 |
| SNI | MicroPython 的 C 擴充模組 | XS 的 host 函式，在 manifest 裡宣告 |
| Abstraction Layer 與 BSP | 不變 | 不變 |
| Kernel & Features（多 App 沙箱） | 幾乎沒有，MicroPython 沒有正式的多 App 隔離 | XS 支援 Hardened JavaScript 的 Compartment，[Mods](https://moddable.com/documentation/xs/mods) 可以用它隔離（見 [Hardening XS](https://www.moddable.com/hardening-xs)） |
| 預先編譯 | `.mpy` bytecode | XS bytecode，直接在 Flash 上執行 |
| PC 端模擬 | 在 CPython 上跑，加 mock 模組 | 在 Node 上跑，或用 XS 模擬器 |

### 代價比較

| 面向 | Java（MicroEJ） | Python | TypeScript |
| --- | --- | --- | --- |
| 型別檢查 | 編譯時強制 | 型別提示不強制，錯誤到執行時才出現 | 編譯時檢查，執行時型別被拿掉 |
| 最低資源 | 小，官方強調可跑在 Cortex-M | [官方 README](https://github.com/micropython/micropython)：最低 256 KiB Flash 加 16 KiB RAM，舒適配置要 512 KiB 加 128 KiB；[FAQ](https://github.com/micropython/micropython/wiki/FAQ) 另有較保守的說法 | XS 也要數百 KB Flash；runmod 範例基底韌體約 700 KB |
| 即時性 | 有 GC 停頓 | 比 C 慢很多，也有 GC 停頓 | 有 GC 停頓 |
| 人才 | 嵌入式 Java 的人少 | 最多，資料與 AI 工程師都會 | 前端與 Web 工程師最多 |
| 授權 | MicroEJ 多年期合約 | MicroPython 用 MIT 授權 | XS 引擎用 Apache 2.0；Moddable 商業授權每台 US$0.50 |
| 先例的下場 | 專攻大型 OEM，活得很好 | 很普及，但幾乎沒人靠它賺錢（見 [06](06-dynamic-language-mcu-products.zh-TW.md)） | Microsoft 的 DeviceScript 在 2025 年封存；Moddable 是小公司 |

### 兩個重點

1. **換成動態語言後，模擬器反而更好做**：App 可以直接在 PC 的 CPython 或 Node 上執行。陷阱是 CPython 與 MicroPython、Node 與 XS 的行為不完全相同。最好讓 PC 端跑同一個引擎，例如把引擎編成 WASM。
2. **最難的是多 App 隔離**：MicroEJ 敢做 App Store，靠的是 Kernel & Features 加上靜態型別，可在安裝前檢查相容性。Python 幾乎做不到；TypeScript 加上 Compartment 最接近，但成熟度不如 MicroEJ。

### 為什麼 TypeScript 比 Python 適合 EHS

- EHS 前端本來就是 React 加 TypeScript，編輯器和裝置端可以共用同一種語言。
- 型別可以從單元描述檔自動產生 `.d.ts`，使用者在 VS Code 裡有自動完成和編譯時檢查。這等於自動產生的 Foundation Library。
- XS 的 Compartment 提供了做多 App 沙箱的可能。
- Python 只在目標客戶偏向 AI 或資料分析時才值得加。

## 三、改造成可以動態載入的 C

### 四條路

| 做法 | 例子 | 速度 | 熱載入 | 安全隔離 | 能用在 MCU 上嗎 |
| --- | --- | --- | --- | --- | --- |
| 1. 直接直譯 C | [PicoC](https://www.osnews.com/story/28602/a-very-small-c-interpreter-for-scripting/)（開源，只占幾 KB，[有 STM32 移植](https://github.com/RTSYork/picoc)，只支援 C 的子集）；[Ch](https://appdevelopermagazine.com/4803/2017/1/4/SoftIntegration's-Ch-7.5-supports-Raspberry-Pi,-Pi-Zero,-and-ARM)（商業 C/C++ 直譯器，Embedded Ch 跑在 ARM Linux）；Cling（桌機） | 很慢 | 可以 | 沒有，野指標能弄壞整個系統 | PicoC 可以；Ch 要 Linux 等級的 ARM |
| 2. 執行中載入原生 C | Zephyr LLEXT、ESP-IDF `hotreload`（見 [11](11-mcu-hot-reload.zh-TW.md)） | 原生速度 | 可以 | 沒有 | 可以 |
| 3. C 編譯成安全的 bytecode | **C → WebAssembly**：[WAMR](https://components.espressif.com/components/espressif/wasm-micro-runtime)（Apache 2.0）、商業的 Atym 與 Ocre、[AkiraOS](https://www.cnx-software.com/2026/06/06/docker-for-microcontrollers-akiraos-combines-zephyr-rtos-with-webassembly-wasm-applications/?amp=1)。**C → eBPF**：RIOT 的 [Femto-Containers](https://arxiv.org/abs/2106.12553v2)（學術研究） | 直譯慢；AOT 接近原生 | 可以 | 有：Wasm 只能存取自己的記憶體；Femto-Containers 的隔離[經過形式化驗證](https://arxiv.org/pdf/2210.03432) | 可以 |
| 4. 長得像 C 的動態語言 | Squirrel（Electric Imp 的商業 IoT 平台）、AngelScript | 中等 | 可以 | 部分 | 語法像 C，語意已經不是 C |

WAMR 在 Cortex-M4F 上的大小：只跑 AOT 約 29 KB，直譯器約 56 至 59 KB（見 [Espressif 元件頁](https://components.espressif.com/components/espressif/wasm-micro-runtime)、[記憶體用量](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/memory_usage.md)）。AkiraOS 的報導說，在 Zephyr 上 AOT 比直譯快 10 到 50 倍。[一份 ESP32-C3 的研究](https://aiotwin.eu/aiotwin/results/blog/webassembly_on_tiny_iot_devices_how_far_can_we_push_portability)也認為 AOT 是效能可接受的關鍵。Femto-Containers 論文報告的記憶體額外負擔低於 10%，啟動時間數十微秒。

### 為什麼 C 轉 WebAssembly 最適合

- **原始碼還是標準 C**：用 Clang 的 `wasm32` 目標編譯，工具與經驗都能照用。
- **野指標只弄壞自己**：模組只能存取自己的記憶體，不會拖垮驅動或匯流排。
- **可以熱載入**：開發時用直譯模式即改即跑，量產時改用 AOT。
- **不只限 C**：同一個執行環境也能跑 Rust、Zig，以及 TypeScript 方言 AssemblyScript，第二節的路也沒被堵死。
- **和 EHS 的模擬器天生契合**：EHS 的模擬器本來就是 WASM。使用者模組編成 `.wasm` 後，可以在瀏覽器或桌面版模擬器裡跑，也可以在 MCU 上由 WAMR 跑**同一個二進位檔**。這樣模擬器和真機之間沒有「CPython 和 MicroPython 不一樣」的落差，[熱重載](11-mcu-hot-reload.zh-TW.md)、[整機模擬](12-mcu-device-simulation.zh-TW.md)與錄製重播也能共用同一套機制。

## 四、建議的 EHS 分層

| 層 | 形式 | 熱載入 | 對應 MicroEJ |
| --- | --- | --- | --- |
| 驅動、匯流排本體、LVGL | 原生 C | 否，需要燒錄 | BSP 與 Core Engine |
| 單元介面 | 從單元描述檔產生的 C 原型與 `.d.ts` | 不適用 | Abstraction Layer 與 Foundation Library |
| 使用者自訂的單元與進階邏輯（Pro Code） | C 編譯成 Wasm | 是，且彼此隔離 | App 與 SNI |
| 交換中心的規則（Low Code） | EHS 規則格式，可編譯成 C 或 Wasm | 是 | 無對應 |
| 選配腳本層 | TypeScript | 是 | 無對應 |

這個分層和 [收費化路線圖](10-ehs-paid-product-roadmap.zh-TW.md)的「不做 MicroEJ 那種虛擬機平台」並不衝突：核心仍是純 C 輸出，Wasm 只用在使用者擴充的部分，而且是選配。

## 五、待驗證

- **Wasm 的 RAM**：Wasm 規格的記憶體以 64 KB 為一頁，在只有數百 KB RAM 的 MCU 上可能太浪費；WAMR 能否縮小要實測。
- **呼叫原生函式的成本**：Wasm 每次呼叫 LVGL 或匯流排都要經過綁定層；畫面更新這類高頻呼叫應留在原生 C。
- **除錯體驗**：Wasm 模組在 MCU 上的原始碼級除錯，工具還不成熟。
- **商業支援**：WAMR 開源但沒有主要的商業支援廠商；Atym 有付費平台，價格不公開。
- **腳本引擎選擇**：Moddable XS 每台收 US$0.50；JerryScript、QuickJS 免費，但沒有現成的沙箱與工具鏈。要實測記憶體與 GC 停頓，看能不能在 STM32F7 等級的 MCU 上和 LVGL 共存。
- **MicroEJ 細節**：Shielded Plug 的內部機制、SNI 讓出執行緒的 API，以及 MicroEJ 官方列出的 JavaScript 支援和 Java 的整合深度，都沒有逐項查證。
