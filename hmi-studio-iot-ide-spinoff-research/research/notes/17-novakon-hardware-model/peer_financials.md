# Peer financials: industrial HMI hardware with free or bundled design software

Compiled 2026-11-20. Purpose: build a gross-margin (毛利率) and average-selling-price (平均單價 / 销售单价) benchmark to estimate NOVAKON's hardware price and gross margin. NOVAKON gives its software away and outsources its hardware. The second question is whether that model suits a small software spin-off.

Labels used below:
- **[P]** means a primary filing: an annual report (年度报告) or IPO prospectus (招股说明书) read directly. The text was extracted from the PDF.
- **[S]** means a secondary source: a data aggregator, a news item, or a third-party page.
- **[I]** means inferred: computed by me from [P] or [S] figures. The method is stated each time.

FX used only for orientation: RMB 7.1 ≈ USD 1, NT$ 32 ≈ USD 1. FX moved a lot over the period, so treat USD conversions as rough.

---

## 1. Headline benchmark table

| Company | Year | HMI (or nearest segment) revenue | Units sold | ASP (ex-VAT, to channel) | Gross margin | Manufacturing | Source | Label |
|---|---|---|---|---|---|---|---|---|
| 步科 Kinco (688160): 人机界面 | 2017 | RMB 138.14M | 238,173 | RMB 580.01 | 36.68% | In-house assembly and test. PCBA outsourced (外协) | [Kinco IPO prospectus 2020](https://pdf.dfcfw.com/pdf/H2_AN202011051426899033_1.pdf) pp.1-1-243/263 | P |
| Kinco: 人机界面 | 2018 | RMB 135.68M | 243,671 | RMB 556.81 | 35.02% | same | same | P |
| Kinco: 人机界面 | 2019 | RMB 138.90M | 245,873 | RMB 564.91 (~USD 80) | 38.17% | same | same | P |
| Kinco: 人机界面 | 2020 H1 | RMB 73.72M | 136,534 | RMB 539.91 | 40.20% | same | same | P |
| Kinco: 控制系统 (HMI + PLC + IoT; HMI is ~82–86% of units) | 2023 | RMB 187.79M [I] | ~375,200 [I] (HMI ~323,500 [I]) | ~RMB 500 [I] | 41.94% [I] | same | Derived from YoY changes in the 2024 AR | I |
| Kinco: 控制系统 | 2024 | RMB 187.83M | 402,851 (HMI 344,410) | RMB 466 [I] | 42.24% | same | [Kinco 2024 AR](https://pdf.dfcfw.com/pdf/H2_AN202504241661694668_1.pdf) | P |
| Kinco: 控制系统 | 2025 | RMB 203.98M | 424,173 (HMI 347,792) | RMB 481 [I] (~USD 68) | 41.33% | same (reconfirmed in the 2025 AR) | [Kinco 2025 AR](https://pdf.dfcfw.com/pdf/H2_AN202604021820984739_1.pdf) pp.18, 45–46 | P |
| 信捷 Xinje (603416): 人机界面 | 2017 / 2018 / 2019 | n/a here | n/a | n/a | 31.72% / 29.72% / 32.40% | In-house production. Some PCBA goes to 外协 in peak season | Cited in the Kinco IPO prospectus (comparable-company table) | P (via Kinco) |
| Xinje: 人机界面 | 2023 | RMB 178.25M [I] | ~630,500 [I] | ~RMB 283 [I] | 30.46% [I] | same | Derived from YoY changes in the 2024 AR | I |
| Xinje: 人机界面 | 2024 | RMB 195.18M | 627,535 | RMB 311 [I] | 32.18% | same | [Xinje 2024 AR](https://pdf.dfcfw.com/pdf/H2_AN202504251662306008_1.pdf) | P |
| Xinje: 人机界面 | 2025 | RMB 205.36M | 655,563 | RMB 313 [I] (~USD 44) | 32.78% | same | [Xinje 2025 AR](https://pdf.dfcfw.com/pdf/H2_AN202604161821274535_1.pdf) pp.19, 41–42 | P |
| 汇川 Inovance: 控制技术类 (HMI + PLC) | 2017 / 2018 / 2019 / 2020H1 | not split | n/a | n/a | 51.15% / 47.72% / 45.08% / 53.99% | In-house | Cited in the Kinco IPO prospectus | P (via Kinco) |
| 昆仑通态 MCGS (private) | 2019 | ~RMB 370M China HMI sales [I] | n/a | n/a | n/a | n/a | MIR 12.3% share (in the Kinco prospectus) × implied market | I |
| 威綸 Weintek (private) | 2019 | ~RMB 430M China HMI sales [I] | n/a | n/a | n/a | In-house. Factories in Taiwan and China | MIR 14.2% share (in the Kinco prospectus); [ipc2u](https://ipc2u.com/about-us/vendors/weintek/) | I / S |
| 深圳显控 Samkoon (private) | n/a | No filing found | n/a | n/a | n/a | n/a | Searches turned up only similarly named companies (显盈科技) | — |
| Ependion (ex-Beijer Group, EPEN): whole group | 2024 / 2025 | Beijer Electronics entity: 858 MSEK in 2025 | n/a | n/a | Group 50.8% / 53.5% [I]. **No segment gross margin.** Beijer Electronics EBIT margin 9.2% (2025) | n/a in report | [Ependion Q4/FY2025 report](https://storage.mfn.se/34508b36-12ac-48ab-88c2-6d59bd59f427/report-in-pdf-format.pdf) | P (group GM computed) |
| Red Lion Controls (now HMS) | LTM Sep-2023 | SEK 1,397M (all products, not only HMI) | n/a | n/a | ~55%; adj. EBIT ~21% | In-house: York, PA and Dinkelsbühl, Germany | [HMS press release 2023-12-11](https://www.hms-networks.com/news/news-details/10-12-2023-hms-networks-acquires-red-lion-controls-and-significantly-expands-its-presence-in-north-america) | P (press release) |
| Unitronics (TASE: UNIT): whole company | 2023 / 2024 / 2025 | ILS 211.7M / 192.2M / 154.8M | n/a | n/a | 49.9% / 51.1% / 49.6% | Reports "design, development, production" of products. Also has an automated-parking segment that blends the margin | [stockanalysis](https://stockanalysis.com/quote/tlv/UNIT/financials/income-statement/); craft.co shows ~65%, which conflicts | S |
| IDEC (6652): whole company | FY Mar-2025 | ¥67.4bn (all products) | n/a | n/a | ~43.7% | In-house | [logmi](https://finance.logmi.jp/articles/381567) / crex-data | S |
| Maple Systems (US, private) | 2026 list price | n/a | n/a | 7" HMI5070Bv4 list price **USD 450**. Basic series "starts at USD 345" | n/a | **Outsourced.** Rebrands Weintek; ships Weintek's EBPro software free | [maplesystems.com](https://maplesystems.com/product/modelname/hmi5070b/), [FAQ](https://maplesystems.com/faq/what-is-the-upfront-cost-of-buying-an-hmi/) | P (vendor page) |
| AutomationDirect (US, private) | 2025 | ~USD 181M total revenue (data-broker estimate) | n/a | n/a | n/a | Distributor/brand. C-more HMIs are sourced | [RocketReach](https://www.rocketreach.co/answers/what-is-the-annual-revenue-of-automationdirect_b5c72101f42e0d04), ZoomInfo | S (weak) |
| Pro-face (Schneider), Delta DOP (中达电通), Inovance HMI line | — | Not disclosed separately | — | — | — | — | Delta reports only an Automation segment. Delta Thailand: Automation segment profit fell from 218M to 102M THB (TTM Sep-2025) | S |

### What the China filings show (the only places that disclose units, ASP and HMI margin)

**ASPs are low.** Kinco sold HMIs at RMB 540–580 each (ex-VAT, mostly to distributors) in 2017–2020. That is about USD 75–85. Its 2024–25 control-system ASP (HMI-dominated) is RMB 466–481, about USD 65–68. Xinje's HMI ASP is RMB 283–313, about USD 40–44. Xinje's mix includes cheap text displays (OP series) and 4.3"/7" resistive panels. These are factory-to-channel prices in a price-war market. End-user street prices are higher.

**Gross margins sit at 30–42%.**
- Xinje: 30–33%.
- Kinco: 35–42%. The higher end needed help from the 2019 VAT cut (16%→13%) and cheaper LCD and touch panels. The premium series runs 41–44%. The low-end "ET / 4030" series ran 23–37% (prospectus p.1-1-265).
- Inovance's 控制技术类 (HMI + PLC, where PLC margins are much higher) runs 45–54%. Xinje's PLC line earns 57% against 33% for its HMI. **HMI is the low-margin product within a vendor's control portfolio.**

**Cost structure.** Direct material is about 87% of HMI COGS:
- Xinje 2025: 87.45% material, 3.45% labour, 8.86% overhead.
- Kinco 2025 control systems: material RMB 104.49M of RMB 119.67M COGS (87.3%).
- Kinco prospectus: direct material is "接近90%" of COGS.
- Per unit (2025): Xinje COGS ≈ RMB 211, of which material ≈ RMB 184. Kinco control-system COGS ≈ RMB 282, of which material ≈ RMB 246 [I].
- Kinco's average purchase price for an LCD panel fell from RMB 97.3 to RMB 77.8 over 2017 to 2020H1. Touch panels fell from RMB 31.3 to RMB 24.7 (prospectus, raw-material purchase-price table).
- **So assembly and test add little value. The margin comes from brand, channel and software.**

**Manufacturing model.** Both listed Chinese vendors do final assembly, firmware flashing, burn-in and test in-house. They outsource PCBA. Kinco: "对于非关键的PCBA加工环节，公司委托专业厂商按照公司要求加工，所需原材料的采购由公司完成" (2025 AR p.18). Xinje uses 委托外协 for PCBA in peak season. Neither is a fully outsourced (ODM) model. Weintek also makes its own panels, in Taiwan and China. **Of the peers checked, only Maple Systems and AutomationDirect-style brands run the fully outsourced model that NOVAKON uses.** They are distributor-brands, not software companies.

**Market shares (MIR, quoted in the Kinco prospectus, 2019 China HMI by value):** Siemens 25.6%, Weintek 14.2%, MCGS 12.3%, Kinco 4.6%. Kinco's HMI revenue was RMB 138.9M that year. That implies a market of about RMB 3.0bn and Weintek China sales of about RMB 430M and MCGS about RMB 370M [I]. This is a rough triangulation that relies on MIR's market definition matching Kinco's revenue basis. Later reports:
- Kinco's 2024 AR: Kinco held 3% of 2023 value and ranked #10. Its 2025 AR: #7 in 2024 (MIR).
- A secondary article quotes Siemens 26.6%, Weintek 15.4% and MCGS 11.7%, with no year given.
- An old Weintek document quotes IMS Research 2010: Weintek held 25% of China HMI.

---

## 2. Weintek Labs (威綸科技), Taiwan, private

- No revenue, profit, margin or unit figures turned up in Taiwan news, credit-report snippets or Chinese filings. Searches for "威綸 營收" return 威剛 (ADATA), 威強電 (IEI) and Wintek instead.
- Facts found:
  - Founded in 1995 and headquartered in New Taipei. Production facilities in Taiwan and China ([ipc2u](https://ipc2u.com/about-us/vendors/weintek/), [EtherCAT member page](https://ethercat.org/en/members/members_7792CA5CD5DD46D286E5139736E0D07F.htm)).
  - Entered China in 2001. The China subsidiary 深圳市威纶通科技有限公司 was set up in 2004 (Baidu Baike, [S]).
- Market-share triangulation [I]: about RMB 430M of China HMI sales in 2019. China was historically Weintek's largest market. Add Europe, the Americas (including Maple Systems OEM volume) and SE Asia, and NT$3–5bn total revenue looks plausible. **This is speculation and must not be cited as a figure.**
- **Software model:** EasyBuilder Pro (EBPro) is free. Maple confirms "100% free HMI configuration software … No license or key required" for its Weintek-built panels.
- **Price reference:** a Weintek-built 7" panel lists at USD 450 at Maple Systems in the US. The basic series starts at USD 345. Weintek cMT3072XH2v2 lists at USD 679 at DigiKey; the headless cMT-SVR-202 at USD 478–589. An online parts dealer lists TK6071iP at USD 360.

---

## 3. Western and other peers

- **Ependion (formerly Beijer Electronics Group), Nasdaq Stockholm, EPEN.**
  - The group reports gross profit, but the two business entities (Westermo and Beijer Electronics) report only sales and EBIT.
  - FY2025: net sales 2,231.8 MSEK, gross profit 1,193.0 MSEK, so group GM 53.5% [I]. FY2024: 50.8% [I]. The report says gross margins rose in both entities.
  - Beijer Electronics (HMI): 858 MSEK sales, EBIT 79 MSEK (9.2%). The new X3 panels launched in autumn 2025.
  - Westermo (industrial networking) runs a 14.8% EBIT margin, so HMI is the weaker business inside the group.
  - Beijer's iX Developer is licensed, not free. Manufacturing model not stated in the report.
- **Red Lion (sold by Spectris to HMS in April 2024 for EV USD 345M).** Sales were SEK 1.4bn in the LTM to Sep-2023, GM about 55%, adj. EBIT about 21%. It has own plants in York, PA and Dinkelsbühl, Germany. Its product mix includes HMIs, Crimson software (free), networking and panel meters.
- **Unitronics.** Group GM is about 50% (secondary source; craft.co shows about 65%, which conflicts). The company combines PLC+HMI "products" with automated parking and logistics "systems", so this is not a clean HMI margin. UniLogic and VisiLogic are free. Revenue fell 19% in 2025.
- **IDEC.** Group GM about 44% (FY3/2025), with sales of ¥67.4bn and an operating margin of 5.4%. HMI is a minor line and is not split out.
- **Pro-face (Schneider Electric Japan).** Not disclosed. BLUE / GP-Pro EX software is licensed separately.
- **Maple Systems.** Private, no financials found. It is an outsourced, Weintek-built brand with free software and published list prices (see section 2).
- **AutomationDirect.** Private. Revenue estimates run around USD 181M (data brokers, low reliability). It is a catalog brand: C-more HMIs are sourced, and C-more programming software is a free download.

---

## 4. Taiwan-listed peers (industrial PC, panel PC, embedded display). Whole-company GM, not HMI-specific

Source: stockanalysis.com income statements [S]; TTM ends Sep-2026. Revenue in NT$ millions.

| Company (ticker) | FY2023 GM | FY2024 GM | FY2025 GM | TTM GM | FY2025 revenue | Note |
|---|---|---|---|---|---|---|
| 研華 Advantech (2395) | 40.5% | 40.8% | 39.8% | 38.0% | 70,882 | Brand. Own plants. Op margin 16% |
| 威強電 IEI (3022) | 37.0% | 35.3% | 33.5% | 30.3% | 6,513 | Panel PCs and HMI-like products |
| 振樺電 Posiflex (8114) | 38.1% | 45.0% | 48.4% | 40.8% | 18,149 | POS and kiosk |
| 飛捷 Flytech (6206) | 38.7% | 43.6% | 44.1% | 45.3% | 5,033 | POS and kiosk ODM-ish, but high margin |
| 安勤 Avalue (3479) | 26.5% | 26.0% | 25.8% | 25.0% | 8,481 | Embedded and panel PC, partly ODM |
| 廣積 iBASE (8050) | 24.6% | 23.8% | 21.3% | 20.5% | 5,358 | Industrial boards and panel PCs. One third-party page (OKmarts) claims NOVAKON is an iBASE subsidiary. **Unverified;** a third-party list of iBASE investees did not include NOVAKON. Check MOPS |
| 樺漢 Ennoconn (6414) | 19.4% | 19.7% | 20.5% | 19.9% | 142,290 | Mostly ODM and integration |
| 大眾控 FIC Global (3701) | 15.5% | 13.7% | 17.5% | 12.1% | 9,833 | ODM/EMS-like |
| 盟立 Mirle (2464) | 17.3% | 18.4% | 18.6% | 20.6% | 6,681 | Automation systems integration |
| **全台晶像 EDT (3038)** | 23.0% | 22.9% | 23.0% | 21.5% | 3,011 | See below |
| 台達電 Delta (2308): Automation segment | — | — | — | — | — | Segment GM not found; Automation was weak in 2025 |

**EDT context.** EDT is Emerging Display Technologies Corp., 全台晶像股份有限公司, TWSE 3038, listed since 2002-08-26 and headquartered in Kaohsiung. It makes TFT LCD and capacitive touch modules plus "smart embedded solutions". It is an independent listed company; no corporate parent was found. The largest holder is chairman 曾瑞銘 with about 11.0M shares. Non-independent directors and supervisors together hold about 18% (Goodinfo, 2026-10).
- Revenue is shrinking: NT$4,387M (2023), NT$3,606M (2024), NT$3,011M (2025), NT$2,861M TTM.
- GM is flat at about 23%. Operating margin fell from 10.3% to 5.2% to 3.0% (TTM).
- Net income was NT$163M in 2025, held up by non-operating items. Sources: [stockanalysis 3038](https://stockanalysis.com/quote/tpe/3038/financials/), [company page](https://stockanalysis.com/quote/tpe/3038/company), [registry via technews](https://info.technews.tw/company/89261652-全台晶像股份有限公司).
- A display-module maker at about 23% GM sits in the "component supplier" band. Its margin is roughly 10–20 pp below HMI brands.

---

## 5. Contract-manufacturing (EMS/ODM) benchmark: what the ODM keeps

Source: stockanalysis [S].

| EMS/ODM | Focus | Gross margin (last 3 FY) | Op margin |
|---|---|---|---|
| Plexus (PLXS) | Industrial, healthcare, aerospace EMS | 9.6% / 10.1% / 10.1% (FY24–26) | ~5% |
| Benchmark (BHE) | Industrial and medical EMS | 9.6% / 10.2% / 10.2% (FY23–25) | ~4% |
| Kimball Electronics (KE) | Industrial, auto and medical EMS | 8.2% / 7.0% / 8.2% (FY24–26) | ~4% |
| 易德龙 Suzhou Etron (603380) | Chinese industrial-control EMS (small, high-mix) | 19.6% / 23.3% / 24.9% (FY23–25) | 6–11% |
| 大眾控 FIC, 樺漢 Ennoconn, 廣積 iBASE, 安勤 Avalue | Taiwan industrial ODM / IPC | 14–26% | 0–9% |

The pattern: pure EMS keeps about 7–10% GM. Small high-mix industrial EMS and Taiwan industrial ODMs, which also do design work, keep about 15–25%.

---

## 6. What this means for NOVAKON (all inferred, [I])

### Price chain for a 7" panel

Anchor points:
- Chinese domestic brands: factory-to-channel ASP of RMB 310–580 (USD 44–82), GM 30–42%.
- Western list price for a Weintek-built 7" panel: USD 345–450. Distributor discounts of 25–40% are common, which puts the brand's net price at roughly USD 210–340.

### NOVAKON hardware price estimate

- NOVAKON is a Taiwan brand selling mainly through export distributors and OEMs. A plausible blended ASP is **USD 150–300 per panel** (net to NOVAKON):
  - Low end: about USD 150 for 4.3"/7" basic panels into price-sensitive channels.
  - High end: about USD 300+ for 10"/cMT-class or panel PCs.
- That range sits above Chinese domestic ASPs (USD 44–82) and below Western list prices. It needs checking against NOVAKON distributor price lists or customs data (Panjiva lists Novakon shipments).

### NOVAKON gross margin estimate

**Starting cost.** An in-house HMI maker's COGS is 58–70% of its ASP, and about 87% of that COGS is material.

**Cost if outsourced.** NOVAKON pays an ODM that keeps 10–25% GM (see section 5). Its unit cost is therefore about 1.1–1.3× an in-house maker's COGS, assuming the same BOM.

**Implied brand GM.**
- Starting point: a Kinco-like 38–42% margin made in-house.
- Fully outsourced, the same prices would give about **25–35% GM**, and **20–30%** if the ODM also does the design (an ODM premium).
- Higher export prices than China could push this back up toward **35–45%**. That is roughly the Taiwan panel-PC band (IEI 33–37%, Advantech 40%).

**Working estimate:** NOVAKON hardware GM **~25–40%**, with ~30% as the central case. Free software adds no direct revenue; it is a cost carried by the hardware margin.

### Fit for a small software spin-off

**The economics are hardware economics.** HMI is the lowest-margin product line even at integrated Chinese vendors: Xinje 33% versus 57% on its PLCs. An outsourced, free-software HMI brand probably earns 25–35% GM. After R&D (Kinco spends about 10–11.5% of revenue), selling costs (about 7–9%) and G&A, EBIT margins are thin. Beijer Electronics, a sizeable HMI-focused brand, earns only a 9.2% EBIT margin. Red Lion reached 55% GM and 21% EBIT only with its own plants, a premium US brand and networking products in the mix.

**The free-software model works for companies with volume:**
- Kinco ships about 350k units a year, Xinje about 650k, and Weintek is probably larger still.
- At NOVAKON-like volumes, perhaps 10^4 units a year (unknown), a free IDE has to be funded from about USD 50–100 of gross profit per panel.

**Implication for the spin-off:** a software spin-off that inherits free software and no hardware margin has no revenue line. It would need one of:
1. a license or royalty per runtime/panel paid by hardware OEMs (an ODM licensing model);
2. paid cloud/IoT services;
3. to remain tied to a hardware brand.

The evidence does not support a standalone software company built on "free software, outsourced hardware". That model only works at the brand level, where hardware margin pays for software.

---

## 7. Gaps and follow-ups

- **Not found:** filings for MCGS (昆仑通态) or Samkoon (深圳显控). Both appear unlisted, and searches kept returning similarly named companies (昆仑联通, 显盈科技). CSRC 辅导备案 lists and 企查查/天眼查 could confirm.
- **Weintek:** no financials anywhere public. Possible routes are a 公司登記 capital check and the annual reports of Taiwan component suppliers listing Weintek as a top customer.
- **Not extracted:** the 2026 H1 reports exist for Kinco (AN202608251828413394) and Xinje (AN202608261828480404). They could add H1-2026 segment data.
- **Unitronics:** the GM conflict (≈50% vs ≈65%) needs the company's own TASE/MAYA filing.
- **Ependion:** the annual report may give a Beijer Electronics gross margin. The Q4 report does not.
- **NOVAKON:** ownership (the iBASE claim), its ODM partner (EDT? some other Taiwan ODM?) and its actual distributor prices all need direct verification.
- **Software pricing:** Kinco (DTools/Kinco HMIware), Xinje (TouchWin), MCGS (MCGS嵌入版/McgsPro) and Samkoon (SKTOOL) are generally distributed free with their hardware. This was not re-verified on official sites in this pass; MCGS was seen only on third-party download pages. Weintek EBPro (via Maple) was verified free.

## Sources (primary filings, read in full text)
- Kinco IPO prospectus (2020-11): https://pdf.dfcfw.com/pdf/H2_AN202011051426899033_1.pdf
- Kinco 2024 AR: https://pdf.dfcfw.com/pdf/H2_AN202504241661694668_1.pdf
- Kinco 2025 AR: https://pdf.dfcfw.com/pdf/H2_AN202604021820984739_1.pdf
- Xinje 2024 AR: https://pdf.dfcfw.com/pdf/H2_AN202504251662306008_1.pdf
- Xinje 2025 AR: https://pdf.dfcfw.com/pdf/H2_AN202604161821274535_1.pdf
- Ependion Q4/FY2025: https://storage.mfn.se/34508b36-12ac-48ab-88c2-6d59bd59f427/report-in-pdf-format.pdf
- HMS / Red Lion: https://www.hms-networks.com/news/news-details/10-12-2023-hms-networks-acquires-red-lion-controls-and-significantly-expands-its-presence-in-north-america
- Maple Systems: https://maplesystems.com/product/modelname/hmi5070b/ and https://maplesystems.com/faq/what-is-the-upfront-cost-of-buying-an-hmi/

## Sources (secondary)
- stockanalysis.com financials: TPE 2395, 3022, 6414, 8114, 6206, 2464, 3701, 3038; TPEX 3479, 8050; TLV UNIT; PLXS, BHE, KE; SHA 603380
- IDEC: https://finance.logmi.jp/articles/381567
- AutomationDirect revenue estimate: https://www.rocketreach.co/answers/what-is-the-annual-revenue-of-automationdirect_b5c72101f42e0d04
- Weintek profile: https://ipc2u.com/about-us/vendors/weintek/ ; NOVAKON/iBASE claim: https://okmarts.com/product/hmi/novakon.html
- EDT holders: https://poorstock.com/holders/3038 ; https://goodinfo.tw/tw/StockDetail.asp?STOCK_ID=3038
