# Verification: IoT platform retreats and deal facts (study 16, 待驗證 → 平台與交易)

Checked on 2026-11-20 against the open items in `research/16-global-evidence.zh-TW.md` (sections 三 and 五, and the 待驗證 list) and the notes `iot_platform_retreats.md` and `mna_spinoff_precedents.md`.

Verdicts: **CONFIRMED** = a primary or near-primary source supports the study's claim. **CORRECTED** = the study or notes need a change. **STILL UNVERIFIED** = no adequate source found; what was tried is listed. Lines marked (inference) are my reasoning, not something a source states.

## Summary

| # | Item | Verdict | Fact |
| --- | --- | --- | --- |
| 1 | Azure IoT Central retirement | CONFIRMED (plus a date refinement) | Retires 2029-09-20. New app creation stopped 2026-09-23. Microsoft IoT blog post dated 2026-09-25; Message Center AZ569914 dated 2026-09-29. |
| 2 | Greengrass V1 end date | CONFIRMED: 2026-10-07 | The V1 and V2 developer guides give 2026-10-07; the V2 migration guide already says support "ended". The marketing page's 2026-06-01 is stale. |
| 3 | PTC → TPG price | CORRECTED | US$600M cash plus up to US$125M contingent, so US$725M at most, not ≈730M. PTC received US$523M at closing (2026-03-16) and about US$375M after tax. |
| 4 | Nordic–Memfault price | CONFIRMED US$120M | Oslo Børs notice: USD 120M cash and debt-free. Closed 2025-07-01. PitchBook's US$138M has no primary support. |
| 5 | TASKING price | CONFIRMED US$110M | US$100M upfront plus US$10M earnout. US$146M is probably the AUD conversion (inference). Divested in Feb 2021. |
| 6 | Golioth funding / Canonical | CORRECTED (funding); CONFIRMED (deal) | US$2.5M pre-seed (2021) plus US$4.6M seed (2023) = US$7.1M per the company, not ≈7.3M. Canonical deal announced 2026-03-03, terms undisclosed. |
| 7 | Qualcomm–Arduino closing | CONFIRMED closed (previously unknown) | Qualcomm said "we completed our acquisition of Arduino" on its Q4 FY25 call on 2025-11-05. No standalone closing press release was found. |
| 8 | TouchGFX licence | CONFIRMED (via secondary copies of ST text) | Free of charge, but only for use with STM32. Licensed under SLA0048. The library checks at runtime that it is running on an STM32. |
| 9 | Toitware ApS | CORRECTED / refined | CVR 38984594 went into bankruptcy on 2024-08-20 and was dissolved after bankruptcy on 2025-10-28. Toit itself continues as open source (v2.0.0, 2026-10-31). |
| 10 | IA / ThingsBoard / LVGL figures | Mostly STILL UNVERIFIED | ThingsBoard says it had 110 employees in 2025 (company-sourced). No audited revenue for any of the three. LVGL now also has a UK entity, LVGL LTD, incorporated 2025-12-08. |
| 11 | Rightware ownership; China-risk concern | CONFIRMED (ownership); STILL UNVERIFIED (customer concern) | Still a ThunderSoft subsidiary (2025 references). No reported US automaker or medical-customer concern found. The BIS rule could reach Kanzi only if it runs on VCS-enabling units (inference). |

---

## 1. Azure IoT Central retirement: CONFIRMED, with a date refinement

