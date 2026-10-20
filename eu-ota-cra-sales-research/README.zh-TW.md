# 歐洲 OTA Server 銷售、客戶信任與 CRA 研究

研究基準日：2026-10-20。適用產品背景：以 Eclipse hawkBit 為基礎提供的 Bitdove OTA 服務；目前描述為英國倫敦 Linode/Akamai 主機、免費 HTTPS、Gandi 網域，並提供免費入門數位簽章。

本研究把產品法規責任、第三方測試／認證、服務營運保障及銷售證據分開。它是產品與商務決策材料，不代替歐盟產品法規律師或合格評估機構對特定產品的正式判定。

## 結論摘要

1. **不要對外宣稱「ETSI EN 303 645 CRA 認證」。** EN 303 645 是面向消費型 IoT 的安全基線；TS 103 701 是對照該基線的評估方法。兩者都不是一張 ETSI 發給 OTA SaaS 的 CRA 證書。若外部實驗室完成測試，應精確稱為「由某實驗室依 ETSI EN 303 645 / TS 103 701 評估」，並界定受測版本、服務、範圍、日期、例外及測試機構資格。
2. **CRA 主要是產品製造商的市場准入責任，不是買一張泛用證書即可解決。** 預設產品可採內部生產控制自我評估。重要產品 Class I 有條件採自我評估；Class II 與關鍵產品通常需第三方評估或適用的歐洲網路安全認證方案。產品分類看核心功能，而非供應商自選的行銷標籤。
3. **Bitdove OTA 的 CRA 身分是本研究最大待確認點。** 若自架軟體以 Bitdove 品牌供應，或 OTA 服務是某個數位產品不可或缺、由該產品製造商設計或負責設計的遠端資料處理方案，可能進入 CRA 產品邊界。另一方面，若 Bitdove 是 OEM 的外部託管服務供應商，OEM 仍可能是整合產品的製造商；Bitdove 的安全與供應鏈證據仍會是客戶合規檔案的重要部分。需按合約、品牌、設計責任與產品失去 OTA 後的功能逐案判斷。
4. **OTA Server 可能與 CRA Class I「network management systems」相近。** 法規技術說明涵蓋監控及控制連網元素的網路操作和設定，也舉出端到端管理及組態管理系統。若產品的核心功能符合該定義，不能逕自套用一般軟體的自我評估結論。OTA 更新管理設備軟體是否達到這個定義，須按實際功能、宣稱、客群與產品邊界取得正式分類意見。
5. **目前沒有可引用的公開固定「CRA 認證價」。** CRA 第三方費用由產品複雜度、產品類別、技術檔案品質、差距、測試範圍和評估機構報價決定。自己做 CRA 符合性評估不收 ETSI 或 CRA 機關認證費，但內部工程／法規人力不是零成本。先買範圍明確的差距評估和滲透測試，再決定是否需要正式第三方 CRA 評估。
6. **對客戶的可信度不該只靠免費 HTTPS 或「免費簽章」。** 優先公開安全白皮書、架構與資料流、威脅模型、SBOM／弱點處理、簽章金鑰管理、更新回滾與分批發布控制、SLA／備份演練、DPA、子處理者清單及獨立測試摘要。讓客戶看到服務如何降低 OTA 的車隊級風險。

## 文件導覽

- [01 歐洲市場進入與銷售策略](research/01-market-entry.zh-TW.md)
- [02 CRA 範圍、分類與自我評估](research/02-cra-scope-and-conformity.zh-TW.md)
- [03 ETSI EN 303 645、測試與付費認證層級](research/03-etsi-testing-and-assurance.zh-TW.md)
- [04 成本模型、客戶信任證據與 90 天執行計畫](research/04-costs-and-trust-plan.zh-TW.md)
- [05 來源、證據限制與待確認事項](research/05-sources.zh-TW.md)

## 目前能安全對外使用的說法

> Bitdove OTA is built on Eclipse hawkBit and provides a managed OTA service hosted in London, UK. We provide HTTPS transport and an introductory digital-signing option. We are preparing a product-scoped security and CRA evidence pack covering the service boundary, update-signing controls, vulnerability handling, hosting and customer responsibilities. ETSI EN 303 645 is used as a security reference where applicable; it is not represented as a CRA certificate.

以上英文文字仍應在實際驗證主機供應商、簽章金鑰架構、服務控制、弱點處理流程後才發布。不要使用「CRA certified」「ETSI certified」或沒有獨立查核佐證的「CRA compliant」。

## 研究界線

本研究沒有登入 Bitdove 管理介面、主機或客戶環境；提供的網站頁面在研究瀏覽器無法讀取，因此現況描述僅採用使用者陳述與可搜尋到的公開資料。未取得實驗室報價，所有預算級距均明確標為規劃估算。CRA 法規、歐盟執委會 2026 FAQ 和標準化進度可能變動，簽約或宣稱前應再查官方頁面及 OJEU。
