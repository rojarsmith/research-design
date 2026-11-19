# MCU 與外部設備的 PC 端模擬：商業工具與 EHS 的整機模擬

查核日期：2026 年 11 月 19 日。本研究沒有實際試用任何產品，內容依據官方文件與公開報導。價格多數不公開，或不同來源互相矛盾，引用前請向原廠確認。

## 結論

1. **沒有一個商業工具能同時模擬 MCU、外部設備、觸控畫面與雲端，而且用起來友善。** 現有工具各自只模擬一層：晶片、電路、UI、RTOS 或控制演算法。開發流程因此很破碎。
2. **市場分成三種：** 友善但支援的晶片少（Wokwi）；能力強但門檻高（Renode）；車用與晶片等級，很貴（Synopsys、Vector）。
3. **主流做法是指令集層級的模擬**，也就是模擬 CPU 和每個暫存器。這樣很精確，但每一顆晶片、每一個周邊都要另外建模型，所以支援的晶片總是很少，設定也很繁瑣。
4. **EHS 可以走另一條路：行為層級的整機模擬。** 依 [單元匯流排架構](09-mcu-unit-bus-architecture.zh-TW.md)，模擬的對象是匯流排上的單元，不是暫存器。代價是抓不到驅動層的 bug，這部分交給 Renode 這類工具。

## 一、模擬晶片加外部設備