- Microsoft IoT blog (Tech Community, Microsoft employee author "JesusbarMsft", dated 2026-09-25): "New IoT Central application creation is unavailable starting September 23, 2026." Also: "After September 20, 2029, IoT Central applications are no longer available." Recommended targets: Azure IoT Hub plus DPS, Azure Device Registry (preview), Event Grid/Functions/Fabric Activator, and Fabric Real-Time Intelligence/Power BI. https://techcommunity.microsoft.com/blog/iotblog/azure-iot-central-is-evolving-what-you-need-to-know/4554570
- Message Center AZ569914, "Retirement: Azure IoT Central will be retired on September 20, 2029" (third-party mirror), published 2026-09-29. https://mc.merill.net/message/AZ569914
- Microsoft Learn: the IoT Central overview page has no retirement banner (ms.date 2025-05-09, not updated since). No Learn or Azure Updates page carrying the 2029 date was found. https://learn.microsoft.com/en-us/azure/iot-central/core/overview-iot-central
- History: in February 2024 a console notice gave 2027-03-31, and Microsoft retracted it as "presented in error". https://www.theregister.com/2024/02/15/microsoft_retires_azure_iot_central/ ; https://techcommunity.microsoft.com/blog/iotblog/microsofts-commitment-to-azure-iot/4059725
- **Edit to the study:** the source can now be the first-party Microsoft blog rather than only the "第三方鏡像". The table's announcement date of 2026-09-29 is the Message Center date. The blog post is dated 2026-09-25, and new app creation was already closed from 2026-09-23.

## 2. AWS IoT Greengrass V1 end date: CONFIRMED 2026-10-07

- The V1 maintenance-policy page banner reads: "On October 7th, 2026, AWS will discontinue support for AWS IoT Greengrass Version 1. After October 7th, 2026, you will no longer be able to access the AWS IoT Greengrass V1 resources." The body text below the banner still carries the older "no plan to stop supporting… until further notice" wording, which is internally stale. https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html
- The V2 migration guide, already past tense: "On October 7, 2026, AWS ended support for AWS IoT Greengrass Version 1. The AWS IoT Greengrass V1 API operations and resources are no longer available." https://docs.aws.amazon.com/greengrass/v2/developerguide/migrate-from-v1.html
- The product marketing page still says "On June 1st, 2026, AWS will discontinue support for Greengrass V1." It was not updated after the date moved. https://aws.amazon.com/greengrass/
- **Edit to the study:** drop "（產品頁另寫 06-01）" or label it as a stale marketing page. As of 2026-11-20 the service is shut down.

## 3. PTC sale of ThingWorx and Kepware to TPG: CORRECTED

- SEC Form 8-K, 2025-11-05 (primary): the buyer is Parrot US Buyer, L.P., "an entity controlled by investment funds affiliated with TPG Global, LLC". The price is "$600,000,000 in cash" at closing, subject to adjustments (working capital, indebtedness, a US$35M cut if monthly performance metrics were missed, and an accounts-receivable adjustment), plus "contingent consideration in an amount not to exceed $125,000,000" payable on a later sale of the business by the buyer. https://www.sec.gov/Archives/edgar/data/857005/000119312525267120/ptc-20251105.htm
- Completion: PTC announced completion on 2026-03-16 and "received cash proceeds of $523 million upon closing", about US$375M after roughly US$40M of costs and US$110M of cash taxes. https://www.digitalengineering247.com/article/ptc-finalizes-divestiture-of-kepware-thingworx-businesses/internet-of-things (PTC's own page returns 403 to automated fetch: https://www.ptc.com/en/news/2026/ptc-completes-divestiture-of-kepware-and-thingworx-businesses)
- Trade press used the same maximum: "up to $725m". https://www.rcrwireless.com/20251111/internet-of-things/ptc-sells-kepware-thingworx
- **Edit to the study:** the study's "約 US$6 億加最多 US$1.25 億業績款" is correct. Replace "價格為二手來源" with the SEC 8-K, and add "交割實收 US$5.23 億". In the notes, change "about $730M" to "max $725M". Note that the earnout depends on TPG's future exit, not on operating performance.

## 4. Nordic–Memfault: CONFIRMED US$120M

- Nordic's Oslo Børs stock-exchange notice, 2025-06-24 (Inderes copy): "for a consideration of USD 120 million on a cash and debt-free basis". Financed with "cash at hand and a new USD 110 million bridge loan facility with Danske Bank". Memfault's ARR was "USD 7.2 million at the end of 2024", "expected to increase by more than 50% in 2025". The founders reinvest 30% of their proceeds, about US$13M, in Nordic shares. No earnout is mentioned. https://www.inderes.dk/releases/nod-nordic-semiconductor-acquires-memfault-launches-first-complete-chip-to-cloud-platform-for-lifecycle-management-of-connected-products ; Euronext copy: https://live.euronext.com/en/products/equities/company-news/2025-06-24-nod-nordic-semiconductor-acquires-memfault-launches-first
- Closing notice, 2025-07-01. https://www.inderes.dk/releases/nod-closing-of-acquisition-of-memfault ; https://live.euronext.com/en/products/equities/company-news/2025-07-01-nod-closing-acquisition-memfault
- The US$138M figure (PitchBook) is paywalled and has no primary support. Treat it as unreliable.
- New derived figure (inference): 120 / 7.2 ≈ 16.7x trailing ARR. Of all disclosed deals in study section 五, this is the highest revenue multiple.

