# 來源、證據邊界與查證紀錄

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-21。主要採官方產品文件、官方教學及上游儲存庫。它們適合確認自己的架構與功能，不足以證明跨產品效率、研發成本或可靠度優劣。本研究沒有引用供應商的生產力倍數，也沒有依專案背景替任何公司安排優先地位。

## 1. 已查閱來源與支持範圍

| ID | 官方／上游來源 | 本研究採用的資訊 | 不支持的延伸結論 |
| --- | --- | --- | --- |
| S01 | [Delphi Design](https://www.embarcadero.com/products/delphi/features/design) | VCL／FireMonkey、視覺設計與元件生態 | 所有平台功能相同、重建 IDE 所需人數 |
| S02 | [GrapesJS Storage Manager](https://grapesjs.com/docs/modules/Storage.html) | 專案資料儲存、載入與 HTML／CSS 輸出的區別 | 單靠框架即可取得完整商用建站服務 |
| S03 | [Appsmith Granular Access Control](https://docs.appsmith.com/advanced-concepts/granular-access-control) | 資源、角色、權限及群組；頁面標示 Business | 所有版本都含完整治理，或僅前端控制就安全 |
| S04 | [Retool Source Control guides](https://docs.retool.com/source-control/guides) | 分支、合併、部署及保護相關指南 | 所有方案皆可使用、其版本管理品質優於他者 |
| S05 | [Retool self-hosted](https://docs.retool.com/self-hosted) | 自管與代管部署、責任分工 | 本機安裝就沒有維運成本；未用於價目估算 |
| S06 | [FlutterFlow Exporting Projects](https://docs.flutterflow.io/flutterflow-cli/exporting/) | 匯出專案與指定分支；忽略檔案機制 | 任意手寫專案可完整匯回模型或所有版本均可匯出 |
| S07 | [FlutterFlow Custom Code](https://docs.flutterflow.io/concepts/custom-code/) | Functions／Actions／Widgets 等擴充點 | 無限制往返、所有原生能力自動可用 |
| S08 | [Mendix Runtime，refguide10](https://docs.mendix.com/refguide10/runtime/) | 模型由 runtime 執行、client／server、對應 patch 版本 | 所有 low-code 都採同架構，或本文估算等於 Mendix 投入 |
| S09 | [Microsoft Power Platform ALM](https://learn.microsoft.com/en-us/power-platform/alm/overview-alm) | solutions、環境、來源控制及 CI/CD 的生命週期概念 | 購買方案自動完成治理或所有流程都可無程式化 |
| S10 | [Node-RED context](https://nodered.org/docs/user-guide/context) | 預設記憶體 context 與可配置儲存 | 儲存 context 等於可靠長流程／交易保證 |
| S11 | [n8n queue mode](https://docs.n8n.io/hosting/scaling/queue-mode/) | main、Redis、workers、資料庫與 queue mode | 端到端 exactly-once、自動消除外部 API 副作用 |
| S12 | [Qt Design Studio MCU projects](https://doc.qt.io/qtdesignstudio/studio-projects-for-mcus.html) | MCU preset、有限元件、QML／UI QML／CMake 產物 | 全部 desktop QML 能力都適用 MCU |
| S13 | [LVGL 9.3 Threading Considerations](https://lvgl.io/docs/open/9.3/porting/os) | 一般 LVGL API 的跨執行緒序列化責任 | LVGL 自帶完整 RAD IDE，或無例外地禁止所有跨執行緒 API |
| S14 | [Ignition 8.3 Gateway](https://docs.inductiveautomation.com/docs/8.3/platform/gateway) | Gateway 連接 PLC／資料、執行模組、備份還原與備援設定 | 安全控制認證、任何網路故障都無損切換 |
| S15 | [NI LabVIEW Core 1](https://education.ni.com/teach/resources/368/labview-core-1) | 圖形環境、dataflow、擷取、儀器控制、記錄與分析 | 其編譯器內部設計、本文團隊可複製完整 LabVIEW |
| S16 | [Godot Editor plugins](https://docs.godotengine.org/en/stable/tutorials/plugins/editor/index.html) | editor／import／inspector／gizmo 等擴充入口 | 工具外掛等於完整自研引擎，或 Godot 本身是企業 low-code |
| S17 | [Appsmith 根 LICENSE，release branch](https://github.com/appsmithorg/appsmith/blob/release/LICENSE) | 該根檔案記載 Apache 2.0 | 全部目錄、企業版、元件及服務皆相同條款 |
| S18 | [n8n 官方用途授權說明](https://support.n8n.io/article/can-i-use-your-license-for-my-use-case) | Community 採 SUL；對外配置流程、託管及 white-label 等商用條件說明 | 已替某特定重售、託管或客戶方案取得授權 |

上述連結多為會更新的文件；明確保留已讀的版本範圍（Mendix 10、Ignition 8.3、LVGL 9.3）。Qt 頁面查閱時標示 Design Studio 4.8.2。這些版本用於架構例證，沒有宣稱是研究日全市場最新版。正式選型須固定 release／commit、保存依賴清單並重新核對條款與支援範圍。

## 2. 事實、推論、估算與未知事項

| 類型 | 本研究的處理方式 | 例子 |
| --- | --- | --- |
| 文件事實／廠商自述 | 附官方來源，只限其文件描述 | Mendix 模型與 runtime、Qt MCU 專案產物 |
| 工程推論 | 明示為本研究判斷，不宣稱實測 | 支援矩陣擴大會增加相容測試與維護責任 |
| 資源估算 | 列前提、工作包、公式與敏感度 | 6 FTE × 12 月、18 萬元完全月成本 |
| 未知事項 | 保留待驗證，不以宣傳補缺口 | 各廠實際投入、特定團隊速度、客戶付費需求 |

不同產品文件交叉支持「工具包含編輯以外責任」這一架構觀察；沒有獨立 benchmark 足以判定哪一家一定更快、更安全或成本更低。本文不做這些結論。

## 3. 查證紀錄與讀取限制

1. 搜尋並開啟 S01–S12、S14–S18 的相關官方頁面或上游檔案，按能力與架構建立領域分類；沒有以 GitHub stars、融資或行銷敘述推算工程人月。
2. n8n queue mode 舊網址重新導向[目前 Markdown 文件](https://docs.n8n.io/deploy/host-n8n/configure-n8n/scaling/enable-queue-mode.md)，讀取到 Redis、worker 與資料庫內容。原 `docs.n8n.io/sustainable-use-license/` 回傳不存在，改用 S18 官方 Help Center，未引用第三方鏡像。
3. LVGL 舊 docs 路徑出現錯誤，新路徑直接開啟未抽出正文；S13 的 threading 內容由搜尋服務的官方頁面索引取得。此項不列為已完整逐段讀取；實作時應以固定版本原始文件再次確認 API 與例外。
4. NI LabVIEW tutorial 網頁要求 JavaScript，改用可讀取的 S15 官方課程頁確認 dataflow 與量測用途；未推論其內部 compiler 架構。
5. Microsoft ALM 頁面顯示登入提示，但仍可讀到正文，僅採可讀內容。不推定存取受限的其他資料。
6. 工業備援搜尋曾出現 7.9 舊文件；主報告未把舊版連接埠與 failover 細節搬入 8.3，只引用 S14 的服務與設定範圍。
7. 本次只完成文件研究及靜態文件／算式檢查，沒有安裝、執行、效能測試或審計任何代表產品，也未取得廠商報價或內部人力資料。

## 4. 可提高下一版可信度的資料

| 待確認 | 對估算的影響 | 取得方式 |
| --- | --- | --- |
| 實際要做工具或只做應用 | 直接改變產品責任與投入數量級 | 以一個實際交付場景確認 |
| 團隊已有 framework／runtime／設備能力 | 決定能否使用低投入情境 | 兩至四週 spike 與技能盤點 |
| OS、部署、SDK、硬體及版本線 | 決定測試矩陣與維護編制 | 列出正式支援及不支援組合 |
| 目標資料量、延遲、可靠度與復原 | 決定架構與驗證成本 | 固定測試 workload 和故障情境 |
| 自訂程式碼、多租戶、外掛需求 | 決定安全與擴充契約 | 原型與威脅／權限模型 |
| 付費問題、競品可替代程度 | 決定值得投入的方向 | 訪談、獨立試用、付費 pilot |
| 招募、人事、設備、雲端、授權成本 | 取代預算占位值 | 實際報價與人事規劃 |
| 保固、維護線、服務時間與部署責任 | 決定 recurring 成本 | 先擬支援政策再估 FTE |

在上述輸入明確前，應把區間視為比較工具，而不是商業承諾。
