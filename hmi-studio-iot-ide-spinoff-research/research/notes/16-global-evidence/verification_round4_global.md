# 16 Global evidence: verification round 4

Date: 2026-11-20. Tools: WebSearch, WebFetch, and curl plus pdftotext/PyMuPDF on public PDFs (Qualcomm IR, cninfo, Clean Energy Ministerial). No browser tools.
Scope: the eight items still open after rounds 1–3 (see `verification_round3_*.md`).
Source labels: **primary** = company filing, regulator, official journal, or the issuer's own release or report; **secondary** = press, data aggregator, or a republished copy; **vendor** = a party with a commercial interest.

## Summary

| # | Item | Result | Key fact |
| --- | --- | --- | --- |
| 1 | Qualcomm–Arduino closing date | STILL UNVERIFIED (exact day); NEW (bounds narrowed by FY26 10-K and CES release) | The FY2026 10-K (FY ended 2026-09-27) never names Arduino. It reports "eight other businesses" acquired in FY26 for US$1.1B. By 2026-01-05 Qualcomm's CES release listed Arduino among completed IE-IoT acquisitions, and arduino.cc says "Qualcomm Technologies, Inc. has acquired Arduino". No closing press release, AGCM or EC filing, or Arduino blog post gives a date. |
| 2 | EN 18031 restrictions | CONFIRMED (restrictions still stand) | EUR-Lex consolidated 2022/2191 (version 15.11.2026) still lists EN 18031-1/-2/-3:2024 at rows 164–166 with the password, parental-control and 6.3.2.4 restrictions. The amending acts after 2025/138 are 2025/893, 2025/1741, 2025/2499 and 2026/2003. None mentions EN 18031. No EN 18031 A1 was found in the OJ. |
| 3 | Omdia 2025 industrial display | CONFIRMED (via reprints) | 2025 revenue US$3.4B (+24% YoY). Units −0.4% YoY. Ranking: AUO Display Plus, Innolux, Tianma. Quasi-military displays: 965k units (+5.9%). Omdia pages return 403 to fetch. |
| 4 | Embedder | STILL UNVERIFIED (funding and user counts); NEW (partnerships, domain) | Official 2026 releases (Verkor 2026-07-14, SEGGER 2026-09-10) give no funding, investor, user or customer numbers. The site moved to embedder.com and shows none either. Only aggregator VCBacked shows US$500K pre-seed (Sep 2025, YC). |
| 5 | ThunderSoft 2025 annual report | NEW (primary) | Rightware goodwill RMB 221.04M at end-2025 (from 202.21M; +18.83M FX only). No impairment. Asset group carrying RMB 366.66M vs recoverable 403.65M. Overseas revenue RMB 3.556B = 45.71% of the 7.778B total. No US/BIS/export-control risk statement. |
| 6 | Unit volumes (medical displays, HMI panels, thermostats) | STILL UNVERIFIED (no with/without-display split) | No public unit figures were found for medical displays. ABI/Omdia "HMI Shipments by Hardware Type" (Table 47, 2025–2035) exists, but no figures are public. Parks: US smart thermostat unit sales 8.1M in 2030. No source splits units with vs without a display. |
| 7 | IEA GEVO 2026 chargers | CONFIRMED (primary report text) | Public charging points >7M at end-2025. ~1.8M added in 2025 (+>33%). ~11 EVs per public point. Private LDV points >43M. China >65% of stock (4.7M). |
| 8 | Toitware ApS | CONFIRMED + CORRECTED dates | CVR 38984594, founded 2017-10-01, Aarhus. Bankruptcy decree 2024-08-20 (Retten i Aarhus). Final meeting 2025-10-28. Status "Opløst efter konkurs" from 2025-10-28. |

---

## 1. Qualcomm–Arduino closing date

**Result: STILL UNVERIFIED for the exact day. NEW: the FY2026 10-K and Qualcomm's own CES 2026 release narrow the context.**

