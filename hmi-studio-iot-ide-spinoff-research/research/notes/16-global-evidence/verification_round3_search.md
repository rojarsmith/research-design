# 16 Global evidence: verification round 3 (web search)

Date: 2026-11-20. Tools: WebSearch and WebFetch only. Pages that need a browser (IEA, Omdia, Gartner abstract, TouchGFX licence, Danish CVR, Open FAR Cases, Taiwan 青創貸款/TTA) were left to the other agent.
This round avoided the searches that already failed in the round-2 notes (`verification_*.md`) and tried filings, local-language queries, vendor checkout pages and partner directories instead.

## Summary

| # | Item | Verdict | One-line fact |
| --- | --- | --- | --- |
| 1 | Qualcomm–Arduino closing date | STILL UNVERIFIED (exact date); NEW FINDING (filings) | The FY2025 10-K does not mention Arduino. The Q1 FY26 10-Q reports "four other businesses" (unnamed) bought for US$846M in the quarter starting 2025-09-29. The window is still 2025-10-07 to 2025-11-05. |
| 2 | EN 18031 restrictions in 2026 | STILL UNVERIFIED (no change found) | No 2026 decision amends 2025/138 or EN 18031. The latest RED amendment, 2026/2003 (2026-09-04), does not mention EN 18031. |
| 3 | MOF 2007 ruling 09604545320 | CONFIRMED (still applied) + NEW FINDING | Taipei High Administrative Court 113-訴-948 (2024-12-19) applied the ruling, held it consistent with the law, and said it applies to any split mode. The court upheld the tax bureau. |
| 4 | Korea/SEA distributors and prices | NEW FINDING | Embedded Wizard sells through Dioiz (Korea and Japan) and Simplify (Taiwan and China), at €199 per month per seat. SEGGER's distributors are In Semiconductor (Korea), Microtime (Taiwan), and Gopalam and Testech (Singapore, covering SEA). Qt's directory lists OCUBE (Korea) and TECHKINETICS (Singapore) as channel sales. LVGL Pro and SquareLine are sold direct in USD. No local-currency prices were found. |
| 5 | Japan yen prices | NEW FINDING (one yen band) | Osaka NDS lists Storyboard on IPROS at 100万円～500万円. The others are quote-only. CRI's Mobility sales rose from ¥633M to ¥932M (FY9/2026), driven by Glassco in Indian motorcycles. |
| 6 | Embedder | STILL UNVERIFIED (claims) + NEW FINDING (source) | The YC page now shows 6 staff, San Francisco, and no user, customer or funding claims. A job-board mirror (riseme.ai) still shows the YC listing text: 5,000+ engineers, 100+ paying customers, CRV/BoxGroup/Emerson Collective, and "$XM+ raised". No TechCrunch or funding press was found. |
| 7 | Rightware / BIS rule | STILL UNVERIFIED (customer concern) + NEW FINDING (ThunderSoft filing) | No automaker, law-firm or trade-press source names Kanzi or ThunderSoft under the rule. ThunderSoft's 2024 annual report shows Rightware Oy 100% owned, with no impairment on goodwill of RMB 202M. Searches of the report for 美国, 地缘, 出口管制, 实体清单 and 网联 found no US-rule risk statement. |
| 8 | US flip from Taiwan | NEW FINDING (legal route); costs STILL UNVERIFIED | 企併法 §31(3) allows a share exchange (股份轉換) with a foreign company and applies §21 to it. The proposed 2025–26 deferral for share exchanges is limited to holding companies that 國發會 certifies and excludes affiliates. No Taiwan source gives flip costs. |
| 9 | Gartner low-code vs vibe coding wording | STILL UNVERIFIED | No InfoWorld, Computerworld, TechTarget or Register article quotes the report. Jitterbit and Mendix pages return 404, and web.archive.org is blocked for fetch. |
| 10 | Final pass: developer count, VS Code share, MCU-with-display share | Nothing new | Only JetBrains "State of Embedded Development 2025" turned up, and its landing page has no IDE, headcount or display figures. |

