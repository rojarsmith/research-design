# 程式碼耦合與可沿用資產盤點

盤點日期：2026 年 11 月 19 日。對象是本機 `edt-hmi-studio` repo（版本 0.9.x）。路徑都相對於該 repo 根目錄。只讀取，未修改。

## 總結

| 類型 | 綁定對象 | 嚴重度 | 去除難度 |
| --- | --- | --- | --- |
| 法律 | EDT（EULA、程式內授權文字） | **高**：目前條文禁止用在非 EDT 硬體 | 低（改文字），但需要 EDT 授權 |
| 品牌 | EDT（名稱、格式 ID、環境變數、logo） | 中 | 低至中（需要舊格式遷移） |
| 硬體 | EDT EVK043027B（1 塊板，含 BSD-3 的 EDT BSP） | 低 | 低 |
| 工具鏈 | ST（STM32CubeCLT、Programmer、HAL） | **高**（技術面） | 中至高 |
| 作業系統 | Windows（PowerShell 建置腳本） | 中 | 中 |
| UI 函式庫 | LVGL 9.x | 結構性，不打算去除 | — |

**沒有**：執行期授權金鑰、啟用機制、加密狗、裝置檢查、EDT 專屬晶片（Levetop 只出現在文件，程式碼沒有）。

## 板子與目標硬體

- **板子清單**：`src/types/hmi.ts` 的 `SUPPORTED_BOARDS`（約第 548 至 897 行）是一個靜態陣列。
  - `stm32f746g-disco` 是預設板。
  - `stm32h747i-disco`。
  - `edt-evk043027b`（STM32U599）。
  - 另有 `virtual`。
- **板子定義的內容**：解析度、色彩格式、方向、Flash 與 RAM、燒錄器比對字串、MCU deviceId、外部 Flash 與 ST `.stldr` 載入器、連接埠與其承載的協定、影音上限，以及必須與 `firmware/<board>/include/lv_conf.h` 一致的 LVGL 設定。
- **Virtual Model**：由 `src/types/boardOf.ts` 依專案內規格合成。文件定位為「客戶即將請 EDT 製作的 LCM 規格」，屬於 EDT 的商業語境，但機制本身中立。
- **散落在清單外的板子知識**：
  - `server/hmi/externalFlashPart.ts`
  - `src/budget/panelBudget.ts`
  - `src/resources/builtinFonts.ts`
  - `src/utils/deadRows.ts`
  - `server/gallery/galleryFolder.ts`
  - `src/assistant/tools.ts`
  - `examples/` 下 28 個以板子命名的範例

  多晶片化時，要把這些收斂成板子描述檔（例如 JSON 或 Zephyr board 名稱加上補充欄位）。

## 韌體

- **共用 runtime**：`firmware/common/`，包含 `hmi_runtime`、`hmi_driver_*`、`hmi_framer_*`、`modbus_rtu*`。
- **移植層很薄**：
  - `hmi_platform.h`：只有毫秒 tick。
  - `hmi_transport.h`：7 個位元組串流操作，註解寫明「UART、USB 虛擬 COM，之後可能是 socket」。

  所以移植到 Zephyr 或 FreeRTOS 的成本主要在 BSP 與建置，不在 runtime。
- **執行模型**：每塊板的 `main.c` 都是 `for(;;) lv_timer_handler()` 的裸機 super-loop。EVK BSP 的 FreeRTOS 部分被刻意排除（`firmware/edt-evk043027b/include/main.h`）。
- **EDT 程式碼**：`firmware/edt-evk043027b/vendor/` 內有 EDT 觸控與 BSP（`edt_bsp_ctp.c`、`ctp/mxt336u.*`）。
  - 授權是 **BSD-3-Clause，© 2023 Emerging Display Technologies Corp.**，含不得背書條款，可以再散布。
  - 同一目錄還有 ST 的 `mx25lm51245g` Flash 驅動。

## 建置與燒錄

- **工具鏈**：STM32CubeCLT（arm-none-eabi-gcc、CMake、Ninja）。`server/hmi/service.ts` 以 Windows PowerShell 執行每塊板的 `scripts/build.ps1`。
- **相依套件**：`bootstrap-deps.ps1` 從 GitHub 抓固定版本的 ST HAL、CMSIS、USB device stack 與 LVGL。
- **燒錄**：經 ST-LINK SWD 執行 `STM32_Programmer_CLI.exe`，燒錄前先比對探針與板子。
- **IoT 化的必要工作**：
  - 建置抽象成「目標描述 → 產生 CMake 或 west 專案 → 呼叫對應工具鏈」。
  - 燒錄器抽象化，支援 OpenOCD、J-Link、esptool、pyOCD。
  - 腳本改成跨平台（Node 或 Python）。

## 通訊與標籤模型

- **協定**：`ProtocolId = 'modbus-rtu' | 'uart-command' | 'can-bus'`。
  - Modbus RTU 主端與 UART 指令協定可以運作。
  - CAN 只有 loopback，訊號尚未送到元件。