| 產品 | 能模擬什麼 | 上手難度 | 收費 | 限制 |
| --- | --- | --- | --- | --- |
| [Wokwi](https://wokwi.com/pricing) | MCU 加感測器、按鈕、顯示器、LED，以拖拉方式接線；可用 Custom Chips API 自訂晶片 | **最友善**：在瀏覽器或 [VS Code](https://docs.wokwi.com/vscode/getting-started) 裡用，可接 [CI 與 GitHub Actions](https://docs.wokwi.com/wokwi-ci/getting-started) | 開源專案免費，[商業用途要買授權](https://wokwi.com/license)；Pro 每席每月約 US$25，含 CI 時數（另有整理站寫每月 €20，以官方頁面為準） | 以 ESP32、RP2040、AVR 為主，STM32 支援範圍要向原廠確認；CI 在 Wokwi 雲端執行。Espressif 的 [ESP-IDF 官方文件](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/third-party-tools/wokwi.html)也介紹它 |
| [Renode](https://antmicro.com/platforms/renode)（Antmicro） | CPU、周邊、感測器、匯流排，以及多節點系統；感測器資料可用腳本或錄製檔驅動 | 能力最強，但要寫平台描述檔（`.repl`）、腳本（`.resc`）與 Robot Framework 測試，門檻高 | 開源；Antmicro 提供平台移植、周邊建模、CI 等付費服務，價格不公開 | 已有 [STM32H7 實體板加數位雙生](https://antmicro.com/blog/2026/07/stm32h7-renode-reference-platform)；[Google TFLite Micro 用它跑 CI](https://zephyrproject.org/renode-based-ci-for-tensorflow-lite-mcu/)；[ADI 也在用](https://antmicro.com/blog/2026/09/how-adi-is-leveraging-renode-to-accelerate-embedded-intelligence) |
| [Corellium](https://corellium.com/solutions/iot-device-devops) 與 [Arm Virtual Hardware](https://www.arm.com/virtual-hardware) | 雲端上的整板模型，例如 [STM32U5 Discovery](https://support.avh.corellium.com/devices/stm32u5/stm32u5-supported-hardware-components)，含 Wi-Fi 與 MEMS 感測器；可用 MMIO [自訂周邊](https://corellium.com/blog/custom-device-modeling) | 網頁介面，可接 GDB | 企業報價 | 板子種類少；Arm 正在停用 [AVH 的 AWS AMI](https://us-west-2b-production.gl-awslz.arm.com/artificial-intelligence/ethos-u/ml-embedded-evaluation-kit/-/commit/9644be5d8b8ba6a89842ebfd2767ac64a443aa39)，但 [AVH 文件 2026-02 仍有更新](https://arm-software.github.io/AVH/main/overview/html/History.html)，產品方向要留意 |
| [Proteus VSM](https://labcenter.com/whyvsm/)（Labcenter） | MCU 與電路圖一起模擬，類比與數位元件都有 | 老牌，教學與小團隊常用 | 按 MCU 系列買授權，例如 [Microchip 代售的套件](https://microchip.com/en-us/development-tool/SW500220)；[官方價格頁](https://labcenter.com/pricing)沒有列出實際金額 | 查到的資料沒有提到 STM32；不適合連網與 RTOS 的情境 |

## 二、車用與晶片等級的虛擬原型

- [Synopsys Virtualizer 與 Silver](https://www.synopsys.com/blogs/chip-design/future-mobility-arm-zena-soafee-workflows-ces.html) 可以模擬整台車的虛擬 ECU。Synopsys 已和 [Vector 整合 MICROSAR 與 CANoe](https://www.vector.com/en/business-unit/software-platform/resources/vector-synopsys/)，並在 [2026 年 3 月推出電子數位雙生平台](https://news.synopsys.com/2026-03-10-Synopsys-Launches-Electronics-Digital-Twin-Platform-to-Accelerate-Physical-AI-System-Development)。
- 這類工具的對象是車廠與晶片公司，價格與導入成本都不是一般 MCU 團隊負擔得起的。Siemens PAVE360、dSPACE 等同類產品這次沒有查證。

## 三、只模擬某一層

| 層 | 例子 | 缺口 |
| --- | --- | --- |
| UI | LVGL、TouchGFX、SquareLine 的 PC 模擬器；EHS 的 WASM 模擬器 | 沒有外部設備與通訊 |
| RTOS 與應用 | Zephyr `native_sim`、QEMU | 周邊要自己做假的，沒有 UI 設計工具 |
| 平台本身 | [MicroEJ Virtual Device](05-microej-comparison.zh-TW.md) | 只能跑 MicroEJ 的應用 |
| 控制演算法 | Simulink 的 SIL 與 PIL（見 [07](07-c-cpp-rad-tools.zh-TW.md)） | 不管 HMI 與通訊 |

## 四、為什麼開發還是這麼破碎

| 原因 | 說明 |
| --- | --- |
| 模擬層級太低 | 指令集模擬要替每顆晶片、每個周邊建模型，支援範圍永遠追不上晶片商推出新料號的速度 |
| 各層各自為政 | 畫面、RTOS、通訊協定、雲端各有自己的模擬器，時間軸對不起來 |
| 外部設備沒有標準模型 | Modbus 從站、CAN 節點、感測器都要自己寫腳本假裝 |
| 設定門檻高 | Renode 這類能力強的工具，要學平台描述與測試框架才能用 |

這和 [MCU 研發困擾排行](08-mcu-development-pain-points.zh-TW.md)第 7 名「硬體還沒到」直接相關：JetBrains 2025 年調查只有 27% 的團隊用模擬。

## 五、EHS 的機會：行為層級的整機模擬

### 做法

依單元匯流排架構，模擬的對象是匯流排上的單元：

| 單元 | 模擬方式 |
| --- | --- |
| 觸控 GUI | 沿用現有 WASM 模擬器，滑鼠當觸控 |
| 實體按鈕、中斷 | 畫面上的虛擬按鈕，或時間軸上的事件腳本 |
| 感測器 | 數值曲線、隨機雜訊，或真機錄製的資料 |
| Modbus、CAN 設備 | 依單元描述檔自動產生虛擬從站（EHS 已有模擬 Modbus 裝置） |
| MQTT、HTTP 雲端 | 內建本機 broker，或接真的雲端 |
| 交換中心的規則 | 和真機跑同一份規則，不需要模擬 |

### 和現有工具的差異

| 比較 | 指令集模擬（Renode、Corellium） | Wokwi | EHS 整機模擬 |
| --- | --- | --- | --- |
| 新晶片支援 | 要建模型 | 要原廠支援 | 不受影響，因為不模擬晶片 |
| 外部設備 | 要寫模型或腳本 | 有零件庫，偏向 Maker 零件 | 由單元描述檔自動產生，偏向工業設備 |
| UI、通訊、規則在同一個時間軸 | 否 | 部分 | 是 |
| 錄製重播 | 部分支援 | 否 | 是，匯流排流量本身就是錄製格式 |
| 抓驅動層 bug | **是** | 部分 | **否** |

### 建議

- 列入 [收費化路線圖](10-ehs-paid-product-roadmap.zh-TW.md)第三節的第一優先功能，和 [熱重載](11-mcu-hot-reload.zh-TW.md)、錄製重播合成同一套開發迴圈：**在 PC 上把整台設備跑起來，改了就在真機上看到，出問題就重播。**
- 不要自己做指令集模擬器。驅動層交給 Renode：EHS 可以匯出 Renode 平台描述與測試腳本，或把 Renode 當成可選的後端。
- 第一版先做 GUI、按鈕、Modbus 從站、感測器曲線四種模擬單元，因為 EHS 已經有前兩項與 Modbus 模擬的基礎。
- 模擬的情境要能存成測試，在 CLI 與 CI 上重跑。這是工程師願意付費、而且 Wokwi 要收 CI 時數費用的部分。

## 六、待驗證

- Wokwi 對 STM32 的支援範圍與目前的官方價格。
- Renode 是否支援 EHS 現有的三塊板子（STM32F746G-DISCO、STM32H747I-DISCO、STM32U599），以及匯出平台描述要花多少工。
- 行為模擬和真機的差距：同一組規則與事件，在模擬器與真機上的時序差多少，要列入 [匯流排原型量測](09-mcu-unit-bus-architecture.zh-TW.md)。
- 客戶是否願意為整機模擬單獨付費，還是只當作 Pro 方案的一部分，要在收費化路線圖第七節的客戶訪談中一併詢問。
