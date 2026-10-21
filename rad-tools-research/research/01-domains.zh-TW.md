# 十個領域的 RAD 工具研發邊界

[回主報告](../README.zh-TW.md) · [資源模型](02-resources.zh-TW.md) · [來源](04-sources.zh-TW.md)

研究日期：2026-10-21。產品文件描述與本研究的產品切分分開呈現；以下 P1／P2 範圍是自研方案假設，**不是代表產品功能清單的完整複製**。此處談的是「做工具」，不是只做一個使用該工具的應用。

## 1. 桌面與原生視覺 IDE

**可查核事實：** Delphi 的設計功能頁列出 VCL、FireMonkey、視覺設計與元件生態。[Delphi Design](https://www.embarcadero.com/products/delphi/features/design)

**P1 切法：** 只支援一種語言、一個桌面 OS 及既有 UI framework；做表單、屬性、事件、建置與錯誤跳轉。使用現有 compiler／LSP／debug adapter，先明確區分自動產生區與手寫區，避免承諾任意程式碼與畫布雙向還原。

**升到 P2 的難點：** 設計期載入第三方元件可能使 IDE 崩潰；元件 metadata、版本、ABI、DPI、IME、無障礙、debugger 與多目標發布形成長期責任。若連語言、編譯器、UI runtime 都自研，已超出窄範圍 IDE 的資源模型。必要能力為編輯器架構、語言工具鏈與原生 UI 整合。

## 2. 網站與內容建構器

**可查核事實：** GrapesJS 支援 project data 的儲存與載入；其文件區分專案資料與 HTML／CSS 產物。[Storage Manager](https://grapesjs.com/docs/modules/Storage.html)

**P1 切法：** 有限版型、區塊與斷點，單人編輯，靜態匯出及版本還原；重用瀏覽器排版與既有編輯器框架。先不做任意 JS、電商交易與多人即時協作。

**P2 難點：** 任意巢狀版面、CSS 相容、SEO、無障礙、CMS schema、多人衝突、資產權限、網域與發布平台。若客戶需求主要是數個版型，範本式工具較可能控制投入；要求任意設計自由度則必須處理更多不可預期組合。必要能力為前端排版、文件模型、UX 與發布整合。

## 3. 內部 CRUD、後台與資料工具

**可查核事實：** Appsmith GAC 文件描述資源、權限、角色及群組，頁面標示 Business；Retool 提供分支、合併與部署的來源控制指南。[Appsmith GAC](https://docs.appsmith.com/advanced-concepts/granular-access-control)、[Retool guides](https://docs.retool.com/source-control/guides)

**P1 切法：** 表格／表單、單一 SQL 資料庫加 REST、既有身份供應商、少量角色與單一部署。密鑰留在伺服器；權限須在 API 與資料存取層驗證，隱藏按鈕不是安全控制。

**P2 難點：** 多租戶、行列權限、查詢資源限制、憑證輪替、環境差異、稽核、客戶端腳本與資料來源外洩。連接器的難點通常包含分頁、限流、型別映射與錯誤語意。需要資料／後端、安全及整合工程，而不只是拖拉元件。

## 4. 行動與跨平台 App builder

**可查核事實：** FlutterFlow 提供程式碼匯出與 Custom Functions、Actions、Widgets 等擴充入口；這不足以推定任意手寫專案可完整匯回視覺模型。[Exporting Projects](https://docs.flutterflow.io/flutterflow-cli/exporting/)、[Custom Code](https://docs.flutterflow.io/concepts/custom-code/)

**P1 切法：** 既有 Flutter runtime、固定導航／狀態模型、REST、少數 native plugin，先選一個主要發布 OS。工具需輸出可在乾淨環境建置的專案，並清楚標示產生區與自訂區。

**P2 難點：** iOS／Android SDK 更新、簽署、商店政策、背景執行、推播、深連結、原生插件、離線同步與多尺寸。各平台建置環境、測試設備與開發者帳號需列入資源；不能把共享 UI 程式碼當成全部整合成本。

## 5. 企業模型驅動平台

**可查核事實：** Mendix Runtime 解譯模型並處理伺服器與客戶端互動；Power Platform ALM 文件包含 solutions、環境及 CI/CD。[Mendix Runtime](https://docs.mendix.com/refguide10/runtime/)、[Power Platform ALM](https://learn.microsoft.com/en-us/power-platform/alm/overview-alm)

**P1 切法：** 聚焦一類流程，例如請購簽核；實體、角色、表單、狀態機與部署均固定在清楚邊界內。重用資料庫、身份與部署系統；不先承諾任意企業流程與所有 ERP 整合。

**P2 難點：** domain model 演化、交易與補償、長流程版本、稽核、環境提升、擴充 SDK、叢集、舊專案相容及專業服務。產品價值與客戶交付服務容易混在一起，需追蹤多少需求靠一般使用者自行完成。需要 runtime／資料架構、企業整合、安全及客戶導入能力。

## 6. 工作流程與 API／IoT 整合

**可查核事實：** Node-RED 的 context 預設保存在記憶體，可配置其他 storage；n8n queue mode 以 main、Redis、worker 與資料庫協作。[Node-RED context](https://nodered.org/docs/user-guide/context)、[n8n queue mode](https://docs.n8n.io/hosting/scaling/queue-mode/)

**P1 切法：** 限定 5–10 類節點、HTTP／排程觸發、持久化任務狀態及可追蹤重試。先不做任意使用者程式碼；對每個具副作用節點宣告逾時、重試與冪等契約。

**P2 難點：** 等待數週的流程、worker 重啟、取消、版本切換、重放、外部 OAuth／API 更新、資料保留與分散追蹤。資料有落盤不代表已執行的副作用可安全重做。需要分散系統、整合維護、執行安全與可觀測性能力。

## 7. 嵌入式與 MCU HMI

**可查核事實：** Qt Design Studio 的 MCU preset 產生 QML／UI QML／CMake 專案，使用受限元件集合。LVGL 9.3 文件說明其一般 API 需避免跨執行緒並行呼叫。[Qt MCU projects](https://doc.qt.io/qtdesignstudio/studio-projects-for-mcus.html)、[LVGL threading](https://lvgl.io/docs/open/9.3/porting/os)

**P1 切法：** 只支援一個圖形 runtime、一組 SDK、一塊正式板，有限 widget／動畫／字型；產生 C／C++ 或既有 runtime 可接受的模型。runtime 是 LVGL 等現有基礎時，工具的工作主要在模型、資源轉換、程式生成及裝置整合。

**P2 難點：** 多板 BSP、GPU／DMA、像素格式、色深、字型 shaping、Flash／RAM、觸控、RTOS 時序、SDK 版本與 simulator 一致性。需要嵌入式／圖形人才與真機測試矩陣；單看 PC preview 無法推定目標板效能。驗收至少包括 Flash／RAM 峰值、frame time 分布及長時間操作。

## 8. 工業 HMI 與 SCADA

**可查核事實：** Ignition 8.3 Gateway 是核心服務，連接資料與 PLC、執行模組、服務客戶端，並提供備份還原與備援設定。[Gateway](https://docs.inductiveautomation.com/docs/8.3/platform/gateway)

**P1 切法：** 一種協定／一個 PLC 家族、單 Gateway、有限 tags、趨勢與基本告警；斷線品質必須明確顯示。先在模擬器與測試設備驗證，不能把一般監控工具假定為安全控制器。

**P2 難點：** 多協定、tag 時間戳／品質、告警確認與歷史、斷線緩衝、主備切換、權限、站點部署及舊設備壽命。工業監控與 MCU 畫面工具的責任不同；前者多了持續連線、事件及服務狀態。需要工業通訊、可靠度工程與現場驗證能力。功能安全或特殊產業認證另列專案，不包含於本文一般商用估算。

## 9. 測試、量測與圖形資料流程

**可查核事實：** NI 的 LabVIEW Core 1 課程涵蓋圖形環境、dataflow、資料擷取、儀器控制、記錄與量測分析。[NI 課程](https://education.ni.com/teach/resources/368/labview-core-1)

**P1 切法：** 一類儀器與固定資料流程，用既有 Python／C++ 數值函式庫及驅動；支援型別檢查、單位、資料記錄及有限除錯。明確界定為非硬即時量測工具。

**P2 難點：** 圖形語言的型別與排程、並行及背壓、串流時間、數值精度、校正追溯、儀器驅動生態；若加入 FPGA／硬即時編譯，須獨立評估。畫出資料流與正確實作資料流語言之間有很大落差。需要量測領域、型別／runtime、效能與驅動人才。

## 10. 遊戲與互動內容編輯器

**可查核事實：** Godot 官方文件提供 editor、import、inspector、3D gizmo 與 visual shader 外掛入口。[Editor plugins](https://docs.godotengine.org/en/stable/tutorials/plugins/editor/index.html)

**P1 切法：** 在現有引擎上做特定類型的互動內容／2D 遊戲編輯器，限制場景與資產類型，只選一個主要匯出目標。把自研部分集中在 domain workflow，而非重造渲染、物理或音訊。

**P2 難點：** 場景與 prefab 引用、資產 import cache、即時編輯、腳本、動畫、物理、效能分析、網路與多平台打包。完整自研遊戲引擎的投入遠超「在引擎上做編輯器」；本研究不把兩者成本混用。需要引擎／圖形、內容管線與工具 UX 人才。

## 跨領域選擇的判準

| 團隊已有的資產 | 可優先驗證的方向 | 必須證明的差異 |
| --- | --- | --- |
| 前端、設計系統與網站交付經驗 | 範本式建站／受限內容工具 | 真實版面需求與發布效率，而非元件數 |
| 資料、後端與企業整合 | CRUD／特定流程平台 | 權限正確、整合穩定、使用者可獨立交付 |
| 嵌入式、BSP 與裝置客戶 | 單 runtime 的 HMI 工具 | 真機資源與交付改善，能維護 SDK 相容 |
| 儀器、工業協定與場域 | 窄量測或監控工具 | 正確性、斷線行為與設備驗證 |
| 語言工具／圖形／引擎 | 特定 IDE／互動內容工具 | 編輯到執行的完整流程及既有生態整合 |

這是能力配對建議，不能代替付費需求驗證。市場競爭、客戶切換成本與導入服務可能讓工程上較容易的工具仍不容易經營。
