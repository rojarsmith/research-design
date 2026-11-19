# EDT HMI Studio 分拆為純軟公司並轉型 IoT 開發環境的可行性研究

研究日期：2026 年 11 月 19 日，Asia/Taipei。完整研究以繁體中文提供；[English overview](README.md)。

## 結論

> **整合結論**：EHS 要成為有競爭力的收費軟體，必須從「UI 編輯器」轉成「讓整台連網裝置做得出來、出貨後還管得住」的工具。具體改法、收費結構與執行順序見 [EHS 收費化路線圖](research/10-ehs-paid-product-roadmap.zh-TW.md)。

**可以轉，但不建議做成「通用 IoT IDE」。** 建議轉成「有螢幕的連網裝置」的 Low Code 開發環境：以 VS Code 付費擴充為主要付費介面，保留現有 No Code 桌面版給 OEM／ODM 客戶。

1. **技術上綁 EDT 的地方很少。** 程式碼沒有授權金鑰、裝置檢查或 EDT 專屬晶片。三塊實體板中，兩塊是任何人都買得到的 ST Discovery 板。真正綁 EDT 的是**法律與品牌**：EULA、程式內授權文字限定「只能用於 EDT 供應的顯示模組」，另外產品名、檔案格式 ID、MIME 類型與 logo 也都帶 EDT。真正的技術鎖定是 **ST 工具鏈、Windows、LVGL**。詳見[程式碼耦合盤點](research/01-codebase-coupling.zh-TW.md)。
2. **離 IoT IDE 還缺一大段。**
   - 裝置端沒有 TCP/IP、MQTT、HTTP、Wi-Fi、BLE，也沒有 OTA。
   - 雲端沒有帳號、裝置管理與計費。
   - 執行模型是裸機 super-loop，**沒有多執行緒**。
   - 只支援 STM32。
   - 開發者只有一位工程師。
3. **「通用 IoT IDE」是紅海，而且對手的工具多半免費。**
   - 晶片商把 IDE 當作賣晶片的成本，ST、NXP、Microchip、Nordic、Espressif 的 VS Code 擴充都免費。
   - Arduino 已被 Qualcomm 收購，推出 App Lab；Espressif 的 RainMaker 雲端後端已開源。
   - 一家純軟新公司要正面競爭，資金與人力都不成比例。詳見[全球競品比較](research/02-competitors.zh-TW.md)。
4. **仍有一個空隙：有螢幕的連網裝置。** EV 充電樁、家電、工控面板、醫療周邊都同時需要 UI、通訊、資料上雲與 OTA，現有工具各管一段：
   - LVGL Pro、SquareLine、TouchGFX 只管 UI。
   - Blynk、ThingsBoard、Arduino Cloud 只管雲端與手機 App。
   - Tuya 綁自家雲。
   - Weintek 綁自家 HMI 硬體。

   - MicroEJ 覆蓋最完整，但它是 Pro Code 加虛擬機平台，走大型 OEM 的企業銷售。

   以 Low Code 把「裝置畫面 + 裝置邏輯 + 通訊標籤 + 上雲 + OTA」用同一個標籤模型串起來、產出可帶走的純 C、且不綁硬體或雲的工具，市面上很少。這也是 EHS 現有 Link → Device → Tag 模型、Logic 轉 C、WASM 模擬器可以延伸的方向。
