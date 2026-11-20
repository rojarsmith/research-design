# Verification round 3: primary pages read in the built-in browser

Date: 2026-11-20. Method: the desktop app's built-in browser (a real browser pane), used read-only. I did not sign in, fill forms or solve bot checks. I clicked no cookie banners, because none of them offered a reject option. Two PDFs that the browser would only download (no inline view) I read with an in-page `fetch` from the same origin and decoded in memory. Nothing was saved to disk. Exception: when the browser was first pointed at the acq.osd.mil PDF and the smeg.org.tw PDF, the app showed a save dialog. I did not accept either dialog and did not retry those URLs.

Claims being checked are in `research/16-global-evidence.zh-TW.md`. Earlier rounds are in `verification_*.md` in this folder.

## Summary

| # | Item | Verdict | Fact |
|---|---|---|---|
| 1 | IEA GEVO 2026, charging chapter | STILL BLOCKED | Cloudflare "Performing security verification" page with a "Verify you are human" checkbox (HTTP 403). Not solved. The study's 2025 figures stay as a secondary source. |
| 2 | Omdia Jan 2026 industrial display PR | STILL BLOCKED (page gone) | The URL now returns 404, and the release is missing from Omdia's newsroom list for Jan 2026. The study's figures stay with Display Daily as the source. |
| 3 | Gartner 7033498 abstract | CORRECTED (quote not in abstract) | The public abstract does not contain "vibe coding". Bibliographic details confirmed: published 07 October 2025, analysts Adrian Leow and Kyle Davis. |
| 4 | TouchGFX licence SLA0048 | CONFIRMED, with nuance | Clause text read on st.com. It limits use to devices "manufactured by or for ST". That covers STM32 MCUs and also ST MPUs and other ST processing units, so "ST devices only" is more exact than "STM32 only". |
| 5 | Toitware ApS CVR 38984594 | STILL BLOCKED | datacvr.virk.dk shows the same Cloudflare check (HTTP 403). The Statstidende search would need typing into a form, which is out of scope. The status still rests on the third-party mirror. |
| 6 | FAR case for the Cyber Trust Mark | CONFIRMED (none exists) | Open FAR Cases report "as of November 06, 2026": no case mentions IoT, Internet of Things, Cyber Trust Mark, labeling, EO 14144 or EO 14306. |
| 7a | 青年創業貸款 terms | CONFIRMED + NEW FINDING | Full text of the 要點 in force from 115-07-01 read: 9.5成 guarantee up to NT$150萬 of new loans, loan caps, interest, terms, subsidy. Details below. |
| 7b | TTA admission conditions | NEW FINDING (TTA South only) | TTA South (Shalun, Tainan) publishes conditions: company registered under 8 years, or a team selected by a government programme or competition. TTA Taipei publishes no admission criteria. |

## 1. IEA Global EV Outlook 2026, EV charging chapter

- URL: https://www.iea.org/reports/global-ev-outlook-2026/electric-vehicle-charging-chap-6-and-10
- Result: page title "Just a moment...", text "Performing security verification ... This website uses a security service to protect against malicious bots." It shows a Cloudflare "Verify you are human" checkbox and returns HTTP 403. After a 6-second wait the page was unchanged. I did not click the checkbox.
- Verdict: STILL BLOCKED. Keep the study's line as it is: 2025 near 1.8 million added, more than 33% growth, end-2025 stock above 7 million, marked as a secondary source from the search snippet. The Europe, US and China split for 2025 remains unverified.

## 2. Omdia January 2026 industrial display press release

