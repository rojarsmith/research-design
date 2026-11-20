# Verification: HMI panel platforms, open-source licences, LVGL XML terms, CRA dates, Windows CE lifecycle

Date: 2026-11-20. This note checks open items in `hmi_runtime_platforms.md` and `ehs_port_to_mpu.md` (same folder). Those notes were not edited; corrections are listed here.

Method: WebSearch and WebFetch only. Some vendor PDFs that WebFetch could not parse were saved by WebFetch and read locally with `pdftotext`. GitHub history was read with `gh api`, read-only. Siemens forum, Omron licence pages, Pro-face OSS pages and EUR-Lex returned 403 or empty pages, so those items rely on other sources, as noted.

Labels:
- **[primary]**: the vendor's or project's own document, repo, or official EU or Microsoft page. A vendor PDF hosted by a distributor also counts.
- **[secondary]**: search-engine snippets, forums, third-party blogs, CVE aggregators and legal-text mirrors.
- **[inference]**: my own reasoning.

Result codes: **confirmed**, **corrected**, **new**, **still unverified**.

---

## Summary

| # | Item | Result | One-line finding |
|---|---|---|---|
| 1a | Siemens Unified Comfort OS | **confirmed (upgraded to primary)** | Siemens: Unified Comfort Panels "contain a Linux-based operating system"; classic Comfort Panels are "Windows CE-based". |
| 1a | Siemens Unified Comfort SoC | **still unverified** | No datasheet, OSS readme or teardown names the CPU. An i.MX8 hit was a different Siemens product. |
| 1b | Pro-face GP4000 OS | **new** | The release notes fix "OS vulnerability" CVEs that are all Wind River **VxWorks** CVEs, including URGENT/11. So the GP4000 (except GP4000M and GP410*) runs VxWorks [inference, high confidence]. |
| 1c | Mitsubishi GOT2000 OS/CPU | **still unverified** | Mitsubishi only calls the firmware "BootOS", "CoreOS" and "standard monitor OS". There is no Linux evidence and the CPU is not published. |
| 1d | Omron NA5 / NA5-V2 OS/CPU | **still unverified** | The NA5-V2 manual points to a third-party licence page, which returned 403. The WEC7 guess remains unproven. |
| 1e | C-more EA9 | **partly confirmed; OS still unverified** | AutomationDirect: "800MHz CPU". The OS is not named anywhere official. |
| 1f | Kinco GL | **CPU confirmed (primary); OS still secondary** | Kinco flyer: "ARM RISC 32Bit 800MHz", 128 MB DDR3. The Linux claim is from a distributor only. |
| 1g | Samkoon SK | **confirmed (upgraded to primary)** | Samkoon guide: "based on Linux system" and "Cortex A7 1GHz. Dual Core". |
| 2 | 21 open-source licences | **all confirmed; four refined** | Refinements: WAMR is Apache-2.0 **with LLVM exception**; SWUpdate's core is GPL-2.0-only with mixed licences elsewhere; Qt lists GPL-only modules; open62541 has some CC0 files. |
| 3 | LVGL XML licence | **corrected** | The XML engine was **removed from LVGL v9.5** (18 Feb 2026). It is now proprietary, and on-device runtime loading is a paid feature. The 2025 XML License never forbade runtime loading. It does forbid third-party UI editors or code generators that read or write the format. |
| 4 | EU CRA dates | **confirmed** | In force 10 Dec 2024. Chapter IV from 11 Jun 2026. Art. 14 reporting from 11 Sep 2026. General application 11 Dec 2027. No postponement found. |
| 5 | Windows CE / WEC 2013 | **corrected (mainstream date)** | WEC 2013 mainstream support ended 9 Oct **2018**, not 2023. Extended support ended 10 Oct 2023. Licence sales are allowed "until 2028". |

---

## 1. Processor and operating system of HMI panels

### 1a. Siemens SIMATIC HMI Unified Comfort Panels (MTP700–MTP2200)

**OS: confirmed, now primary.**
- Siemens application example "Guide for Migrating to WinCC Unified – Technical Information on Comfort Panels/Unified Comfort Panels", Article ID 109768002, V1.0, 09/2024, §2.6 "Software" [primary]:
  > "In contrast to the Windows CE-based Comfort Panels, the Unified Comfort Panels with "Industrial OS" contain a Linux-based operating system. Access to the operating system by the user is not intended."
  - https://support.industry.siemens.com/cs/attachments/109768002/109768002_Guide_to_Unified_Comfort_V10_DOC_en.pdf
  - Redirects to https://cache.industry.siemens.com/dl/files/002/109768002/att_1305388/v3/109768002_Guide_to_Unified_Comfort_V10_DOC_en.pdf
