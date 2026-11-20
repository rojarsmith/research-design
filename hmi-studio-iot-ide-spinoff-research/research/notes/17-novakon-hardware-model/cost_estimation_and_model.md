# Estimating BOM, Factory Cost and Gross Margin of 7" / 10" Industrial HMI Panels, and the Economics of the "Free Software, Outsourced Hardware" Model

Research date: 2026-11-20. Scope: methods and benchmarks for a Taiwan software spin-off (from EDT, a display-module maker with a free no-code LVGL HMI tool) that is weighing a NOVAKON-style model: give the HMI design software away and earn revenue on HMI panels built by contract manufacturers. Annual-report margins of HMI vendors are out of scope; a separate note covers them.

## Labels used

- **[primary]**: the issuing organisation's own document: a regulator, a standard, a company filing, an analyst's own press release, or a maker's own product or price page about its own product.
- **[secondary]**: a journalist, trade-press or advisory compilation of other sources (for example Lincoln International's EMS review, built from Capital IQ and company filings).
- **[vendor]**: marketing or blog claims by a company with a commercial interest (labs, consultants, sellers' blogs, a brand describing itself).
- **[marketplace listing]**: a price or term shown on a distributor, e-commerce or B2B listing (DigiKey, Wolf Automation, Zoro, eBay, Alibaba, Made-in-China, JD, LCSC). These are one seller's price on one date, not quotes.
- **[inference]**: my own calculation or judgement from the cited inputs.

**Currency assumptions [inference]:** USD 1 ≈ RMB 7.1 ≈ TWD 31 for rough conversions. Prices are in USD unless stated.

**Method note:** Most evidence came from search-engine summaries and directly fetched pages. Several pages could not be fetched: Panelook (not reached), the IHS/electronics360 NEC panel-PC teardown (HTTP 403), makepolo (403), the CalcuQuote markup blog (404), and the UL Hong Kong price list (404). Figures from those pages come from search snippets only and are flagged where used.

---

## 0. Executive summary [inference unless tagged]

1. **Bottom-up BOM (2026 prices, ~1k–5k units/yr, China/Taiwan sourcing):**
   - 7" Linux-class HMI (1024×600 TFT, Cortex-A7/A53, 256 MB–1 GB DRAM, 4–8 GB eMMC, Ethernet, 2–3 serial ports, USB host, plastic IP65 front): about **US$45–90 (midpoint ~65)** for an entry configuration and **US$85–170 (midpoint ~125)** for a PCAP, 1 GB-RAM, aluminium-front configuration.
   - 10.1" equivalent: about **US$70–125 (midpoint ~95)** entry and **US$120–230 (midpoint ~170)** upper.
   - Before the memory shock (2025 H1), the same BOMs were roughly US$5–35 lower. The swing comes almost entirely from DRAM and eMMC.
2. **Factory cost (ODM/EMS ex-works or FOB):** BOM plus US$5–12 of assembly and test, plus an 8–25% ODM/EMS margin and overhead. The low end is a pure EMS build-to-print; the high end is a white-label ODM selling its own design. That gives:
   - 7" FOB: **~US$55–115** (entry) and **~US$100–220** (upper).
   - 10" FOB: **~US$80–165** (entry) and **~US$140–295** (upper).
   - These ranges match Chinese finished-goods prices at quantity 1: Mochuan 7" at US$120–160 and 10.1" at US$185–190 [marketplace listing].
3. **Street prices:**
   - Branded Taiwan/US 7" HMIs sell for about US$345–400 (Weintek MT8071iP/MT8072iP in the US), and capacitive "cMT" 7" models for US$650–845.
   - Chinese brands sell for US$90–300 export and about RMB 450–750 domestically (Kunlun TPC7062, snippet only).
   - Street-price-to-BOM ratio: about **4–6× for branded Taiwan/US products** and about **1.5–2.5× for Chinese brands**.
4. **Channel stack (Western markets, inferred):**
   - Of a US$100 list/street price: distributor or reseller 20–30, brand gross margin 35–50, factory price 25–40.
   - Of the factory price, the ODM/EMS keeps 8–25% (EMS gross margin ~9–10% at large and mid-tier [secondary]; small EMS 25.6% gross but 2.5% operating [secondary]).
5. **Model economics:**
   - At realistic volumes for a newcomer (1k–5k panels/yr), hardware gross profit is roughly **US$60–200 per unit**. That is US$0.1–1M a year, before fixed costs.
   - Fixed costs per product family run about US$100–350k one-off (certifications, tooling, ODM NRE, driver work) plus US$50–150k a year (UL follow-up, compliance updates, RMA handling, driver maintenance).
   - The model works only if the software builds an installed base that pulls hardware, or if attach revenue exists (remote access, cloud, runtime licences on third-party hardware).
6. **Biggest structural risks:**
   - The 2026 memory shock (DDR4 8Gb spot at a record US$42.50 in Aug 2026; eMMC contract prices +147–183% in Q1 2026).
   - Display and memory EOL.
   - The PLC driver gap versus Weintek's "400+ PLC drivers" [primary].
   - Chinese HMIs at US$90–200 that also ship free software.
   - Channel conflict with EDT's own display-module customers.

---

## 1. BOM components and price ranges (2025–2026)

### 1.1 Display (TFT LCD) and touch

| Item | Evidence | Price | Label |
|---|---|---|---|
| 7" 1024×600 TFT with capacitive touch (module) | Global Sources listing, tiers to 2,000–2,499 pcs, 15–25 day lead time | ~US$15 → US$14.94 | marketplace listing — https://www.globalsources.com/Modul-LCD/Modul-LCD-TFT-1237031013p.htm |
| 7" 1024×600 TFT module (bare, touch optional) | BuyDisplay China retail | RMB 111.93 (~US$15.8) | marketplace listing — https://www.buydisplay.com/chinese/color-7-inch-1024x600-tft-lcd-panel-optional-touchscreen-at070tna2 |
| 7" capacitive touch panel with FT5316 controller | BuyDisplay China retail; RMB 51.14 at 100 pcs | RMB 56.85 (~US$8) | marketplace listing — https://www.buydisplay.com/chinese/7-inch-capacitive-touch-panel-with-controller-ft5316-for-800x480 |
| 7" 4-wire resistive touch panel | BuyDisplay: US$5.61, US$4.99 @100; RMB 38.15 on Chinese site; Alibaba US$7–8.80 | US$4–8 | marketplace listing — https://www.buydisplay.com/7-inch-4-wire-resistive-touch-screen-panel |
| 7" western-distributor modules (for contrast) | Crystalfontz EVE 7" US$138 → US$127 @1k; Ampire on RS SGD 413 | US$127–300 | marketplace listing — https://www.crystalfontz.com/product/cfaf1024600b0070sc-7-inch-1024x600-tft-lcd ; https://sg.rs-online.com/web/p/lcd-colour-displays/2056985 |
| 10.1" 1280×800 IPS LVDS panel (industrial brands) | Robotshop retail incl. US tariffs: AUO G101EVN01.0 US$95; BOE EV101WXM-N80 US$100; Innolux G101ICE-LH1 (−30 to 80 °C) US$140 | US$95–140 retail | marketplace listing — https://www.robotshop.com/products/dualturb-boe-ev101wxm-n80-101-wxga-ads-tft-lcd-display-module |
| 10.1" IPS LVDS (Focus LCDs, DigiKey) | US$255.87 @1, US$193.64 @100 (non-touch) | US$194–256 | marketplace listing — https://www.digikey.com/en/products/detail/focus-lcds/E101RGL1912LB6M350-N/16601998 |
| 10.1" panel, Chinese suppliers | Alibaba sample prices US$43 and US$45.50; cover-lens customisation MOQ 500 | ~US$43–46 (sample) | marketplace listing — https://www.alibaba.com/product-detail/Industrial-10-1-Inch-LCD-Panel_1600819562905.html |
| 10.1" PCAP touch | Alibaba LVDS kit with GT911 sample US$35; Global Sources USB PCAP US$25; BuyDisplay USB controller board US$18–20 | ~US$15–35 | marketplace listing — https://www.alibaba.com/product-detail/Complete-LCD-Touch-Display-Kits-LVDS_1600951811357.html ; https://www.globalsources.com/Capacitive-touch/Capacitive-touch-panels-1236365064p.htm |

**Inferences:**
- **Volume estimates.** At 1k–5k pcs from Chinese/Taiwanese panel houses, a 7" 1024×600 industrial-ish TFT plus touch costs about US$20–30 with resistive touch and US$25–40 with PCAP plus cover glass. A 10.1" costs about US$40–70 with resistive and US$55–95 with PCAP. Wide-temperature, high-brightness (800+ cd/m²) or optically bonded parts add 30–100%.
- **EDT angle.** As a display-module maker, EDT should be able to supply LCD plus touch near its own cost. Display plus touch is typically the largest BOM line (25–40% of BOM), so this is the spin-off's one structural cost advantage over a pure software entrant. It is also a channel-conflict point (section 5).
- **Panelook:** not reached in this round. Panelook shows panel specs publicly, but price access was not verified. Treat it as a spec and EOL-status source, not a price source.

### 1.2 SoC / CPU options used in HMIs

| SoC | Price evidence | Label |
|---|---|---|
| Allwinner T113-S3 (dual A7, **128 MB DDR3 in-package**) | LCSC US$5.84 @1, US$4.51 @90+ | marketplace listing — https://www.lcsc.com/product-detail/C5197687.html |
| Allwinner T113-i (industrial) | Xonelec US$10.23 @1, US$4.69 @119 | marketplace listing — https://www.xonelec.com/mpn/allwinnertech/t113i |
| Allwinner T507 (quad A53) | LCSC US$9.13 @1, US$6.53 @126+ | marketplace listing — https://www.lcsc.com/product-detail/C669217.html |
| MYiR T113-S3 CPU module (128 MB RAM + 256 MB NAND / 4 GB eMMC), "designed for HMI" | US$14 / US$17 (May 2023, pre-memory-shock) | secondary (CNX) — https://cnx-software.com/2023/05/11/14-allwinner-t113-s3-cpu-module-128mb-ram-256mb-nand-flash-4gb-emmc-flash |
| NXP i.MX6ULL (MCIMX6Y2) | Future Electronics US$7.83 @1k+; IBS US$12.51 @1k (16-week lead); Avnet 900 MHz US$9.71 @760+ (112-week factory lead) | marketplace listing — https://futureelectronics.com/p/semiconductors--microprocessors--embedded-processors/mcimx6y2dvm05abr-nxp-2127790 |
| ST STM32MP135 | ST eStore STM32MP135FAG7 US$10.31 @1–9, US$6.71 @260–1,039, US$5.84 @1,040–10k; DigiKey ~US$5.15 @250 with "constrained supply" note | primary (ST eStore) — https://estore.st.com/en/stm32mp135fag7-cpn.html ; marketplace listing — https://www.digikey.com/en/products/detail/stmicroelectronics/STM32MP135AAG3/17775578 |
| TI AM625/AM623 | Launch: "less than US$5 in 1,000-unit quantities" (June 2022) | primary (TI press release via CNX) — https://www.cnx-software.com/2022/06/07/ti-sitara-am623-am625-cortex-a53-socs-offer-low-power-ai-for-hmi-and-iot-applications/ |
| Variscite AM62x SoM | "starts at US$33" (Feb 2023) | secondary — https://linuxgizmos.com/ti-sitara-am62x-based-som-from-variscite-starts-at-33-00/ |
| TI AM335x (AM3352) | Status ACTIVE on TI page; price not displayed | primary — https://www.ti.com/product/AM3352 |
| Rockchip RK3568 (chip) | LCSC from US$18.66; Xonelec US$25.12 @1 → ~US$17.75 @490 | marketplace listing — https://www.xonelec.com/mpn/rockchip/rk3568 |
| RK3568 SoMs | Forlinx FET3568-C (2 GB/16 GB) US$54 on DigiKey; Rongpin SoM RMB 350 (~US$55) on Taobao; Mixtile Core 3568 US$95–133 | marketplace listing — https://www.digikey.com.mx/en/products/detail/forlinx-embedded/FORLINX-FET3568-C-202GSE16GCA10/21810672 ; secondary — https://www.cnx-software.com/2021/10/23/rockchip-rk3566-rk3568-som-board-to-board-connectors-castellated-holes/ |
| i.MX8M Mini SoMs | Toradex Verdin iMX8M Mini "from US$44.90" volume; Ezurio Nitrogen8M Mini SMARC US$86.90–149.20 (DigiKey) | primary (Toradex) — https://www.toradex.com/computer-on-modules/verdin-arm-family/nxp-imx-8m-mini ; marketplace listing — https://www.digikey.com/en/product-highlight/b/boundary-devices/nitrogen8m-mini-smarc |
| NOVAKON's own HMIs (benchmark spec) | S07: ARM Cortex-A9 1 GHz Dual-Lite (i.MX6DL class), 1 GB SDRAM, 4 GB eMMC; P07: Cortex-A8 1 GHz, 512 MB, 4 GB eMMC; resistive touch | vendor (DirectIndustry) — https://directindustry.com/prod/novakon-co-ltd/product-222514-2457560.html ; https://www.directindustry.com/prod/novakon-co-ltd/product-222514-2272877.html |
| Mochuan 10.1" (Chinese, benchmark spec) | 1 GHz Cortex-A8; 256 MB DDR3 + 4 GB eMMC or 512 MB DDR3 + 256 MB flash | marketplace listing — https://www.madeinchina.com/mall/show-Mochuan-10-1inch-Embedded-Touch-Screen-HMI-Panel-Monitor-for-PLC_906338.html |
| ST STM32MP2 | No price found this round | gap |

**Inferences:**
- **SoC cost range.** Chip-down SoC cost for a 7"/10" HMI is about US$5–10 (T113, STM32MP13, i.MX6ULL, AM62x entry), US$10–25 (RK3566/RK3568, AM62x quad) and US$45–130 as a SoM. SoMs cut NRE and certification risk but add US$20–80 per unit at low volume.
- **SiP memory advantage.** SoCs with in-package DRAM (T113-S3 with 128 MB, T113-S4 with 256 MB) shield the BOM from the 2026 DRAM shock, though not from eMMC and NAND. This suits an LVGL runtime, which needs far less RAM than a Qt/HTML/Android runtime.
- **MCU-class option.** EDT's tool is LVGL-based, so the spin-off could also target MCU-class HMIs (STM32H7/ESP32-P4-class with PSRAM). The low end of that segment is a 7" UART "smart display" at US$24–26 per piece (Alibaba, 1–999 pcs; logo customisation at MOQ 1,000) [marketplace listing — https://www.alibaba.com/product-introduction/HMI-screen-7-inch-1026x600dots-UART_62590373094.html]. Those are display modules, not PLC HMIs, but they show how low a non-Linux BOM can go.

### 1.3 DRAM and eMMC: the 2025–2026 memory shock

| Data point | Label / URL |
|---|---|
| TrendForce (31 Mar 2026): conventional DRAM contract +58–63% QoQ in 2Q26; NAND +70–75%; eMMC/UFS "rise sharply". | primary — https://www.trendforce.com/presscenter/news/20260331-12995.html |
| TrendForce series via search summary: DRAM increases moderated to +13–18% in 3Q26 and +10–15% (forecast) in 4Q26; eMMC/UFS supply "relatively more abundant" in 3Q26 as OEMs resisted. | secondary (TrendForce via iConnect007/guru3d) — https://iconnect007.com/article/151792/ai-server-demand-drives-4q26-memory-price-increases-consumer-pressure-persists/151789/smt ; https://www.guru3d.com/story/dram-and-nand-contract-prices-forecast-to-rise-as-much-as-in-q4-2026/ |
| eMMC contract prices: Q4 2025 +10–20%. Q1 2026 actual: 4 GB +165%, 8 GB +183%, 16 GB +147%. Analysts' forecasts: Q2 +60–100%, Q3 +30–50%, Q4 +20–30%. MLC capacity −41.7% in 2026 (TrendForce). | secondary (TechNews, 17 Mar 2026) — https://technews.tw/?p=1526986 |
| DDR4 8Gb (1Gx8 3200) spot: US$16 (Apr 2026) → US$20 (29 May) → record US$42.50 (10 Aug 2026). DDR4 8Gb contract/fixed ~US$25 (Aug) → US$27.50 (Oct 2026). | secondary (tech-insider summarising DRAMeXchange) — https://tech-insider.org/dram-ram-price-crisis-2026/ |
| Raspberry Pi (2 Feb 2026): "the cost of some parts has more than doubled over the last quarter". Price rises of +US$10 (2 GB), +15 (4 GB), +30 (8 GB), +60 (16 GB) on Pi 4/5 and CM4/CM5; 1 GB models protected. Later rounds in Apr, Oct and Nov 2026. | primary — https://raspberrypi.com/news/more-memory-driven-price-rises |
| SONOFF raised prices on DDR4/eMMC devices by +27–45% vs the previous price (Feb 2026). SolidRun: shortages "expanded to SoCs"; some items need >30 weeks. | secondary (CNX, 29 Jan 2026) — https://www.cnx-software.com/2026/01/29/electronic-components-price-and-lead-time-increases-announced-across-the-board-and-not-only-because-of-ram/ |
| Avnet: in "a typical embedded BOM... $5 each for the processor, flash, RAM", when "RAM jumps from $5 to $20, the economics of the entire product change". Memory changes can trigger requalification costs larger than the price difference. | vendor (distributor) — https://www.avnet.com/americas/resources/article/memory-is-breaking-product-plans/ (fetch failed; search summary) |
| Legacy parts: DDR3, early DDR4 and SLC NAND face rising EOL risk and unpredictable pricing. | secondary (Altium) — https://resources.altium.com/p/how-ai-broke-memory-market |

**Inference (BOM memory line):**

| Configuration | 2025 H1 | Late 2026 |
|---|---|---|
| 512 MB DDR3/DDR4 + 4 GB eMMC | US$4–7 | US$15–30 |
| 1 GB DDR4/LPDDR4 + 8 GB eMMC | US$6–10 | US$30–50 |

- At late-2026 prices, memory can be **25–35% of a 7" Linux-HMI BOM**, against about 10% before.
- Two practical consequences:
  - Size the runtime to 256–512 MB, or use SiP-DRAM SoCs, and use SPI-NAND where feasible.
  - Write price-adjustment clauses into ODM contracts, because ODMs will pass memory through at cost plus handling.

### 1.4 Power, I/O, PCB, enclosure

| Item | Evidence / estimate | Label |
|---|---|---|
| Ethernet PHY (RTL8201F) | LCSC ~US$0.19–0.30 @980+ | marketplace listing — https://lcsc.com/product-detail/Ethernet-ICs_Realtek-Semicon-RTL8201F-VB-CG_C45044.html |
| Isolated transceiver CA-IS2092VW (Chipanalog) | US$1.44 @1k (datasheet not verified as RS-485) | marketplace listing — https://www.lcsc.com/product-image/C7422559.html |
| RJ45 with magnetics, RS-232 (MAX3232-class), non-isolated RS-485, USB host switch, RTC + coin cell, 24 VDC input protection and buck converters | US$0.3–1.5 each; power stage US$2–5 | inference (typical LCSC-tier pricing; not individually cited) |
| PCB (4–6 layer, ~120–200 cm²) plus passives and connectors | US$6–14 | inference |
| Off-the-shelf die-cast aluminium box (reference) | Polycase AL-48P US$39.80 @1–9 → US$30.45 @50–99 | marketplace listing — https://www.polycase.com/al-48p |
| Injection-mould tooling | US$15k–80k (P20 steel), single uncertain source | vendor/low-reliability — https://industrialmonitordirect.com/blogs/knowledgebase/selecting-plastic-molding-for-custom-enclosures-process-comparison-guide |
| Die-cast vs folded-metal tooling | "initial tooling costs will always be higher" for die-cast or moulded | vendor (Rolec) — https://rolec-usa.com/en/news/blg1911-customized-enclosures |
| Housing in practice | Mochuan 10.1": aluminium alloy, front IP65 / rear IP20, 890 g. Weintek MT8071iP: plastic. | marketplace listing — Mochuan URL above; https://wolfautomation.com/hmi-7-0-plastic-600mhz-w-ethernet-not-ul |

**Inference (enclosure line):**
- Plastic front bezel plus back cover with gasket and clips: about US$5–10 per unit for 7" and US$8–15 for 10.1" at 1k–5k pcs, plus US$20–60k of moulds per size.
- Aluminium die-cast or CNC front frame (IP65/IP66): about US$12–25 for 7" and US$18–35 for 10.1", plus US$15–50k die-cast tooling, or no tooling if CNC is used at higher unit cost.
- Off-the-shelf ODM housings carry no tooling cost but put the logo on someone else's industrial design.

### 1.5 Certification and test costs

| Item | Evidence | Label |
|---|---|---|
| Applicable standards for HMIs | UL 61010-2-201 / CSA C22.2 No. 61010-2-201 (industrial control equipment); ANSI/ISA 12.12.01 and CSA C22.2 No. 213 for Class I Div 2 | primary (Schneider Electric product docs) — https://product-help.schneider-electric.com/Machine%20Expert/V1.1/en/hmigtu/hmigtu/Chapter1/Chapter1-5.htm ; Maple C1D2 page — https://maplesystems.com/hmi/class1-div2/ |
| UL listing cost | US$3k–15k plus US$1.5k–4k/yr follow-up (seller blog); US$15k–50k+ for complex products | vendor (low reliability) — https://seller.alibaba.com/blogs/2026/southeast-asia/testing-instruments/ul-fcc-certification-guide-alibaba-b2b ; https://www.accio-ai.com/blog/everything-you-need-to-know-about-ul-certification-costs |
| CE (EMC + LVD, self-declared with lab testing) | €2k–5k (seller blog). Lab quotes: EMC €600–1,000 and LVD €1.2k–2k for simple consumer products; quotes vary 3× between labs. | vendor — https://seller.alibaba.com/blogs/2026/southeast-asia/industrial-equipment/ce-vs-ul-certification-guide-alibaba-b2b ; https://www.compliancegate.com/?p=6748 |
| KC (Korea) EMC | RMB 15k–30k (US$2.1k–4.2k) industrial; agent fee from US$400; factory inspection US$1k–2.5k; 2–3 months | vendor — https://seller.alibaba.com/blogs/2026/southeast-asia/agricultural-waste/kc-certification-korea-market-entry-guide-alibaba-b2b ; https://www.jjrlab.com/news/kc-certification-south-korea-requirements-and-costs.html |
| FCC | Wired HMIs are Part 15 unintentional radiators under **SDoC**: no FCC ID and no FCC database entry. The responsible party must be in the US. Class A applies to business/industrial marketing. | primary (eCFR 47 CFR 2.1077, 15.3) — https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-2/subpart-J/subject-group-ECFR64d0de5de218f0c/section-2.1077 ; secondary — https://www.compliancegate.com/fcc-sdoc |
| Benchmark of a Chinese ODM's certificates | Mochuan: CE (EMC 2014/30/EU certificate), RoHS, CCC, ISO 9001. No UL listed. | marketplace listing — Mochuan URL above |
| "Not UL" as a price tier | Wolf Automation lists Weintek MT8071iP as "HMI 7.0 plastic 600MHz w/ Ethernet, not UL" | marketplace listing — https://wolfautomation.com/hmi-7-0-plastic-600mhz-w-ethernet-not-ul |

**Inference (certification budget per product family of 7" + 10"):**

| Item | One-off cost |
|---|---|
| CE EMC + LVD (industrial EN 61000-6-2/-6-4 + EN 61010-2-201) | US$8–20k |
| FCC Part 15 Class A SDoC testing (often combined with CE EMC in one lab session) | US$2–5k |
| UL/cUL 61010-2-201 listing | US$15–40k, plus US$3–6k/yr follow-up inspections |
| Class I Div 2 (optional, needed for oil and gas and some North American OEMs) | US$10–30k |
| KC | US$3–6k |
| Marine (DNV/ABS) or ATEX (if pursued) | US$20–60k each |
| Production test fixtures and burn-in setup | US$5–20k |

- Core total for CE + FCC + UL + KC: about **US$30–75k**, rising to **US$60–150k** with C1D2 or marine.
- Re-certification follows any significant BOM change (display, power stage, memory supplier), which the 2026 memory shock makes more likely.

---

## 2. Teardown and cost-estimate references

- **IHS teardown of the NEC FC-P20X industrial panel PC (2010):** assumed 36k lifetime units and direct materials plus basic test. The main cost drivers were "~80% of total materials cost". The page returned 403, so this comes from the search snippet. The display share was not itemised in the snippet. [secondary] — https://electronics360.globalspec.com/article/3503/nec-fc-p20x-industrial-pc-teardown
- **Control.com teardown of an unnamed HMI (Dec 2022):** qualitative only. Main board holds processing and communications; a backlight driver PCB steps up voltage; PCAP film is bonded to glass; a plastic or metal bezel with gasket; a CMOS coin cell. No costs. [secondary] — https://control.com/technical-articles/teardown-whats-inside-a-human-machine-interface-hmi/
- **Instructables OP320 text-HMI teardown:** a low-end reference only. [secondary] — https://www.instructables.com/How-to-Open-and-See-Whats-Inside-an-HMI-OP-320-Tea
- **Avnet's illustrative embedded BOM** of "US$5 each for processor, flash, RAM" before the shock (see 1.3). [vendor]
- **No published BOM estimate for a 7"/10" PLC HMI was found.** The closest public price anchors for factory-level cost are Chinese "no-MOQ" finished goods: Mochuan 7" at US$120–160 and 10.1" at US$185–190 with a 36-month warranty, and Coolmay 7" at about US$90 on eBay (section 4). [marketplace listing]
- **Why BOM ≠ COGS:** Hackaday notes "COGS = BOM + other stuff" (labour, yield, test, freight, duty, warranty) and that the other costs are largely unrelated to the BOM. [secondary] — https://hackaday.com/2018/12/06/your-bom-is-not-your-cogs

---

## 3. Price-to-BOM rules of thumb, ODM/EMS pricing, channel margins

### 3.1 Generic hardware rules of thumb

- **Consumer hardware:** charge 2.5–4× BOM (Bolt). [secondary] — https://blog.bolt.io/?p=112
- **Retail price ≈ 4× COGS**, with a plausible range of 2.6–4× (Make, interviews with hardware entrepreneurs). [secondary] — https://makezine.com/article/technology/maker-pro-how-much-should-i-charge/
- **Floor of about 3× COGS**; the best consumer products reach 4–5× SRP/COGS (Sinovation). [vendor/VC] — https://www.sinovationventures.com/blog/consumer-iot-start-up-advice
- **Predictable Designs:**
  - Retail at ≥4× manufacturing cost to keep ~50% through the channel, which means wholesale ≈ 2× cost.
  - Distributors take 10–15%; brick-and-mortar retailers take 40–60%.
  - Target brand margin ≥40%, ideally 50–60%.
  - Subscriptions can justify selling hardware at cost.
  - [vendor/consultant] — https://predictabledesigns.com/how-to-price-your-product-for-various-distribution-channels/

### 3.2 EMS / ODM pricing structure

- **EMS gross margin, LTM Q2 2026 (Capital IQ and filings):**
  - Large tier: 9.8% gross, 5.6% operating, 7.3% EBITDA (Celestica 12.0%, Flex 9.5%, Jabil 9.2%, Sanmina 9.0%, USI 9.2%).
  - Mid tier: 10.4% / 5.4% / 7.4% (Benchmark 10.3%, Kimball 8.0%, Plexus 10.1%, Pan-International 11.6%, Fabrinet 12.0%).
  - Small tier: 25.6% gross but 2.5% operating (Scanfil 31.1%, Nortech 16.1%, Key Tronic 7.3%).
  - Source: [secondary] Lincoln International, *Q2 2026 EMS Quarterly Review*, p.5 — https://www.lincolninternational.com/wp-content/uploads/2026-Q2_EMS-Quarterly-Review_vF.pdf
  - The LTM Q2 2025 edition shows mid-tier gross margin at 8.9%. [secondary] — https://cdn.lincolninternational.com/wp-content/uploads/2025_Q2_EMS-Quarterly-Review.pdf
- **How EMS quotes are built:** labour and assembly with a labour margin, plus materials with a materials margin, plus a total margin. NRE and engineering NRE are front-loaded, so early per-unit profit is low. [secondary] — https://ventureoutsource.com/contract-manufacturing/profit-margins-pcb-electronic-assembly-quotes-ems-manufacturing-costs-oem-pricing/ ; https://ventureoutsource.com/contract-manufacturing/contract-pricing-profits-and-oem-direct-costs
- **PCBA should-cost heuristics:** labour about 8–10% of material cost; about 20% gross profit as a starting point for small builds. [vendor; source page 404, search snippet only] — https://www.calcuquote.com/blog/profit-markup-pcba-quote
- **Inference: EMS vs ODM pricing for this use case.**
  - A **pure EMS** (build-to-print, brand owns design, tooling and BSP) prices at about BOM × 1.05–1.10 for materials handling, plus labour/test of US$4–12 per unit, plus a 5–10% margin. The factory price is about **BOM × 1.15–1.25**.
  - A **white-label ODM** (owns the design and housing, sells its catalogue HMI with your logo and firmware) prices at about **BOM × 1.25–1.6**. This is the "ODM product margin" on top of EMS-like economics; Mochuan and Coolmay-type sellers are here.
  - NRE for logo, packaging and firmware is often US$0–10k, with logo MOQs of 200–1,000 pcs (see 4.1). A custom board or housing ODM project is more like US$30–150k NRE plus tooling.

### 3.3 Distributor and system-integrator margins (HMI channel)

- **Broad-line electrical distributors (primary filings):**
  - Wesco FY2025 gross margin 21.1% (21.6% in 2024). [primary] — https://wesco.gcs-web.com/news-releases/news-release-details/wesco-international-reports-fourth-quarter-and-full-year-2025
  - Rexel 2025 consensus gross margin about 25.0–25.1% of sales. This is a pre-results estimate, not an actual. [primary, company-published consensus] — https://www.rexel.com/app/uploads/2026/02/Estimates-pre-Q4-2025.pdf
- **AutomationDirect (direct-sales model):** volume discounts begin at about US$100k/yr of purchases, with negotiated discounts "potentially reaching 3%". Pricing is near-net, with no distributor layer. [vendor/low reliability, IMD blog] — https://industrialmonitordirect.com/blogs/knowledgebase/industrial-automation-component-pricing-models-markup-structures-and-procurement-strategies
- **Reseller financing float:** typically 30–120 days between paying suppliers and collecting from customers (same source). [vendor/low reliability]
- **Not found:** no published automation-distributor discount schedule for HMIs. The brief's "20–35% distributor discount" is consistent with the 21–25% gross margins of broad-line electrical distributors and the higher margins typical of specialist automation distributors that bundle support, but it is **not directly verified**. [inference]
- **Integrators and machine builders:** they generally do not resell the HMI at a margin. They embed it in a machine or project, and the margin sits in engineering hours. The control engineering press notes that a PLC or HMI is "rarely sold without additional engineering". [secondary] — https://www.controleng.com/service-offerings-expand-from-automation-distributors/

### 3.4 How a list price splits [inference, built from the above]

| Layer (per US$100 of end-customer price) | Branded Taiwan/US HMI via distributor | Chinese brand via distributor/export | Direct online (AutomationDirect-style) |
|---|---|---|---|
| Distributor / reseller margin | 20–30 | 15–25 | 0 |
| Brand gross margin (covers R&D, drivers, support, sales, marketing) | 35–50 | 20–35 | 40–55 |
| Freight, duty, warranty, royalties (inside COGS) | 3–6 | 3–6 | 3–6 |
| Factory price (ODM/EMS) | 20–35 | 40–60 | 25–40 |
| … of which BOM | 16–28 | 32–50 | 20–32 |
| … of which ODM/EMS labour, overhead and margin | 4–8 | 8–12 | 5–8 |

- Read the table as street price ≈ **3.5–6× BOM** for branded and **2–3× BOM** for Chinese brands.
- **Cross-check:** Weintek MT8072iP street US$398 vs the inferred 7" entry BOM of US$45–90 (midpoint ~65) is about 4.4–8.8×, or about 6× at the midpoint. Mochuan 7" US$150 (qty 50) vs the same BOM band is about 1.7–3.3×, or about 2.3× at the midpoint.

---

## 4. White-label / OEM HMI availability and brand sourcing

### 4.1 ODMs and white-label sources (prices, MOQs)

| Supplier | Offer | Label / URL |
|---|---|---|
| Wuxi Mochuan Drives (Jiangsu, CN) | 7" HMI (1024×600 PCAP, IP65, "free software"): US$160 (1–9), US$155 (10–29), US$153 (30–49), US$150 (50+). Made-in-China 7": US$120–185, MOQ 1. 10.1" MC-H100ED (resistive, aluminium, IP65 front, CE/CCC): US$185–190, MOQ 1. 36-month warranty on all HMIs; free "Mochuanstudio" software. | marketplace listing — https://www.alibaba.com/product-introduction/Mochuan-hmi-free-software-touch-screen_60477361034.html ; https://www.madeinchina.com/mall/show-Mochuan-10-1inch-Embedded-Touch-Screen-HMI-Panel-Monitor-for-PLC_906338.html ; https://www.madeinchina.com/mall/show-Mochuan-7-Inch-Embedded-Industrial-TFT-LCD-Touch-Screen-OEM-ODM-HMI_906337.html |
| Coolmay (Shenzhen) | Self-described "original manufacturer of OEM and ODM PLC and HMI". 7" TK6070HD ~US$90 on eBay store. Logo MOQ 200 seen on a Coolmay PLC listing (not HMI). | marketplace listing — https://coolmay.en.alibaba.com/company_profile.html ; https://www.ebay.com/str/hmiplcstore |
| Generic 7" UART smart display (DWIN-type) | US$26 (1–999), US$24 (1,000+); logo customisation MOQ 1,000 | marketplace listing — https://www.alibaba.com/product-introduction/HMI-screen-7-inch-1026x600dots-UART_62590373094.html |
| Shenzhen OEM/ODM panel maker (Portworld) | Custom logo, firmware, UI, Rockchip/MTK/Intel, Android/Linux/Windows. No MOQ or price published (Jan 2026). | vendor — https://portworld-solu.com/?p=17576 |
| Weintek via reseller (Alibaba) | Custom logo, packaging and graphics each at MOQ 500 (third-party reseller page, not Weintek terms) | marketplace listing — https://www.alibaba.com/product-introduction/WEIN-VIEW-Weintek-HMI-4-3_1600622639882.html |
| Weintek production base | "Production facilities located in Taiwan and China" | secondary (ipc2u vendor page) — https://ipc2u.com/about-us/vendors/weintek/ |
| Taiwan options | Advantech WOP-200K operator panels (4.3–15") bundled with HMINavi software (standard line, not ODM). Brainchild lists itself as an operator-panel manufacturer. | primary (Advantech press release, Nov 2024) — https://www.advantech.com/en-eu/resources/news/advantech-hminavi-innovative-open-hmi-software-for-advanced-industrial-automation ; vendor — https://www.chanchao.com.tw/automationtaipei/en/visitorProductDetail.asp?no=122006 |
| Siemens OEM programme (reference) | Customised HMIs graded as "Design products, OEM products, Turnkey products" by degree of hardware change | primary — https://mall.industry.siemens.com/mall/en/de/Catalog/Products/10175213 |
| Outlier listings | V708CD Alibaba listing US$1,000–1,500 with logo MOQ 10,000; HMIG3U at US$15. Both are implausible and should be ignored. | marketplace listing — https://www.alibaba.com/product-introduction/Fast-shipping-factory-price-hmi-touch_1600309184285.html |

**Inference: FOB price expectation for a white-label buyer at 500–2,000 pcs/yr.**
- 7": US$80–130 (resistive, plastic) or US$110–160 (PCAP, aluminium).
- 10.1": US$120–180 (resistive) or US$150–220 (PCAP, aluminium).
- Logo and boot-screen MOQ 200–1,000; custom-firmware runtime port NRE about US$5–30k.
- Add 10–25% for 2026 memory pass-through if quotes predate Q1 2026.
- This is a Chinese ODM benchmark. Taiwan ODMs (EMS-grade, UL-experienced) should be expected at +15–40%.

### 4.2 Do Western brands source from ODMs or Weintek?

- **Maple Systems (US):** Maple's FAQ, under "What is the relationship between Maple Systems and Weintek Labs?", says "Maple Systems has been the #1 authority for Weintek products in the North American market for nearly three decades." Maple also states that Weintek USA (founded 2017) is run by "different management and owners". [vendor/primary for Maple] — https://maplesystems.com/faq-category/hmis-smart-cmt/ ; https://maplesystems.com/faq/what-is-the-relationship-between-maple-systems-and-weintek-usa/
- **Maple's naming and alliance:**
  - A 2015 alliance announcement made Maple Weintek's "premier partner and technical support center for HMIs in North America". Maple calls itself the largest North American supplier of HMI products "powered by Weintek Labs". [vendor, via search summary]
  - Maple sells cMT models under Weintek-identical names (e.g., "cMT3072XH2v2", IP66, on DigiKey's marketplace). [marketplace listing] — https://www.digikey.com/en/products/detail/maple-systems-inc/cMT3072XH2v2/27803563
  - Maple's software is EBPro (EasyBuilder Pro), Weintek's tool. [vendor]
- **KEP and Maple model mapping:** a third-party blog maps Weintek MT8103iE to KEP EA3-8103L and Maple T10AH, with "hardware identical across brands" (19 Apr 2026). Low reliability; the same site contradicts itself elsewhere. [secondary/low] — https://industrialmonitordirect.com/blogs/knowledgebase/weintek-hmi-vs-kep-vs-maple-systems-oem-relationship-and-product-comparison
- **Trade-data hint for Maple:**
  - Seair shows Maple Systems (Everett, WA) imports with Taiwan as the major origin (7 shipments).
  - Panjiva profiles list origins China, Taiwan and US.
  - [marketplace/data-aggregator] — https://www.seair.co.in/us-importers/maple-systems-inc.aspx ; https://panjiva.com/Maple-Systems-Inc/33103106
- **AutomationDirect C-more EA9:** no source found naming the manufacturer. AutomationDirect's EA9 pages and spec sheets do not state one, and nothing links C-more to Weintek. **Unresolved.**
  - EA9 street prices from the AutomationDirect page: 6" base US$799, 10" widescreen US$1,096, 10" SVGA US$1,718, 12" US$1,479, 15" US$2,684. [primary price list] — https://www.automationdirect.com/c-more/ea9/hardware
  - Next step: check the FCC SDoC responsible-party text and the "Made in" label in EA9 manuals, and run C-more part numbers through Panjiva/ImportYeti. [inference]

---

## 5. Model economics and risks for a small software company taking on hardware

### 5.1 Price competition: where a new brand would sit

| Product (7" unless stated) | Street price | Label / URL |
|---|---|---|
| Weintek MT8071iP (plastic, 600 MHz, not UL) | US$345–352 new (US); ₹9,500–13,999 India | marketplace listing — https://wolfautomation.com/hmi-7-0-plastic-600mhz-w-ethernet-not-ul ; https://ddpartssolution.com/products/weintek-mt8071ip-7-inch-hmi-touchscreen-ethernet |
| Weintek MT8072iP | US$397.95 (Wolf); €230.58 ex-VAT (Unipi, EU) | marketplace listing — https://www.wolfautomation.com/products/hmi/mt8072ip/ ; https://unipi.technology/weintek-mt8072ip-p539 |
| Weintek cMT3072XP (PCAP) / cMT3072XH2 | US$843.99 / US$712.95 | marketplace listing — https://www.wolfautomation.com/products/operator-interfaces-hmi-touchscreens/cmt3072xp/ ; https://www.wolfautomation.com/products/operator-interfaces-hmi-touchscreens/cmt3072xh2/ |
| Weintek MT8102iE (10.1") | US$1,084.99 (Zoro) | marketplace listing — https://www.zoro.com/weintek-hmi-panel-101-inch-mt8102ie/i/G423561819/ |
| Kinco GL070/GL070E | US$168 @1 → US$130.94 @2,500 (Anaheim Automation); £128 UK; ₹15,000 India; ~US$200–243 on eBay | marketplace listing — https://anaheimautomation.com/KNC-HMI-G070.html |
| Kunlun Tongtai (MCGS) TPC7062 series | China B2B quotes about RMB 450–750 (US$63–106) for TPC7062KD (makepolo snippet, undated); ~US$300 on eBay export | marketplace listing (snippet) — http://b2b.makepolo.com/cpinfo/jiage/fn2266619.html ; https://www.ebay.com/itm/387327123779 |
| Samkoon SK-070ME / SK-070FE | ~US$199–261 / US$212 (eBay) | marketplace listing — https://www.ebay.com/p/28055807858 |
| Xinje TG765 | ~US$104–269 (eBay) | marketplace listing — https://www.ebay.de/itm/124414245342 |
| Coolmay TK6070HD | ~US$90 | marketplace listing — https://www.ebay.com/str/hmiplcstore |
| Mochuan 7" / 10.1" | US$120–160 / US$185–190 | marketplace listing (above) |
| NOVAKON P07/S07 | No USD price found; one EU dealer lists P07F-N at "41.853,14" (currency not shown) | marketplace listing — https://epsolutions.cust.webglobe.com/en/p-series-hmi-operator-panel-standard-hmi-panel |

**Inference:**
- **China domestic floor.** The brief's "7" HMIs at RMB 500–1,000" is plausible: Kunlun's RMB 450–750 B2B quotes are partially consistent. In China a 7" HMI sells at about US$65–140, which is roughly 1–1.5× an estimated factory cost.
- **Export.** Chinese brands sell for US$90–300.
- **Where a newcomer can fit.** A spin-off brand without UL, without 400 drivers and without field history cannot charge Weintek's US$345–400 for a 7" in the US channel. A realistic positioning is US$180–280 street for a 7" and US$300–450 for a 10.1". That leaves a gross margin of about 35–50% at the brand level after a 20–30% channel margin, if FOB is held at US$80–130 / US$130–200.

### 5.2 Fixed and semi-fixed costs (per family of 7" + 10.1") [inference]

| Item | One-off | Recurring per year |
|---|---|---|
| Certifications (CE, FCC SDoC, UL/cUL 61010-2-201, KC) | US$30–75k (US$60–150k with C1D2/marine) | US$3–10k UL follow-up and updates |
| Tooling (2 sizes; plastic or die-cast) | US$0 (ODM housing) to US$40–120k | — |
| ODM NRE (logo/firmware on existing design → custom board) | US$5–30k → US$50–150k | — |
| Runtime and BSP port and hardening (Linux, OTA, watchdog, secure boot) | 2–4 engineer-years | 0.5–1 engineer-year |
| PLC driver library (Modbus RTU/TCP plus top 10–20 native protocols) | 2–5 engineer-years | 1–2 engineer-years (Weintek claims "400+ PLC drivers") |
| RMA and repair logistics, spares | US$10–30k setup | 1–3% of hardware revenue |
| Inventory buffer (2–4 months) plus LTB stock for display and memory | 25–40% of annual COGS tied up | carrying cost 10–20%/yr of inventory value |

- Weintek "400+ PLC Drivers Support" [primary] — https://www.weintek.com/Software/EasyBuilder/EasyBuilderPro
- Weintek USA FAQ says the software "supports over 400 industrial communication protocols" [primary] — https://www.weintekusa.com/faq-category/easybuilder-pro/

### 5.3 Worked volume economics [inference]

Assumptions:
- 7" street US$240; distributor margin 25% → net revenue US$180.
- FOB US$105; landed cost (+5% freight, duty, insurance) US$110.
- Warranty reserve 2.5% of revenue → US$4.5 (benchmarks below).
- Unit gross profit ≈ US$65 (36%).

| Annual units | Hardware gross profit | Less recurring hardware overhead (UL follow-up, RMA staff, ODM management, inventory carry: ~US$150–250k) | Payback of one-off US$150–350k |
|---|---|---|---|
| 1,000 | US$65k | −US$85k to −US$185k | never on hardware alone |
| 5,000 | US$325k | +US$75k to +US$175k | 1–5 years |
| 20,000 | US$1.3M | +US$1.05M to +US$1.15M | <1 year |

- **Mix effect:** with a 10.1" at street US$380, net US$285, landed cost US$165 and unit gross profit about US$113, a 60/40 7"/10" mix raises the blended per-unit profit to about US$84.
- **Break-even:** the hardware P&L breaks even at roughly 2.5–4k units a year, before any software team cost.
- **Benchmark for scale:** NOVAKON-like scale is unknown here; the other researcher covers it.

### 5.4 Warranty and RMA benchmarks

- **Warranty Week (heavy equipment):** 2024 claims averaged 1.37% of sales and accruals 1.45%; 2023 industry-wide claims 1.17% and accruals 1.4%. [secondary] — https://warrantyweek.com/archive/ww20241031.html
- **IBM survey of 300+ companies:** electronics warranty cost median 3.0% of revenue. [secondary] — https://www.ibm.com/downloads/cas/D6QBER28
- **Chinese competitors' warranties:** Mochuan offers a 36-month warranty and a 7-day refund. A newcomer will be pushed toward 2–3-year terms. [marketplace listing]
- **Inference:** budget 2–4% of hardware revenue for warranty and RMA in years 1–3, higher during early field failures. Resistive touch wear, backlight life and electrolytic capacitors are the typical failure drivers.

### 5.5 Long-life supply and EOL

- **Toppan Display Solutions (ORTUSTECH industrial TFT)** is ending LCD production: last-time buy to end-Oct 2026, final deliveries to end-Mar 2028, with replacements via DATA MODUL. [secondary] — https://embeddedcomputing.com/application/industrial/industrial-computing/data-modul-supports-customers-as-toppan-display-solutions-ends-industrial-lcd-display-production
- **Kyocera LCD module** LED-chip change PCN: LTB 11 Sep 2027, sample approval by 15 Jun 2027. [primary] — https://www.tti.com/content/dam/ttiinc/products/PCN/Kyocera/KYOCERA-PCN-DSHKB-2607-B01.pdf
- **Memory:** legacy DDR3, early DDR4 and SLC NAND face EOL risk; Samsung extended DDR4 production to end-2026. [secondary]
- **Inference:**
  - Industrial customers expect 5–10 years of availability and form-fit-function continuity.
  - EDT's display longevity programme is an advantage only if the HMI uses EDT panels.
  - The SoC choice matters for longevity: NXP, ST and TI publish longevity programmes (10–15 years); Allwinner and Rockchip industrial variants are shorter and less formal.
  - Every forced component change risks EMC/UL re-test (section 1.5).

### 5.6 Channel conflict

- **Inference:** EDT sells display modules to OEMs, some of which build HMIs or HMI-like terminals. A branded EDT-spin-off HMI competes with those customers. Mitigations:
  - Sell finished panels mainly to end users and machine builders that do not build displays.
  - Offer "HMI Studio inside" runtime licensing to display-module customers instead of competing hardware.
  - Keep distributor territories separate.
- **Inference:** a second conflict is with distributors and integrators. If the spin-off also sells direct online (the AutomationDirect model), distributors lose the 20–30% layer and will not promote the line.

### 5.7 How free-software / hardware-revenue vendors defend margin

- **Free tool, own-hardware runtime:**
  - Weintek EasyBuilder Pro is a "Free Download", but projects run on Weintek-platform hardware. [primary] — https://www.weintek.com/Software/EasyBuilder/EasyBuilderPro
  - NOVAKON describes iFace-Designer as "FREE HMI development software", running on its ARM HMIs and x86 panel PCs. [vendor/distributor] — https://pr.mono.ipros.com/en/toamec/product/detail/2000823538 ; https://www.directindustry.com/prod/novakon-co-ltd/product-222514-2273366.html
  - Chinese ODMs (Mochuan) also bundle free software, so **free software is table stakes, not a differentiator**. [inference from marketplace listing]
- **Paid remote access and cloud attach:**
  - Weintek EasyAccess 2.0 activation card: US$103.95 (Wolf) to US$121.99 (Zoro).
  - Top-up card 10 GB/month for 1 year: US$267.75.
  - Datasheet lists 5 GB/month (RZSCEAT05) and 10 GB/month (RZSCEAT10) top-ups; Dashboard limited to 20 tags with activation.
  - [marketplace listing / primary datasheet] — https://wolfautomation.com/easyaccess-2-0-activation-card ; https://www.wolfautomation.com/easyaccess-2-0-top-up-card-10gb ; https://nnz-ipc.ru/files/documentation/weintek/EasyAccess_2.0_Datasheet_ENG_20250313.pdf
  - **Inference:** a one-time US$100–120 activation plus optional about US$270/yr top-up adds 25–35% to the revenue of a US$345–400 7" HMI at near-100% gross margin.
- **Ecosystem breadth:** Weintek sells controllers, remote I/O, gateways and built-in CODESYS on cMT. Maple pages reference "Weintek Built-in CODESYS". [vendor] — https://maplesystems.com/support-center/controller-connection-guides/maple-systems-built-in-codesys-ebpro/
- **Custom services:** NOVAKON offers custom software services and "HMI turnkey solutions". [vendor]
- **Inference: other levers for the spin-off.**
  - A runtime licence for third-party hardware, especially EDT display modules and MCU-class LVGL targets. This is where an LVGL tool is distinctive versus Linux-only HMIs.
  - Paid "Pro" features (recipes, data logging, OPC UA, MQTT/cloud connectors, user management for regulated industries).
  - Secure remote-access subscription.
  - OEM private-label runtime for machine builders.

---

## 6. Practical estimation methods

### (a) Top-down from street price

Steps:
1. Collect street prices from 3–5 channels per model (Wolf, Zoro, Unipi, Anaheim, JD, makepolo, eBay new-only).
2. Remove VAT.
3. Remove the channel margin (20–30% Western specialist distributor, 15–25% China).
4. Remove brand gross margin (assumption bands: 35–50% branded Taiwan/US, 20–35% Chinese). Cross-check against the companion financials note rather than repeating it here.
5. The result is factory price. Factory price ÷ 1.15–1.35 gives BOM.

Worked example, Weintek MT8072iP [inference]:
- Street US$398 × (1 − 0.25) = distributor cost about US$299.
- Importer/brand-level gross margin of 45–55% gives factory cost/COGS about US$135–165.
- If Weintek manufactures in-house (Taiwan and China plants per ipc2u), COGS includes its own factory overhead. BOM ≈ 70–80% of COGS ≈ **US$95–130**.
- An EU street price of €230.58 ex-VAT gives a lower implied COGS (about US$85–110).
- Against the bottom-up 7" entry BOM midpoint of about US$65 in (b), Weintek's EU street price (€230.58 ≈ US$250) is about 3.8× BOM and its US street price (US$398) about 6×. The gap between the top-down estimate (US$95–130) and the bottom-up one (US$45–90) is itself informative. Either Weintek's BOM is richer than the entry spec (likely: wider temperature range, more serial ports, isolated I/O, longer-life parts), or its brand gross margin is higher than assumed. Resolve this by triangulating with the companion financials note. [inference]

### (b) Bottom-up BOM + ODM markup

Worked example (2026 prices, 1k–5k units/yr) [inference; component anchors cited in section 1]:

| Line | 7" entry (T113-S3/STM32MP13, 256–512 MB, 4 GB eMMC, resistive, plastic) | 7" upper (AM62x/RK3566/i.MX6ULL, 1 GB, 8 GB eMMC, PCAP, aluminium front) | 10.1" entry | 10.1" upper |
|---|---|---|---|---|
| TFT panel | 14–22 | 18–30 | 30–45 | 40–65 |
| Touch | 4–7 (resistive) | 9–18 (PCAP + cover glass) | 8–14 | 15–30 |
| SoC (chip-down) | 5–8 | 8–20 | 5–8 | 8–20 |
| DRAM | 0–8 (SiP or 4Gb) | 15–28 (8Gb DDR4/LPDDR4) | 0–8 | 15–28 |
| eMMC / NAND | 5–12 | 8–20 | 5–12 | 8–20 |
| Power (24 VDC, protection, PMIC) | 2–4 | 3–5 | 2–4 | 3–6 |
| Comms (Ethernet + magnetics, 2–3 serial ports with 1 isolated RS-485, USB host, RTC) | 3–6 | 5–9 | 3–6 | 5–9 |
| PCB, passives, connectors, terminal block, coin cell | 6–10 | 8–14 | 7–11 | 9–15 |
| Enclosure, gasket, clips, label | 5–9 (plastic) | 12–22 (aluminium) | 8–14 | 18–32 |
| Packaging, manual, accessories | 1–2 | 1–3 | 1–2 | 1–3 |
| **BOM total** | **~US$45–88** (midpoint ~65) | **~US$87–169** (midpoint ~125) | **~US$69–124** (midpoint ~95) | **~US$122–228** (midpoint ~170) |
| BOM total, 2025 H1 memory prices (memory lines −US$5 to −35) | ~US$40–65 | ~US$70–130 | ~US$60–100 | ~US$100–190 |
| SMT + assembly + functional test + burn-in | 5–9 | 6–10 | 6–10 | 7–12 |
| ODM/EMS margin and overhead (8–25% of BOM + labour) | 4–20 | 8–40 | 6–30 | 10–55 |
| **Factory price (FOB)** | **~US$55–115** | **~US$100–220** | **~US$80–165** | **~US$140–295** |
| Landed (+4–8% freight, insurance, duty into TW/EU; US Section 301/IEEPA tariffs on China-origin goods not modelled) | ~US$58–125 | ~US$105–235 | ~US$85–175 | ~US$150–315 |

- **Midpoint checks:**
  - 7" entry factory price about US$80 vs Mochuan 7" US$150 at qty 50: a Mochuan gross margin of about 45%, plausible for a direct seller.
  - 10.1" entry factory price about US$120 vs Mochuan 10.1" US$185–190: about 35–37%.
- **Mark-ups to street:**
  - Branded at about 3.5–6× BOM: 7" entry US$230–390 (matches Weintek MT807x at US$345–400); 10.1" upper US$600–1,000 (matches Weintek 10" MT8102iE at about US$1,085 and C-more 10" widescreen US$1,096).
  - Chinese at 2–3× BOM: 7" US$90–260 (matches Coolmay, Xinje, Kinco, Samkoon, Mochuan).

### (c) Comparable listed companies

- Use the companion note's gross margins (Weintek-class, Chinese HMI and PLC makers, panel-PC makers) as the "brand GM" band in (a). Use EMS/ODM margins (section 3.2) for the factory layer and distributor gross margins (Wesco 21.1%, Rexel about 25%) for the channel layer.
- Do not mix vendors with in-house factories (whose gross margin includes manufacturing profit) with fabless brands that use ODMs (whose gross margin excludes it). For a fabless spin-off, a target brand gross margin of 35–45% sits about 5–10 points below an integrated maker's equivalent. [inference]

### (d) Trade data and regulatory filings

- **Customs records:** Panjiva, ImportGenius, ImportYeti and Seair show consignee/shipper pairs, HS codes (8537.10 control panels; 8471/8528 for some displays), declared weight and value. Examples: Maple Systems, origin Taiwan (Seair); origins China, Taiwan, US (Panjiva). Use them to identify the actual factory behind a brand and to estimate declared FOB per unit (value ÷ quantity). Caveat: under-declaration and mixed shipments are common. [marketplace/data-aggregator] — https://www.seair.co.in/us-importers/maple-systems-inc.aspx ; https://panjiva.com/Maple-Systems-Inc/33103106
- **FCC:**
  - Wired HMIs use SDoC, so there is **no FCC ID and no database record**. [primary eCFR]
  - FCC ID filings, with grantee, test report, internal photos and sometimes BOM-visible board photos, exist only for **Wi-Fi/cellular variants** or radio modules. For brand-to-factory mapping, check the user manual's SDoC statement and "Made in" labels instead. [inference]
- **Other certificate registries:** UL Product iQ (file number, applicant vs manufacturer, factory locations), CE certificates (e.g., Weintek's cMT3162X certificate names "Weintek Labs., Inc."), KC certificate search (lists manufacturer and factory), China CCC database. [primary where accessed]
  - Weintek CE certificate example — https://rusavtomatika.com/upload_files/documents/weintek/cMT/Certificate/cMT3162X_CE_Certificate.pdf
  - UL listing for HMIs in general (Red Lion Graphite UL 61010-2-201 example) — https://www.automation.com/en-us/articles/2014-2/red-lion-receives-ul-listing-for-graphite-series-h

### (e) Interviews

Suggested interview targets and questions [inference]:
- **Taiwan and Shenzhen ODMs (2–3 each):**
  - FOB at 500/2k/5k pcs for a 7" and 10.1" spec.
  - Logo and firmware MOQ, NRE.
  - Memory price-adjustment clause.
  - Warranty split, RMA turnaround.
  - UL experience (do they hold UL files and allow multiple listing?).
- **Automation distributors (TW, EU, US, SEA):**
  - Discount off list by tier (stocking vs non-stocking).
  - Rebates, MDF, return rights.
  - Expected first-year stocking commitment.
  - Which PLC drivers their customers actually need.
- **System integrators and machine builders:**
  - Price they pay for 7"/10" today.
  - Switching triggers (price gap, drivers, remote access, warranty).
  - Willingness to adopt a new brand without UL/C1D2.
- **EDT internal:**
  - Transfer price for LCD plus touch.
  - Longevity commitments.
  - Which EDT customers would see a branded HMI as competition.

---

## 7. Gaps and next steps

1. **No direct FOB quote** for a white-label 7"/10" Linux HMI at 1k+ pcs. Request 3–5 ODM quotes (Mochuan, Coolmay and similar Shenzhen/Jiangsu ODMs; 1–2 Taiwan ODMs).
2. **Panelook pricing** not accessed. Use EDT's internal panel costs instead.
3. **TI AM62x, STM32MP2, RK3566 and i.MX8M Mini current (2026) chip prices** not verified. The AM62x figure is from 2022.
4. **C-more EA9 manufacturer** unresolved.
5. **Chinese domestic street prices** (JD/1688/Taobao) for 7" Kunlun, Weintek China, Wecon and Flexem not captured beyond an undated makepolo snippet (RMB 450–750). A manual check on JD is needed.
6. **Certification costs** come from low-reliability seller and lab blogs. Get real quotes from a Taiwan lab (e.g., UL Taiwan, TÜV, ETC) for UL 61010-2-201 + CE + KC on a 7"/10" family.
7. **Distributor discount levels** for HMI lines are not published. Interviews are needed.
