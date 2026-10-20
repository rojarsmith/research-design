# 02｜CRA 範圍、分類與自我評估

研究日期：2026-10-20。法規主文為 [Regulation (EU) 2024/2847](https://eur-lex.europa.eu/eli/reg/2024/2847/oj/eng)；歐盟執委會 [CRA FAQ](https://digital-strategy.ec.europa.eu/en/library/cyber-resilience-act-implementation-frequently-asked-questions)（頁面於 2026-09-04 更新）提供非拘束性說明。正式判定以法規、適用的委任／執行法規、官方公報及具體事實為準。

## 時程

- CRA 已於 2024-12-10 生效。
- 通報機構制度相關章節自 2026-06-11 起適用。
- Article 14 的重大主動遭利用弱點與嚴重事件通報自 2026-09-11 起適用；不能等到 2027 才準備事故流程。
- 大部分產品義務自 2027-12-11 起適用。該日前已上市產品若其後有重大修改，可能觸發 CRA；Article 14 通報範圍則涵蓋既有產品。

## CRA 產品邊界：需分清三種角色

CRA 的產品含有數位元素（PDE）可包括軟體／硬體產品和遠端資料處理解決方案。遠端處理的關鍵法定測試包括：

1. 是在遠端進行的資料處理；
2. 軟體由 PDE 製造商設計、開發，或在其責任下設計／開發；以及
3. 沒有該遠端處理，PDE 就無法執行其某一功能。

因此，單憑「使用 OTA」不代表所有 OTA 雲端供應商都自動成為 OEM 產品的製造商；也不能因服務稱 SaaS 就假設完全不在 CRA。要檢查裝置的核心／宣稱功能、斷網或停用 OTA 的影響、誰選擇及設計遠端軟體、品牌和合約安排。裝置製造商把外包平台納入產品設計或責任下時，平台可能進入該產品的合規邊界；OEM 仍要管理第三方依賴及證據。

另一路徑是 Bitdove 將自架版／軟體產品以 Bitdove 名稱或商標供應市場。若屬在 EU 市場商業供應的軟體產品，Bitdove 可能就自己的產品承擔製造商責任。此時需對該產品自身作範圍和分類判斷。純粹免費不等於自動豁免，是否屬商業活動也要看實際供應情境。

## OTA Server 可能的 Class I 分類

《[執行規則 (EU) 2025/2392](https://eur-lex.europa.eu/eli/reg_impl/2025/2392/oj/eng)》對 Important Class I「network management systems」的技術說明包括：管理連網元素（伺服器、路由器、交換器、工作站、印表機或行動裝置等），監控並控制其網路操作和設定；例子包括端到端管理系統、專用組態管理系統及 SDN 控制器。

OTA 平台管理連網裝置、軟體版本與更新部署，可能與這一技術描述重疊，但不宜僅憑名稱直接判定。核心功能是否是管理網路元素及其 operations/configuration、是否屬於產品類別和產品宣稱範圍，是法律分類分析的事實問題。供應前準備 1–2 頁的 classification memo，列出功能、非功能、架構、用途、部署模式及依規則逐段比對，請歐盟產品法規專業人士或未來 CRA 通報機構書面覆核。

## 符合性評估路徑（概念摘要）

| CRA 類別 | 通常可採用的路徑 | 對 Bitdove 產品的含意 |
| --- | --- | --- |
| 一般／預設類產品 | Annex VIII Module A 內部生產控制（自我評估），須有完整風險評估、技術文件、符合性證據、EU DoC；不等於免除實質要求 | 若 OTA 服務／自架軟體不落入 Annex III/IV，這是可能路徑；不能只填自我聲明而沒有證據 |
| Important Class I | 充分套用適用且已公布於 OJEU 的 CRA harmonised standards、適用 common specifications，或適用歐洲網路安全認證方案時，可在法定條件下走 Module A；否則需 Module B+C 或 Module H 等第三方路徑 | 若 OTA server 被分類為 Class I network management system，需核對該類別適用標準在銷售時的正式狀態，並確認完整套用範圍 |
| Important Class II | 通常需第三方型式審查加生產符合性（B+C）、品質保證（H），或適用認證方案 | 費用和時程可能比一般內部評估高，先做書面分類 |
| Critical product | 通常需第三方評估，或法規允許且可適用的歐洲網路安全認證方案 | 要看 Annex IV 的實際類別；OTA 平台不因「高風險」自行成為 critical 類 |

自我評估的法律效果是製造商負責證明，不是客戶信任標章。第三方滲透測試、ISO 27001、SOC 2、ETSI 評估也不會單獨替代 CRA 符合性評估、技術檔案、EU Declaration of Conformity 或依法所需的 CE 標示。

## 標準狀態與「CRA 自我主張」

CRA 的 harmonised standard 只有在標準引用依 EU 標準化程序公布於 OJEU 後，才能就其涵蓋範圍產生法律上的 presumption of conformity（見歐盟執委會[標準化說明](https://digital-strategy.ec.europa.eu/en/policies/cra-standardisation)）。ETSI EN 303 645 本身是 consumer IoT baseline，不應把它當作 CRA 下自動的 harmonised standard。ENISA/JRC 的[映射報告](https://www.enisa.europa.eu/sites/default/files/2024-11/Cyber%20Resilience%20Act%20Requirements%20Standards%20Mapping%20-%20final_with_identifiers_0.pdf)可用來找控制缺口，但不是法律上的 presumption of conformity。

截至研究日，歐盟執委會 [CRA 標準化頁面](https://digital-strategy.ec.europa.eu/en/policies/cra-standardisation)表示依 M/606 正在制定 41 項橫向及產品專用標準；官方[2026 ICT 標準化工作規劃](https://interoperable-europe.ec.europa.eu/collection/rolling-plan-ict-standardisation/cybersecurity-network-and-information-security-rp-2026)把 vulnerability handling 和部分水平標準列於 2026 年 8 月、重要／關鍵產品專用標準列於 2026 年 10 月交付目標。ETSI 工作稿或預期交付不是 OJEU 引用。每次簽約前應到 Commission/OJEU 確認發佈狀態與適用產品類別。

若依法可以自我評估，真正可辯護的 CRA 主張應是「我們已針對具體產品版本完成 CRA 符合性評估，並備妥技術文件和 EU Declaration of Conformity」，而不是籠統的網站標語。證據包至少應包括：

- 範圍與角色 memo：產品型號／版本、製造商、授權代表（如有）、進口商／經銷商、部署型態、預期用途、remote data processing 分析、類別判定。
- 產品風險評估：威脅、風險接受／緩解、CRA Annex I 每項要求的適用性、控制、測試及理由。
- 技術檔案與完整供應鏈：架構、設計／建置／測試流程、SBOM、漏洞管理、更新支援期、依賴元件與變更紀錄。
- Annex II 使用說明／安全資訊、支援期限和聯絡點。
- 依適用程序完成 conformity assessment 後的 EU DoC；適用時 CE 標示及對外產品資訊。
- 弱點揭露、事故分類和 Article 14 通報流程；在 CRA 適用的通報制度下按時向指定 CSIRT／ENISA 管道提交並通知受影響使用者。

## Article 14 特別注意

自 2026-09-11 起，製造商需為受 CRA 涵蓋產品的 actively exploited vulnerabilities 和 severe incidents 進行規定通報。重大情形有 24 小時早期警示、72 小時通報、最終報告等期限要求（依事件類型和法規條件適用）；需建立值班聯絡、時區覆蓋、技術確認和客戶通報流程。這是製造商產品責任，若 Bitdove 是服務供應商，合約應訂明發現後的即時轉交通知，讓 OEM 有時間履行法定期限。

## 所需正式判定資訊

1. Bitdove OTA 是只有託管服務，還是也授權／交付可執行軟體？誰以什麼品牌簽約、行銷和交付？
2. 客戶設備在沒有 Bitdove OTA 時還能否執行核心功能？OTA 是否只是維護通道，或它本身就是設備的核心遠端功能？
3. OEM 是否在其責任下設計該遠端解決方案？Bitdove 僅提供通用平台，或替客戶設計／客製並承諾產品功能？
4. 平台對設備做哪些管理：只發布韌體，還是監控／控制網路操作、配置、身份或憑證？
5. 客戶產品本身落在哪個法規類別、用途／環境、例外或其他 EU 法規？

在取得答案前，對外採供應商安全保證和客戶責任分工的語言，不宣稱 Bitdove 已取得 CRA 認證。