- **Qualcomm FY2026 10-K** (FY ended 2026-09-27; the IR PDF `QCOM-09-27-26-10-K_Final-v2-IR.pdf`), full text extracted. **Primary.**
  - "Arduino" does not appear anywhere.
  - Note 9 (Acquisitions) names only Alphawave, closed **2025-12-18** for US$2.3B, and Modular Inc, closed **2026-07-28**.
  - Then: "Other. During fiscal 2026, we acquired eight other businesses for a total accounting purchase price of $1.1 billion … $313 million of intangible assets and $767 million of goodwill, with $694 million allocated to our QCT segment and $73 million allocated to our Data Center nonreportable segment."
  - Combined with the round-3 finding (Q1 FY26 10-Q: "four other businesses" for US$846M in the quarter 2025-09-29 to 2025-12-28), Arduino is almost certainly one of the four Q1 FY26 deals. It closed between 2025-09-29 and 2025-12-28, and in practice after the 2025-10-07 announcement. This is an inference; the filings never name it.
  - https://s204.q4cdn.com/645488518/files/content_files/QCOM-09-27-26-10-K_Final-v2-IR.pdf
- **Qualcomm CES release, "Qualcomm's IE-IoT Expansion Is Complete", dateline Las Vegas, 2026-01-05.** **Primary**, read through the edge-ai-vision.com reprint because qualcomm.com returned only the title.
  - It says the expansion was "fueled by the acquisition of Augentix, Arduino, Edge Impulse, FocusAI, and Foundries.io in the last 18 months".
  - It says explicitly that the Augentix acquisition is completed, but gives no date for Arduino.
  - https://www.qualcomm.com/news/releases/2026/01/qualcomm-s-ie_iot-expansion-is-complete--edge-ai-unleashed-for-d ; reprint: https://www.edge-ai-vision.com/2026/01/qualcomms-ie‑iot-expansion-is-complete-edge-ai-unleashed-for-developers-enterprises-oems/
- **Arduino Qualcomm FAQ** (undated, © 2026): "Yes, Qualcomm Technologies, Inc. has acquired Arduino." No date. **Primary.** https://www.arduino.cc/qualcomm-faq/
- **Arduino blog, 2025-11-21** (ToS / privacy policy update): "The Qualcomm acquisition doesn't modify how user data is handled…". It treats the deal as done but gives no closing date. **Primary.** https://blog.arduino.cc/2025/11/21/the-arduino-terms-of-service-and-privacy-policy-update-setting-the-record-straight/
- **DLA Piper, 2025-11-04**: says it advised Qualcomm Technologies on the acquisition, but does not say the deal had closed and gives no date. **Secondary** (adviser). https://www.dlapiper.com/en/news/2025/11/dla-piper-advises-qualcomm-group-on-acquisition-of-open-source-hardware-and-software-business
- **The Maker's Workbench, 2026-01-05 update**: "has since been completed", citing the CES 2026 release. No date. **Secondary.**
- **Wikipedia (Arduino)**: says "now owned by Qualcomm". Its only citation is CNBC on 2025-10-07, the announcement, so it gives no closing date. **Secondary.**
- **Dead ends**:
  - No AGCM or EC merger notice was found, which is consistent with a below-threshold deal.
  - The Italian golden-power annual report to Parliament (camera.it OCD177-9112, 2026-07-08) returned 403.
  - No Qualcomm "completes acquisition of Arduino" release exists in search results.

## 2. EN 18031 restrictions (RED Delegated Regulation 2022/30)

**Result: CONFIRMED. The restrictions still stand as of the consolidated version dated 15.11.2026.**

Note: Implementing Decision (EU) 2025/138 is an *amending* act. It added EN 18031 to the base act, Implementing Decision (EU) 2022/2191, so the consolidation to check is that of 2022/2191.

- **EUR-Lex "ALL" page for 32022D2191**, "Modified by" list. **Primary.**
  - Amending acts: 32023D2392, 32023D2669, **32025D0138** (EN 18031, 30.01.2025), 32025D0893 (15.05.2025), 32025D1741 (14.08.2025), 32025D2499 (11.12.2025) and 32026D2003 (07.09.2026).
  - Current consolidated version: **15.11.2026**.
  - https://eur-lex.europa.eu/legal-content/EN/ALL/?uri=CELEX:32022D2191
- Each amending act after 2025/138 was checked. **None mentions EN 18031, Delegated Reg. 2022/30, or Art. 3(3)(d)/(e)/(f)**:
  - 2025/893 (14 May 2025): RED standards, Art. 3(1)(b) and 3(2) only. https://eur-lex.europa.eu/eli/dec_impl/2025/893/oj/eng
  - 2025/1741 (13 Aug 2025): video links, satellite earth stations, base stations, SRD. https://eur-lex.europa.eu/eli/dec_impl/2025/1741/oj/eng
  - 2025/2499 (9 Dec 2025, OJ 11 Dec 2025): SRD and on-board aircraft mobile communications. It adds EN 300 220-2 V3.3.1, EN 302 480 V3.1.1, EN 302 729-1 V3.1.1, EN 303 659 and EN 305 550-6. https://eur-lex.europa.eu/eli/dec_impl/2025/2499/oj/eng
  - 2026/2003 (4 Sep 2026): radiodetermination and location tracking (already known from round 3).
  - 2026/803 and 2026/2032 came up in search but amend other base acts (rail 2023/2584; toys 2023/740).