## 5. Altium → FSN Capital (TASKING): CONFIRMED US$110M

- Altium release (iConnect007 copy, Dec 2020): sale of the TASKING business assets to FSN Capital for US$110M, all cash. US$100M was paid upfront and US$10M depended on FY2021 revenue targets. Closing was expected in Q1 of calendar 2021. https://iconnect007.com/index.php/article/125627/altium-divests-non-core-tasking-to-support-enable-investment-in-altium-365/125630
- FSN's own release: https://www.fsncapital.com/en/news/fsn-capital-v-has-signed-an-agreement-to-acquire-100-of-tasking
- Altium's FY21 annual report says TASKING was divested in February 2021. https://renesas.com/ja/document/rep/altium-annual-report-2021 (seen only in a search summary; the exact completion day was not read)
- About US$146M (inference): US$110M at the December 2020 AUD/USD rate of about 0.75 is about A$146M. The aggregator figure is probably the AUD amount labelled as USD. I did not confirm the aggregator's working.

## 6. Golioth funding and Canonical acquisition: funding CORRECTED, deal CONFIRMED

- 2021 round: Golioth's blog calls it a "Pre-Seed" of US$2.5M, led by Zetta Venture Partners, dated 2021-05-31 (PR Newswire on 2021-06-02 called it a "seed"). https://blog.golioth.io/golioth-announces-seed-and-developer-beta ; https://prnewswire.com/news-releases/golioth-a-cloud-platform-for-iot-announces-private-beta-and-2-5m-seed-301304014.html
- 2023 round: US$4.6M seed led by Blackhorn Ventures and Differential Ventures, with Zetta, MongoDB Ventures and Lorimer. The post, dated 2023-05-04, says Golioth has "raised a total of $7.1 million". https://blog.golioth.io/may-2023-golioth-seed-round/
- Conflicting trackers: CB Insights has US$4.8M and US$7.3M total; StartupIntros has US$5M and US$9.5M. https://www.cbinsights.com/company/golioth/financials
- Canonical deal: Golioth blog, 2026-03-03, "Golioth is now a part of Canonical". Terms undisclosed. The platform and APIs "continue without disruption". https://blog.golioth.io/golioth-is-now-a-part-of-canonical ; Canonical's later blog presents Golioth as part of its stack (a copy dated 2026-06-18): https://canonical.com/blog/microcontrollers-ubuntu-core-golioth ; Hackster: https://www.hackster.io/news/iot-specialist-golioth-gets-snapped-up-by-linux-goliath-canonical-4c738a5a2935
- **Edit to the study:** change "種子合計約 US$730 萬" to "pre-seed US$250 萬 + seed US$460 萬，合計 US$710 萬（公司自述）".

## 7. Qualcomm–Arduino closing: CONFIRMED closed

- Qualcomm's Q4 FY2025 earnings call, 2025-11-05 (official IR transcript PDF): "In industrial IoT, we completed our acquisition of Arduino, a premier open-source hardware and software company with an IoT development ecosystem of more than 30 million users worldwide." https://s204.q4cdn.com/645488518/files/doc_financials/2025/q4/Q4FY25-Earnings-Call-Transcript_11-6-25_Final.pdf
- Arduino FAQ: "Yes, Qualcomm Technologies, Inc. has acquired Arduino." The FAQ gives no closing date. https://www.arduino.cc/qualcomm-faq/
- Announcement, 2025-10-07; the deal was subject to regulatory approval, and terms were not disclosed. https://www.qualcomm.com/news/releases/2025/10/qualcomm-to-acquire-arduino-accelerating-developers--access-to-i
- The exact closing day was not found. It falls between 2025-10-07 and 2025-11-05, and no standalone closing press release was found. Note that the user count was "33M+" at announcement and "more than 30 million" on the call.

