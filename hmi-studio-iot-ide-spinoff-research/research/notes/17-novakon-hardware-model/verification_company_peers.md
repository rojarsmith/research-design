# Verification pass: NOVAKON company facts and HMI peer financials

Date of research and access for all URLs: **2026-11-20**.
Scope: open items from `novakon_company_and_prices.md` and `peer_financials.md` (both in this folder). Those files were read but not edited.

Labels:
- **[P]** primary: a regulatory or exchange filing, a government register, or the company's own filing or official document
- **[S]** secondary: news, aggregator, reseller or data-broker page
- **[I]** inferred or computed by me from [P] or [S] figures (the method is stated)
- **[U]** unverified

Result codes: **confirmed / corrected / new / still unverified / not public**.

Text in the Chinese-language MOPS and SSE/SZSE PDFs cannot be extracted (their fonts have no Unicode map). I read the relevant pages by rendering them to images. English MOPS versions were text-extracted.

---

## Item 1. iBASE–Novakon related-party transactions, FY2024, FY2025 and 9M-2026

**Result: new (FY2024 and 9M-2026 figures); FY2025 not disclosed by name; correction to the "parent manufactures for Novakon" inference.**

### Figures (NT$ thousand) [P]

| Period | iBASE sales to Novakon | iBASE trade receivable from Novakon (period end) | iBASE trade payable to Novakon (period end) | iBASE rental income from Novakon | Guarantee deposit received from Novakon | Source |
|---|---|---|---|---|---|---|
| FY2022 | 48,392 | — | — | — | — | earlier note (iBASE FY2022 AR) |
| FY2023 | 26,661 | — | 338 | 2,175 | 633 | earlier note; parent-only FS 2024 (2023 comparatives) |
| **FY2024** | **12,420** | **1,301** | 357 | 3,674 | 933 | Consolidated FS 2024 Table 6; parent-only FS 2024 Note 31 |
| **FY2025** | **Not listed** (absent from Table 6; parent-only FS shows subsidiaries only in aggregate) | not listed | not named (aggregate trade payables to subsidiaries 502) | **5,634** | 1,446 | Consolidated FS 2025 Table 6; parent-only FS 2025 Note 28 |
| **9M-2026 (Jan–Sep)** | **21,997** | **12,060** (at 2026-09-30) | — | — | — | Q3-2026 consolidated FS (Chinese), 附表六 「母子公司間業務關係及重大交易往來情形」: 「速博康公司 / 銷貨 / 21,997 / 與非關係人無明顯差異」, 「應收帳款－關係人 12,060」 |

