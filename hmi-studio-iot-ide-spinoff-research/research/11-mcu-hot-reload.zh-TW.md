# MCU 上的熱重載：商業產品與 EHS 的機會

查核日期：2026 年 11 月 19 日。本研究沒有實際試用任何產品，內容依據官方文件與公開報導。部分功能廠商標為實驗性質，引用前請向原廠確認。

## 結論

1. **沒有商業產品能在 MCU 上熱重載 C 程式碼。** 桌機上的 Live++ 很成熟，但沒查到支援 Cortex-M。
2. **商業產品的熱重載都是「C 韌體固定，替換上面那層可載入的東西」**，例如 bytecode、腳本、XML 格式的 UI 或容器。
3. **目前沒有商業工具能讓畫面、規則與通訊綁定在真機上存檔就生效，C 核心維持不動。** 這是 EHS 依 [單元匯流排架構](09-mcu-unit-bus-architecture.zh-TW.md) 演進後可以補上的位置，可以和整機模擬、錄製重播合成同一套開發迴圈，作為收費功能。

## 一、熱重載分兩種，不要混在一起

| 類型 | 目的 | 典型做法 |
| --- | --- | --- |
| 開發時熱重載 | 縮短「改一行到看到結果」的時間 | 存檔後在數秒內推到真機，不必重燒、不必重開機 |
| 出貨後動態更新 | 現場更新應用或功能，不必更新整份韌體 | 沙箱應用的安裝與移除、App Store、部分 OTA |

兩者底層機制相近，賣點卻不同：前者賣給工程師，後者賣給產品管理與維運。

## 二、商業產品