- The same sentence also confirms, from a primary source, that **classic Comfort Panels are Windows CE-based**. The prior note marked this "inference, unverified". The guide does not give the CE version; CE 6.0 remains inference.
- Debian lineage [secondary, search snippet only; page returned 403]:
  - A Siemens forum post (16 Dec 2024) says the UCP image is Debian-based: V17 = Debian 10, V18 = Debian 11, V19 = Debian 12.
  - https://support.industry.siemens.com/forum/CN/en/post/1232241
- Marketing wording: "proprietary operating system based on SIMATIC Industrial OS" [secondary, distributor copy of Siemens text].
- An Industrial Edge app on UCP gets at most 768 MB RAM [secondary: Siemens Italy community repo] https://github.com/SiemensIndustrialEdgeITA/edge-node-red-unified

**SoC: still unverified.**
- No Siemens datasheet, operating manual, migration guide or OSS readme reached names the CPU. The V19 operating instructions were also checked (prior note).
- A search hit giving "NXP i.MX8 QuadXPlus, 1.2 GHz, 2 GB RAM, 8 GB eMMC" is the **Desigo PXC7.E400.A** building controller, not a UCP. https://sid.siemens.com/r/A6V12957866/19996096395_21457181323__en-US_20034598667
- A Siemens HMI brochure's "Intel Celeron N2807/N2930" rows belong to SIMATIC IPC/Panel PC tables, not UCP. https://sistronic.com.co/wp-content/uploads/2024/01/paneles-hmi.pdf
- [inference, weak] x86-64 is somewhat more likely than ARM:
  - Siemens lists "Google Chrome Browser" as preinstalled (WinCC Unified white paper). Google Chrome for Linux was historically x86-64 only.
  - Siemens community Edge apps for UCP are built with a plain `docker-compose up --build`, with no `--platform` or cross-arch step.

  Neither point is proof. To settle it, look for the UCP image's OSS readme (`ReadMe_OSS`) on SIOS, or a teardown.

### 1b. Pro-face GP4000 series

**OS: new finding. Wind River VxWorks** on GP4000, excluding the GP4000M and GP410* models [inference, high confidence, built on primary release notes].

The GP-Pro EX V4.09 readme [primary] https://www.proface.tech/otasuke/download/update/proex/proex/v409/Readme_E_GPProEX_v409450.txt contains:
- "Sub-system/OS Core: Ver.4.9.123" for "GP4000 series (excluding GP4000M and GP410* series)".
- Fixed defect, V4.09.120: OS vulnerability fixed on "the LT4000 and GP4000 series (excluding GP4000M and GP410* series)" (CVE-2020-10664).
- Fixed defect, V4.09.400: for "GP-4000 Series (excluding GP4000M and GP410* Series) and GP-4G01", "the operating system was updated" for CVE-2016-20009, CVE-2020-35198 and CVE-2020-28895. Remote PC Access support ended as a result.
- An earlier section: "Fixed OS vulnerability. [CVE-2019-12256, CVE-2019-12257, CVE-2019-12255, … CVE-2019-12265". These are the URGENT/11 IPnet CVEs. The model list for this entry was not visible in the excerpt.

Every one of these CVE IDs is a Wind River VxWorks vulnerability [secondary: NVD-derived aggregators]:
- CVE-2016-20009: VxWorks DNS client `ipdnsc_decode_name()`, VxWorks 6.5–7.
- CVE-2020-28895: VxWorks memory allocator overflow, VxWorks 6.9 before 6.9.4.12.
- CVE-2020-10664: VxWorks 6.8.3 IPNET.
- CVE-2020-35198: VxWorks 7 memory allocator overflow.
- CVE-2019-12255 to CVE-2019-12265: URGENT/11 in VxWorks IPnet.

Sources:
- https://nvd.nist.gov/vuln/detail/CVE-2020-28895
- https://nvd.nist.gov/vuln/detail/CVE-2016-20009
- https://feedly.com/cve/vendors/windriver
- https://cxsecurity.com/cveproduct/13152/vxworks

Pro-face never names VxWorks in the text read. The conclusion is therefore labelled inference, but the CVE fingerprint is unambiguous.