Details:
- FY2024 Table 6 ("Intercompany relationships and significant intercompany transactions", relationship code a = company to subsidiary): "Novakon Co., Ltd. a Sale 12,420 Not significantly different from non-related parties" and "Novakon Co., Ltd. a Trade receivables 1,301". [P]
- FY2025 Table 6 lists only TMC UK (sale 352,100), IBASE USA (sale 256,229), IBASE Singapore (sale 42,876), IBASE Japan (sale 15,802), and other receivables from Shanghai and Japan. **Novakon does not appear.** The smallest item listed is 10,544. In FY2024 the table went down to 1,301, so the FY2025 cutoff appears higher, and the FY2025 Novakon amount is unknown; it is probably below about NT$10–15M [I]. The parent-only FS 2025 names Novakon only for rental income and the guarantee deposit, not for sales. [P]
- Direction: all disclosed flows are **iBASE → Novakon sales**. Purchases from Novakon are tiny: iBASE trade payables to Novakon were NT$338k at end-2023 and NT$357k at end-2024 [P]. iBASE's total purchases from all subsidiaries were only NT$3.6–4.1M a year (FY2024–FY2025) [P].
- Novakon rents space from iBASE. Rental income rose from NT$2.2M (2023) to 3.7M (2024) to 5.6M (2025) [P]. This fits the shared HQ address and possibly a larger footprint [I].
- Share of Novakon revenue [I]: iBASE sales to Novakon equal 36.6% of Novakon revenue in FY2022, 28.3% in FY2023, **8.7% in FY2024** (12,420 / 142,806), and an unknown share in FY2025 when Novakon revenue was 230,628.
- **Reading [I].** Intra-group supply shrank sharply in FY2024–FY2025 while Novakon's revenue grew 61%. It then rose again in 2026: NT$22.0M in 9 months, with NT$12.1M still unpaid at 30 Sep. Only a small part of Novakon's cost of goods can be bought from iBASE. **The earlier inference that "production is most likely sourced from the parent" should be weakened.** iBASE supplies some goods (boards, panel-PC platforms or finished units: the filings do not say which), but most of Novakon's COGS must come from third-party suppliers or contract manufacturers. Some may also be booked through other channels.
- No filing text says iBASE manufactures **for** Novakon. Related relevant text:
  - Consolidated FS (FY2024 and FY2025, subsidiaries note) describe Novakon's main business as "Manufacture of computers and peripheral equipment" [P].
  - The Q3-2026 investee table (附表, 被投資公司) gives Novakon's business as 「電腦及其週邊設備、資料儲存媒體製造及買賣」 [P].
  - The annual report describes the group's "division of work" only generically: "mutual support of product technology, production capacity, marketing and services" [P].
  - iBASE's own product list (FY2025 AR) includes "ARM-based HMI" and "ODM Panel PC", and says it develops "HMI human-machine interface with x86 or ARM architecture" touchscreen IPCs [P]. iBASE therefore has in-house HMI capability. Whether it builds Novakon's units is not stated.

### Other new facts from the same filings [P]
- **9M-2026 results.** As of 2026-09-30 (Q3-2026 investee table), iBASE holds 4,816,000 shares (70.00%) with an original cost of NT$124,633k and a carrying value of NT$152,057k. Novakon's **net income for Jan–Sep 2026 was NT$65,058k**, of which iBASE's share was NT$45,540k. Nine months of 2026 already exceed FY2025's full-year net income of NT$49,954k.
- **Share-count discrepancy.** The Q3-2026 table still shows 4,816,000 shares. GCIS shows iBASE at 6,216,000 shares after the capital change approved on 2026-05-26 (prior note). The original cost is unchanged, so the increase may be a stock dividend (capitalised earnings) that the filing table does not yet reflect. [I/U]
- **Probable 2025 cash dividend.** The iBASE parent-only FS 2025 equity-method schedule rolls the Novakon carrying value from 117,661 to 128,365, with a share of profit of 34,968 and a decrease of 24,947. The decrease is probably a cash dividend paid to iBASE, about NT$5.2 per share [I].
- **Novakon holds a listed stock.** The Q3-2026 securities table (附表二) shows 速博康 holding 668,000 shares (2.29%) of 懷藝精密科技, at a fair value of NT$11,490k [P].
- **Auditors.** Novakon's FY2024 statements were audited by other auditors, not iBASE's principal auditor (FY2024/FY2025 audit reports, "Other Matter" paragraph). In FY2025 Novakon is no longer on that list [P].

### URLs [P]
MOPS document server: the step=9 page returns a one-time /pdf/ link.
- Consolidated FS FY2024 (English): https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=A&co_id=8050&filename=202404_8050_AIA.pdf (Table 6, p.82; subsidiaries note)
- Parent-only FS FY2024 (English): …&filename=202404_8050_AIC.pdf (Note 31, pp.59–61)
- Consolidated FS FY2025 (English): …&filename=202504_8050_AIA.pdf (Table 6)
- Parent-only FS FY2025 (English): …&filename=202504_8050_AIC.pdf (Note 28, pp.58–60)
- Q3-2026 consolidated FS (Chinese): …&filename=202603_8050_AI1.pdf (附表六 p.68; 附表二 p.64; investee table p.66)
- Annual reports FY2024/FY2025 (English): kind=F, filenames 2024_8050_20250620FE4.pdf and 2025_8050_20260626FE4.pdf (product lines p.~96; affiliates section)
- Filing listing: POST https://doc.twse.com.tw/server-java/t57sb01 with step=1&co_id=8050&year=113|114|115&mtype=A