- URL: https://omdia.tech.informa.com/pr/2026/jan/industrial-display-revenues-grow-24percent-yoy-in-2025-led-by-auo-display-plus
- Result: HTTP 404, "404 - Page not found Omdia".
- I checked the newsroom list (https://omdia.tech.informa.com/pr?page=1&perPage=300). It has 13 releases dated `/pr/2026/jan/`, and none is about industrial displays. Display-related January items: "large-area-display-shipments-grew-2point9percent-year-over-year-in-2025" and "smartphone-amoled-shipments-to-fall-in-2026...". The release appears to have been removed or moved.
- The newsroom does still list the earlier industrial display forecast release, from Jan 2025: https://omdia.tech.informa.com/pr/2025/jan/ecigarettes-game-consoles-and-2-wheeled-vehicles-drive-industrial-display-panel-revenue-to-usd-2point4-billion-up-13point6percent-year-on-year-in-2025. Its newsroom summary says shipments were "projected to reach 167.9 million units". I did not open this page. It is a 2025 forecast, not the 2025 actual.
- Verdict: STILL BLOCKED. The study's 2025 actuals (US$3.4B, +24%; units −0.4%) still rest on Display Daily. I could not confirm a smart home and office share from Omdia.
- Suggested study note: the Omdia original URL now returns 404 and the release is not in Omdia's newsroom list.

## 3. Gartner document 7033498

- URL: https://www.gartner.com/en/documents/7033498. The page opened normally (no 403 this time).
- Title: "Why AI Won't Replace the Need for Low-Code Application Platforms". Published: 07 October 2025. Analysts: Adrian Leow, Kyle Davis.
- The public summary is two sentences. It says AI code assistants and agents "have led some to question the need for low-code application platforms", and that the research explains why the platforms remain critical. The table of contents lists Overview, Strategic Planning Assumption, Introduction and Analysis.
- A search of the full page text for "vibe" found 0 matches.
- Verdict: the bibliographic details are CONFIRMED. The "vibe coding accelerates typing; LCAPs accelerate engineering" line is NOT in the public abstract. If it exists, it is in the paywalled body or in vendor paraphrases. The study should keep it marked as unverified, or drop it.

## 4. ST TouchGFX licence (SLA0048)

- URL (opened, then redirected to the ccc path): https://www.st.com/resource/en/additional_license_terms/additional-license-terms-x-cube-touchgfx-v4-27.html
- Resolved URL: https://www.st.com/content/ccc/resource/legal/legal_agreement/additional_license_terms/group1/82/53/5d/8a/62/6d/45/4d/additional-license-terms-x-cube-touchgfx/files/additional-license-terms-x-cube-touchgfx-v4-27.html/jcr:content/translations/en.additional-license-terms-x-cube-touchgfx-v4-27.html
- Page title: "Package License Terms for STM32Cube.TOUCHGFX". Copyright 2026. It includes "SLA0048 Rev4/March 2018 ... SOFTWARE PACKAGE LICENSE AGREEMENT".
- Restriction clause, as written on the page: "The SOFTWARE PACKAGE or any part thereof, including modifications and/or derivative works of the SOFTWARE PACKAGE, must be used and execute solely and exclusively on or in combination with a device or a processing unit device manufactured by or for ST, including without limitation processing units which are part of a sensor and/or microcontrollers and/or microprocessors manufactured by or for STMicroelectronics." (Notes file only. When citing in the study, quote a short phrase such as "solely and exclusively on or in combination with a device ... manufactured by or for ST".)
- In the SBOM on that page, ST's board setups, demo applications, examples and graphical images are listed under SLA0048. The third-party parts of the core framework are MIT or BSD (Penner, QR code, QOI) or Nema.GFX (Annex 5).
- The same clause also bans any use that would put the package under open-source terms ("No use, reproduction or redistribution ... in any manner that would subject the SOFTWARE PACKAGE to any Open Source Terms").
- Verdict: CONFIRMED, with a nuance. The licence binds use to ST silicon ("manufactured by or for ST"). That includes STM32 MCUs and STM32MP MPUs, so "only on ST chips" is the exact reading. The study's "只限 STM32" is right in practice. Suggested wording: 只限 ST 自家晶片（以 STM32 為主）.

## 5. Toitware ApS, CVR 38984594

- URL: https://datacvr.virk.dk/enhed/virksomhed/38984594
- Result: Cloudflare check "datacvr.virk.dk / Performing security verification" with a "Verify you are human" checkbox (HTTP 403). It was unchanged after a 6-second wait. Not solved.
- Fallback tried: https://www.statstidende.dk/messages?term=38984594. The page loaded, but the term was not applied ("0 resultater"). Searching would need typing into the search form, which these browsing rules exclude.
- Verdict: STILL BLOCKED. Keep the dates (konkurs 2024-08-20, ophørt 2025-10-28) marked as a secondary source from the danskcvrapi.dk mirror.

## 6. US Open FAR Cases: Cyber Trust Mark or consumer IoT purchasing

- Report URL: https://www.acq.osd.mil/dpap/dars/opencases/farcasenum/far.pdf (linked as "FAR Open Cases Report" from https://www.acq.osd.mil/dpap/dars/far_case_status.html, which was last updated January 21, 2026).
- The browser offered the PDF only as a download, so I read it with an in-page fetch and in-memory decoding. Report header: "Open FAR Cases as of November 06, 2026".
- The text has about 29 open case numbers. I searched it for cyber, Internet of Things, IoT, "Trust Mark", 14144, 14306, label and consumer. No case concerns consumer IoT, the Cyber Trust Mark, EO 14144 or EO 14306. The only cyber-related cases:
  - 2019-014, "EO Strengthening America's Cybersecurity Workforce" (EO 13870). Status 08/07/2026: "Case on hold. Case will be implemented in Revolutionary FAR Overhaul Rule 2026-001 published on 06/23/2026 (91 FR 37550)."
  - 2021-019, "Standardizing Cybersecurity Requirements for Unclassified Federal Information Systems" (EO 14028 sec. 2(i) and (b)). Status 08/12/2026: the DARC Chair re-tasked the Ad Hoc Team to revise the draft final FAR rule "due to emerging policy decisions".
- Other open cases are unrelated: CUI 2017-016, FASCSA 2019-018 and 2020-011, Section 889 2019-009 and 2018-017, PNT services 2024-005, drones 2024-002, and the Revolutionary FAR Overhaul part cases 2026-005 to 2026-012.
- Verdict: CONFIRMED. There is no open FAR case implementing the Cyber Trust Mark requirement for federal purchases as of 2026-11-06. This is consistent with the study's "沒找到 FAR 規則". The report also shows the Revolutionary FAR Overhaul is now absorbing cases, which may be where an implementation would go. The closed-cases report "has been taken down temporarily for corrections", so I could not check whether such a case was opened and closed.

## 7a. 青年創業貸款 (current terms from 115-07-01 = 2026-07-01)

Sources (all official):
- MOEA press release, 2026-06-29, "強化對青年金融支持 青創貸款9.5成保證額度提高至150萬元": https://www.moea.gov.tw/MNS/populace/news/News.aspx?kind=1&menu_id=40&news_id=123170
- SMEA youth-loan portal (0800-280-280): https://0800280280.sme.gov.tw/0800/disaster/index.php?area=youthloan
- 要點 full text, "1150701青年創業貸款要點(含沿革)": https://0800280280.sme.gov.tw/upload/1150701青年創業貸款要點(含沿革).pdf. Read in-page; the same file is linked from https://www.sme.gov.tw/article-tw-2570-4238 (updated 2026-11-16) and from the 信保基金 page https://www.smeg.org.tw/basic/?node=10193.

Facts:
- Amendment history (沿革): the latest amendments are the 115-06-04 令 (effective 115-09-01) and the 115-06-18 令 (effective 115-07-01).
- 貸款對象 (point 3): an SME under the 中小企業認定標準, excluding finance, insurance and special entertainment businesses, set up less than 8 years ago. The representative or owner must be aged 18 to 45 and hold at least 20% of paid-in capital personally. Taiwan nationals need at least 20 hours or 2 credits of government-recognised startup courses within 3 years. Non-nationals need a 創業家簽證, 就業金卡 or valid residence permit.
- 額度 (point 5): working capital up to NT$600萬. Capital spending up to 80% of the plan budget, capped at NT$1,200萬. Different companies with the same representative share the combined cap.
- 期限 (point 6): working capital up to 6 years, including up to 2 years' grace. Plant or premises up to 20 years, including up to 5 years' grace. Machinery and equipment up to 7 years, including up to 3 years' grace.
- 利率 (point 7): 中華郵政 two-year time-deposit floating rate + 0.575%, floating.
- 保證 (point 8): guarantee ratio 8成 to 9成 in general. For new loans after 115-07-01, the first NT$150萬 per borrower is guaranteed at 9.5成, or 10成 under the 信保 batch-guarantee scheme. Guarantee fee is charged at the bottom of the 信保 fee range. At NT$150萬 or below, no guarantor other than the representative may be required; above that, one guarantor as a rule.
- 申貸 (point 9): from 115-07-01, loans of NT$150萬 or below may use the short application form instead of a business plan.
- 利息補貼 (point 10): interest subsidy on new loans up to NT$100萬 for up to 5 years. Above NT$100萬, the subsidy is calculated on NT$100萬. It stops when the budget runs out. Note that the guarantee cap rose to NT$150萬 but the subsidy base stayed at NT$100萬.
- Press release context: from 109 to the end of May 115, 120,508 guaranteed cases, about NT$956.01億 guaranteed, about NT$1,028.83億 financed. The average loan rose from NT$87.1萬 (109) to NT$149萬 (115).
- Verdict: CONFIRMED. The study's "9.5成保證上限自 2026-07-01 從 NT$100 萬提高到 NT$150 萬" (currently cited to 旺得富) now has a primary source, the MOEA release and the 要點. NEW FINDING: loan caps, interest, terms, the subsidy base of NT$100萬 and the eligibility rules above.

## 7b. Taiwan Tech Arena (TTA) admission conditions

- TTA main site https://www.taiwanarena.tech/ and https://www.taiwanarena.tech/about/: no admission criteria. The site describes TTA as a programme "launched by National Science and Technology Council in 2018" in Taipei (No.2, Sec. 4, Nanjing E. Rd.). For space pricing it says to "Contact us". The cookie banner offers only "Continue" (no reject option), so I left it alone.
- TTA South application guideline: https://apply2ttas.taiwanarena.tech/ (linked from the TTA home page).
  - Location: Shalun Smart Green Energy Science City, Zone C (Tainan), 6F of the Cyber Security & Smart Technology R&D Building. Run for NSTC.
  - Fields: smart technology, precision health, net-zero and circular tech.
  - Startups qualify if (1) the company has been registered for less than 8 years and is willing to commercialise products or services through empirical proof of concept, or (2) it is a new team selected by a government business-plan programme or competition. Enterprises qualify if willing to cooperate with startups.
  - Lease period: startups 2 years plus a 1-year extension; enterprises 3 years plus up to a 3-year extension.
  - Review: an NSTC review panel with an in-person briefing. Written review only for TTA-coached teams, regulatory-sandbox or 5G private-network applicants, government competition winners, national award winners, and listed or Global 500 companies. Criteria: innovation, business-model feasibility and market potential, corporate collaboration, and use of nearby PoC fields.
  - Fees: co-working NT$2,000 a month (open seat) or NT$2,500 a month (booth), tax included. Private office "NT$600/month including tax" (read as per ping) plus a NT$130 per ping management fee plus utilities. Company registration is allowed at the base.
- Verdict: NEW FINDING for TTA South. TTA Taipei still has no published admission conditions.

## Browser hygiene

- One tab was used (the pane's seed tab) and closed at the end.
- No sign-ins, form entries or submissions. No bot checks solved. No files saved.