Related findings:
- The GP4000M and GP410* models are excluded from these fixes and probably run a different OS [inference].
- The GP4000 CPU is still not published.
- The **GP6000** has a Pro-face OSS page that lists GPLv2 components [secondary snippet; page returned 403]. That hints the GP6000 generation may run Linux, but this is unverified. https://www.proface.com/en/product/hmi/gp6000/oss
- The GP-Pro EX Dec 2025 release (V5.0) lists "System/Runtime Ver.5.1.1" for GP4000 [secondary snippet].

**Correct in `hmi_runtime_platforms.md`:** the Pro-face row should read "OS: VxWorks (inference from CVE list in vendor release notes)", not "not published".

### 1c. Mitsubishi GOT2000 (GT27 / GT25)

**Still unverified.** No CPU or OS name was found.

Mitsubishi PSIRT advisory "Multiple vulnerabilities in TCP/IP Stack on GOT2000 Series", 3 July 2020 [primary] https://www.mitsubishielectric.com/psirt/vulnerability/pdf/2020-005_en.pdf:
- It refers to "TCP/IP stack of the firmware in GT27 model, GT25 model and GT23 model … with CoreOS version '-Y' and earlier".
- The fix is "Core OS version -Z and later", written via GT Designer3 → "BootOS/CoreOS Write".
- JVNVU#95413676 (published 2020-07-03) and CISA ICSA-20-189-02 do not name a third-party stack. https://jvn.jp/en/vu/JVNVU95413676/index.html and https://www.cisa.gov/news-events/ics-advisories/icsa-20-189-02

My reading [inference]:
- The firmware layers are Mitsubishi's own: BootOS, then CoreOS, then the "standard monitor OS". GT Designer3 writes them.
- A firmware-embedded TCP/IP stack with its own CVEs points to an RTOS with a vendor or third-party stack, not Linux.
- It is often said that GOT uses Zuken Elmic's KASAGO stack, but **no source found confirms it**.

No GOT2000 open-source licence notice or CPU specification was found. GT27 product pages list only display, memory and interfaces (256 MB internal memory on GT2710-STBD) [primary product pages, via search].

### 1d. Omron NA5 / NA5-V2

**Still unverified.**

The NA-series Hardware (-V2) User's Manual V128-E1-01 [primary] says:
> "This product incorporates certain third party software. The license and copyright information associated with this software is available at https://www.fa.omron.co.jp/product/tool/na5-info/index_en.html"
- That page returned **403**. The manual's only named component is FreeType.
- Manual: https://files.omron.eu/downloads/latest/manual/en/v128_na-series_programmable_terminal_hardware_(-v2)_users_manual_en.pdf
- The Soft-NA manual points to http://www.fa.omron.co.jp/nj_info_e/ (also 403) [secondary snippet].

Neither the V128 manual nor the NA5-V1→V2 replacement guide names an OS or CPU [primary, full-text grep].
- The trademark list mentions Microsoft Edge and Intel Celeron/Atom. The Celeron/Atom names relate to Soft-NA PC requirements.
- Replacement guide: https://files.omron.eu/downloads/latest/manual/en/v472_na-series_programmable_terminal_replace_guide_from_na5-v1_to_na5-v2_replacement_guide_en.pdf

NA5 runtime ≤ V1.15 had CVE-2022-34151 (hard-coded credentials); the fix is runtime ≥ 1.16 [secondary]. https://jvn.jp/en/vu/JVNVU97050784/index.html

The "Windows Embedded Compact 7" guess in the prior note remains **unverified**. Nothing found supports or refutes it. To settle it, try the na5-info page from a non-blocked client, or Omron support.

### 1e. AutomationDirect C-more EA9

**CPU speed confirmed [primary]. OS still unverified.**
- "800MHz CPU"; "26MB to 82MB project memory depending on model". https://www.automationdirect.com/c-more/ea9/hardware
- AutomationDirect FAQ: "We have updated the processor from 400 MHz to an 800MHz processor." https://library.automationdirect.com/new-c-more-ea9-series-hmi-faqs-issue-29-2014/
- Panel firmware is bundled with the programming software (for example V6.53) and is not downloadable separately [primary]. https://support.automationdirect.com/firmware/historydetail.php?pid=62
- Recent CVE fixes: v6.78 (ICSA-24-086-01) and v6.80 (ICSA-25-035-08) [primary, CISA].
- No CPU architecture or OS name in official sources.
- Third-party claims conflict: one says "RTOS + TI ARM9", another says "Cortex-A8 600 MHz". Both are [secondary, low reliability]. The ARM9 claim is hard to square with the official 800 MHz.

