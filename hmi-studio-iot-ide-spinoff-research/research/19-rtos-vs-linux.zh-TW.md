# 不跑 Linux：Cortex-A 加 RTOS、高階 MCU 加 RTOS 與 Linux 的比較

研究日期：2026 年 11 月 20 日，Asia/Taipei。

本文回答：「如果 Cortex-A 上不跑 Linux 而跑 FreeRTOS，網路還能正常運作嗎？」並把問題擴大成三條路線的比較：Cortex-A 加 Linux（[18 號研究](18-mpu-runtime.zh-TW.md)的建議）、Cortex-A 加 RTOS、高階 MCU 加 RTOS。範圍包括各晶片原廠對 RTOS 的支援、高階 MCU 的規格與價格、網路與圖形效能、資安與 CRA、EHS 原始碼的移植工作量，以及產品線建議。

> **工時、效能推估與產品線建議都是推論。** EHS 程式碼的事實來自唯讀檢視 `edt-hmi-studio` 原始碼，檔名與行號是當天版本。晶片價格多為搜尋時看到的經銷商價格，日期與數量見筆記。

**標示說明**

| 標示 | 意思 |
| --- | --- |
| （無標示） | 原廠文件、SDK、資料表、官方公告，或 EHS 原始碼 |
| 二手來源 | 論壇、評測文章、經銷商頁面、搜尋摘要 |
| 推論 | 本研究的判斷，不是來源的說法 |

## 結論

1. **可以。Cortex-A 跑 RTOS，網路能正常運作，效能也足夠。** 實測數字：
   - Xilinx Zynq-7000（Cortex-A9）跑 lwIP，裸機約 943 至 948 Mbps，加上 FreeRTOS 用 socket 約 541 至 572 Mbps。
   - TI AM62L 的 A53 跑 FreeRTOS 加 lwIP，TCP 約 93 Mbps，只用 11% 至 14% CPU。

   Modbus TCP、MQTT 這類 HMI 流量不到 10 Mbps，所以網路效能不是問題。**真正的限制是晶片原廠有沒有提供 A 核心上的 RTOS 驅動。**
2. **只有三家原廠正式支援「RTOS 跑在 A 核心本身」，而且都是單核晶片。**
   - **Renesas RZ/A3UL**（單核 Cortex-A55）：Renesas 主打開機 1 秒內的 RTOS HMI。SDK 支援 FreeRTOS 與 ThreadX，有兩個 Gigabit 乙太網路、LCD、MIPI-DSI、SD、USB 主機的驅動，但沒有 2D 繪圖引擎。
   - **ST STM32MP13**（單核 Cortex-A7）：STM32CubeMP13 支援裸機、ThreadX 加 NetX Duo，或 FreeRTOS 加 lwIP，附雙乙太網路與 USB 主機範例。這條路和 EHS 現有的 STM32 經驗最接近。
   - **Microchip SAMA7D65、SAMA5D2、SAMA7G5**（單核 Cortex-A7 或 A5）：Harmony v3 支援裸機或 FreeRTOS，有自家 TCP/IP、圖形套件與 LVGL 指南。
3. **其他晶片不適合，修正先前對話的部分說法。**
   - **更正：** 先前說 ST STM32MP1 的 A 核心只支援 Linux，這不完全對。STM32MP13 有官方 RTOS 支援；STM32MP2 的裸機支援仍是 alpha。
   - **NXP i.MX93**：Real-Time Edge 3.5 與 Zephyr 可以在 A 核心跑 RTOS，但定位是和 Linux 並行的即時分區，沒提到顯示。
   - **TI AM62x**：A53 上的 FreeRTOS 標明「原樣提供，不在論壇支援」。
   - **Zynq**：網路很好，但沒有 FPGA 以外的顯示控制器。
   - **Rockchip、Allwinner**：每個公開的配置裡，網路與顯示都交給 Linux。
   - **RZ/A3M**：DRAM 在封裝內、有 2D 引擎、有官方 LVGL 9.5 移植，但規格沒列乙太網路（推論沒有，待資料表確認）。
4. **高階 MCU 加 RTOS 也能做 7 吋乙太網路 HMI，而且比 Cortex-A 加 RTOS 成熟。** 候選晶片：
   - **STM32H747、H7S7**：EHS 已經在跑 H747，換晶片成本最低。
   - **STM32N6**：800 MHz Cortex-M55，4.2 MB SRAM，Gigabit 乙太網路加 TSN。
   - **i.MX RT1170**：1 GHz Cortex-M7，網路與 OTA 範例最成熟。
   - **Renesas RA8D2**：1 GHz Cortex-M85，Gigabit 加 TSN 交換器。
   - **ESP32-P4**：晶片約 US$4，封裝內含 32 MB PSRAM。

   800×480 到 1024×600 沒問題。10 吋 1280×800 可行但吃緊，因為畫面緩衝與乙太網路會搶外部記憶體頻寬。
