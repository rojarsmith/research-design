[English overview](README.md) | **繁體中文**

# 付費 Markdown 編輯器的市場空間

研究日期：2026-11-03（Asia/Taipei）。問題：自己寫一個 Markdown 編輯器並收費，還有沒有市場空間？收費與授權的做法見[桌面軟體收費研究](../desktop-licensing-research/README.zh-TW.md)。

## 結論

**再做一個「通用 Markdown 編輯器」的收費空間很小；鎖定一群願意付費、而且免費工具解決不了需求的客群，才有機會。** 建議不要做「另一個 Typora」；要做，先挑一個利基並用預購驗證，過了門檻再開發。

## 為什麼通用型很難

### 1. 免費對手太強

- **Obsidian**：本體免費且不需註冊；商業授權改為自願付費（約 $50/人/年），收入來自 Sync（約 $4–5/月）與 Publish（約 $8–10/月）。商業授權改為自願的日期，來源有 2025-02 與 2026-02 兩種說法。
- **VS Code、Zettlr、MarkText**：免費。
- 許多人直接用 Notion，或在 AI 程式工具裡寫 Markdown。

### 2. 付費對手價格低、品牌已建立

| 產品 | 收費方式（查核時） | 定位 |
| --- | --- | --- |
| Typora | $14.99 買斷（官方商店） | 即時預覽、便宜好用 |
| iA Writer | 買斷，價格較高 | 專注寫作、設計感 |
| Bear | 約 $2.99/月 | Apple 生態筆記 |
| Ulysses | 約 $5.99/月或 $49.99/年 | 長篇寫作、書稿；僅 Apple 平台 |

除 Typora 外，價格來自第三方整理，彼此有出入，採用前以官方頁面為準。

### 3. 新產品持續出現

- 2026 年仍有新的 Markdown 編輯器上架，例如 Markdown Studio（2 月）、OpenMark 2（3 月）、BetterLocale MarkDown（3 月，$3.99 買斷，可自帶 AI 金鑰）。
- 第三方估計多數獨立 Markdown App 月營收低於 $1 萬美元；Bear 估計約 $4.5 萬/月。這些是未經查證的估算，只能看量級。

功能若與 Typora 相近，就要同時對抗「免費」與「$15 買斷」，很難說服使用者換工具。

## 還有空間的方向

付費理由要從「編輯器好不好用」換成「它幫我完成一件免費工具做不到的工作」。

| 方向 | 例子 | 付費理由 |
| --- | --- | --- |
| 團隊與企業授權 | 大量授權、集中管理、離線環境部署 | Typora 官方商店註明不提供大量授權，企業合規採購會卡住 |
| 特定輸出格式 | 台灣公文、論文格式、技術手冊直接輸出合格的 PDF 或 Word | 排版與格式是真正痛點，現有工具要靠外掛或手調 |
| 中文排版 | 中英文間距、直排、標點擠壓、注音 | 國外產品較少處理 |
| AI 工作流 | 管理給 AI 代理的規格、提示詞與知識文件，可接本機模型 | 新需求，對手尚未定型 |
| 文件即程式碼（Docs-as-code） | 與 Git、審查流程、靜態網站整合，讓非工程師寫技術文件 | 公司願意付團隊席位 |

以上方向是工程與市場判斷，尚未以訪談或預購驗證。

## 驗證步驟

1. **選一個利基**：最好是自己或客戶實際碰過的痛點。
2. **先做著陸頁，不先寫程式**：寫清楚幫誰、解決什麼、多少錢，接上 Stripe 預購或候補名單。建議門檻為兩到四週內 30–50 筆預購或付費意向；這是本研究的建議值，不是業界標準。
3. **定價**：個人版 $20–40 買斷或約 $5/月；團隊版按席位收年費。收費與授權採用[桌面軟體收費研究](../desktop-licensing-research/README.zh-TW.md)第八、九節的「美國 LLC＋Stripe Managed Payments＋自建授權」方案。
4. **期待值**：通用型個人產品較實際的目標是副業收入；要做到公司規模，通常要靠團隊或企業授權。

## 來源

第三方整理居多，數字僅供參考：

- [Typora 官方商店](https://store.typora.io)
- [Obsidian 官方價格頁](https://obsidian.md/ro/pricing)
- [Obsidian pricing 2026](https://www.eesel.ai/blog/obsidian-pricing)
- [Obsidian 統計（第三方估算）](https://fueler.io/blog/obsidian-usage-revenue-valuation-growth-statistics)
- [Bear vs iA Writer vs Ulysses](https://unmarkdown.com/blog/bear-vs-ia-writer-vs-ulysses)
- [Best Markdown Editors 2026](https://unmarkdown.com/blog/best-markdown-editors-2026)
- [Typora vs iA Writer](https://toolradar.com/compare/typora-vs-ia-writer)
- [Star History：獨立 Markdown 編輯器 2026 年 5 月](https://newsletter.star-history.com/p/star-history-monthly-standalone-markdown-editor-may-2026)
- [BetterLocale MarkDown](https://marlvel.ai/intel-report/developer-tools/betterlocale-markdown)
- [Bear 營收估算](https://adapty.io/paywall-library/bear-markdown-notes/)
- [Markdown Studio](https://www.producthunt.com/products/markdown-studio/launches)
- [OpenMark 2](https://www.producthunt.com/products/openmark-2/launches/openmark-2)

限制：未取得任何產品的官方營收或使用者數；未做使用者訪談或預購測試。