### 1f. Kinco GL series

**CPU confirmed from a vendor flyer [primary via distributor]. OS still secondary.**
- Kinco GL100 flyer (2021-06): "Industrial grade CPU, 800MHz, 128M DDR3"; "ARM RISC 32Bit 800MHz"; "128MB NAND Flash Memory+128MB DDR3 Memory"; editor Kinco DTools. https://www.gotronik.pl/images/Flyer_GL100_EN%20202106.pdf
- "Cortex-A7" comes only from distributor copy (Anaheim Automation) [secondary].
- **Linux:** only distributor copy (Anaheim: "based on a Linux Operating System") [secondary].
  - A search snippet of Kinco's HMIware manual (legacy MT4000/MT5000) mentions updating "kernel and rootfs", a Linux-style process [secondary snippet; page 403].
  - One third-party blog claims a "proprietary RTOS" [secondary, low reliability].
- I found no Kinco GT-series primary data.

### 1g. Samkoon SK series

**Confirmed, now primary.**

Samkoon "Product Selection Guide" dated 2022-10-26 (vendor PDF hosted by a distributor) [primary]: https://www.rusavtomatika.com/manuals/Product-Selection-Guide-20221026.pdf
- SK series features: "based on Linux system, which is stable, efficient, safe and reliable"; "Industrial high-performance 1GHz processor"; "128MB FLASH+128MB DDR3"; "Support more than 300 PLC communication protocols"; "Support macro instruction based on C language".
- Spec tables: "Cortex A7 1GHz. Dual Core" on most SK models. Some models use "Cortex A8 600MHz" or "Cortex A8 800MHz"; one model has "4nuclear Cortex A17 1.5GHz"; another "Cortex A7 1.2GHz" with 256 MB DDR3.
- EA series (low end): "Real-time system", "ARM9 processor, main frequency up to 300MHz", 128 MB flash + 64 MB DDR3.
- The SK-102HE "embedded Linux 3.2.0" kernel claim is still [secondary, low reliability].

---

## 2. Open-source licences, checked against each project's repo or official page

All URLs fetched on 2026-11-20. All entries are [primary].