5. **EHS 原始碼對 RTOS 很友善。** 執行環境已經是非阻塞設計，傳輸層註解寫著「以後會是 socket」，協定驅動是 start、poll、cancel 狀態機，EHS 文件也已規劃在 H747 上跑 lwIP 與 Modbus TCP。真正的難點不是多執行緒，而是記憶體預算、網路堆疊的穩健性、安全更新，以及從「每個專案一份韌體」改成「固定韌體加專案」。
6. **工時（推論，一人使用 Claude Code）：**

   | 路線 | MVP | 完整產品 |
   | --- | --- | --- |
   | MCU 加 FreeRTOS | 單獨做 8 至 13 人月；已有可移植的執行環境核心時，當第二個目標只要多 5 至 8 人月 | 15 至 26 人月，沒有 OPC UA、SQLite，驅動較少 |
   | Cortex-A 加 RTOS | MCU 路線再加 3 至 6 人月（每個晶片家族） | — |
   | Linux（18 號研究） | 9 至 14 人月 | 22 至 38 人月 |

7. **RTOS 的代價在資安維護。**
   - **每個修補都是整份韌體更新。** 核心、網路堆疊、TLS、驅動與執行環境是同一個簽章映像。
   - **SBOM 要自己產生。** STM32Cube 不產生 SBOM。
   - **上游支援期短。** FreeRTOS 2024 LTS 已在 2026-06-30 結束，2026 LTS 只支援兩年；lwIP 沒有 LTS，2026 年的 SNMPv3、mDNS 弱點只修在 git、沒有發布版。
   - **CRA 要求不變。** 弱點通報 2026-09-11 起適用，安全更新至少五年。

   安全開機加 A/B 更新是 CRA 的前提，不是加分功能。
8. **建議三條產品線（推論）：**
   - **EHS Lite：** STM32H7、N6 加 FreeRTOS 加 lwIP，4.3 至 7 吋，Modbus RTU/TCP、一兩種 PLC、MQTT 加 TLS、網頁遠端、簽章 OTA。
   - **主力線：** Linux（18 號研究），7 至 15 吋，完整功能。
   - **快速開機線：** Cortex-A 加 RTOS，只在客戶明確要求 1 秒內開機或不用外部 DRAM 時才做。

   同一套設計器與產生器可以同時支援三者，前提是一個可移植的執行環境核心、驅動只寫一次、專案資料不放在 C 裡、每個目標有能力設定檔、全部釘同一個 LVGL 版本。
9. **Lite 線離現有程式碼最近，也保住「LVGL 能跑 MCU」這個 Linux 對手沒有的優勢；Linux 主力線的市場較大。** 先做哪一條，看客戶，不看技術。EHS 自己規劃的「H747 上 lwIP 無 RTOS 模式加 Modbus TCP」約 1 人月，可以先做，不影響後面的選擇。

## 一、Cortex-A 加 RTOS：誰支援、支援到什麼程度

