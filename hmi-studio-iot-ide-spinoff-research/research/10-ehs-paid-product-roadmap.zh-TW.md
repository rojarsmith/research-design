# EHS 收費化路線圖

整理日期：2026 年 11 月 19 日。本文整合 01 至 09 號研究（熱重載見 11 號，PC 端模擬見 12 號，硬體與應用分離及上層語言見 13 號），回答：**EHS 要怎麼改，才可能成為有競爭力的收費軟體？** 文中的價格、人力與時程都是假設，要靠第七節的證據驗證。

## 結論

**不要再以「UI 編輯器」收費，要把產品改成「讓整台連網裝置做得出來、出貨後還管得住」的工具。**

單比 UI，EHS 打不過三種對手：

- 原廠 LVGL Pro：每席每年 US$990 至 3,990。
- SquareLine：每月 US$9 至 19。
- 晶片商的免費工具。

詳見 [LVGL Pro 分析](04-lvgl-pro-and-paid-value.zh-TW.md) 與 [全球競品比較](02-competitors.zh-TW.md)。

| 走法 | 作為收費軟體的競爭力 |
| --- | --- |
| 維持現狀，只是開始收費 | 低：功能和 LVGL Pro、SquareLine 重疊，價格又比免費工具高 |
| 照本文的方向轉型 | 中到高：站在 UI 工具、IoT 雲、晶片商 IDE 都沒有覆蓋的中間層 |

## 一、先拿掉讓人無法付費的障礙

不做這幾項，其他功能再好也賣不出去。現況見 [程式碼耦合盤點](01-codebase-coupling.zh-TW.md)。

| 障礙 | 現況 | 要改成 |
| --- | --- | --- |
| 授權 | EULA 規定只能用在 EDT 硬體 | 不綁硬體的商業授權（若分拆，先處理 IP） |
| 硬體與工具鏈 | 只支援 STM32、ST 工具、Windows PowerShell 腳本 | 透過 Zephyr 與 FreeRTOS 支援多晶片，第一批 STM32 與 ESP32-S3；建置改為跨平台 |
| 工程流程 | 只能在桌面 GUI 操作 | 核心引擎拆成 CLI，能在 CI 上自動建置與測試 |
| 品牌 | 到處是 EDT 字樣 | 改名，保留舊專案格式的遷移路徑 |

## 二、核心架構改成「單元匯流排」

這是轉型的骨幹，也是多執行緒安全的來源，詳見 [單元匯流排架構提案](09-mcu-unit-bus-architecture.zh-TW.md)。EHS 現有設計可以順著演進，不必打掉重做：

| EHS 現有 | 演進為 |
| --- | --- |
| Tag 模型（Link → Device → Tag） | 匯流排上的狀態與事件 |
| Logic 節點圖 | 交換中心的規則（觸發 → 條件 → 動作） |
| Modbus 與 UART 連線 | 通訊單元 |
| LVGL 畫面 | GUI 單元 |
| 裸機主迴圈 | 單元各自的 Task，由匯流排分派 |

## 三、把收費功能放在最痛的地方

依 [MCU 研發困擾排行](08-mcu-development-pain-points.zh-TW.md) 排序：

| 優先 | 功能 | 對應痛點 | 現況 |
| --- | --- | --- | --- |
| 1 | **匯流排錄製重播與整機模擬**：真機錄下的流量可在 PC 上重播、單步，任何單元都能換成模擬單元；畫面、規則與 Tag 參數可在真機上熱重載（見 [MCU 熱重載](11-mcu-hot-reload.zh-TW.md)）；外部設備以行為層級模擬，驅動層交給 Renode（見 [PC 端模擬](12-mcu-device-simulation.zh-TW.md)） | 除錯最花時間、硬體還沒到就無法測試 | 已有 WASM 模擬器，錄製重播與熱重載要新做 |
| 2 | **現場通訊與連網單元**：Modbus、CAN、MQTT、HTTP，免寫程式綁定 | 連網是最大顧慮（Eclipse 調查 48%） | 已有 Modbus RTU 與 UART，其餘要補 |
| 3 | **OTA 加 SBOM 輸出**（MCUboot 搭配 hawkBit 或 Bitdove） | 出貨後的召回風險、歐盟 CRA 2027-12 全面適用 | 尚未開始；管理層會撥預算的項目 |
| 4 | **可寄給客戶的單檔 Demo** | 業務週期長，客戶沒看到成品就不下單 | 已有，是現成的差異化 |
| 5 | **AI 助理擴充**：從需求產生單元、規則與測試情境 | 開發複雜度、人才不足 | 已有畫面生成，擴充範圍即可 |

前三項是 LVGL Pro、SquareLine、TouchGFX 都沒做的，**這就是收費的理由**。

## 四、產品形態：混合式