---

## 1. Qualcomm–Arduino closing date

**Verdict: STILL UNVERIFIED for the exact date. NEW FINDING from filings.**

- **FY2025 10-K** (fiscal year ended 2025-09-28): I extracted the full text of the PDF. The word "Arduino" does not appear. Note 9 (Acquisitions) lists only the pending Alphawave deal, then "expected to complete during the first quarter of calendar 2026". So Arduino had not closed by 2025-09-28, or was too small to disclose. — https://s204.q4cdn.com/645488518/files/doc_financials/2025/q4/QCOM-09-28-25-FY2025-10-K-Final.pdf
- **Q1 FY2026 10-Q** (period ended 2025-12-28; accepted by EDGAR 2026-02-04), read through the StockTitan mirror:
  - Alphawave closed on 2025-12-18 for US$2.3B.
  - The quarter also included "four other businesses for a total accounting purchase price of $846 million". The acquired assets were mainly US$226M of intangibles and US$611M of goodwill, allocated to QCT.
  - The four businesses are not named. The parts of the 10-Q I read do not mention Arduino. — https://www.stocktitan.net/sec-filings/QCOM/10-q-qualcomm-inc-de-quarterly-earnings-report-cebab8b158aa.html
  - Inference, not fact: Arduino is probably one of the four, but the filing does not say so.
- **DLA Piper** (2025-11-04) says it "has advised Qualcomm Technologies on the acquisition of Arduino". It gives no closing date. — https://www.dlapiper.com/en-cn/news/2025/11/dla-piper-advises-qualcomm-group-on-acquisition-of-open-source-hardware-and-software-business
- **Other searches with no closing date:** German ("Übernahme abgeschlossen"), Italian ("golden power", "perfezionata"), and "now part of Qualcomm" / "transaction closed".
- **Unchanged from round 2:** the closing falls between the 2025-10-07 announcement and the 2025-11-05 earnings call, where Qualcomm said "completed".

## 2. EN 18031 restrictions in 2026

**Verdict: STILL UNVERIFIED. No amendment was found.**

- **Latest RED amendment found:** Commission Implementing Decision (EU) 2026/2003 of 2026-09-04 (OJ 2026-09-07) amends 2022/2191. It covers EN 301 489-x, EN 50566 and similar standards. It does not mention EN 18031 or Art. 3(3)(d)/(e)/(f). — https://eur-lex.europa.eu/eli/dec_impl/2026/2003/oj
- **Other 2026 decisions checked (none about RED or EN 18031):**
  - 2026/1971 covers PPE. — https://eur-lex.europa.eu/eli/dec_impl/2026/1971/oj/eng
  - 2026/901 covers GPSR. — https://eur-lex.europa.eu/eli/dec_impl/2026/901/oj/eng
- **Industry sources:** A Würth Elektronik/Eurofins deck (2026) still describes the password restriction as current. — https://www.we-online.com/files/pdf1/eurofins-en18031-cra_wuerth-elektronik.pdf (search summary only)
- **DIN draft:** The draft I opened (402331001) is prEN 40000-10:2026, a CRA vertical standard, not an EN 18031 revision. — https://www.din.de/de/mitwirken/normenausschuesse/nia/entwuerfe/wdc-beuth:din21:402331001/toc-3698139/download
- **Searches tried with no amendment found:** "EN 18031-1:2024/A1", German "Einschränkung Passwort 2026 Durchführungsbeschluss", and "2022/2191 amending 2026".
- **Inference:** the restrictions in 2025/138 most likely still apply, but no EUR-Lex consolidated text dated after 2025-12-11 was read.

## 3. MOF 2007 ruling 台財稅字第09604545320號

**Verdict: CONFIRMED (still applied) + NEW FINDING.**