| 晶片 | 原廠在 A 核心支援的 RTOS | 網路 | 顯示與圖形 | 判斷（推論） |
| --- | --- | --- | --- | --- |
| **Renesas RZ/A3UL**（1 核 A55） | FreeRTOS、ThreadX（RZ FSP） | 2 個 Gigabit；FreeRTOS+TCP 或 NetX Duo | LCD、MIPI-DSI；沒有 2D 引擎 | 網路型 RTOS HMI 的首選；HD 解析度要外接 DDR，會碰到記憶體漲價 |
| Renesas RZ/A3M（1 核 A55） | 同上 | 規格沒列（待確認） | 2D 引擎、128 MB DRAM 在封裝內、官方 LVGL 9.5 移植（[LVGL](https://github.com/lvgl/lv_port_renesas-ek-rz_a3m)）；約 US$14（500 顆） | 成本與記憶體風險最好，但若沒有乙太網路就不適合連網 HMI |
| **ST STM32MP13**（1 核 A7） | 裸機、ThreadX 加 NetX Duo、FreeRTOS 加 lwIP（[STM32CubeMP13](https://github.com/STMicroelectronics/STM32CubeMP13)） | 雙乙太網路 lwIP 範例 | TouchGFX（X-TOUCHGFX-MPU）；沒有 2D GPU，沒有 ST 的 LVGL 移植 | 和 STM32 團隊最接近 |
| **Microchip SAMA7D65、SAMA5D2、SAMA7G5**（1 核 A7 或 A5） | 裸機、FreeRTOS（Harmony v3）；Zephyr 支援 SAMA7D65 板 | Microchip TCP/IP 堆疊 | 圖形套件用晶片內 GPU；有 LVGL 指南（[Microchip](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/lvgl/mplab-harmony/)） | 可行；Zephyr 版沒有顯示驅動 |
| TI AM62x、AM62L（A53） | FreeRTOS（MCU+ SDK） | lwIP | 有顯示驅動 | TI 註明「原樣提供，不在 E2E 論壇支援」；AM64x 上的 SMP 是實驗性 |
| AMD Zynq-7000、UltraScale+ | FreeRTOS、裸機（Vitis） | 最好：lwIP 近 Gigabit 線速 | Zynq-7000 要用 FPGA 做影像輸出（推論）；UltraScale+ 有 DisplayPort | 只有本來就要 FPGA 時才划算 |
| NXP i.MX93、95、8M | Zephyr、FreeRTOS、裸機（Real-Time Edge 3.5，2026-07） | lwIP | 文件沒提 A 核心 RTOS 的顯示 | 主要是 Linux 旁的即時分區；Zephyr 有 i.MX93 A55 板（[Zephyr](https://docs.zephyrproject.org/latest/boards/nxp/imx93_evk/doc/index.html)） |
| Rockchip RK3506、RK3568 | RT-Thread 或裸機與 Linux 混跑 | 交給 Linux | 交給 Linux | 不適合純 RTOS（二手來源） |
| Allwinner T113 | Melis RTOS（只有二手來源） | 沒有證據 | — | 不適合 |

**商用前例：** Pro-face GP4000 推論跑 VxWorks（18 號研究）；Mitsubishi GOT2000 韌體叫「CoreOS」，自有 TCP/IP 堆疊出過弱點，推論是專有 RTOS（[JVN](https://jvn.jp/en/vu/JVNVU95413676/index.html)）。C-more EA9 的作業系統查不到。

**常見陷阱（筆記中各有出處）：**

1. **快取與乙太網路 DMA。** 封包描述子要放非快取或對齊快取行的記憶體；lwIP 在快取維護時可能蓋掉剛收到的資料（[FreeRTOS 論壇](https://forums.freertos.org/t/freertos-tcp-and-dma-cache-with-zynq7000/21663)）。
2. **MMU 設定。** A53 的非快取區最小 2 MB；TI 範例的記憶體屬性有錯。
3. **只用一顆核心。** Vitis 沒有 FreeRTOS SMP，TI 的是實驗性。
4. **缺驅動。** 3D GPU、Wi-Fi 幾乎只有 Linux 驅動（推論）。
5. **原廠 SDK 變動。** Renesas 舊的 RZ/A FSP 在 2026-02 停用，要搬到 RZ FSP v4，舊版只支援到 2027-01（[Renesas](https://github.com/renesas/rza-fsp)）。
6. **各家 TCP/IP 堆疊不同**（FreeRTOS+TCP、NetX Duo、lwIP、Microchip 自家），EHS 執行環境要有自己的 socket 抽象層。

## 二、高階 MCU 加 RTOS

| 晶片 | 核心 | 記憶體 | 顯示與 2D | 乙太網路 | 價格（二手來源，搜尋當天） |
| --- | --- | --- | --- | --- | --- |
| STM32H747 | Cortex-M7 480 MHz 加 M4 | 1 MB SRAM，外接 SDRAM | LTDC、Chrom-ART | 10/100 | 約 US$12.6（595 顆） |
| STM32H7S7 | Cortex-M7 600 MHz | 外接 XIP flash | LTDC、NeoChrom | 10/100 | 約 £5.9（160 顆）；開發板 US$99 |
| **STM32N6** | Cortex-M55 800 MHz 加 NPU | 4.2 MB SRAM，沒有內部 flash | LTDC、NeoChrom | Gigabit 加 TSN，3 個 CAN-FD | 約 US$13 至 14（500 顆）（[ST eStore](https://estore.st.com/en/stm32n657a0h3q-cpn.html)） |
| **i.MX RT1170** | Cortex-M7 1 GHz 加 M4 | 2 MB SRAM | LCDIF、PXP、VGLite | 2 個 Gigabit（1 個 TSN）加 1 個 10/100 | 約 US$12（600 顆）；新設計要用 B 版料號 |
| Renesas RA8D2、RA8P1 | Cortex-M85 1 GHz | 2 MB SRAM | Dave2D | 2 個 Gigabit 加 TSN 交換器 | 約 US$16 至 20 |
| ESP32-P4 | RISC-V 400 MHz | 32 MB PSRAM 在封裝內 | MIPI-DSI | 10/100 | 晶片約 US$4；v1.3 版停產，改 v3.2，軟硬體不相容（[Espressif](https://documentation.espressif.com/en/PCN202600801_ESP32-P4_Chip_Revision_v3.2_Upgrade_Chip_Revision_v1.3_Demand_Collection_and_EOL_Plan_Description.html)） |

不適合的晶片：
- STM32U5G9：找不到乙太網路。這也是 EHS 現有的 EVK 板無法走這條路的原因。
- i.MX RT1180、MCX N947、PIC32CZ CA：沒有顯示控制器。
- Alif E7：工業生態薄。

**LVGL 效能（二手來源）：**

| 晶片 | 解析度 | 結果 |
| --- | --- | --- |
| STM32H723 550 MHz | 800×480 | 82 FPS |
| STM32H7S78-DK | 800×480 | 27 FPS |
| i.MX RT1170 | 720×1280 | 47 FPS（-O2）（[NXP](https://community.nxp.com/t5/i-MX-RT-Crossover-MCUs-Knowledge/LVGL-Benchmark-Performance-Optimization/ta-p/2252208)） |
| ESP32-P4 | 1024×600 | 55 FPS |
| ESP32-P4 | 800×1280 | 40 FPS |

STM32N6 沒有公開數字。沒有任何評測同時跑 LVGL 與網路負載，EHS 要自己量。

**網路與協定：**
- **TCP 吞吐量。** 10/100 晶片上的 lwIP、NetX Duo、Zephyr 調校後約 75 至 95 Mbps；RT1170 Gigabit 範例約 130 至 430 Mbps。
- **TLS。** 每個連線約 40 至 80 KB RAM，握手約 0.2 至 1 秒（推論）。
- **OPC UA。** open62541 最小伺服器在 100 KB 左右，維護者說 256 KB RAM 很充裕；但 1.4 版移除了 FreeRTOS 支援，要留在 1.3 版或直接用 lwIP（[open62541](https://groups.google.com/g/open62541/c/eOo9n78MmtM)）。
- **MQTT、Modbus TCP、HTTP 伺服器：** 在這些晶片上都是例行工作。
- **遠端畫面。** MCU 上沒有開源 VNC 伺服器（商用有 SEGGER emVNC）。比較好的做法是讓設備提供 EHS 現成的 WASM 畫面，再用 WebSocket 傳標籤值（推論）。
- **Wi-Fi。** 外接 ESP32-C6，實測約 36 Mbps。

**商用產品：** 沒有大廠 PLC 品牌的 HMI 公開確認用 MCU。Weintek 最低階的 MT8051iP、MT8071iP 是 Cortex-A8 600 MHz 加 Linux。用 MCU 的多是智慧顯示模組，例如：
- Riverdi 10.1 吋 1280×800，STM32H757，乙太網路靠 PoE 擴充板，約 US$243。
- ESP32-P4 的 10.1 吋板，有乙太網路，約 US$80。

**更新與安全開機：**
- **ST 舊方案停止發展。** 舊版 STM32H7 的 X-CUBE-SBSFU 已不再發展。ST 改用 MCUboot 架構，H7R/S 有 STiRoT 加 OEMuRoT（[ST 論壇](https://community.st.com/t5/stm32-mcus-security/clarification-needed-sbsfu-legacy-mcuboot-and-stirot/m-p/747513)）。所以要嘛選 H7R/S，要嘛自己把 MCUboot 移植到舊版 H7。
- **NXP 有範例。** NXP SDK 附乙太網路 MCUboot OTA 範例（[NXP](https://docs.mcuxpresso.nxp.com/mcuxsdk/24.12.00/html/examples/ota_examples/mcuboot_opensource/readme.html)）。
- **MCUboot 是共通點。** ST、NXP、Renesas、Zephyr 都支援。

**物料成本（推論）：**

| 方案 | 核心成本（處理器加記憶體加 flash） |
| --- | --- |
| MCU 加 PSRAM 或 SDRAM 加 NOR flash | 約 US$18 至 35 |
| ESP32-P4 | 約 US$8 至 14 |
| Linux（依 17 號研究的 2026 年記憶體價格） | 約 US$35 至 75 |

MCU 方案也不是完全不受記憶體漲價影響：台灣 PSRAM 廠愛普 2026 年調漲 IoT RAM 價格，SDR SDRAM 也有舊型 DRAM 退場的風險。內部 SRAM 夠大、或 DRAM 在封裝內的晶片，才完全避開。

## 三、EHS 原始碼的實況與移植工作

| 事實 | 位置 | 含意 |
| --- | --- | --- |
| 三塊板子都是裸機主迴圈，`LV_USE_OS LV_OS_NONE` | `firmware/stm32h747i-disco/src/main.c:48-54` 等 | 目前沒有 RTOS |
| 傳輸層是「輪詢並取出位元組」，註解寫著以後會是 socket | `firmware/common/include/hmi_transport.h:26-37` | 換成 socket 的介面已經預留 |
| 協定驅動是 start、poll、cancel 狀態機 | `hmi_driver.h:41-59` | 不會阻塞，放進 RTOS 任務容易；同一份驅動也能跑 Linux |
| 已規劃 H747 上 lwIP 無 RTOS 模式與 Modbus TCP 實驗 | `docs/protocol-connections.md:807`、`:836-837` | 網路第一步不需要 RTOS |
| H747 映像已用 flash bank 1 約 74%、SRAM 約 55%；LVGL heap 放在 32 MB SDRAM | `docs/stm32h747i-disco-dual-core.md:93-100` | 記憶體是主要限制 |
| EVK043027B 的 STM32U5 沒有乙太網路 | `docs/protocol-connections.md` | MCU 路線要換 H7 或 N6 等級的板子 |
| 圖片與字型連結在固定 flash 位址 | `docs/images-external-flash.md:41` | 要做「不重燒就換專案」，素材要改成有索引的資料塊 |
| EDT 的 EVK 原廠程式本來就用 FreeRTOS，EHS 刻意沒採用 | `docs/edt-evk043027b.md:220` | EDT 韌體部門已有 FreeRTOS 經驗 |

**MCU 路線的建議做法（推論）：**

- **RTOS 與網路堆疊：** FreeRTOS 加 lwIP，用 STM32Cube。ST 在 2025-05 宣布 Cube 回到以 FreeRTOS 為主要核心（[ST 論壇](https://community.st.com/t5/developer-news/new-strategic-directions-for-stm32cube/ba-p/799537)）。
- **關掉用不到的功能。** IPv6、mDNS、LLMNR、SNMP 都關掉；TLS 用 mbedTLS，MQTT 用 coreMQTT。
- **任務切分：**
  - UI 任務：唯一呼叫 LVGL 的任務。
  - 執行環境任務：沿用現有 `hmi_runtime_task`。
  - lwIP 核心任務。
  - 網路應用任務：MQTT、TLS、OTA、專案下載。
  - 儲存任務：警報與趨勢寫入 SD 或 flash 環形紀錄，不用 SQLite。
- **專案載入：** MVP 先用「整份韌體 OTA」；產品階段改成「固定簽章執行環境加 QSPI 專案槽」：
  - 專案槽放 PC 編好的 UI 與 Logic 程式碼和資料，透過版本化的函式表呼叫執行環境。
  - 這就是 18 號研究建議的 (c) 在 MCU 上的版本，也延續 11 號研究「固定層與可熱重載層」的切分。
  - 同一顆 QSPI 執行中不能寫入，所以要寫到另一個 A/B 專案槽再重啟 UI。
- **PLC 驅動：** 自己寫 MC/SLMP、FINS、S7 PUT/GET、EtherNet/IP 明確訊息，全部照 `hmi_driver.h` 寫。這樣 Linux 也能用；snap7、libplctag 這類 Linux 函式庫不適合 MCU。

**MCU 路線工時（推論，人月）：**

| # | 工作 | 人月 | 和 Linux 共用 |
| --- | --- | --- | --- |
| A1 | FreeRTOS 整合、任務切分、`LV_OS_FREERTOS`、看門狗 | 0.5 至 1 | 否 |
| A2 | 乙太網路與 lwIP：描述子、快取與 MPU、DHCP、拔線與廣播風暴的穩健性 | 0.5 至 1 | 否 |
| A3 | Modbus TCP 客戶端與伺服器 | 0.25 至 0.5 | 是 |
| A4 | 多型別、多連線、多裝置標籤資料庫；資料表移出 C | 1 至 2 | 是 |
| A5 | 設計器：RTOS-MCU 目標、能力設定檔、網路搜尋與下載，主流程不再用 ST-LINK | 0.5 至 1 | 大部分 |
| A6 | TLS（mbedTLS）、MQTT（coreMQTT）、SNTP、憑證與金鑰佈建 | 1 至 1.5 | 部分 |
| A7 | 安全開機與 OTA | 1.5 至 2.5 | 否 |
| A8 | 不重燒換專案：MVP 0.25；分割映像 1.5 至 3 | 0.25 或 1.5 至 3 | 部分 |
| A9 | PLC 驅動：MC、FINS、S7、EtherNet/IP | 2.5 至 4.75 | **是** |
| A10 | 警報、趨勢、配方、使用者（精簡版） | 2 至 3.5 | 邏輯是 |
| A11 | 遠端：精簡 VNC 或網頁遠端 | 0.75 至 2 | 網頁遠端是 |
| A12 | 測試：CI 裡的 PLC 模擬器、解析器 fuzz、長測、斷電、真實 PLC 互通 | 1.5 至 2.5 | 部分 |
| A13 | CRA：SBOM、弱點監控、文件 | 0.5 至 1 | 流程是 |
| A14 | USB 隨身碟匯入 | 0.5 至 1 | 否 |
| | **MVP（單獨做）** | **8 至 13** | |
| | **MVP（Linux 之後的第二個目標）** | **5 至 8** | |
| | **完整產品** | **15 至 26** | |

完整列表在 `notes/19-rtos-vs-linux/ehs_rtos_fit.md`。

**Cortex-A 加 RTOS 額外工作（每個晶片家族 3 至 6 人月）：**
- 原廠 SDK 與開機鏈。
- MMU 與快取。
- 顯示、2D 與觸控驅動接進 LVGL。
- 乙太網路與 lwIP 移植。
- 自己的 A/B 開機與簽章檢查：沒有找到 RZ/A 或 Zynq 現成的 MCUboot 移植。
- USB 與 SD。

換來的是約兩倍於 H7 的 CPU、1280×800 加 2D、約 1 秒開機（推論，沒找到實測）。但和同級晶片跑 Linux 相比，少了程序隔離、現成驅動（OPC UA、VNC、SQLite、Wi-Fi）與 Yocto 的 SBOM 工具。

**Zephyr 作為跨平台選項：**
- **優點：**
  - 原生 TCP/IP、TLS、MQTT、HTTP/2、LwM2M、Modbus。
  - MCUboot、可執行期載入 ELF 擴充（LLEXT）。
  - `west spdx` 產生 SBOM，有資安回應團隊（[Zephyr CRA](https://docs.zephyrproject.org/latest/security/standards/cyber-resilience-act.html)）。
  - 也能跑 i.MX93 A55。
  - Canonical 宣布最長 15 年的商用支援。
- **缺點：**
  - EHS 的板子層全是 STM32Cube，要改寫成 devicetree 與 Zephyr 驅動。
  - Zephyr 對 STM32H747I-DISCO 的支援不完整。
  - Zephyr 4.2 的 LVGL 模組是 9.3，EHS 釘 9.5。
  - ST 的主力方向是 FreeRTOS。
  - 台灣人才較少。
- **工時比較（推論）：** 第一塊板子和 FreeRTOS 差不多（多 2 至 4 人月移植、省 1.5 至 3 人月現成元件），第二塊起較划算。建議先 FreeRTOS 加 lwIP，Zephyr 當第二平台。

## 四、資安與 CRA

| 事件 | 影響的堆疊 | 對 EHS 的意義 |
| --- | --- | --- |
| Zimperium，2018 | FreeRTOS+TCP（13 個 CVE，含 4 個遠端執行）（[NVD](https://nvd.nist.gov/vuln/detail/CVE-2018-16522)） | 解析器（DNS、DHCP、TCP 選項）是弱點集中處 |
| AMNESIA:33，2020 | uIP、picoTCP、FNET、Nut/Net；**lwIP 與 FreeRTOS+TCP 沒受影響**（[Black Hat EU 2020](https://i.blackhat.com/eu-20/Wednesday/eu-20-dosSantos-How-Embedded-TCPIP-Stacks-Breed-Critical-Vulnerabilities.pdf)） | 常被誤傳為影響 FreeRTOS+TCP，實際沒有 |
| NAME:WRECK，2021 | NetX（ThreadX 的網路堆疊）等（[CISA](https://cisa.gov/news-events/alerts/2021/04/15/namewreck-dns-vulnerabilities)） | 選 ThreadX 時要注意 |
| FreeRTOS+TCP，2024 至 2026 | DNS、IPv6、ICMPv6 弱點（例如 [CVE-2024-38373](https://nvd.nist.gov/vuln/detail/CVE-2024-38373)） | 關掉 IPv6 與 DNS 可以降低風險 |
| lwIP，2026 | SNMPv3、mDNS 附加功能弱點（[CVE-2026-8836](https://security-tracker.debian.org/CVE-2026-8836)），修在 git、沒有發布版 | 不用的附加功能不要編進去；要追 git 提交 |
| Zephyr，2024 至 2026 | HTTP 伺服器、DNS、ICMP、DTLS | 有公開的資安公告流程 |

**RTOS 產品在 CRA 下的四個後果（推論）：**

1. **每個修補都是整份韌體更新。** 安全開機、A/B 更新與回復是 CRA 的前提。
2. **SBOM 要自己產生。** STM32Cube 不產生；Zephyr 有 `west spdx`；Linux 有 Yocto。
3. **上游支援期要撐五年以上。** FreeRTOS 2026 LTS 支援兩年，付費延長最多十年；lwIP 沒有 LTS。要編列每年重新對齊上游的工時。
4. **專案下載服務也在範圍內。** 要驗證、簽章、檢查版本，並能在生產環境關閉。

HMI 面板在 CRA 屬一般類還是 Annex III 第一類，本研究沒有確認，要問律師。

## 五、三條路線比較（推論）

| 項目 | Cortex-A 加 Linux | Cortex-A 加 RTOS | 高階 MCU 加 RTOS |
| --- | --- | --- | --- |
| 網路協定 | 全部：Modbus TCP、S7、EtherNet/IP、MQTT、OPC UA、HTTPS、SNMP | Modbus TCP、自寫 PLC 驅動、MQTT、HTTP；每多一種協定都要自己寫 | 同左，受 RAM 限制 |
| Wi-Fi | 主線驅動加 wpa_supplicant | 原廠或外接模組 | 外接模組（自帶堆疊，可用已認證模組） |
| USB 隨身碟 | 原生 | 原廠 USB 主機堆疊 | H7 OTG 加 FatFs，0.5 至 1 人月 |
| 遠端畫面 | neatvnc、網頁 | 精簡 VNC 或網頁，自寫 | 精簡 VNC 或網頁（WASM 加 WebSocket） |
| OPC UA | 完整 | 可，要自己移植 | 原理上可，RAM 吃緊，不放 MVP |
| 同時連 PLC 數 | 數十台 | 數台 | 2 至 4 條連線實際可行 |
| 程序隔離 | 有 | 沒有 | 沒有 |
| 更新 | RAUC A/B，專案另外更新 | 自寫 A/B，整份映像 | MCUboot A/B，整份或分割映像 |
| SBOM | Yocto 自動 | 手動或 Zephyr | 手動或 Zephyr；元件最少，但每次修補都要整份更新 |
| 開機到第一個畫面 | 8 秒內，調校後 3 秒內 | 約 1 秒 | 遠低於 1 秒 |
| 核心物料成本 | SoM US$15 至 60（18 號研究） | RZ/A3M 約 US$14（DRAM 在內）；RZ/A3UL 加 DDR 較高 | US$18 至 35；ESP32-P4 方案 US$8 至 14 |
| 記憶體漲價影響 | 高 | DRAM 在封裝內時低，外接 DDR 時高 | 低到中 |
| 開發與除錯 | gdb、perf、journald；難在 BSP | 原廠 IDE 加 J-Link，社群支援少 | CubeIDE、ST-LINK、SystemView；EHS 已在用 |
| 台灣人才 | Linux BSP 多 | 少 | STM32、FreeRTOS 多；Zephyr 少 |

## 六、對 EHS 的建議（推論）

| 產品線 | 尺寸與用途 | 平台 | 功能 |
| --- | --- | --- | --- |
| **EHS Lite** | 4.3 至 7 吋，480×272 至 800×480，成本導向的 OEM 與單一 PLC 機械廠 | STM32H7（或 H7R/S、N6）加 FreeRTOS 加 lwIP | Modbus RTU/TCP、一兩種 PLC、MQTT 加 TLS、警報與環形紀錄、配方、網頁遠端、USB 匯入、簽章 OTA；不含 OPC UA、SQLite、多使用者稽核 |
| **EHS 主力線** | 7 至 15 吋 | Linux，RK3568 或 i.MX93（18 號研究） | 完整驅動、OPC UA、VNC 加網頁、SQLite 歷史、使用者與稽核、RAUC、程序隔離 |
| **EHS 快速開機線**（選配） | 7 至 10 吋，合約要求 1 秒內開機或不用外部 DRAM | Zephyr 跑 i.MX93 A55，或 RZ/A3UL 加 FreeRTOS | Lite 功能加較高解析度；有客戶才做 |
| Windows 執行環境 | 客戶工業電腦、PC 模擬器 | Linux 執行環境的 Windows 版 | 同主力線 |

**一套設計器支援三條路線的條件：**

1. 一個可移植的 C 執行環境核心。現有 `firmware/common` 已經能編 MCU 與 WASM，平台層（時間、傳輸、儲存、顯示與輸入）分成 Linux、FreeRTOS、Zephyr、WASM 四種實作。
2. PLC 驅動只照 `hmi_driver.h` 寫一次，不依賴 Linux 專屬函式庫。Linux 可以另外提供 snap7、libplctag、open62541 這類只在 Linux 上的能力。
3. 每個目標的專案資料都不放在 C 裡；產生的 C 只負責畫面與 Logic。
4. 設計器裡每個目標有能力設定檔（RAM、flash、協定、連線數、歷史儲存、遠端方式），做不到的功能變灰。
5. 所有目標釘同一個 LVGL 版本（目前 9.5.0）。
6. CI 把每個範例專案編給每個目標，並和 Emulator 做截圖比對。測試矩陣是三個目標的主要長期成本，之後每個功能約多 15% 至 25% 工時。

**先做的實驗：**

- **量測一：** 在 H747 量 lwIP 加 mbedTLS 加 coreMQTT 加 MCUboot 的 flash 與 RAM 增量，以及 TLS 握手時的 UI 畫面時間。
- **量測二：** 在 H747-DISCO、H7S78-DK、N6570-DK、RT1170-EVKB 上，用 1024×600 與 1280×800 同時跑 LVGL 9.5、lwIP iperf 與一個 TLS 連線。
- **原型：** 做 MCU 分割映像，包括函式表 ABI、XIP 專案槽、A/B 專案切換。
- **確認：** RZ/A3M 是否真的沒有乙太網路；哪些 H7R/S 有乙太網路。

## 待驗證

- RZ/A3M 是否有乙太網路；RZ/A3UL 的解析度上限（各來源 1280×720 與 1280×800 不一致）。
- 哪些 STM32H7R/S 有乙太網路；STM32N6 的 USB 速度與安全功能；RA8D2 是否支援 SDRAM。
- STM32N6 的 LVGL 效能；所有晶片在網路負載下的 LVGL 效能。
- MCU 上 TLS、MQTT、OPC UA 的實際 RAM 用量（目前只有規劃數字）。
- Cortex-A 加 RTOS 的實測開機時間。
- HMI 面板在 CRA 的分類。
- 台灣 FreeRTOS、Zephyr、嵌入式 Linux 職缺數。
- 晶片價格是搜尋當天的經銷商價格，要向台灣代理商取得 1,000 與 5,000 顆報價。

## 來源

### Cortex-A 加 RTOS

- Renesas：[RZ/A FSP（舊版，已停用）](https://github.com/renesas/rza-fsp)、[RZ/A FSP 網路模組](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___n_e_t_w_o_r_k_i_n_g___m_o_d_u_l_e_s.html)、[RZ/A FSP 圖形模組](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___g_r_a_p_h_i_c_s___m_o_d_u_l_e_s.html)、[LVGL RZ/A 移植](https://lvgl.io/docs/open/integration/chip_vendors/renesas/rza_family.mdx)、[LVGL EK-RZ/A3M](https://github.com/lvgl/lv_port_renesas-ek-rz_a3m)
- ST：[STM32CubeMP13](https://github.com/STMicroelectronics/STM32CubeMP13)、[ST 論壇（MP2 裸機）](https://community.st.com/stm32-mpus-embedded-software-and-solutions-46/stm32mp257f-dk-bare-metal-163756)、[ST 論壇（MP 與 TouchGFX）](https://community.st.com/stm32-mcus-touchgfx-and-gui-33/mp-series-and-gfx-140464)
- Microchip：[Harmony LVGL](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/lvgl/mplab-harmony/)、[SAMA7G54 乙太網路網頁伺服器](https://microchip-mplab-harmony.github.io/reference_apps/apps_mpu/sama7g54_ek/sama7g54_ek_ethernet_web_server_getting_started/readme.html)、[Zephyr SAMA7D65](https://docs.zephyrproject.org/latest/boards/microchip/sam/sama7d65_curiosity/doc/index.html)
- AMD：[Zynq UltraScale+ FreeRTOS](https://docs.amd.com/r/2020.2-English/ug1137-zynq-ultrascale-mpsoc-swdev/FreeRTOS-Software-Stack)、[SMP](https://docs.amd.com/r/2023.1-English/ug1137-zynq-ultrascale-mpsoc-swdev/Symmetric-Multiprocessing-SMP)、[FreeRTOS 論壇（Zynq TCP 效能）](https://forums.freertos.org/t/tcp-zynq-performance/6834)
- NXP 與其他：[Zephyr i.MX93](https://docs.zephyrproject.org/latest/boards/nxp/imx93_evk/doc/index.html)、[RT-Thread Rockchip](https://github.com/RT-Thread/rt-thread/tree/master/bsp/rockchip/rk3500)、[Circle（Raspberry Pi 裸機）](https://github.com/rsta2/circle)
- 陷阱：[FreeRTOS 論壇（Zynq 快取與 DMA）](https://forums.freertos.org/t/freertos-tcp-and-dma-cache-with-zynq7000/21663)、[FreeRTOS 論壇（A53 TCP/IP）](https://forums.freertos.org/t/freertos-tcp-ip-on-arm-cortexa53/9898)

### 高階 MCU

- [ST eStore STM32N6](https://estore.st.com/en/stm32n657a0h3q-cpn.html)、[NXP RT1170 LVGL 效能](https://community.nxp.com/t5/i-MX-RT-Crossover-MCUs-Knowledge/LVGL-Benchmark-Performance-Optimization/ta-p/2252208)、[NXP MCUboot OTA 範例](https://docs.mcuxpresso.nxp.com/mcuxsdk/24.12.00/html/examples/ota_examples/mcuboot_opensource/readme.html)、[NXP RT1170 Gigabit 討論](https://community.nxp.com/t5/i-MX-RT/iMXRT-1170-Slow-Gigabit-Ethernet/td-p/1507077)、[Espressif LCD 應用說明](https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/advanced-development/lcd-application-note/overview.html)、[ESP32-P4 停產與改版通知](https://documentation.espressif.com/en/PCN202600801_ESP32-P4_Chip_Revision_v3.2_Upgrade_Chip_Revision_v1.3_Demand_Collection_and_EOL_Plan_Description.html)、[Alif Ensemble E7](https://alifsemi.com/ensemble-e7-series/)
- [ST 論壇（SBSFU、MCUboot、STiRoT）](https://community.st.com/t5/stm32-mcus-security/clarification-needed-sbsfu-legacy-mcuboot-and-stirot/m-p/747513)、[ST 論壇（H7 lwIP 吞吐量）](https://community.st.com/t5/stm32-mcus-embedded-software/10-100-ethernet-maximum-throughput-with-lwip/td-p/573506)、[open62541 記憶體需求](https://groups.google.com/g/open62541/c/eOo9n78MmtM)、[ESP32-P4 加 C6 Wi-Fi 測試](https://github.com/r4d10n/esp32p4-c6-wifi-test)、[LVGL 論壇（H7S7）](https://forum.lvgl.io/t/stm32h7s7-lvgl-port/18538)
- 記憶體：[愛普 2026 年 9 月營收（TechNews）](https://finance.technews.tw/2026/10/05/ap-memory-6531-202609-financial-report/)、[DDR2 價格（iConnect007）](https://iconnect007.com/article/150498/dram-shortages-hit-ddr2-prices-set-to-keep-rising-in-3q26/150495/smt)

### EHS 適配、Zephyr 與資安

- EHS 原始碼（唯讀）：`firmware/common/include/hmi_transport.h`、`hmi_driver.h`、`hmi_platform.h`；`firmware/*/src/main.c`；`docs/protocol-connections.md`、`docs/stm32h747i-disco-dual-core.md`、`docs/images-external-flash.md`、`docs/edt-evk043027b.md`
- ST 策略：[STM32Cube 新方向（2025-05）](https://community.st.com/t5/developer-news/new-strategic-directions-for-stm32cube/ba-p/799537)
- Zephyr：[網路總覽](https://docs.zephyrproject.org/latest/connectivity/networking/overview.html)、[Modbus](https://docs.zephyrproject.org/latest/services/modbus/index.html)、[STM32H747I-DISCO](https://docs.zephyrproject.org/latest/boards/st/stm32h747i_disco/doc/index.html)、[4.2 版本說明](https://docs.zephyrproject.org/4.2.0/releases/release-notes-4.2.html)、[CRA 說明](https://docs.zephyrproject.org/latest/security/standards/cyber-resilience-act.html)、[Canonical Zephyr LTS](https://canonical.com/blog/zephyr-lts-announcement)
- FreeRTOS：[LTS](https://freertos.org/lts-libraries.html)、[2026 LTS 公告](https://aws.amazon.com/about-aws/whats-new/2026/04/freertos-lts/)、[LVGL FreeRTOS 整合](https://docs.lvgl.io/9.5/integration/rtos/freertos.html)
- 資安：[NVD CVE-2018-16522](https://nvd.nist.gov/vuln/detail/CVE-2018-16522)、[Black Hat EU 2020（AMNESIA:33）](https://i.blackhat.com/eu-20/Wednesday/eu-20-dosSantos-How-Embedded-TCPIP-Stacks-Breed-Critical-Vulnerabilities.pdf)、[CISA NAME:WRECK](https://cisa.gov/news-events/alerts/2021/04/15/namewreck-dns-vulnerabilities)、[NVD CVE-2024-38373](https://nvd.nist.gov/vuln/detail/CVE-2024-38373)、[Debian CVE-2026-8836](https://security-tracker.debian.org/CVE-2026-8836)、[lwIP 新聞](https://savannah.nongnu.org/news/?group=lwip)、[歐盟 CRA 通報](https://digital-strategy.ec.europa.eu/en/policies/cra-reporting)

研究筆記在 [notes/19-rtos-vs-linux](notes/19-rtos-vs-linux/)：`cortex_a_rtos_support.md`、`highend_mcu_path.md`、`ehs_rtos_fit.md`。
