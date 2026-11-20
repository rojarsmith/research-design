# Verification of regional open items in study 16 (section 六 and 待驗證 → 區域)

> Researched 2026-11-20 with web search plus direct fetches of primary pages where possible. Each claim has a URL. Verdict codes: CONFIRMED, CORRECTED, NEW FINDING, STILL UNVERIFIED. Where a claim rests only on a search-engine summary (page not opened), it is marked *(search summary only)*.

---

## 1. Korea: embedded GUI / HMI tool ecosystem

**Verdict: NEW FINDING (partial). Distributor channels and prices remain STILL UNVERIFIED.**

The study's "沒有找到資料" is no longer true. The findings below cover the domestic HMI maker, foreign-tool presence and the main OEM relationships. No Korean reseller list or price was found for any embedded GUI tool.

### Domestic HMI maker: M2I Corporation (엠투아이, KOSDAQ 347890)
- M2I describes itself as Korea's leader in smart HMI and SCADA. About 50–60% of its HMI revenue comes from semiconductor customers, with HMIs installed in Samsung Electronics and SK hynix fab equipment. Its named global rivals are Siemens and Mitsubishi. In Sep 2025 it acquired logistics automation firm SM Core from SK Group (combined scale about KRW 200B revenue), and it invested in robot-controller firm Bemrobotics. It aims to become an "AI 자율제조 통합 플랫폼" — [Hankyung 2025-12-07](https://www.hankyung.com/article/2025120785641)
- Q2 2026: standalone revenue KRW 11.68B, operating profit KRW 1.83B; consolidated revenue KRW 40.41B. Semiconductor-related revenue is more than 50% of the total — [Edaily 2026-08-18](https://en.edaily.co.kr/news/eda202608185620)
- M2I is adding next-generation HMI lines and robot-control services — [MTN 2025-09-22](https://news.mtn.co.kr/news-detail/2025092216375898252) *(search summary only)*
- M2I's screen-design software XDesignerPlus is described as a free download bundled with its HMIs. This follows the industrial-HMI norm where the editor is free and money comes from panel hardware — [Asiae (older article)](https://cm.asiae.co.kr/en/article/2020071014594230239) *(search summary only; the date of the free-download claim is old)*
- Implication: M2I is an industrial PLC-panel HMI vendor (the Weintek and Pro-face category), not an MCU/LVGL GUI-tool vendor. It is a possible partner or comparison, not a direct competitor.

### Foreign embedded GUI tools in Korea
- **ST TouchGFX:** ST Korea promotes it directly. A French ST engineer presented it at K-TEC Seoul on 2023-11-21. It is free on STM32. Korean uses cited are industrial HMI panels, door locks, boilers, appliances (coffee machines, washers, microwaves, vacuums), wearables and medical devices. No Korean company is named. About 150 people took part in ST Korea's "STM32 Quest: GFX Challenge" (Jul–Sep 2023) — [Digital Daily 2023-11-29](https://www.ddaily.co.kr/page/view/2023112814485539624)
- **Qt (Qt Group):** Qt has the strongest Korean OEM relationships found.
  - Hyundai Motor Group selected Qt as its key HMI technology partner for Hyundai, Kia and Genesis in Aug 2021, and uses Qt Design Studio — [Qt press release](https://qt.io/press/hyundai-motor-group-selects-qt-as-their-key-hmi-technology-partner) *(the press page body did not render on fetch; the date and claim come from the search summary)*
  - LG Electronics signed a strategic partnership in Jan 2020. webOS is built with Qt, and in 2024 Qt was embedded in LG's webOS in-vehicle content platform — [Qt: LG built with Qt](https://qt.io/lg-electronics-built-with-qt), [Qt press: LG partnership deepened](https://www.qt.io/press/the-qt-company-deepens-strategic-partnership-with-lg-electronics), [Qt press: LG automotive content platform](https://qt.io/press/qt-features-lgs-advanced-automotive-content-platform)
  - Qt worked with Korean automotive SoC maker Telechips in 2017 — [Qt blog 2017-01-09](https://qt.io/blog/2017/01/09/qt-together-telechips)
  - Qt sells directly in Korea, with staff based in Bundang ([Qt careers: SE Korea](https://www.qt.io/open-positions/se-korea)). It has a Korean-language Qt for MCUs page ([qt.io/ko-kr](https://www.qt.io/ko-kr/product/develop-software-microcontrollers-mcu-korea)). Kyungwoo Systech is a Korean "Built with Qt" customer ([Qt](https://www.qt.io/kyungwoo-built-with-qt)).
- **Altia:** listed on TI's Korean partner page as a GUI tool for TI processors — [TI Korea partner page](https://ti.com/partner/ko-kr/ALTIA)
- **LVGL:** listed on TI's Korean partner page ([TI ko-kr LVGL](https://www.ti.com/partner/ko-kr/LVGL)). No Korean company LVGL case study and no Korean LVGL distributor was found.
- **Samsung TizenRT:** Samsung's appliance RTOS, commercialised in low-end appliances since 2017 ([Samsung SDC23 session](https://developer.samsung.com/conference/sdc23/sessions/sw-platform-for-digital-appliance-part-i-tizenrt), [GitHub Samsung/TizenRT](https://www.github.com/Samsung/TizenRT)). No source tied TizenRT to LVGL or to a specific GUI tool. **STILL UNVERIFIED.**
- **Korean embedded-tool distributors:** MDS Tech (MDS테크, now under Hancom) is the main Korean embedded-tools distributor. It is the exclusive Korean supplier for Elektrobit and the outside-group distributor of Hyundai AutoEver's "mobilgene" — [Newswire (Elektrobit)](https://www.newswire.co.kr/newsRead.php?no=536279), [Digital Daily 2024-12-10](https://www.ddaily.co.kr/page/view/2024121017035847680) *(search summary only)*. No source shows MDS Tech or anyone else as a Korean distributor for Qt, Crank, Embedded Wizard, SquareLine or LVGL Pro. **STILL UNVERIFIED.**
- **Pricing in Korea:** nothing published. **STILL UNVERIFIED.**

### Inference
In Korea, foreign GUI vendors sell direct or through chip-vendor field teams (Qt has a Bundang office, ST Korea pushes TouchGFX) rather than through a Japanese-style master distributor. The big OEMs (Hyundai, LG) have standardised on Qt for Linux-class HMI. The MCU-class space appears to be served by free chip-vendor tools (TouchGFX). For a Taiwanese LVGL tool, the realistic Korean entry points are the chip-vendor ecosystems (ST, Renesas, NXP, Espressif), MDS Tech-type distributors, and appliance and industrial tier-2 suppliers. **[推論]**

---

## 2. Southeast Asia: manufacturing shift, design centres, GUI-tool buyers and distributors

**Verdict: NEW FINDING on the manufacturing shift and design centres. STILL UNVERIFIED on embedded GUI-tool buyers and distributors.**

### Vietnam
- H1 2026: computers, electronics and optical products took nearly 66% of newly registered manufacturing FDI, more than US$7.03B across 65 projects (+614% YoY). Large projects: Samsung Semiconductor Asia Holdings US$4.08B and Samsung Electro-Mechanics US$1.28B (both Thai Nguyen), and LG Innotek US$1B (Hai Phong). Source: Savills — [VOV 2026-09-07](https://english.vov.vn/en/economy/northern-vietnam-attracts-805-of-new-manufacturing-fdi-in-first-half-post1330684.vov)
- About 98% of electronics exports come from foreign-invested firms. Samsung's Hanoi R&D centre (US$220M) is its largest in SEA. Qualcomm has an R&D centre in Vietnam. Decree 182/2024 can subsidise up to 50% of R&D-centre initial costs — [Vietnam Briefing](https://www.vietnam-briefing.com/news/vietnams-electronics-industry.html/) *(search summary only)*
- Commentators note that design and advanced R&D still stay mostly abroad — [VnEconomy 2026-09-08](https://en.vneconomy.vn/a-focus-on-low-cost-production.htm) *(search summary only)*
- Vietnamese embedded services: FPT says it supports 160+ digital cockpit, HMI and telematics programmes with 1,800+ automotive engineers, and partners with Korea's Telechips on an AI cockpit platform — [TechNode Global 2026-01-02](https://technode.global/2026/01/02/fpt-collaborates-with-telechips-to-introduce-ai-powered-intelligent-cockpit-platform/) *(search summary only)*

### Malaysia (Penang)
- congatec (an embedded computing module maker) set up a Penang R&D subsidiary on 2026-01-27. It onboarded 23 embedded engineers from Kontron Asia and targets about 70 staff. The centre does x86 and Arm COM platform design, customisation and the aReady. software stack — [MIDA](https://www.mida.gov.my/media-release/congatec-expands-penang-rd-presence-anchoring-high-value-embedded-computing-capabilities-in-malaysia/)
- More than 45 IC-design companies operate in Penang (Penang Silicon Design @5km+, Jul 2025). AMD opened a Penang engineering lab on 2025-08-14 — [MIDA: Penang Silicon Design](https://www.mida.gov.my/mida-news/penang-silicon-design-5km-to-boost-its-position-as-tech-hub/), [MIDA: AMD](https://www.mida.gov.my/media-release/amd-expands-rd-footprint-with-state-of-the-art-malaysian-facility/) *(search summary only)*

### Thailand
- BOI 2025 investment applications hit a record THB 1.876 trillion across 3,370 projects (+67% by value). Electronics and electrical appliances: THB 277.6B across 470 projects. PCB production and PCB materials: 94 projects worth THB 249.2B. Thailand is now ASEAN's largest PCB base — [Nation Thailand 2026-01-26](https://www.nationthailand.com/business/investment/40061707), [Nation Thailand (PCB)](https://www.nationthailand.com/business/investment/40062873)

### Philippines
- 2025 electronics exports were US$49.64B (+16.1%), 58.8% of total Philippine exports. Semiconductors are about 70–73% of that, and the industry is assembly, test and EMS rather than design — [SEIPI](https://www.seipi.org.ph/?p=7660), [Manila Bulletin 2026-01-05](https://mb.com.ph/2026/01/05/electronic-exports-on-track-to-hit-49-billion-in-2025) *(search summary only)*

### Singapore and distribution
- Taiwan's WT Microelectronics completed its acquisition of Singapore-listed distributor Excelpoint on 2022-09-06 (about SGD 230M). In Dec 2024 it agreed to buy the founder's remaining 20% of WT Semiconductor for about US$30M — [MarketScreener](https://ca.marketscreener.com/quote/stock/EXCELPOINT-TECHNOLOGY-LTD-6811928/news/WT-Semiconductor-Pte-Ltd-completed-the-acquisition-of-Excelpoint-Technology-Ltd--41845347/), [Digitimes 2024-12-17](https://www.digitimes.com/news/a20241217PD207.html) *(search summary only)*. This gives a Taiwanese-owned component-distribution channel into SEA that could carry a software tool alongside MCUs and displays. **[推論]**
- Industrial HMI/SCADA software reaches SEA through regional distributors. For example, LINX became sole distributor of COPA-DATA zenon for Singapore, Malaysia and Thailand in 2023 — [COPA-DATA](https://www.copadata.com/en/newsroom/linx-new-distributor-for-zenon-in-singapore-malaysia-and-thailand/)
- No SEA distributor or reseller was found for Qt, Crank Storyboard, Embedded Wizard, SquareLine or LVGL Pro. **STILL UNVERIFIED.**

### Inference
SEA is a growing *manufacturing* base (Vietnam assembly, Thai PCBs, Philippine back-end), and some *embedded design* is moving there (congatec Penang, FPT automotive). GUI-tool purchase decisions mostly stay with the brand owner, as the original study assumed. The direct buyers to target are SEA design-services houses (FPT-type) and Taiwanese-owned distributors (WT/Excelpoint). **[推論]**

---

## 3. Japan: price levels of embedded GUI tools

**Verdict: STILL UNVERIFIED for yen price lists. CORRECTED on one citation. NEW FINDING: the Japanese channel is quote-only.**

- **Citation correction:** the study cites [IPROS 2000859227](https://mono.ipros.com/product/detail/2000859227) for "Crank Storyboard is sold through Osaka NDS". That page is actually **テガラ株式会社's (Tegara) listing for Embedded Wizard**. Storyboard by Osaka NDS appears only in its "related products" sidebar (verified by fetch). The Osaka NDS claim itself is correct; the right sources are:
  - Crank/AMETEK named OsakaNDS its authorised Japanese partner in Sep 2022 — [Crank press release](https://cranksoftware.com/press-release-crank-expands-support-in-japan-with-osakands)
  - SDTech (エスディーテック) became a sub-reseller under Osaka NDS on 2024-05-23 — [newscast.jp](https://newscast.jp/smart/news/2428053), [SDTech on IPROS](https://pr.mono.ipros.com/en/sdtech/product/detail/2001523956)
- **Embedded Wizard in Japan (Tegara):** licensed as a "Studio license" (tool) plus a "PlatformPackage license" (runtime). The price is quoted per project, based on target MCU, annual production volume, colour format and TrueType use. No price is published — [IPROS 2000859227](https://mono.ipros.com/product/detail/2000859227)
- **SEGGER emWin:** Envitech (エンビテック) is the Japanese distributor. Renesas offers it free for mass production on RX and RA ([Renesas JP](https://renesas.com/ja/document/prb/segger-emwin)), and STemWin is free on STM32 ([ST JP](https://www.st.com/ja/embedded-software/stemwin.html)). No yen price was found. **The only public anchor is SEGGER's global list:** emWin PRO €11,980; BASE colour €5,980; BASE greyscale €3,980; BASE b/w €2,980; emWin-sim €2,480. These are "from" prices for a single-product licence, each including 6 months of support and updates; product-family, CPU and buyout licences are quoted on request — [SEGGER emWin price list](https://www.segger.com/pricelist-emwin.html). The USD list shows PRO US$14,980 and BASE colour US$7,480 — [SEGGER pricing](https://www.segger.com/purchase/pricing/emwin/) *(search summary only)*
- **Qt (Qt for MCUs / Device Creation):** no Japanese or embedded price is published; sales goes through Qt Japan's sales-contact form — [Qt JP sales contact](https://www.qt.io/contact-us/jp/sales-contact-request/). Qt's online pricing FAQ covers only Application Development and Small Business — [qt.io/pricing](https://www.qt.io/pricing)
- **HI Corporation exbeans UI Conductor:** no price found. The last product news found is a 2018 joint solution with ITEC Hankyu Hanshin's CANTOOL — [Nikkan Kogyo (PR TIMES)](https://www.nikkan.co.jp/releases/view/19313) *(search summary only)*
- **CRI Middleware:** this is a domestic HMI-tool vendor the study did not list. "CRI Glassco" is its automotive HMI tool, in mass production for meter clusters and in development for IVI (as of Jul 2025) — [CRI Glassco product page](https://www.cri-mw.co.jp/business/product/automobile/criglassco/). A FISCO note (2026-05-22) says Glassco is widely adopted in India's motorcycle market via Japanese exports — [FISCO report PDF](https://www.fisco.co.jp/wordpress/wp-content/uploads/FISCO/cri-mw20260522_e.pdf) *(search summary only; PDF encrypted, could not extract)*. CRI also sells "Aeropoint GUI" (screens designed in PowerPoint) on a contact-us basis — [IPROS middleware category](https://mono.ipros.com/en/list/cg1/Middleware) *(search summary only)*. No prices found.

### Inference
Japanese embedded GUI pricing is **quote-only**, priced by MCU, volume and features through a distributor. The only transparent anchor is SEGGER's €2,980–11,980 per product. A Taiwanese tool with a public price list would be unusual in Japan, but Japanese buyers will still expect a distributor quote and volume-based runtime terms. **[推論]**

---

## 4. EU RED Delegated Regulation (EU) 2022/30 and EN 18031

**Verdict: CONFIRMED (application date and harmonised standards with restrictions) + NEW FINDING (2022/30 has been repealed with effect from 2027-12-11, when the CRA takes over).**

- **Application date:** the Commission's RED page says Delegated Regulation 2022/30 "entered into application in August 2025" (i.e. 2025-08-01). It covers RED Art. 3(3)(d) network protection, (e) personal data and privacy, and (f) fraud — [EC RED page](https://single-market-economy.ec.europa.eu/sectors/electrical-and-electronic-engineering-industries-eei/radio-equipment-directive-red_en)
- **Harmonised standards:** Commission Implementing Decision (EU) 2025/138 was adopted 2025-01-28 and published in OJ L on 2025-01-30, applying from publication. It added EN 18031-1:2024 (Art. 3(3)(d)), EN 18031-2:2024 (3(3)(e)) and EN 18031-3:2024 (3(3)(f)) to Implementing Decision 2022/2191, **with restrictions** — [EUR-Lex 2025/138](https://eur-lex.europa.eu/eli/dec_impl/2025/138/oj/eng):
  - All three: the "rationale" and "guidance" sections give no presumption of conformity. There is also no presumption when, under clauses 6.2.5.1 and 6.2.5.2, the user may choose not to set or use any password.
  - EN 18031-2: no presumption for clauses 6.1.3 to 6.1.6 equipment (childcare, toys, wearables) unless parental or guardian access control is ensured.
  - EN 18031-3: clause 6.3.2.4 (secure-update assessment criteria) gives no presumption at all.
  - Practical effect: a product that allows "no password", or that relies on the 18031-3 secure-update criteria, cannot self-declare (Module A) for that requirement and needs a notified body or another route — [Spanish ministry guide to EN 18031](https://digital.gob.es/content/dam/portal-mtdfp/avance-digital/telecomunicacion-e-infraestructuras-digitales/areas_interes/equipos-telecomunicacion/documents/Guia-aplicacion-EN-18031.pdf) *(search summary only)*
  - No 2026 amendment that lifts these restrictions was found. **STILL UNVERIFIED** whether one exists; search found none.
- **NEW: repeal by the CRA.** Commission Delegated Regulation (EU) 2026/339 was adopted 2026-02-16 and published in OJ 2026-04-29. Art. 1: "Delegated Regulation (EU) 2022/30 is repealed with effect from 11 December 2027." Recital 3: the CRA's (Reg. 2024/2847) essential requirements cover all elements of RED Art. 3(3)(d), (e) and (f). Recital 4: the repeal coincides with full CRA application to avoid overlap. Recital 5: market surveillance of equipment placed on the market between 2025-08-01 and 2027-12-10 is unaffected — [EUR-Lex 2026/339](https://eur-lex.europa.eu/eli/reg_del/2026/339/oj/eng); also confirmed on the [EC RED page](https://single-market-economy.ec.europa.eu/sectors/electrical-and-electronic-engineering-industries-eei/radio-equipment-directive-red_en)
- **Status as of Nov 2026:** RED cybersecurity (EN 18031) is mandatory now for internet-connected radio equipment such as ESP32-class Wi-Fi and BLE devices. From 2027-12-11 the CRA replaces it. CRA harmonised standards (request M/606, about 41 standards) are still being drafted: ETSI's 17 vertical drafts are in public enquiry, and a Commission draft amendment (C(2026)7861) shifts deadlines — [CEN-CENELEC](https://www.cencenelec.eu/news-events/news/2025/newsletter/ots-62-cra/), [Commission draft C(2026)7861](https://ec.europa.eu/transparency/documents-register/api/files/C(2026)7861_0/de00000001090398), [Infosecurity Magazine (ETSI 17 drafts)](https://infosecurity-magazine.com/news/etsi-proposes-17-cybersecurity) *(search summary only; exact new deadlines not verified)*

### Implication
The selling window has two phases. (1) Now to Dec 2027: RED/EN 18031 features such as forced password setup, secure update and data protection, with the restriction meaning "no password" designs need a notified body. (2) From Dec 2027: CRA (SBOM, vulnerability handling, update support period). An IDE that bakes in an EN 18031-compliant authentication and update template now carries forward into the CRA. **[推論]**

---

## 5. US Cyber Trust Mark and the federal procurement linkage

**Verdict: CONFIRMED (ioXt is lead administrator; program not yet operational) + CONFIRMED (federal procurement linkage still in effect; it was retained, not rescinded, by EO 14306) + NEW FINDING (no implementing FAR rule located; CLAs cannot yet authorise labels).**

- **Procurement linkage:** EO 14144 (2025-01-16) originally required the FAR to have agencies, by **2027-01-04**, require consumer IoT vendors to carry the Cyber Trust Mark. **EO 14306 (2025-06-06, published FR 2025-06-11, doc 2025-10804)** rewrote much of EO 14144 but **kept** this as the new Sec. 7(c): FAR Council agency members shall "amend the FAR to adopt requirements for agencies to, by January 4, 2027, require vendors to the Federal Government" of "consumer Internet-of-Things products, as defined by 47 CFR 8.203(b)," "to carry United States Cyber Trust Mark labeling for those products" — [Federal Register / govinfo EO 14306](https://www.govinfo.gov/content/pkg/FR-2025-06-11/html/2025-10804.htm); law-firm confirmation: [WilmerHale 2025-06-13](https://www.wilmerhale.com/en/insights/client-alerts/20250613-new-executive-order-modifies-cybersecurity-requirements-to-be-imposed-on-federal-contractors-and-subcontractors), [Paul Weiss](https://www.paulweiss.com/insights/client-memos/trump-s-new-cyber-executive-order-preserves-many-cyber-initiatives-rolls-back-security-requirements-for-contractors-and-changes-cyber-sanctions-regime)
- No later executive order revoking EO 14144 Sec. 7(c) was found (searched for 2026 revocations) — [search context: SecurityWeek](https://www.securityweek.com/trump-cybersecurity-executive-order-targets-digital-identity-sanctions-policies/). **Implementing FAR rule: none found.** The FAR Council is in the "Revolutionary FAR Overhaul", and the Open FAR Cases report could not be fetched (acq.osd.mil certificate error). An Aug 2026 procurement-signal item says agencies are preparing to include USCTM in consumer wireless IoT buys by Jan 2027, but its only source is a Reddit thread — [Govly signal 2026-08-04](https://app.govly.com/public/signals/161153). Treat as **STILL UNVERIFIED** whether a FAR clause will actually be in force on 2027-01-04.
- **Program status:** UL withdrew as lead administrator on 2025-12-19, and the FCC selected ioXt Alliance on 2026-04-13 — [ioXt](https://www.ioxtalliance.org/news-events-blog/fcc-names-ioxt-alliance-lead-administrator-for-us-cyber-trust-mark-program), [Nextgov 2026-04](https://www.nextgov.com/cybersecurity/2026/04/fcc-selects-ioxt-alliance-lead-cyber-labeling-program/412800)
- **FCC DA 26-834 (2026-08-11):** conditionally approved two more CLAs (IIA Lab Services; Element Materials Technology Portland – Evergreen) and opened a rolling CLA filing window. Key point: **"Conditionally approved CLAs are not authorized by the Commission to approve the use of the FCC IoT Label."** CLAs must get ISO/IEC 17065 accreditation within 6 months after the FCC adopts labelling standards and testing procedures, and those standards are not yet adopted. CLA authority terminates automatically if the CLA becomes affiliated with a Covered List equipment producer — [FCC DA 26-834](https://docs.fcc.gov/public/attachments/DA-26-834A1.pdf)
- **2026-10-27 Public Notice:** proposes the label design (mark plus QR code), English and Spanish registry pages, 3-year renewal, and post-market surveillance by CyberLABs or CLA labs (not in-house). Comments are due 30 and 45 days after FR publication. Commissioner Trusty hopes labelled products reach shoppers "within a year" — [Cooley 2026-11-10](https://www.cooley.com/news/insight/2026/2026-11-10-fcc-advances-us-cyber-trust-mark-program), [Nextgov 2026-10](https://www.nextgov.com/cybersecurity/2026/10/fcc-commissioner-hopes-cyber-trust-mark-products-reach-shoppers-within-year/416878/)

### Implication
There is a timing mismatch. The EO still says federal buyers must require the mark by 2027-01-04, but as of Nov 2026 no product can carry it: standards are not adopted, no CLA is authorised, and no FAR clause was found. Expect either a slipped FAR effective date or a requirement that cannot yet be met. For a tool vendor this is a medium-term (2027–2028) signal, not an immediate purchase trigger. **[推論]**

---

## Summary table

| # | Verdict | One-line fact | Key source |
|---|---|---|---|
| 1 | NEW FINDING (distributors and pricing STILL UNVERIFIED) | M2I is Korea's industrial HMI leader (50–60% of HMI revenue from Samsung and SK hynix fabs); Qt is the OEM standard at Hyundai and LG; ST Korea pushes free TouchGFX; no Korean GUI-tool reseller or price found | https://www.hankyung.com/article/2025120785641 ; https://www.ddaily.co.kr/page/view/2023112814485539624 |
| 2 | NEW FINDING (GUI-tool buyers and distributors STILL UNVERIFIED) | VN H1-2026 electronics ≈66% of new manufacturing FDI (US$7.03B); congatec Penang embedded R&D (23→70 staff); Thai BOI 2025 E&E THB 277.6B and PCB THB 249.2B | https://english.vov.vn/en/economy/northern-vietnam-attracts-805-of-new-manufacturing-fdi-in-first-half-post1330684.vov |
| 3 | STILL UNVERIFIED (yen prices) + CORRECTED (citation) | Japanese GUI tools are quote-only; only public anchor is SEGGER emWin €2,980–11,980; study's IPROS link is Tegara's Embedded Wizard, not Crank | https://www.segger.com/pricelist-emwin.html ; https://cranksoftware.com/press-release-crank-expands-support-in-japan-with-osakands |
| 4 | CONFIRMED + NEW FINDING | 2022/30 applies from 2025-08-01; EN 18031 listed with restrictions (no-password and 18031-3 secure update); repealed by 2026/339 from 2027-12-11 when the CRA applies | https://eur-lex.europa.eu/eli/reg_del/2026/339/oj/eng |
| 5 | CONFIRMED + NEW FINDING | EO 14306 kept the 2027-01-04 federal procurement requirement; ioXt is lead administrator; no CLA can yet authorise labels and no FAR rule was found | https://www.govinfo.gov/content/pkg/FR-2025-06-11/html/2025-10804.htm ; https://docs.fcc.gov/public/attachments/DA-26-834A1.pdf |