---

## Item 2. NOVAKON factory registration

**Result: confirmed absent (new, primary). NOVAKON has no registered factory in production status.**

- Source: 經濟部 "登記工廠名錄" (生產中工廠清冊), data.gov.tw dataset 6569. The file is 11510.csv (ROC 115/10, i.e. Oct 2026), dated 2026-11-19. It has 101,527 factories and carries the 統一編號 field. [P]
  - Dataset page: https://data.gov.tw/dataset/6569 → https://www.ida.gov.tw/opendata/02/SDD6569.csv → https://serv.gcis.nat.gov.tw/RDownLoad/Data/statistical/%E7%94%9F%E7%94%A2%E4%B8%AD%E5%B7%A5%E5%BB%A0%E6%B8%85%E5%86%8A.zip
- No row matches 速博康 or 統編 29176695. [P, by absence]
- iBASE (統編 70479315) has **four** factories in production status [P]. All four list products 271 電腦及其週邊設備, 269, 273, 281 and 289. The factory head is the same person who is Novakon's supervisor.

| Factory | Address | Registered |
|---|---|---|
| 三重廠 | 新北市三重區興德路129號6樓 | 2005-03-29 |
| 新莊廠 | 新北市新莊區化成路83巷17號1–3樓 | 2011-11-08 |
| 平鎮廠 | 桃園市平鎮區民族路雙連二段118巷12號 | 2020-05-22 |
| 平鎮二廠 | 桃園市平鎮區民族路雙連二段118巷10號 | 2021-06-18 |