- **Consolidated text CELEX 02022D2191-20261115, Annex I rows 164–166.** **Primary**, with EUR-Lex's own caveat that consolidation is a documentation tool.
  - **164 EN 18031-1:2024** [Art. 3(3)(d)]:
    - The 'rationale' and 'guidance' sections confer no presumption.
    - No presumption "if, when applying its clauses 6.2.5.1 and 6.2.5.2, the user is allowed not to set and use any password."
  - **165 EN 18031-2:2024** [Art. 3(3)(e)]:
    - The same rationale/guidance and password restrictions apply.
    - In addition, for clauses 6.1.3–6.1.6, there is no presumption where "parental or guardian access control is not ensured" (clauses 6.1.3.4.2, 6.1.4.4.2, 6.1.5.4.2, 6.1.6.4.2).
  - **166 EN 18031-3:2024** [Art. 3(3)(f)]:
    - The same rationale/guidance and password restrictions apply.
    - In addition, the clause 6.3.2.4 assessment criteria confer no presumption.
  - https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:02022D2191-20261115
- **EN 18031-x:2024/A1**: no amendment was found cited in the OJ or in search. The only 2026 standardisation item found is C(2026)7861 (10 Nov 2026), which amends standardisation request C(2025)618 (CRA) on deliverable deadlines. It is not a harmonised-standards listing. https://ec.europa.eu/transparency/documents-register/api/files/C(2026)7861_0/de00000001090398

## 3. Omdia 2025 industrial display results

**Result: CONFIRMED through two independent reprints. The Omdia PR page and the Omdia "Display Dynamics" page (om143843) both return HTTP 403 to fetch.**

- Original Omdia PR (Jan 2026): https://omdia.tech.informa.com/pr/2026/jan/industrial-display-revenues-grow-24percent-yoy-in-2025-led-by-auo-display-plus (403). Omdia analysis page: https://omdia.tech.informa.com/om143843/display-dynamics--january-2026-industrial-display-revenue-grew-24-yoy-in-2025-led-by-auo-display-plus (403).
- **Display Daily, 2026-01-21**, "Industrial Display Panel Revenue Climbs 24% to $3.4 Billion as Gaming and Action Cameras Drive Growth". **Secondary.** https://displaydaily.com/?p=385009
- **iTiger (Tiger Brokers news), 2026-01-22**, "Omdia: Global Industrial Display Panel Makers to Generate $3.4 Billion Revenue in 2025, Up 24% YoY". **Secondary.** https://www.itiger.com/news/1116948322
- Figures, identical in both reprints:
  - 2025 industrial display panel revenue: **US$3.4B, +24% YoY**.
  - Unit shipments: **−0.4% YoY**. iTiger attributes this to Europe's phase-out of disposable e-cigarettes.
  - About **three-quarters** of the incremental revenue came from gaming handheld/console displays (**+101% YoY**) and action cameras (**+23% YoY**). Mini-LED backlights in marine and drone applications raised ASPs.
  - AMOLED share of gaming handheld panels: **19%**.
  - Revenue ranking: **1 AUO Display Plus** (POS-led), **2 Innolux** (gaming handhelds), **3 Tianma** (HMI, handhelds, home appliances, home medical). No share percentages are given.
  - Military-like displays: **965,000 units in 2025 (+5.9%)**, forecast **+6.5% in 2026**. Leaders: AUO Display Plus, Tianma, BOE.
  - Source named: Omdia "Industrial and Commercial Display & OEM Intelligence Service". Analyst: TY Huang.
- Context: the July 2025 Omdia forecast had projected 2025 shipments of 290.6M (+3.4% from 281.1M). The Jan 2026 actuals (−0.4%) supersede it, and the absolute 2025 unit total was not given in the reprints.

## 4. Embedder (YC S25, embedded-firmware AI agent)

**Result: STILL UNVERIFIED for funding, investors, "5,000+ engineers" and "100+ paying customers". NEW: 2026 partnership releases and a domain change.**