Source: 臺北高等行政法院 113 年度訴字第 948 號判決, 2024-12-19, 全興食品 v. 財政部北區國稅局. I extracted the full text. — https://judgment.judicial.gov.tw/FILES/TPBA/113%2c%e8%a8%b4%2c948%2c20241219%2c1.pdf

- **Ruling text:** The court quotes the 2007-10-03 ruling: for a split into a 100% subsidiary, absent improper tax avoidance, 「應以被分割資產之帳面價值作為實際成交價格」 for computing the splitting company's gain or loss.
- **Validity:** The court held that this ruling and the 2007-10-31 ruling (09604546420) 「內容與相關法律規定暨立法意旨無違，應可適用」.
- **Scope:** It agreed with the tax bureau that the book-value price applies 「不因採何種分割模式有別」. The case was a 兄弟式 split under §44, not a 100%-subsidiary split.
- **Result:** The claim was dismissed. The new company's tax cost for the transferred shares was the book value at the split (NT$337M), not the original cost (NT$546M). The disallowed loss was NT$210M.
- **Appeal:** No appeal decision was found in the search. 最高行政法院 113 上 616 (2026-01-08) is an unrelated case about undistributed earnings. — https://judgment.judicial.gov.tw/FILES/TPAA/113%2c%e4%b8%8a%2c616%2c20260108%2c1.pdf
- **No later ruling found:** no MOF ruling replacing 09604545320 turned up. The MOF law database still serves the companion ruling 09604546420 (page states no status). — https://law-out.mof.gov.tw/LawContent.aspx?id=GL008395
- **Implication (inference):**
  - The book-value rule cuts both ways. Low book value for self-developed IP means low taxable gain at the split.
  - The new company inherits that low tax basis, so a later sale of the IP or shares is taxed on the full appreciation.
  - Still needs a CPA's confirmation.

## 4. Korea and Southeast Asia distributors and prices

**Verdict: NEW FINDING (distributors). STILL UNVERIFIED (local-currency prices).**

- **Embedded Wizard** (official pricing and checkout page):
  - Studio costs €199 per month per seat, the ChoraC CLI €79 per month per seat, and Product Support €99 per month per user. Prices are net. Yearly billing "Save 17%". Perpetual licences are by request.
  - Distributors you can pick at checkout include:
    - **Dioiz (South Korea & Japan)**
    - **Simplify (Taiwan & China)**
    - Ruby Embedded Technologies (India)
    - Emprog (US, Canada and Mexico)
    - MIGTEC (Australia and New Zealand)
    - NeoMore (France), and others
  - — https://www.embedded-wizard.de/pricing
  - Dioiz is a Korean embedded engineering firm and the Korea/Japan partner of Oryx Embedded. — https://oryx-embedded.com/partners/dioiz.html
- **SEGGER (emWin)** authorised distributors:
  - Korea: In Semiconductor Inc., Seoul
  - Taiwan: Microtime Computer Inc., Taipei
  - Japan: EmbITeK Co., Ltd., Tokyo (= エンビテック; "Envitech" in round 2 is a romanisation of the same firm)
  - Singapore, Thailand, Vietnam, Malaysia, Philippines and Indonesia: Gopalam Embedded Systems and Testech Electronics (both Singapore)
  - India: four firms, including Embedded Systems Solutions and VVDN
  - — https://www.segger.com/purchase/distributors
- **Qt** partner directory (WebFetch of the JS page):
  - **OCUBE Co. LTD (South Korea)**: Technology + Channel Sales
  - **TECHKINETICS Pte Ltd (Singapore)**: Technology + Channel Sales
  - SECO (India, Taiwan) and CoreEL and Spark Magic (India): Channel Sales
  - "Qt Professional Services" covers South Korea, Japan and India.
  - No Channel Sales partner is listed for Thailand, Vietnam or Indonesia.
  - — https://qt.io/contact-us/partners
  - A Korean search for OCUBE returned nothing that independently confirms the listing.
