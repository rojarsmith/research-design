# Verification of open items: market sizing, surveys and AI (study 16)

Done 2026-11-20. Scope: the 11 items under 待驗證 (market and AI) in `16-global-evidence.zh-TW.md`.

Verdicts:
- **CONFIRMED**: a primary source, or the primary source's own text as surfaced in search, supports the claim.
- **CORRECTED**: the study's figure is wrong or incomplete. The corrected fact is given.
- **STILL UNVERIFIED**: no source supports it. I record what I tried.

Access limits during this pass: iea.org, omdia.tech.informa.com, gartner.com, businesswire.com, community.nxp.com, microchip.com and the ABI press page all returned HTTP 403 or 404 to the fetch tool. web.archive.org is not reachable from this environment. Retrying with a browser user agent was refused by the environment's permission policy. Where I fell back to search-engine snippets of a primary page, or to secondary coverage, I say so.

---

## Market and surveys

### 1. Size of the embedded GUI / HMI software tools market (VDC, Omdia and others)
**Verdict: STILL UNVERIFIED** (no Tier-1 figure). There is one Tier-3 figure that is closer in scope than what the study now cites.

- I searched "VDC Research embedded GUI software tools market size" and found no VDC, Omdia or IDC figure for embedded GUI tools. VDC's 2025 report *AI Copilots & Code Generation for the IoT* is paywalled and its public page shows no market values. — https://www.vdcresearch.com/Coverage/IoT-Tech/reports/25-AI-Copilots-EB.html
- The closest-scoped figure is Tier 3. Market Research Future (MRFR) sizes "Embedded GUI Development Software" at **US$239.8M in 2024**, forecast at US$2,694.3M by 2035 (27.2% CAGR). I saw this in a search snippet. Older MRFR editions disagree: US$194.9M in 2023 forecast to US$1,643.2M by 2032, and a 2023 release projecting about US$1,012.4M by 2030 at 25.9% CAGR. — https://marketresearchfuture.com/reports/embedded-graphical-user-interface-gui-development-software-market/market-size ; https://marketresearchfuture.com/press-release/embedded-gui-development-software-market
- Inference: the MRFR value (about US$0.2B) is the same order of magnitude as VDC's historic sizing of all commercial embedded tools (below US$0.5B in the 2000s). It is much more plausible for a GUI-tool niche than the US$5.8–8.3B "HMI software" range, which is mostly industrial SCADA/HMI. I have not verified the MRFR method or its 25–30% CAGRs.
- HMI software figures for 2025 differ by source: US$5.8B (Marketintelo), US$5.93B (MRFR), US$6.23B (TechSci), US$6.47B (MarketPublishers), US$8.27B (Insight Partners). All are Tier 3. — https://www.theinsightpartners.com/reports/hmi-software-market ; https://techsciresearch.com/report/human-machine-interface-hmi-software-market/25451.html

### 2. Embedded developer count (2024–2026); VS Code share; GUI tool share; Eclipse 2025 survey
**Verdict: STILL UNVERIFIED** for all four sub-items. Partial data and one correction follow.