- **YC company page** (fetched 2026-11-20). **Primary.**
  - Profile: founded 2025, team size 6, San Francisco, S25, founders Ethan Gibbs (CEO) and Bob Wei (CTO), partner Jared Friedman.
  - No user, customer or funding figures.
  - News listed:
    - 2026-09-10 SEGGER partnership
    - 2026-09-09 "Embedder Joins the Zephyr Project"
    - 2026-07-22 Joulescope partnership
    - 2026-07-14 Verkor partnership
    - 2026-05-15 start-up profile (epdtonthenet)
  - https://www.ycombinator.com/companies/embedder
- **PR Newswire, Embedder × Verkor, 2026-07-14.** **Primary/vendor.**
  - Boilerplate: "Engineering teams across automotive, defense, IoT, and consumer electronics are using Embedder…".
  - No user counts, customer counts, funding or investors.
  - https://www.prnewswire.com/news-releases/embedder-and-verkor-partner-to-take-agentic-ai-from-chip-design-to-working-firmware-302824710.html
- **SEGGER news, 2026-09-10.** **Primary (partner).** Embedder's agent gets direct J-Link/J-Trace access. No funding or user figures. https://www.segger.com/news/260910-partnership-embedder-segger/
- **embedder.dev now 301-redirects to embedder.com.** **Vendor.**
  - Claims "Support for 600+ platforms across 14 manufacturers and 5,800+ peripherals".
  - Other news items: "Embedder Free" launch, RISC-V International strategic membership, and an Arduino/Qualcomm build challenge.
  - No user, customer, funding or investor numbers. The MCU-vendor logos are ecosystem partners, not customers.
  - https://embedder.com/
- **VCBacked**: US$500K, pre-seed, Sep 2025, investor Y Combinator. Last updated 2025-09-09, so it is stale. **Secondary aggregator.** https://www.vcbacked.co/company/embedder
- No funding announcement (seed/Series A) since mid-2026 was found.
- The round-3 job-board mirror claims (5,000+ engineers, 100+ paying customers, CRV/BoxGroup/Emerson Collective) remain uncorroborated by any official page.
- Caution: "Embedd" (London, US$2.7M pre-seed led by Seedcamp, Aug 2026, and a Zephyr Silver member in Feb 2026) is a **different company**.

## 5. Rightware / ThunderSoft (中科创达, 300496.SZ) 2025 annual report

**Result: NEW, from a primary filing.**

Source: 中科创达《2025年年度报告》, cninfo announcement 1225141001, published **2026-04-22**. It is 238 pages and was text-extracted with PyMuPDF. **Primary.**
http://static.cninfo.com.cn/finalpage/2026-04-22/1225141001.PDF (summary: …/1225141000.PDF)

- **Group results**:
  - Revenue **RMB 7,778.04M** (+44.45%).
  - Net profit to parent **RMB 450M** (+10.47%).
  - Non-GAAP (扣非) net profit **RMB 344M** (+96.43%).
  - By business line: Smart IoT RMB 3,605.0M (+133.26%), Smart Auto RMB 2,655.8M (+9.91%), Smart Software RMB 1,517.3M (+6.64%).
- **Revenue by region** (分地区):
  - China RMB 4,222.53M (54.29%, +23.23%)
  - Europe & Americas (欧美) RMB 2,456.49M (31.58%, +137.80%)
  - Japan RMB 654.28M (8.41%, +18.30%)
  - Other countries RMB 444.74M (5.72%, +19.53%)
  - **Overseas total RMB 3,555.5M = 45.71%** (2024: 36.37%). The MD&A text: "来自于欧美、日本等海外国家或地区的营业收入35.56亿元，较上年增长81.57%".
- **Overseas assets** (境外资产): net assets **RMB 1,652.02M**, 15.98% of company net assets, profitable. "Significant impairment risk?" is answered 否 (no).
- **Goodwill note 18** (商誉):
  - Rightware Oy opening balance **RMB 202,206,315.41**, plus **18,834,671.03** from FX translation (no business-combination additions). Closing balance **RMB 221,040,986.44**.
  - The note has no impairment-provision table, and impairment is blank for every CGU.
  - Total group goodwill: RMB 604.43M.
- **Rightware impairment test** (value in use):
  - CGU carrying amount **RMB 366,660,183.87** vs recoverable amount **RMB 403,654,412.11**, about 10% headroom.
  - Forecast 2026–2030: revenue growth **2%–7%**, profit margin **14.19%–17.54%**, discount rate **14.71%**.
  - Terminal assumptions: 0% growth and 17.20% margin.
  - Small headroom: a modest miss would trigger impairment. This is analyst inference, not stated in the report.