## 8. ST TouchGFX licence: CONFIRMED (free, STM32 only)

- ST press release, 2018-11-28 (p4101): ST is "making the software available free of charge for production and redistribution with STM32 MCUs." https://www.st.com/content/st_com/en/about/media-center/press-item.html/p4101.html (st.com timed out for automated fetch; wording taken from a search-result excerpt)
- Licence: TouchGFX is delivered under "Mix Ultimate Liberty+OSS+3rd-party V1" (SLA0048). Third-party components carry MIT, BSD or Apache terms; ST components carry SLA0048. https://www.st.com/resource/en/additional_license_terms/additional-license-terms-x-cube-touchgfx-v4-27.html (not fetchable; seen in search) ; https://community.st.com/stm32-mcus-touchgfx-and-gui-33/clarification-on-licensing-for-touchgfx-and-stm32cubemx-in-commercial-products-145152
- ST Technical Moderator, 2024-03-06: "You cannot port TouchGFX on other platforms or MCUs as it's an ST propriety… in the library there is a check at runtime if the MCU is an STM32 device." https://community.st.com/stm32-mcus-touchgfx-and-gui-33/porting-touchgfx-code-to-non-st-mcus-129161
- Caveat: I could not retrieve the SLA0048 text itself, because st.com blocked automated download. The STM32-only rule therefore rests on ST's press release, ST moderator statements and the runtime check, not on clause text I read.

## 9. Toitware ApS: CORRECTED / refined (bankrupt 2024, dissolved 2025)

- CVR data (third-party mirror danskcvrapi.dk; the official datacvr.virk.dk returned 403 to automated fetch): Toitware ApS, CVR 38984594, credit status "Under konkurs" from 2024-08-20. Creditors were summoned on 2025-03-20 and 2025-08-21, bankruptcy proceedings closed on 2025-07-04 and 2025-09-23, and the company was dissolved after bankruptcy and struck off on 2025-10-28 ("2017–2025"). Its last published accounts are for 2022, a net result of DKK −1.8M. https://danskcvrapi.dk/nyt/38984594-kredit-2024-08-20-under-konkurs ; https://danskcvrapi.dk/en/news/38984594-ophor-2025-10-28 ; https://danskcvrapi.dk/en/news/38984594-kredit-2025-10-28-opl-st-efter-konkurs ; official lookup: https://datacvr.virk.dk/enhed/virksomhed/38984594
- The Toit language continues as open source. GitHub `toitlang/toit` had commits on 2026-11-14 by Florian Loitsch (toit.io address) and released v2.0.0 on 2026-10-31. Jaguar was last pushed on 2026-10-31. https://github.com/toitlang/toit ; https://github.com/toitlang/toit/releases ; https://github.com/toitlang/jaguar
- No company shutdown statement was found.
- **Edit to the study:** replace "只見 Wikipedia 草稿鏡像" with "Toitware ApS 2024-08-20 進入破產、2025-10-28 破產後解散（CVR 38984594）；語言本身由原創辦人以開源持續維護".

## 10. Revenue and headcount: Inductive Automation, ThingsBoard, LVGL Kft (mostly STILL UNVERIFIED)