- Limits: the list covers only factories in 生產中 (in production) status. It does not exclude a closed or suspended historic registration, or a small workshop exempt from registration (below the factory-registration threshold). serv.gcis.nat.gov.tw's interactive factory search was not used.
- **Conclusion.** NOVAKON's Taiwan production claim ("臺灣研發、生產") must rely on other companies' factories, iBASE or third parties [I]. Together with Item 1 (iBASE supplies only part of Novakon's COGS), a third-party Taiwan EMS/ODM for at least part of the line is plausible but undocumented [U].

---

## Item 3. Kinco (688160) and Xinje (603416) 2026 half-year reports

**Result: new.** Half-year reports give no unit volumes.

### Kinco 步科, H1-2026 report (AN202608251828413394) [P]
URL: https://pdf.dfcfw.com/pdf/H2_AN202608251828413394_1.pdf

**Group results (p.35).**
- Revenue RMB 447,690,762.94, +44.00% year on year.
- COGS RMB 299,156,872.22.
- Gross margin **33.18%** [I]. H1-2025 was 35.50% [I]: revenue 310.90M, COGS 200.54M.
- Net profit attributable to the parent RMB 36.33M, +39.16%.
- The report says COGS grew faster than revenue because "部分原材料市场供给偏紧、采购价格上涨" and because the new base is still ramping up.

**By product type (Note 61(2), p.178).**

| Product type | Revenue (RMB) | COGS (RMB) | Gross margin [I] |
|---|---|---|---|
| **控制系统** (HMI + PLC + IoT) | **118,224,557.57** | 69,806,669.39 | **40.95%** |
| 驱动系统 | 325,409,263.73 | 228,418,791.49 | 29.81% |
| 其他 | 3,733,577.83 | 690,935.60 | — |

For comparison, FY2025 控制系统 GM was 41.33% (prior note), so the control-system margin held at about 41%.

**By industry (MD&A, p.15).**
- 机器物联网行业 (人机界面, 触控一体机, 物联网盒子, 云平台) revenue RMB **102.7355M**, +31.31%, 22.95% of revenue.
- New products: F2 MAX HMI and the IWP web-type HMI, with up to 2 GB RAM / 64 GB storage and up to 21.5". Software DToolsPro V1.10.
- 机器人行业 revenue RMB 243.38M, +62.31%, 54.36% of revenue.

**Units: not disclosed in H1.**

### Xinje 信捷, H1-2026 report (AN202608261828480404) [P]
URL: https://pdf.dfcfw.com/pdf/H2_AN202608261828480404_1.pdf

**Group results (pp.22, 35).**
- Revenue RMB 1,101,659,608.04, +25.60%.
- COGS RMB 701,759,988.02.
- Gross margin **36.30%** [I]. H1-2025 was 38.29% [I]: 877.15M revenue, 541.34M COGS.
- Net profit attributable RMB 1.55亿, +21.98%.

**By product line (MD&A text, p.22).**
- PLC 3.66亿元 (33.20%)
- 驱动系统 5.64亿元 (51.23%)
- **人机界面 1.00亿元**
- 具身智能 0.60亿元

For comparison, FY2025 HMI revenue was RMB 205.36M, so H1 is about half of last year's annual run rate.

**Not disclosed in H1 [P, by absence]:** HMI gross margin and units. The revenue note (p.157) splits only "设备销售及其他主营业务" against other business, and domestic (1,076.05M) against overseas (25.61M).

---

## Item 4. Unitronics (TASE: UNIT) gross margin, 2023–2025

**Result: confirmed about 50% (primary). The about 65% figure (craft.co) is wrong.**

Source: Unitronics (1989) (R"G) Ltd, Periodic and Annual Report for 2025 (English translation), Board of Directors report §2.2, and Regulation 10A quarterly table [P]:
https://www.unitronicsplc.com/Download/unitronics_shared/Financial%20/2025%20Periodic%20Report.pdf

| NIS thousand (consolidated) | 2025 | 2024 | 2023 |
|---|---|---|---|
| Revenues | 154,779 | 192,236 | 211,671 |
| Cost of revenues | 77,985 | 93,954 | 106,096 |
| Gross profit | 76,794 | 98,282 | 105,575 |
| **Gross margin (as printed)** | **49.6%** | **51.1%** | **49.9%** |

**Quarterly GM in 2025:** 49.9% (Q1), 50.4% (Q2), 49.1% (Q3), 49.0% (Q4).

**Why margin fell in 2025.** The report gives "the increase in fixed expenses (primarily depreciation and deductions and wages) as a share of revenues due to lower sales, partially offset by … reduc[ing] the prices of raw materials". Revenue fell by about NIS 37.5M, which the report blames on the US slowdown after the tariff plan and on a strong NIS.

**Correction to the earlier note.** "PLC's and expansion units" are **95% (2025), 96% (2024), 93% (2023)** of revenue. The report says the group's products have "basically similar" gross-profit rates. So about 50% is effectively a PLC+HMI product margin, not one blended with automated parking. The parking or "systems" mix is negligible now.

**Manufacturing model (relevant to NOVAKON) [P].** "The Company manufactures its products primarily through subcontractors. Subcontractors are responsible for placing the electronic components on printed circuits, injection of cases and plastic parts and sub-assemblies such as keyboards." Unitronics itself does part of the assembly, final assembly, electrical testing, calibration and packaging in Airport City (§1.9.16). Its software (VisiLogic/UniLogic) is free.
- **Unitronics is an outsourced-production PLC+HMI brand earning about 50% GM.** This is a useful upper benchmark for NOVAKON.

**Parent-only statements.** Revenue 131,664 / 163,842 / 191,435 and gross profit 54,427 / 72,359 / 82,330 give about 41% / 44% / 43%. These are lower because of transfer pricing to subsidiaries. Do not use them as the group margin.

---

## Item 5. Ependion 2025 annual report: Beijer Electronics gross margin

**Result: confirmed not disclosed (primary).**

Source: Ependion AB Annual & Sustainability Report 2025 [P]: https://attachment.news.eu.nasdaq.com/ad29f4e42e511690546637a5d98dde474

**Segment note.** Segments are Beijer Electronics and Westermo, and "Management evaluates operating segments based on a metric termed EBIT". The segment table gives sales, EBITDA and EBIT, but no segment gross profit.
- Beijer Electronics 2025: sales 858 MSEK (946), EBITDA 165 (183), EBIT 79 (106), EBIT margin 9.2% (11.2).

**Group.** Gross profit 1,193,026 kSEK on sales 2,231,753 kSEK, giving **group GM 53.5%** (five-year summary, as printed) [P]. FY2024 was about 50.8% [I], per the earlier note.

**Qualitative statements only.**
- "gross margins in both business entities increased".
- Beijer phased out "low-profitability and low-volume products to improve our gross margins".
- Goodwill test note: for Beijer Electronics, the forecast gross margin "is approx. 5% higher … than the five-year historical average … only 1% higher than the actual gross margin in 2025". The levels are not given.

**Other facts.**
- 2025 sales mix for Beijer Electronics: Manufacturing 35%, Marine 26%, Energy 25%, Infrastructure & other 14%.
- Capitalised development: Beijer Electronics 256,611 kSEK (2025).

**Q2-2026 interim** (https://attachment.news.eu.nasdaq.com/ac043488fe1c945a1e9c800b27716c0c1): group GM 50.4% is shown. No Beijer gross margin was found there either [P]; this was a text search only.

---

## Item 6. MCGS 昆仑通态, Samkoon 显控, Weintek 威綸

**Result: MCGS and Samkoon: no IPO, 辅导 or revenue filing found (still unverified / not public). Weintek: registry data is new (primary); revenue is not public.**

### MCGS (北京昆仑通态自动化软件科技有限公司)
- Several extended searches turned up no CSRC 辅导备案, no prospectus and no NEEQ listing. Results kept returning 昆仑芯 (Baidu AI chips; STAR 辅导 filed 2026-04-29), 昆仑万维 and 昆仑联通. [by absence; search engine only, the CSRC 辅导公示 database was not queried directly]
- One search summary claimed MCGS is listed as "688390". **This is false**: 688390 is not MCGS (it is 固德威 GoodWe). Do not use it.
- Market-share figures (all [S], with unnamed or unclear underlying sources):

| Year | Source | Siemens | Weintek | MCGS | Other |
|---|---|---|---|---|---|
| 2022 | 自动化新闻网 ca168, Feb 2024 (search snippet; page returned 418) | ~20%, or 26.6% in another statistic | ~15%, "销售额约8亿" | 11.7% | — |
| 2023 | 观研报告网 | 27.2% | 15.4% | 11.6% | — |
| 2024 | Sohu self-media "2024年中国HMI市场研究简报", 2025-05-21; total market 92.8亿元 (touch HMI 90.8亿元); the market definition includes display modules | 14% | 10.8% | 9.7%, sales growth 13% | 迪文 11.3%, 汇川 5.6% |

  - ca168: https://news.ca168.com/202402/130617.html
  - 观研报告网: https://www.chinabaogao.com/detail/755561.html
  - Sohu brief: https://www.sohu.com/a/897318670_121948943
- Implied MCGS sales [I, weak]: about RMB 6–9亿 a year, using 11.7% of a market in which 15% is about 8亿, or 9.7% × 92.8亿. This is consistent with the earlier 2019 triangulation (about RMB 370M) plus growth. **Not citable as a figure.**

### Samkoon 显控 (深圳市显控科技股份有限公司)
- No 辅导, IPO or NEEQ record found [by absence]. The only 辅导 hit was a different company, 广东视安通智慧显控 (CITIC 辅导 report, 2024-01): https://www.citics.com/newsite/tzgg/ipoqyfdgg/202401/P020240116448015094104.pdf
- Company site (https://www.samkoon.com.cn/about) [V]:
  - Founded March 2007; "300+" employees, "110+" R&D engineers.
  - R&D "在10%以上" of revenue.
  - Over 60,000 m² production/R&D base in the 深汕特别合作区; CE/FCC/UL certified.
  - 国家级专精特新"小巨人"; 14,000+ cumulative customers; 30+ countries.
  - **No revenue figure.**
- A 深圳发改委 page reports a new AI factory in 深汕 with "年产值有望达20亿元" once at full capacity. This is a target, not revenue [S, snippet only; the page returned 404 on fetch].

### Weintek (威綸科技股份有限公司)
- GCIS company record [P]:
  - 統編 **89624465**, set up 1995-11-10.
  - Authorised capital NT$100,802,500; **paid-in capital NT$10,080,250**.
  - Registered at 新北市中和區橋和路13號14樓; last change 2026-07-01.
  - Chair represents an investment holding company (翔輊投資).
  - API: https://data.gcis.nat.gov.tw/od/data/api/5F64D864-61CB-4D0D-8AD9-492047CC1EA6?$format=json&$filter=Business_Accounting_NO%20eq%2089624465&$skip=0&$top=50
- Related entity [P]: **香港商威綸物聯網有限公司台灣分公司**, 統編 42961394, registered 2018-05-21, operating capital NT$90M. The responsible person is the same as on the Weintek factory registration.
- Factory registry [P] (same 11510.csv as Item 2):
  - 威綸科技股份有限公司, factory no. 65009878, 新北市中和區中原里橋和路17號14樓, registered 2021-10-19, products 271/272.
  - 香港商威綸物聯網台灣分公司, factory no. 65010785, 新北市土城區中央路四段51號4樓, registered 2022-11-16, products 271/272.
  - Weintek's production is therefore partly held under a Hong Kong parent's Taiwan branch [I].
- Revenue: **not public**. Data brokers give about US$32.5–32.8M (ZoomInfo, Datanyze, RocketReach) with inconsistent headcounts (25 to 1,000). Baidu Baike gives an old figure of RMB 76.55M for the China entity. All are [S, low reliability] and probably understate a company with a 10–15% share of China's HMI market. The ca168 snippet's "销售额约8亿" (2022, China) is the best secondary indicator [S/U].
  - https://www.zoominfo.com/c/weintek/346299814
  - https://baike.baidu.com/en/item/Weinview/3378724

---

## Item 7. iFace Designer x86 runtime licence price

**Result: still unverified / not public.**

- The NOVAKON NPP-156P02 page lists only "Software Option: iFace-Designer® Runtime, Siemens Industrial Edge Runtime, Siemens Industrial Management". It has no part number or price; the only ordering text is "Request a quote". [V] https://www.novakon.com.tw/en/hardware/detail/Industrial_Panel_PC/NPP_156P02_Industrial_Panel_PC
- EP Solutions (Serbia) lists "iFace Designer" at 0,00 and offers a download of "iFace 2.0.01 runtime for Industrial Panel PC (X86 PC-based ; Win10 64bit Operating System)". No separate runtime licence item or price is shown. [S] https://www.ep-solutions.rs/en/novakon-iface-designer-software
  - The x86 runtime installer is freely downloadable. Whether it needs a paid licence key to run is not stated.
- A search snippet for PLC-City "X15P-i" (Novakon panel PC) says an "iFace Designer® HMI Software Runtime License" is **included** with the unit. The page is behind a Cloudflare bot check, so it was not read and the check was not bypassed. [S/U] https://www.plc-city.com/shop/en/novakon-panel-pc/x15p-i-nfs.html
- device.report and the manuals.plus iFACE SCADA guide returned 403. No distributor price list with a runtime part number was found.
- **Best current reading [I]:** on NOVAKON-branded x86 panel PCs the runtime licence appears to be bundled with the hardware rather than sold separately. No evidence exists of a standalone priced runtime SKU.

---

## Item 8. Who manufactures AutomationDirect C-more EA9 HMIs

**Result: new (primary). Made by Koyo Electronics (now JTEKT Electronics); country of origin China.**

- AutomationDirect's EA9 overview brochure, headed **"Made by Koyo"** [P, vendor document]: "Koyo Electronics, our parent company, is part of the multibillion dollar JTEKT group … C-more was designed with quality in mind and engineered by Koyo". https://cdn.automationdirect.com/static/catalog/images/product-pdf/ea9overview.pdf
  - Koyo is described as AutomationDirect's parent. So C-more is a captive, sister-made product, not an outsourced third-party ODM. **Correct the earlier note** ("C-more HMIs are sourced … distributor-brand").
- AutomationDirect's EA9-RHMI page states "Country Of Origin: CHINA", "HS Code: 8471.30", price US$632.00 [P]. https://www.automationdirect.com/adc/shopping/catalog/hmi_(human_machine_interface)/graphical_hmi_devices/headless_hmi_devices/ea9-rhmi
  - Search snippets show the same China origin for EA9-T10WCL, T15CL and T10CL, and for the Lamonde reseller (HS 8537.10) [S].
  - The EA9-T10CL page shows UL Listed File E157382 and is "retired as of 01/2026", replaced by CM5-T12W [P].
- The EA9 user manual (EA9-USER-M, 1st Ed. Rev. V, 02/24) shows the accessory label "KOYO ELECTRONICS INDUSTRIES CO., LTD." on the EA-COMCON-3 adapter drawing (Appendix) [P]. https://cdn.automationdirect.com/static/manuals/ea9userm/ea9userm.pdf
  - No FCC SDoC responsible party or "Made in" statement for the panels was found in the manual text.
- JTEKT Electronics (Wuxi) develops, produces and sells PLCs, HMIs and encoders [S]:
  - It absorbed Koyo Electronics (Wuxi) on 2024-07-01, and merged with Wuxi Yan Guang Automotive Parts on 2026-07-01 [P, JTEKT Electronics notices].
  - https://www.electronics.jtekt.co.jp/en/topics/202607027745/ ; https://www.ethercat.org/de/members/members_F74D118498E145BAB55259B944EE56E0.htm
  - **Inference [I/U]:** EA9 panels are most likely built at JTEKT Electronics (Wuxi), the former Koyo Electronics (Wuxi). Not directly documented for the EA9 SKU.
- Import records and the FCC SDoC party were not checked (no public SDoC database for Part 15B unintentional radiators; Panjiva not used).

---

## Changes this implies for the earlier notes (summary)

1. NOVAKON manufacturing: replace "most likely built by parent iBASE" with the following. iBASE supplies a minority and falling share of goods: 37% (2022), 28% (2023), 9% (2024), an unknown amount below the disclosure cutoff in 2025, and NT$22.0M in 9M-2026. NOVAKON has no factory of its own. The remaining supply chain is undisclosed.
2. NOVAKON 2026: 9M net income NT$65.1M, already above FY2025.
3. Unitronics: GM about 50% is confirmed from its own filing, and it is a clean PLC+HMI margin (95% of revenue). It is an outsourced-production model (subcontractors).
4. Ependion: no Beijer Electronics GM is disclosed. Keep the group GM of 53.5% only.
5. AutomationDirect C-more: captive Koyo/JTEKT product, origin China. It is not a third-party ODM model.
6. Kinco 控制系统 GM 40.95% in H1-2026 (stable). Xinje HMI revenue RMB 1.00亿 in H1-2026. Neither report gives units.
7. Weintek: paid-in capital is only NT$10.08M, a small share base for a large business. Revenue is still not public.