- **Eclipse 2025 IoT & Embedded Developer Survey report.** The press release is dated 5 Mar 2026 and the report was released with embedded world 2026 (10–12 Mar 2026). The release has **no figures**: no sample size and no percentages. — https://newsroom.eclipse.org/node/43632
- The report is gated behind an Eclipse account sign-in. The landing page lists topics only: safety certification, security and deployment, developer preferences, open source, hardware and OS. — https://outreach.eclipse.foundation/2025-iot-embedded-developer-survey-report
- The Eclipse annual report 2026 also has no survey figures. — https://www.eclipse.org/org/foundation/annual-reports/2026/
- I found no press coverage or blog quoting any 2025 survey numbers, searching in both standard and extended modes.
- **The 2024 Eclipse report has no IDE, VS Code, GUI or AI questions.** I read the full public PDF. It has 747 respondents; 75% use open source and 24% contribute. The 2025 report probably also lacks VS Code data, but I could not check because it is gated. — https://5413615.fs1.hubspotusercontent-na1.net/hubfs/5413615/Eclipse%20IoT%20White%20Papers%20and%20Case%20Studies/IoT%20and%20Embedded%20Survey%20Report%202024.pdf
- **Developer count.** SlashData puts the total at about 47.2M developers in early 2025. It says the embedded developer population "has more than doubled since 2022" but gives no headcount. — https://slashdata.co/post/global-developer-population-trends-2025-how-many-developers-are-there
- SlashData Q3 2025 counts 38.4M professional developers. Of these, 11M (29%) build or integrate AI for edge devices. This measures edge-AI activity, not "embedded developers." Published 12 Feb 2026. — https://www.slashdata.co/post/rapid-growth-in-edge-ai-developers-and-where-the-opportunity-lies
- The study's figures (VDC 2010: 880K; SlashData 2023: about 5%) remain the latest published counts. Inference only: if the "more than doubled" claim is right, the study's 1–2.5M working range may be low. I have not quantified this.
- **VS Code share.** No embedded-specific figure was found.
  - Nearest proxy: JetBrains *State of C 2025* (about 900 C developers, vendor survey). The IDE used most for C is VS Code 42%, CLion 19%, Visual Studio 14%, Vim 9%, Keil µVision 2%. 21% regularly use "Embedded C" and 11% use the Keil compiler. Only 11% "already use AI coding agents." — https://lp.jetbrains.com/the-state-of-c-2025/
  - Blues IoT Developer Survey 2025 (vendor, sample size not stated) ranks Arduino IDE first, VS Code + PlatformIO second and STM32Cube third. Percentages appear only in chart images. — https://dev.blues.io/blog/blues-iot-developer-survey-2025/
- **GUI tool share.** No figure for LVGL, TouchGFX, SquareLine, Qt or emWin was found. JetBrains *State of Embedded 2025* reports 13% doing "graphical system design," and the study already cites this. — https://lp.jetbrains.com/the-state-of-embedded-2025

### 3. Share of MCUs or IoT devices with a display
**Verdict: STILL UNVERIFIED** for unit share. **CORRECTED** for the study's survey proxy: it understates GUI incidence and mixes up two questions.

- No analyst source gives the share of MCUs, by units, that drive a display, or the share of IoT devices with a screen. Searches covered MCU/TFT share, "IoT devices with a display" and Mordor/Transforma.
  - Mordor's "Smart Speakers and Displays 26.61%" is a category share of the IoT devices market, not display incidence. — https://www.mordorintelligence.com/industry-reports/iot-devices-market
  - Vendor and trade texts are qualitative only. — https://www.digikey.com/es/articles/microcontrollers-with-graphics-display-capabilities-are-in-high-demand
- **Correction to the study's 34–37% proxy.** I read the AspenCore/UBM *Embedded Markets Study* decks. Two different questions give different answers:
  - "Does your current design use a graphical user interface?" **Yes: 49% in 2017 (N=870), 49% in 2015 (N=806), 41% in 2014 (N=1,155), 41% in 2013 (N=1,993).** The 2017 summary slide says GUI usage "stayed even at 49%." — https://staff.emu.edu.tr/mehmetbodur/Documents/courses/CMPE423/Notes/2017-embedded-market-study.pdf (slides 100–101)
  - "Which capabilities are included in your current project?" **GUI: 37% in 2015 (N=1,606; first year asked), 36% in 2017 (N=1,107).** — 2015: https://webpages.charlotte.edu/~jmconrad/ECGR4101-2016-08/notes/UBM%20Tech%202015%20Presentation%20of%20Embedded%20Markets%20Study%20World%20Day1.pdf (slide 14); 2017: same 2017 PDF, slide 26
  - The study's "約 34% 至 37%" appears to take 34% from another row; 2015 has only one GUI value, 37%. The correct reading is **about 36–37% (capability list) to 49% (direct question) of current embedded designs**. These are design shares among survey respondents, not unit shares, and the data is 2015–2017.
- No AspenCore edition after 2019 was found. The 2019 deck exists but I did not extract any GUI figure from it.

