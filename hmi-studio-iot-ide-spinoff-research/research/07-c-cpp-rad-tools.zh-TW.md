# 以 C/C++ 為輸出的新式 RAD 付費工具

查核日期：2026 年 11 月 19 日。不包含 IAR、Keil 這類傳統 IDE。價格多取自廠商頁面或第三方整理；Edge Impulse、Simulink、Altia 沒有官方公開價，引用前請向原廠確認。

## 結論

1. **RAD 工具分成兩派，各管一半。** UI 派（LVGL Pro、Embedded Wizard、Altia、AppWizard）與邏輯派（QM、itemis CREATE、Simulink）分開。很少有工具把「UI、行為邏輯、通訊、多執行緒」放在同一個 Low Code 模型裡，這呼應主報告的定位。
2. **QP 的 Active Object 模型可以直接借用。** 把 EHS 的 Logic 圖升級成「每個 Task 是一個帶事件佇列的 Active Object」，就有成熟理論支撐多執行緒的安全性，也比自己發明模型容易向工程師解釋。
3. **收費的共同點：工具便宜或免費，執行環境或量產收費。** QM 免費、QP 收商業授權；Slint 每台約 US$1 起；Moddable 每台 US$0.50；emWin 透過晶片商授權。EHS 的量產授權可以參考這種設計，不一定只靠席位收費。

## 一、視覺化 GUI 設計，產生 C/C++

[全球競品比較](02-competitors.zh-TW.md)已涵蓋 LVGL Pro、SquareLine、Embedded Wizard、Qt for MCUs、Crank Storyboard，這裡補上其他三個：

| 產品 | 輸出 | 收費 | 特點 |
| --- | --- | --- | --- |
| [Altia Design + DeepScreen](https://altia.com/2022/11/17/designing-guis-for-microcontroller-units/) | 量產等級 C | 依專案報價，不公開 | 主打汽車與工業，支援 STM32、Infineon XMC7200 |
| [SEGGER emWin AppWizard](https://www.segger.com/pricelist-emwin.html) | emWin C | 不單賣，綁在 emWin PRO 套件裡（約 US$14,980）；Infineon 晶片可免費用 | 老牌圖形庫加上 RAD 介面；常透過晶片商以優惠價授權 |
| [Slint](https://slint.dev/pricing) | C++ 與 Rust API，宣告式 UI | 嵌入式必須買商業授權，**每台約 US$1 起**，也可以買斷 | 新世代的宣告式 UI，按裝置數收費 |

## 二、狀態機與模型驅動，產生 C/C++（邏輯的 RAD）

| 產品 | 方式 | 收費 | 與 EHS 的關聯 |
| --- | --- | --- | --- |
| [Quantum Leaps QM + QP](https://www.state-machine.com/licensing) | 畫階層式狀態機，產生 C/C++，跑在 QP 即時事件框架上 | QM 工具免費；QP 框架是 GPL 與商業授權二選一（以每項產品授權），閉源產品必須買商業授權，價格公開在官網 | **最值得參考**。QP 的核心是 Active Object：每個物件有自己的事件佇列與執行緒，用訊息傳遞取代共用記憶體，和主報告的「Task 加 Tag Store」同一個思路 |
| [itemis CREATE](https://www.itemis.com/en/products/itemis-create/licenses)（原 YAKINDU） | 狀態圖產生 C、C++、Java、Python 等；2026-07 的 5.6.3 版改善了 MISRA C++:2023 相容性 | 訂閱（最短一年）或洽詢買斷 | 只管邏輯建模，不管 UI，可以和 UI 工具並用 |
| [MathWorks Simulink + Embedded Coder](https://nl.mathworks.com/products/new_products/r2026a-transition.html) | 方塊圖模型產生量產 C；R2026a 起 STM32 支援改成獨立的 STM32 Microcontroller Blockset | 昂貴，需洽詢；學生版與 Home 版不含 Embedded Coder | 控制演算法的業界標準，但不適合用來做 HMI |
| IAR Visual State | 狀態機產生 C | 付費 | 屬於 IAR 生態，不展開 |

## 三、AI 模型，產生 C++

[Edge Impulse](https://www.edgeimpulse.com/plans)（已歸 Qualcomm）用網頁介面收資料、訓練模型，再匯出可在任何 MCU 上編譯的 C++ 函式庫。

- 個人與學術用的 Developer 方案免費，企業方案需洽詢。
- 第三方報價差很大：[Vendr](https://www.vendr.com/marketplace/edge-impulse) 列每年 US$4,980 至 14,100；[AWS Marketplace](https://aws.amazon.com/marketplace/pp/prodview-rli2dgqql7ofg) 上有一個 12 個月、1 位使用者的方案標價 US$36,699。

## 四、新式 IDE 外掛（不是 RAD，但付費模式值得參考）

[VisualGDB](https://visualgdb.com/buy)（Sysprogs）是付費的 Visual Studio 嵌入式外掛，有 Embedded、Custom、Ultimate 等版本，這次沒查到目前價格。

- [Live Tracing](https://visualgdb.com/documentation/tracing/) 與 FreeRTOS 即時狀態檢視要 Custom 版以上才有。
- 它證明「付費 IDE 外掛加多執行緒除錯」可以活很多年，是 VS Code 付費擴充路線最接近的先例。