5. **VS Code 付費擴充是比獨立 IDE 更好的主要付費介面，但不能是唯一形態。**
   - 好處：
     - 嵌入式開發者已在 VS Code。
     - 多執行緒除錯可借用既有生態（Cortex-Debug、SEGGER SystemView、Percepio Tracealyzer、Nordic Thread Viewer），不用自己做 IDE 外殼。
     - EHS 本來就是 Web UI，可以放進 VS Code Webview。
   - 限制：
     - Marketplace 沒有內建付款，必須自建授權。
     - Microsoft 條款限定 Marketplace 只供 Microsoft 產品使用，Cursor 等分支要另走 Open VSX。
     - LVGL 原廠的付費工具 LVGL Pro 定位正是專業 UI 開發：Developer 方案 US$3,990／席／年，另有按產品計價的 US$20,000 方案；Renesas 客戶免費。
     - 原本 No Code 的目標使用者（非程式背景的機構、產品工程師）不會用 VS Code。
   - 詳見[方向比較](#三個方向比較)。
7. **面對 LVGL Pro，付費價值不能放在 UI 編輯器本身。** LVGL Pro 已涵蓋 XML、資料綁定、Figma、UI 測試、CLI、AI agent，但官方頁面沒有列出通訊協定、應用邏輯流程、多執行緒、OTA、板級建置與可寄送的 Demo。EHS 該收費的是「整台連網裝置一天跑起來」、「在設計期擋掉並行 bug」、「給客戶看的 Demo」與「CRA 之後的 OTA 與 SBOM」，並評估與 LVGL XML 互通，採互補而非正面衝突。詳見 [LVGL Pro 分析與付費價值](research/04-lvgl-pro-and-paid-value.zh-TW.md)。
8. **MicroEJ 證明「完整的 MCU 裝置研發平台」可以收費，但它走的是另一條路。** 它是以 MEJ32 虛擬機為核心的 Pro Code 平台，具備多應用沙箱、完整網路函式庫、應用層 OTA 與 Virtual Device 模擬；以多年期合約賣給大型 OEM，價格不公開。EHS 應借用它的虛擬裝置、晶片商與垂直市場合作、出貨後更新等概念，但不做 VM、不只做企業銷售，站在「Low Code、純 C 輸出、公開價格、中小型 OEM」的位置。詳見 [MicroEJ 比較](research/05-microej-comparison.zh-TW.md)。
9. **MCU 研發最痛的地方，正好是 EHS 可以收費的地方。** 綜合多份調查，排在前面的是除錯（尤其並行與時序 bug）、複雜度與時程、連網、出貨後的 OTA 與現場故障、資安與 CRA。並行 bug、硬體還沒到就要測試、協定整合，EHS 已有或已規劃對應；現場可觀測性則該交給專業服務。詳見 [MCU 研發困擾排行](research/08-mcu-development-pain-points.zh-TW.md)。
10. **其他付費 MCU 工具的收費模式，都是「工具便宜或免費，執行環境或量產收費」。** Moddable（JavaScript）每台 US$0.50、Slint 每台約 US$1 起、Quantum Leaps 的 QM 工具免費但 QP 框架收商業授權、Toit 語言開源但收雲端 OTA 費用。QP 的 Active Object 模型也可直接作為 EHS 多執行緒設計的理論基礎。詳見 [動態語言 MCU 產品](research/06-dynamic-language-mcu-products.zh-TW.md) 與 [C/C++ RAD 工具](research/07-c-cpp-rad-tools.zh-TW.md)。
11. **新架構提案：單元匯流排加可程式化交換中心。** 不受 EHS 現有設計限制，在 MCU 上跑一條軟體匯流排：觸控 GUI、實體按鈕中斷、感測器、Modbus、MQTT 都是匯流排上的設備單元，行為由可程式化的交換中心以 Low Code 規則決定。判斷是值得做：有 Zephyr zbus、PX4 uORB、QP、ESPHome 等先例，又直接解決並行除錯、硬體未到、晶片綁定等痛點；差異化在 IDE（單元描述檔、整機模擬、匯流排錄製重播、跨晶片），風險是即時性、中心瓶頸與記憶體，需先以最小原型量測。詳見 [單元匯流排架構提案](research/09-mcu-unit-bus-architecture.zh-TW.md)。
12. **MCU 上沒有商業產品能熱重載 C 程式碼，這是空缺。** Toit、MicroEJ、Moddable、LVGL Pro 的熱重載或動態更新，都是 C 韌體固定、只換 bytecode、腳本、XML 或容器。EHS 若讓畫面、規則與 Tag 參數在真機上存檔就生效，並和整機模擬、錄製重播合成同一套開發迴圈，就是沒有對手覆蓋的收費功能。詳見 [MCU 熱重載](research/11-mcu-hot-reload.zh-TW.md)。
13. **分拆的前提是 EDT 的決策，不是技術。** 現有文件寫明 EHS 是 EDT 自主 IP，EULA 寫明 EDT 擁有全部權利，也沒有任何文件討論過分拆、IP 移轉或授權回用。這必須由 EDT 經營層與法務決定。本研究只提供決策材料，不構成以個人名義另起爐灶的建議。

## 目前狀態

| 面向 | 現況（2026-11） | 對 IoT IDE 的意義 |
| --- | --- | --- |
| UI 編輯 | 22 個 LVGL 元件、多畫面、動畫、多語系、影片、AI 助理 | 已是強項，可直接沿用 |
| 邏輯 | 節點圖，編譯成 C（`ui_logic.c`）；裝置端沒有 VM | 可延伸為裝置應用邏輯，但要加入並行與非同步模型 |
| 通訊 | Modbus RTU（主端）、UART 指令協定；CAN 僅 loopback；每專案上限 1 個裝置 | 需加入 MQTT／HTTP／Modbus TCP、多裝置，以及網路傳輸層 |
| 標籤模型 | Link → Device → Tag，Logic 只透過 Read／Write Tag 節點碰線路，圖本身與協定無關 | **這是 IoT 化最好的基礎**：同一個 Tag 可同時接上畫面、邏輯、雲端 |
| 目標硬體 | STM32F746G-DISCO、STM32H747I-DISCO、EDT EVK043027B（STM32U599），外加 Virtual Model | 需多晶片：ESP32-S3（Wi-Fi）、NXP、Renesas，最好經 Zephyr |
| 執行模型 | 裸機 `for(;;) lv_timer_handler()`，刻意未用 EDT BSP 的 FreeRTOS | 多執行緒需要從頭設計 |
| 建置與燒錄 | STM32CubeCLT、PowerShell 腳本、STM32_Programmer_CLI | 綁 ST 與 Windows，需抽象化 |
| 預覽 | LVGL 編譯成 WASM 的模擬器、可寄送的單檔 Demo | 強項，可擴充成「整台裝置連同模擬雲端」的模擬 |
| 雲端與商業 | 只有 AI 助理；雲端圖庫、帳號、儲值後台、自動更新都還在研究階段 | 帳號、授權、計費要自建 |
| 授權 | EULA 與程式內授權限定 EDT 硬體；repo 根目錄沒有 LICENSE；README 卻標示 MIT（自相矛盾） | 分拆前必須先釐清 |
| 規模 | `src` 約 59 萬行（約 33 萬行是測試）、`server` 2.1 萬、`firmware` 2.9 萬；2,268 個 commit，全部出自一人 | 產品成熟度高，但「巴士因子」為 1 |

## 「IoT IDE」該怎麼定義

「IoT 整合開發環境」範圍太大。Arduino App Lab、PlatformIO、ESP-IDF、nRF Connect 都自稱 IoT 開發環境，它們的核心是**寫韌體**。EHS 的核心是**不寫或少寫程式就做出完整的裝置應用**，兩者定位不同。

建議的定位：

> **給有螢幕的連網裝置使用的 Low Code 應用開發環境**：在同一個專案裡定義裝置畫面、裝置邏輯、現場通訊（Modbus／CAN／UART）、上雲（MQTT／HTTP）、遠端更新（OTA），可在 PC 上整機模擬，並產生可帶走、可在任何主流 MCU 或 RTOS 上建置的 C 程式碼。

這個定位刻意**不做**：

- 通用韌體 IDE：編輯器、編譯器、除錯器交給 VS Code 和晶片商的擴充。
- 雲端 IoT 平台：只提供連接器，接 AWS IoT、Azure、ThingsBoard、自架 MQTT broker 與 Bitdove OTA（hawkBit），不自己經營雲。
- 自有硬體。

## 新增需求：可接受 Low Code、強大的多執行緒與好用的 IDE 能力

原產品原則是「主路徑不寫程式」。新產品放寬為 Low Code 後，程式碼可以成為**正式的一等公民**，不再只是逃生口。以下是建議的設計方向。

### 執行與並行模型

LVGL 本身不是執行緒安全的。v9 起有 `lv_lock()`／`lv_unlock()`，要在 `LV_USE_OS` 不為 `LV_OS_NONE` 時才生效，並提供 `lv_os` 抽象層（thread、mutex、sleep）。所以多執行緒的關鍵是**讓使用者不必自己管鎖**：

| 元素 | 設計 | 理由 |
| --- | --- | --- |
| UI Task | 唯一擁有 LVGL 的執行緒，其他 Task 不直接呼叫 LVGL | 從結構上避免 LVGL 競態，不靠使用者記得加鎖 |
| Task 節點 | 在專案中以視覺方式宣告 Task（名稱、優先權、堆疊、週期或事件驅動），每個 Task 有自己的 Logic 圖或程式碼檔，並以帶事件佇列的 Active Object 實作（參考 [Quantum Leaps QP](research/07-c-cpp-rad-tools.zh-TW.md)） | 讓並行在設計圖上可見；Active Object 有成熟理論，容易向工程師解釋 |
| Tag Store | 所有跨 Task 資料都經過 Tag。Tag 寫入以訊息佇列送達擁有者，讀取取快照；UI 端用類似 `lv_subject` 的觀察者更新元件 | 沿用現有 Tag 模型，把「共用變數」變成安全的發布／訂閱 |
| 通訊 Task | Modbus、CAN、MQTT 各自是一個 Task（或由連線設定自動產生），逾時與重試由產生器處理 | 慢速 I/O 不會卡住畫面 |
| 目標 OS | 第一目標選 **Zephyr**：多板支援、有網路堆疊、MCUboot OTA；第二目標選 FreeRTOS：既有客戶多；裸機 super-loop 保留給小型板 | Zephyr 一次解決多晶片與網路，FreeRTOS 滿足市場慣性 |
| 模擬 | 在 PC／WASM 上以**確定性排程器**模擬多個 Task，可重播、可單步 | 一般 RTOS 除錯最痛的是無法重現，可重播是差異化點 |

### IDE 能力

| 能力 | 做法 | 自建或借用 |
| --- | --- | --- |
| Task 與佇列總覽 | 專案視圖顯示 Task、優先權、Tag 讀寫關係圖（誰寫、誰讀），並在設計期偵測「兩個 Task 寫同一個 Tag」 | 自建（這是差異化） |
| 堆疊與時序預估 | 由產生的程式碼估算最壞堆疊與週期負載，延伸現有的 `panelBudget` | 自建 |
| 執行期追蹤 | 產生的程式碼內建 SystemView 或 Tracealyzer 的追蹤掛點，在 IDE 中把 Task 切換對應回 Logic 節點 | 借用 SEGGER／Percepio 的工具，自建對應層 |
| 執行緒感知除錯 | 借用 Cortex-Debug 與 J-Link RTOS plugin（Nordic Thread Viewer 也是這樣做的） | 借用 |
| Logic 節點中斷點 | 現有 Logic 單步除錯延伸到真機：節點對應到產生的 C 行號 | 自建 |
| 程式碼與圖互通 | 每個 Logic 圖可「展開為 C」並保留雙向對應；C 程式碼節點有型別化的輸入與輸出埠 | 自建 |
| 腳本選項（後期） | 評估裝置端嵌入 Lua、MicroPython 或 Berry 作為熱更新邏輯層；此 repo 的 minilang 研究可作 VM 設計參考 | 後期評估 |

## 三個方向比較

| 評估項目 | A：獨立 IoT IDE（桌面版擴充） | B：VS Code 付費擴充 | C：混合（建議） |
| --- | --- | --- | --- |
| 形態 | 延伸現有 .NET 加 WebView 桌面殼，自帶編輯器、建置、除錯 | 把現有 React 編輯器放進 VS Code Webview，建置與除錯交給 VS Code 生態 | 共用引擎（專案格式、codegen、模擬器、CLI），提供 VS Code 擴充與 No Code 桌面版兩個殼 |
| 目標使用者 | 非程式背景到中階工程師 | 韌體與嵌入式工程師 | 兩者都涵蓋：工程師付費、OEM 客戶走通路 |
| 多執行緒與除錯 | 要自己做除錯器 UI、GDB、RTOS awareness，最貴 | 直接用既有除錯生態，只做對應層 | 同 B |
| 與現有程式碼的落差 | 小 | 中：需拆出本機服務、改用 VS Code API 處理檔案、Webview 通訊與狀態 | 中：先拆引擎與 CLI，其餘逐步完成 |
| 收費 | 授權金鑰或訂閱，自建授權 | 同樣要自建授權（Marketplace 沒有付款機制，GitLens Pro 是外部訂閱）；要額外處理 Open VSX | 擴充按席位訂閱，No Code 版走 OEM 或通路授權 |
| 直接競品 | MicroEJ、Arduino App Lab、晶片商 IDE、Weintek EasyBuilder Pro | LVGL Pro（類 VS Code 編輯器，Renesas 文件提及 VS Code 擴充）、SquareLine、晶片商的免費擴充 | 同 B，但多了 IoT 串接這個差異 |
| 風險 | 範圍最大，一人團隊做不完；正面對上免費工具 | 原本的 No Code 定位在 VS Code 裡會被稀釋；平台條款可能變動 | 兩個殼加倍維護，要嚴格控制共用核心 |
| 判斷 | 不建議作為主線 | 適合作為主要付費介面 | **建議** |

### 為什麼不是「只做 VS Code 擴充」

- **付費者與使用者不同。** EHS 的價值主張是讓不寫程式的人一天做出 HMI，這群人不會開 VS Code。VS Code 是工程師的地盤，而工程師最能接受「免費晶片商工具加 LVGL 開源」。
- **擴充的護城河薄。** LVGL 原廠的 LVGL Pro 已有類 VS Code 的編輯器、XML 格式、CLI 與 AI agent，若只比 UI 編輯器，等於和 LVGL 原廠正面競爭。差異化必須來自 **IoT 串接、多執行緒模型、整機模擬**，而不是 UI 編輯本身。
- **平台風險。** 2025 年 4 月起，Microsoft 的 C/C++ 擴充已拒絕在 Cursor 等分支執行。自家擴充雖不受此限，但 Marketplace 條款的使用範圍會影響發行策略，所以要同時上架 Open VSX。

### 為什麼不是「只做獨立 IDE」

- 多執行緒除錯、編輯器、Git、AI coding 這些「好用的 IDE 能力」，VS Code 生態已經免費提供。自己重做既貴又追不上。
- 嵌入式產業正集體搬到 VS Code。Microchip、ST、NXP、Nordic、Espressif、Arm Keil 都已推出 VS Code 擴充，獨立 IDE 會被視為又一個要安裝的孤島。

## 分拆的前提條件

| 項目 | 目前狀態 | 分拆前要完成 |
| --- | --- | --- |
| IP 歸屬 | EULA §1、§3 寫 EDT 擁有全部權利；內部 FAQ B3 稱「EDT 完全自主的 IP」 | EDT 董事會決議：IP 讓與、獨家授權，或 EDT 作價入股新公司 |
| EDT 授權回用 | 無 | 新公司授權 EDT 以 OEM 方式繼續免費或優惠使用，保住 EDT 「免費軟體帶動模組銷售」的模式 |
| EDT BSP 程式碼 | EVK043027B 觸控與 BSP 是 BSD-3-Clause（© 2023 EDT），含不得背書條款 | BSD 本身可再散布，但仍需書面確認，並移除 EDT 名稱的背書用法 |
| 品牌 | 產品名、`EDT_HMI_*` 環境變數、`com.edt.hmistudio.project` 格式 ID、MIME 類型、logo | 改名；保留讀取舊格式 ID 的遷移路徑（現有 `src/project/format/` 已有版本遷移機制） |
| 授權文字 | EULA 草稿未經律師審閱；README 寫 MIT | 新 EULA 去除「僅限 EDT 硬體」；決定是否開放核心原始碼 |
| 人員 | 唯一開發者是 EDT 員工（研發 PM） | 雇傭合約、競業條款、轉任或借調安排須由法務處理 |
| Factory Mode | 密碼寫在前端 bundle | 改為正式授權層級，不要靠隱藏密碼 |
| 創業資源 | Claude Startups 等計畫要求真實新公司 | 新公司成立後才可申請，不可以 EDT 產品名義冒充新創 |

## 建議路線與檢核點

> 下列人力與時間是假設，參考本 repo [RAD 工具研究](../rad-tools-research/README.zh-TW.md)的「聚焦商業產品：4 至 8 人、9 至 18 個月」區間，不是實測。

| 階段 | 期間（假設） | 內容 | 過關條件 |
| --- | --- | --- | --- |
| 0. 決策 | 1 至 3 個月 | EDT 決定分拆形式與 IP 安排；完成 10 至 15 家目標客戶訪談（EV 充電、家電、工控），驗證願付價格 | 有書面 IP 協議；至少 3 家願意簽付費試用意向 |
| 1. 去 EDT 化與引擎拆分 | 2 至 3 個月 | 改名；新 EULA；把專案格式、codegen、模擬器拆成無 UI 的核心與 CLI；建置流程從 PowerShell 改為跨平台 | 同一份專案可以用 CLI 在 Linux CI 上建出韌體 |
| 2. 多執行緒與 Zephyr 目標 | 3 至 4 個月 | Task、Tag Store 與 UI Task 模型；Zephyr 目標（先 STM32 加 ESP32-S3）；確定性多 Task 模擬器 | 洗衣機或充電樁示範：UI、Modbus、感測三個 Task 在真機與模擬器上行為一致 |
| 3. 連網與 OTA | 2 至 3 個月 | MQTT／HTTP 連接器（Tag 對應 topic）、裝置憑證、MCUboot 加上 hawkBit（Bitdove）OTA 串接 | 示範機從雲端改參數、推新韌體，失敗可回滾 |
| 4. VS Code 擴充 | 2 至 3 個月（可與 3 並行） | Webview 編輯器、Task 視圖、追蹤對應、授權伺服器；同時上架 Marketplace 與 Open VSX | 10 位外部工程師完成試用任務，NPS 與留存可量測 |
| 5. 商業化 | 持續 | 席位訂閱、OEM 授權、導入服務；EDT 作為第一個 OEM 客戶 | 12 個月內有可預測的經常性收入 |

### 定價參考（需實際驗證）

| 產品 | 價格（來源與日期見[來源清單](research/03-sources-and-open-questions.zh-TW.md)） |
| --- | --- |
| LVGL Pro | Starter US$990／席／年（最多 2 席）、Developer US$3,990／席／年、Product US$20,000／產品（5 年、5 席）；Community 免費但限非商業 |
| Embedded Wizard | 約 €199／席／月 |
| SquareLine | 個人方案 Free、US$9、US$19／月 |
| Blynk | Prototype US$99／月；Production US$199 至 2,099／月（按裝置數） |
| Ignition | Perspective 模組約 US$11,225（伺服器授權） |
| MicroEJ | 多年期合約（CAPEX），價格不公開 |
| Arduino Cloud | Maker 約 US$72／年（第三方資料） |

**初步建議**：工程師席位 US$790 至 1,490／年，低於 LVGL Pro Developer，高於 SquareLine，用 IoT 與多執行緒能力支撐；量產授權 US$4,000 至 8,000／產品，遠低於 LVGL Pro 的 US$20,000；OEM 與通路白標另議，導入服務與客製連接器另收。完整結構見 [LVGL Pro 分析](research/04-lvgl-pro-and-paid-value.zh-TW.md#建議的定價結構待訪談驗證)。正式數字要等第 0 階段的訪談結果。

## 主要風險

| 風險 | 說明 | 緩解 |
| --- | --- | --- |
| 免費替代品 | 晶片商工具加 LVGL 開源已足夠讓工程師自己做 | 鎖定「UI 加連網加 OTA 一次完成」的整合價值，不比單點功能 |
| LVGL 原廠 | LVGL Pro 已有 Subject 資料綁定，v2.0 內容尚未查明，可能往資料來源與邏輯延伸 | 評估 LVGL XML 互通；把差異放在 IoT、並行模型、整機模擬與 OTA 這些原廠目前沒做的層；持續追蹤其 releases |
| 人力 | 目前只有一人 | 新公司至少 4 至 6 人（韌體與 RTOS、前端與 VS Code、雲端與授權、業務與 FAE） |
| 客戶證據 | 目前沒有付費客戶與量測過的節省時間 | 第 0 階段的訪談與付費試用是硬性檢核點 |
| EDT 關係 | EDT 的商業模式是免費軟體帶動模組營收；分拆可能失去這個通路 | OEM 授權回用，並讓 EDT 持股 |
| CRA | 2027-12-11 起歐盟 CRA 全面適用；工具產生的韌體若含網路功能，客戶需要 SBOM 與更新機制 | 把 SBOM 輸出與 OTA 做成工具功能，轉成賣點（見 [EU OTA 與 CRA 研究](../eu-ota-cra-sales-research/README.zh-TW.md)） |

## 文件

- [程式碼耦合與可沿用資產盤點](research/01-codebase-coupling.zh-TW.md)
- [全球同類競品比較](research/02-competitors.zh-TW.md)
- [LVGL Pro 分析與付費價值](research/04-lvgl-pro-and-paid-value.zh-TW.md)
- [MicroEJ 比較](research/05-microej-comparison.zh-TW.md)
- [用動態語言開發 MCU 的付費產品](research/06-dynamic-language-mcu-products.zh-TW.md)
- [以 C/C++ 為輸出的新式 RAD 付費工具](research/07-c-cpp-rad-tools.zh-TW.md)
- [MCU 軟韌體研發的需求與困擾排行](research/08-mcu-development-pain-points.zh-TW.md)
- [架構提案：MCU 上的單元匯流排與可程式化交換中心](research/09-mcu-unit-bus-architecture.zh-TW.md)
- [EHS 收費化路線圖](research/10-ehs-paid-product-roadmap.zh-TW.md)
- [MCU 熱重載：商業產品與 EHS 的機會](research/11-mcu-hot-reload.zh-TW.md)
- [來源與未解問題](research/03-sources-and-open-questions.zh-TW.md)
