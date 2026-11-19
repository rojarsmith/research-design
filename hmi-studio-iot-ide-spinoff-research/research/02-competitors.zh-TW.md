# 全球同類競品比較

查核日期：2026 年 11 月 19 日。價格多來自第三方整理或廠商頁面，各來源常有出入，**只作量級參考**；每筆的來源列在[來源清單](03-sources-and-open-questions.zh-TW.md)。LVGL Pro 與 MicroEJ 另有專文：[LVGL Pro](04-lvgl-pro-and-paid-value.zh-TW.md)、[MicroEJ](05-microej-comparison.zh-TW.md)。

## 市場地圖

一個連網、有螢幕的裝置要完成，需要四層：

```
[ 裝置 UI ] — [ 裝置應用邏輯／並行 ] — [ 現場通訊（Modbus/CAN） ] — [ 上雲／App／OTA ]
```

現有產品大多只覆蓋一到兩層，或者把多層綁在自家硬體或雲端上：

| 類別 | 代表產品 | 覆蓋層 | 綁定 |
| --- | --- | --- | --- |
| A. 嵌入式 UI 工具 | LVGL Pro、SquareLine、Embedded Wizard、Qt for MCUs、Crank Storyboard、Slint | UI（部分含簡單邏輯） | 不綁硬體，按席或按產品收費 |
| B. 晶片商免費工具 | ST TouchGFX、NXP GUI Guider、ST、NXP、Microchip、Nordic、Espressif 的 VS Code 擴充 | UI 或韌體開發 | 綁自家晶片 |
| C. IoT 開發平台 | **MicroEJ**、Arduino App Lab 與 Cloud、Particle、Golioth、ESP RainMaker、Tuya | 韌體、上雲、OTA、App | 多半綁自家硬體或雲端 |
| D. IoT 雲與儀表板 | Blynk、ThingsBoard、Ubidots、Datacake、FlowFuse（Node-RED） | 上雲、儀表板、手機 App、流程 | 不碰裝置端 UI |
| E. 工業 HMI 與 SCADA | Weintek EasyBuilder Pro、Ignition、CODESYS Visualization | 畫面、PLC 通訊、上雲 | Weintek 綁自家 HMI；Ignition 綁伺服器；CODESYS 按 runtime 收費 |
| F. 串口屏編輯器 | DWIN、Nextion、Winstar、4D Systems Workshop4 | UI 加串口指令 | 綁自家模組（編輯器免費） |

**空隙**：MicroEJ 是少數覆蓋四層的付費平台，但它依賴自家虛擬機、以 Pro Code 為主、走企業銷售。以 Low Code 把四層用同一個模型串起來、產出可帶走的純 C、又不綁硬體或雲端的工具很少。EHS 的 Tag 模型與 Logic 和協定解耦，剛好可以往這裡延伸。

## A. 嵌入式 UI 工具

| 產品 | 廠商 | 收費 | 重點 | 對 EHS 的意義 |
| --- | --- | --- | --- | --- |
| LVGL Pro | LVGL 原廠 | Starter US$990、Developer US$3,990／席／年；Product US$20,000；Renesas 客戶免費 | XML、Subject 綁定、Figma、UI 測試、CLI、MCP、AI agent | 最直接的對手；見專文 |
| SquareLine Studio（Vision） | SquareLine | 個人 Free、US$9、US$19／月；商業版另計 | 拖放 LVGL，有 VS Code 專案匯出；2024 年 1 月與 LVGL 分道揚鑣 | 價格錨點最低 |
| Embedded Wizard | TARA Systems | 約 €199／席／月；第三方資料 US$2,466 至 7,300 | 自有語言 Chora，多晶片 | 歐洲工業客戶多 |
| Qt for MCUs | Qt Group | 商業授權加裝置權利金（未公開） | QML、Design Studio | 高階、昂貴 |
| Crank Storyboard | AMETEK | 訂閱或永久授權，洽詢 | 設計師與工程師協作、Lua | 高階 |
| Slint | SixtyFPS | 每裝置約 US$1 起 | Rust 與宣告式 UI | 新興 |

## B. 晶片商免費工具

| 產品 | 重點 | 意義 |
| --- | --- | --- |
| ST TouchGFX Designer 4.26 | 免費，只能用在 STM32 | EDT 既有的 SE 業務就在用；免費加綁晶片 |
| NXP GUI Guider 2.0（2026-06-30 GA） | 免費，LVGL 9.4，新增 JSON 編輯與即時模擬 | 晶片商也在做 LVGL 編輯器 |
| ST、NXP、Microchip、Nordic、Espressif、Arm Keil 的 VS Code 擴充 | 多數免費；Microchip 部分進階功能需訂閱 | 嵌入式開發已集體移到 VS Code |
| Nordic nRF Connect for VS Code | 內建 Zephyr Thread Viewer，借用 J-Link RTOS plugin | 多執行緒除錯的參考做法 |