- **沒有**：Modbus TCP、MQTT、BLE、Wi-Fi、HTTP、lwIP、OPC UA。H747 的乙太網路標示為 `not-compiled`。
- **標籤模型**：`src/types/connections.ts` 的 `ConnectionSet = {links, devices, tags, signals}`。
  - 每個專案目前限 1 個裝置。
  - Tag 是 Modbus 暫存器區或 `internal` 面板記憶體。
- **Logic 與協定解耦**：Logic 只透過 Read Tag 與 Write Tag 節點碰觸線路（`docs/logic-node-taxonomy.md` Decision 1），**這是 IoT 化最有價值的既有設計**。新增 MQTT 時，只要把 Tag 對應到 topic 與 JSON path，畫面與邏輯都不用改。
- **PC 端**：
  - Web Serial 的 Modbus RTU 框架：`src/serial/`。
  - 模擬裝置與主機：`src/emulator/`。

## 邏輯與預覽

- **Logic 節點**：Trigger（Timer、Event、Tag）、Flow、Screen、Data、Device、Custom（Call Function、C 區塊）。
- **編譯**：Logic 編譯成 `ui_logic.c`。`src/codegen` 約 13 萬行，含測試。裝置端沒有 VM。
- **模擬器**：LVGL 與產生的 C 一起以 emcc 編成 WASM（`wasm/`、`server/emulator/`）。
- **Demo**：離線單一 HTML，含模擬裝置（`src/standalone/`）。

## 雲端、商業與殼

- **已實作**：
  - AI 助理，4 種提供者：OpenRouter、Ollama、NCHC、Claude（`src/assistant/` 約 2.6 萬行）。
  - 授權同意閘門（`src/legal/`）。
  - .NET 10 加 NativeWebHost 桌面殼，可產出 Windows、Linux、macOS 安裝檔。
- **只有研究文件**：雲端圖庫、桌面自動更新、儲值後台。**沒有**帳號登入，**沒有** OTA。

## 授權與品牌

- **授權方**：`src/legal/legalText.ts` 寫「Emerging Display Technologies Corp.」。條文限定軟體只能為 EDT 供應的顯示模組製作內容、runtime 只能用在 EDT 硬體。`installer/licence/EULA.txt` §1(a) 相同，`installer/licence/README.md` 註明是**未經律師審閱的草稿**。
- **授權矛盾**：repo 根目錄**沒有 LICENSE 檔**，README 卻顯示 MIT 徽章。
- **品牌出現處**：
  - 產品名：`desktop/Program.cs`、`ProductLog.cs`
  - `EDT_HMI_*` 環境變數
  - `EDT_CONTACT_EMAIL`
  - 格式 ID `com.edt.hmistudio.project`
  - MIME 類型 `application/vnd.edt.hmistudio.project`
  - logo 設計理念（`docs/logo-design.md`）
- **Factory Mode**：以寫在前端 bundle 的通關密語解鎖（`docs/factory-dev-mode.md`）。分拆後應改為正式的授權層級。

## 專案檔（.ehsp）

- **結構**：4 KB 標頭、卡片、chunk、媒體。
- **對目標的描述**：`target {board, title, virtual}` 加上 `platform: null`，大致與板子無關。
- **綁定點**：
  - 已知板子 ID，以及 LVGL 與 C 語意（色彩格式、`lv_conf`）。
  - EDT 只出現在格式 ID 與 MIME 類型。
- **遷移**：`src/project/format/` 已有版本遷移機制，改名時可沿用。

## 規模

| 區域 | 行數 | 備註 |
| --- | --- | --- |
| `src/` | 約 59.4 萬（1,970 檔） | `src` 加 `server` 約有 32.9 萬行是測試 |
| `server/` | 約 2.1 萬 | — |
| `firmware/` | 約 2.85 萬自有程式，另有 0.46 萬 vendor | — |
| `desktop/`（C#） | 約 0.46 萬 | — |
| `wasm/` | 約 0.7 萬 | — |
| `tools/` | 約 1.57 萬 | — |
| `docs/` | 約 28.3 萬行 Markdown（404 檔） | — |

自 2026-02-07 起有 2,268 個 commit，全部出自同一位作者。

## 給 IoT IDE 的可沿用資產與缺口

| 可直接沿用 | 需要改造 | 需要新建 |
| --- | --- | --- |
| Link → Device → Tag 模型；Logic 與協定解耦；codegen；WASM 模擬器；單檔 Demo；AI 助理；`.ehsp` 容器與遷移；跨平台桌面殼；薄移植層 | 板子知識收斂；建置與燒錄抽象化、跨平台；1 裝置上限改為多裝置；CAN 完成；品牌與授權 | 網路堆疊與 MQTT、HTTP；Task 與並行模型；Zephyr 與 FreeRTOS 目標；OTA；帳號與授權伺服器；VS Code 擴充殼；SBOM 輸出 |
