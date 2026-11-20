# Verification of Open Price Items: SoCs/SoMs, Windows IoT, China HMI Street Prices, Boot Times, Panels

Verification date: 2026-11-20. This note checks open items from `cost_estimation_and_model.md` (this folder) and `../18-mpu-runtime/ehs_port_to_mpu.md`. It does not edit them.

**Labels** (same scheme as note 17):
- **[primary]**: a page from the maker or the maker's own store (TI, ST eStore, Variscite, Toradex, MYIR, Advantech eStore, Dell, Olimex, Panelook record), fetched directly on 2026-11-20.
- **[marketplace listing]**: one seller's price on a distributor, e-commerce or B2B listing (DigiKey, LCSC, CDW, element14, gys.cn, ymgk, Taobao via the hooos.com aggregator).
- **[snippet]**: the figure came from a search-engine summary, because the page returned 403/blank or was not fetched. Treat it as unconfirmed.
- **[secondary]**: trade press or a third-party blog.
- **[inference]**: my own conversion or judgement.

**Result codes:** confirmed / corrected / new / still unverified.

**Currency assumptions [inference]:** USD 1 ≈ RMB 7.1 (as in note 17). CAD 1 ≈ USD 0.72.

**Access problems this round:**
- HTTP 403: Toradex, Variscite shop, CDW, Microsoft devicepartner, manualslib.
- DigiKey product page reports "no longer available", with no prices.
- LCSC, TI.com and ST eStore pages render without prices unless the second half of the page is fetched. ST eStore worked with an offset.
- JD.com, 1688 and Taobao were not reached directly. Taobao listings were read through the tao.hooos.com aggregator, which shows no listing dates and mixes new, used, "议价/询价" (price on request) and placeholder prices.
- Search results are US-indexed, so Chinese pages were thin.

---

## Summary

| # | Item | Result | Headline figure |
|---|---|---|---|
| 1a | TI AM625/AM623 chip | **corrected** | "< US$5 @1k" was 2022 launch marketing for the entry part. Quad AM6254 today: DigiKey ~US$29.02 @1 → ~US$19.54 @357 [snippet]. TI.com shows no price. |
| 1b | AM62x SoMs | **confirmed (dated)** | Variscite VAR-SOM-AM62 from US$33 (2023 launch). Toradex Verdin AM62 "from US$31.90" at volume [snippet]. MYIR MYC-YM62X US$29 (2023 launch). Forlinx: contact for price. |
| 1c | STM32MP2 chips | **new / corrected** | ST eStore, fetched: MP257CAI3 US$29.39 @1 → US$20.66 @360–500. MP215FAM3 US$20.38 @1 → US$12.91 @500. MP215DAL3 US$10.79 @10k. Note 18's "MP215 from ~US$15.6" is corrected to US$10.79–12.12. |
| 1d | STM32MP2 SoM | **new** | MYIR MYC-LD257 (1 GB/8 GB) US$43.80 @1 on DigiKey [snippet]; the listing is now marked no longer available. Dev board US$110. |
| 1e | STM32MP13/15 | **corrected / new** | MP135FAG7 on ST eStore is now US$10.81 @1 → US$6.61 @500, with 500+ "contact sales" (note 17 had US$5.84 @1,040–10k). MYIR MYC-YF13X US$19 (launch, Jun 2023). MP157 SoMs €29.95 (Olimex) and about US$29–44 elsewhere. |
| 1f | RK3566 | **new** | Chip on LCSC ~US$15.5 @1 → ~US$10.7 @102+ [snippet]. SoMs: Geniatech OSM US$58, Waveshare Core3566 €40.45 ex-VAT / US$77.99, LuckFox from US$23.99 (older). |
| 1g | Allwinner T527 SoMs | **confirmed** (note 18 estimate of US$40–60) | MYIR MYC-LT527 (2 GB/16 GB, -E) US$43.00 @1 on DigiKey [snippet]; OzRobotics US$40–89. |
| 1h | NXP i.MX93 SoMs | **confirmed** | Variscite VAR-SOM-MX93 from US$39. DART-MX93 US$39 at launch. MYIR MYC-LMX9X US$43 (1 GB) / US$49 (2 GB) at the May 2024 launch, a sample price for the first 2 pcs. |
| 2 | Windows 11 IoT Ent. LTSC 2024 | **corrected** | Note 18's "~US$88 Entry" is a CDW **Canada** price, C$87.99 (≈ US$63). Dell sells an LTSC 2024 Entry upgrade at US$64.99. Advantech US lists Windows 10 2021 LTSC Entry/Value/High End at US$60.62/130.62/240.62; its LTSC 2024 Value is "call for price". Base (Arm64) tier exists since Dec 2023; no public price. |
| 3 | China 7" HMI street prices | **new / partly confirmed** | TPC7062Ti ¥600–919 for new units (¥680 ymgk; ¥760 gongboshi [snippet]). Weintek China TK6071iQ ¥399–618; MT6071iP ¥400–450 by quantity. Wecon PI3070ie ~¥220–450. Flexem FE6070 ¥200–525. No dates shown. |
| 4 | HMI boot times | **still unverified** (no primary) | Secondary blog only: Weintek MT8071iE 11.9 s; Red Lion G3 ~7 s; PanelView 800 32 s; PanelView Plus 6 ~67 s. No figures found for cMT, DOP-100, NOVAKON P07, Unified Comfort, Kinco or MCGS. |
| 5 | Panel prices | **new** | Taobao: AUO G101EVN01.0 10.1" mostly ¥168–200 (~US$24–28); AUO G070VW01 7" ¥157–180 (~US$22–25), with "1-year warranty" listings at ¥446–550. Panelook shows no price, and lists G101EVN01.0 as "Discontinued". |

