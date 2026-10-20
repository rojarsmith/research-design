# 05｜來源、證據限制與待確認事項

查核日期：2026-10-20。優先列 EU 法規、歐盟執委會、ETSI、ENISA 和服務商官方資料。研究中把廠商的價格／銷售主張明確標為廠商資料，不把它們當作 CRA 法律結論。

## CRA 法規與官方說明

1. [Regulation (EU) 2024/2847（Cyber Resilience Act，EUR-Lex）](https://eur-lex.europa.eu/eli/reg/2024/2847/oj/eng) — 法規正文；重點 Article 3 定義、13 製造商義務、14 漏洞／事件通報、27–32 符合性程序、Annex I–VIII。
2. [歐盟執委會 CRA Summary](https://digital-strategy.ec.europa.eu/en/policies/cra-summary) — 時程、遠端資料處理解釋、經濟營運者、符合性和重要／關鍵產品概覽。
3. [歐盟執委會 CRA FAQ](https://digital-strategy.ec.europa.eu/en/library/cyber-resilience-act-implementation-frequently-asked-questions) — 2025-12 發佈；頁面顯示 2026-09-04 更新。官方但屬解釋性 FAQ，不取代法規文本。
4. [歐盟執委會 CRA Conformity Assessment](https://digital-strategy.ec.europa.eu/en/policies/cra-conformity-assessment) — 預設自我評估與重要／關鍵產品第三方評估路徑概述。
5. [Commission Implementing Regulation (EU) 2025/2392](https://eur-lex.europa.eu/eli/reg_impl/2025/2392/oj/eng) — Annex 對 Important Class I/II、Critical 類別做技術描述；包含 network management systems 技術範圍。
6. [歐盟執委會 CRA Standardisation](https://digital-strategy.ec.europa.eu/en/policies/cra-standardisation) — M/606 和 41 項 CRA 標準化工作說明。標準草案不等於正式 OJEU 引用。
7. [歐盟 harmonised standards 說明](https://single-market-economy.ec.europa.eu/single-market/goods/european-standards/harmonised-standards_en) — harmonised standard 引用須於 OJEU 發佈。
8. [ENISA/JRC CRA Requirements Standards Mapping (2024)](https://www.enisa.europa.eu/sites/default/files/2024-11/Cyber%20Resilience%20Act%20Requirements%20Standards%20Mapping%20-%20final_with_identifiers_0.pdf) — 現有標準對 CRA 要求的技術映射，適合做差距分析參考；不是法定符合性證書或 presumption of conformity。

## ETSI 標準

9. [ETSI EN 303 645 V2.1.1 PDF](https://www.etsi.org/deliver/etsi_en/303600_303699/303645/02.01.01_60/en_303645v020101p.pdf) — 消費型 IoT 網路安全基線和範圍。
10. [ETSI TS 103 701 V2.1.1 PDF](https://www.etsi.org/deliver/etsi_ts/103700_103799/103701/02.01.01_60/ts_103701v020101p.pdf) — EN 303 645/TS 103 645 的符合性評估方法；包含 self/second/third party 評估角色、DUT/ICS/IXIT、測試群組及評估程序。
11. [ETSI TS 103 701 Work Item](https://portal.etsi.org/webapp/WorkProgram/Report_WorkItem.asp?WKI_ID=73703) — EN 303 645 V3.1.3 對應更新工作狀態；實際使用須查正式已發布版本。
12. [ETSI hawkBit](https://hawkbit.eclipse.dev/) — Eclipse hawkBit 對 API、安全和 TLS 功能的上游描述。這是上游專案功能說明，不是 Bitdove 部署實際設定的獨立驗證。

## 供應商／成本／資料位置

13. [SafetyBis IoT penetration testing pricing](https://www.safetybis.com/pentest/iot-penetration-testing/) — 單一供應商公開價目樣例：裝置、App＋雲端或進階方案的價格區間。只用作詢價量級參照，不是官方費率、CRA 評估費或 Bitdove 報價。
14. [Linode/Akamai Data Protection Addendum](https://www.linode.com/content/dam/site/en/documents/corporate/akamai-data-protection-addendum.pdf) — 雲端服務商 DPA 文件；簽約時應核實現行版本、實際帳戶條款及所有次處理者。
15. [歐盟執委會 adequacy decisions](https://commission.europa.eu/law/law-topic/data-protection/international-dimension-data-protection/adequacy-decisions_en) — 英國充分性決定狀態；倫敦是 UK 資料位置，不等於 EU/EEA。
16. [Commission Implementing Decision (EU) 2025/2574](https://eur-lex.europa.eu/eli/dec_impl/2025/2574/oj/eng) — 英國 GDPR 充分性決定續期／有效期間法律文件。需按其有效日期與例外條款審查，不以資料中心位置替代 GDPR 角色及 DPA 分析。

## 網站與可得性限制

使用者提供的 [Bitdove OTA 業務頁](https://bitdove.net/bitdove-ota) 及可搜尋到的 [Bitdove 產品簡介 PDF](https://www.bitdove.net/document/bitdove-ota-solution-dm-en-1.5.pdf) 在本研究瀏覽器中未能完整讀取；因此本報告沒有核實網站上的功能宣稱、訂價、法律文件或簽章流程。實際服務細節採用使用者提供的背景描述；部署現況應另以程式碼、設定、合約和控制證據確認。

## 主要待確認事項

- 是否將產品提供給 EU 市場、實際簽約主體及 Bitdove 的品牌／商標位置。
- 託管服務與自架產品是否都在銷售；由誰設計／控制遠端資料處理解決方案。
- 設備即使沒有 OTA 是否仍能執行核心功能；平台的核心功能是否落入 network management systems。
- CRA Class I 標準在銷售日是否已定稿並刊於 OJEU；是否適用 common specification 或 EU cybersecurity certification scheme。
- 免費數位簽章的私鑰持有人、產生方式、儲存保護、客戶隔離、輪替／撤銷、裝置端驗證與防降版。
- Linode/Akamai 實際服務區域、次處理者、備份／日誌位置、支援存取位置與帳戶適用的 DPA 版本。
- 目標客戶是否採購要求 ISO 27001/SOC 2/產品標章，以及各方案對應採購決策者和預算。

## 引用與判讀界線

- 法規和公告按研究日期記錄；OJEU 是查 CRA harmonised standard 法律效果的關鍵核對點。
- 廠商網站只能證明廠商公開提供什麼主張／標價，不能證明測試品質、普遍市場價格或 Bitdove 的安全狀態。
- 方案選擇、成本預算和客戶信任材料是根據已知背景提出的商業建議；正式法律義務需由產品事實和最新法規判定。
