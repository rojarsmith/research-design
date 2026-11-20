[English](README.md) | **繁體中文**

# AI 研究與設計

整理軟體平台的工程研究、可行性分析、架構提案與跨境經營研究。每個主題各有獨立目錄、主報告、延伸研究與參考資料；可先從想了解的主題主報告開始閱讀。

## 研究主題

| 主題 | 範圍 | 英文 | 繁體中文 |
| --- | --- | --- | --- |
| HMI Studio 分拆與 IoT 開發環境 | EDT HMI Studio 能否脫離硬體公司、轉型為 IoT 開發環境：程式碼中的 EDT 綁定、缺口（連網、OTA、多執行緒、多晶片）、「有螢幕的連網裝置」Low Code 定位、多執行緒與 IDE 能力設計、獨立 IDE 對 VS Code 付費擴充對混合方案、LVGL Pro 與 MicroEJ 專文、約二十項全球競品、定價、分拆前提與分階段路線；另含動態語言 MCU 付費產品、C/C++ 新式 RAD 工具、MCU 研發困擾排行、MCU 單元匯流排架構提案、EHS 收費化路線圖，MCU 熱重載與 PC 端整機模擬調查，硬體與應用分離和上層語言研究，MCU 軟韌體工具的收費重點，以及 MCU 領域的新軟體機會，和以全球證據檢驗的市場規模、出場先例、IoT 平台退場、AI 衝擊、區域市場與台灣分拆實務，以及 NOVAKON「軟體免費、靠硬體賺錢」模式與 HMI 價格、毛利率推估，和把執行環境搬到 Cortex-A 與 x86，以及 Cortex-A 與高階 MCU 上以 RTOS 取代 Linux 的比較 | [English overview](hmi-studio-iot-ide-spinoff-research/README.md) | [完整研究](hmi-studio-iot-ide-spinoff-research/README.zh-TW.md) |
| 高級工程師面試準備 | 一個總覽加兩份報告，不背答案。刷題：學習科學依據、每題固定流程、觸發條件與模式對照、題單與題量、HackerRank 線上測驗格式、兩個解答 repo 評估。演算法以外的回合：系統設計框架、容量估算、需求觸發與設計模式對照、14 題選題；對應 Meta 與 Amazon 評估訊號的 6 到 8 個 STAR 故事；LLD/OOD；Meta、Canva、Google 與 HackerRank Chakra 的 AI 輔助回合。總覽集中 2025–2026 面試現況、時間分配、所有回合共用的 12 週計畫與準備好了的標準。四個已驗證的 VS Code 練習包：74 題 C++ 練習（含計時模擬面試與準備度報告）、三階段 AI 輔助回合、會量出每次需求變更逼你改了多少程式的 LLD/OOD，以及容量估算 | [English overview](senior-interview-research/README.md) | [完整研究](senior-interview-research/README.zh-TW.md) |
| 限價訂單簿 DEX 與 ICO 發幣平台 | 每條 EVM 鏈各自部署的全鏈上訂單簿、合約設計、MetaMask 多鏈、2026 技術選型、收費方式、無審查的極限、ICO rug pull 與法律風險、開發路線 | [English overview](orderbook-dex-launchpad-research/README.md) | [完整研究](orderbook-dex-launchpad-research/README.zh-TW.md) |
| Hyperliquid 與去中心化衍生品交易所 | 從 EtherDelta 式 L1 合約到專用應用鏈、免 gas 簽名下單、永續合約機制、去中心化取捨，以及狀態與歷史成長的處理方式 | [English overview](hyperliquid-dex-research/README.md) | [完整研究](hyperliquid-dex-research/README.zh-TW.md) |
| 適合收費的軟體類型 | 判斷條件、適合與不適合收費的類別、Bitdove OTA＋CRA 優先方向、任職公司權利歸屬與下一步 | [English overview](paid-software-opportunity-research/README.md) | [完整研究](paid-software-opportunity-research/README.zh-TW.md) |
| 付費 Markdown 編輯器市場 | 免費與付費對手、定價與營收量級、仍有空間的利基、著陸頁預購驗證與定價建議 | [English overview](markdown-editor-market-research/README.md) | [完整研究](markdown-editor-market-research/README.zh-TW.md) |
| 桌面軟體收費、授權與 IAM | 序號與訂閱模式、MoR 與 Stripe（台灣主體限制）、Keygen／Cryptlex／LicenseSpring、桌面 OAuth 登入、IdP 選型與驗收情境 | [English overview](desktop-licensing-research/README.md) | [完整研究](desktop-licensing-research/README.zh-TW.md) |
| GitHub 求職 AI 應用 | 近三個月星數前三名（career-ops、ai-job-search、get_jobs）優劣比較、為何都不支援免安裝 Playwright 瀏覽器保留登入與 2FA、以及已實測的 session broker 原型 | [English overview](job-search-ai-research/README.md) | [完整研究](job-search-ai-research/README.zh-TW.md) |
| Codex 桌面版與 Claude API key | 直接登入限制、模型閘道、Responses 相容性與 Windows 驗證方向 | [English overview](codex-claude-api-research/README.md) | [完整研究](codex-claude-api-research/README.zh-TW.md) |
| Computex AI 產品影片 | Claude 經 MCP 指揮雲端影音製作、英文與繁體中文字幕、50 吋 4K／1080p、分鏡、預算及展場驗收 | [English overview](computex-ai-product-video-research/README.md) | [完整規劃](computex-ai-product-video-research/README.zh-TW.md) |
| WebAssembly 研究 | 執行模型、Emscripten 移植、Claire Chang 文章查核、JavaScript 整合、效能、部署、安全與 WASI 元件 | [English overview](webassembly-research/README.md) | [完整研究](webassembly-research/README.zh-TW.md) |
| RAD 快速應用開發工具研發 | 十類領域、原型至成熟平台的工程難度、人力、工期、預算、維護與驗收門檻 | [English overview](rad-tools-research/README.md) | [完整研究](rad-tools-research/README.zh-TW.md) |
| MSIX 檔案與 Windows 封裝 | 套件身分、工具、簽章、部署更新、相容性與指定教學校正 | [English overview](msix-research/README.md) | [完整研究](msix-research/README.zh-TW.md) |
| 動態語言從 0 到 1 | Python、Node.js 與 Lua 的誕生順序、移植性與執行環境六層評估、自製動態語言九階段路線，以及可執行的樹走訪直譯器、其位元組碼 VM（含相對樹走訪的效能量測）、執行同一份位元組碼且含標記清除 GC 的 C 語言 VM、該 VM 的 C 嵌入 API，與三個引擎全數通過一致性測試的語言規格 | [English overview](dynamic-language-design-research/README.md) | [完整研究](dynamic-language-design-research/README.zh-TW.md) |
| AI 與 Git 團隊協作研發 | 跨部門交付、分支保留與刪除、AI 分工、PR 審查、CI 及發布流程 | [English overview](ai-team-development-research/README.md) | [完整研究](ai-team-development-research/README.zh-TW.md) |
| 存讀檔與版本相容性 | 大型軟體與遊戲的存檔架構、資料與內容遷移、安全提交及驗證 | [English overview](save-load-compatibility-research/README.md) | [完整研究](save-load-compatibility-research/README.zh-TW.md) |
| MCU 嵌入式 HMI 生態 | 八項 UI 工具、圖形框架、控制晶片、MCU 與智慧顯示方案；授權、架構與評測方法；另含華凌智慧顯示器完整產品組合：CANopen、自訂 CAN、RS485／Modbus、UART、GUI Builder 版本、容量與整合限制 | [English overview](mcu-embedded-hmi-research/README.md) | [完整研究](mcu-embedded-hmi-research/README.zh-TW.md) |
| IAM 比較與 Keycloak 營運 | 11 組 IAM 方案、應用整合、指定 iThome 系列、部署盤點、備份還原與營運 | [English overview](iam-research/README.md) | [完整研究](iam-research/README.zh-TW.md) |
| 軟體平台支付整合 | 平台串接方向、支付服務比較框架，以及官方核對的 Stripe 測試卡與驗收情境 | [English overview](payment-integration-research/README.md) | [完整研究](payment-integration-research/README.zh-TW.md) |
| 網站維護與 VPS 故障切換 | 維護訊息、nginx 故障邊界、A 主機與 B 備援、自動切回、成本及驗收 | [English overview](website-availability-research/README.md) | [完整研究](website-availability-research/README.zh-TW.md) |
| 多資產交易平台 | 股票、加密貨幣、債券與選擇權的市場資料、分析、回測、券商連線、風控及部署，涵蓋台灣市場 | [Main report](multi-asset-trading-platform/README.md) | [主報告](multi-asset-trading-platform/README.zh-TW.md) |
| VPS 上的 OpenRouter 用量平台 | 快速部署方案、上游授權、API Key、預付額度、帳務、成本試算與驗收 | [Main report](openrouter-vps-token-platform/README.md) | [主報告](openrouter-vps-token-platform/README.zh-TW.md) |
| SSL.com OV Code Signing 驗證 | 台灣公司登記、第三方地址及電話資料、上傳 PDF 與回撥準備 | [Main report](code-signing-research/README.md) | [主報告](code-signing-research/README.zh-TW.md) |
| 台灣居民經營美國公司 | 組織與州別方案、準備、台美稅務、初期及年度成本、維護行事曆與老闆手冊；僅公開研究 | [English overview](us-company-from-taiwan/README.md) | [完整攻略](us-company-from-taiwan/README.zh-TW.md) |
| Blender 研發歷程 | 起步、開源轉型、架構、製作回饋、經費與商業成熟度 | [English overview](blender-development-research/README.md) | [完整研究](blender-development-research/README.zh-TW.md) |
| PC 故障分析與解法 | Windows 事件判讀、依症狀排查、證據蒐集及離線雙語查閱頁；個案包括裝置管理員與 USB Hub 重設、Kingston NVMe 消失、RTX 5070 Ti 桌機雙螢幕喚醒，以及附唯讀採集工具的 LOXJIE A40 USB 爆音與斷聲 | [Main report](pc-troubleshooting/README.md) | [完整攻略](pc-troubleshooting/README.zh-TW.md) |
| 電腦圖形學實作課程 | 從向量與軟體光柵化開始，逐步打造小型即時渲染器：八個瀏覽器實驗，涵蓋陰影貼圖、反鋸齒與小型渲染器期末專題；提供可離線使用的雙語互動課程，以及原生 GPU 渲染的 Part II 草稿 | [English course](computer-graphics-practice/README.md) | [繁體中文課程](computer-graphics-practice/README.zh-TW.md) |
| Claude Startups 申請 | 最新資格與福利、HMI Studio 申請主體評估、Max API 額度與英文詢問草稿 | [English overview](claude-startups-research/README.md) | [完整研究](claude-startups-research/README.zh-TW.md) |
| 網路交友與婚育規劃 | 高雄與跨國認識管道、平台限制、自介與30天行動、來源與研究紀錄 | [English overview](marriage-and-dating/README.md) | [完整研究](marriage-and-dating/README.zh-TW.md) |
| 機車事故證據與求償 | 對向機車左轉、閃避摔車個案的證據保全、警方資料、醫療、機車修復與求償程序 | [研究目錄](motorcycle-accident-research/README.zh-TW.md) | [個案行動指南](motorcycle-accident-research/README.zh-TW.md) |
| C 語言、編譯器與處理器 | C 的起源、編譯器的建構與自舉、一天內的處理器 C 支援評估流程、契約堆疊學習路線、具 RV32 與 x86-64 後端的可執行 C 子集編譯器，以及 19 種 clang 目標的實測比較 | [English overview](c-compiler-foundations/README.md) | [完整研究](c-compiler-foundations/README.zh-TW.md) |
| 作業系統：從核心到合成器（草稿） | 待審閱的課綱草稿，附階段 00–10 的實驗草稿，含第一部分的無螢幕顯示伺服器專題與第二部分第一個 Windows 合成實驗（Windows 部分已執行，xv6 與 Linux 部分已在 Ubuntu 容器中執行，階段 04 的核心模組除外）：以 xv6-riscv 動手學作業系統基礎並對照 Linux 與 Windows NT，再進入顯示與視窗系統（DRM/KMS、平面與 vblank、dma-buf 與顯式同步、輸入、座位、隔離），最後完成最小 Wayland 合成器專題 | [English course](os-foundations/README.md) | [繁體中文課程](os-foundations/README.zh-TW.md) |
| MCU 跨平台學習課程 | 給有軟體開發經驗者從零學微控制器：共通架構與軟體堆疊、以同一個資料採集器逐關推進並移植到第二平台的實作路線、選型與文件閱讀，以及學習紀錄。課程涵蓋從 reset 到 main、GPIO 與時脈、timer 與中斷、UART 環形緩衝區與封包、ADC 與 DMA 的 buffer 所有權、RTOS 的 task 與 queue、watchdog 恢復與設定保存，以及換平台移植，每課附桌機可執行的 C 實驗與上板驗收條件 | [僅繁體中文](mcu-foundations/README.md) | [繁體中文課程](mcu-foundations/README.md) |
| 歐洲 OTA Server 銷售與 CRA | OTA 服務市場進入、CRA 產品邊界與自我評估、ETSI EN 303 645 測試層級、成本模型及客戶信任證據 | [English overview](eu-ota-cra-sales-research/README.md) | [完整研究](eu-ota-cra-sales-research/README.zh-TW.md) |

