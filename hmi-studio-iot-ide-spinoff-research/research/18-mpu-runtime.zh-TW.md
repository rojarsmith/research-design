# 把 EHS 從 MCU 搬到 Cortex-A 與 x86：工業 HMI 的執行環境

研究日期：2026 年 11 月 20 日，Asia/Taipei；同日完成一輪查證（見「查證結果」）。

本文回答：「NOVAKON iFace 這類工業 HMI 跑在 x86 或 Cortex-A 上，用 Linux 或 Windows 這種傳統嵌入式作業系統。EHS 目前產生 C 程式、編譯成 STM32 裸機韌體。如果要做這類競品，執行環境該怎麼做、要花多少工？」範圍包括主流 HMI 廠的執行環境架構、EHS 程式碼可沿用的程度、執行環境方案、Linux 系統堆疊、開源工業函式庫、候選硬體與工時。NOVAKON 的商業模式與價格見 [17 號研究](17-novakon-hardware-model.zh-TW.md)。

> **工時與設計建議都是推論。** 程式碼的事實來自唯讀檢視 `edt-hmi-studio` 原始碼（0.9.2-dev），檔名與行號是當天版本。

**標示說明**

| 標示 | 意思 |
| --- | --- |
| （無標示） | 原廠資料表、手冊、官方文件，或 EHS 原始碼 |
| 二手來源 | 代理商說明、論壇、部落格、搜尋摘要 |
| 推論 | 本研究的判斷，不是來源的說法 |

## 結論

1. **主流 HMI 廠沒有一家把客戶的專案編譯成韌體。** Weintek、Siemens、Mitsubishi、Delta、Red Lion、Exor 等都在面板上裝一套固定的執行環境與作業系統，設計軟體只下載專案檔，作業系統另外更新。EHS「每個專案編一次韌體」的做法，比較像 Qt for MCUs、TouchGFX 或直接用 LVGL，不像工業 HMI。
2. **處理器以 32 位元 ARM Cortex-A 為主，作業系統以嵌入式 Linux 為主。** 例如 Delta、Samkoon、昆侖通態用 Cortex-A7，Weintek cMT X 用四核 Cortex-A17，NOVAKON 用 Cortex-A8。Weintek、Delta、Exor、Red Lion、Unitronics、Samkoon 都明寫 Linux；Siemens 官方文件確認 Unified Comfort 是 Linux（舊款 Comfort 是 Windows CE），Rockwell、昆侖通態、Beijer 在 Windows CE 停止支援後也轉向 Linux 或網頁執行環境。例外是日系：Pro-face GP4000 修補的都是 VxWorks 的弱點，推論跑 VxWorks；Mitsubishi GOT2000 推論是即時作業系統。x86 只出現在工業電腦與 PC 版執行環境。
3. **EHS 大部分可以沿用。** 可沿用的部分包括：設計器、`.ehsp` 專案格式與版本遷移、元件、Logic 圖、C 程式產生器、LVGL 9.5。證據就在原始碼裡：Emulator 已經把產生的 C 加上韌體共用執行環境（`firmware/common`）用 emcc 編成 WASM，代表這些程式碼不依賴 STM32。
4. **要重做的是產品外形，不是畫面。** 包括四項：
   - 每個專案一份韌體，改成固定執行環境載入專案。
   - 單一主迴圈改成 Linux 多執行緒程序。
   - 標籤模型擴充：目前只有 float、一個裝置、只支援 Modbus 區域，要改成多型別，帶品質與時間戳，並支援多裝置。
   - 儲存改成檔案、SQLite 與 A/B 系統映像。

   另外還有新功能要做：警報、歷史趨勢、配方、使用者權限、遠端存取。
5. **建議的執行環境：固定執行環境，加上在 PC 上預先編好的專案模組（推論）。** 設計器照舊產生 C，在 PC 上用內附的交叉編譯器編成共享函式庫（Linux 的 `.so`、Windows 的 `.dll`）。標籤、裝置、警報、配方、使用者改放資料檔。這個做法有五個好處：
   - Emulator 與實機跑同一份程式碼。
   - 設備端不需要編譯器。
   - 下載只要幾秒，可以熱更新。
   - 自訂 C 區塊照樣能用。
   - 同一套執行環境編成 x86 版，就是能接真實 PLC 的 PC 模擬器。

   對使用者來說，這和競品一樣是「一鍵下載專案」。
6. **Windows 只做軟體，不做自有硬體（推論）。** Windows 版執行環境幾乎免費就有（它就是 PC 模擬器），可以賣執行環境授權給客戶自己的工業電腦。自有硬體則用 Linux：Windows 11 IoT Enterprise LTSC 2024 每台 Entry 級約 US$40 至 65、Value 級約 US$80 至 130（查證後更正），相對 US$65 至 125 的 ARM HMI 物料成本仍然偏貴。
7. **工時（推論）：一位熟悉嵌入式 Linux 的工程師使用 Claude Code，MVP 約 9 至 14 人月，接近 iFace 的完整產品約 22 至 38 人月。** MVP 指一款 SoM，Modbus 加一種 PLC，警報、趨勢、配方、使用者、OTA、下載。這比先前對話中「軟體 3 至 4 個月」的估計長，因為逐項列出後，PLC 驅動、Linux 系統映像與測試都比預想重。

   AI 加速不了的部分有三項：
   - 真實 PLC 互通測試：要一個約 US$1 萬至 3 萬的 PLC 實驗室。
   - 認證：CE、UL、FCC、ODVA，以及 CRA 流程。
   - BSP 啟動、開機時間與散熱驗證。