- **Rightware Oy** (Finland, functional currency EUR) is listed as a 100%-owned subsidiary, acquired by business combination (企业合并), business "车载电子解决方案".
- **Kanzi R&D**: project "Kanzi2025", spend **RMB 26,877,137.95**, status 已完成. It builds architecture tools for the long-term roadmap (cross-platform Studio, an engine running on Web/iOS) and upgrades **Kanzi One**. A separate "CustomerPortal" project (RMB 156,824.69) built the Kanzi user portal.
- **US/BIS**: the full text has no occurrence of 出口管制, 实体清单, 制裁, 地缘 or 美国商务部. The risk section (可能面临的风险) lists only three risks: new-business development, R&D effectiveness and receivables. **No mention of the US BIS connected-vehicle rule or customer concerns.** "美国" appears only in a director biography and subsidiary listings (e.g., Thundercomm America Corporation).

## 6. Unit volumes: medical displays, HMI operator panels, building automation and thermostats

**Result: STILL UNVERIFIED. No public source splits units with vs without a display.**

- **Industrial HMI operator panels**:
  - The ~1.4M units/yr figure (≈US$2,000 ASP) still traces only to LP Information / GIR report listings. Their methodology is not shown. **Secondary/vendor.** https://www.marketresearch.com/LP-Information-Inc-v4134/Global-HMI-Operator-Panels-Growth-43279043
  - ABI Research (now part of Omdia), "Industrial Automation Hardware", published 2025-10-17, has the tables "HMI Shipments by Hardware Type, World Markets: 2025 to 2035" (Table 47), "by Region" (50), "by Industry" (53) and "Fixed HMI Shipments by Industry" (56). It is paywalled, no figures are public, and nothing indicates a display/no-display split. **Primary (analyst, paywalled).** https://www.abiresearch.com/market-research/product/7785289-industrial-automation-hardware
  - One aggregator claims Chinese "installations surpassed 18.25 million units". Its definition is unclear, so it is not usable.
- **Medical device displays**:
  - No public unit total was found.
  - Omdia's Medical Imaging & Specialty Displays Tracker (clinical review, diagnostic, surgical) is paywalled. https://omdia.tech.informa.com/om005559/medical-imaging-specialty-displays-tracker-h
  - iData ("Surgical Display and PACS Monitor Units Sold, Global, 2025") and APO Research (K units) show unit tables, but the excerpts give no values.
  - Revenue-only estimate: Market Intelo puts the medical-grade diagnostic display market at US$1.6B (2025). **Secondary/vendor.**
- **Smart thermostats**:
  - ABI's 67.2M shipments by 2030 is unchanged (prior round). https://www.prnewswire.com/apac/news-releases/the-smart-thermostat-sector-heats-up-with-67-2-million-devices-shipped-by-2030--302187759.html
  - **Parks Associates (2025-08-13): US smart thermostat unit sales 8.1M in 2030, revenue >US$1.1B.** No display split. **Secondary (analyst PR).** https://www.parksassociates.com/blogs/energy-pr/market-for-smart-thermostats-will-reach-unit-sales-of-81-million-in-2030-with-annual-revenues-exceeding-11-billion
  - A touchscreen-thermostat report gives only a revenue split (Wi-Fi touchscreen ≈75% of touchscreen-thermostat revenue). It gives no unit split and is a low-quality source.

## 7. IEA Global EV Outlook 2026: EV chargers

**Result: CONFIRMED from the report text.**

The iea.org pages sit behind a Cloudflare challenge (403), so the full PDF copy hosted by the Clean Energy Ministerial was used instead. It is "Revised version, May 2026", 295 pp. **Primary** (IEA report; the host is CEM, an IEA-linked ministerial forum).
https://www.cleanenergyministerial.org/content/uploads/2026/05/globalevoutlook2026.pdf (official page: https://www.iea.org/reports/global-ev-outlook-2026/electric-vehicle-charging)

