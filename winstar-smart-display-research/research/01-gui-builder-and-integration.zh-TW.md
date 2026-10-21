# GUI Builder 版本與主控整合評估

[回主報告](../README.zh-TW.md) · 查核日期：2026 年 10 月 21 日。

GUI Builder 的適用能力取決於工具版本、模組韌體、通訊系列及記憶體配置。選型應先固定這些條件，再確認頁面、元件與協定需求。

## 設計與部署流程

官方介紹描述 Windows 視覺設計、多頁 layout、模擬與自行更新圖資；在 PC 模擬階段不必先有模組或 USB2CAN。這讓畫面原型可以先行，但模擬結果仍需與目標裝置的通訊和資源限制一起驗證。[GUI Builder 介紹](https://www.winstar.com.tw/zh-tw/smart-display-GUI-builder-introduction)。

2022 年操作指南列出啟動畫面 GIF、上傳小物件、更新整個專案、baud rate、工具版本與模組軟體版本等教學。這些標題可作為評測任務；指南沒有提供本次可直接核對的完整現行 widget 規格，因此不從影片標題推定所有型號具有相同能力。[操作指南](https://www.winstar.com.tw/zh-tw/gui-builder-user-guide.html)。

實務上可分為四步：建立畫面與圖資、設定控制器資料、在 PC 模擬、部署至模組並測通訊。切換為自行開發韌體路線時，應另做 BSP 與程式建置驗證。

## 版本與適用範圍

官方版本總覽目前最高列 v0.8.7_WS，版本發布月份標為 2026 年 9 月。其更新文章日期是 2026 年 10 月 1 日；兩者分別是版本發布月份與文章日期。[版本總覽](https://www.winstar.com.tw/zh-tw/gui-builder-version-history.html)、[v0.8.7_WS](https://www.winstar.com.tw/zh-tw/gui-builder-v087_WS.html)。

| 版本 | 可確認的更新 | 適用限制 |
| --- | --- | --- |
| v0.8.5_WS | Number Widget 可直接設定小數位數 | 文章限 7 吋 16 MB Custom CAN ID |
| v0.8.6_WS | 新增 32 MB 模組支援及較大的專案容量 | 尺寸與通訊組合須按更新說明確認 |
| v0.8.7_WS | 調整新增 widget 的操作；加入預設亮度、Number Widget 縮放指數設定 | 亮度新增支援 4.3 吋 UART 與 7 吋 1024×600 UART 16／32 MB；縮放指數限後者 |

來源：[v0.8.5_WS 與 v0.8.6_WS](https://www.winstar.com.tw/zh-tw/gui-builder-v085ws-v086ws.html)、[v0.8.7_WS](https://www.winstar.com.tw/zh-tw/gui-builder-v087_WS.html)。v0.8.5／v0.8.6 的總覽列 2026 年 6 月，詳細文章則標發布日期 2026 年 7 月，保留此日期差異。

### 專案容量

以下重整自 v0.8.5／v0.8.6 官方比較表。它是特定系列的設定容量，不是所有元件同時渲染的效能保證。

| 項目 | 支援的 32 MB 系列 | 16 MB Custom CAN ID | 16 MB CANopen／RS485 | 16 MB UART |
| --- | --- | --- | --- | --- |
| 單頁 widgets 上限 | 255 | 255 | 64 | 255 |
| 專案 widgets 上限 | 1,000 | 500 | 500 | 500 |
| 頁面上限 | 64 | 30 | 30 | 30 |

同篇指出，32 MB 的 7／10.1 吋支援 Custom CAN ID、RS485、專屬 UART 與 GPIO，5 吋則只列 Custom CAN ID；不能由 32 MB 推論全部尺寸與協定均有對應產品。[容量及系列限制](https://www.winstar.com.tw/zh-tw/gui-builder-v085ws-v086ws.html)。

工程判斷：升到 32 MB 是否有價值，需同時看頁數、元件總量、圖資及字型空間。Flash 加倍不表示 RAM 或 FPS 加倍。

## 主控整合的四條路線

以下為建議驗證方法，不是宣稱已提供的協定功能清單。

| 路線 | 先取得的文件 | 首輪實驗 |
| --- | --- | --- |
| CANopen | EDS／物件字典、PDO／SDO、網路管理與版本說明 | 映射數值與觸摸回報，測啟動、節點狀態及失聯恢復 |
| Custom CAN ID | 支援的 DBC 語法、標準／延伸 ID、訊號映射與回傳格式 | 測端序、signed、scale／offset、多訊號封包與逾時 |
| RS485／Modbus | 主從角色、RTU／ASCII 模式、功能碼、位址、字序及異常碼 | 主控讀寫數值、取得觸摸狀態，測多站與輪詢負載 |
| UART | 電氣標準、封包邊界、命令、校驗、回覆與版本相容性 | 測完整與截斷封包、重試、連續操作及重新同步 |

以 4.3 吋 RS485 型號為例，官方將其定義為從裝置。若現有 PLC 也僅能當從站，必須增加主站或重新設計資料流程。UART 型號頁則描述點對點；不能直接套用 RS485 多站設計。[RS485 詳細頁](https://www.winstar.com.tw/zh-tw/products/smart-display/rs485_display/wl0f00043000wgdaasa00.html)、[UART 詳細頁](https://www.winstar.com.tw/zh-tw/products/smart-display/uart_display/4_3-inch.html)。

## 概念驗證建議

採相同 480×272 畫面語意，比較 CANopen、Custom CAN ID、Modbus 與 UART 的端到端操作。沿用[原 MCU HMI 共同評測方法](../../mcu-embedded-hmi-research/research/02-architecture-and-evaluation.zh-TW.md)，新增下列驗收點。

| 項目 | 驗證方式 |
| --- | --- |
| 專案容量 | 依所選 SKU 的上限建立專案，檢查工具提示、儲存、模擬及部署 |
| 資料正確性 | 相同原始值顯示相同單位，測負數、小數、溢位、長文字與多語缺字 |
| 觸控與通訊 | 分開量測觸摸到回報、主控處理、畫面更新；記錄忙碌時是否遺失事件 |
| 斷線與重啟 | 測主控晚啟動、模組重啟、斷線重連，確認舊值及失聯提示 |
| 專案更新 | 分開測圖資、專案與韌體更新；斷電後確認復原及版本辨識 |
| 重建與維護 | 固定 editor／firmware 版本，從備份環境重建；測換模組與專案遷移 |
| 自寫韌體 | 取得 initial code 後，先完成顯示、觸控、Flash 與通訊基本驗證，再比較框架效能 |

建議先完成畫面與資料正確性，再跑持續更新與反覆切頁。工具容量上限只能作為測試條件；產品合格的延遲與穩定性門檻由實際需求設定。

## 授權與成本

總覽提供申請表以 Email 取得 GUI Builder 試用連結；版本頁有先試用後購買的用語。這些資料不足以判定正式商用席次、runtime 權利或更新費用。[取得方式](https://www.winstar.com.tw/zh-tw/products/smart-display.html)、[版本頁](https://www.winstar.com.tw/zh-tw/gui-builder-version-history.html)。

報價應分開列模組、工具授權、轉接／燒錄設備、客製 BSP、工程支援與維護，並確認專案檔、圖資、初始程式及量產韌體的使用權。完整成本仍需計入主控協定及部署工具的工時。

採購前最有價值的交付物是「指定 SKU＋硬體版號＋韌體版號＋GUI Builder 版本＋協定手冊＋正式規格書＋授權條件」。有了這組資料，才能把網站宣示轉成可驗收的整合方案。