**Inductive Automation (private; Folsom, CA)**
- Company-sourced (dated): press release 2017-01-19 says it had "over 100" employees, up from 20 when it moved to Folsom, and that revenue had grown "more than 60% per year" on average since Ignition launched in 2010. https://inductiveautomation.com/news/inductive-automation-buys-larger-building-headquarters
- Current headcount, third-party estimates only: Revelio Labs 423 (https://www.reveliolabs.com/companies/inductive-automation/employees/); Growjo 511 (https://growjo.com/company/Inductive_Automation); Built In 240 (https://builtin.com/company/inductive-automation).
- Revenue estimates range from US$5–25M (Owler) to US$225M (Growjo) and are not usable. No filing exists, and no Sacramento Business Journal or Inc. 5000 entry was found.

**ThingsBoard, Inc. (New York HQ address)**
- Company timeline (company-sourced): 10 employees (2017), 50 (2022), 65 (2023), 100 (2024), 110 (2025). The page lists 83 people in its team section and gives no revenue. https://thingsboard.io/company/
- Revenue estimates only: ZoomInfo under US$5M; LeadIQ US$10–25M. https://www.zoominfo.com/c/thingsboard/465417188

**LVGL Kft (Budapest)**
- The Hungarian register (e-cegjegyzek, Opten, CompanyWall) was not reachable by search or fetch, and no registration number, revenue or headcount was found.
- New fact: lvgl.io/about now names LVGL Ltd. (Cambridge, UK) for commercial operations and LVGL Kft. (Üllői út 71, Budapest) as the engineering arm, and lists 11 core team members. https://lvgl.io/about
- UK Companies House: LVGL LTD, company number 16897003, incorporated 2025-12-08. Directors are Gabor Kiss-Vamosi and Mihaly Szilagyi, both Hungarian. No accounts are due yet. https://find-and-update.company-information.service.gov.uk/company/16897003/officers
- Hungarian IP office (SZTNH) notice, Nov 2025: LVGL Kft. applied for the word mark "LVGL". https://sztnh.gov.hu/sw/static/file/H_02_Vedjegy_gyorsitott_meghirdetes_22_2511.pdf
- Still to do: check e-cegjegyzek.hu or Opten by hand for LVGL Kft's beszámoló (filed annual accounts), and Companies House for LVGL LTD's first accounts (due around 2027).

## 11. Rightware (Kanzi): ownership CONFIRMED; customer concern STILL UNVERIFIED

- Ownership: ThunderSoft (中科创达, SZSE 300496) bought Rightware in December 2016 for about €64M. https://techcrunch.com/2016/12/19/os-builder-thundersoft-buys-ui-maker-rightware-for-68m-to-expand-in-automotive/ A 2025 report still calls it a "中科创达子公司Rightware" (MarkLines, 2025-05-08). https://www.marklines.com/cn/news/325959 No divestment news for 2025–2026 was found. Rightware won a 2026 iF Design Award (AquaDrive AIOS). https://www.drivingvisionnews.com/news/2026/03/26/rightware-aquadrive-award-intellisphere-reality/
- I could not open the subsidiary list in ThunderSoft's 2025 annual report (filed 2026-04-21); the cninfo full text was not located. https://finance.eastmoney.com/a/202604213712842030.html
- BIS Connected Vehicle rule (15 CFR 791 subpart D; final rule 2025-01-16; software prohibitions start with model year 2027; software requirements effective 2026-03-17):
  - "Covered software" means "software-based components, including application, middleware, and system software, in which there is a foreign interest, executed by the primary processing unit or units of an item that directly enables the function of Vehicle Connectivity Systems or Automated Driving Systems". Firmware and unmodified open-source software are excluded. https://www.govinfo.gov/content/pkg/FR-2025-01-16/html/2025-00592.htm
  - The preamble says an infotainment module "if [it] included a cellular module for its own wireless communication… could be considered VCS". Same source.
  - Effective-date guidance: https://www.venable.com/insights/publications/2026/02/bis-connected-vehicle-rule-requires-vehicle ; https://www.bis.gov/connected-vehicles
- Inference: Rightware is a PRC-controlled person. Kanzi is UI middleware that often runs on infotainment or cockpit SoCs. If such a unit integrates cellular, Wi-Fi or Bluetooth, Kanzi could fall under "covered software" for US-market vehicles from MY2027. Proprietary Kanzi would not qualify for the open-source exclusion. This is my reading of the rule text and needs legal review.
- Concern by automakers or medical customers: no public report, statement or article was found that links Rightware or Kanzi to the BIS rule or to China-risk procurement concerns. Searches tried: "Rightware Kanzi BIS connected vehicle rule", "ThunderSoft connected vehicle rule", "Rightware Kanzi news 2026". Nothing medical-specific was found either. The rule applies only to vehicles; no FDA equivalent was found.