| Project | Licence (verified) | Evidence / notes | Prior note |
|---|---|---|---|
| libmodbus | **LGPL-2.1-or-later** | README: "The license of libmodbus is _LGPL v2.1 or later_." https://github.com/stephane/libmodbus | confirmed |
| snap7 | **LGPL-3.0** | SourceForge licence field "GNU Library or Lesser General Public License version 3.0 (LGPLv3)". The project licensing page says commercial software linked with Snap7 may be distributed "without the requirement to distribute the source code of your application". Latest 1.4.2; SourceForge last update 2025-06-24. https://sourceforge.net/projects/snap7/ and https://snap7.sourceforge.net/licensing.html | confirmed |
| libplctag | **MPL-2.0 OR LGPL-2.0-or-later** (dual, core C library) | README: "dual licensed under the Mozilla Public License 2.0 (MPL 2.0) or the GNU Lesser/Library General Public License 2 or later (LGPL 2+)"; files `LICENSE-1.MPL`, `LICENSE-2.LGPL`. Language wrappers may differ. https://github.com/libplctag/libplctag | confirmed |
| OpENer | **Adapted BSD-style** ("Software Distribution License for the EtherNet/IP Communication Stack"), © 2009 Rockwell Automation | Licence: no right to the EtherNet/IP mark; products must "obtain the necessary license from ODVA through its Terms of Usage Agreement"; no patent grant. https://github.com/EIPStackGroup/OpENer/blob/master/license.txt | confirmed |
| open62541 | **MPL-2.0** | LICENSE = MPL 2.0. README: "Some plugins and examples are in the public domain (CC0 license) and some are licensed under MPLv2." https://github.com/open62541/open62541 | confirmed (CC0 detail new) |
| Eclipse Paho C | **EPL-2.0 OR EDL-1.0** | Repo ships `epl-v20`, `edl-v10`; `about.html`: "Eclipse Public License Version 2.0 ("EPL") and Eclipse Distribution License Version 1.0 ("EDL")". https://github.com/eclipse-paho/paho.mqtt.c | confirmed |
| Mosquitto | **EPL-2.0 OR EDL-1.0** | LICENSE.txt: "dual licensed under the Eclipse Public License 2.0 OR the Eclipse Distribution License 1.0"; SPDX `EPL-2.0 OR BSD-3-Clause`. EDL-1.0 is the BSD-3-Clause text. https://github.com/eclipse-mosquitto/mosquitto | confirmed |
| RAUC | **LGPL-2.1-or-later** | COPYING = LGPL 2.1; README: "version 2.1 of the License, or (at your option) any later version", © 2015–2026 RAUC project. https://github.com/rauc/rauc | confirmed |
| SWUpdate | **GPL-2.0-only** (SWUpdate itself), plus other files under LGPL-2.1-or-later, MIT, BSD-1/2/3-Clause, ISC, CC0-1.0 and CC-BY-SA-4.0 | Licensing page: "You can redistribute SWUpdate and/or modify it under the terms of version 2 of the GNU General Public License". It lists the SPDX IDs used in the tree. Which files are LGPL is not mapped on that page; my understanding is that the IPC/client library is LGPL-2.1+ [inference, unverified]. https://sbabic.github.io/swupdate/licensing.html | refined |
| Mender client | **Apache-2.0** | LICENSE "Apache License Version 2.0", "Copyright 2026 Northern.tech AS". https://github.com/mendersoftware/mender | confirmed |
| SQLite | **Public domain** | "All of the code and documentation in SQLite has been dedicated to the public domain by the authors." Hwaci sells a "Warranty of Title" for organisations that need a licence. https://www.sqlite.org/copyright.html | confirmed |
| libsodium | **ISC** | LICENSE "ISC License", © 2013-2026 Frank Denis. https://github.com/jedisct1/libsodium | confirmed |
| neatvnc | **ISC** | COPYING = ISC text, © Andri Yngvason. https://github.com/any1/neatvnc | confirmed |
| libvncserver | **GPL-2.0** | GitHub licence: GPL-2.0, file COPYING; README: "under the terms of the GNU General Public License". https://github.com/LibVNC/libvncserver | confirmed |
| noVNC | **MPL-2.0** (core library) | LICENSE.txt: "The noVNC core library files are licensed under the MPL 2.0". `core/des.js` uses BSD-style licences, `vendor/pako` is MIT, and HTML/CSS/fonts/images are more permissive (e.g. BSD-2). https://github.com/novnc/noVNC | confirmed |
| WAMR | **Apache-2.0 WITH LLVM-exception** | LICENSE: Apache 2.0 plus "LLVM Exceptions to the Apache 2.0 License". https://github.com/bytecodealliance/wasm-micro-runtime | refined (exception added) |
| FFmpeg | **LGPL-2.1-or-later** by default | "Some optional parts of FFmpeg are licensed under the GNU General Public License version 2 or later". `--enable-gpl` makes the build GPL-2.0+; `--enable-version3` makes it LGPL-3/GPL-3; `--enable-nonfree` "will cause the resulting binary to be unredistributable". https://github.com/FFmpeg/FFmpeg/blob/master/LICENSE.md | confirmed |
| LVGL | **MIT** | LICENCE.txt "MIT licence", "Copyright (c) 2025 LVGL Kft". The XML engine is no longer part of the repo from v9.5 (see §3). https://github.com/lvgl/lvgl | confirmed |
| Qt 6 | **LGPL-3.0 / GPL-3.0 / commercial** | doc.qt.io/qt-6/licensing: Qt is available under LGPLv3 (or GPLv3) and commercially. Details below the table. https://doc.qt.io/qt-6/licensing.html | refined |
| Zig | **MIT (Expat)** | LICENSE "The MIT License (Expat)", "Copyright (c) Zig contributors". The bundled libc sources and headers carry their own licences [inference; not checked]. https://github.com/ziglang/zig | confirmed |
| Clang / LLVM | **Apache-2.0 WITH LLVM-exception** | LICENSE.TXT: "The LLVM Project is under the Apache License v2.0 with LLVM Exceptions". Code from before the relicensing is also under the legacy University of Illinois/NCSA licence. https://github.com/llvm/llvm-project | confirmed |