- **Crank Storyboard:** the only Asian distributors found are AdvanTRAK (India, 2025-04-30) and Osaka NDS (Japan). None for Korea, Taiwan or SEA. — https://www.cranksoftware.com/press-release-crank-expands-global-reach-with-new-distribution-partner-advantrak
- **LVGL Pro:** sold direct in USD, with no reseller listed.
  - Starter: $990 per seat per year (up to 2 seats per company)
  - Developer: $3,990 per seat per year
  - Product: $20,000 per product
  - Platform: custom
  - Free for Renesas customers
  - — https://lvgl.io/pro/pricing (redirect from pro.lvgl.io/pricing)
  - A search snippet showed older figures ($1,000 Starter, $3,000 Business). Use the fetched page.
- **SquareLine Studio:** sold direct in USD.
  - Tiers: Personal (free, non-commercial), Small business (revenue + funding under US$150K), Business, Enterprise (quote).
  - The amounts did not render in the fetch.
  - — https://squareline.io/pricing/licenses
- **Local-language searches with no price found:**
  - Korean: "총판", "대리점", "가격" for Embedded Wizard, emWin and Crank
  - Vietnamese: "nhà phân phối … giá bản quyền"
  - Thai: "ตัวแทนจำหน่าย … ราคา"

## 5. Japan yen prices

**Verdict: NEW FINDING (one yen band). The rest are STILL UNVERIFIED (quote-only).**

- **Crank Storyboard (Osaka NDS on IPROS):** 「価格帯 100万円 ~ 500万円」, 「納期 ~ 1週間」. No licence detail. — https://mono.ipros.com/product/detail/2000808564
  - The SDTech sub-reseller listing says 「詳細はお問い合わせください」. — https://mono.ipros.com/product/detail/2001523956
  - ISB became a sales agent under Osaka NDS on 2024-04-01. — https://newscast.jp/smart/news/1781211
- **Embedded Wizard (Tegara):** still quote-only. — https://mono.ipros.com/product/detail/2000859227
  - The global page now shows €199 per month per seat, with Dioiz as the Japan distributor (see item 4).
  - TrustRadius lists €2,990 Small Business and €7,300 Professional as perpetual per-seat prices (third-party, date unknown). — https://www.trustradius.com/products/embedded-wizard/pricing (search summary only)
- **emWin (EmbITeK/エンビテック):** no yen price. The SEGGER list is US$14,980 for PRO and US$7,480 for BASE colour. — https://www.segger.com/purchase/pricing/emwin/
- **Qt for MCUs:** sold under Qt for Device Creation Professional/Enterprise. No yen price. — https://doc.qt.io/QtForMCUs/ja/qtul-licensing.html
- **HI exbeans UI Conductor:** no price. The only programme found is 2016 trial licences for AGL members. — https://www.atpress.ne.jp/news/107433
- **CRI Glassco:** no price. Revenue data from the FY9/2026 results briefing (event 2026-11-05, published 2026-11-11):
  - Mobility sales rose from ¥633M to ¥932M.
  - 「インドのオートバイに「CRI Glassco」が採用されたことにより、前期比で約3億円の増収」
  - The briefing adds that Indian adoption is still through exports by Japanese makers.
  - — https://finance.logmi.jp/articles/385976

## 6. Embedder (YC S25)

**Verdict: STILL UNVERIFIED (user, customer and funding claims). NEW FINDING (current pages).**

