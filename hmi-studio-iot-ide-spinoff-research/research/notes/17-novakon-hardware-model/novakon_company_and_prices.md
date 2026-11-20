# NOVAKON (速博康科技) and iFace Designer: company, hardware, prices, financials

Research date / access date for all URLs: **2026-11-20** (unless stated otherwise).
Purpose: test whether a spun-off EDT HMI Studio software company could copy NOVAKON's model (free HMI design software, revenue from HMI hardware).

Evidence labels:
- **[P]** primary (company or government page, regulatory or stock-exchange filing)
- **[S]** secondary (news, marketplace or distributor listing, trade data)
- **[V]** vendor claim (marketing text, including on the company's own site)
- **[U]** unverified or inferred

---

## 0. Key findings

1. **NOVAKON is not a standalone startup.** It is 速博康科技股份有限公司, a 70%-owned subsidiary of listed industrial-PC maker **iBASE / 廣積科技 (TPEx 8050)**. iBASE is in turn about 26–32% owned by **AAEON / 研揚 (ASUS group)**. [P]
2. **It is small but profitable.** Revenue was **NT$230.6M (about US$7.2M) in FY2025**, with operating income of NT$66.8M (about 29% operating margin). FY2023 was a loss year. [P: iBASE annual reports on MOPS]
3. **The model matches the hypothesis.** iFace Designer is marketed as "FREE HMI development software" [V], and revenue comes from selling N/P-series HMIs, panel PCs and gateways. There is no public paid licence tier for HMI panels. A "runtime" software option is listed for x86 panel PCs, but its terms are not published.
4. **Manufacturing is done inside the group, not by a third-party ODM.** Novakon has manufacturing items in its business registration and says its products are "Taiwan R&D and production" [V]. iBASE sells to Novakon (NT$48.4M in 2022, NT$26.7M in 2023 [P]), and iBASE runs Taiwan plants in Sanchong, Xinzhuang and Pingzhen, plus a Vietnam plant [P]. So production is most likely sourced from the parent [U, inference]. No independent ODM evidence was found.
5. **Prices for new units:** 7" P07 is about US$250–370 and 10" P10 is about US$480–530 at Western resellers. Equivalent Weintek 7" units cost about US$390–710 at US resellers. NOVAKON is roughly **30–50% cheaper** than Weintek. [S]
6. **Security weakness.** In 2025, CyberDanube published five CVEs against the P-series (empty root password, unauthenticated UDP service with RCE as root). The vendor did not respond. [S]

---

## 1. Company facts

### 1.1 Registration (經濟部商工登記 open data, GCIS API)

| Field | Value | Label / source |
|---|---|---|
| Legal name | **速博康科技股份有限公司** (English: NOVAKON Co., Ltd.) | [P] GCIS API (URL below) |
| 統一編號 | **29176695** | [P] GCIS |
| 核准設立日期 | **2010-08-17** (民國 99-08-17) | [P] GCIS. Bressner/DirectIndustry say "September 2010" [S] |
| Status | 核准設立 (active) | [P] |
| 登記機關 | 臺北市政府 | [P] |
| Address | 臺北市南港區園區街3號15樓 (same building as iBASE HQ, 3號15樓之1) | [P] GCIS; bznk.com notes the shared address [S] |
| 資本總額 | NT$168,800,000 | [P] |
| 實收資本額 | **NT$88,800,000** (8,880,000 shares at NT$10) | [P] GCIS, last change approved **2026-05-26** |
| Earlier paid-in capital | NT$68,800,000 (as of 2024-03-06 and at FY2024/FY2025 year-end) | [S] data.bznk.com; [P] iBASE annual reports |
| 代表人 / 董事長 | 林明男, representing 廣積科技股份有限公司 | [P] |
| Directors | 林明男 (Chair), 林秋旭, 陳世雄, 李家富: all representing 廣積科技, which holds **6,216,000 shares (70.0%)**. 許志堅 (natural person) holds 237,832 shares (2.68%). Supervisor: 陳友南 (0 shares). | [P] GCIS directors API |
| Business items | CC01110 電腦及其週邊設備製造業, CC01120, CC01990 其他電機及電子機械器材製造業, F113050/F118010/F119010 wholesale, F213030/F218010/F219010 retail, F401010 國際貿易, I301010–I301030 software and data services, I501010 產品設計, IZ10010, ZZ99999 | [P] GCIS business-items API |
| Factory registration (工廠登記) | None found | [U]: not checked in the factory registry; no secondary site listed one |

URLs:
- GCIS company basic data: https://data.gcis.nat.gov.tw/od/data/api/5F64D864-61CB-4D0D-8AD9-492047CC1EA6?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50
- GCIS directors: https://data.gcis.nat.gov.tw/od/data/api/4E5F7653-1B91-4DDC-99D5-468530FAE396?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50
- GCIS business items: https://data.gcis.nat.gov.tw/od/data/api/236EE382-4942-41A9-BD03-CA0709025E7C?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50
- data.bznk.com (older snapshot, 2024-03-06 state): https://data.bznk.com/29176695-%E9%80%9F%E5%8D%9A%E5%BA%B7%E7%A7%91%E6%8A%80%E8%82%A1%E4%BB%BD%E6%9C%89%E9%99%90%E5%85%AC%E5%8F%B8
- findbiz.nat.gov.tw, twincn.com, info.technews.tw and 104.com.tw all returned 403 or empty pages to the fetcher, so the GCIS open-data API was used instead.

### 1.2 Capital history (from iBASE filings plus GCIS)

| Date | Event | Source |
|---|---|---|
| 2010-08-17 | Incorporated | [P] |
| End 2021 | iBASE holds 81.50% | [P] iBASE FY2022 annual report (Eng.), investment note |
| Dec 2022 | iBASE board approved a "follow-on offering for investment in NOVAKON". iBASE subscribed 1,338k new shares at a different ratio than its holding, so its stake fell from 81.50% to **70.00%**. Other investors (management or employees, e.g. 許志堅) took the rest. | [P] iBASE FY2022 annual report |
| 2023–2025 | Paid-in capital NT$68.8M; iBASE holds 4,816,000 shares (70.00%). Including indirect holdings, iBASE plus controlled companies hold 73.84–73.98%. | [P] iBASE FY2023–FY2025 annual reports |
| 2026 (approved 2026-05-26) | Paid-in capital raised to NT$88.8M (+2.0M shares). iBASE went to 6,216,000 shares, keeping 70.0%, so it subscribed pro rata. | [P] GCIS |

### 1.3 Listing status

- NOVAKON is **not listed**: not on TWSE, TPEx or 興櫃. It does not appear as an issuer on MOPS; it only appears inside the parent's filings. [P, by absence: MOPS searches by its name return only iBASE documents]
- An old 518.com.tw job profile says the company is "planning to list (上市/上櫃)". This is a stale vendor claim with no follow-up evidence. [V]
  https://www.518.com.tw/company-ywnA8k.html

### 1.4 Group structure

- **Parent:** 廣積科技股份有限公司 iBASE Technology Inc. (TPEx **8050**), 統編 70479315. Industrial PC and motherboard maker founded 2000, HQ in 南港. FY2025 consolidated revenue is about NT$5.36B. [P]/[S] (financialfilings.com, Simply Wall St)
- **Grandparent influence:** AAEON 研揚科技 (ASUS group) has held about 28.6% of iBASE's voting power since the 2018 share swap; iBASE's shareholder table lists AAEON at 31.68% (Jul 2022). In turn, iBASE holds 25.83% of AAEON directly (38.63% including indirect). This is a cross-shareholding. ASUSTeK is AAEON's ultimate parent (40.53%). [P] AAEON FY2022 consolidated report (English); iBASE FY2024 annual report
  https://newdata.aaeon.com.tw/DOWNLOAD/prenentations/Y111%20Consolidated%20Financial%20Report_English%20Version.pdf
- **Sister companies** (iBASE affiliates): IBASE Solution 6441 (gaming), IBASE USA, TMC Technology UK, IBASE Japan, IBASE Singapore, IBASE Italy, IBASE Vietnam (factory), IBASE Shanghai. [P] iBASE FY2024 annual report
- **Link to Weintek 威綸, 研華 Advantech or 台達 Delta:** **none found.** NOVAKON claims "20+ years of HMI R&D experience" despite its 2010 founding [V]. An older 518.com.tw profile says "over 15 years HMI development experience" and gives an old address at 新北市新店區寶橋路235巷120號6樓, which is the AAEON campus area [S]. This suggests the team came out of the iBASE/AAEON group, but **no source documents a spin-out from another HMI maker** [U].

### 1.5 People

- General manager **張正明**: received the 19th 國家品牌玉山獎 "傑出企業領導人" national first prize in 2022, and accepted the 26th 經濟部「小巨人獎」 in 2023. [S] 工商時報
- iBASE FY2024 annual report lists Novakon "Director (President)" Chen Chih-Yung. This name appears in a jumbled table and its mapping is uncertain [U].
- Employee count: **20 on 104人力銀行** (via search-result snippet; the 104 page itself returned 403) [S/U]. **30 on 518熊班** (old profile) [S]. Treat as about 20–30 staff.

### 1.6 Awards, positioning and markets

- 2023: one of 12 export-performance winners of the 26th 小巨人獎. [S] https://www.ctee.com.tw/news/20231124700537-431202 (工商時報 2023-11-24)
- 2022: 玉山獎 double honour. [S] https://www.ctee.com.tw/news/20221021700544-431202 (2022-10-21). The article calls NOVAKON a "well-known HMI contract manufacturer (代工) at home and abroad", so it also does ODM/OEM work for others [S/V].
- 2023: first Asian certified edge-device partner of **Siemens Industrial Edge**. The NPP-156P01 and NPP-215P01 panel PCs are "Siemens certified". [V] https://www.novakon.com.tw/en/about
- Markets: "sold in **18 countries**, overseas sales **>90%**" [V] (English About page). "Mainly USA, Europe, Taiwan" [V/S] (OKmarts, EP Solutions). 518 profile: "Europe, Americas, China; expanding to Japan" [V].
- Taiwan R&D and production claim: "臺灣研發、生產" [V] https://www.novakon.com.tw/tw/about

### 1.7 Overseas channel and distributors

| Country | Partner | Evidence |
|---|---|---|
| USA | **Novatek Controls, Inc.**, 1090 Lawrence Dr. Suite 103, Thousand Oaks, CA 91320: sole US consignee in customs B/L data. Sample shipment 2025-11-05: 84 cartons, 613 kg, Kaohsiung to Los Angeles, HTS 8473.30. | [S] Panjiva https://cn.panjiva.com/Novakon-Co-Ltd/45298805 (157 US shipment records; descriptions redacted). The distributor role is inferred [U]. |
| Germany / EU | **BRESSNER Technology GmbH** (One Stop Systems subsidiary): partnership announced Aug 2024 for Siemens-Edge-certified NPP panel PCs. CyberDanube also contacted Novakon through Bressner. | [S] https://pressebox.com/pressrelease/bressner-technology-gmbh/Collaboration-for-Siemens-Industrial-Edge-Certified-Panel-PCs/boxid/1213753 |
| Serbia | **Electro Pneumatic Solutions d.o.o.**: "representative of the NOVAKON brand for Serbia", with a public price list (see §4). | [S] https://www.ep-solutions.rs/en/novakon |
| China | **OKmarts** claims to have been a NOVAKON distributor "for years". Likely a gray-market reseller. | [S] https://okmarts.com/product/hmi/novakon.html |
| Mexico | Unlimited Electro (unlimitedelectro.co): listed P07F-N / P10F-N with prices "IVA incluido". Seen only in a search snippet; the site did not resolve. | [S/U] |
| Global | The home page banner says "Global Distributors Wanted", so the channel is still being built. | [V] https://www.novakon.com.tw/en |

---

## 2. iFace Designer (software)

Product page: https://www.novakon.com.tw/tw/software/detail/iFace-Designer (EN: https://www.novakon.com.tw/en/software/detail/iFace-Designer)

| Item | Finding | Label |
|---|---|---|
| What it is | "iFace Designer®是一套跨平台的可視化圖控軟體。編譯後的應用程式可下載至人機介面或工業觸控電腦中運行。" HMI/SCADA graphics editor, ribbon UI | [V] |
| Current version | **2.0.05 Build 2026.02.09** (V2.0.05 announced 2025-10-15). Manuals V1.6.71 / V2.0.01; separate "iFACE SCADA" quick guide V2.0.01 | [V] https://www.novakon.com.tw/en/news/detail/iFace_Designer_V2_0_05_Software_Update |
| Editor OS | Windows 10/11 64-bit | [V] |
| Licence / price | Company news (2024-07-22): "iFace-Designer® is an easy-to-use and user-friendly **FREE** HMI development software." The download is **members-only (會員專屬)**, so registration is required. The Serbian distributor lists "Novakon iFace Designer Software" at **0,00**. No EULA text is public. | [V] https://www.novakon.com.tw/en/news/detail/Novakon_s_Versatile_Human_Machine_Interface__HMI__Solutions_with_iFace_Designer ; [S] https://www.ep-solutions.rs/en/novakon-software-for-hmi-and-gateway |
| Runtime licence | HMI panels (P07 etc.) list "iFace-Designer Runtime (software option)". The x86 NPP panel PC lists "Software options: iFace-Designer® Runtime / Siemens Industrial Edge Runtime / Siemens Industrial Management". A search snippet from device.report showed a bundle line "iFace Designer HMI / SCADA Software Runtime License", which could not be opened (403). **The runtime on x86 panel PCs is probably a priced option; there is no public price.** | [V] product pages; [U] device.report |
| Hardware lock-in | Runtime targets NOVAKON's own ARM HMIs (N/P/S series) and NOVAKON x86 panel PCs ("compiled runtime can work on both HMI Operator Panels (ARM-based) and Industrial Panel PCs (X86)"). No evidence that it runs on third-party hardware except through ODM/OEM projects. | [V]/[U] |
| HMI runtime platform | **Embedded Linux plus Qt** (Qt/Embedded "-qws", LinuxFb). Process names `iFACE_RT` and `PSeriesbiosinterface`; Qt lib path `/opt/qt_arm48_p/lib` (Qt 4.8 era); hostname `am335x-evm` (TI AM335x reference BSP). Datasheets: N-series "Linux Kernel 2.21" (sic), P-series "Linux Kernel 3.14". | [S] CyberDanube advisory; [V] datasheets |
| Panel PC OS | Windows 10 IoT or Debian 11 (NPP-156P02) | [V] |
| Protocols | Modbus RTU/TCP/ASCII master and slave, CANopen, BACnet IP client (v2.0.05), **OPC UA Server** (v2.0.05), MQTT publisher/subscriber, RESTful API (JSON over HTTPS). PLC drivers: Allen-Bradley, Beckhoff TwinCAT 2/3 ADS, CODESYS V3, Delta, FATEK, Hitachi, HUST CNC, Keyence, Koyo, Lust, Mitsubishi (FX, iQ-R), Motrona, Omron (incl. NJ/NX), Optex, Panasonic, Siemens S5/S7-200/300/1200/1500, TECO, Toshiba, Vigor, Yamatake, Yaskawa (MP), Yokogawa. Claims "90%+ mainstream PLC drivers". | [V] |
| Features | Vector widget library, trends and XY charts, alarms, 16 data logs × 32 items, recipes, macros, triggers, scheduler, 16 languages, security/passwords, CNC G-code, 3-phase vector diagrams, simulation without compile, barcode scanner and USB keyboard. Logs export to TXT/CSV/PDF. | [V] |
| Remote / cloud | VNC remote view (PC/phone/tablet), FTP file transfer, **PLC pass-through, VPN**, IIoT services; Wi-Fi/Bluetooth/GSM through USB dongles. No NOVAKON-hosted cloud subscription is advertised. | [V] |
| Paid add-ons observed | None with public prices. Possible paid items: x86 runtime licence, "iFACE SCADA" (Windows SCADA mode on panel PCs), ODM software customisation (sold as a service). | [U] |
| Sister software | **iM Connect®** (protocol-conversion and data-acquisition software for the GW-01 DIN-rail gateway, web-based; listed free at the Serbian distributor). **iM-Connect Universal®**. | [V]/[S] |

### 2.1 Security record (relevant to an EDT copy: security debt is a real cost)

CyberDanube advisory, released 2025-09-19 (Full Disclosure 2025-09-25), covers P-series firmware **P - V2001.A.c518o2**:
- CVE-2025-9962: unauthenticated stack overflow in `PSeriesbiosinterface` on UDP/60681, giving RCE as root
- CVE-2025-9963: symlink path traversal through USB, giving root file read/write
- CVE-2025-9964: root has an empty password
- CVE-2025-9965: unauthenticated project upload/download over UDP/60681
- CVE-2025-9966: processes run as root

There was no fix at publication, and the vendor did not agree to publication.
[S] https://www.mail-archive.com/fulldisclosure@seclists.org/msg09126.html

---

## 3. Hardware lineup

Source: https://www.novakon.com.tw/en/hardware/series/HMI and the model pages. [V]

### 3.1 HMI operator panels (ARM, iFace Designer runtime)

| Model (order code) | Size / resolution | SoC | RAM / storage | Notes |
|---|---|---|---|---|
| N04 | 4.3" 480×272, resistive | ARM9 400 MHz | n/a | entry |
| **N07 (N07F-N)** | 7" 800×480, 4-wire resistive | ARM9 400 MHz | 128 MB DRAM / 128 MB NAND / 8 KB FRAM | 3× COM, 1× 10/100 Ethernet, USB host+device, SD, 24 VDC, IP65 front, plastic, VESA 75 |
| **P07 (P07F-N)** | 7" 800×480 | **TI Sitara AM3352 Cortex-A8 1 GHz** | 512 MB DRAM / 4 GB eMMC / 32 KB FRAM | 3× COM (COM3 can be CAN), 1 or 2× Ethernet (option), USB, SD, 8 W, IP65 |
| P10 (P10F-N) | 10.2" 800×480 | Cortex-A8 1 GHz | n/a | |
| P12 | 12.1" 1280×800 | Cortex-A8 1 GHz | n/a | 5-wire resistive (DirectIndustry) |
| P15 (P15F-N) | 15.6" 1366×768 | Cortex-A8 1 GHz | n/a | |
| Legacy / other | NK03 keypad text HMI; P04; S-series (S07, S15, S3010); "Cortex-A9" platforms mentioned by EP Solutions | | | [S] DirectIndustry, OKmarts, Ruten |

Certifications on the N07/P07 pages: **CE, FCC (EN 61000-6-4 / 6-2), UL**. **KC is not listed.** Certificate files are "Member Only". [V]
- **No FCC ID exists.** HMIs are Class A unintentional radiators handled under SDoC; an fccid.io search for "novakon" returned only unrelated Korean grantee "NOV". [P by absence] https://fccid.io/search.php?q=novakon
- No UL file number was found publicly. UL Product iQ needs a login and was not checked. [U]

### 3.2 Industrial panel PCs and box PCs (x86, plus one ARM)

NFP-121/156 (Atom x7425E or Core i5-1335U, slim bezel); NOF-156/215 (open frame); NRP-156/215 (stainless); **NPP-156/215 P01/P02/P05** (J6412 / N97 / i5-1335U; Siemens Industrial Edge certified; Win10 IoT or Debian 11; CE/FCC); NWH10 (ARM Cortex-A55 quad 1.8 GHz control panel); NBP-01/02S/05S and NRB-03 (box PCs). [V] https://www.novakon.com.tw/en/hardware/series/Industrial_Panel_PC
These look like iBASE-platform derivatives: iBASE builds the same Intel SoC classes. [U]

### 3.3 Gateway

GW-01 protocol conversion gateway (DIN rail, runs iM Connect). [V]

### 3.4 Who manufactures?

| Evidence | Reading | Label |
|---|---|---|
| Novakon says its products are "fully developed and manufactured in Taiwan" / "臺灣研發、生產" | Vendor claim | [V] |
| Business registration includes CC01110/CC01990 manufacturing items, but no factory registration was found | Registration allows manufacturing; no evidence of its own plant | [P]/[U] |
| iBASE consolidated intercompany tables: **iBASE → Novakon "Sale" NT$48,392k (FY2022)** and **NT$26,661k (FY2023)**, "not significantly different from non-related parties" | The parent sells goods to Novakon: about 37% of Novakon's 2022 revenue and 28% of 2023 revenue. This fits iBASE supplying boards or finished units [U] | [P] iBASE FY2022/FY2023 annual reports (Eng.) |
| iBASE plants: Sanchong (6F, No.129 Xingde Rd, New Taipei), Xinzhuang (No.17 Ln.83 Huacheng Rd), Pingzhen ×2 (Taoyuan), and IBASE Technology (Vietnam) Co., Ltd. (new plant, planned start 2025 Q3; 6 SMT, 4 DIP, 10 system lines) | Parent has in-house EMS capacity in Taiwan and Vietnam | [P] iBASE FY2025 annual report; [S] https://uanalyze.com.tw/articles/3571312901 |
| iBASE 2023 GHG inventory boundary includes "the subsidiary, Novakon Co., Ltd." | Novakon sits inside iBASE's operational footprint | [P] iBASE FY2023 annual report |
| US customs: shipped from Taipei via Kaohsiung, HTS 8473.30 (parts and accessories of ADP machines) | Country of origin is not stated; it is Taiwan shipper data only | [S] Panjiva |
| Reference design: hostname "am335x-evm", TI AM3352 | Built on the TI EVM BSP, not on a Chinese white-label HMI board | [S]/[U] |

**Conclusion [U]:** most likely NOVAKON designs the products and has them built by its parent iBASE (Taiwan plants, possibly Vietnam later), not by an outside ODM. **This differs from EDT's planned "outsourced" model in one key way:** NOVAKON's "outsourcing" is to a captive sister manufacturer with IPC-grade QA and certification experience.

---

## 4. Prices

FX assumptions (approximate, Nov 2026, for comparison only): **US$1 ≈ NT$32; €1 ≈ RSD 117; £1 ≈ US$1.30.** Prices are as displayed on the access date unless noted. Search-snippet prices are marked [S/U] because the page could not be opened.

### 4.1 NOVAKON: new units, overseas

| Model | Size | Price | Currency, tax | Approx. US$ | Seller (country) | Date | Label |
|---|---|---|---|---|---|---|---|
| N07F-N | 7" | **28.748,17** ex VAT / 34.497,80 incl. VAT | RSD (inferred) | ≈ €246 ex VAT ≈ US$265 | EP Solutions (RS), "in stock" | 2026-11-20 | [S] https://www.ep-solutions.rs/en/novakon-n07-touch-screen-hmi-n07f-n |
| P07F-N | 7" | list 41.853,14; **sale 36.830,76** ex VAT (44.196,92 incl. VAT) | RSD | ≈ €315 ≈ US$340 | EP Solutions (RS) | 2026-11-20 | [S] https://www.ep-solutions.rs/en/p-series-hmi-operator-panel-standard-hmi-panel |
| P10F-N | 10.2" | list 64.190,49; **sale 56.487,63** ex VAT | RSD | ≈ €483 ≈ US$520 | EP Solutions (RS) | 2026-11-20 | [S] same |
| P15F-N | 15.6" | list 150.541,98; sale 132.476,94 ex VAT | RSD | ≈ €1,130 ≈ US$1,220 | EP Solutions (RS), "need to order" | 2026-11-20 | [S] same |
| N07F-N | 7" | US$251.39 (new, factory sealed, ex import tax) | USD | 251 | PLC-City (gray-market reseller) | snippet | [S/U] https://plc-city.com/shop/en/novakon-hmi-panel/n07f-n-nfs.html |
| P10F-N | 10.2" | US$507.09 (new, backorder) | USD | 507 | PLC-City | snippet | [S/U] https://www.plc-city.com/shop/en/novakon-hmi-panel/p10f-n-nfs.html |
| N07F-N | 7" | £257.39 ex VAT / £308.87 incl. VAT | GBP | ≈ 335 | Automation Database (UK) | snippet | [S/U] https://www.automationdatabase.com/novakon/n07f-n |
| P07F-N | 7" | "Regular price $249.00" | USD | 249 | The Circuit Breaker Pros (US surplus) | snippet | [S/U] https://thecircuitbreakerpros.com/products/novakon-p07f-n-operator-control-panel |
| P07F-N | 7" | $371.20 IVA incluido | presumably USD (MX) | ≈ 320 ex IVA | Unlimited Electro (MX) | snippet | [S/U] https://unlimitedelectro.co/brand/novakon/ |
| P10F-N | 10.2" | $533.60 IVA incluido | presumably USD (MX) | ≈ 460 ex IVA | Unlimited Electro (MX) | snippet | [S/U] same |
| N07-S / N07-P / S3010-N | 7" / 10" | $692.31 / $884.62 / $1,346.15 | "$", presumably USD | (inflated broker pricing) | OKmarts (CN) | 2026-11-20 | [S] https://okmarts.com/product/hmi/novakon.html |
| P07F-N | 7" | US$157.77 (seller-refurbished) | USD | 158 | eBay | snippet | [S/U] https://www.ebay.com/itm/374656854100 |
| P10F-N | 10.2" | US$1,499.99 (used, repair house) | USD | — | NJT Automation | snippet | [S/U] https://njtautomation.com/p10f-n/ |

The manufacturer publishes **no price list**: every product page says "Request a quote". [V]

### 4.2 NOVAKON in Taiwan (露天 Ruten)

Only used or surplus units were found, mostly from China-based gray resellers. These listings do not show new-unit pricing. [S] https://www.ruten.com.tw/find/?q=novakon (2026-11-20)

| Item | Title (abridged) | Price TWD |
|---|---|---|
| 22614404183094 | NOVAKON觸摸屏N07-P，二手正常拆 | 1,500 |
| 22549633016056 | NOVAKON 觸摸屏 N07-P 功能正常 | 1,050 |
| 22628102373084 | NOVAKON觸控式螢幕N07-P | 1,000 |
| 22638673680067 | NOVAKON NK03-P 工業文字顯示器 | 6,000 |
| 22635476881877 / …79931 | 速博康NOVAKON N07 to 士林/三菱 PLC cables | 650 / 600 |

No listings were found on PChome, momo, Yahoo 拍賣 or 蝦皮 (searches returned nothing relevant; 蝦皮/momo pages are JS-rendered and were not fetched). [U]

### 4.3 Weintek comparison (same or similar sellers)

| Model | Size | Price | Seller | Label |
|---|---|---|---|---|
| MT8071iE (legacy entry, now replaced by cMT2078X) | 7" | US$391.99 (one listing) / US$776.99 (another) | Zoro (US) | [S/U] snippet https://zoro.com/weintek-mt8071ie-800x480-600-mhz-7-hmi-mt8071ie/i/G700719681 |
| MT8071iE | 7" | US$418 | DDPartsSolution | [S/U] |
| MT8071iE | 7" | €595.80 incl. tax | Anodas (LT) | [S/U] |
| cMT2078X | 7" | US$586.95 | Wolf Automation (US) | [S/U] |
| cMT3072XH / XH2 | 7" | US$651.99 (Zoro), US$666.75 / US$712.95 (Wolf) | US distributors | [S/U] |
| MT8071iE | 7" | Ruten: used NT$2,300–6,000; new-in-box NT$5,390–6,000 (≈ US$170–190) | Ruten gray market, 100 listings | [S] https://www.ruten.com.tw/find/?q=MT8071iE |
| MT8102iE | 10" | Ruten: NT$3,300–10,600; new about NT$8,000–10,000 (≈ US$250–310) | Ruten | [S] https://www.ruten.com.tw/find/?q=MT8102iE |
| cMT3075XH2 | 7" | Ruten: NT$11,413–16,200 (≈ US$355–505) | Ruten | [S] https://www.ruten.com.tw/find/?q=%E5%A8%81%E7%B6%B8+cMT3072XH |

**Takeaway:** new 7" NOVAKON N07 or P07 at Western resellers is US$250–370; Weintek 7" (MT8071iE, cMT2078X, cMT3072XH) is US$390–710. NOVAKON is positioned as a lower-price alternative, but its volume is far smaller (see §5). The Taiwan gray market is flooded with cheap used Weintek units, which an EDT newcomer would also face.

---

## 5. Revenue, scale and margins (from iBASE annual reports, "Operational results of each affiliate")

Source: iBASE (8050) English annual reports on MOPS, doc.twse.com.tw. [P]
- FY2022: `2022_8050_20230620FE4.pdf` → https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=F&co_id=8050&filename=2022_8050_20230620FE4.pdf
- FY2023: `2023_8050_20240619FE4.pdf` → …&filename=2023_8050_20240619FE4.pdf
- FY2024: `2024_8050_20250620FE4.pdf` → …&filename=2024_8050_20250620FE4.pdf
- FY2025: `2025_8050_20260626FE4.pdf` → …&filename=2025_8050_20260626FE4.pdf
(The t57sb01 step=9 page returns a one-time /pdf/… link.)

Units: NT$ thousand.

| FY | Capital | Total assets | Liabilities | Net worth | **Operating revenue** | **Operating income** | Net income | EPS (NT$) | Op. margin (calc.) |
|---|---|---|---|---|---|---|---|---|---|
| 2022 | 42,675* | 166,578 | 16,458 | 150,120 | **132,128** | 29,200 | 32,003 | 7.50 | 22.1% |
| 2023 | 68,800 | 129,242 | 33,374 | 95,868 | **94,274** | (25,198) | (24,045) | (3.49) | −26.7% |
| 2024 | 68,800 | 189,944 | 33,626 | 156,318 | **142,806** | 25,993 | 60,026 | 8.72 | 18.2% |
| 2025 | 68,800 | 205,177 | 46,823 | 171,610** | **230,628** | 66,811 | 49,954 | 7.26 | 29.0% |

\* The FY2022 table shows capital 42,675 even though a Dec-2022 share issue had happened; the issue was probably registered after year-end.
\** As printed. Assets minus liabilities gives 158,354, so the filing's table is internally inconsistent.

- FY2025 revenue NT$230.6M ≈ **US$7.2M**, up 61% year on year. FY2024 net income exceeds operating income by about NT$34M, a non-operating gain whose nature is not disclosed.
- iBASE's commentary on Novakon: "Stable profit" in FY2024 and FY2025; FY2023: "Decrease in operating revenue and increase in operating expenses". The "reason/plan" text oddly says "Tapping into the data storage media industry", which looks like copy-paste boilerplate. [P]
- Figures in iBASE's "investment gain/loss" table for Novakon: 2023 (16,897), 2024 42,166, 2025 34,968. These are about 70% of Novakon's net result, i.e. iBASE's share. The FY2022 table shows 32,003, which equals 100% of net income. [P]
- **Gross margin is not disclosed.** Novakon files no standalone public statements. [Not public]
- **Units shipped, customer names, and share of HMI versus panel-PC revenue: not public.** [Not public]. Rough order of magnitude: if about half of revenue were HMIs at about US$250 ex-works, that would be about 10–15k HMI units per year [U, illustrative only].
- Context: Weintek (威綸科技, unlisted) is commonly reported to sell hundreds of thousands of units per year. NOVAKON is a niche player at roughly 1/50 or less of that scale [U].
- Parent context: iBASE consolidated revenue in FY2025 was about NT$5.36B, so Novakon is about 4% of group revenue. [S] financialfilings.com, Simply Wall St

---

## 6. Implications for the EDT spin-off model (analyst notes, [U])

1. **The model is viable but small.** A free editor plus owned-brand hardware sustains a profitable company of about 20–30 people with about US$4–7M revenue. It took about 13 years and a listed parent's balance sheet (capital injections in 2022 and 2026, shared HQ, captive manufacturing).
2. **Hardware economics carry the business.** NOVAKON monetises through panels (US$250–1,200) and panel PCs. The editor is free, but runtime on x86 is likely an option, and ODM software customisation is a service line. EDT HMI Studio would need a similar hardware SKU line and an x86/Linux runtime upsell.
3. **The "outsourced manufacturing" variant differs from NOVAKON's actual setup.** NOVAKON relies on a sister EMS (iBASE plants) for quality, certification (CE/FCC/UL) and supply. A spin-off without an affiliated factory would carry higher COGS and QA risk. EDT, as a display-module maker, is in a position similar to iBASE as a captive supplier.
4. **Competitive pricing pressure.** NOVAKON already undercuts Weintek by 30–50%, and Chinese brands (Wecon, Kinco) go lower. Gray-market used Weintek panels in Taiwan sell for NT$2–6k.
5. **Security and compliance debt.** NOVAKON's 2025 CVEs show the risk of an old Qt4/AM335x Linux stack with root services. EU CRA obligations from 2027 make this a cost line for any new entrant.
6. **Channel.** NOVAKON sells >90% overseas through a few distributors (US: Novatek Controls; DE: Bressner; RS: EP Solutions) and is still recruiting distributors. Channel building is slow.

---

## 7. Not found / not public

- Gross margin, unit shipments, customer list, revenue split by product: not public.
- Exact iFace Designer EULA and runtime licence prices: not public. The download requires membership.
- UL file number and KC certification: not found.
- New-unit prices in Taiwan retail (PChome, momo, 蝦皮): none found. Ruten has only used or gray-market units.
- Amazon, AliExpress, India: no NOVAKON listings found.
- Any historical tie to Weintek, Advantech or Delta: none found.
- 104.com.tw, findbiz, twincn, technews, DirectIndustry PDFs and several resellers returned 403 to the fetcher. Data came from the GCIS open API and MOPS instead.

## 8. Source list (all accessed 2026-11-20)

- NOVAKON iFace Designer page (TW/EN): https://www.novakon.com.tw/tw/software/detail/iFace-Designer
- NOVAKON About (EN/TW): https://www.novakon.com.tw/en/about , https://www.novakon.com.tw/tw/about
- NOVAKON HMI series: https://www.novakon.com.tw/en/hardware/series/HMI ; P07: https://www.novakon.com.tw/en/hardware/detail/HMI/P07_Touch_Screen_HMI ; N07: https://www.novakon.com.tw/en/hardware/detail/HMI/N07_Touch_Screen_HMI
- NOVAKON Panel PCs: https://www.novakon.com.tw/en/hardware/series/Industrial_Panel_PC ; NPP-156P02: https://www.novakon.com.tw/en/hardware/detail/Industrial_Panel_PC/NPP_156P02_Industrial_Panel_PC
- NOVAKON news "FREE HMI development software" (2024-07-22): https://www.novakon.com.tw/en/news/detail/Novakon_s_Versatile_Human_Machine_Interface__HMI__Solutions_with_iFace_Designer
- NOVAKON V2.0.05 release (2025-10-15): https://www.novakon.com.tw/en/news/detail/iFace_Designer_V2_0_05_Software_Update
- iM Connect: https://www.novakon.com.tw/en/software/detail/iM_Connect
- GCIS open data API (3 endpoints listed in §1.1)
- data.bznk.com record: see §1.1
- 518熊班 profile: https://www.518.com.tw/company-ywnA8k.html
- iBASE annual reports FY2022–FY2025 (MOPS doc server, see §5)
- AAEON FY2022 consolidated report (English): https://newdata.aaeon.com.tw/DOWNLOAD/prenentations/Y111%20Consolidated%20Financial%20Report_English%20Version.pdf
- 工商時報 2023-11-24 小巨人獎: https://www.ctee.com.tw/news/20231124700537-431202
- 工商時報 2022-10-21 玉山獎: https://www.ctee.com.tw/news/20221021700544-431202
- CyberDanube advisory: https://www.mail-archive.com/fulldisclosure@seclists.org/msg09126.html
- Panjiva: https://cn.panjiva.com/Novakon-Co-Ltd/45298805
- Bressner press release: https://pressebox.com/pressrelease/bressner-technology-gmbh/Collaboration-for-Siemens-Industrial-Edge-Certified-Panel-PCs/boxid/1213753
- EP Solutions (RS): https://www.ep-solutions.rs/en/novakon , https://www.ep-solutions.rs/en/p-series-hmi-operator-panel-standard-hmi-panel , https://www.ep-solutions.rs/en/novakon-n07-touch-screen-hmi-n07f-n , https://www.ep-solutions.rs/en/novakon-software-for-hmi-and-gateway
- OKmarts: https://okmarts.com/product/hmi/novakon.html
- Ruten searches: https://www.ruten.com.tw/find/?q=novakon , https://www.ruten.com.tw/find/?q=MT8071iE , https://www.ruten.com.tw/find/?q=MT8102iE , https://www.ruten.com.tw/find/?q=%E5%A8%81%E7%B6%B8+cMT3072XH
- iBASE Vietnam plant: https://uanalyze.com.tw/articles/3571312901
- fccid.io: https://fccid.io/search.php?q=novakon
- Weintek reseller prices (search snippets): Zoro, Wolf Automation, DDPartsSolution, Anodas (URLs in §4.3)