8. **時程換算（推論）：一人做，到第一台能賣的產品約 12 至 18 個月；兩到三人約 6 至 9 個月。** 硬體打樣與認證可以和軟體並行。
9. **NOVAKON 的技術落後是切入點。** NOVAKON 的 HMI 跑 Qt 4.8 加 TI AM335x 參考設計，2025 年有 5 個重大 CVE。EHS 可以用四點拉開差距：
   - LVGL 用的記憶體少，可以降低 2026 年記憶體漲價的衝擊。
   - 開機目標 8 秒內，進階做到 3 秒內。
   - 採用 A/B 簽章更新，程序不以 root 執行。
   - 提供 SBOM，對應 CRA 要求。

   而且 LVGL 也能跑 MCU，可以同時做比 Linux HMI 更便宜的機型。這是只做 Linux 的對手做不到的。

## 一、主流 HMI 廠怎麼做執行環境

| 廠商 | 處理器 | 面板作業系統 | 專案怎麼上機 | PC 版執行環境 | 執行環境授權 |
| --- | --- | --- | --- | --- | --- |
| Weintek cMT X、MT8000iE | 四核 Cortex-A17 1.6 GHz；舊款 Cortex-A8 | Linux（資料表，經代理商） | 下載專案檔，韌體另外更新 | 模擬、cMT Viewer | 自家硬體免費；400 多種驅動（二手來源） |
| Siemens Unified Comfort | 沒公開 | Linux：Siemens 移轉指南寫「Industrial OS」是 Linux 系統，舊款 Comfort 是 Windows CE（[Siemens](https://support.industry.siemens.com/cs/attachments/109768002/109768002_Guide_to_Unified_Comfort_V10_DOC_en.pdf)）；論壇稱 V17 至 V19 對應 Debian 10 至 12（二手來源） | TIA 下載，另有「Update OS」 | WinCC Unified PC，按 PowerTag 收費 | 面板內含，PC 版收費 |
| Pro-face GP4000 | 沒公開 | 推論是 VxWorks：GP-Pro EX 4.09 修補的「OS 弱點」都是 VxWorks 的 CVE（含 URGENT/11）（[Pro-face 版本說明](https://www.proface.tech/otasuke/download/update/proex/proex/v409/Readme_E_GPProEX_v409450.txt)） | 下載專案，OS Core 另外更新 | WinGP、BLUE Runtime（收費，推論） | 面板免費 |
| Mitsubishi GOT2000 | 沒查到 | BootOS、CoreOS、標準監控 OS；弱點公告指向韌體內的 TCP/IP 堆疊，推論是即時作業系統（[Mitsubishi PSIRT](https://www.mitsubishielectric.com/psirt/vulnerability/pdf/2020-005_en.pdf)） | 專案資料與 OS 一起寫入 | GT SoftGOT2000（USB 金鑰，紐西蘭零售約 NZ$3,800） | 面板免費，GOT Mobile 每 10 授權約 ¥38 萬 |
| Delta DOP-100 | Cortex-A8 800 MHz；大尺寸雙核 Cortex-A7 | Embedded Linux | 下載專案檔，支援 Lua | 模擬 | 免費 |
| Samkoon SK | 雙核 Cortex-A7 1 GHz | Linux（選型手冊，2022-10）；低階 EA 系列是 ARM9 加即時系統；300 多種 PLC 協定（[Samkoon 選型手冊](https://www.rusavtomatika.com/manuals/Product-Selection-Guide-20221026.pdf)） | 下載專案檔 | 模擬 | 免費 |
| Kinco GL、昆侖通態 | ARM 800 MHz（Kinco 型錄）；Cortex-A7（昆侖通態） | Linux（代理商說法；昆侖通態舊款是 WinCE）（二手來源） | 下載專案檔 | 模擬 | 免費；昆侖通態 PC 版按點數收費 |
| Red Lion FlexEdge | ARM，1 GB RAM | Alpine Linux，核心 4.14 | 下載 Crimson 資料庫，韌體一起更新 | 模擬 | 免費；300 多種驅動 |
| Exor eX700 | 雙核 Cortex-A9 | Linux RT | 下載 JMobile 專案，執行環境依平台 | JMobile PC Runtime 約 £456 | 自家硬體免費 |
| Beijer X3 | 沒公開 | OS3，用容器（推論是 Linux） | 下載專案或容器應用 | — | — |
| Unitronics UniStream | 雙核 | Linux | PLC 與 HMI 一起下載 | 模擬 | 免費 |
| CODESYS TargetVisu for Linux | x86-64、ARM、ARM64 | Debian 12 以上 | 隨 PLC 應用下載 | — | €0，授權改收在 PLC 端；用 Qt 6 |
| NOVAKON iFace | Cortex-A8（AM3352）；工業電腦 x86 | Linux 加 Qt 4.8；工業電腦 Windows 10 IoT 或 Debian 11 | 下載編譯好的專案 | x86 執行環境（可能收費） | 自家硬體免費 |

來源見文末。還沒查到的有：Siemens Unified Comfort 與 Pro-face GP4000 的 CPU、Mitsubishi GOT2000 的 CPU 與作業系統名稱、Omron NA5 的作業系統（授權清單頁面被擋）、C-more EA9 的作業系統（只知道是 800 MHz CPU）。

**2020 至 2026 年的趨勢**

- **Windows CE 退場。** WEC 2013 延伸支援 2023-10-10 結束，OEM 授權賣到 2028 年（[Microsoft](https://learn.microsoft.com/windows/iot/windows_ce/windows-ce-migration-faq)）。各廠的轉法：
  - Siemens 從 CE 版 Comfort 轉到 Unified，改用網頁執行環境。
  - Rockwell PanelView 5000 改跑 Linux。
  - 昆侖通態從嵌入版轉到 McgsPro（Linux）。
  - Beijer 從 X2 轉到 X3、OS3。
  - Weintek、Delta、Exor、Red Lion 在 2020 年前就已經是 Linux。
- **兩種畫面架構並存（推論）。** 一種是「面板跑原生執行環境，遠端用網頁看」，Weintek、Delta、Unitronics 用這種，便宜的 ARM 也跑得順。另一種是「面板本身就是瀏覽器」，Siemens Unified、B&R、Weintek Web Panel、Beijer X3 用這種，要 Cortex-A53 以上或 x86，以及 1 至 4 GB 記憶體。
- **區網遠端免費，雲端遠端收費。** VNC、網頁檢視是基本功能；Weintek EasyAccess 2.0、Exor Corvina、Pro-face Connect、Mitsubishi GOT Mobile 是付費加購。
- **容器與資安。** Siemens Industrial Edge、Beijer OS3 讓面板跑其他應用；連線加密、強制密碼、可更新的 Linux 執行環境，正在變成必要條件。

（推論）客戶會拿 Weintek、Delta、Kinco、昆侖通態來比。對他們來說，「執行環境」就是面板上一套固定程式，設計軟體負責下載專案。EHS 必須提供相同的使用體驗：一鍵下載、幾秒生效、不碰 ST-LINK。

## 二、EHS 有多少可以沿用

| 部分 | 判斷 | 說明 |
| --- | --- | --- |
| 設計器（React 19、Vite 7）與桌面殼層（.NET 加 WebView） | 沿用 | 已經能在 Windows、Linux、macOS 建置；加 MPU 目標類型，拿掉 MCU 專用的記憶體預算 |
| `.ehsp` 專案格式與版本遷移 | 沿用 | `target` 幾乎和板子無關；加 `mpu-linux`、`windows` 目標，以及警報、配方、使用者、多裝置區段 |
| 元件與 C 程式產生器 | 沿用 | `ui.c.ts` 約 10,116 行、`ui_events.c.ts` 1,635 行、`ui_logic.c.ts` 2,020 行；產生的 C 只呼叫 LVGL 和少量 `hmi_*` API，幾乎不含板子知識 |
| Logic 圖 | 沿用並擴充 | 加多型別值與新節點（警報確認、載入配方、登入、寫記錄）；「Read Tag／Write Tag 與協定無關」的設計最有價值 |
| LVGL 9.5.0 | 沿用 | `lv_conf.h` 改用 pthread、NEON、Linux 驅動；可以開 FreeType 顯示完整中日韓字型，不必再裁切字集 |
| Emulator（瀏覽器內 WASM） | 沿用 | 仍是設計器內的預覽；原生 PC 執行環境變成第二個、更接近實機的模擬器 |
| 共用執行環境 `firmware/common`（約 8,900 行 C） | 移植 | 平台層只有一個 `hmi_platform_tick_ms`，傳輸層 7 至 9 個位元組流函式；換成 `clock_gettime`、termios（含 RS-485）、TCP、SocketCAN |
| 板子層（LTDC、QSPI、SD、JPEG、AVI 影片） | 換成 Linux 驅動 | DRM/KMS 或 fbdev、evdev、sysfs 背光、ALSA、FFmpeg 或 GStreamer、`/dev/watchdog` |
| 建置與燒錄（PowerShell、STM32CubeCLT、ST-LINK） | 重做 | 改成「PC 編譯專案模組，經乙太網路或 USB 隨身碟傳送」 |
| 每專案一份韌體、單一主迴圈、float 標籤、MCU 儲存 | 重做 | 見下 |
| 警報、歷史趨勢、配方、使用者、遠端、OTA、資安強化 | 新增 | 見第四節 |

**重做的四件事（推論）：**

- **標籤模型。** 目前 `hmi_tag_read(uint16_t tag, float *value)` 只有 float，超過 2^24 的整數會失真，也無法表示 INT64、字串寫入、陣列或 PLC 結構；每個專案只有一條連線、一個裝置（`src/types/connections.ts`）。要改成多型別值，每個值帶品質（正常、通訊失敗、過期）與時間戳，支援多連線、多裝置與各自的輪詢週期。MCU 目標可以保留舊 API。
- **並行。** LVGL 不是執行緒安全，所有 LVGL 呼叫留在 UI 執行緒；每條連線一個 I/O 執行緒；標籤資料庫發送變更事件給 UI、警報、記錄、OPC UA、MQTT。現有的非同步 Modbus RTU 客戶端可以原封不動放進 I/O 執行緒。用 systemd 監看，當機自動重啟。這和 [09 號研究](09-mcu-unit-bus-architecture.zh-TW.md)的匯流排構想一致。
- **儲存。** 外部 flash 的圖片與字型改成專案檔裡的檔案；保留值、警報、趨勢、操作紀錄放 SQLite（WAL 模式）；系統映像分 A/B 兩份唯讀，加上資料分割區與出廠分割區。最大風險是斷電，要選有斷電保護的 eMMC。
- **建置與下載。** 見第三節。

## 三、執行環境的四種做法

| | (a) 每個專案編一份完整執行檔 | (b) 固定執行環境讀 JSON 或 XML | **(c) 固定執行環境載入預編模組（建議）** | (d) 設備上跑 Emulator 的 WASM |
| --- | --- | --- | --- | --- |
| 做法 | 和現在部署一樣，只是目標換成 Linux | 設備端解析畫面、標籤與邏輯，Logic 要直譯器 | 執行環境裝一次；設計器在 PC 把產生的 C 編成小型共享函式庫，其他設定放資料檔 | 瀏覽器 Emulator 的同一個 `.wasm` 在設備上用 WAMR 執行 |
| 產生器沿用 | 全部 | 低：約 1.4 萬行範本邏輯要重寫成載入器 | 全部 | 全部 |
| 預覽與實機一致 | 一致 | **不一致**：兩套引擎，EHS 已經因此放棄過 JSON 預覽（`docs/preview-ladder.md` §8） | 一致 | 完全一致（同一個二進位檔） |
| 自訂 C 區塊 | 可用 | **不可用** | 可用 | 可用（沙盒內） |
| 不重建就下載 | 否，每次數十 MB | 是 | 資料修改不用編譯；畫面修改在 PC 編幾個檔案，幾秒 | 是 |
| GPU 或 2D 加速 | 可 | 可 | 可 | **不可**（只有軟體繪圖） |
| 風險 | — | LVGL 9.5 已移除 XML 引擎，9.5 起執行期載入要付費授權；XML 格式授權也禁止第三方編輯器與產生器未經書面許可使用（見下）。要用就用 EHS 自己的格式 | 設計器要附交叉編譯器（clang 或 zig，都是寬鬆授權）；模組要簽章 | 效能要實測 |

**建議 (c) 的理由（推論）：**

- **維持單一事實來源。** EHS 在 `logicTrace.ts` 明白寫了設計原則：除錯要看「客戶的程式碼回報它做了什麼」，不是「編輯器裡的直譯器猜它會做什麼」。(b) 等於在設備上重新引入直譯器。
- **下載快。** 重的部分（LVGL、驅動、協定）都在執行環境裡；Emulator 用 emcc 增量建置只要「幾秒到幾十秒」，原生 clang 差不多。
- **熱更新。** 改標籤位址、警報上下限、配方、使用者不用編譯；改畫面或 Logic 時換模組，目標在 1 秒內生效。這延續了 [11 號研究](11-mcu-hot-reload.zh-TW.md)的熱重載方向。
- **PC 模擬器免費得到。** 同一套執行環境編成 x86-64 Windows 或 Linux 版，用 SDL 驅動，就是能輪詢真實 PLC 的離線與線上模擬器。
- **具體設計。**
  - 專案包含四類內容：說明檔（執行環境 ABI 版本、架構、雜湊）、各架構的模組、資料檔、素材，以及原始 `.ehsp`（方便從 HMI 上傳回編輯器，也方便執行環境升級後重編）。
  - 專案以 Ed25519 或 X.509 簽章。
  - 設計器在區網用 mDNS 找到設備，驗證後上傳，設備保留前一版供回復。
  - 現有的 `hmi_tag_descriptors` 與 `hmi_binding_descriptors` 本來就是產生出來的表格，改成資料檔的工作量小。

(b) 留到以後：如果客戶要求在設備或網頁上直接改畫面，再把 `wasm/src/ui_from_json.c`（已支援約 40 種元件）擴充成完整載入器，並在 CI 用截圖比對驗證和產生的 C 一致。

**不要用 LVGL XML（查證後更正）：**
- **9.5.0 起拿掉了 XML 引擎。** LVGL 9.5.0（2026-02-18）移除 XML 引擎，原始碼沒有 `lv_xml*` 檔案，9.4 有 71 個。EHS 釘的正是 9.5.0。
- **9.5 起執行期載入要付費。** LVGL 2026-07-27 的公告說，設備上執行期載入 XML 是「付費授權功能、沒有免費方案」，9.4 以前的 XML 引擎永久維持 MIT（[LVGL 公告](https://lvgl.io/blog/announcement-lv_xml-removal-from-v9-5)）。
- **格式本身也有限制。** 2025 年的 XML 授權禁止任何第三方「UI 編輯器、視覺化建構工具、程式碼產生器」未經書面許可讀寫 LVGL XML 格式（[LVGL XML 授權](https://lvgl.io/docs/open/9.3/details/auxiliary-modules/xml/license)）。EHS 正是這類工具。
- **初版的說法有誤。** 初版說「授權條款兩個版本互相矛盾」，實際上是 2025 年條文與 2026 年政策改變之間的差異。
- **結論：** EHS 如果要做資料驅動的載入器，要用自己的格式。(d) 值得做兩週的實驗：它讓所有 CPU 共用一個檔案，又有沙盒，但要先量 Cortex-A55 上的畫面時間。

## 四、Linux 堆疊、工業功能與硬體

### 系統堆疊（推論）

| 項目 | 建議 |
| --- | --- |
| 建置系統 | NXP、TI、ST 用 Yocto（原廠 BSP 與安全更新在那裡）；Rockchip、Allwinner 低價線用 Buildroot；原廠 Debian 只用來做原型 |
| 繪圖 | LVGL 9.5 的 DRM/KMS 加 evdev 為預設；舊 BSP 用 fbdev；只有多個應用共用螢幕才用 Wayland（[LVGL 9.5 Linux 文件](https://docs.lvgl.io/9.5/integration/embedded_linux/index.html)） |
| 加速 | 先用軟體加 NEON 與局部更新；需要時再用 NXP PXP 或 G2D 這類 LVGL 支援的 2D 引擎；OpenGL ES 繪圖單元還不成熟 |
| 開機時間 | 開機畫面 1 秒內，可操作畫面 8 秒內，進階 3 秒內。公開資料：Forlinx 的 i.MX93 LVGL 3.1 秒（[Forlinx](https://www.forlinx.net/article_view_652.html)），i.MX93 從 13.2 秒調到 1.8 秒（[Elektor](https://www.elektormagazine.com/labs/how-we-reduced-linux-boot-time-on-imx93-to-18s)）。競品沒有公布開機規格。唯一的數字來自一個沒說明方法的第三方部落格：Weintek MT8071iE 11.9 秒、Red Lion G3 約 7 秒、Delta DOP 30 至 60 秒、PanelView Plus 6 約 67 秒（[industrialmonitordirect](https://industrialmonitordirect.com/blogs/knowledgebase/micro820-hmi-boot-times-panelview-vs-red-lion-vs-maple-vs-weinview)，二手來源）。要自己買機實測 |
| 更新 | 唯讀 A/B 根檔案系統，用 RAUC（LGPL）；專案另外由執行環境更新；之後再用 hawkBit 或自建伺服器管理機隊 |
| Windows | 執行環境做 Windows 版，當 PC 模擬器並賣授權；自有硬體不用 Windows。Windows 11 IoT Enterprise LTSC 2024 依 CPU 等級分級：加拿大經銷價 Entry 約 CAD 88（約 US$63）、Value 約 CAD 132、High End 約 CAD 238（[CDW](https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-entry-license-1-license/7983139)，二手來源）；Dell 精簡型電腦的 Entry 升級 US$64.99（[Dell](https://www.dell.com/en-us/shop/windows-11-iot-enterprise-ltsc-2024-entry-upgrade-solution-for-dell-thin-clients/apd/634-czbg/software)）；Advantech 的 Windows 10 IoT 2021 LTSC Entry US$60.62、Value US$130.62（[Advantech](https://buyemt.advantech.com/Peripherals-Modules/Embedded-Peripherals-Microsoft-Windows-Embedded-Windows-10-IoT-Licenses/EMT_30970.products.htm)）。Arm 版（Base）沒有公開價格；OEM 量價不公開 |

### 工業功能與開源元件

授權已在查證時逐一對照各專案的官方授權檔（2026-11-20）。LGPL 要動態連結並可替換；MPL-2.0 是檔案層級的 copyleft；執行環境程序內避免 GPL。其他元件：RAUC 是 LGPL-2.1+；SWUpdate 核心是 GPL-2.0；Mender 客戶端是 Apache-2.0；WAMR 是 Apache-2.0 加 LLVM 例外；FFmpeg 預設 LGPL-2.1+，開 `--enable-gpl` 就變 GPL；LVGL、zig 是 MIT；Qt 6 的 Virtual Keyboard、Quick 3D、MQTT 等模組只有 GPL 或商業授權。

| 需求 | 元件 | 授權 | 說明 |
| --- | --- | --- | --- |
| Modbus RTU/TCP | 擴充 EHS 自己的 RTU 程式，或 libmodbus | 自有；LGPL | 自己加 TCP 工作量小，也避開 LGPL |
| Siemens S7 | snap7 | LGPL-3.0 | S7-1200、1500 要開 PUT/GET；最佳化存取（S7CommPlus）沒有免費 C 函式庫 |
| Mitsubishi MC、SLMP | 自己寫 | — | 協定小且有官方文件；有 MIT 授權的 C#、Rust 實作可參考 |
| Omron FINS | 自己寫 | — | 協定小；NJ/NX 也支援 EtherNet/IP |
| EtherNet/IP 客戶端 | [libplctag](https://libplctag.github.io) | MPL-2.0 或 LGPL | 當 HMI 讀 Rockwell、Omron PLC；做 adapter 要 ODVA 授權 |
| OPC UA | open62541 | MPL-2.0 | 官方認證要另付費 |
| MQTT | Paho C 或 libmosquitto | EPL-2.0／EDL-1.0 | |
| 警報、趨勢、配方、記錄 | SQLite 加自有程式 | 公有領域 | 即時趨勢元件已經存在（`trendCode.ts`） |
| 使用者與密碼 | libsodium（Argon2id） | ISC | 稽核紀錄可做成付費進階功能 |
| 遠端 | [neatvnc](https://github.com/any1/neatvnc)；網頁可用 noVNC | ISC；MPL-2.0 | 避開 GPL 的 libvncserver。EHS 已經能把專案編成 WASM，「同一套畫面在瀏覽器看」是現成的差異化 |

### 候選硬體（模組價格，量級參考）

| SoC | 規格 | 尺寸 | 模組價格 | 說明（推論） |
| --- | --- | --- | --- | --- |
| Rockchip RK3506 | 3 核 Cortex-A7，有 2D，沒有 3D GPU | 4.3 至 7 吋低價 | 約 US$15 至 20（[CNX](https://www.cnx-software.com/2025/08/15/armsom-cm1-a-15-compute-module-sbc-hybrid-powered-by-rockchip-rk3506j-soc/)） | 新晶片，文件在補齊 |
| Allwinner T113-S3 | 2 核 Cortex-A7，內含 128 MB DDR3 | 4.3 至 7 吋最低價 | 約 US$14（2023 年，[CNX](https://cnx-software.com/2023/05/11/14-allwinner-t113-s3-cpu-module-128mb-ram-256mb-nand-flash-4gb-emmc-flash)） | DRAM 在封裝內，避開記憶體漲價；原廠 BSP 較弱 |
| Rockchip RK3566、RK3568 | 4 核 Cortex-A55，Mali-G52 | 7 至 15.6 吋 | RK3568 模組約 US$54（[DigiKey](https://www.digikey.com.mx/en/products/detail/forlinx-embedded/FORLINX-FET3568-C-202GSE16GCA10/21810672)）；RK3566 晶片約 US$10.7（102 顆以上）、模組約 US$40 至 78 | 中國 HMI 常用，亞洲供貨好 |
| NXP i.MX93 | 2 核 Cortex-A55 加 M33，PXP | 7 至 10 吋 | 約 US$39 至 59（[Variscite](https://shop.variscite.com/product/system-on-module/dart-mx93-system-on-module/)） | 長期供貨，歐美客戶信任；開機 3.1 秒已有實例 |
| NXP i.MX8M Mini | 4 核 Cortex-A53，3D 與 2D | 7 至 10 吋 | 量價 US$44.90 起（[Toradex](https://www.toradex.com/computer-on-modules/verdin-arm-family/nxp-imx-8m-mini)） | Yocto BSP 成熟 |
| ST STM32MP13、MP15、MP2 | Cortex-A7 或 A35 | 4.3 至 10 吋 | 晶片：MP135 約 US$6.6（500 顆）、MP257 約 US$20.7（360 至 500 顆）（[ST eStore](https://estore.st.com/en/stm32mp257cai3-cpn.html)）；模組：MP13 約 US$19（2023）、MP157 約 €27.6 至 29.95、MP257 約 US$43.8 | 和現有 STM32 同一家原廠，文件好 |
| TI AM62x | 1 至 4 核 Cortex-A53，PowerVR | 7 至 15 吋 | 四核 AM6254 晶片約 US$19.5（357 顆）（[DigiKey](https://www.digikey.com/en/products/detail/texas-instruments/AM6254ATCGHAALW/19236195)）；模組 2023 上市價 US$29 至 33 起，單顆零售約 €59 | 長期供貨；NOVAKON 現用的 AM335x 是前一代 |
| Allwinner T527 | 8 核 Cortex-A55，Mali-G57 | 10 至 15 吋 | MYIR 模組約 US$43 | 原廠 BSP，主線支援進行中 |
| Intel N100、N97 | x86 | 10 至 21 吋工業電腦 | 板子約 US$90 至 200 | 高階與 Windows |

（推論）建議先選兩條線：低價線 RK3506 或 T113（4.3 至 7 吋），主力線 RK3568 或 i.MX93 擇一（7 至 10 吋）。STM32MP2 列入觀察，因為可以延續「和現有 MCU 同一家原廠」的故事。執行環境保持可移植，換 SoM 只要多 0.5 至 1.5 人月做系統映像。

## 五、工時與時程（全部是推論）

一位熟悉嵌入式 Linux 的工程師全職，大部分程式由 Claude Code 撰寫、審查與測試；買現成 SoM，不自己設計主板。

| # | 工作 | 人月 |
| --- | --- | --- |
| 1 | Linux 執行環境骨架：移植 `firmware/common`、LVGL DRM 與 evdev、systemd、看門狗、記錄 | 0.5 至 1 |
| 2 | 專案模組流程：ABI、內附交叉編譯器、專案格式與簽章、熱替換、回復 | 1 至 2 |
| 3 | 標籤搬到資料檔；多型別標籤資料庫、多連線多裝置、執行緒模型、保持 MCU 相容 | 1.5 至 3 |
| 4 | 設計器：MPU 目標、設備搜尋與下載、多裝置標籤表 | 1 至 2 |
| 5 | 協定：Modbus TCP 0.5、S7 0.5 至 1、MC 0.5 至 1、FINS 0.5 至 1、EtherNet/IP 0.5 至 1、MQTT 0.5、OPC UA 1 至 2 | 4 至 7.5 |
| 6 至 9 | 警報、歷史趨勢與記錄、配方、使用者與稽核 | 3.5 至 5.5 |
| 10 | 遠端：VNC；選配網頁檢視 | 0.5 至 3 |
| 11 | 第一款 SoM 的系統映像：Yocto 或 Buildroot、A/B、RAUC、開機調校、出廠設定 | 2 至 4（每多一款 SoM 加 0.5 至 1.5） |
| 12 | 測試：CI 裡的 PLC 模擬器、長時間測試、斷電測試、真實 PLC 互通 | 1.5 至 3 |
| 13 | Windows 執行環境與 PC 模擬器打包 | 0.5 至 1 |
| 14 | 文件、授權與 SBOM、CRA 流程、設備授權金鑰 | 1 至 2 |
| | **MVP**（1 至 4、Modbus 加一種 PLC、6 至 9 基本版、一款 SoM、部分測試） | **9 至 14** |
| | **接近 iFace 的完整產品** | **22 至 38** |

- **Claude Code 加速得多的部分：** 把 C 移植到 POSIX、照規格寫協定封包、沿用現有 React 模式的編輯器介面、SQLite 結構、測試、Yocto 與 Buildroot 配方、文件。這些部分約可快 2 至 3 倍。
- **加速不了的部分：**
  - 真實 PLC 互通：Siemens S7-1200/1500、Mitsubishi FX5U/iQ-R、Omron NX/NJ、Rockwell CompactLogix，每個系列要幾週，加一個 US$1 萬至 3 萬的實驗室。
  - 認證：CE、UL/cUL 61010-2-201、FCC、ODVA；CRA 弱點通報 2026-09-11 起適用，2027-12-11 全面適用（見 [16 號研究](16-global-evidence.zh-TW.md)）。實驗室排程以月計。
  - BSP 與硬體驗證：面板時序、觸控調校、開機時間、1,000 小時長測、eMMC 斷電。
  - 客戶試用：機械廠換 HMI 品牌很慢，第一批案子會暴露缺少的功能。

**時程換算：**

| 人力 | 第一台能賣的產品 | 接近 iFace 的完整產品 |
| --- | --- | --- |
| 1 人 | 約 12 至 18 個月（MVP 9 至 14 人月，加互通測試與認證的等待） | 約 2 至 3 年 |
| 2 至 3 人（執行環境與系統映像、協定與測試、設計器） | 約 6 至 9 個月 | 約 12 至 18 個月 |

硬體打樣、認證與軟體並行。EDT 自己有面板，可以省掉面板選型與供貨談判（見 [17 號研究](17-novakon-hardware-model.zh-TW.md)）。

## 六、對 EHS 的含意（推論）

| 決策 | 建議 |
| --- | --- |
| 架構 | 固定執行環境加預編模組，(c)；MCU 執行環境保留為同一套核心的精簡版，讓一個設計器同時支援 MCU 與 MPU |
| 第一個平台 | Linux，RK3568 或 i.MX93 擇一；以第一批目標客戶的地區決定（亞洲選 RK3568，歐美選 i.MX93） |
| Windows | 只做軟體：PC 模擬器加工業電腦執行環境授權 |
| 協定順序 | Modbus RTU/TCP、Siemens S7、Mitsubishi MC、Omron FINS、EtherNet/IP，再到 OPC UA、MQTT；順序以代理商訪談結果調整 |
| 差異化 | 低記憶體 LVGL、8 秒內開機、A/B 簽章更新與 SBOM、非 root 執行、瀏覽器檢視同一套畫面、同時有 MCU 低價機型 |
| 授權 | 自家硬體免費；第三方工業電腦與 EDT 面板模組客戶收執行環境授權（同業慣例）。目前 EULA 限 EDT 硬體（[01 號研究](01-codebase-coupling.zh-TW.md)），要賣給第三方硬體時要先改 |
| 先做的實驗 | 在選定 SoM 上，用現有範例專案量 1024×600 與 1280×800 的軟體繪圖效能；兩週的 WASM 上機實驗；量競品（NOVAKON、Weintek cMT、Delta DOP）實際開機時間，作為行銷目標 |

## 查證結果

2026-11-20 針對初版的待驗證項目做一輪查證，紀錄在 [notes/18-mpu-runtime](notes/18-mpu-runtime/) 的 `verification_platforms_licences.md`，價格部分在 [notes/17-novakon-hardware-model](notes/17-novakon-hardware-model/) 的 `verification_prices_components.md`。

| 項目 | 結果 | 一句話 |
| --- | --- | --- |
| Siemens Unified Comfort 作業系統 | 確認 | Siemens 移轉指南：Unified Comfort 是 Linux，舊款 Comfort 是 Windows CE；CPU 仍未公開 |
| Pro-face GP4000 | 新增 | 修補的都是 VxWorks 弱點，推論跑 VxWorks；CPU 不明 |
| Mitsubishi GOT2000 | 仍未驗證 | 推論是即時作業系統，CPU 不明 |
| Omron NA5、C-more EA9 | 仍未驗證 | NA5 授權清單頁面被擋；EA9 只知道是 800 MHz CPU |
| Samkoon、Kinco | 確認（Samkoon）／部分（Kinco） | Samkoon 選型手冊寫 Linux、雙核 Cortex-A7；Kinco 型錄確認 ARM 800 MHz，Linux 只有代理商說法 |
| 開源授權 | 確認 | 約 20 個元件逐一對照官方授權檔；WAMR 多了 LLVM 例外，Qt 部分模組只有 GPL |
| LVGL XML 授權 | **更正** | 9.5 起移除 XML 引擎、執行期載入要付費；XML 格式也禁止第三方編輯器未經許可使用。**第三節已改寫** |
| CRA 日期 | 確認 | 弱點通報 2026-09-11 起，全面適用 2027-12-11 |
| Windows CE | 更正（日期） | WEC 2013 主流支援 2018-10-09 結束、延伸支援 2023-10-10 結束；WEC 7 延伸支援 2021-04-13 結束 |
| Windows IoT 價格 | **更正** | Entry 約 US$40 至 65、Value 約 US$80 至 130；刪除 US$295 的數字 |
| 晶片與模組價格 | 更正／新增 | STM32MP2、AM62x、RK3566、T527 有了實際報價；大多是上市價，沒反映 2026 年記憶體漲價 |
| 競品開機時間 | 仍未驗證 | 只有一個沒說明方法的部落格數字；要自己實測 |

**對核心結論的影響：** 沒有推翻任何結論。LVGL XML 的更正反而加強了「用自己的格式與預編模組，不用 LVGL XML 載入器」的建議。Windows 授權比初版估得便宜，但「自有硬體用 Linux」的判斷不變。

## 待驗證

- Siemens Unified Comfort 與 Pro-face GP4000 的 CPU；Mitsubishi GOT2000 的 CPU 與作業系統；Omron NA5、C-more EA9 的作業系統；Kinco 的作業系統只有代理商說法。
- 競品實測開機時間：要自己買機量測。
- 模組報價大多是上市價，沒有反映 2026 年記憶體漲價；Windows IoT 的 OEM 量價與 Arm 版價格不公開。
- 工時：全部是推論，要在選定 SoM 上完成第一個月的骨架後重新校正。

## 來源

### 各廠執行環境

- Microsoft：[Windows CE 移轉 FAQ](https://learn.microsoft.com/windows/iot/windows_ce/windows-ce-migration-faq)、[IoT Enterprise 授權](https://learn.microsoft.com/windows/iot/iot-enterprise/commercialization/licensing)；[The Register（WEC 2013 結束支援）](https://www.theregister.com/software/2023/10/30/windows-ce-reaches-end-of-life-if-not-end-of-sales/1358506)
- Windows IoT 價格：[CDW Entry](https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-entry-license-1-license/7983139)、[CDW Value](https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-value-license-1-license/7983137)、[Arrow FAQ](https://www.arrow.com/ais/msembedded/resources/windows-iot-enterprise-ltsc-frequently-asked-questions-faqs)
- 趨勢：[ARC（HMI 軟體市場）](https://arcweb.com/press/hmi-software-market-driven-demand-innovations-sustainability)、[Meticulous Research](https://meticulousresearch.com/reports/human-machine-interface-hmi-market-5805)、[Scalosoft（CE 之後的轉移）](https://www.scalosoft.com/blog/beyond-windows-ce-2013-transition-strategies-for-end-of-life/)
- 查證新增：[Siemens Unified Comfort 移轉指南](https://support.industry.siemens.com/cs/attachments/109768002/109768002_Guide_to_Unified_Comfort_V10_DOC_en.pdf)、[Pro-face GP-Pro EX 4.09 版本說明](https://www.proface.tech/otasuke/download/update/proex/proex/v409/Readme_E_GPProEX_v409450.txt)、[Mitsubishi PSIRT 2020-005](https://www.mitsubishielectric.com/psirt/vulnerability/pdf/2020-005_en.pdf)、[Samkoon 選型手冊](https://www.rusavtomatika.com/manuals/Product-Selection-Guide-20221026.pdf)、[Kinco GL100 型錄](https://www.gotronik.pl/images/Flyer_GL100_EN%20202106.pdf)、[Microsoft WEC 2013 生命週期](https://learn.microsoft.com/en-us/lifecycle/products/windows-embedded-compact-2013)、[歐盟 CRA 摘要](https://digital-strategy.ec.europa.eu/en/policies/cra-summary)、[開機時間部落格](https://industrialmonitordirect.com/blogs/knowledgebase/micro820-hmi-boot-times-panelview-vs-red-lion-vs-maple-vs-weinview)
- 各廠資料表、手冊與代理商頁面的完整連結，在 `notes/18-mpu-runtime/hmi_runtime_platforms.md` 各廠商小節。

### EHS 移植

- EHS 原始碼（唯讀）：`docs/preview-ladder.md`、`docs/logic-node-taxonomy.md`、`src/codegen/`、`src/codegen/logicTrace.ts`、`src/ehsp/projectCodec.ts`、`src/types/connections.ts`、`firmware/common/include/hmi_runtime.h`、`hmi_platform.h`、`hmi_transport.h`、`vite-plugin-emulator.ts`、`wasm/src/ui_from_json.c`
- LVGL：[9.5 嵌入式 Linux](https://docs.lvgl.io/9.5/integration/embedded_linux/index.html)、[9.5 DRM 驅動](https://docs.lvgl.io/9.5/integration/embedded_linux/drivers/drm.html)、[XML 授權（9.3 文件）](https://lvgl.io/docs/open/9.3/details/auxiliary-modules/xml/license)、[LVGL 移除 XML 引擎公告](https://lvgl.io/blog/announcement-lv_xml-removal-from-v9-5)、[LVGL issue #9663](https://github.com/lvgl/lvgl/issues/9663)
- 開機時間：[Forlinx i.MX93](https://www.forlinx.net/article_view_652.html)、[Elektor i.MX93](https://www.elektormagazine.com/labs/how-we-reduced-linux-boot-time-on-imx93-to-18s)、[NXP i.MX6ULZ](https://community.nxp.com/t5/i-MX-Processors-Knowledge-Base/i-MX-6ULZ-How-to-boot-Linux-in-under-2s/ta-p/2196625)、[ST 開機最佳化](https://wiki.st.com/stm32mpu/wiki/How_to_optimize_the_boot_time)
- 價格（查證新增）：[ST eStore STM32MP257](https://estore.st.com/en/stm32mp257cai3-cpn.html)、[DigiKey AM6254](https://www.digikey.com/en/products/detail/texas-instruments/AM6254ATCGHAALW/19236195)、[Dell Windows IoT Entry](https://www.dell.com/en-us/shop/windows-11-iot-enterprise-ltsc-2024-entry-upgrade-solution-for-dell-thin-clients/apd/634-czbg/software)、[Advantech Windows IoT](https://buyemt.advantech.com/Peripherals-Modules/Embedded-Peripherals-Microsoft-Windows-Embedded-Windows-10-IoT-Licenses/EMT_30970.products.htm)
- 硬體：[Forlinx FET3568-C](https://www.digikey.com.mx/en/products/detail/forlinx-embedded/FORLINX-FET3568-C-202GSE16GCA10/21810672)、[Variscite DART-MX93](https://shop.variscite.com/product/system-on-module/dart-mx93-system-on-module/)、[Toradex Verdin iMX8M Mini](https://www.toradex.com/computer-on-modules/verdin-arm-family/nxp-imx-8m-mini)、[CNX T113-S3](https://cnx-software.com/2023/05/11/14-allwinner-t113-s3-cpu-module-128mb-ram-256mb-nand-flash-4gb-emmc-flash)、[CNX RK3506J](https://www.cnx-software.com/2025/08/15/armsom-cm1-a-15-compute-module-sbc-hybrid-powered-by-rockchip-rk3506j-soc/)、[CNX STM32MP25](https://cnx-software.com/2024/09/26/myir-tech-myc-ld25x-a-compact-stm32mp25-system-on-module-running-debian-12/)、[CNX AM62](https://www.cnx-software.com/2023/02/07/var-som-am62-system-on-module-features-ti-am625x-cortex-a53-m4-soc/)
- 函式庫：[libplctag](https://libplctag.github.io)、[OpENer](https://github.com/EIPStackGroup/OpENer)、[snap7 授權](https://docs.quanser.com/quarc/documentation/snap7_license.html)、[neatvnc](https://github.com/any1/neatvnc)、[Apache PLC4X SLMP](https://plc4x.apache.org/plc4x/pre-release/users/protocols/slmp.html)

研究筆記在 [notes/18-mpu-runtime](notes/18-mpu-runtime/)：`hmi_runtime_platforms.md`、`ehs_port_to_mpu.md`；查證紀錄：`verification_platforms_licences.md`。