- **YC company page:** "building an autonomous firmware engineer", team size 6, San Francisco, founders Ethan Gibbs (CEO) and Bob Wei (CTO), 0 open jobs. It lists partnership news (SEGGER, Joulescope, Verkor, Zephyr Project). It makes no user, customer, funding or investor claims. — https://ycombinator.com/companies/embedder
- **embedder.com:** no user or customer counts, no funding information, no prices. It has a "TRY FREE" button and an "Embedder Free" launch headline. — https://www.embedder.com/
- **Original YC job page:** returns 404.
- **Job-board mirror (riseme.ai)** still shows the listing text:
  - "5,000+ engineers using the product"
  - "100+ paying customers across automotive, aerospace, medical, IoT, and consumer electronics"
  - backed by "CRV, Box Group, and Emerson Collective"
  - "$XM+ raised" (placeholder)
  - salary $120K–$250K, "Posted 6 months ago" (about May 2026), closed
  - — https://riseme.ai/jobs/embedder/dda9239e-aae2-4f11-9a47-29994bba7452/founding-ai-engineer
  - These are company recruiting claims. No investor announcement confirms CRV, BoxGroup or Emerson Collective.
- **Press:** RuntimeWire (2026-09-10) covers the SEGGER probe integration and gives no figures. — https://runtimewire.com/article/embedder-segger-ai-firmware-hardware-debugging
  - No TechCrunch article was found.
  - Aggregators disagree on funding: Neuronfeed says US$500K, a job listing says US$550K, and Crunchbase says pre-seed. None is authoritative.

## 7. Rightware Kanzi and the US BIS connected-vehicle rule

**Verdict: STILL UNVERIFIED (customer or press concern). NEW FINDING (ThunderSoft filing).**

- **Press and law-firm commentary:**
  - Searches of DLA Piper (2026-04-15), Venable, Taylor Wessing, Hogan Lovells and Automotive World found nothing that names Kanzi, Rightware or ThunderSoft. — https://www.dlapiper.com/insights/publications/2026/04/connected-vehicles-rule-takes-effect ; https://www.automotiveworld.com/topics/software-defined-vehicle/beyond-trade-policy-what-the-bis-connected-vehicle-rule-really-demands-from-automotive-software-teams/
  - DLA Piper confirms the dates: software designed or supplied on or after 2026-03-17 with a China nexus is now subject to the rule, and MY2027 sales are prohibited.
- **ThunderSoft 2024 annual report** (published 2025-04, report no. 2025-015). I extracted the full text. — https://pdf.dfcfw.com/pdf/H2_AN202504221660624764_1.pdf
  - Rightware Oy: Finland, 车载电子解决方案, 100% indirect, acquired by 企业合并 (p.209).
  - Goodwill: Rightware RMB 202.2M at end-2024. Asset-group carrying value RMB 344.96M against recoverable amount RMB 399.35M, so no impairment. The forecast assumes revenue growth of −2.73% to 13.12% and a 14.44% discount rate (pp.178–179).
  - Revenue by region in 2024: 欧美 RMB 1.033B (19.18%); 日本 10.27%; China 63.63% (p.18).
  - The report mentions a new Detroit office (p.16).
  - Keyword searches of the full text for 美国, 地缘, 出口管制, 实体清单 and 网联 found no statement on the US connected-vehicle rule. The risk section is said to be in 第三节 十一, but the text extracted from it has no risk list.
- **2025 annual report** (filed 2026-04-21): full text still not located. A broker note (CICC via cfi.cn) lists only generic risks (international situation, FX). — https://quote.cfi.cn/ybdata.aspx?id=20260423000515
- **Implication:** still my reading of the rule (see round 2), not a reported concern.

## 8. US flip from Taiwan

**Verdict: NEW FINDING (legal route in 企併法). STILL UNVERIFIED (typical costs and a Taiwan-specific step guide).**

- **企併法 §31(3)** (law last amended 2022-06-15): 「公司與外國公司進行股份轉換時，準用前二項、前二條及第二十一條規定。」 — https://law.moj.gov.tw/LawClass/LawAll.aspx?PCODE=J0080041
  - So a Taiwan company can become a 100% subsidiary of a foreign (for example Delaware) company by share exchange under §29.
  - §21 (applied by reference) requires:
    - the foreign company's law permits it
    - approval under that law
    - a Taiwan process agent
  - — https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080041&flno=21
  - §4: 股份轉換 is defined in item 5 and 外國公司 in item 8 (same law page).