**Qt 6 details** (same doc.qt.io page):
- These modules are **GPL-only, not LGPL**: Qt Canvas Painter, Qt CoAP, Qt Graphs, Qt GRPC, Qt HTTP Server, Qt Lottie Animation, Qt MQTT, Qt Network Authorization, Qt Qml Compiler, Qt Quick 3D, Qt Quick 3D Physics, Qt Quick Timeline, Qt Virtual Keyboard and Qt Wayland Compositor.
- Tools are under GPL-3.0 with Qt GPL exception 1.0.
- Examples are BSD-3-Clause.
- The page does not mention GPLv2.
- [inference] Of these GPL-only modules, Qt Virtual Keyboard, Qt Quick 3D, Qt MQTT and Qt Graphs matter most for an HMI. Under LGPL use, they force either GPL for the application or a commercial licence.

**Implications for `ehs_port_to_mpu.md` §4** [inference]:
- The licence list there is correct. Add the LLVM exception for WAMR.
- SWUpdate's GPL core runs as a separate process, so it does not affect a proprietary runtime that only uses its IPC library. Even so, RAUC (LGPL) stays the simpler choice.
- Avoid the GPL-only Qt modules if Qt is ever used.

---

## 3. LVGL XML licence and the runtime loader

**Result: corrected.** The prior note said "the two published versions of the LVGL XML License contradict each other on whether runtime loading is allowed". That is not accurate. The actual sequence is below.

### 3.1 The XML License text (v1.0, June 2025) [primary, read from the lvgl/lvgl git history]

