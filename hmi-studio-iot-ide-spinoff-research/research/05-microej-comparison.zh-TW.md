# MicroEJ 比較：付費的完整 MCU 裝置研發平台

查核日期：2026 年 11 月 19 日。資料來自 MicroEJ 官方頁面（[首頁](https://www.microej.com/)、[VEE](https://www.microej.com/product/vee-software-container-embedded-systems/)、[SDK](https://www.microej.com/product/sdk/)、[授權](https://www.microej.com/licensing/)、[關於](https://www.microej.com/about-us/)、[新聞](https://www.microej.com/press/)、[多沙箱](https://www.microej.com/multi-sandboxed-containerized-apps/)）與[開發文件](https://docs.microej.com/)（含 [Kernel FAQ](https://docs.microej.com/en/latest/KernelDeveloperGuide/kernelFAQ.html)）。

## 一句話結論

**MicroEJ 證明「完整的 MCU 裝置研發平台」可以收費，而且收得到大客戶**：宣稱 120 家以上公司、4.5 億至 5 億台以上裝置。但它走的是**以虛擬機為核心的 Pro Code 平台、對大型 OEM 做多年期企業銷售**，經過約 12 年、宣稱投入 4,000 萬美元研發。EHS 不應複製這條路，而是：

- 借用它驗證過的概念：虛擬裝置、應用隔離、應用層 OTA、晶片商合作。
- 站在它沒有占據的位置：Low Code 與 No Code、產出無 VM 的純 C、公開價格、中小型 OEM。

## MicroEJ 是什麼

| 面向 | 內容 |
| --- | --- |
| 核心 | MICROEJ VEE：「嵌入式系統的軟體容器」，由 MEJ32 虛擬處理器（32 或 64 位元）執行 |
| 語言 | Java、C 為主；各頁面另列 JavaScript、C++、Rust，Kotlin 在路線圖上 |
| 執行環境 | MCU、MPU、SoC；FreeRTOS、Zephyr、ThreadX 等 RTOS、Linux 或裸機；宣稱支援 12 種以上 ISA、10 種以上 OS 與 RTOS |
| IDE | **不自己做 IDE**：SDK 6 以 Gradle 為基礎，支援 IntelliJ IDEA、Android Studio、Eclipse、Visual Studio 與命令列，可接 Jenkins 與 Artifactory（2023 年 12 月宣布與 IDE 無關的策略） |
| 模擬 | Virtual Device：模擬程式執行、螢幕、觸控、按鍵與 LED、IP 連線、感測器、自訂 mockup |
| 除錯 | 程式碼覆蓋率、堆疊追蹤、記憶體與 heap 檢視、即時追蹤、JUnit |
| GUI | MicroUI 與 MWT 函式庫、字型產生器、Storyboard designer、多語系；**官方頁面未見拖放式 No Code UI 設計器** |
| 並行 | 優先權搶占加同優先權時間片輪轉；Kernel 可限制應用調整優先權，並可對每個應用設 CPU 執行配額。Java 執行緒是否一對一對應 RTOS task，文件未說明 |
| 隔離 | Kernel & Features 規範：應用之間不能互相持有物件、執行緒與資源；Kernel 可隨時停止或卸載應用；位元碼驗證、應用簽章；應用間以 Shared Interface 與 RPC 溝通 |
| IoT（免費函式庫） | NET（TCP/IP、UDP、Ethernet、Wi-Fi）、BLE、MQTT（MicroPaho）、TLS 與加密、檔案系統、Watchdog、Trace |
| IoT（另列） | CoAP、Thread、JSON、CBOR、Modbus、CAN、USB、Zigbee、CatM、NB-IoT |
| 商業附加元件 | UX 與 UI 引擎、Hoka（HTTP 與 REST 伺服器）、Binary Loader（應用層 OTA 動態部署）、PicoAI 與 MicroAI、DLMS/COSEM（電表） |
| 應用商店 | 可自建公開或私有應用商店管理裝置上的應用 |

### 授權與價格

- 核心 IP（MEJ32，以及選購的 TrustAPP、MultiLang）以**多年期合約（MYC）**授權：CAPEX 模式，在約定年數內固定價格。
- 商業函式庫可以折扣綁進 MYC。
- 頁面另提到「按使用量與經常性（ARR）」模式。
- **未公開價格**，只能索取報價。評估入口是需要填表的 Virtual Device Demo；SDK 是否免費下載，頁面沒有寫明。

### 公司

| 項目 | 內容 |
| --- | --- |
| 創辦人兼 CEO | Fred Rivard |
| 創立 | 約 2012 年 |
| 總部 | 法國南特，另有美國據點 |
| 投資人 | 列出 Innovacom |
| 首頁客戶 logo | Polar、Landis+Gyr、Groupe SEB、Schneider Electric、Stanley Black & Decker、Sensus、Fossil、Hill-Rom、Decathlon 等 |
| 合作夥伴 | NXP、Qualcomm（Snapdragon Sound）、Actions Technology、Thales Cinterion、Zephyr Project |
| 2025 至 2026 動態 | Vitalist 與 Reebok 手錶的 VitalOS（2025-11）；Boon Logic 邊緣 AI（2025-11）；Kimbal 電表合作（2025-11）；Facer 錶面上 RTOS 手錶（2026-01）；VEE Audio 平台（2026-07） |

客戶與裝置數都是官方自述，未經獨立查證。

## 與 EHS 新產品定位對照

| 面向 | MicroEJ | EHS 新產品（建議） |
| --- | --- | --- |
| 開發方式 | Pro Code（Java、C 等），工程師導向 | Low Code 加 No Code，應用工程師導向，可展開為 C |
| 裝置端架構 | VM 容器（MEJ32）加 Kernel 與應用 | 產生純 C 加 LVGL，**不需要 VM**；後期可評估小型腳本層 |
| 多應用與隔離 | **強項**：沙箱、配額、簽章、動態安裝 | 不做多應用；以 UI Task 獨占加 Tag Store 解決單一應用內的並行安全 |
| 並行 | 搶占式執行緒，可設配額 | 視覺化 Task、設計期衝突偵測、確定性模擬重播 |
| UI 設計 | 函式庫與 Storyboard，偏程式 | **強項**：拖放、屬性面板、節點邏輯、AI 從草圖建畫面 |
| 現場通訊 | Modbus、CAN 列為支援 | 標籤模型直接把 Modbus 與 CAN 對應到畫面與邏輯，免寫程式 |
| 連網 | 完整（TCP/IP、MQTT、TLS、BLE、CoAP…） | 待建：先做 MQTT 與 HTTP，借用 Zephyr 的網路堆疊 |
| OTA | 應用層 Binary Loader 加應用商店 | 整機韌體 OTA（MCUboot 加 hawkBit 或 Bitdove） |
| 模擬 | Virtual Device（含感測器與 IP） | WASM 整機模擬，加模擬 Modbus 裝置與可寄送的單檔 Demo |
| IDE | 不自己做，支援多種 IDE | VS Code 擴充加 No Code 桌面版 |
| 目標客戶 | 大型 OEM：穿戴、電表、家電、醫療 | 中小型 OEM、系統整合商、模組商通路 |
| 銷售 | 企業銷售、多年期合約、不公開價格 | 公開價格、自助購買，另有 OEM 與通路授權 |
| 鎖定 | 應用依賴 VEE 執行環境 | 產出的 C 可帶走 |

## 從 MicroEJ 學到的五件事

1. **「平台」可以收大錢，但要用「量產」的語言來賣。** MicroEJ 賣的不是 IDE，而是「降低多產品線的軟體成本、出貨後還能更新功能」。它的價值主張寫成 CAPEX 與多年期合約，對象是產品線主管。EHS 的 Product 方案也應該用「每條產品線省多少人月、出貨後能否更新」來包裝。
2. **IDE 不必自己做。** 連 MicroEJ 這種完整平台都選擇支援現有 IDE 加 Gradle 與 CI。這支持「VS Code 擴充加 CLI」的方向，也支持「把引擎拆成無 UI 的 CLI」作為第一步。
3. **虛擬裝置是核心賣點。** MicroEJ 把 Virtual Device 放在開發流程的中心，支援感測器、連線與自訂 mockup。EHS 的 WASM 模擬器已經有類似能力，而且多了「可寄給客戶的單檔 Demo」，應該擴充成可模擬 MQTT broker 與雲端的整機模擬。
4. **晶片商與垂直應用合作是通路。** MicroEJ 透過 NXP、Qualcomm、Actions，以及電表的 DLMS/COSEM、手錶的 VitalOS 這類垂直方案進入市場。EHS 應挑一到兩個垂直市場做深（例如 EV 充電樁的 OCPP 加 Modbus 電表，或家電），而不是做通用平台。
5. **出貨後的功能更新是長期收入的理由。** Binary Loader 與應用商店讓客戶出貨後還能加功能。EHS 現階段做整機 OTA 即可；長期可評估「只更新 UI 與 Logic 層」的資料或腳本更新：以 Logic 圖產生可熱更新的位元碼，搭配本 repo minilang 研究中的 VM 設計，這是向 MicroEJ 靠攏的高階功能。

## 不該學的地方

- **不要做 VM 平台。** MEJ32 加 Kernel 加多語言要多年與大量資金。EHS 的優勢正是「純 C 輸出、沒有額外執行環境」，這也是 LVGL Pro 強調的賣點，市場顯然重視。
- **不要只做企業銷售。** 不公開價格加多年期合約適合有銷售團隊的公司，一家剛分拆、人數個位數的公司做不起長週期大案。應以公開價格與自助試用取得中小客戶，再用 OEM 授權接大單。

## 結論對主報告的影響

- MicroEJ 是「付費完整 MCU 研發平台」的最佳證據，說明這個市場**願意為降低整體研發與維護成本付費**，不只是為 UI 付費。
- MicroEJ 與 EHS 的客群與開發方式不重疊（Pro Code 大 OEM 對 Low Code 中小 OEM），**不是直接對手，而是天花板的參考**：EHS 長期可以往應用層更新與多產品線管理延伸，但第一階段不應該碰。
