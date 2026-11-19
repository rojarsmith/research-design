# MCU 軟韌體開發：錢收在哪裡

查核日期：2026 年 11 月 19 日。本文綜合 04 至 13 號研究，並補查 Keil、ThreadX、IAR、Memfault 的授權與計價。價格多數不公開或來自第三方，引用前請向原廠確認。靜態分析與測試工具的價格這次沒有查證。

## 結論

**MCU 軟韌體開發的錢，現在幾乎不收在寫程式的編輯器上。** IDE、HAL、程式碼產生器、VS Code 擴充、GCC、FreeRTOS 與 Zephyr 大多免費；晶片商也把工具送給客戶，好帶動晶片銷售。

收費集中在四類：

1. **有人要負法律責任的地方**：功能安全認證、合規證據。
2. **量產出貨的數量**：每台權利金、每產品授權。
3. **出貨後的維運**：OTA、裝置群管理、可觀測性、資安。
4. **稀缺的專業能力**：除錯與追蹤工具、移植與整合服務。

EHS 的收費點應該放在**量產授權、出貨後管理、整機除錯**，而不是編輯器本身。

## 一、收費重點

| 收費重點 | 典型產品 | 怎麼收 | 為什麼收得到錢 |
| --- | --- | --- | --- |
| 1. 功能安全與認證 | IAR 與 Keil 的功能安全版編譯器、Green Hills、TASKING；SAFERTOS、embOS 等認證版 RTOS；ThreadX 的安全文件 | 席位訂閱或年費；認證文件另外授權 | IEC 61508、ISO 26262 等標準要求工具或元件有認證，免費的 GCC 與 FreeRTOS 拿不到這張證書 |
| 2. 測試、靜態分析與合規 | MISRA 檢查（PC-lint Plus、Polyspace、Parasoft、LDRA）、Coverity 與 Black Duck、單元測試與覆蓋率工具 | 席位或專案授權（價格未查證） | 法規、客戶稽核與歐盟 CRA 要求有證據可交 |
| 3. 量產的執行環境授權 | emWin、Embedded Wizard、Qt for MCUs、Slint（每台約 US$1 起）、Moddable（每台 US$0.50）、QP 商業授權、LVGL Pro Product（每產品 US$20,000）、MicroEJ | 每台權利金，或按產品收費 | 錢跟著出貨量成長，客戶也比較能接受「賣多少付多少」（見 [06](06-dynamic-language-mcu-products.zh-TW.md)、[07](07-c-cpp-rad-tools.zh-TW.md)、[04](04-lvgl-pro-and-paid-value.zh-TW.md)） |
| 4. 出貨後的維運 | OTA 與裝置群管理、現場可觀測性（Memfault）、資安弱點管理、SBOM | 每台每年，或按裝置數 | Memfault 2025 年調查：38% 的公司一年內發生過召回；CRA 在 2027 年 12 月全面適用，管理層有預算（見 [08](08-mcu-development-pain-points.zh-TW.md)） |
| 5. 除錯與追蹤 | SEGGER J-Link、Lauterbach TRACE32、Percepio Tracealyzer、VisualGDB 的 Live Tracing | 硬體加軟體授權 | 除錯是排名第一的痛點，時序與並行 bug 沒有好工具就看不見 |
| 6. 移植、整合與顧問服務 | Antmicro（Renode 平台移植）、MicroEJ 的 VEE Port、晶片商的設計夥伴 | 專案報價 | BSP 移植和協定整合很花工時，人才稀缺（見 [12](12-mcu-device-simulation.zh-TW.md)、[13](13-app-layer-separation-and-languages.zh-TW.md)） |
| 7. 特定領域的工具 | Edge Impulse（邊緣 AI）、Simulink 加 Embedded Coder（控制演算法）、Wokwi 與 Corellium（模擬加 CI 時數） | 企業方案或訂閱 | 解決單一高價值問題（見 [07](07-c-cpp-rad-tools.zh-TW.md)、[12](12-mcu-device-simulation.zh-TW.md)） |

## 二、補查的四個例子

### Keil MDK：免費給非商業，商業要訂閱