## 主要文件

| 文件 | 用途 |
| --- | --- |
| [交易平台建議架構](multi-asset-trading-platform/architecture/recommended-architecture.zh-TW.md) | 建議架構與實作方向 |
| [交易平台架構評估](multi-asset-trading-platform/architecture/evaluation.zh-TW.md) | 各架構提案的比較 |
| [交易平台決策紀錄](multi-asset-trading-platform/docs/adr/README.zh-TW.md) | 設計決策的理由與取捨 |
| [OpenRouter 可行性與方案](openrouter-vps-token-platform/research/01-feasibility-and-options.zh-TW.md) | 商業模式、工具比較、授權與成本試算 |
| [OpenRouter VPS 建置路線](openrouter-vps-token-platform/research/02-vps-mvp-roadmap.zh-TW.md) | 最小產品範圍、部署順序與驗收案例 |
| [OpenRouter 來源與待確認事項](openrouter-vps-token-platform/research/03-sources-and-open-questions.zh-TW.md) | 官方資料與上線前須確認的問題 |

## 目錄導覽

| 目錄 | 內容 |
| --- | --- |
| `hmi-studio-iot-ide-spinoff-research/` | 主報告、程式碼耦合盤點、全球競品比較、LVGL Pro 與 MicroEJ 分析、動態語言與 C/C++ RAD 工具調查、MCU 研發困擾排行、單元匯流排架構提案、收費化路線圖、MCU 熱重載與整機模擬調查、硬體與應用分離及上層語言、MCU 工具收費重點、MCU 軟體新機會、全球證據（市場、出場、平台退場、AI、區域、台灣分拆）、NOVAKON 模式與 HMI 毛利、MPU 執行環境、RTOS 與 Linux 比較、來源與未解問題 |
| `senior-interview-research/` | 總覽（共用的面試現況、時間分配與 12 週計畫）、`dsa-practice` 刷題報告與 `beyond-dsa` 回合報告；`cpp-practice/` VS Code C++ 練習專案：題目、範本、測試框架、練習流程命令列工具與說明文件；`ai-round-practice/` VS Code 練習包：三階段 AI 輔助回合情境、共用測試框架與練習命令列工具；`lld-practice/`：4 題 LLD/OOD 題目、一次揭曉一個的「如果……」變體，以及計算每個變體改了多少類別、函式和程式碼行的命令列工具；`estimation-practice/`：容量估算練習，檢查單位、依數量級計分並統計偏差 |
| `orderbook-dex-launchpad-research/` | 僅主報告，無延伸文件 |
| `hyperliquid-dex-research/` | 僅主報告，無延伸文件 |
| `paid-software-opportunity-research/` | 僅主報告，無延伸文件 |
| `markdown-editor-market-research/` | 僅主報告，無延伸文件 |
| `desktop-licensing-research/` | 僅主報告，無延伸文件 |
| `job-search-ai-research/` | 工具比較報告、求職流程設計，以及 `prototype/` Node.js session broker 與檢查腳本 |
| `codex-claude-api-research/` | Codex 與 Claude API key 報告，以及運用既有 API 額度的替代工具 |
| `computex-ai-product-video-research/` | 工具、Claude MCP、來源與成本研究；分鏡、素材與宣稱、交付及專案計畫；製作提示、清單範例與發布檢查表 |
| `msix-research/` | MSIX 封裝、簽章、部署、更新與教學校正 |
| `webassembly-research/` | Wasm 原理、C/C++ 建置指南、部署與採用評估、文章導讀與查證限制 |
| `rad-tools-research/` | RAD 工具領域與產品規模比較、研發資源與預算模型、架構、驗收路線及官方來源 |
| `dynamic-language-design-research/` | 動態語言誕生史、移植性評估、自製語言路線圖、來源、minilang 單檔直譯器範例、minilang-vm 位元組碼編譯器與 VM 範例、minilang-c 的 C 語言 VM 與垃圾回收器範例、minilang-embed 的嵌入 API 範例，以及 minilang-spec 的語言規格與一致性測試 |
| `ai-team-development-research/` | 團隊交付主報告、分支生命週期、AI 治理與平台、來源及工作流程範本 |
| `save-load-compatibility-research/` | 存讀檔架構、公開實作、遷移範例、相容性情境與官方來源 |
| `iam-research/` | IAM 比較、Keycloak 整合與營運、來源索引、現況盤點及獨立本地實驗環境 |
| `mcu-embedded-hmi-research/` | MCU HMI 生態、產品能力、系統架構、評測方法與官方來源；第 04 至 06 篇為華凌智慧顯示器系列、代表型號、GUI Builder 限制、整合與來源差異 |
| `payment-integration-research/research/` | Stripe 測試卡、來源修正與支付整合驗收情境 |
| `website-availability-research/` | 網站維護與 A/B 故障切換、成本模型、實作指南、官方來源及 nginx 模板 |
| `multi-asset-trading-platform/research/` | 各子系統與市場需求的研究 |
| `multi-asset-trading-platform/architecture/` | 架構提案、評估與建議設計 |
| `multi-asset-trading-platform/docs/adr/` | 架構決策紀錄 |
| `openrouter-vps-token-platform/research/` | 可行性、部署、帳務與來源資料 |
| `code-signing-research/` | OV 驗證指南、價格與供應商比較、附來源清單的公司登記與上市資料，以及產生上傳 PDF 的腳本與驗證輸出 |
| `us-company-from-taiwan/research/` | 台灣居民經營美國公司的通用研究；私人個案資料保存在 Git 之外 |
| `blender-development-research/research/` | Blender 工程、製作流程、參考來源與證據範圍 |
| `pc-troubleshooting/` | PC 故障研究、截圖事件表、排查流程、個案範本與本機工具；`cases/` 存放各個案，包括雙螢幕喚醒研究，以及附唯讀 PowerShell 採集工具的 LOXJIE A40 USB 音訊研究（原始證據不納入 Git） |
| `computer-graphics-practice/` | 雙語電腦圖形學實作課程、可執行的瀏覽器實驗 01–08 與 Part II 課綱草稿 |
| `claude-startups-research/` | 計畫研究、申請資料包、申請主體專案評估，以及來源與待確認事項 |
| `marriage-and-dating/` | 平台研究、自介與行動計畫、跨國交友與安全、來源、研究紀錄、付費方案評估及每週檢討範本 |
| `motorcycle-accident-research/` | 機車事故個案行動指南、六篇參考文章摘要與查核、官方程序與證據清單 |
| `c-compiler-foundations/` | C 語言與編譯器起源、處理器 C 支援評估、tinyc 編譯器、RV32IM 模擬器與探針實作 |
| `os-foundations/` | 作業系統課程草稿：階段 00–09 的 xv6-riscv 實驗草稿、階段 10 的 Windows 合成實驗、無螢幕顯示伺服器專題與測試框架、Linux 與 NT 對照、DRM/KMS 與 Wayland 合成器課綱 |
| `mcu-foundations/` | 繁體中文 MCU 課程：共通架構、實作路線、選型與文件閱讀、第一課至第九課與桌機可執行的 C 實驗、學習紀錄及官方參考資料 |
| `eu-ota-cra-sales-research/` | 歐洲 OTA 服務銷售、CRA 範圍與評估路徑、ETSI 基線和測試成本、供應商信任方案 |
| `national-industrial-innovation-award-10/` | 僅限本機的申請工作區；Git 只納入其繁體中文說明、作業規則與 `.gitignore`，所有輸入資料與草稿均不納入 Git |

## 閱讀與語言慣例

1. 先閱讀主題主報告，了解範圍、假設、結論與文件入口。
2. 再依延伸研究與決策紀錄查看證據及實作細節。
3. 解讀版本、價格與條款時，以報告標示的研究日期為準；實作前重新查核可能變動的資訊。

文件通常提供英文與對應的 `*.zh-TW.md` 繁體中文版，各語言導覽連至同語言文件。美國公司經營攻略與 Blender 研究以繁體中文為完整版本，英文入口明示為概要。機車事故個案行動指南與 MCU 跨平台學習課程僅提供繁體中文版。

研究以證據與使用者需求為中心，對廠商採用相同查證標準，並區分已記載的功能、估算與建議設計。閱讀建議、工期及成本估算時，應一併考量其假設與限制。