| 產品 | 熱重載的是什麼 | 收費 | 注意 |
| --- | --- | --- | --- |
| [Toit 與 Jaguar](https://github.com/toitlang/jaguar/) | 整個 Toit 應用，透過 Wi-Fi 推到 ESP32；`jag watch` 存檔後自動重載，官方說不到 2 秒，不必重燒、不必重開機 | 語言與 Jaguar 開源，原本付費的是雲端裝置群管理與 OTA；Toitware 據報已在 2024 年停止營運，現為社群維護（見 [06](06-dynamic-language-mcu-products.zh-TW.md) 第五節） | 開發體驗最接近「真的熱重載」，但只支援 ESP32，也不管 UI 設計 |
| [MicroEJ Kernel 與 Features](https://repository.microej.com/packages/training/kernel/DEV-M0127_0001_Kernel_Development_2.1_20260313.pdf) | 沙箱應用可在執行中安裝、啟動、停止、移除，Kernel 會先檢查相容性；另有 [Binary Loader](https://www.microej.com/licensing/binary-loader-ip/) | 多年期合約，價格不公開（見 [05](05-microej-comparison.zh-TW.md)） | 偏向出貨後動態更新與 App Store，不是開發時的即時迴圈 |
| [Moddable Mods](https://moddable.com/documentation/xs/mods) | 預先編譯的 JavaScript bytecode 加上圖片、憑證等資源，原廠韌體不動；可用 Hardened JavaScript 沙箱隔離 | 每台 US$0.50 的商業授權 | 官方早期稱 Mods 為實驗功能；[runmod](https://github.com/phoddie/runmod) 範例透過 WebSocket 安裝，並註明不是正式產品程式碼 |
| [LVGL Pro 執行期 XML 載入](https://docs.pro.lvgl.io/integration/xml.html) | 只換 UI：XML 或打包檔可從 SD 卡、序列埠、藍牙、Wi-Fi 或伺服器載入，執行中建立畫面 | 隨 LVGL Pro 授權 | 需要自訂 C 程式的元件無法載入；Pro 文件說這個引擎不公開，[開源文件](https://docs.lvgl.io/master/details/other-components/xml.html)卻寫在函式庫裡，**授權說法互相衝突，要向原廠確認** |
| [Crank Storyboard](https://resources.cranksoftware.com/cranksoftware/v3.2.0/docs/ch01s03.html) | UI 與 Lua 腳本和執行引擎分開；Designer 可直接部署到目標板 | 付費 | 查到的文件只說 Lua 在**啟動時**載入，沒有證據支持執行中熱重載 |
| [TI C2000 LFU](https://ti.com/lit/spruiu9)、[Microchip PIC32MZ Live-Update](https://www.digikey.com/en/articles/how-to-perform-firmware-updates-without-halting-firmware-execution) | 整份韌體在現場切換，可以不重置裝置 | 晶片內建功能 | 是現場不停機更新，不是開發時熱重載 |

## 三、開源或研究階段（不是商品）

| 項目 | 做法 | 狀態 |
| --- | --- | --- |
| [ESP-IDF `hotreload` 元件](https://components.espressif.com/components/igrr/hotreload/versions/0.9.0/readme) | 執行中載入位置無關的 C 程式碼，可透過 HTTP 重載 | 開發活躍，仍是 0.x 版 |
| [Zephyr LLEXT](https://docs.zephyrproject.org/latest/services/llext/index.html) | 執行中載入、連結、卸載 ELF 擴充；支援 ARM、RISC-V、Xtensa（ARC 為實驗性質） | Apache 2.0；有工業場合評估，但沒查到正式量產案例與簽章機制的說明 |
| [FPBInject](https://osrtos.com/projects/fpbinject) | 利用 Cortex-M 的 FPB 硬體，透過序列埠在執行中替換任一函式 | 開發工具，授權未查到 |
| [HERA](https://www.ndss-symposium.org/wp-content/uploads/2021-159b-paper.pdf) | 用 Cortex-M 內建硬體做熱修補 | 學術研究 |
| Atym 與 Ocre（見 [06](06-dynamic-language-mcu-products.zh-TW.md)） | 熱部署 WebAssembly 容器 | Ocre 開源，付費的是 Atym Hub 編排平台 |
| [Live++](https://blog.jetbrains.com/dotnet/2024/09/24/hot-reloading-cplusplus-with-liveplusplus-and-jetbrains-rider/) | 桌機與遊戲主機上的 C++ 熱重載 | 商業產品，但沒查到 MCU 支援 |

## 四、為什麼 C 很難熱重載

- 函式位址在連結時固定，換掉一個函式就要處理所有呼叫點與函式指標。
- 全域變數與堆疊上的狀態，換程式之後不一定還對得上。
- 程式碼大多在 Flash 上執行，寫 Flash 時通常不能同時從同一區執行。
- 中斷與 RTOS Task 可能正好在執行被換掉的程式。

LLEXT、ESP-IDF `hotreload`、FPBInject 都是用「限定邊界」來處理這些問題：固定的 API 表、位置無關的程式碼、硬體斷點重導。商業產品乾脆不換 C，改換 bytecode 或資料。

## 五、對 EHS 的意義

### 熱重載的範圍

依單元匯流排架構，把專案分成「可熱重載」與「固定」兩層：

| 層 | 內容 | 熱重載方式 |
| --- | --- | --- |
| 可熱重載 | 畫面（LVGL XML 或 EHS 自己的畫面描述） | 重新載入畫面描述，重建畫面 |
| 可熱重載 | 交換中心的規則（觸發 → 條件 → 動作） | 開發時跑選配的 bytecode VM；量產時可改為編譯成 C |
| 可熱重載 | Tag、單元參數、通訊對應（例如 Modbus 暫存器位址） | 本來就是資料，重新載入表格即可 |
| 固定 | 驅動、單元 Task、匯流排本體、RTOS | 重新建置並燒錄 |

這樣一來，時序安全與 MISRA 這類論述只需要套在固定層。可熱重載層由 IDE 在推送前做型別與衝突檢查。

### 開發迴圈

1. 在 EHS 改畫面或規則，存檔。
2. IDE 檢查規則與單元描述檔是否相容，再把差異打包。
3. 透過除錯器、序列埠或網路推到真機，交換中心在兩次規則評估之間換上新版本，匯流排上的狀態保留。
4. 同一個包也能推到 WASM 模擬器，錄下的匯流排流量可以拿來重播比對。

### 和對手的差異

| 對手 | 缺口 |
| --- | --- |
| Toit | 只支援 ESP32，不管 UI 設計，要學新語言 |
| MicroEJ | 要用 Java VM，偏向出貨後更新，價格不公開 |
| LVGL Pro | 只能換 UI，引擎授權說法不清 |
| 晶片商工具、IAR、Keil | 改 C 就要重新建置、重新燒錄 |

### 建議

- 列入 [收費化路線圖](10-ehs-paid-product-roadmap.zh-TW.md) 第三節，和「匯流排錄製重播與整機模擬」合成同一個收費賣點：**改了就在真機上看到，出問題就重播。**
- 第一版只做畫面與 Tag 參數，因為兩者本來就是資料。規則熱重載要等 bytecode VM 原型量測過記憶體與延遲再做。
- 量產韌體預設關閉熱重載介面。若開放給出貨後使用，必須搭配簽章、版本檢查與回滾，並納入 CRA 要求的弱點處理流程。

## 六、待驗證

- LVGL 執行期 XML 載入引擎的授權範圍：開源函式庫可用，還是 LVGL Pro 專屬。
- 規則 VM 在 STM32F7 等級 MCU 上的記憶體占用，以及一次規則評估的最差延遲。
- 熱重載時，正在進行的事件與畫面動畫如何收尾，才不會出現半新半舊的狀態。
- 工程師是否真的願意為熱重載付費，要在收費化路線圖第七節的客戶訪談中一併詢問。
