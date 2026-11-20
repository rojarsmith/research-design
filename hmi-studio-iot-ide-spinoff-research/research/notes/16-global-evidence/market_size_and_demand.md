# Market Size and Demand Signals for a Low-Code IoT IDE for MCU-Based Connected Devices with Displays

Research date: 2026-11-20. Reliability tags used below:
- **[Tier 1]**: primary analyst or institution with a stated method (IoT Analytics, Gartner, IDC, Omdia, Yole, IEA, Ericsson, TechInsights, company filings).
- **[Tier 3 / low reliability]**: syndicated "market research" vendors (Fortune Business Insights, MarketsandMarkets, Marketintelo, Wise Guy, TechSci, Grand View "Horizon" tables, Straits, IMARC, and similar). Their figures disagree widely, definitions are opaque, and several were found on pages with unrelated URL slugs. Use them only to show a range, never as a single anchor number.

Most sources were read through search-engine summaries. Only two pages were fetched in full (IoT Analytics MCU article and IoT Analytics embedded-systems article). Two fetches were blocked (IEA GEVO 2026 charging chapter and Omdia's January 2026 industrial-display press release, both HTTP 403), so the figures from those pages come from search snippets.

## 1. Connected IoT devices: installed base, growth, and the MCU share

### Takeaway
IoT Analytics puts the installed base at about 21.1 billion connected IoT devices at end-2025, growing about 13–14% a year to about 39 billion by 2030. Ericsson counts about 4.5 billion cellular IoT connections, a subset of that base. No source found gives an MCU-based share of devices. The best proxy is spend: IoT uses about 22% of MCU revenue.

### Cited Findings
- **Installed base end-2025:** 21.1 billion connected IoT devices, +13% YoY. About 45% are enterprise connections. The forecast is about 39 billion in 2030 (13.2% CAGR from 2025) and more than 50 billion by 2035. [Tier 1] — [IoT Analytics, State of Enterprise IoT 2026 / number of connected IoT devices](https://iot-analytics.com/number-connected-iot-devices/); [IoT Business News summary of State of IoT 2025](https://iotbusinessnews.com/?p=44499)
- **Earlier growth figure:** IoT Analytics' H1-2025 update had put 2025 growth at 14% (to the same 21.1 billion). The September 2026 embedded-systems article repeats 21.1 billion installed and "surpass 50 billion by 2034". There is a small internal inconsistency on the horizon year (2034 vs 2035). [Tier 1] — [IoT Business News](https://iotbusinessnews.com/?p=44499); [IoT Analytics, embedded systems market article, 3 Sep 2026](https://iot-analytics.com/embedded-systems-market-opportunities-and-challenges-for-developers/)
- **2024 baseline:** 18.5 billion devices, +12% over 2023. [Tier 1] — [IoT Analytics](https://iot-analytics.com/number-connected-iot-devices/)
- **Downward revision, China weakness:** The 2025 figure is about 300 million connections below IoT Analytics' September 2024 forecast, because of "ongoing capex deferrals and softer-than-expected demand in China". [Tier 1] — [IoT Analytics](https://iot-analytics.com/number-connected-iot-devices/)
- **Connectivity mix of the installed base:** Wi-Fi 32%, Bluetooth 24%, cellular 22%. [Tier 1] — [IoT Analytics](https://iot-analytics.com/number-connected-iot-devices/)
- **Counting method:** IoT Analytics counts active nodes or gateways, not every end sensor. [Tier 1] — [IoT Analytics](https://iot-analytics.com/number-connected-iot-devices/)
- **Cellular IoT:** About 4.5 billion cellular IoT connections at end-2025, of which about 2.6 billion are broadband/critical IoT (4G/5G). The June 2026 Ericsson Mobility Report forecasts close to 8 billion by end-2031. Telecomlead, citing Ericsson, gives about 4.3 billion for 2025, which conflicts slightly. [Tier 1] — [Ericsson IoT connections outlook](https://www.ericsson.com/en/reports-and-papers/mobility-report/dataforecasts/iot-connections-outlook); [Ericsson Mobility Report June 2026 (PDF)](https://www.ericsson.com/49ce36/assets/local/reports-papers/mobility-report/documents/2026/ericsson-mobility-report-june-2026.pdf); [Telecomlead](https://telecomlead.com/broadband/global-cellular-iot-connections-to-reach-5-9-bn-by-2035-as-5g-advanced-accelerates-growth-126453)
- **Edge AI:** About 1% of IoT connections today, projected to reach about 25% by 2035. [Tier 1] — [IoT Analytics, Sep 2026](https://iot-analytics.com/embedded-systems-market-opportunities-and-challenges-for-developers/)
- **IoT share of MCU spend:** The IoT MCU market was $5.1 billion in 2024 (−9% vs 2023), out of total MCU spending of $23.2 billion. IoT MCU revenue was up 1.8% YoY in 1H2025 and is forecast to reach $7.32 billion by 2030 (6.3% CAGR). Total MCU spend is forecast at $29.4 billion by 2030 (3.9% CAGR). [Tier 1] — [IoT Analytics, "IoT MCU market: $7 billion opportunity by 2030", 8 Oct 2025](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/)
- **Regional MCU outlook:** All regions contracted in 2024. Asia is expected to recover most in 2025, and China is forecast to be the fastest-growing IoT MCU market through 2030. [Tier 1] — [IoT Analytics, Oct 2025](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/)

### Inferences
- About 22% of MCU dollars go into IoT ($5.1B / $23.2B in 2024, my calculation). By unit count the IoT share is probably higher, because IoT MCUs tend to be lower-ASP parts, but no source quantifies that.
- Wi-Fi plus Bluetooth make up 56% of the installed base. Those are the radios on ESP32-class and STM32WB/WBA-class parts, which fits a "connected device with screen" IDE better than cellular.
- China is both the fastest-growing IoT MCU region and the source of the 2025 forecast miss. A Taiwan-based tool vendor faces both upside and volatility there.

### Gaps
- No published share of IoT devices that are MCU-based (as opposed to MPU/Linux-based) was found.
- No published share of IoT devices or MCUs that drive a display was found.

## 2. Global MCU shipments (units and revenue) and share with displays or graphics

### Takeaway
MCU unit shipments are about 28–37 billion a year. The last firm figure is 28 billion in 2021; Yole has forecast 36.7 billion by 2028, and TechInsights expected +9% units in 2025. Revenue is about $23–24 billion (2024), heading to $29–34 billion by 2030. No analyst figure was found for the share of MCUs driving a display.

### Cited Findings
- **2021 units:** About 28 billion MCU units shipped in 2021, +3.6% vs 2020 (Statista citing Yole). This is a 2021 figure. [Tier 1 via aggregator] — [Statista / Yole](https://statista.com/statistics/1327567/worldwide-microcontroller-unit-shipments-forecast)
- **Unit forecast:** Yole's earlier outlook expected more than 36.7 billion units by 2028. Its mid-2024 monitor said 2024 shipments would rise more than 3% even as revenue dipped. [Tier 1] — [Yole Group](https://www.yolegroup.com/?p=219884); [Yole MCU market monitor](https://www.yolegroup.com/product/quarterly-monitor/microcontroller-mcu-market-monitor-/)
- **2025 units:** TechInsights' McClean Report (February 2025) forecast MCU unit shipments up 9% in 2025, with ASP slightly down. [Tier 1] — [TechInsights, McClean Report Feb 2025](https://techinsights.com/blog/mcclean-report-february-2025-microcontroller-market-update)
- **Yole revenue outlook (December 2025):** The MCU market is forecast to pass $34 billion by 2030, a 6% CAGR for 2024–2030. Automotive is the largest driver at $13 billion by 2030. Infineon, NXP, Renesas and ST together held almost 70% of the 2024 market. Yole's quarterly MCU monitor stopped updating after Q4 2025. [Tier 1] — [Yole via Edge AI and Vision Alliance, Dec 2025](https://www.edge-ai-vision.com/2025/12/microcontrollers-enter-a-new-growth-cycle-as-the-market-targets-us34-billion-in-2030/); [Yole MCU monitor page](https://www.yolegroup.com/product/quarterly-monitor/microcontroller-mcu-market-monitor-/)
- **IoT Analytics revenue:** $23.2 billion in 2024, rising to $29.4 billion in 2030 (3.9% CAGR). [Tier 1] — [IoT Analytics, Oct 2025](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/)
- **Tier-3 revenue spread for 2025:** Research and Markets $34.75 billion; Future Market Insights $36.4 billion; a MarketsandMarkets-style figure of $23.94 billion. Marketintelo claims more than 35 billion units shipped by mid-2026, which is unverified. [Tier 3] — [Research and Markets](https://www.researchandmarkets.com/reports/5601267/global-microcontroller-mcu-market-share); [Future Market Insights](https://www.futuremarketinsights.com/reports/microcontroller-unit-market); [MarketResearch.com](https://www.marketresearch.com/Knowledge-Sourcing-Intelligence-LLP-v4221/Microcontroller-Unit-Forecasts-42726396); [Marketintelo](https://marketintelo.com/report/microcontroller-unit-market/amp)
- **Arm:** More than 310 billion Arm-based chips shipped cumulatively, with 30.6 billion in FY2025. Arm said in 2021 that Cortex-M accounts for about three-quarters of annual Arm-based shipments. That is an older ratio, and applying it to FY2025 is an extrapolation. [Tier 1 company statement] — [Quartr summary of Arm Q4 FY25 earnings](https://quartr.com/events/arm-holdings-plc-arm-q4-2025_FGFOjaSG); [Arm 200bn chips blog](https://newsroom.arm.com/blog/200bn-arm-chips)
- **Espressif (connected-MCU proxy, China):** 2025 revenue was about CNY 2.565 billion. 1H2025 revenue was CNY 1.246 billion (+35.35% YoY), led by smart home. 9M2025 revenue was CNY 1.912 billion (+30.97%). Cumulative IoT chip shipments exceed 1.5 billion. Quartr's "+40%" for Q1 2026 conflicts with the filing's own numbers (CNY 647.97M vs 557.85M, about +16%). [Tier 1 filings via aggregators] — [Espressif 2025 Annual Report](https://www.espressif.com/sites/default/files/financial/Espressif%20Systems%202025%20Annual%20Report.pdf); [cfi.cn 2025 H1 report](https://cfi.cn/p20250830001929.html); [Quartr Q1 2026](https://quartr.com/events/espressif-systems-co-ltd-688018-q1-2026_Fgwbarc3)
- **Display/GUI share (older survey data only):** The 2015 UBM (later AspenCore) Embedded Markets Study listed "GUI" as a feature of current embedded projects at about 34–37%. The year mapping in the extracted slide is ambiguous. [Tier 2 survey, 2015] — [UBM 2015 Embedded Markets Study (PDF)](https://webpages.charlotte.edu/~jmconrad/ECGR4101-2016-08/notes/UBM%20Tech%202015%20Presentation%20of%20Embedded%20Markets%20Study%20World%20Day1.pdf) *(Second-round check 2026-11-20: corrected to 36–37% (capabilities question) to 49% (direct question), 2015–2017; see verification_market_ai.md.)*

### Inferences
- Taking 28 billion units (2021) with about 3–9% yearly growth, 2025 shipments land at roughly 31–35 billion units. That is my extrapolation, not a published figure.
- The two Tier-1 revenue curves agree on the base (about $23–24 billion in 2024; Yole's $34B at 6% CAGR implies about $24B in 2024). They differ on growth: 3.9% (IoT Analytics) vs 6% (Yole). Tier-3 2025 figures of $34–36 billion look inflated against both.
- Espressif's +30–35% growth during 2025 while overall IoT MCU revenue was roughly flat (+1.8% 1H2025) suggests that connected, Wi-Fi-native MCUs aimed at smart home, increasingly with display variants such as the ESP32-S3/P4, are taking share within a flat market.
- If roughly a third of embedded projects carry a GUI (2015 survey), the count of display-bearing MCU designs is large. Unit volume, however, is dominated by display-less parts (sensors, motor control), so the display share by units is likely well under a third.

### Gaps
- No Tier-1 figure for 2024 or 2025 MCU units was found; Yole's figures are in paid reports.
- No source quantifies the share of MCUs with a display controller (LTDC, MIPI-DSI, parallel RGB) or a 2D GPU (Chrom-ART, NXP PXP/VGLite), or the share of MCU-based products with a TFT.
- No current AspenCore or Embedded.com GUI-feature percentage was found.

## 3. Market size and CAGR of adjacent software markets

### Takeaway
The software layers stack up as follows. Low-code overall is about $58 billion by 2029 (Gartner, 14.1% CAGR), but it is enterprise-IT dominated. Embedded software is about $18–21 billion (2025, Tier 3 only). IoT platforms are about $9–16 billion (Tier 3). HMI software is about $6 billion (Tier 3). Embedded development tools are $2.3–7.7 billion (Tier 3, a spread of more than 3x). The commercial embedded tools niche has historically been small: VDC sized it below $0.5 billion in the 2000s.

### Cited Findings
**Low-code development technologies**
- **Gartner, 4 Nov 2025:** "Forecast Analysis: Low-Code Development Technologies, Worldwide" projects the market at $58.2 billion by 2029, a 14.1% CAGR. Named drivers are agentic AI, citizen development and operational excellence. [Tier 1] — [Gartner document 7146430](https://www.gartner.com/en/documents/7146430)
- **Older Gartner vintages:**
  - December 2022: $26.9 billion in 2023 (+19.6%), with LCAP the largest segment and citizen automation platforms growing fastest (+30.2%) — [Gartner press release, 13 Dec 2022](https://www.gartner.com/en/newsroom/press-releases/2022-12-13-gartner-forecasts-worldwide-low-code-development-technologies-market-to-grow-20-percent-in-2023)
  - A widely quoted $44.5 billion by 2026 — [InfoWorld](https://www.infoworld.com/article/2337677/low-code-development-technologies-market-forecast-to-hit-445-billion-by-2026.html)
  - February 2021: $13.8 billion for 2021 (+23%) — [Gartner press release, Feb 2021](https://www.gartner.com/en/newsroom/press-releases/2021-02-15-gartner-forecasts-worldwide-low-code-development-technologies-market-to-grow-23-percent-in-2021)
  - Each vintage revised the trajectory, so they should not be mixed.

**IoT platforms**
- **IoT Analytics, IoT Platforms Market Report 2025–2030 (August 2025):** The market grew steadily from 2010 to 2025, but "the core assumption of exponential platform-scale demand never materialized". Competition peaked in 2019. IoT Analytics' own dollar figure was not visible in what was retrieved. [Tier 1] — [IoT Analytics report page](https://iot-analytics.com/product/iot-platforms-market-report-2025-2030/); [IoT Analytics, "What happened to IoT platforms"](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)
- **Tier-3 spread for 2025 (about $9–16 billion):**
  - Fortune Business Insights: about $15.98 billion (2025), rising to $47.63 billion in 2034 at 12.91% CAGR — [Fortune](https://www.fortunebusinessinsights.com/iot-platform-market-116234)
  - Expert Market Research: $14.52 billion — [MarketResearch.com](https://www.marketresearch.com/Expert-Market-Research-v4220/Internet-Things-IoT-Platform-Size-44618735/)
  - VPA Research: $12.73 billion — [MarketResearch.com](https://www.marketresearch.com/VPA-Research-v4245/IoT-Platform-Size-Share-Outlook-40858927)
  - DIResearch: about $9.08 billion — [MarketResearch.com](https://www.marketresearch.com/Deep-Insights-Research-Co-DIR-v4285/Global-IoT-Platforms-Research-Competitive-41706216)
  - MarketsandMarkets: $65.29 billion by 2030 at 8.1% CAGR — [MarketsandMarkets](https://www.marketsandmarkets.com/Market-Reports/iot-platform-market-247198638.html)
  - [Tier 3]

**Embedded systems and embedded software**
- **Embedded systems (hardware plus software), 2025:** $585 billion, about 32% of a $1.8 trillion electronic-systems market. Forecast is 5.6% CAGR to about $857 billion by 2032. Automotive/mobility plus industrial/robotics are more than half of demand. [Tier 1] — [IoT Analytics, 3 Sep 2026](https://iot-analytics.com/embedded-systems-market-opportunities-and-challenges-for-developers/)
- **Embedded software, 2025 (Tier 3, about $18–21 billion, CAGR about 6.4–9.8%):**
  - Fortune $19.98 billion (9.64% CAGR to $45.75 billion in 2034) — [Fortune](https://www.fortunebusinessinsights.com/embedded-software-market-116609)
  - GMI $21 billion (8.67% CAGR, 2026–2035) — [GMI](https://www.gminsights.com/industry-analysis/embedded-software-market)
  - MRFR $20.09 billion — [MRFR](https://marketresearchfuture.com/reports/embedded-software-market-2103)
  - Straits $19.01 billion — [Straits](https://straitsresearch.com/report/embedded-software-market)
  - IMARC $18.42 billion (6.41% CAGR) — [IMARC](https://www.imarcgroup.com/embedded-software-market)
  - Insight Partners $11.47 billion for "embedded software and tools", a narrower definition — [Insight Partners](https://www.theinsightpartners.com/reports/embedded-software-and-tools-market)
  - [Tier 3]

**Embedded development tools**
- **Tier-3 spread for 2025 ($2.3–7.7 billion, more than 3x):**
  - Grand View Horizon puts "software/embedded development tools" within the MCU market at $2,288.6 million in 2025, reaching about $7.0 billion by 2033 — [Grand View Research Horizon](https://www.grandviewresearch.com/horizon/statistics/microcontroller-market/embedded-systems/software-embedded-development-tools/global)
  - Wise Guy Reports: "Embedded Software Tool Market" $7.65 billion in 2025, reaching $15.0 billion by 2035 (7.0% CAGR) — [Wise Guy Reports](https://www.wiseguyreports.com/reports/embedded-software-tool-market)
  - Marketintelo: automotive embedded software development tools $4.8 billion in 2025, reaching $8.9 billion by 2034 — [Marketintelo](https://marketintelo.com/report/automotive-embedded-software-development-tools-market)
  - [Tier 3]
- **Historical VDC sizing (Tier 1 but old):**
  - Embedded software development tools were "more than $290 million" in 2009, about 10% below 2008 — [Military & Aerospace Electronics citing VDC](https://www.militaryaerospace.com/commercial-aerospace/article/14224846/slow-and-steady-growth-expected-for-embedded-software-development-tools-market)
  - Over $330 million in 2002, forecast at $525.2 million for 2007 (9.5% CAGR) — [LinuxDevices archive citing VDC](https://linuxdevices.org/findings-from-vdcs-embedded-software-strategic-market-research)
  - A later VDC blog says the market for commercial standalone tools "remains under pressure and challenged" — [VDC blog](https://www.vdcresearch.com/News-events/iot-blog/2012/software-development-tools-market-update.html)

**HMI software**
- **Tier-3 spread for 2025 (about $5.8–6.2 billion, CAGR 5.5–15.8%):**
  - Marketintelo $5.8 billion (10.2% CAGR) — [Marketintelo](https://marketintelo.com/report/hmi-software-market)
  - TechSci $6.23 billion (15.81% CAGR, 2026–2031) — [TechSci](https://techsciresearch.com/report/human-machine-interface-hmi-software-market/25451.html)
  - Emergen 5.5% CAGR — [Emergen](https://www.emergenresearch.com/industry-report/hmi-software-market)
  - Lucintel 7.9% CAGR — [MarketResearch.com / Lucintel](https://www.marketresearch.com/Lucintel-v2747/HMI-Software-Trends-Forecast-Competitive-41923262)
  - MRFR 7.8% CAGR — [MRFR](https://www.marketresearchfuture.com/reports/hmi-software-market-8653)
  - Broader HMI (hardware plus software): $6.63 billion in 2026 rising to $12.74 billion by 2034 (8.5% CAGR) — [Fortune](https://www.fortunebusinessinsights.com/human-machine-interface-market-114618)
  - [Tier 3]
  - These "HMI software" figures mostly describe industrial SCADA/HMI runtime and configuration software (Siemens, Rockwell, AVEVA and similar), not embedded MCU GUI tools.

**Embedded display / small TFT modules**
- **Omdia, industrial displays:** 281.1 million industrial display panels shipped in 2024, expected to reach 290.6 million in 2025 (+3.4%). "Smart home and office" is 64% of those units. A January 2026 release said industrial display panel makers' revenue reached $3.4 billion in 2025 (+24% YoY), and noted that Tianma ranked third on the strength of HMI, handheld, home appliance and home medical products. [Tier 1, from search snippets; the full page returned 403] — [Omdia industrial display press release, Jan 2026](https://omdia.tech.informa.com/pr/2026/jan/industrial-display-revenues-grow-24percent-yoy-in-2025-led-by-auo-display-plus); [Business Wire, Omdia industrial, Jul 2025](https://www.businesswire.com/news/home/20250704977334/en/Industrial)
- **Omdia, small/medium displays (below 9 inches, all applications, smartphone-dominated):**
  - 1Q25: 1,048 million units
  - 3Q25: 740.0 million units (+6% QoQ, +4% YoY)
  - 2Q25: Omdia figures conflict (696.6 million on the tracker page vs 942 million in a June 2025 projection)
  - [Tier 1] — [Omdia S/M tracker 3Q25](https://omdia.tech.informa.com/om138703/small-medium-display-market-tracker--3q25-analysis-with-2q25-results); [Business Wire, Jun 2025](https://www.businesswire.com/news/home/20250618242100/en/Omdia-Inventory-Adjustments-to-Trigger-10-Drop-in-Small-and-Medium-Display-Shipments-in-2Q25)

### Inferences
- The 14.1% low-code CAGR is enterprise application software (Mendix, OutSystems, Power Apps and similar). Nothing in Gartner's abstract covers embedded or firmware low-code, so it supports "low-code is mainstream" but does not size an embedded low-code segment.
- Industrial display units (about 290 million in 2025, of which about 186 million are "smart home and office" at 64%) are the most direct volume proxy found for "devices with screens outside phones, PCs and cars". Not all are MCU-driven; some are MPU/Linux.
- The VDC data points (below $0.5 billion through the 2000s, "under pressure") and the Tier-3 $2.3–7.7 billion figures cannot both describe the same thing. The Tier-3 numbers probably bundle IDEs, compilers, debuggers/probes, RTOS, test and services. Revenue for pure commercial MCU IDE and GUI tools is likely a low-single-digit share of these headline numbers. That is an inference.
- IoT Analytics' finding that IoT platform demand never became exponential is a direct caution for any "IoT IDE plus cloud" business plan.

### Gaps
- No Tier-1 sizing of an "embedded GUI software" or "MCU HMI tools" market was found (nothing from VDC, Omdia or IDC). Comparable data would need VDC's paid embedded software reports.
- No current VDC dollar figure for embedded development tools was found.
- No Forrester or IDC low-code sizing was retrieved; only Gartner was verified.
- No Omdia unit breakdown for TFT modules in home appliances, EV chargers or medical devices was found.

## 4. Vertical demand: EV chargers, smart appliances, industrial HMI, medical, building automation

### Takeaway
The strongest unit-growth vertical with verified data is public EV charging: more than 1.3 million public points added in 2024 (+30%) and a global stock above 7 million by end-2025. Smart home device shipments are large (about 0.9 billion a year) but grow slowly (about 4%). Building automation and industrial HMI figures are mostly Tier-3 dollar values. No usable medical-device unit data was found.

### Cited Findings
- **EV charging (IEA Global EV Outlook 2025):** More than 1.3 million public charging points were added in 2024, more than 30% above the previous year. Europe's public stock grew more than 35% to just over 1 million points, and the US stock grew 20% to just under 200,000. [Tier 1] — [IEA GEVO 2025, EV charging chapter](https://www.iea.org/reports/global-ev-outlook-2025/electric-vehicle-charging)
- **EV charging (IEA GEVO 2026):** The global public charging stock passed 7 million points at end-2025. This comes from a search snippet; the page returned 403. Electromaps' "over 4 million installed in 2024" conflicts with the IEA additions figure and probably confuses stock with additions. [Tier 1 snippet / Tier 3 conflict] — [IEA GEVO 2026 charging chapter](https://iea.org/reports/global-ev-outlook-2026/electric-vehicle-charging-chap-6-and-10); [Electromaps](https://electromaps.com/en/blog/en-ieas-global-ev-outlook-2025)
- **Smart home (IDC, October 2024):** 892.3 million smart home devices shipped in 2024 (+0.6%), forecast to reach 931.1 million in 2025 (+4.4%), with growth led by emerging markets. IDC's March 2021 forecast of 1.4 billion in 2025 (12.2% CAGR) proved far too high. [Tier 1] — [IDC press release, 1 Oct 2024](https://www.idc.com/resource-center/press-releases/smart-home-market-on-track-for-rebound-thanks-to-emerging-regions-according-to-idc/); [Business Wire, IDC Mar 2021](https://www.businesswire.com/news/home/20210326005053/en)
- **Smart home sub-segment:** Smart vacuums shipped 6.17 million units in 2Q25 (+20.5% YoY), with Chinese brands gaining share. [Tier 1] — [IDC, 3 Sep 2025](https://www.idc.com/resource-center/press-releases/global-smart-vacuum-market-grows-20-5-year-over-year-in-q2-2025-according-to-idc/)
- **Smart home saturation:** TechInsights' "Smart Home Devices Forecast 2025–2030" describes growth slowing as the market nears saturation. [Tier 1, title only] — [TechInsights](https://www.techinsights.com/ja/node/60701)
- **Wi-Fi smart home devices:** ABI Research forecasts a 16% CAGR for Wi-Fi smart home device shipments over 2025–2030. [Tier 1] — [ABI Research chart](https://www.abiresearch.com/news-resources/chart-data/forecast-wifi-device-shipments-by-connectiivty-technology)
- **Connected major appliances:** About 175 million annual shipments projected for 2025, up from about 54.6 million in 2020, when connected models were 8% of all major appliances. This is an Omdia projection made in about 2021, not a 2025 actual. [Tier 1, dated] — [Omdia](https://omdia.tech.informa.com/om018781/strategy-shifts-contribute-to-million-connected-major-home-appliance-shipments-in)
- **Wi-Fi share of smart appliances by revenue:** About 58% (Fortune) and 57.93% (another vendor). [Tier 3] — [Fortune Business Insights](https://www.fortunebusinessinsights.com/smart-appliances-market-116511)
- **Building automation, dollars:**
  - Memoori: Building IoT in commercial buildings $64.1 billion (2024), reaching $101.0 billion by 2030; IoT devices in commercial buildings 4.12 billion by 2030. [Tier 2 specialist] — [Memoori IoT Platforms 2025 synopsis (PDF)](https://memoori.com/wp-content/uploads/IoT-Platforms-2025-Synopsis.pdf); [Memoori IoT Devices 2025 synopsis (PDF)](https://memoori.com/wp-content/uploads/IoTDevices2025_Synopsis.pdf)
  - MarketsandMarkets: building automation systems $101.34 billion (2025), reaching $191.13 billion by 2030 (13.4% CAGR) — [MarketsandMarkets](https://www.marketsandmarkets.com/Market-Reports/intelligent-building-automation-technologies-408.html)
  - Fortune: $87.85 billion (2025), reaching $184.42 billion by 2034 (8.7% CAGR)
  - [Tier 3]
- **Industrial and medical displays:** Omdia's industrial display data (section 3) covers HMI, home medical and appliance panels together, at about 290.6 million units in 2025. [Tier 1 snippet] — [Omdia, Jan 2026](https://omdia.tech.informa.com/pr/2026/jan/industrial-display-revenues-grow-24percent-yoy-in-2025-led-by-auo-display-plus)
- **Grid/energy context (China, India):** State Grid announced a record $88 billion grid investment in January 2025. India targets 250 million smart meter installations by 2027. [Tier 1] — [IoT Analytics, Oct 2025](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/)

### Inferences
- EV chargers are the best-fitting vertical for "screen plus connectivity plus OTA", since public chargers need OCPP connectivity, a user display and remote updates. Volume is about 1.3–2 million public units a year plus a larger home-charger volume, which was not quantified. That is a design-win market measured in thousands of OEM projects, not billions of units.
- Smart home is the volume engine (about 0.9 billion units a year) but grows slowly. The growth pocket is Wi-Fi/Matter-native devices (ABI 16% CAGR) and appliances with displays from Chinese OEMs.
- Regional split: Europe leads public-charging growth (+35%), China leads MCU growth and appliance and robot-vacuum share, and the US is slower on charging (+20%).

### Gaps
- No Tier-1 unit volumes were found for industrial operator panels or HMI terminals.
- No unit volumes were found for connected medical devices with displays.
- No unit volumes were found for building-automation wall controllers or thermostats with screens.
- Home and private EV charger shipments were not found.
- The IEA GEVO 2026 page could not be fetched, so 2025 additions by region are unverified.

## 5. Embedded spend split (tools vs silicon vs services) and tool spend per developer

### Takeaway
No source found publishes a tools vs silicon vs services split of embedded developer spending, or spend per developer on tools. Rough proportions can be triangulated: embedded systems about $585 billion, MCU silicon about $23 billion, embedded software about $20 billion (Tier 3), and development tools about $2–8 billion (Tier 3), historically below $0.5 billion (VDC, 2009).

### Cited Findings
- **Embedded systems:** $585 billion (2025). [Tier 1] — [IoT Analytics, Sep 2026](https://iot-analytics.com/embedded-systems-market-opportunities-and-challenges-for-developers/)
- **MCU silicon:** $23.2 billion (2024). [Tier 1] — [IoT Analytics, Oct 2025](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/)
- **Embedded software:** About $18–21 billion (2025). [Tier 3] — [Fortune](https://www.fortunebusinessinsights.com/embedded-software-market-116609); [GMI](https://www.gminsights.com/industry-analysis/embedded-software-market)
- **Development tools:** $2.29–7.65 billion (2025) from Tier-3 sources — [Grand View Horizon](https://www.grandviewresearch.com/horizon/statistics/microcontroller-market/embedded-systems/software-embedded-development-tools/global); [Wise Guy](https://www.wiseguyreports.com/reports/embedded-software-tool-market). VDC put it at $290 million or more in 2009 (Tier 1, dated) — [Military & Aerospace Electronics citing VDC](https://www.militaryaerospace.com/commercial-aerospace/article/14224846/slow-and-steady-growth-expected-for-embedded-software-development-tools-market)
- **Pricing pressure:** VDC describes commercial standalone tools as under pricing and competitive pressure, which fits with free vendor IDEs (STM32CubeIDE, MCUXpresso, ESP-IDF). [Tier 1] — [VDC blog](https://www.vdcresearch.com/News-events/iot-blog/2012/software-development-tools-market-update.html)

### Inferences
- If MCU silicon is about $23 billion and tools are about $2–8 billion, tools come to roughly 10–30% of MCU silicon spend in dollars. This ratio rests on Tier-3 numbers and is likely overstated for MCU-class work, because much tool spend is automotive (about $4.8 billion per Marketintelo).
- Spread across about 1–2.5 million embedded developers (section 6), $2–8 billion works out to about $1,000–8,000 per developer per year. That is a top-down inference with wide error bars, not a survey result. Free vendor IDEs pull the median MCU developer's paid tool spend far below this.

### Gaps
- No VDC, AspenCore or Eclipse survey figure on tool budget per engineer was found; VDC's paid "Embedded Engineer Survey" probably has it.
- No breakdown of tools vs silicon vs services was found from any Tier-1 source.

## 6. How many embedded and IoT developers exist worldwide

### Takeaway
Estimates range from about 0.5 million to 4 million depending on definition. SlashData's roughly 5% "self-identified embedded" share applied to a 27–47 million total developer population gives about 1.3–2.4 million. VDC counted more than 880,000 in 2010.

### Cited Findings
- **SlashData (Q1 2023):** Developers who self-identify as embedded are about 5% of developers worldwide, a share steady for about two years. [Tier 1 survey firm, 2023] — [SlashData](https://slashdata.co/embedded-software-dei/embedded-developers-population-forecast). The page as fetched in November 2026 no longer shows this figure.
- **SlashData, related scale references:** 19.9 million cloud-native developers (about 39% of developers) in Q1 2026, implying about 51 million developers in total (my calculation). JavaScript has 27.3 million developers (Q1 2026). [Tier 1] — [SlashData](https://slashdata.co/embedded-software-dei/embedded-developers-population-forecast)
- **VDC (2010):** More than 880,000 embedded developers. [Tier 1, dated] — [Percepio blog citing VDC / Max Maxfield](https://percepio.com/tag/max-maxfield/)
- **Practitioner estimates:**
  - Jack Ganssle: about 500,000 firmware developers worldwide — [Ganssle](https://www.ganssle.com/rants/wearenotthem.html)
  - Max Maxfield: 2–4 million "embedded professionals", including hardware engineers — [Percepio](https://percepio.com/tag/max-maxfield/)
  - [opinion]
- **Total developer population:** Estimates of about 26–27 million and about 47.2 million (early 2025) conflict. [Tier 3 blogs] — [Grid Dynamics](https://www.griddynamics.com/blog/number-software-developers-world)
- **Eclipse 2024 IoT & Embedded Developer Survey (December 2024):**
  - Connectivity is still the top concern but fell to 48% (from 52%); security rose to 35%.
  - OS use: Linux 46%, FreeRTOS 29%, Zephyr gaining.
  - No 2025 edition was found.
  - [Tier 1 community survey] — [Eclipse Foundation, Dec 2024](https://newsroom.eclipse.org/node/41237); [ADTmag](https://adtmag.com/Articles/2024/12/03/Eclipse-Foundation-Survey.aspx)
- **AspenCore 2017 survey sample:** 1,234 respondents (North America 56.3%, Europe 25.2%, Asia 10.6%). This is a reminder that Western embedded surveys under-sample Asia. [Tier 2] — [CNX Software](https://cnx-software.com/2017/08/15/aspencore-2017-embedded-markets-study-programming-languages-operating-systems-mcu-vendors-and-more)

### Inferences
- A defensible planning range is about 1–2.5 million professional embedded software developers, of whom maybe a third or more work on GUI projects (2015 survey). That gives very roughly 0.3–0.8 million developers who touch embedded GUIs. This chain of inferences is unverified.
- Embedded survey samples under-weight Asia, while China is the fastest-growing MCU and IoT region, so Asian developer counts are likely understated in Western estimates.
- Linux (46%) leading over FreeRTOS (29%) among Eclipse respondents shows that many connected-device projects with screens run on MPU/Linux. An MCU-only IDE addresses part of the "devices with screens" developer base, not all of it.

### Gaps
- No current (2024–2026) Tier-1 count of embedded developers was found (VDC, Evans Data or SlashData). SlashData's embedded population forecast page now redirects to unrelated content.
- No country split was found for China, the EU or the US.
- No count of LVGL, TouchGFX or other embedded GUI framework users was found within this research scope.

## Note on conflicting estimates (summary table)

| Metric | Low | High | Spread | Best anchor |
|---|---|---|---|---|
| Connected IoT devices 2025 | 21.1B (IoT Analytics) | — | YoY growth quoted as 13% vs 14% | IoT Analytics 21.1B |
| Cellular IoT 2025 | 4.3B (Telecomlead) | 4.5B (Ericsson) | ~5% | Ericsson 4.5B |
| MCU revenue 2024/25 | $23.2B (IoT Analytics 2024) | $36.4B (FMI 2025, Tier 3) | ~1.6x | IoT Analytics / Yole (~$23–24B) |
| MCU revenue CAGR to 2030 | 3.9% (IoT Analytics) | 6% (Yole) | 2.1 pp | Use both |
| IoT platforms 2025 | $9.08B | $15.98B | ~1.8x (all Tier 3) | None reliable |
| Embedded software 2025 | $11.47B (narrow) / $18.4B | $21B | 1.1–1.8x (Tier 3) | Range only |
| Embedded dev tools 2025 | $2.29B | $7.65B | 3.3x (Tier 3) | Range only |
| HMI software 2025 | $5.8B | $6.23B | CAGR 5.5–15.8% | Range only |
| Low-code | $26.9B (2023) | $58.2B (2029) | Forecast vintages differ | Gartner Nov 2025 |
| Public EV chargers added 2024 | 1.3M (IEA) | "4M" (Electromaps) | IEA vs secondary source | IEA |
| Smart home devices 2025 | 931M (IDC 2024 forecast) | 1.4B (IDC 2021 forecast) | Old forecast overshot | IDC 931M |
| Embedded developers | 0.5M (Ganssle) | 2–4M (Maxfield) | ~4–8x | 1–2.5M |