- **Proposed share-exchange deferral** (企併法 §44-2 and related draft):
  - Shareholders could defer the AMT (基本所得額) on securities gains from a share exchange until they actually sell.
  - Only for holding companies that 國發會 certifies, and 「關係企業不得適用」.
  - Applies for 5 years after taking effect.
  - — https://kpmg.com/tw/zh/media/press-releases/2025/12/draft-amendments-insights-from-kpmg1.html
  - Third reading was scheduled for 2026-08-25 or 08-28. — https://udn.com/news/story/6656/9705721
  - The outcome was not confirmed. The law database still shows the 2022-06-15 amendment as the latest.
  - Inference: this targets domestic industrial holding companies, not flips to a foreign parent.
- **Taiwan commentary (weak source, author unknown, Substack 2025-05-02):**
  - Points to weigh when deciding on an offshore holding company: preferred-share terms, foreign-employee ESOP exercises needing 投審司 approval each time, the NT$1.5B prior-approval threshold for outbound investment, and CFC reducing the tax motive.
  - It gives no flip steps and no costs.
  - — https://yylallen.substack.com/p/194
- **Other context:** KPMG Taiwan (2025-08) says Delaware is Taiwanese firms' traditional choice for US incorporation, with Texas emerging. — https://kpmg.com/tw/zh/media/press-releases/2025/08/taiwanese-business-eye-expansion-in-texas.html
- **General flip mechanics (non-Taiwan):** stock-for-stock exchange at 1:1 or another ratio. Flips are costly and hard to unwind. No amounts given. — https://www.haynesboone.com/news/publications/the-flip-transaction-bringing-your-foreign-startup-into-the-us-investment-market ; https://www.orrick.com/Insights/2023/04/The-Australia-to-US-Flip-Up
- **Searches with no Taiwan flip cost or step guide:** 數位時代 / Meet創業小聚 queries, and "境外控股 換股 投審會 律師".

## 9. Gartner low-code vs vibe coding wording

**Verdict: STILL UNVERIFIED.**

- **Trade-press searches:** InfoWorld, Computerworld, TechTarget, The Register and "LCAPs quote Leow" all returned only vendor pages (Jitterbit, Mendix, OutSystems, Salesforce) and Gartner abstracts.
- **Fetch attempts:**
  - Jitterbit page returns 404. — https://www.jitterbit.com/report/gartner-report-why-ai-wont-replace-low-code-application-platforms
  - Mendix page returns 404. — https://www.mendix.com/?p=253477
  - web.archive.org cannot be fetched with this tool.
- **Status:** the line "Ultimately, vibe coding accelerates typing. LCAPs accelerate engineering." is still seen only in search snippets of vendor pages.

## 10. Final pass (5 searches/fetches)

**Verdict: nothing new.**

- **Developer count:** still only SlashData's total of about 47M developers. No embedded headcount. — https://www.slashdata.co/post/global-developer-population-trends-2025-how-many-developers-are-there
- **VS Code among embedded developers:**
  - JetBrains "The State of Embedded Development 2025" exists, based on Developer Ecosystem 2025. Its landing page shows no IDE share, headcount or display figures. — https://lp.jetbrains.com/the-state-of-embedded-2025
  - The Stack Overflow 2026 survey (VS Code first) is general, not embedded. — https://visualstudiomagazine.com/articles/2026/10/06/stack-overflow-2026-survey-vs-code-leads-visual-studio-takes-third.aspx
- **MCU designs with a display:** no figure. The Qt Group 2025 survey covers 420 embedded UI/UX designers (80% work on both MCU and MPU), not a display attach rate. — https://www.qt.io/hubfs/Designing-For-Embedded_Qt-Group-Market-Research.pdf?hsLang=en