### 4. IEA GEVO 2026 chargers; Omdia January 2026 industrial displays
**IEA: CONFIRMED** from the IEA chapter's own text as surfaced in search, plus secondary press. The page itself returned 403.
- IEA GEVO 2026 (published 20 May 2026), "Electric vehicle charging" chapter: "nearly **1.8 million** public charging points were added to the global stock [in 2025], representing an increase of **more than 33%**"; "total stock of public charging points worldwide reached **more than 7 million** at the end of 2025." About 11 electric LDVs per public point, similar to 2024. More than 43 million private LDV chargers. — https://www.iea.org/reports/global-ev-outlook-2026/electric-vehicle-charging-chap-6-and-10
- Secondary coverage dated 20 May 2026 attributes the following to GEVO 2026:
  - China: from nearly 3.4M to more than 4.7M public points, over 75% of global growth and more than 65% of the stock.
  - Europe: public points up about 20% (NL 210K, DE 196K, FR 185K).
  - US: up 20%; fast and ultra-fast points nearly 70K (+30%); slow points over 160K; 3% of global public points.
  - Source: https://greentechlead.com/electric-vehicle/global-ev-charging-infrastructure-expands-rapidly-as-ultra-fast-chargers-and-investments-surge-53460
- Other sources use different units: Roland Berger counts about 1.1M public points added in 2025, and BNEF counts 6.7M connectors (+28%). I saw both in search snippets only. — https://www.rolandberger.com/en/Insights/Publications/EV-Charging-Index-2026-Currents-of-change.html ; https://about.bnef.com/electric-vehicle-outlook/
- I could not get the IEA PDF. The launch webinar deck has no charger slide (https://iea.blob.core.windows.net/assets/beae65ce-6b6d-4a8b-b67a-c591093756be/GEVO2026_LaunchWebinarSlides.pdf). Full-report mirrors were over 10 MB and not fetched (https://www.cleanenergyministerial.org/content/uploads/2026/05/globalevoutlook2026.pdf).

**Omdia: CORRECTED.**
- Omdia's January 2026 release reports 2025 actuals: industrial display panel makers' revenue was **US$3.4B, +24% YoY**, while **unit shipments fell 0.4%**, mainly because of Europe's phase-out of disposable e-cigarettes.
  - Growth came from gaming handheld displays (+101%) and action cameras (+23%), about three-quarters of the incremental revenue.
  - AUO Display Plus led on revenue, driven by POS.
  - "Military-like" displays: 965K units (+5.9%).
  - Sources: Display Daily, 21 Jan 2026 — https://displaydaily.com/?p=385009 ; iTiger, 22 Jan 2026 — https://www.itiger.com/news/1116948322 ; original (403) — https://omdia.tech.informa.com/pr/2026/jan/industrial-display-revenues-grow-24percent-yoy-in-2025-led-by-auo-display-plus
  - The release gives no absolute 2025 unit total.
- The study's "290.6M units in 2025 (+3.4%), smart home and office 64%" is **Omdia's July 2025 forecast**, not the January 2026 result. Its text: "281.1 million units in 2024 … expected to grow 3.4% … to 290.6 million units in 2025"; smart home and office "accounts for 64% of total shipments this year." Communications Today, 8 Jul 2025. — https://www.communicationstoday.co.in/?p=360711
  - Implied 2025 actual, by my arithmetic: 281.1M × 0.996 ≈ 280M units. Omdia did not publish this number.
- Definition caveat: an earlier Omdia release (27 Jan 2025) put 2024 industrial display shipments at **167.9M** (forecast 178M for 2025), with smart home and office at 46%. That is a different scope or baseline from the 281.1M series. — https://www.emsnow.com/?p=44979

### 5. Unit volumes: medical devices with displays, HMI operator panels, building-automation wall controllers and thermostats
**Verdict: STILL UNVERIFIED** at Tier 1. Tier-3 and vendor data points:
- **HMI operator panels.** The LP Information / GlobalInfo abstract (January 2026) says: "average price … approximately $2,000 per unit, with global sales of approximately 1.4 million units." The market is forecast at US$4,373M by 2032 (5.9% CAGR). Tier 3, method not stated. — https://pdf.marketpublishers.com/globalinfo/global-hmi-operator-panels-gir.pdf (report page: https://marketpublishers.com/r/GF0B85669238EN.html)
- Pro-face (Schneider) says it has sold more than 5 million HMIs cumulatively as of 2019, at about 1 million every 3 years. Vendor data, from a search snippet; the page returned 403. — https://www.proface.com/en-us/content/over-5-million-hmis-sold
- **Smart thermostats.** ABI Research (2 Jul 2024) forecasts **67.2M smart thermostats shipped globally by 2030**. Thermostats, smart lights and smart plugs together grow from 230.9M (2024) to over 584M (2030). There is no thermostat-only base-year figure and no mention of screens. — https://www.prnewswire.com/apac/news-releases/the-smart-thermostat-sector-heats-up-with-67-2-million-devices-shipped-by-2030--302187759.html
- Parks Associates (US only) forecasts 8.1M units in 2030, from a search snippet. — https://www.parksassociates.com/blogs/energy-pr/market-for-smart-thermostats-will-reach-unit-sales-of-81-million-in-2030-with-annual-revenues-exceeding-11-billion
- None of these separates thermostats with displays.
- **Medical devices with displays.** I found no unit data from Omdia, Yole, DSCC or others. Omdia's industrial display series includes home medical panels but does not break them out. — https://displaydaily.com/?p=385009
- **Building-automation wall controllers.** No unit data found.

---

## AI

### 6. Neutral (non-vendor), large-sample survey of AI coding use among embedded developers, 2025–2026
**Verdict: STILL UNVERIFIED** (no public numbers). One lead exists.
- **VDC Research**, an analyst firm and not a tool vendor, published *AI Copilots & Code Generation for the IoT* on 31 Aug 2025. It is based on VDC's **Voice of the Engineer survey, 600 qualified respondents**.
  - The public abstract gives only: embedded demand for code generation "has lagged behind" enterprise; adoption "will be strongest in non-safety-critical IoT segments"; and "38%" of AI users were ahead of schedule.
  - On 3–6 month delays: "3.0% of AI users versus 10.9% of non-AI users."
  - The adoption rate is behind the paywall.
  - Sources: https://www.giiresearch.com/report/vd1802915-ai-copilots-code-generation-iot-transforming.html ; https://www.vdcresearch.com/Coverage/IoT-Tech/reports/25-AI-Copilots-EB.html
- The Eclipse 2025 survey, from a foundation, may have AI questions, but it is gated (see item 2).
- The JetBrains *State of C 2025* (vendor) reports 11% "already use AI coding agents" among C developers. It is not embedded-only. — https://lp.jetbrains.com/the-state-of-c-2025/
- A DEV Community post cites an "Embedded.com Survey, 2026" (47% say AI halved debugging time). I could not find that primary survey, so I treat the figure as unreliable. — https://dev.to/nlocoding/ai-coding-assistants-for-embedded-systems-2026-guide-1h8j
- The RunSafe and Black Duck surveys remain the only public embedded AI-adoption figures, and both are from vendors.

### 7. Gartner's wording on low-code vs vibe coding (October 2025)
**Verdict: STILL UNVERIFIED** for the quote. The report's metadata is CONFIRMED.
- The report is *Why AI Won't Replace the Need for Low-Code Application Platforms*, **7 October 2025, Adrian Leow and Kyle Davis**. OutSystems' reprint page confirms the title, date and authors. — https://www.outsystems.com/1/low-code-ai-platforms-gartner/
- The Gartner abstract page appears in search but returned 403 to the fetch tool. — https://www.gartner.com/en/documents/7033498
- Lines on the OutSystems page, which come from the report: "The rise of generative AI tools like AI code assistants and vibe coding platforms has led teams to question…", and "Choose low-code platforms for code abstraction, maintainability, and risk mitigation over vibe coding alone…". Search snippets of the vendor pages also show the advice to "limit vibe coding to scoped, developer-supervised use cases such as boilerplate generation, prototyping, or noncritical internal tools."
- "Ultimately, vibe coding accelerates typing. LCAPs accelerate engineering." appears only on the Jitterbit and Mendix pages, which redistribute the report. Both pages now return 404 to fetch; I saw the line in search snippets.
  - I found **no Gartner newsroom press release** on this topic. Gartner's own abstract page could not be read, so the line cannot be checked against Gartner-hosted text.
  - Sources: https://www.jitterbit.com/report/gartner-report-why-ai-wont-replace-low-code-application-platforms ; https://www.mendix.com/?p=253477
- Related Gartner abstracts, seen as titles and snippets only:
  - "How to Scale Vibe Coding Using Low-Code Engineering Principles" (dated 11 Aug 2026 in the snippet)
  - "Deliver the Benefits of Vibe Coding Without the Risk With These 4 Low-Code Platform Markets" (28 Sep 2026)
  - Source: https://www.gartner.com/en/documents/8437013

### 8. IoT-SkillsBench (arXiv 2603.19583): success rates by configuration
**Verdict: CONFIRMED** (numbers extracted).
- Paper: Li et al. (Duke), submitted 20 Mar 2026.
- Setup: backbone model **Claude Sonnet 4.5** for all experiments; LangGraph agent with manager, coder and assembler nodes. The LLM-written skills were generated with Claude Sonnet 4.6.
- Scope: 3 platforms (Arduino/ATmega2560, ESP-IDF/ESP32-S3, Zephyr/nRF52840) × 42 tasks × 3 configurations = 378 hardware-validated instances.
- Cells are counts of compile failure / behavior failure / behavior correct (CF/BF/BC) out of 42. Percentages are my arithmetic. — https://arxiv.org/html/2603.19583v1 (Tables 1 and 2); abstract: https://arxiv.org/abs/2603.19583

| Config | pass@1 Arduino | pass@1 ESP-IDF | pass@1 Zephyr | **pass@1 overall** | **pass@5 overall** |
|---|---|---|---|---|---|
| No skills | 37/42 (88.1%) | 26/42 (61.9%) | 24/42 (57.1%) | **87/126 = 69.0%** | 101/126 = 80.2% (Arduino 42, ESP-IDF 31, Zephyr 28) |
| LLM-written skills | 39/42 (92.9%) | 26/42 (61.9%) | 19/42 (45.2%) | **84/126 = 66.7%** | 95/126 = 75.4% (41, 27, 27) |
| Human-expert skills | 41/42 (97.6%) | 40/42 (95.2%) | 39/42 (92.9%) | **120/126 = 95.2%** | 124/126 = 98.4% (42, 41, 41) |

- Takeaways:
  - LLM-written skills did **not** help overall and hurt on Zephyr.
  - Expert skills mostly removed behavior failures on ESP-IDF and Zephyr. Those are the platforms where code compiled but misbehaved on hardware.
  - Arduino is near ceiling even without skills.

### 9. Customer willingness to pay for MCP / AI layers in embedded tools
**Verdict: STILL UNVERIFIED.** No survey or customer data was found. The price signals all point to free or beta offerings:
- Microchip MPLAB AI Coding Assistant: "available for free; some advanced features may require a subscription license." No subscription price was found. — https://www.globenewswire.com/news-release/2025/02/19/3028656/0/en/artificial-intelligence-meets-embedded-development-with-microchip-s-mplab-ai-coding-assistant.html
- In July 2026 Microchip also made the formerly paid MPLAB XC Pro compilers and the MPLAB ML Development Suite free. I saw this in a search snippet; the page returned 403. — https://www.microchip.com/en-us/about/news-releases/products/microchip-expands-developer-access-with-free-mplab-xc-compilers-and-mplab-ml-development-suite
- NXP's agentic AI is an "experimental beta" with no price (item 11). ST's is a free beta and Nordic's and Espressif's MCP servers are free (study 16 sources).
- Embedder's site has a "TRY FREE" button and a headline "Embedder Launches Embedder Free…", with no prices listed. — https://www.embedder.com/
- Embedr (a different product) is listed "from $25/month" by an aggregator. Unverified. — https://aisotools.com/pricing/embedr
- VDC's 2025 AI copilots report includes 2024–2029 forecasts, but the public abstract has no WTP or pricing data. — https://www.giiresearch.com/report/vd1802915-ai-copilots-code-generation-iot-transforming.html
- The general AI coding price anchor is GitHub Copilot paid plans from $10/month, from a snippet. — https://pricepertoken.com/coding-assistants/github-copilot

### 10. Embedder (YC S25): funding and user claims; US$500K vs US$625M
**Verdict: CORRECTED.**
- **No round has been announced by the company or by an investor.** The YC profile, embedder.com, the PR Newswire release (14 Jul 2026, Verkor partnership) and the Embedded Computing Design article (16 Mar 2026, Embedded Award nomination) state no funding or user numbers.
  - https://www.ycombinator.com/companies/embedder
  - https://www.embedder.com/
  - https://www.prnewswire.com/news-releases/embedder-and-verkor-partner-to-take-agentic-ai-from-chip-design-to-working-firmware-302824710.html
  - https://embeddedcomputing.com/technology/ai-machine-learning/ai-logic-devices-worload-acceleration/embedder-v031-ai-powered-firmware-engineering-platform-nominated-for-embedded-award-2026-in-the-startup-category
- **US$500K** matches the standard YC deal: "We invest $500,000," made up of a $125K post-money SAFE for 7% plus a $375K uncapped MFN SAFE. — https://www.ycombinator.com/deal
  - VCBacked lists "Embedder has raised $500K," Pre-Seed, September 2025, backed by Y Combinator (data as of 9 Sep 2025). — https://www.vcbacked.co/company/embedder
- **US$625M is an aggregator error.** It appeared on StartupHub, which lists a seed round and about 14–15 staff, versus a YC team size of 6. The current StartupHub page no longer shows a funding figure. No other source supports it. — https://www.startuphub.ai/startups/embedder-yc-s25
- A YC job listing, seen in search snippets only (the page now returns 404), says "backed by CRV, Box Group, and Emerson Collective" with an unfilled "$XM+ raised" placeholder. It also claims "5,000+ engineers" using the product and "100+ paying customers." These are unverified self-claims. — https://www.ycombinator.com/companies/embedder/jobs/wMYukwR-founding-ai-engineer
  - Inference only: a later seed round from CRV and others may exist but is undisclosed.
- Do not confuse Embedder with **Embedd** (London), which raised about €2.31M / US$2.7M pre-seed led by Seedcamp (a different company). — https://therecursive.com/embedd-raises-eur2-31m-to-build-software-infrastructure-for-physical-ai

### 11. Does NXP offer an MCP server or AI coding assistant for its MCUs / MCUXpresso?
**Verdict: CORRECTED.** The study said "not found." NXP does have an offering.
- NXP's "Agentic AI Development" page says NXP updated **MCUXpresso for VS Code** and **S32 Design Studio** to support agentic AI. "The feature is currently available as an experimental beta." No device families are named. — https://www.nxp.com/design/design-center/software/agentic-ai-development:AGENTIC-AI
- MCUXpresso for VS Code provides "VS Code Agent and Copilot views," "MCUXpresso SDK project skills," "Zephyr project skills" and "use case examples." This is a GitHub Copilot agent plus skill files; no MCUXpresso-specific MCP server is named.
- MCP appears elsewhere on the same page:
  - S32 Design Studio (automotive S32): "MCP gateway to IDE, compiler, configuration and debug tools"
  - FreeMASTER: "MCP server for FreeMASTER interaction"
  - an "MCP Server for Documentation" (FAQ)
- A community tutorial describes a "MCUXpresso — Combine Examples" agent that runs in Copilot Chat using MCUXpresso skill files and tool sets. I saw this in a search snippet; the page returned 403. — https://community.nxp.com/t5/Agentic-AI-Development/tkb-p/agentic-ai-development
- Earlier, NXP application note AN14859 (Rev. 1.0, 5 Nov 2025) showed generic GitHub Copilot agent-mode use on the FRDM-MCXA346. It contains no MCP. — https://www.nxp.jp/docs/en/application-note/AN14859.pdf
- NXP's eIQ Agentic AI Framework is for on-device edge AI, not an IDE, and supports MCP. EE Times Japan, February 2026. — https://eetimes.itmedia.co.jp/ee/spv/2602/16/news053_2.html
- Implication for study 16: NXP belongs in the chip-vendor AI table. Its pattern is the same as the others: skills plus MCP plus existing IDE, in beta.