## C. IoT 開發平台

| 產品 | 現況 | 意義 |
| --- | --- | --- |
| MicroEJ（VEE 與 SDK 6） | 付費的完整 MCU 裝置平台：MEJ32 虛擬機容器、Java 與 C 等多語言、多應用沙箱、Virtual Device 模擬、MQTT、TLS、BLE、Modbus、CAN、應用層 OTA 與應用商店；支援 IntelliJ、Eclipse、Visual Studio；多年期合約，價格不公開；宣稱 120 家以上客戶、4.5 億台以上裝置 | 「完整 MCU 研發平台可以收費」的最強證據；Pro Code、大型 OEM，與 EHS 的 Low Code、中小型 OEM 不重疊。見[專文](05-microej-comparison.zh-TW.md) |
| Arduino App Lab 與 Arduino Cloud | Qualcomm 2025 年 10 月宣布收購。UNO Q 雙處理器（Linux 加 STM32U585），App Lab 以 Bricks 與 RPC Bridge 整合 Linux、MCU、AI；Cloud Maker 約 US$72／年 | 「一個專案管多處理器、多執行環境」的標竿，但綁 Arduino 硬體 |
| Particle | 免費層；Basic 約 US$299、Plus 約 US$599／月（第三方資料）；有一則來源稱 2026 年被 Digi 收購，未經第二來源證實 | 硬體、連網、雲一體 |
| Golioth | 改為用量計價，個人開發者裝置管理免費；Zephyr 生態 | Zephyr 加雲的代表 |
| ESP RainMaker | 2026-10 推 Matter Fabric；RainMaker Neo 雲端後端開源 | 晶片商把雲也免費化 |
| Tuya | No Code OEM App 與面板；2026-08 推 Tuya AI Coding | 家電 No Code 已很成熟，但綁塗鴉雲 |

## D. IoT 雲與儀表板

| 產品 | 收費 | 意義 |
| --- | --- | --- |
| Blynk | 免費 5 台；Prototype US$99、Production US$199 至 2,099／月，按裝置數 | No Code 手機 App 加雲，不做裝置端螢幕 |
| ThingsBoard | 開源 CE；PE 自架約 US$10 至 1,499／月或永久 US$4,999 起（來源不一）；4.3 版 | 可作為 EHS 的上雲對接目標，不必自己做雲 |
| Ubidots | Professional US$99／月（50 台） | 同上 |
| FlowFuse（Node-RED） | Edge 與 Hub 產品；價格不一 | Node-RED 的流程式 Low Code 是邏輯編輯的參考 |

## E. 工業 HMI 與 SCADA

| 產品 | 收費 | 意義 |
| --- | --- | --- |
| Weintek EasyBuilder Pro | 免費，但只能用在 Weintek HMI；支援 400 種以上 PLC 協定、MQTT、OPC UA、AWS 與 Azure、Weincloud | 「軟體免費、賣硬體」的成功範例，和 EDT 現行模式相同 |
| Inductive Automation Ignition | Perspective 約 US$11,225（伺服器授權） | 工業軟體可以收高價，但需要龐大的整合商生態 |
| CODESYS Visualization | 按 tag 數分級（S 128、M 2048…）；MQTT 需要 IIoT Libraries SL；Automation Server 為 SaaS | 按 runtime 收費的模式 |

## F. 串口屏編輯器

DWIN、Nextion、Winstar、4D Systems、Matrix Orbital 等都提供免費編輯器，靠模組（每片約 US$130 至 470）、文件與客製收入。這是 EDT 原本要切入的市場，也證明「編輯器本身難以單獨收費」。詳見 `edt-hmi-studio-support` 的 serial HMI 報告。

## 對 EHS 分拆的含意

1. **UI 編輯器是商品化的層**：有免費的晶片商工具、US$9 的 SquareLine，還有 LVGL 原廠。單靠 UI 收不到錢。
2. **雲端也在免費化**：RainMaker 開源、Golioth 免費裝置管理、ThingsBoard CE。所以不要自己經營雲，做連接器就好。
3. **可收費的層在中間**：裝置應用邏輯、並行、現場通訊、OTA 的整合，以及「工具產生的程式碼可帶走、不綁硬體」的承諾。
4. **商業模式上，「工具免費、硬體或晶片商付費」是主流**（Weintek、TouchGFX、LVGL 與 Renesas、Arduino 與 Qualcomm）。分拆後的純軟公司要同時經營**席位訂閱**與 **OEM、晶片商授權**兩條收入線，後者可能才是大宗。