---

## 1. SoC and SoM prices

### 1a. TI AM62x chip

| Evidence | Figure | Date | Label / URL |
|---|---|---|---|
| TI part page AM6254ATCGHAALW | Price table empty. The page says "Out of stock / Not available" for direct TI orders, and the part is active. | fetched 2026-11-20 | [primary] https://www.ti.com/product/AM625/part-details/AM6254ATCGHAALW |
| TI launch (via CNX) | "starting at less than US$5 in 1,000-unit quantities"; TI web sample price US$19.50 | 2022-06-07 | [secondary] https://www.cnx-software.com/2022/06/07/ti-sitara-am623-am625-cortex-a53-socs-offer-low-power-ai-for-hmi-and-iot-applications/ |
| DigiKey AM6254ATCGHAALW (quad, 1.4 GHz) | ~US$29.02 @1 → ~US$19.54 @357 (tray) | undated snapshot | [marketplace listing, snippet] https://www.digikey.com/en/products/detail/texas-instruments/AM6254ATCGHAALW/19236195 |
| LCSC C5218241 (AM6254ATCGHAALW) | ~US$34.15 @1, US$32.35 @30+, 8 pcs in stock | undated | [marketplace listing, snippet] https://www.lcsc.com/product-detail/C5218241.html |
| Mouser AM6254ATCGGAALW / AM6252ATCGHAALW | US$43.46 @1 / US$26.00 @1 | undated | [marketplace listing, snippet] https://www.mouser.ca/c/semiconductors/integrated-circuits-ics/embedded-processors-controllers/microprocessors-mpu/?pg=12 |
| Octopart / Greelly (broker) | ~US$20.31 / US$23.95 @1 | undated | [marketplace listing, snippet] https://octopart.com/part/texas-instruments/AM6254ATCGHAALW |