- **共用引擎與 CLI**：專案格式、程式碼產生、模擬器、匯流排執行環境。
- **VS Code 付費擴充**：給工程師。多執行緒除錯借用 SystemView、Tracealyzer 等現有工具；同時上架 Microsoft Marketplace 與 Open VSX；授權伺服器要自建。先例見 [C/C++ RAD 工具](07-c-cpp-rad-tools.zh-TW.md) 的 VisualGDB。
- **No Code 桌面版**：給 OEM 與模組商通路，沿用 EHS 現有的使用體驗。
- **與 LVGL 生態互補**：評估匯入 LVGL XML，讓 LVGL Pro 使用者把 UI 帶進來再加上 IoT 層，而不是和原廠正面衝突。

## 五、收費結構

| 方案 | 價格（假設，待訪談驗證） | 參考依據 |
| --- | --- | --- |
| Community | 免費、限非商業、Demo 加浮水印 | LVGL Pro 也有免費的 Community 方案 |
| Pro | 每席每年 US$790 至 1,490 | 低於 LVGL Pro Developer 的 US$3,990，高於 SquareLine |
| Product | 每產品 US$4,000 至 8,000，或每台 US$0.5 至 1 權利金 | Moddable 每台 US$0.50、Slint 每台約 US$1 起、LVGL Pro 每產品 US$20,000（見 [06](06-dynamic-language-mcu-products.zh-TW.md)、[07](07-c-cpp-rad-tools.zh-TW.md)） |
| OEM 與晶片商 | 模組商白標內建，或由晶片商贊助客戶免費使用 | LVGL Pro 對 Renesas 客戶免費；Weintek 與 TouchGFX 都是「軟體免費、賣硬體」 |
| 垂直範本與服務 | 充電樁、家電、工控面板的範本專案，加上客製連接器 | MicroEJ 靠電表、手錶等垂直方案打進市場（見 [05](05-microej-comparison.zh-TW.md)） |

**EDT 可以是第一個 OEM 客戶**，同時保住它「免費軟體帶動模組銷售」的模式。

## 六、不要做的事

- 不跟 LVGL Pro 比元件數量與 UI 細節，原廠永遠比較快。
- 不自己經營雲端：接 ThingsBoard、AWS、自架 MQTT 就好，雲端正在快速免費化。
- 不自己做 IDE 外殼與除錯器：交給 VS Code 生態。
- 不做 MicroEJ 那種虛擬機平台：純 C 輸出、不需額外執行環境本身就是賣點，VM 只做選配。使用者擴充可選用 C 編譯成 WebAssembly（見 [13](13-app-layer-separation-and-languages.zh-TW.md)）。
- 不做「通用 IoT IDE」：晶片商與 Arduino（已歸 Qualcomm）都免費。

## 七、投入前要先拿到的證據

1. **客戶訪談 10 到 15 家**（充電樁、家電、工控），問清楚他們願意為哪一層付錢、願意付多少。至少拿到 3 家付費試用意向。
2. **匯流排原型量測**：按鈕中斷到畫面更新的延遲、匯流排占用的記憶體、錄製重播是否一致。這三個數字決定能不能說服工程師。
3. **「一天完成連網樣機」實測**：同一個情境，比較「LVGL Pro 加手寫程式」和「新 EHS」各要幾小時，並錄影存證，作為行銷的核心數字。

## 八、執行順序

> 人力與時程是假設，參考 [RAD 工具研究](../../rad-tools-research/README.zh-TW.md) 的「聚焦商業產品：4 至 8 人、9 至 18 個月」區間。

| 階段 | 內容 | 過關條件 |
| --- | --- | --- |
| 0. 決策與驗證 | EDT 決定 IP 與組織形式；客戶訪談；匯流排原型量測 | 書面 IP 安排；3 家付費試用意向；原型六項量測通過 |
| 1. 去綁定與拆引擎 | 新授權與品牌；引擎拆成 CLI；建置跨平台 | Linux CI 上能建出韌體 |
| 2. 單元匯流排與 Zephyr | Tag 與 Logic 演進為匯流排與規則；Zephyr 目標；錄製重播 | 示範產品在真機與模擬器上行為一致，可重播 |
| 3. 連網與 OTA | MQTT、HTTP、CAN 單元；MCUboot 加 hawkBit；SBOM 輸出 | 示範機可遠端改參數、推新韌體、失敗回滾 |
| 4. VS Code 擴充與授權 | Webview 編輯器、匯流排時間軸、授權伺服器；兩個 Marketplace 上架 | 10 位外部工程師完成試用任務 |
| 5. 商業化 | 席位、產品授權、OEM、垂直範本 | 12 個月內有可預測的經常性收入 |

## 九、兩個前提

- **人力**：一人做不完。至少需要 4 到 6 人：韌體與 RTOS、前端與 VS Code、雲端與授權、業務與 FAE。
- **IP 歸屬**：EHS 的 IP 屬於 EDT。無論分拆，還是留在 EDT 內部收費，都需要經營層先做決定（見[主報告的分拆前提](../README.zh-TW.md#分拆的前提條件)）。