- [Keil MDK v6 Community 版](https://keil.arm.com/mdk-community)供非商業專案免費使用；在 VS Code 安裝 Keil Studio Pack 會自動取得 Community 授權。
- 商業用途要買 [Essential 或 Professional 版](https://store.arm.com/mdk-6/)。Arm 商店列的是每人每月 US$99 與 US$199；Professional 多了 Arm Virtual Hardware 與功能安全版編譯器。
- Community 授權是否每年要更新，Arm 2022 年的公告與 Renesas 的說明不一致。

模式：**工具對學習者與原型免費，商業用途按人訂閱，功能安全另外加價。**

### ThreadX：程式碼開源，安全認證文件收費

- Microsoft 把 Azure RTOS 捐給 Eclipse 基金會，[Eclipse ThreadX](https://threadx.io/faq/) 現在以 MIT 授權免費使用。
- 2024 年 10 月成立的 [ThreadX Alliance](https://newsroom.eclipse.org/node/41004) 是取得安全文件的途徑：要做安全認證產品的公司，必須加入聯盟並授權安全文件包。收入用來支付後續版本的認證。
- 特定版本經 SGS-TÜV Saar 依 IEC 61508、IEC 62304、ISO 26262、EN 50128 認證。

這是「**程式碼免費，責任收費**」最清楚的例子。

### IAR：功能安全版與訂閱制

- [IAR 的功能安全版](https://blog.iar.com/products/requirements/functional-safety/iar-embedded-workbench-for-arm-functional-safety)經 TÜV SÜD 依 IEC 61508、ISO 26262 認證，附認證書、TÜV 報告與安全手冊；更新只修正錯誤，以維持認證狀態。
- IAR 正從永久授權轉向訂閱平台。購買頁目前放在 [qt.io](https://www.qt.io/development/iar-embedded-tools/how-to-buy) 上，顯示 IAR 已併入 Qt Group。
- 功能安全版的價格沒有公開。
- 一頁寫目前認證版本是 9.20.3，另一則 2024 年的公告是 9.50.3，要向原廠確認。

### Memfault：按裝置數收費，OTA 內含

- [官方價格頁](https://memfault.com/pricing/)有免費的 Developer 方案，10 台開發裝置以內不收費。
- [報價頁](https://go.memfault.com/get-a-quote-scale)說是**每台固定費率**，不按資料量或 coredump 數計價，裝置愈多單價愈低，而且每個方案都包含 OTA。
- [Toolradar](https://toolradar.com/tools/memfault/pricing) 列出付費方案每月 US$3,495 至 6,695，但其他整理站都寫「客製報價」，只有這一個來源。

模式：**出貨後的錢按裝置數收，OTA 當成附送，賣點是可觀測性。** 這也提醒 EHS：單獨賣 OTA 很難收費。

## 三、正在變免費的部分

| 項目 | 現況 |
| --- | --- |
| IDE 與編輯器 | 晶片商 IDE 與 VS Code 擴充免費；PlatformIO 把付費功能開源；Keil 對非商業免費 |
| 一般 RTOS | FreeRTOS、Zephyr 免費；ThreadX 改為 MIT 授權。收費的只剩**認證** |
| 單純的 UI 編輯器 | SquareLine 很便宜，晶片商有免費工具，LVGL Pro 有免費的 Community 版 |
| 單純「更好的語言」 | MicroPython 幾乎沒人靠它賺錢；Toitware 據報已在 2024 年停止營運；Microsoft 的 DeviceScript 已封存（見 [06](06-dynamic-language-mcu-products.zh-TW.md)） |
| 單純的 OTA | Memfault 把 OTA 內含在每個方案裡；開源有 MCUboot 與 hawkBit |

## 四、規律

1. **責任在誰身上，錢就在那裡。** 認證、合規、稽核證據，這些客戶不能自己隨便做。ThreadX 是最清楚的例子：程式碼免費，認證文件收費。
2. **跟著量產走，不跟著開發者人數走。** 開發者只有幾個人，出貨卻是幾萬台。按台或按產品收費，天花板比按席位高。
3. **出貨後的錢比開發時多。** 開發工具是一次性的；可觀測性、資安、裝置群管理每台每年都要付。
4. **晶片商會把能幫它賣晶片的工具做成免費。** 晶片商願意免費提供的東西不要賣，要賣晶片商沒興趣做、或做不好的跨晶片那一層。

## 五、對 EHS 的意義

| 收費重點 | EHS 能不能做 | 判斷 |
| --- | --- | --- |
| 量產授權 | 可以：每產品授權，或每台權利金 | **要做**，[收費化路線圖](10-ehs-paid-product-roadmap.zh-TW.md)已列入 |
| 出貨後維運 | 只做 OTA 串接與 SBOM 輸出，雲端交給別人 | **要做**，但 OTA 本身難單獨收費，要包進 Product 方案 |
| 除錯 | 匯流排錄製重播、[整機模擬](12-mcu-device-simulation.zh-TW.md)、[熱重載](11-mcu-hot-reload.zh-TW.md) | **要做**，這是差異化的核心 |
| 合規 | 產生的 C 碼符合 MISRA、輸出 SBOM、測試情境能在 CI 上跑 | 可以做到「讓客戶交得出證據」，但 EHS 本身不該去拿功能安全認證，成本太高 |
| 移植與服務 | 板級支援包、垂直範本、客製連接器 | 可以做，而且 EDT 本身就是第一個 OEM 客戶 |
| 功能安全認證工具 | 不適合 | **不做** |
| 單純的 UI 編輯器 | 正在變免費 | **不要拿它收費** |

## 六、待驗證

- 靜態分析與測試工具（PC-lint Plus、Polyspace、Parasoft、LDRA、VectorCAST 等）的現行價格與授權。
- IAR 功能安全版的價格，以及併入 Qt Group 後的授權變化。
- Memfault 付費方案的實際報價。
- Keil MDK Community 授權的更新規則。
- ThreadX Alliance 的會費與安全文件授權條款。