The licence was added in commit 90a8a19 on 2025-06-17 (PR #8438). It was re-added, with an "Overview/purpose" paragraph and the word "format" replaced by "Specification", in commit 9d810a4 on 2025-10-01 (PR #8984). It shipped in the v9.4 docs at `docs/src/details/xml/xml/license.rst`. Both versions say the same things on the points that matter:
- Permitted: "Use the LVGL XML loader freely in accordance with its MIT license."
- Permitted: "Load XMLs at runtime using LVGL's XML loader."
- Permitted: "Share LVGL-based projects with XML files as part of a commercial product or firmware."
- Permitted: "If you are building embedded software using LVGL, **this license does not affect you**."
- **Restricted** without written permission from LVGL LLC: "Create, release, or distribute any **UI editor, visual builder, layout designer, code generator, or tool** that reads, writes, or interprets XML files using this Specification — **whether commercial, open-source …**".
- **Restricted:** "Implement or extend this Specification in any **software or platform that provides UI creation, editing, or design capabilities similar to those in the LVGL UI Editor**."
- Closing line: "All rights not expressly granted herein are reserved by LVGL LLC."

The 9.3 docs page shows the same terms. https://lvgl.io/docs/open/9.3/details/auxiliary-modules/xml/license (redirect from docs.lvgl.io/9.3/…)

So the licence text itself never forbade runtime loading in commercial firmware. It **does** forbid a third-party UI editor or code generator from reading or writing the LVGL XML format.

### 3.2 What changed in 2026 [primary]

1. **v9.5.0 (18 Feb 2026) removed the XML engine.**
   - Changelog: "XML Engine Removed. XML UI engine development continues outside the main repository"; "feat(xml): remove the XML parser and loader (PR 9565)". https://lvgl.io/docs/open/9.5/CHANGELOG
   - Checked with `gh api` on the git trees: `release/v9.4` has 71 `src/**/lv_xml*` files; `release/v9.5` has **0**.
2. **Maintainer statements** in GitHub issue #9663 "XML License and roadmap" (opened 31 Jan 2026, closed as completed 5 Feb 2026). https://github.com/lvgl/lvgl/issues/9663
   - kisvegabor (1 Feb 2026): "there will be no open-source XML runtime loader in core LVGL going forward". Existing projects must stay on ≤ 9.4 or use the commercial engine, sold as a static library or as source under commercial terms.
   - AndreCostaaa (3 Feb 2026) claims the loader "has not been MIT licensed since commit 9d810a4". That commit only added the licence documentation, and the LVGL LICENCE file stayed MIT, so this claim conflicts with the licence text and with the July 2026 blog (below).
3. **Forum thread "XML engine further plan"** (Feb–Mar 2026). https://forum.lvgl.io/t/xml-engine-further-plan/23587
   - LVGL services team: LVGL 9.5 stays MIT, while `lv_xml` "will continue development under closed source".
   - AndreCostaaa: "The LVGL XML engine is no longer open source."
   - kisvegabor: the team "decided to make the XML loader proprietary".
4. **Latest resolution: LVGL blog "The Future of the XML Engine", 27 July 2026.** kisvegabor linked it from #9663 on 30 Jul 2026. https://lvgl.io/blog/announcement-lv_xml-removal-from-v9-5
   - The XML engine "moved out of the `lvgl/lvgl` repository", is "no longer open source", and is "licensed under a custom LVGL license, not MIT".
   - "On your device, runtime XML loading is a licensed feature." "There's no free tier for on-device use."
   - "License holders get either a precompiled library or the source code."
   - "The LVGL library stays MIT." "**Versions up to 9.4 keep their MIT XML engine forever.**" This also settles the 9d810a4 claim in favour of MIT for ≤ 9.4.
   - "The Editor exports plain C code that depends only on MIT-licensed LVGL."
5. **The Pro docs agree with the blog.**
   - "the engine that can load XML files at runtime is not publicly available"; for product use, contact LVGL. https://lvgl.io/docs/pro/integration/loading-xml-files.mdx
   - The open docs on master (LVGL v9.6.0, released 16 Sep 2026) now carry a single redirect page, "XML documentation has moved to the LVGL Pro Docs" (commit b9291be, 26 May 2026). The separate XML License page no longer exists in the master docs. https://lvgl.io/docs/open/xml/xml/license
   - The Pro Editor licence is free for personal, educational and non-commercial use. Commercial development needs a per-product or platform licence. https://lvgl.io/docs/pro/editor/license.mdx
   - Pro General Terms are version 2025-08. A Starter tier at about US$1,000/yr/seat was seen [secondary snippet]. https://pro.lvgl.io/terms, https://pro.lvgl.io/pricing

### 3.3 The contradiction and its resolution

- **Contradiction:** the June/Oct 2025 XML License says the loader is MIT and runtime loading is free. The 2026 statements (issue, forum, Pro docs) say the engine is proprietary and runtime loading is paid.
- **Resolution (blog, 27 Jul 2026):** both are true for different versions.
  - LVGL ≤ 9.4 ships an MIT XML loader, and runtime loading in commercial firmware is allowed.
  - LVGL ≥ 9.5 has no XML loader in the open-source tree. The proprietary engine needs a paid licence for on-device use.
  - The 2025 XML Specification licence's restrictions on third-party editors and code generators were never relaxed. No newer version of that licence text was found.

### 3.4 Corrections and implications for EHS [inference]

- **Correct in `hmi_runtime_platforms.md` §2.21:** "The MIT LVGL XML module exposes `lv_xml_component_register_from_file()` …" holds for **LVGL ≤ 9.4 only**. EHS pins **LVGL v9.5.0**, which contains no `lv_xml_*` code at all.
- **Correct in `ehs_port_to_mpu.md` §2.3:** replace "two published versions … contradict each other on whether runtime loading is allowed" with the sequence in §3.3.
- Option (b), the data-driven runtime, should use **EHS's own format and loader** (for example, growing `ui_from_json.c`), not LVGL XML. This follows from two points:
  - EHS is itself a UI editor and code generator. Reading or writing LVGL XML would fall under the XML Specification licence restriction, whatever the runtime licence.
  - The only open-source LVGL XML loader is frozen at 9.4.

---

## 4. EU Cyber Resilience Act, Regulation (EU) 2024/2847

**Result: confirmed.** The dates in the prior notes are right.

Article 71:
- **Entry into force:** "on the twentieth day following that of its publication" in the OJ. Published OJ L, 2024/2847, **20.11.2024**, so in force **10 December 2024**.
- **General application:** "This Regulation shall apply from **11 December 2027**."
- **Early application:** "However, **Article 14 shall apply from 11 September 2026** and **Chapter IV (Articles 35 to 51) shall apply from 11 June 2026**."
  - Article 14 covers reporting of actively exploited vulnerabilities and severe incidents.
  - Chapter IV covers notified bodies.

Sources:
- European Commission CRA summary [primary]: entry into force 10 Dec 2024; Chapter IV 11 Jun 2026; reporting 11 Sep 2026; "fully applicable as of 11 December 2027". https://digital-strategy.ec.europa.eu/en/policies/cra-summary
- Article 71 text [secondary mirrors of the OJ text]: https://www.springlex.eu/en/packages/cra/cra-regulation/article-71/ and https://prighter.com/resources/laws/cyber-resilience-act/articles/article-71
- EUR-Lex itself (https://eur-lex.europa.eu/eli/reg/2024/2847/oj/eng) returned an empty page to WebFetch and could not be read directly this session.
- Commission CRA reporting page: the Single Reporting Platform is operational from 11 Sep 2026 [primary]. https://digital-strategy.ec.europa.eu/en/policies/cra-reporting

No postponement was found. Law-firm notes from July–September 2026 treat 11 Sep 2026 as live:
- Jones Day, Freshfields, Travers Smith; LexisNexis news of 11/09/2026 [secondary]. Example: https://www.jonesday.com/de/insights/2026/07/eu-cyber-resilience-act-24hour-reporting-duties-start-september-11-2026
- The Digital Omnibus is mentioned only for merging reporting channels, not for moving dates [secondary].
- I did not do a targeted search for a delegated act changing dates.

---

## 5. Windows CE / Windows Embedded Compact lifecycle

**Result: corrected for the WEC 2013 mainstream date; the rest confirmed.**

Microsoft Lifecycle pages [primary]. All follow the Fixed Lifecycle Policy. The pages store times as UTC 06:59:59 on the following day, which is 23:59:59 Pacific on the date shown here.

| Product | Start | Mainstream end | Extended end | Source |
|---|---|---|---|---|
| Windows Embedded Compact 2013 | 11 Aug 2013 | **9 Oct 2018** | **10 Oct 2023** | https://learn.microsoft.com/en-us/lifecycle/products/windows-embedded-compact-2013 |
| Windows Embedded Compact 7 | 15 Mar 2011 | 12 Apr 2016 | **13 Apr 2021** | https://learn.microsoft.com/en-us/lifecycle/products/windows-embedded-compact-7 |
| Windows Embedded CE 6.0 | 30 Nov 2006 | 9 Apr 2013 | **10 Apr 2018** | https://learn.microsoft.com/en-us/lifecycle/products/windows-embedded-ce-60 |

Raw table values, for reference:
- WEC 2013: "8/11/2013 8:00:00 AM | 10/10/2018 6:59:59 AM | 10/11/2023 6:59:59 AM".
- WEC 7: "3/15/2011 … | 4/13/2016 6:59:59 AM | 4/14/2021 6:59:59 AM".
- CE 6.0: "11/30/2006 … | 4/10/2013 6:59:59 AM | 4/11/2018 6:59:59 AM".

Licence sales:
- Microsoft "Frequently Asked Questions – CE Migration" (archived page, ms.date 2024-05-16) [primary]: "While Windows CE 2013 will reach end of extended support in late 2023, Microsoft will allow license sales to continue for Windows Embedded Compact 2013 until 2028. And of course, Windows CE devices can continue to be used indefinitely."
  - Also: "Microsoft does not currently have plans to provide extended support beyond 2023."
  - Windows 10 IoT Core Services "continues to receive security updates until 2029".
  - No exact day in 2028 is given.
  - Current canonical URL: https://learn.microsoft.com/en-us/previous-versions/windows/iot-core/windows_ce/windows-ce-migration-faq; the old URL https://learn.microsoft.com/en-us/windows/iot/windows_ce/windows-ce-migration-faq still serves it.
- CE 6.0 "end of licence 22 Feb 2022" was not re-checked; it remains [secondary].

**Correct in `hmi_runtime_platforms.md` §0.8 and §3.1:**
- "WEC 2013 mainstream support ended 9 Oct 2023" should read **9 Oct 2018**.
- Extended end 10 Oct 2023 is confirmed.
- "WEC7 extended support ended April 2021" is now confirmed (13 Apr 2021); drop the "unverified" flag.
- CE 6.0 extended end April 2018 is confirmed (10 Apr 2018).

---

## 6. Open items that remain

1. Siemens Unified Comfort SoC and architecture. Look for the UCP image's OSS readme on SIOS (often a "ReadMe_OSS" in the image download, entry 109825605) or a teardown.
2. Mitsubishi GOT2000 CPU and RTOS name. Possible sources: GOT2000 OSS notice, if one exists; JVN/ICS advisories naming a third-party stack.
3. Omron NA5 / NA5-V2 OS. Fetch https://www.fa.omron.co.jp/product/tool/na5-info/index_en.html from a non-blocked client.
4. C-more EA9 OS. Ask AutomationDirect support, or find a licence notice inside the programming-software install folder.
5. Kinco OS from a primary source.
6. Pro-face GP4000M / GP410* OS, and GP6000 OS (the OSS page lists GPLv2 and returned 403).