**Result: corrected.**
- The "< US$5 @1k" figure applies only to the entry single-core AM623/AM625 at 2022 launch pricing.
- For a quad-core AM6254, budget about **US$18–22 at a few hundred units** from Western distributors [inference from DigiKey's @357 tier].
- No price was found for the AM6231/AM6232. The TI.com 1ku price could not be read.

### 1b. AM62x SoMs

| SoM | Figure | Date | Label / URL |
|---|---|---|---|
| Variscite VAR-SOM-AM62 | "starting from only US$33" (production quantities) | Feb 2023 launch. Eval kit now "from US$299" (was US$249 in 2025). | [primary, launch PR] https://variscite.com/newsroom/a-new-cost-effective-som-based-on-the-texas-instruments-am62x-processor-starting-from-only-33/ ; shop 403 |
| Toradex Verdin AM62 | "Volume price starting from US$31.90" | current page [snippet] | [primary, snippet] https://toradex.com/computer-on-modules/verdin-arm-family/ti-am62 (403 on fetch) |
| Verdin AM62 single units | Solo 512 MB (00711200) €59.38 ex-VAT at Farnell IE; Dual 1 GB €122.73 at DigiKey FR; Quad 2 GB ~US$309.63 @1 at Future | undated | [marketplace listing, snippet] https://ie.farnell.com/toradex/00711200/com-1ghz-512mb-arm-cortex-a53/dp/4759327 |
| MYIR MYC-YM62X | MYC-YM6231-8E1D-100-I (1 GB/8 GB) US$29 | Aug 2023 launch | [secondary] https://www.cnx-software.com/2023/08/16/ultra-compact-45x43mm-cpu-module-features-ti-sitara-am6231-am6252-or-am6254-arm-soc/ |
| Forlinx FET6254-C (quad, 2 GB/8 GB) | "Contact us for price"; OK6254-C SBC US$178 on DigiKey | current | [primary] https://www.forlinx.net/product/am625x-system-on-module-127.html |

**Result: confirmed as launch/volume list prices (2023).**
- No 2026 repricing notice was found from Variscite, Toradex, MYIR or Forlinx.
- Given the 2026 memory shock (note 17 §1.3), treat the US$29–33 "from" prices as pre-shock floors for the smallest-memory variant [inference].

### 1c. STM32MP2 chips (ST eStore, fetched 2026-11-20)

| Part | Tiers (USD) | Stock | Label / URL |
|---|---|---|---|
| STM32MP257CAI3 | 1–9: 29.39; 10–24: 24.28; 25–99: 22.81; 100–249: 21.57; 250–359: 21.07; 360–500: **20.66**; 500+: contact sales | In stock | [primary] https://estore.st.com/en/stm32mp257cai3-cpn.html |
| STM32MP215FAM3 | 1–9: 20.38; 10–24: 16.19; 25–99: 14.51; 100–249: 13.99; 250–499: 13.86; 500: **12.91**; 500+: contact sales | In stock | [primary] https://estore.st.com/en/stm32mp215fam3-cpn.html |
| STM32MP215 family list | DAL3 **US$10.79 @10k** (in stock); DAM3 US$12.12 @500 (in stock); AAO3 15.19, AAL3 16.71, AAN3 16.92, AAM3 18.01, CAO3 16.49, CAL3 18.01, CAN3 18.23, CAM3 18.65 (all @500, out of stock) | mixed | [primary] https://estore.st.com/en/products/microcontrollers-microprocessors/stm32-arm-cortex-mpus/stm32mp2-series/stm32mp215.html |
| STM32MP235FAK3 | US$12.39 @500 (in stock); MP235DAJ3 US$11.73 (out of stock) | snapshot | [primary, snippet] https://estore.st.com/en/products/microcontrollers-microprocessors/stm32-arm-cortex-mpus/stm32mp2-series/stm32mp235.html |
| STM32MP235FAL3 (Future Electronics) | US$11.44 @1,008+ | undated | [marketplace listing, snippet] https://www.futureelectronics.com/p/semiconductors--microprocessors--digital-signal-processors/stm32mp235fal3-stmicroelectronics-2205124 |
| STM32MP231 | US$18.19–20.21 @500 (mostly out of stock). This is higher than the MP235, which is odd; the part mapping may be wrong. | snapshot | [primary, snippet] https://estore.st.com/en/products/microcontrollers-microprocessors/stm32-arm-cortex-mpus/stm32mp2-series/stm32mp231.html |
| STM32MP257 at distributors | DigiKey FR FAI3 €17.23 @360; DigiKey UK FAL3 £12.49 @100 | undated | [marketplace listing, snippet] https://www.digikey.co.uk/en/products/detail/stmicroelectronics/STM32MP257FAL3/24883089 |

**Result: new.** Note 18 said "chip from ~US$15.6 for MP215"; that is **corrected** to **US$10.79 @10k (DAL3) / US$12.12–12.91 @500**. No 1k price is published: the ST eStore stops at 500, then "contact sales".

### 1d. STM32MP2 SoMs

| SoM | Figure | Label / URL |
|---|---|---|
| MYIR MYC-LD257-8E1D-150-I (1 GB/8 GB) | US$43.80 @1 (DigiKey US); £32.72 UK; €37.57 ES. The US page now says "This product is no longer available at DigiKey" (fetched 2026-11-20). | [marketplace listing, snippet] https://www.digikey.com/en/products/detail/myir-tech-limited/MYC-LD257-8E1D-150-I/25928195 |
| MYC-LD257-8E2D-150-I (2 GB/8 GB) | €42.72 (DigiKey ES), AUD 75.70 | [marketplace listing, snippet] https://www.digikey.es/en/products/detail/myir-tech-limited/MYC-LD257-8E2D-150-I/25928197 |
| MYD-LD25X dev board | US$110.00 @1 (DigiKey MX) | [marketplace listing, snippet] https://www.digikey.com.mx/en/products/detail/myir-tech-limited/MYD-LD257-8E1D-150-I/25928196 |

**Result: new.** Note 18 estimated US$60–100 for an MP2 SoM. **Corrected** to about **US$44–50** for the MYIR MP257 SoM at quantity 1 (pre-2026 listing). No ST-partner (e.g. Ka-Ro, DH, Octavo) MP2 SoM price was found.

### 1e. STM32MP13 / MP15

| Item | Figure | Label / URL |
|---|---|---|
| STM32MP135FAG7 (ST eStore, fetched) | 1–9: 10.81; 10–99: 9.25; 100–249: 7.07; 250–499: 6.82; 500: **6.61**; 500+: contact sales; in stock | [primary] https://estore.st.com/en/stm32mp135fag7-cpn.html |
| Note 17 earlier figure | US$10.31 @1–9, US$5.84 @1,040–10k | The tiers have changed. The 1k–10k tier is gone, and @1 is ~5% higher. **corrected** |
| MYIR MYC-YF13X (MP135, 256 MB/256 MB NAND) | US$19 per unit, "volume discounts available" | [primary, MYIR news 2023-06-12] https://www.myirtech.com/news_list.asp?id=870 |
| Same, DigiKey FI | €16.41 ex-VAT | [marketplace listing, snippet] https://www.digikey.fi/en/products/detail/myir-tech-limited/MYC-YF135-256N256D-100-I/20412042 |
| Olimex STMP157-SOM / BASE-SOM-EXT | €29.95 @1 → €27.55 @1,000+ | [primary, snippet] https://olimex.com/Products/SOM/STMP1/STMP157-BASE-SOM-EXT/ |
| MYIR MYC-YA157C | US$29 (2020 launch); DigiKey UK £29.92 | [secondary] https://embeddedcomputing.com/news/myir-introduces-29-arm-som-powered-by-st-stm32mp1 ; [snippet] https://www.digikey.co.uk/en/products/detail/myir-tech-limited/MYC-YA157C-V3-4E512D-65-I/16680834 |
| Octavo OSD32MP157F-512M-EAA (SiP) | £43.65 @84 (tray), 18-week lead time | [marketplace listing, snippet] https://www.digikey.co.uk/en/products/detail/octavo-systems-llc/OSD32MP157F-512M-EAA/16272822 |
| Seeed SoM STM32MP157C | €70.20 @1 (RS IE) | [marketplace listing, snippet] https://ie.rs-online.com/web/p/microcontroller-development-tools/2011718 |

**Result:** Note 18 estimated US$30–60 for MP13/MP15 SoMs. **Corrected** to **US$17–20 (MP13)** and **US$29–45 (MP15)** at quantity 1.

### 1f. Rockchip RK3566

| Item | Figure | Label / URL |
|---|---|---|
| RK3566 chip, LCSC C2943786 | ~US$15.52–15.64 @1; ~US$10.66–10.73 @102+ (tray of 102). Other snapshots show US$9.93–10.87 @1. | [marketplace listing, snippet; the page rendered without prices on 2026-11-20] https://www.lcsc.com/product-detail/C2943786.html |
| Geniatech SoM-3566-OSM | US$58 @1+, 1–8 GB LPDDR4 options | [primary, snippet] (via search) |
| Waveshare / LuckFox Core3566 (CM4-compatible) | Waveshare store US$77.99 (out of stock); SOS Solutions NL €40.45 ex-VAT; LuckFox "from US$23.99" (older CNX) | [marketplace listing, snippet] https://www.waveshare.com/core3566.htm ; https://www.sossolutions.nl/core3566-module-rockchip-rk3566-quad-core-processor-compatible-with-raspberry-pi-cm4-options-for-ram-emmc-wireless |
| LCSC LCKFB-TSPI RK3566 board (2 GB/16 GB) | US$68.76 @1 | [marketplace listing, snippet] https://www.lcsc.com/product-detail/C19273641.html |

**Result: new.** Chip about US$10–11 at tray quantity. Industrial or OSM SoMs cost about US$45–60, and consumer CM4-style modules about US$24–78 depending on RAM.

### 1g. Allwinner T527

| Item | Figure | Label / URL |
|---|---|---|
| MYIR MYC-LT527M-16E2D-180-E (2 GB/16 GB, extended temp) | US$43.00 @1 (DigiKey US); industrial -I-G €116.44 (DigiKey IT) | [marketplace listing, snippet] https://www.digikey.com.au/en/products/detail/myir-tech-limited/MYC-LT527M-16E2D-180-E/22528182 |
| MYC-LT527 at OzRobotics | US$40–89 by configuration (2/4 GB, 16/32 GB, NPU or not, temperature grade) | [marketplace listing, snippet] https://ozrobotics.com/shop/system-on-module-som-based-on-allwinner-t527-processor-myir-myc-lt527/ |
| Forlinx FET527N-C / FET527N-UP4 | No price published | [primary] https://www.forlinx.net/product/t527-c-system-on-module-149.html |
| EBYTE ECB32-PBT4-N SBC (T527M) | ~US$81.99; T527MN variant ~US$89.79 (AliExpress) | [secondary, CNX 2025-10-16] https://www.cnx-software.com/2025/10/16/raspberry-pi-allwinner-a527-t527-industrial-sbc-dual-camera-ai-acceleration/ |

**Result: confirmed.** Note 18's estimate of US$40–60 fits for the 2 GB/16 GB SoM at quantity 1.

### 1h. NXP i.MX93 SoMs

| SoM | Figure | Label / URL |
|---|---|---|
| Variscite VAR-SOM-MX93 | "starting from US$39"; eval kits from US$299 | [primary, shop snippet + 2023 PR] https://shop.variscite.com/product/evaluation-kit/var-som-mx93-evaluation-kits ; https://www.businesswire.com/news/home/20230313005752/en |
| Variscite DART-MX93 | US$39 at announcement (alpha customers, production quantities) | [secondary] https://hackster.io/news/variscite-expands-the-dart-pin2pin-system-on-module-range-with-the-edge-ai-capable-dart-mx93-d39984a2d1a4 |
| MYIR MYC-LMX9X | US$43 (1 GB) / US$49 (2 GB) at the May 2024 launch. MYIR says sample price applies to the first 2 pcs only. DigiKey AU 2 GB AUD 75.50. | [secondary] https://linuxgizmos.com/myc-lmx9x-myirs-low-cost-som-with-integrated-npu-for-industrial-iot/ ; [primary] https://en.myir.cn/IMX93/57.html |

**Result: confirmed** at US$39–49. The US$59 upper bound in note 18 was not re-found. Forlinx FET-MX9352 and Toradex Verdin iMX93 prices were not found.

### 1-bis. Revised SoM table for note 18 [inference]

| Platform | Chip @ hundreds of units | SoM @1 (list, mostly pre-2026) |
|---|---|---|
| STM32MP13 | US$6.6–7 | US$17–20 |
| STM32MP15 | — | US$29–45 |
| STM32MP21 | US$10.8–12.9 | none found |
| STM32MP23 | ~US$11.4–12.4 | none found |
| STM32MP25 | US$20.7 (@360–500) | ~US$44 (MYIR 1 GB) |
| TI AM62x (quad) | ~US$19.5 (@357) | US$29–33 "from" (smallest variant, volume); single units €59–€310 |
| RK3566 | ~US$10.7 (@102) | US$24–78 (consumer CM), US$58 (OSM industrial) |
| T527 | — | US$40–89 |
| i.MX93 | — | US$39–49 |

All SoM "from" prices predate the 2026 DRAM/eMMC price rises. Expect +US$5–30 on 1–2 GB/8–16 GB configurations if repriced (note 17 §1.3) [inference].

---

## 2. Windows 11 IoT Enterprise LTSC 2024 pricing

| Evidence | Figure | Date | Label / URL |
|---|---|---|---|
| CDW Canada, Win 11 IoT Ent **2024 LTSC Entry**, OEI via PKEA | **C$87.99** (≈ US$63 [inference]) | current listing | [marketplace listing, snippet; 403 on fetch] https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-entry-license-1-license/7983139 |
| CDW Canada, **2024 LTSC Value** | **C$131.99** (≈ US$95) | current | [snippet] https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-value-license-1-license/7983137 |
| CDW Canada, **2024 LTSC High End** | **C$237.99** (≈ US$171) | current | [snippet] https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-high-end-license-1-license/7983138 |
| CDW US, "Win 11 IoT Enterprise LTSC 2024, 1 license" (tier not stated) | US$202.99 | current | [snippet] https://www.cdw.com/product/windows-11-iot-enterprise-ltsc-2024-license-1-license/8025005 |
| Dell 634-CZBG, "Windows 11 IoT Enterprise LTSC 2024 Entry upgrade solution for Dell thin clients" | **US$64.99** | fetched 2026-11-20 | [primary] https://www.dell.com/en-us/shop/windows-11-iot-enterprise-ltsc-2024-entry-upgrade-solution-for-dell-thin-clients/apd/634-czbg/software |
| Advantech US eStore, Win 11 IoT Ent (GAC, **non-LTSC**) | Entry US$38.29 (EPKEA 37.99); Value US$80.98 (80.67); High End US$148.05 (147.74) | fetched 2026-11-20 | [primary distributor] https://buy.advantech.com/Software/Microsoft-Windows-Embedded-Windows-11-IoT/AUS_32898.products.htm |
| Advantech US eStore, **Win 10 IoT Ent 2021 LTSC** | Entry US$60.62 (EPKEA 60.44); Value US$130.62; High End US$240.62 (EPKEA 240.44) | fetched 2026-11-20 | [primary distributor] https://buyemt.advantech.com/Peripherals-Modules/Embedded-Peripherals-Microsoft-Windows-Embedded-Windows-10-IoT-Licenses/EMT_30970.products.htm |
| Advantech EU, "Win 11 IoT Ent Value LTSC 2024 EPKEA" 968TW1124VL0XE, MS PN EP2-07324 (Core i3–i5 / Ryzen 3–5) | "Call for Price"; "You need Microsoft CLA to buy this product" | fetched 2026-11-20 | [primary distributor] https://buy.advantech.eu/Microsoft-Software/Windows-11-IoT-Enterprise/model-968TW1124VL0XE.htm |
| Microsoft device-partner price-list update: "Windows 11 IoT Enterprise LTSC **Base** for Arm64" added, effective 1 Dec 2023 | no price shown | 2023 | [primary, snippet; 403] https://devicepartner.microsoft.com/en-id/communications/comm-windows-iot-enterprise-non-oa-price-list-update-summary-communication |
| Arrow FAQ / LTSC 2024 datasheet (Sep 2026): tiers Base, Entry, Value, High-end. Base covers "i.MX 8/93 class" Arm; Base Arm64 LTSC 2024 ESD OEI SKUs end in 2034. | no price | 2026-09 | [secondary distributor, snippet; PDF timed out] https://www.arrow.com/ais/msembedded/resources/windows-iot-enterprise-ltsc-frequently-asked-questions-faqs ; https://www.arrow.com/ais/msembedded/wp-content/uploads/sites/3/2026/09/Win11_IoT_Ent_LTSC-2024-Datasheet_Sep-24.pdf |
| Avnet | "fill in our form… we will send an offer based on… tier level" (no public price) | current | [snippet] https://www.avnet.com/msembedded/embedded-software/windows-11-iot-enterprise-2024 |
| o365hq blog "US$295 per device" | unverified | — | [secondary, low reliability] https://o365hq.com/blog/windows-enterprise-ltsc-vs-n-ltsc-vs-iot-enterprise-ltsc-which-edition-who-can-buy-it-and-what-it-costs |

No price was found at DigiKey, Mouser or Seco.

**Result: corrected.**
- Note 18 said "about US$88 (Entry), US$203 (standard)". The US$88 is actually **C$87.99 at CDW Canada**, about US$63. The US$203 is CDW US with no tier named.
- Single-unit reseller prices for **LTSC Entry are about US$60–65** (Dell US$64.99; Advantech's Win 10 LTSC Entry US$60.62; CDW CA C$87.99).
- Value is about **US$95–131** and High End about **US$170–241**.
- The non-LTSC (GAC) Windows 11 IoT Enterprise is cheaper at Advantech: US$38 / 81 / 148.
- OEM prices at volume from Arrow and Avnet are quote-only and were not found.
- **Base (Arm64)** exists as a tier, but no public price was found. It sits below Entry by definition [inference].
- **Suggested revision for note 18 §3.6 [inference]:** Windows adds about **US$40–65 (GAC/LTSC Entry, Intel N-series/Atom)** or **US$80–130 (Value, Core i3–i5)** per unit. Drop the US$295 figure.

---

## 3. China domestic street prices for 7" HMIs

All prices are RMB, single unit unless stated. None of the pages show a listing date; pages were read on 2026-11-20. Aggregator rows marked "议价/询价" may be placeholders. Rows labelled 可维修/成色/拆机 (repairable / condition-graded / dismantled) were treated as used and excluded.

### Kunlun Tongtai (MCGS) TPC7062 series

| Model | Price | Seller | Label / URL |
|---|---|---|---|
| TPC7062Ti / TX / KX (title) | **¥680**, 500 in stock, ~7-day delivery | 上海诺丞仪器 on 易卖工控网 | [marketplace listing] https://m.ymgk.com/detail-1521685.html |
| TPC7062Ti | ¥760 | 工博士 (gongboshi) | [marketplace listing, snippet; TLS error on fetch] http://www.kunluntongtai.gongboshi.com/ |
| TPC7062Ti | ¥744 incl. 13% VAT (¥658.41 ex-VAT) per search snippet. On fetch the item was "暂未上架" (not listed); a related card shows ¥950. | 鑫方盛 xfs.com | [marketplace listing, snippet / conflicting] https://www.xfs.com/productspu/153358.html |
| TPC7062Ti | ¥919 (coupon ¥954) | 顺鑫厂家直销店 (Taobao) | [marketplace listing via aggregator] https://tao.hooos.com/search?w=TPC7062Ti |
| TPC7062Ti (multi-model "原装正品") | ¥770 | 奥魅尔旗舰店 (Taobao) | same |
| TPC7062TI / 7072Gt | ¥852 | 方禾自动化工控 (Taobao) | same |
| TPC7062TX(KX)/TD(KT) | ¥600 | 广州工控自动化企业店 (Taobao) | same |
| TPC7062ti | ¥550 | supplier on gys.cn | [marketplace listing, snippet] https://m.gys.cn/renjijiemian/5212575586.html |
| TPC7062KX | Most Taobao rows are used or repairable at ¥92–488. 谷瀑 lists TPC7062KX/TX/TD at ¥500. | various | [marketplace listing, snippet] https://tao.hooos.com/search?w=TPC7062KX ; https://namexian.cn.goepe.com/apollo/prodetail-8632645.html |
| TPC7062KD | ¥450–750 (makepolo, from note 17) | — | **still unverified** (snippet only, undated) |

**Result:**
- A new TPC7062Ti from regular channels costs about **¥600–920 (≈ US$85–130)**, with a central value of about ¥680–770.
- The KX is mostly sold used or as a repair item. It appears to have been superseded by the TX/Ti [inference].
- Note 17's "RMB 450–750" is **confirmed in range** but sits slightly low for the Ti.

### Weintek China (威纶通) TK/MT

| Model | Price | Seller | Label / URL |
|---|---|---|---|
| MT6071iP / MT6071iE | **¥450 @1, ¥430 @2, ¥400 @3+** | 浙江荣尚电气设备有限公司 (gys.cn) | [marketplace listing, fetched] https://m.gys.cn/renjijiemian/5105136082.html |
| MT6071iP | ¥550–580 (other gys.cn sellers) | various | [marketplace listing, snippet] https://m.gys.cn/plc/4998247566.html |
| TK6071iQ | ¥399–466 by quantity; "reference ¥466.48, final price per contract" | gys.cn / chem17 sellers | [marketplace listing, snippet] https://m.gys.cn/renjijiemian/4963633034.html (503 on fetch) ; https://www.chem17.com/st421105/product_33238554.html |
| TK6071iQ | ¥579.99 / ¥600.57 | 亿有自动化 (Taobao) | [marketplace listing via aggregator] https://tao.hooos.com/search?w=TK6071IQ |
| TK6071iQ (multi-model title) | ¥617.5 (coupon; list ¥647.5) | 水榭听可工厂店 (Taobao) | same |
| TK6071iQ (US/export, for contrast) | US$360 (ddpartssolution); US$450 (Made-in-China) | — | [marketplace listing, snippet] https://ddpartssolution.com/products/weintek-tk6071iq-7-inch-hmi-with-ethernet |

**Result: new.** Weintek's China-market 7" TK/MT iP/iQ models sell for about **¥400–620 (≈ US$56–87)**. That is 4–6× below US street prices for the global MT8071iP (US$345–352, note 17).

### Wecon (维控) and Flexem (繁易)

| Model | Price | Seller | Label / URL |
|---|---|---|---|
| Wecon PI3070ie | ¥448 (吉祥自动化设备), ¥390 (聚鑫自动化, PI3070i/ie), ¥249–277 (several "议价"); outliers ¥996–1,222 on generic "小店" listings | Taobao | [marketplace listing via aggregator] https://tao.hooos.com/search?w=PI3070ie |
| Wecon PI3070ie (India) | ₹7,500–9,000 | IndiaMART sellers | [marketplace listing, snippet] https://m.indiamart.com/hrtechnologies/wecon-hmi.html |
| Flexem FE6070C/WE/H "全新" | ¥200 | 奥魅尔旗舰店 (Taobao) | [marketplace listing via aggregator] https://tao.hooos.com/search?w=FE6070 |
| Flexem FE6070WE | ¥337.3 (品设旗舰店); ¥372–373 (汾阳工控, 强哥工控) | Taobao | same |
| Flexem FE6070C | ¥524.95 (亿有自动化); ¥1,365 (方禾自动化) | Taobao | same |

**Result: new, low reliability.**
- Wecon PI3070ie: about **¥250–450 (≈ US$35–63)**.
- Flexem FE6070: about **¥200–525 (≈ US$28–74)**.
- No JD.com or 1688 prices were obtained.

---

## 4. Measured boot times (power-on to first usable screen)

| HMI | Figure | Source | Label |
|---|---|---|---|
| Weintek (Weinview) MT8071iE | 11.9 s | industrialmonitordirect.com, "Micro820 HMI Boot Times…", 2026-03-12. No method or source given. | [secondary, low reliability] https://industrialmonitordirect.com/blogs/knowledgebase/micro820-hmi-boot-times-panelview-vs-red-lion-vs-maple-vs-weinview |
| Red Lion G3 (G304K2) / Kadet / CR1000 / CR3000 | ~7 s / ~9 s / ~20 s / 20–25 s | same | [secondary, low reliability] |
| Rockwell PanelView 800 2711R-T4T / PV Component 2711C-T4T / 2711C-T7T | 32.1 s / 71 s / 24 s | same. A plctalk user calls the T4T's ">1 min" boot "horrible". | [secondary] + forum https://www.plctalk.net/threads/hmi-recommendations-for-micro820-controller.119812/ |
| PanelView Plus 6 (2711P-T10C4D8) | ~40 s to Windows CE, ~67 s to the main screen. Another IMD page says 90–160 s. | industrialmonitordirect.com | [secondary, low reliability] https://industrialmonitordirect.com/blogs/knowledgebase/panelview-plus-boot-time-performance-analysis |
| AutomationDirect Productivity EA9 | 12–25 s | IMD (search snippet) | [secondary, snippet] |
| Delta DOP (series not specified) | "Delta logo, then project screen within 30–60 s" | IMD firmware-update article | [secondary, low reliability] https://industrialmonitordirect.com/blogs/knowledgebase/delta-dopsoft-hmi-firmware-update-dopsoft-20007-procedure |
| MCGS TPC7062KS | "expected main screen within 10–30 s" (troubleshooting anecdote) | IMD | [secondary, low reliability] https://industrialmonitordirect.com/blogs/knowledgebase/troubleshooting-mcgstpc-tpc7062ks-hmi-startup-delay-with-mitsubishi-fx2n-plc |
| Siemens Unified Comfort (MTP700…) | No boot-time figure. Runtime auto-start is configurable: immediately, after a 1–60 s delay, or manual. A "50 s" value in the MTP700 Hygienic manual (p. 62) is not labelled as boot time; the page returned 403. | Siemens manuals | [primary, partial] https://support.industry.siemens.com/cs/attachments/109773257/hmi_unified_comfort_panels_operating_instructions_enUS_en-US.pdf ; manualslib p.62 (403) |
| Weintek cMT / cMT X | No figure. Installation sheets say only that if the display does not come on within 5 s of power-up, remove power. That is a display check, not time to a usable screen. | Weintek installation instructions | [primary, not a boot spec] https://nnz-ipc.ru/files/documentation/weintek/cmt3072x2_xh2_installation.pdf |
| NOVAKON P07, Kinco, Delta DOP-100 | Nothing found in datasheets, manuals, forums or videos | — | still unverified |

**Result: still unverified.**
- No maker publishes a boot-time spec for these HMIs.
- The only numbers come from a single SEO-style knowledge-base site (industrialmonitordirect.com) that cites no method. Its pages also contradict each other on PanelView Plus.
- Order of magnitude [inference]: Linux/RTOS-class HMIs (Weintek MT, Red Lion G3/Kadet, MCGS) boot in about **7–30 s**; Windows CE/Embedded-class (PanelView Plus) in about **40–160 s**.
- That supports note 18's target of a first screen in under 8 s (stretch under 3 s) as a credible differentiator.
- To get publishable figures, time the boot of purchased units: Weintek cMT3072XH2, Delta DOP-107, MCGS TPC7062Ti, Kinco GL070E, NOVAKON P07. Use a phone video from power-on to the first touch-responsive screen, with 3 cold boots each.

---

## 5. Panel prices (7" and 10.1" industrial TFT)

| Panel | Price | Seller / note | Label / URL |
|---|---|---|---|
| AUO G101EVN01.0 (10.1", 1280×800, industrial) on Panelook | No price shown, and none visible only to members. "Production State: Discontinued" (MP 2005 Q2); 0 stocks, 0 suppliers; record updated 2026-06-16. | Panelook record | [primary database] https://www.panelook.com/G101EVN01.0_AUO_10.1_LCM_overview_21917.html |
| AUO G101EVN01.0 (and .1/.2/.3 variants) on Taobao | Most listings **¥168–200** (滢新 ¥170, 晶彩世界LCD ¥180, 特丽科技 ¥199, 工控液晶屏之家 ¥200 "new, 1-year warranty"); range ¥100–380 | Taobao via aggregator, undated | [marketplace listing] https://tao.hooos.com/search?w=G101EVN01.0 |
| AUO G070VW01 V0/V1 (7", 800×480, industrial) on Taobao | Cluster **¥157–180** (165, 175, 178, 180); "1-year warranty / original" listings ¥446–550; G070VW01V1 at ¥490–707 | same | [marketplace listing] https://tao.hooos.com/search?w=G070VW01 |
| TrendForce 2026 | TV panel prices turned upward in Jan 2026 while notebook panels fell. No small-size industrial series is public; the price book is paywalled. | TrendForce news | [secondary] https://www.trendforce.com/news/2026/01/06/insights-early-january-panel-prices-tv-panel-prices-turn-upward-while-notebook-panel-prices-trend-lower/ |

**Result: new.**
- Chinese street prices for name-brand AUO industrial panels at quantity 1 are about **US$22–28 (7" 800×480 and 10.1" 1280×800)**, before touch and cover glass.
- That is well below Robotshop's US$95 for the same G101EVN01.0 (note 17), and consistent with note 17's volume estimate of US$40–70 for 10.1" with resistive touch.
- Some of these "new" units are probably stock of a discontinued model (Panelook) [inference]. A design should use an active part, e.g. the BOE/Innolux equivalents.
- No Panelook prices exist publicly. Taiwan distributor quotes were not found.

---

## Open after this round

1. TI.com 1ku price for AM623/AM625 single and dual core. Fetch from a browser, or use TI's pricing API.
2. A 2026 price for any SoM after memory repricing. Ask Variscite, Toradex or MYIR for a 1 GB/8 GB quote.
3. Windows IoT LTSC 2024 Entry and Base (Arm64) OEM prices at 100–1,000 units. Request from Arrow, Avnet or Advantech; requires a Microsoft CLA.
4. JD.com and 1688 prices with dates for TPC7062Ti/TX, TK6071iQ, PI3070ie and FE6070. Needs a browser session in China.
5. Measured boot times. Run your own stopwatch test (see §4).
