[English overview](README.md) | **繁體中文**

# 華凌智慧顯示器系列與整合研究

研究日期：2026 年 10 月 21 日，Asia/Taipei。

華凌 Smart Display 應按「顯示模組、內建執行環境、GUI Builder 與主控協定」整套評估。相較單獨圖形框架，它提供整合硬體與通訊入口；實際開發自由度則取決於採用標準韌體，或取得板級程式後自行開發。

使用者指定的 [Smart Display 總覽](https://www.winstar.com.tw/zh-tw/products/smart-display.html)與 [CAN 系列](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display.html)均納入本研究。總覽分為 CAN、RS485、UART；本次把既有 CAN 比較延伸為完整產品組合研究。

## 與既有研究的關係

[MCU Embedded HMI 生態研究](../mcu-embedded-hmi-research/README.zh-TW.md)於 2026 年 10 月 12 日已比較華凌 CAN、CANopen／Custom CAN ID、DBC 與代表 SKU。因此 CAN 並非未研究過的新增品牌。

本次補上總覽、RS485／Modbus、專屬 UART、GUI Builder 版本與容量，以及 CAN 第二頁和型號詳細頁。保留原報告作為跨廠商比較入口；以下「新增」指研究覆蓋範圍，不表示產品在兩次查核間才推出。

## 系列比較

| 路線 | 官方記載的整合方式 | 適合優先評估的情境 | 要取得的整合資料 |
| --- | --- | --- | --- |
| CANopen | GUI 元件預綁 CANopen Object Dictionary | 已使用 CANopen 資料模型的設備 | 物件字典、裝置描述、訊息週期與觸控回報 |
| Custom CAN ID | 自訂 CAN ID 與資料框架，支援 DBC 匯入及訊號綁定 | 已有自訂 CAN 訊息的控制器 | DBC 支援範圍、端序、縮放、逾時與回報格式 |
| RS485／Modbus | GUI 元件與 Modbus 綁定 | 已有 Modbus 控制器的設備 | 主從角色、暫存器地圖、功能碼、時序及字序 |
| Proprietary UART | 使用華凌專屬 UART 協定綁定 GUI 元件 | 主控可加入專用串口協定的產品 | 電氣介面、封包、命令、回覆與觸控事件 |

官方依據：[CAN](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display.html)、[RS485](https://www.winstar.com.tw/zh-tw/products/smart-display/rs485_display.html)、[UART](https://www.winstar.com.tw/zh-tw/products/smart-display/uart_display.html)。情境欄為工程判斷。

CAN 系列頁明示 CAN 2.0A/B，沒有據此確認 CAN FD。UART 是介面名稱，不能推定其協定與 DWIN、Levetop 或其他串口屏相容。RS485 產品詳細頁把下列 4.3 吋型號定義為從裝置，因此不能只因使用 Modbus，就假定顯示器可主動輪詢 PLC。[CAN 規格範圍](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display.html)、[RS485 型號角色](https://www.winstar.com.tw/zh-tw/products/smart-display/rs485_display/wl0f00043000wgdaasa00.html)。

## 代表型號

下表選用相同尺寸與解析度，便於比較通訊路線。這是公開規格對照，並非效能或價格排名。

| 項目 | CANopen | RS485／Modbus | Proprietary UART |
| --- | --- | --- | --- |
| 型號 | WL0F00043000WGAAASA00 | WL0F00043000WGDAASA00 | WL0F00043000WGFAASA00 |
| 顯示與觸控 | 4.3 吋、480×272、PCAP | 4.3 吋、480×272、PCAP | 4.3 吋、480×272、PCAP |
| MCU | STM32F750 | STM32F750 | STM32F750 |
| Flash | 16 MB | 16 MB | 16 MB |
| 典型亮度 | 400 cd/m² | 400 cd/m² | 400 cd/m² |
| 詳細頁工作電壓 | 5–24 V，電氣表典型 12 V | 5–24 V，電氣表典型 12 V | 5–28 V，典型 12 V |
| GPIO | 分類表列 No | 分類表列 No | 列 8 通道雙向、3.3 V 邏輯 |

型號來源：[CANopen](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/4_3.html)、[RS485](https://www.winstar.com.tw/zh-tw/products/smart-display/rs485_display/wl0f00043000wgdaasa00.html)、[UART](https://www.winstar.com.tw/zh-tw/products/smart-display/uart_display/4_3-inch.html)。GPIO 的 CAN／RS485 欄依各自系列分類表。

CAN 與 RS485 的分類表將這兩個 SKU 列為 5 V，詳細頁則列 5–24 V；兩者的數字可能分別表示供電選項與可用範圍，但官方未在這些頁面解釋。下單與接線前應以對應硬體版本的正式規格書確認。不能以另一個相似料號的規格代替。

### CAN 分類也包含 OLED

CAN 第二頁除較大尺寸 TFT，亦列 WLEP02566400DGAAASA00 與一款 OLED demo。前者的詳細頁明示 3.55 吋、256×64、CANopen、白色被動矩陣 OLED 與 4-bit 灰階，因此 CAN 類別不能一概稱為彩色 TFT。[CAN 第二頁](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/page:2.html)、[OLED 詳細頁](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/smart-oled.html)。

此 OLED 的分類表與詳細頁有尺寸欄位差異，尚不適合作為機構設計依據，詳見[來源與規格差異](research/02-sources-and-open-questions.zh-TW.md)。

## 兩種開發路線

| 路線 | 使用方式 | 工程評估重點 |
| --- | --- | --- |
| 標準韌體與 GUI Builder | 編輯頁面、圖資、元件及支援的通訊綁定，主控交換命令或資料 | 元件限制、資料模型、工具／韌體版本、更新與復原 |
| 自行開發 STM32 韌體 | 取得對應硬體的初始程式與支援資料，使用自己的應用程式與圖形框架 | BSP、顯示與觸控驅動、通訊堆疊、授權、下載及量產維護 |

型號頁表示購買後可聯絡取得供 TouchGFX 開發的 initial code；CAN、RS485、UART 系列頁亦提到 LVGL。這支持「可評估自行開發」的路線，沒有證明 GUI Builder 專案能轉成 LVGL／TouchGFX 原始碼，或替換韌體後仍保留原廠協定。[CAN 型號頁](https://www.winstar.com.tw/zh-tw/products/smart-display/can-display/4_3.html)、[UART 系列頁](https://www.winstar.com.tw/zh-tw/products/smart-display/uart_display.html)。

總覽另外提及 Qt 與 Embedded Wizard；此總體敘述不足以確認任一 SKU 已提供可用 BSP、授權與資源配置，應另索取型號級相容矩陣。[總覽](https://www.winstar.com.tw/zh-tw/products/smart-display.html)。

## 對原 MCU HMI 比較的補充

1. 華凌應以 CAN、RS485、UART 多路線的整合模組供應商納入比較，而非只以 CAN 顯示器代表全部產品。
2. GUI Builder 有模擬、圖資更新與通訊綁定，應與其他工具的完整交付流程比較；不能只比較畫面排版或 widget 數量。
3. Flash 容量與通訊系列會影響專案限制，不能把支援 32 MB 系列的上限套用到 16 MB。
4. 與 DWIN／Levetop 比較時，應計入主控協定及換模組成本；與 LVGL／TouchGFX 比較時，則要區分採標準韌體或自寫韌體。

以上為工程判斷；跨廠商能力沿用[原研究的日期與證據](../mcu-embedded-hmi-research/research/01-product-comparison.zh-TW.md)，本次未全面更新其他七項方案。

## 文件導覽

| 文件 | 內容 |
| --- | --- |
| [GUI Builder 與整合評估](research/01-gui-builder-and-integration.zh-TW.md) | 版本、容量、資料映射、評測與商用條件 |
| [來源與待確認事項](research/02-sources-and-open-questions.zh-TW.md) | 指定網址覆蓋、官方連結、日期及規格差異 |

本版依公開資料整理，未安裝 GUI Builder 或完成硬體測試。對華凌、EDT 與其他廠商採相同標準；廠商所稱降低成本、縮短時程或量產驗證，不直接構成獨立的效能與可靠性結論。