Chapter 6 (2025 actuals):
- "In 2025, nearly **1.8 million** public charging points were added to the global stock, representing an increase of **more than 33%** compared to the previous year… The total stock of public charging points worldwide reached **more than 7 million** at the end of 2025."
- About **11 electric LDVs per public charging point** (similar to 2024). **4.5 kW** of public capacity per electric LDV. Global average public charger speed is about **50 kW**.
- Private LDV charging points: **more than 43 million** in 2025, supporting about **76 million** electric LDVs. About one-third are in China, one-third in Europe, and one in six in the US.
- Ultra-fast (≥150 kW) chargers are close to **15%** of global public points.
- **China**:
  - More than **65%** of public points.
  - Stock grew from about 3.4M (end-2024) to **more than 4.7M** (end-2025), over 75% of global growth.
  - About 1.3M were added in 2025.
  - Fast and ultra-fast chargers grew from 1.5M to **2.2M** (+40%).
  - Average speed over 55 kW. About 10 EVs per point.
  - Plan: more than +60% national network by end-2027.
- **Europe**:
  - Public points grew about **+20%**. Additions were about 260k in 2025 (record about 270k in 2024).
  - Netherlands 210k, Germany 196k, France 185k, UK about 116k (+>30%).
  - EU: 11 EVs per point, about 3 kW per EV.
  - Ultra-fast chargers grew 30%.
- **US**:
  - Record additions (+20% vs 2024).
  - **3%** of global public points vs 10% of the electric LDV stock.
  - Fast and ultra-fast chargers about **70k** (+30%). Slow chargers more than 160k.
  - 33 EVs per point. About 235k public points in 2025 (ch. 9/11).
- **Korea**: fast chargers 47k to 51k (+10%).
- **India**: 88k public points at end-2025.
- **Heavy-duty vehicles**: more than 70k HDV-accessible public points, about 1,200 truck-dedicated.

Outlook (Current Policies Scenario):
- More than 350M charging points added 2026–2035. Public points are 5% of the 2035 stock.
- EVs per public point rise from about 11 (2025) to 19 (2035).
- Average rated power of public points rises from about 50 kW to nearly 65 kW.
- China's public stock reaches nearly 18M by 2035.
- Europe reaches more than 4.3M public points by 2035. The EU reaches 2M by 2030 and about 3.5M by 2035.
- US reaches more than 420k public points by 2035.

Secondary corroboration: GreentechLead, 2026-05-20, quotes the same figures. https://greentechlead.com/electric-vehicle/global-ev-charging-infrastructure-expands-rapidly-as-ultra-fast-chargers-and-investments-surge-53460

## 8. Toitware ApS (Denmark)

**Result: CONFIRMED (the company went bankrupt and has been dissolved). CORRECTED: precise dates added.**

datacvr.virk.dk and cvrapi.dk both returned 403 to fetch. The data below comes from two CVR/Statstidende mirrors, which agree with each other. **Secondary**, but the mirrors republish official CVR (Erhvervsstyrelsen) and Statstidende data.

- **Identity**: Toitware ApS, **CVR 38984594**, founded **2017-10-01** (ApS), address Jægergårdsgade 97, 3., 8000 Aarhus C. Industry DB07 620100 (computer programming), recoded to 621000 from 2025-01-01.
- **Statstidende notices** (Retten i Aarhus; trustee: advokat Bo Christensen):
  - 2024-08-22, notice S20082024-155: **bankruptcy decree (konkursdekret) dated 2024-08-20**. Creditors were asked to file within 4 weeks.
  - 2025-03-20, S18032025-297: notice of claims hearing (fordringsprøvelse) for § 95 claims on 2025-04-03.
  - 2025-07-04, S02072025-110: final accounts and closing (slutning). Final meeting 2025-08-28. Proposed dividend **35.169386%** on § 95 (employee-priority) claims.
  - 2025-08-21, S19082025-187: notice of claims hearing on 2025-09-05.
  - 2025-09-23, S19092025-83: final accounts and closing (slutning). Final meeting **2025-10-28**. Proposed dividend **2.181796%** on § 97 (ordinary unsecured) claims.
- **CVR status**: credit status "Under konkurs" from **2024-08-20**. Status **"Opløst efter konkurs"** (dissolved after bankruptcy) and struck off from **2025-10-28**.
- Sources:
  - ownr.dk (Statstidende mirror): https://ownr.dk/companies/statstidende/38984594
  - danskcvrapi.dk: https://danskcvrapi.dk/en/news/38984594-ophor-2025-10-28 ; https://danskcvrapi.dk/en/news/38984594-kredit-2025-10-28-opl-st-efter-konkurs ; https://danskcvrapi.dk/nyt/38984594-kredit-2024-08-20-under-konkurs
- Implication: "stopped operating in 2024" is consistent with the record. Operations ended with the bankruptcy decree on 2024-08-20, and the legal entity was dissolved on 2025-10-28.
