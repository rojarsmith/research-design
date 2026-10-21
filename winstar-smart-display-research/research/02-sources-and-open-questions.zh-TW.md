# 華凌智慧顯示器來源與待確認事項

[回主報告](../README.zh-TW.md) · 查核日期：2026 年 10 月 21 日，Asia/Taipei。

## 既有覆蓋與本次新增

| 使用者指定網址 | 原研究狀態 | 本次處理 |
| --- | --- | --- |
| [Smart Display 總覽](https://www.winstar.com.tw/zh-tw/products/smart-display.html) | 原研究未以此頁建立完整系列研究 | 納入 CAN、RS485、UART 結構與工具取得方式 |
| [CAN 系列](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display.html) | 已在原報告研究 CANopen、Custom CAN ID、DBC 與一個 SKU | 重新核對，延伸第二頁、OLED 與詳細規格 |

覆蓋查核依據為[原主報告](../../mcu-embedded-hmi-research/README.zh-TW.md)、[產品比較](../../mcu-embedded-hmi-research/research/01-product-comparison.zh-TW.md)及[原來源表](../../mcu-embedded-hmi-research/research/03-sources-and-open-questions.zh-TW.md)。

## 官方來源

| 來源 | 用途與讀取範圍 |
| --- | --- |
| [Smart Display 總覽](https://www.winstar.com.tw/zh-tw/products/smart-display.html) | 系列定位、工具與框架宣示 |
| [CAN 第 1 頁](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display.html) | CAN 2.0A/B、CANopen、Custom CAN ID、DBC 與 SKU |
| [CAN 第 2 頁](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/page:2.html) | 較大 TFT、32 MB 型號與 OLED；列表顯示共 24 筆，含 demo |
| [RS485 系列](https://www.winstar.com.tw/zh-tw/products/smart-display/rs485_display.html) | Modbus 路線與代表 SKU |
| [UART 系列](https://www.winstar.com.tw/zh-tw/products/smart-display/uart_display.html) | Proprietary UART 路線與代表 SKU |
| [4.3 吋 CANopen](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/4_3.html) | WL0F00043000WGAAASA00 詳細規格與開發入口 |
| [4.3 吋 RS485](https://www.winstar.com.tw/zh-tw/products/smart-display/rs485_display/wl0f00043000wgdaasa00.html) | WL0F00043000WGDAASA00、從裝置角色與規格 |
| [4.3 吋 UART](https://www.winstar.com.tw/zh-tw/products/smart-display/uart_display/4_3-inch.html) | WL0F00043000WGFAASA00、GPIO、電氣與點對點描述 |
| [3.55 吋 OLED](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/smart-oled.html) | WLEP02566400DGAAASA00 顯示與硬體欄位 |
| [GUI Builder 介紹](https://www.winstar.com.tw/zh-tw/smart-display-GUI-builder-introduction) | 2021-02-01 文章的設計、模擬與圖資流程；安裝範例不是現行版本 |
| [GUI Builder 指南](https://www.winstar.com.tw/zh-tw/gui-builder-user-guide.html) | 2022-11-10 教程索引；未逐一觀看影片 |
| [版本總覽](https://www.winstar.com.tw/zh-tw/gui-builder-version-history.html) | 列示版本、發布月份及詳細文章入口 |
| [v0.8.5／v0.8.6](https://www.winstar.com.tw/zh-tw/gui-builder-v085ws-v086ws.html) | 16／32 MB 容量表、尺寸與通訊限制 |
| [v0.8.7](https://www.winstar.com.tw/zh-tw/gui-builder-v087_WS.html) | 以瀏覽器成功讀取正文，核對機種限制與發布月份 |

本輪未提交試用申請、下載安裝軟體或索取報價。原報告其他廠商的資料仍以其原查核日期為準。

## 規格與日期差異

| 項目 | 網站所見差異 | 選型處理 |
| --- | --- | --- |
| 4.3 吋 CANopen 供電 | 分類表列 5 V；詳細頁工作範圍與電氣表列 5–24 V，典型 12 V | 確認實際 PCBA 版號、可用供電腳位與正式規格；不把分類表單值當成完整範圍 |
| 4.3 吋 RS485 供電 | 同樣出現分類表 5 V 與詳細頁 5–24 V | 按該型號與硬體版本查核，不借用 CAN 型號結論 |
| 4.3 吋 UART baud rate | 詳細頁寫 115200 Kbps，單位有疑點 | 可能原意為 115,200 bit/s，但僅為推測；以協定手冊與實機設定確認 |
| OLED 尺寸 | 分類表外型 107×68.7 mm；詳細頁為 157×40×11.45 mm；詳細頁有效尺寸亦與像素間距計算不符 | 正式圖面確認前，不用這些欄位做機構設計 |
| v0.8.5／v0.8.6 日期 | 總覽列 2026 年 6 月；詳細文章標 2026 年 7 月 | 分別保留，不自行定出唯一精確發布日 |
| v0.8.7 日期 | 文章日期 2026-10-01；內文版本發布月份為 2026 年 9 月 | 區分文章發布與軟體發布 |

對應來源分別為上表的 CAN／RS485 分類及型號頁、UART 型號頁、OLED 型號頁與版本文章。OLED 的交叉檢查為 `256×0.342 = 87.552 mm`、`64×0.333 = 21.312 mm`，與詳細頁有效尺寸 `95.04×53.856 mm` 不一致；不能據此反推出正確圖面。

這些差異表示採購所需的數值仍需型號級文件解決，不構成產品實際不符合規格的結論。

## 優先待確認資料

| 主題 | 需要取得的具體答案 |
| --- | --- |
| SKU 與硬體 | 正式 datasheet、圖面、PCBA 版號、供電／I/O、觸控及環境條件 |
| CANopen | EDS、物件字典、支援的網路管理與 PDO／SDO 行為 |
| Custom CAN ID | DBC 語法涵蓋範圍、訊號上限、端序／縮放、觸摸回報與失聯處理 |
| RS485／Modbus | 每個 SKU 的主從角色、RTU／ASCII、功能碼、字序與更新能力 |
| UART | 真正 baud rate、邏輯／電氣標準、封包／校驗、命令與相容性政策 |
| GUI Builder | 現行 widget 清單、相容韌體、專案限制、離線使用與專案遷移 |
| 自寫韌體 | initial code 的來源、BSP／driver、框架版本、授權及原廠協定可否保留 |
| 商用與維護 | 工具與 runtime 費用、專案與程式權利、供貨週期、PCN／EOL 與支援窗口 |

上述問題是後續文件索取及概念驗證的輸入；本次未替使用者向廠商發送詢問。
