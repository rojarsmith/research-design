# Runtime platforms and runtime architectures of established industrial HMI products

Research date: 2026-11-20. Scope: how established HMI vendors build the software that runs on the panel. For each vendor the note covers:
- the processor class and the operating system;
- the UI toolkit;
- how a project reaches the device;
- whether a PC runtime or simulator exists;
- remote access;
- driver count;
- runtime licensing.

The purpose is to compare EDT HMI Studio (EHS) with MPU-class products such as NOVAKON iFace Designer. EHS turns a no-code LVGL design into generated C, which is compiled into bare-metal STM32 firmware. NOVAKON is covered in a separate note and is not repeated here.

## Labels used

- **[primary]**: the vendor's own datasheet, manual, product page, store page, release note or press release about its own product. This includes vendor PDFs hosted by a distributor (for example a Delta datasheet on rs-online).
- **[secondary]**: trade press, distributor descriptions, forums, third-party blogs and reseller listings.
- **[inference]**: my own judgement from the cited evidence or from general background knowledge that I could not verify this session. These items are flagged "unverified" where relevant.

**Method note.** Most evidence came from search-engine summaries plus directly fetched pages. Two Siemens PDFs were saved and read in full with pdftotext:
- the Unified Comfort Panels operating instructions, TIA V19 (11/2023);
- the WinCC Unified white paper.

Several vendor PDFs timed out or were refused (HTTP 403 or oversize), including:
- the Weintek datasheets on mouser.com;
- the Omron NA5 specification page;
- the Mitsubishi GOT2000 catalogue L08311;
- the Siemens forum.

Facts from those come only from search snippets and are flagged. Search summaries sometimes contradict each other. Where they do, the more conservative reading is kept.

---

## 0. Executive summary [inference unless tagged]

1. **No mainstream HMI vendor compiles the customer's project into firmware.** Every vendor checked ships a fixed runtime, usually called "firmware", "OS" or "runtime". The design tool downloads a project or data file to it over USB, Ethernet or SD. The runtime and OS image are updated separately and must match the editor version:
   - Siemens "Update OS" [primary];
   - Mitsubishi "Project data, OS" write [secondary];
   - Red Lion Crimson firmware [primary];
   - Exor JMobile runtime [primary].

   EHS's compile-to-firmware model therefore matches the vendor-runtime market only in look and feel, not in architecture. It is closer to Qt for MCUs, Crank Storyboard on MCUs, TouchGFX, or LVGL used directly.
2. **The processor class is overwhelmingly ARM Cortex-A, 32-bit and mostly 600 MHz–1.6 GHz:**
   - Cortex-A7 in Delta (bigger models), Kinco, Samkoon and MCGS;
   - Cortex-A8 in Delta (smaller models) and older Weintek iE;
   - Cortex-A9 in Exor eX700;
   - quad-core Cortex-A17 in Weintek cMT X.

   x86 (Intel Atom, Celeron or Core) appears only in the PC-runtime and Panel-PC tiers:
   - Pro-face SP5000 Open Box;
   - B&R Terminal OS panel PCs;
   - Omron Soft-NA;
   - Siemens Unified PC.

   MCU-class panels survive only at the bottom end. AutomationDirect C-more Micro is an example; one secondary source says the EA9 runs an RTOS on an ARM9.
3. **The operating system on the panel is now mostly embedded Linux.** Evidence:
   - Weintek: "Linux based operating system" [primary datasheet via distributor].
   - Delta DOP-100: "Embedded Linux System" [primary].
   - Exor eX700: "Linux RT" [primary].
   - Red Lion FlexEdge: Alpine Linux, kernel 4.14 [primary].
   - Unitronics UniStream: Linux [primary FAQ].
   - Siemens Unified Comfort: "proprietary operating system". Its preinstalled apps (Chrome, VLC, Okular, LibreOffice) point to Linux, and a forum answer says so [primary plus inference].
   - Rockwell PanelView 5000: embedded Linux [secondary].
   - MCGS McgsPro: for Linux TPCs [secondary].
   - Beijer: new OS3 with containers [primary].

   Holdouts:
   - Schneider Harmony ST6/GTU: "Proprietary" [primary].
   - Pro-face GP4000: not disclosed. Release notes show a separately versioned "OS Core".
   - Mitsubishi GOT2000: proprietary monitor OS (inference).
   - Several legacy Windows CE / Windows Embedded Compact lines: Beijer X2 pro, Siemens classic Comfort, Rockwell PanelView Plus 7 Series A, Omron NA (unverified).
4. **UI toolkits are mostly the vendors' own engines.** They are typically C/C++ on framebuffer or OpenGL ES, and none of the big HMI vendors publicly says it uses Qt or LVGL on the panel. The exceptions are documented ones:
   - Siemens WinCC Unified: an open-source web runtime with HTML5, SVG and JavaScript [primary].
   - CODESYS TargetVisu for Linux: Qt 6 plus Qt WebEngine [primary].
   - B&R mapp View: HTML5 in an embedded browser [primary].
   - Exor: a Chromium web engine alongside its own JMobile runtime [primary].
   - Weintek cMT X Web Panels: a Chrome-based browser [secondary].
   - Ignition Perspective: HTML5 [primary].
5. **Remote access is standard and mostly free.** Every vendor offers VNC or a proprietary viewer:
   - Weintek: cMT Viewer is free; EasyAccess 2.0 is a paid cloud service.
   - Siemens: Sm@rtServer/VNC and a web client.
   - Unitronics: VNC (up to 8 connections) and a web server.
   - Delta: VNC.
   - C-more: free browser-based Remote Access.

   The 2020–2026 trend is HTML5 web clients: Siemens Unified, Weintek WebView/EasyWeb 2.0, Exor, B&R, Beijer WebIQ, and the Mitsubishi GOT Mobile licence. Cloud relays (Weincloud/EasyAccess, Corvina, SamkoonCloud) are increasingly bundled.
6. **Driver count is the moat.** Figures found:
   - Weintek: "400+ communication drivers" [secondary, distributor quoting Weintek].
   - Red Lion: "300+ drivers" [primary].
   - Pro-face: per-manufacturer tables with dozens of brands [primary, count not totalled].
   - Delta DIAScreen: mainly Delta plus Modbus/CANopen [primary].

   A new entrant with Modbus RTU/TCP plus 5–10 major PLC families covers most small-OEM needs. However, this is the dimension buyers compare (inference).
7. **Runtime licensing splits by vendor type:**
   - **Hardware vendors** (Weintek, Delta, Kinco, Samkoon, MCGS, Unitronics, Red Lion, C-more, Maple): the runtime is free on their own panels. The editor is free or nearly so, and money is made on hardware.
   - **Platform vendors** (Siemens Unified PC, Exor JMobile PC Runtime about £456, Mitsubishi GT SoftGOT2000 key about US$3.8k at retail, Pro-face WinGP and BLUE Runtime, Omron Soft-NA, Ignition Edge Panel about US$1.5–2k, Qt per-device distribution licences): they charge for the runtime on third-party PCs.
   - **CODESYS:** TargetVisu for Linux SL is now €0, because licensing moved to the PLC application.
8. **Windows on HMIs is shrinking at the panel level:**
   - Windows Embedded Compact 2013 lost extended support on 10 Oct 2023 [secondary, citing Microsoft]. OEM licence sales continue until 2028 [secondary].
   - CE 6.0 lost extended support in April 2018 [secondary].
   - Vendors responded with Linux (Siemens Unified, Rockwell PV5000, Beijer OS3, MCGS McgsPro), with web runtimes, or with Windows IoT Enterprise LTSC on x86 Panel PCs.
   - Windows IoT Enterprise LTSC 2024 per-device licence tiers are tied to CPU class. Reseller prices seen: Entry ≈ US$54–88, Value ≈ US$110–132, High End ≈ US$238 (CAD on cdw.ca). An unverified reseller claims about US$295.

   This cost is one reason ARM/Linux dominates sub-US$1,000 HMIs.

**Implication for EHS [inference].** Two different products sit under the name "HMI tool":
- **(a) A vendor-runtime panel.** A fixed Linux runtime on Cortex-A loads a project file, with drivers, web/VNC, a PC simulator and a free editor. This is what Weintek, Delta, Kinco, MCGS and NOVAKON sell.
- **(b) A code-generator for embedded GUIs.** This is what EHS, TouchGFX, Qt for MCUs and LVGL Pro sell to firmware teams.

To compete with (a), EHS would need either a Linux runtime build of LVGL that loads XML or binary project files, or an MCU firmware with a built-in interpreter. LVGL's own Pro "load XML at runtime" engine exists, but it is not publicly available [primary].

---

## 1. Summary table

Legend: P = primary, S = secondary, I = inference. "Project file" means the editor downloads data to a fixed runtime and nothing is compiled per project.

| Vendor / product | Processor | Panel OS | UI toolkit | Project delivery | PC runtime / sim | Remote | Drivers | Runtime licence |
|---|---|---|---|---|---|---|---|---|
| Weintek cMT X (EasyBuilder Pro) | Quad-core Cortex-A17 1.6 GHz on high-performance models (S/P) | Linux-based (P datasheet via distributor) | Own engine (I) | Project file plus firmware/OS updates (S/I) | Offline/online simulation; cMT Viewer on Windows (P manual) | cMT Viewer (free), VNC, WebView/EasyWeb 2.0 browser, Weincloud/EasyAccess 2.0 (S) | 400+ (S) | Free on Weintek HW; EBPro licence-free (S) |
| Weintek MT8000iE | Cortex-A8 600 MHz or 1 GHz RISC (S, brochure) | Linux-based on sibling iE/XE (S) | Own engine (I) | Project file | Simulation | VNC, EasyAccess | same | Free on HW |
| Weintek cMT-SVR / FHDX (headless) | Quad-core 32-bit RISC 1.6 GHz (S) | "Linux based operating system" (P via distributor) | Own engine; displays on cMT Viewer/iPad/PC | Project file | as above | cMT Viewer free | same | Free |
| Siemens Unified Comfort (MTP700–2200) | Not published in the manual (I: ARM SoC likely, unverified) | "Proprietary operating system" (P); Linux-based (S forum; I from Chrome/VLC/Okular/LibreOffice) | Web runtime with HTML5/SVG/JavaScript, "open-source runtime engine" (P) | TIA Portal download; separate "Update OS" (P) | WinCC Unified PC Runtime on Windows, PowerTag-licensed (P) | Web client, SmartServer/VNC (TigerVNC), Industrial Edge apps (P) | S7 family plus third-party drivers (I) | Panel runtime included; PC runtime paid by PowerTags (P) |
| Siemens classic Comfort | x86 or ARM by size (I, unverified) | Windows CE 6.0 (I, unverified) | WinCC Advanced runtime | TIA download | WinCC RT Advanced (paid) | Sm@rtServer | — | Included on panel |
| Pro-face GP4000 (GP-Pro EX / BLUE) | Not published (I) | Not published; separate "Sub-system/OS Core" version (P release notes) | Own engine | Project file plus OS core from editor (P) | WinGP (Windows IPC runtime) and BLUE Runtime on Windows 11 (P) | Pro-face Remote HMI / Pro-face Connect (I) | Large per-maker tables (P) | Free on panel; WinGP/BLUE Runtime paid (I) |
| Pro-face SP5000 Open Box | x86 (I) | Windows Embedded Standard 7 and Windows 10 IoT Enterprise 2019 LTSC 32-bit (P) | WinGP or BLUE | — | — | — | — | Windows licence included in SKU |
| Schneider Harmony ST6 / GTU / STO | n/a | "Proprietary" (P catalogue); Harmony iPC = Windows | EcoStruxure Operator Terminal Expert runtime | Project file | Simulation in EOTE (I) | Web Gate, VNC (I) | — | Included |
| Mitsubishi GOT2000 (GT27/GT25) | Not found | Proprietary "standard monitor OS" written by GT Designer3 (S manual summary; I) | Own engine | "Project data, OS" written together (S) | GT SoftGOT2000, USB key GT27-SGTKEY-U ~US$3.8k retail NZ (S) | GOT Mobile browser function (licensed, ¥380k per 10 lic.), VNC (S/I) | Many (I) | Panel free; SoftGOT and GOT Mobile paid (S) |
| Omron NA5 / NA5-V2 | Not published in sources reached | Not confirmed; Windows Embedded Compact 7 (I, unverified) | Own engine with VB.NET scripting (I) | Sysmac Studio download | Soft-NA on Windows 10/11 Pro, Atom x5-E3940 or Celeron N4000 class (P) | VNC (P spec snippet) | NJ/NX/NY and others | Soft-NA paid (I) |
| Delta DOP-100 (DIAScreen) | Cortex-A8 800 MHz; dual-core Cortex-A7 1 GHz on 12"/15" advanced (P) | "Embedded Linux System" (P) | Own engine plus Lua scripting (P) | Project file | DIAScreen simulation (I) | VNC server, FTP, web monitoring (P) | Delta plus Modbus/CANopen; count not found | Free (I) |
| Kinco GL / Green series (DTools) | ARM Cortex-A7 800 MHz (S) | Linux (S distributor only) | Own engine | Project file | DTools simulation (I) | Kinco cloud (I) | — | Free |
| Samkoon SK series (SKTOOL) | Dual-core Cortex-A7 1 GHz (S, selection guide) | Embedded Linux (S, single blog) | Own engine | Project file | Simulation (I) | SamkoonCloud app (free) (S) | — | Free |
| MCGS 昆仑通态 TPC (McgsPro / MCGS 嵌入版) | Cortex-A7 dual-core 800 MHz to 1 GHz (S manual summaries) | Newer TPCs: Linux (McgsPro); older: WinCE (MCGS 嵌入版) (S forums) | Own engine | Project file (.MCP / .MCE) | Simulation in editor (I) | MCGS cloud (I) | Many CN PLCs (I) | Embedded editor free; PC "通用版/网络版" paid per point (S) |
| Beijer X2 pro / X2 extreme (iX) | ARM (I) | Windows Embedded Compact 2013 (S, garbled table); X2 extreme 15 HP web: Linux plus Chromium kiosk (P) | iX runtime (.NET-based, I) | Project download | iX Runtime on any Windows PC (S) | Web server, VNC (I) | — | Panel included; PC runtime paid (I) |
| Beijer X3 (iX3 / OS3 / WebIQ) | Not published | OS3: "software container technology and multi-app support" (P); Linux (I) | iX3 (C#, NuGet) or WebIQ web HMI (P) | Project download or container app | — | Web | — | — |
| Red Lion FlexEdge DA50A/DA70A, Graphite (Crimson 3.2) | ARM, 1 GB RAM (S) | Linux; Alpine since 3.2.1019, kernel 4.14.323 (P) | Crimson runtime (own) | Crimson database download; firmware updated with it (P) | Crimson emulator (I) | Web server, Crimson remote (I) | 300+ drivers (P) | Crimson free (I) |
| AutomationDirect C-more EA9 | ARM9 (S, unverified) | RTOS (S, unverified) | Own engine | Project file | C-more simulator (I) | Free browser Remote Access & Control (P) | AD plus major PLCs | Software free download (P) |
| Maple Systems cMT / HMI5000 | Weintek-made hardware (S) | As Weintek (I) | EBPro | Project file | As Weintek | EasyAccess 2.0 incl. a Linux client (S) | As Weintek | Free |
| Unitronics UniStream (UniLogic) | Dual-core CPU, core not named (S) | Linux (P FAQ) | Own engine | Combined PLC plus HMI download | UniLogic simulator (I) | VNC (port 5900, up to 8 connections) and web server (P) | Unitronics plus Modbus etc. | UniLogic free (P/S) |
| Exor eX700 (JMobile) | Cortex-A9 dual 800 MHz (P datasheet 2017) | "Linux RT" (P) | JMobile runtime plus Chromium web engine (P) | JMobile project download; runtime per platform (UN60/UN65/UN70) (P) | JMobile PC Runtime on Win32/64 ~£456 per activation (S) | Corvina cloud, web (P) | "multiple protocols" | Free on Exor HW; PC runtime paid |
| B&R Power Panel C70 / C80 / T-series | C80: hypervisor splits CPU; T-series: embedded browser (P) | Automation Runtime RTOS, plus embedded Linux for HMI on C80 (P); Terminal OS for panel PCs (P); Linux for B&R is Debian-based (P) | mapp View (HTML5 in browser) or VC4 (P) | Automation Studio download | ARsim (I) | VNC, web | Fieldbuses | AR licence by HW (I) |
| CODESYS TargetVisu for Linux SL / WebVisu | x86_64, ARM, ARM64 (P) | Debian 12+, glibc 2.36 (P) | Qt 6 plus Qt WebEngine (P) | Part of the CODESYS application download (P) | CODESYS simulation (I) | WebVisu (HTML5) (P) | Fieldbuses | TargetVisu for Linux SL €0, licence on PLC side (P) |
| Ignition Edge Panel / Perspective | x86 or ARM (P docs) | Windows, Linux, macOS (P) | Java gateway plus HTML5 Perspective or Vision (Java) (P) | Gateway project | — | Browser | Many | Edge Panel ~US$1.5–2k (S) |
| Crank Storyboard | MCU to MPU (P) | Linux, QNX, FreeRTOS, Zephyr, Android, Windows (P) | Storyboard Engine; back ends fbdev, DRM, OpenGL ES, X11, VGLite (P) | Storyboard app file (.gapp) loaded by the engine (I) | Simulator | — | n/a | Commercial; dev seat ~US$4k/yr (S, unverified) |
| Qt for MCUs / Qt on Linux | MCU (STM32, i.MX RT, RW612) and MPU (i.MX 93) (P) | Bare metal, RTOS, Zephyr, Linux (P) | QML compiled to C++ (Qt Quick Ultralite) or Qt Quick | Compiled firmware (MCU) or app binary (Linux) | Desktop | — | n/a | Commercial; per-device distribution licence (P) |
| LVGL on Linux | any | Linux | LVGL with fbdev, DRM/KMS (+GBM/EGL), Wayland or SDL drivers (P) | Compiled app, or XML loaded at runtime (Pro engine not public) (P) | SDL simulator | — | n/a | MIT |

---

## 2. Vendor notes

### 2.1 Weintek (MT8000iE, cMT X, cMT-SVR; EasyBuilder Pro)

Hardware and OS:
- The cMT X high-performance models cMT3092X, cMT3152X and cMT3162X use a "Quad-core Cortex A17 1.6 GHz" CPU. [secondary: Design World] https://www.designworldonline.com/weintek-releases-super-fast-hmis-the-cmt3092x-cmt3152x-and-cmt3162x/
- Other cMT X datasheets say only "Quad-core RISC". [primary datasheet titles via Mouser] https://www.mouser.com/datasheet/3/6153/1/cMT2108X2_Datasheet_ENG.pdf
- The headless cMT-FHDX-220/820 datasheets list "Quad-Core 32-bit RISC 1.6GHz (High Performance dedicated 3D Processor), Linux based operating system". [primary datasheet hosted by distributor] https://www.wachendorff-prozesstechnik.de/en/product-groups/archive/discontinued-products/archive-hmi-iiot-edge-gateways/TV-machine-interface-CMT-FHDX-220/pdf and https://www.mouser.com/datasheet/3/6153/1/cMT-FHDX-820_Datasheet_ENG.pdf
- The quad-core Cortex-A17 at 1.6 GHz matches the Rockchip RK3288 profile. [inference, unverified]
- The MT8000iE uses "32 bits RISC Cortex-A8 600MHz" on some models and "32-bit RISC 1GHz" on others. [secondary: Weintek brochure copy] https://www.wolfautomation.com/content/weintek-ie-xe-ip-series-hmi-brochure.pdf
- Sibling iE/XE models are listed as "Linux-based operating system, EasyBuilder Runtime". [secondary: Wachendorff] https://www.wachendorff-prozesstechnik.de/en/product-groups/archive/discontinued-products/archive-hmi-iiot-edge-gateways/control-device-hmi-MT8090XE

Features:
- cMT X: "over 400 communication drivers", OPC UA, MQTT, databases, and "mobile monitoring through WebView, cMT Viewer and VNC Viewer". [secondary: Wachendorff, a Weintek distributor] https://www.wachendorff-prozesstechnik.de/en/product-groups/hmi-iiot-edge-gateways/information-on/cmtx-hmi-wachendorff-prozesstechnik
- CODESYS V3 can run on cMT X Standard, Advanced and Headless, with "the HMI divides the processor cores between the EasyBuilder Pro visualization and the CODESYS application". [secondary] same URL.
- The CODESYS runtime on Weintek panels is Weintek's own build. [secondary: Maple Systems] https://maplesystems.com/support-center/controller-connection-guides/maple-systems-built-in-codesys-ebpro/

Web and remote access:
- WebView: point a browser at the HMI's IP address and log in, with no client software. [secondary: Maple Systems] https://maplesystems.com/technical-notes/webview-quick-start/
- EasyWeb 2.0 manual. [primary] https://www.rusavtomatika.com/upload_files/documents/weintek/Document/UM0/cMT-X-Series-Easyweb-2.0-UserManual-eng.pdf
- A PLC Web Browser object (HTTPS, "most HTML5 webpages") is available on cMT3000X. [secondary] https://www.lamonde.com/news/new-feature-announcement-plc-web-browser-on-weintek-cmt-x-series-hmi
- In Nov 2025 Weintek launched the "cMT X Web Panel" line, for example the cMT2058XH: a 4.3" "Chrome-based web panel" with quad-core RISC, 4 GB flash and 1 GB RAM. [secondary: Control.com] https://control.com/news/weintek-debuts-compact-4.3-inch-cmt2058xh-html5-web-panel-with-web-browsing/
- cMT Viewer is a free client for Android, iOS and Windows that remotely shows the cMT-SVR screen. [secondary quoting Weintek docs] https://www.wachendorff-prozesstechnik.de/en/product-groups/hmi-iiot-edge-gateways/products/cmt-series-hmi-iiot-edge-gateways-wachendorff-prozesstechnik/Smart-HMI-Server-cMT-SVR-100
- The cMT-SVR is a "frontless HMI": runtime and communication in a box, with the display on a tablet or PC. [secondary] same URL.

Project delivery, PC side and licensing:
- EasyBuilder Pro builds a project file that is downloaded to the panel's fixed runtime. Firmware/OS images are separate; newer OS versions require newer EBPro and cMT Viewer for encrypted connections. [secondary: Maple/Weintek docs] https://maplesystems.com/Software/EBProReleaseNotes.htm
- cMT Viewer on Windows can run online or offline simulation of cMT/cMT X projects from the command line. [primary: EBPro V6.10.01 manual chapter 27] https://www.rusavtomatika.com/upload_files/documents/weintek/EBPro/UserManual/eng/UserManual_separate_chapter/Chapter_27_EasySimulator.pdf
- The PC simulation has documented limitations, for example video input and web browser objects. [primary] https://www.rusavtomatika.com/upload_files/documents/weintek/EBPro/UserManual/eng/UserManual_separate_chapter/Appendix-C-PC-Simulation-Limitations-UserManual-eng.pdf
- I found no evidence of a commercially sold Weintek PC runtime for production use; the cMT-SVR plus a PC viewer fills that role. [inference]
- EasyBuilder Pro is described as licence-free. [secondary: distributor]
- The runtime is free with Weintek hardware. [inference]

### 2.2 Siemens (Unified Comfort Panels, Unified Basic Panels, WinCC Unified; classic Comfort)

OS and architecture (Unified Comfort Panels operating instructions, TIA V19 11/2023, A5E46641217-AE):
- "High level of IT security to protect the system against hacker attacks through a proprietary operating system." [primary] https://cache.industry.siemens.com/dl/files/164/109826164/att_1161674/v1/HWUnifiedComfortPanelsenUS_en-US.pdf
- "Visualization based on SIMATIC WinCC Unified, the new web-based visualization system in the TIA Portal". [primary] same URL.
- "Integrated Industrial Edge functionality for simple expansion of standard functionality through apps." [primary] same URL.
- The manual has an "Open Source Software" licence section. It does not state the CPU. [primary] same URL.
- A firmware/OS image must match the WinCC version. "Update OS" is run from WinCC or ProSave, and "updating the operating system deletes data on the HMI device". [primary] same URL.

Remote access:
- "Enable web access to runtime" lets any browser reach the runtime project. Operation is asynchronous, so the client does not mirror the panel. [primary] same URL.
- "Enable Smart Server": access via the SmartClient application "or a VNC client, such as Tigervnc". [primary] same URL.

WinCC Unified white paper:
- "WinCC Unified uses an open-source runtime engine that supports full native web technologies, such as HTML 5, Scalable Vector Graphics (SVG), and Javascript". [primary] https://assets.new.siemens.com/siemens/assets/api/uuid:2708335d-45e8-4bce-8e47-2aae6ace80b9/unfd-wincc-0920-wincc-whitepaper-2-1-.pdf
- Preinstalled apps: Google Chrome Browser, VLC Media Player, Okular PDF Viewer, LibreOffice. [primary] same URL.

Linux:
- Okular and LibreOffice are KDE/Linux applications, so the "proprietary OS" is in practice a hardened Linux. [inference]
- A Siemens support forum answer states the Unified Panels' OS "is based on Linux". [secondary, snippet only; page returned 403] https://support.industry.siemens.com/forum/cn/en/post/1218014
- Trade press says it is "based on Simatic Industrial OS", Siemens' Debian-based industrial Linux. [secondary] https://www.controldesign.com/sensing/vision-systems/product/55293681/siemens-siemens-visualization-system

Other products:
- Unified Basic Panels (MTP400/1000/1200 Unified Basic) can be configured from WinCC Unified Basic V18 Update 3, with web client, remote access and open-source software. [secondary: Control Design; primary: Siemens promo] https://controldesign.com/displays/panel-meters/product/55087983/siemens-siemens-unified-basic-panel
- WinCC Unified PC Runtime is sold by PowerTag tiers with Powerpack upgrades and an SUS subscription. Prices sit behind the Industry Mall login. [primary: Industry Mall listings] https://mall.industry.siemens.com/mall/en/WW/Catalog/Products/10416453
- The PC runtime runs on Windows. Siemens also promotes WinCC Unified for Linux users as a Docker container. [secondary: search summary of Siemens CN page] https://www.siemens.com/zh-cn/products/simatic-hmi/wincc-unified/
- Classic Comfort Panels (WinCC Advanced) are widely known to run Windows CE 6.0. [inference, unverified this session]
- Classic and Unified PC runtimes need paid licences, while panel runtimes are included with the hardware. [inference]

### 2.3 Pro-face / Schneider Electric (GP4000, SP5000, BLUE, GP-Pro EX, Harmony)

Pro-face:
- GP-Pro EX release notes list a "System/Runtime" version and a separate "Sub-system/OS Core: Ver.4.9.x (SP5000 series, GP4000 series, LT4000 series…)". "The operating system has been updated to address a vulnerability" on GP-4000. [primary] https://www.proface.tech/otasuke/download/update/proex/proex/v409/Readme_E_GPProEX_v409450.txt
- The OS core is shipped with the editor and written alongside the project. The GP4000 OS itself is not named in any source found. [inference]
- The EWF mentions in the same notes refer to the SP5000 Open Box (Windows). [primary] same URL.
- SP5000 Open Box runs Windows Embedded Standard 7 or Windows 10 IoT Enterprise 2019 LTSC 32-bit. [primary: WinGP operating environment] https://www.proface.com/data/manual/gpproex/new/refer/mergedProjects/wingp/wingp_mm_supportedmodels.htm
- The WES7 Open Box supply ended July 2025. [primary FAQ via search]
- WinGP is "the IPC runtime engine" that runs GP-Pro EX screens on a Windows PC. [primary] https://www.proface.com/en/product/soft/gpproex/wingp
- The BLUE runtime environment is Windows 11 or Windows Server 2022; older BLUE Runtime 3.4 SP1+ ran on Windows 10. [primary] https://www.proface.com/data/manual/blue/4.0/en/featureguide/before_use/03-3-009.htm
- Drivers are documented per manufacturer (Siemens, Rockwell, Mitsubishi, Omron, Bosch Rexroth, WAGO, Bachmann, etc.). [primary] https://www.proface.com/data/manual/gpproex/v4_0/device/sie.htm and https://www.proface.com/en/product/soft/gpproex/driver

Schneider Harmony:
- The catalogue lists the terminal OS as "Proprietary" for Harmony STO, ST6/STM6 and GTU/GTUX, and Windows for Harmony iPC. The configuration tool is EcoStruxure Operator Terminal Expert. [primary] https://download.schneider-electric.com/doc/DIA5ED2140703EN/DIA5ED2140703EN.pdf
- EcoStruxure Machine SCADA Expert does not run on generic Linux, only on Schneider's Linux-based HMI IoT Box. [primary FAQ] https://www.se.com/us/en/faqs/FAQ000279925/

### 2.4 Mitsubishi Electric (GOT2000, GT Designer3, GT SoftGOT2000)

- GT Designer3 writes "Project data, OS" to the GOT. You tick the standard monitor OS, communication driver, extended function OS and communication settings, then click "GOT Write". [secondary: manual reproduced on ManualsLib] https://manualslib.com/manual/1254790/Mitsubishi-Electric-Got2000-Series.html?page=36
- So the runtime, the drivers and optional feature OS modules are installed from the editor, and the project is data. [inference]
- The underlying kernel is not disclosed. It is a proprietary monitor OS, not Linux or Windows on the panel. [inference, unverified]
- GT SoftGOT2000 (Windows PC runtime) needs a USB licence key GT27-SGTKEY-U per PC. RS lists it at about NZ$3,824 / SGD 3,079 ex-GST. [secondary: RS listing] https://nz.rs-online.com/web/p/plc-programming-software/8280544/
- The GOT Mobile function lets tablets, phones and PCs connect over (W)LAN with a browser, for up to 5 devices at once. It needs a licence. Japanese reference price (Feb 2023): GT25-WEBSKEY series about ¥380,000 for 10 licences. [secondary] https://www.mitsubishielectric.com/fa/products/hmi/got/items/license/index.html

### 2.5 Omron (NA5 / NA5-V2, NB)

- Sysmac Studio NA applications "can either be run on the NA5 dedicated hardware platform or the IPC" via Soft-NA. [primary] https://automation.omron.com/en/us/products/family/Soft%20NA
- Soft-NA needs Windows 10 Pro 1903+ 64-bit or Windows 11 Pro 24H2+, an Atom x5-E3940 or Celeron N4000 class CPU, and 4 GB RAM. [primary, via search of Omron NA5-V2 lineup] https://www.ia.omron.com/products/family/3960/lineup.html
- The NA5-V2 has an Ethernet port "for connecting to devices and VNC clients" and 256 MB of user data. [primary datasheet snippet] https://www.mouser.lt/pdfDocs/NA5-V2-Datasheet-202602.pdf
- The panel OS was not confirmed. The NA series' VB.NET scripting suggests Windows Embedded Compact 7. [inference, unverified]
- The NB series (low-cost, NB-Designer) is generally reported to be a Weintek-style OEM ARM panel. [inference, unverified]

### 2.6 Delta (DOP-100, DIAScreen)

- "The Basic, Standard, and Handheld HMI adopt the latest Cortex-A8 (800 MHz)". "12" / 15" advanced HMIs feature the dual-core Cortex-A7 1GHz". "Embedded Linux System". DIAScreen has a built-in Lua editor. [primary: Delta catalogue] https://deltronics.ru/images/catalogue/DOP-100_C_EN_20200109_web.pdf and https://assets.rs-online.com/v1695390379/Datasheets/ef456d47ccf0267a97630ef6c2978199.pdf
- Cloud integration includes FTP Server, VNC Server and Web Monitoring. [primary/secondary] https://www.mouser.in/new/delta-electronics/delta-dop-100-hmi-platforms/
- DIAScreen fieldbus support covers Modbus, CANopen and Modbus TCP, plus Delta PLC tag sharing. A full driver count was not found. [primary] https://deltronics.ru/images/catalogue/DIAStudio_C_EN.PDF

### 2.7 Kinco (Green GL/GT/G series, DTools)

- "The GREEN series products use the ARM Cortex-A7 architecture industrial-grade processors". The GL series has an 800 MHz CPU with 128 MB RAM and 256 MB flash. DTools is the editor. [secondary: Anaheim Automation, a Kinco distributor] https://anaheimautomation.com/hmi-and-plc/human-machine-interface-hmi/kinco-gl-type/
- One distributor says Kinco HMIs are "based on a Linux Operating System". [secondary, unverified]
- The legacy MT4000/MT5000 series used Kinco HMIware. [secondary]

### 2.8 Samkoon 显控 (SK/SA/AK/EA; SKTOOL)

- SK series: "Cortex A7 1GHz. Dual Core", 128 MB DDR3, 128 MB flash, SKTOOL editor. [secondary: Samkoon product selection guide copy] https://www.rusavtomatika.com/manuals/Product-Selection-Guide-20221026.pdf
- One third-party blog says the SK-102HE runs "embedded Linux 3.2.0". [secondary, low reliability] https://industrialmonitordirect.com/blogs/knowledgebase/resolving-samkoon-sk-102he-upload-lock-and-screen-saver-timeout
- SamkoonCloud remote client app is free on the App Store. [secondary] https://apps.apple.com/ba/app/samkoon/id1472715713

### 2.9 MCGS 昆仑通态 (TPC; MCGS 嵌入版 / McgsPro)

- TPC7012El: "Cortex-A7 2-core 800MHz". The TPC1431Ni has a multi-core Cortex-A7 at 1 GHz with McgsPro preinstalled. [secondary: manual summaries] https://manuals.plus/ae/1005007806356751
- McgsPro was created for the new hardware platform and Linux-based HMIs. The older MCGS 嵌入版 targets WinCE TPCs. Projects change from .MCE to .MCP when migrated. [secondary: CN forums/blogs] https://www.jx-auto.cn/thread-26744-1-1.html and https://blog.csdn.net/weixin_45875994/article/details/140553267
- The embedded (panel) editor is free. The PC "通用版/网络版" SCADA is charged per point. [secondary] https://www.toutiao.com/w/1673334860816387/
- MCGS is the clearest published Chinese example of a WinCE-to-Linux runtime migration in 2020–2024. [inference]

### 2.10 Beijer Electronics (X2, X3; iX, iX3, OS3, WebIQ)

- X2 pro models are "HMI with iX runtime". iX Runtime runs the project on X2 panels or on any Windows PC. [primary/secondary] https://www.beijerelectronics.us/en-US/Products/Operator___panels/X2___series/X2___pro/X2___pro___12
- The X2 pro OS appears in Beijer tables as Windows Embedded Compact 2013 (table garbled in extraction). [secondary, treat as likely]
- The X2 extreme 15 HP web variant runs "Linux with Chromium in Kiosk mode". [primary spec sheet via search]
- X3 and OS3 (announced Nov 2024, rollout from Q4 2024, iX-based X3 pro/extreme/marine added Sept 2025):
  - "The operating system utilizes software container technology and multi-app support".
  - iX3 adds C# and NuGet.
  - WebIQ is the web-HMI runtime and can run on the device or in a container.
  [secondary: Control.com; primary: Beijer news] https://control.com/news/beijer-electronics-to-launch-x3-series-hmis-with-new-os3-open-platform/ and https://beijerelectronics.us/en-US/news---events/News/2025/X3___pro______marine___extreme___iX___versions
- OS3 updates 1.3 and 1.4 shipped in 2026, with DNV compliance. [primary] https://beijerelectronics.us/en-US/news---events/News/2026/X3___OS3___1,-d-,4
- OS3 is Linux-based (container runtime). [inference]

### 2.11 Red Lion / HMS (FlexEdge DA50A/DA70A, Graphite; Crimson 3.2)

- "Current versions of Crimson 3.2 are using an updated version of the Linux OS, which we call Alpine." Release 3.2.1019.0 says "Updated Linux Distro to Alpine; Updated Linux Kernel to 4.14.323". Some units need an interim update first or "the Linux operating system upgrade will fail… and render the FlexEdge inoperable". [primary: HMS/Red Lion KB] https://www.hms-networks.com/support/tech-support/kb-articles/27220863641618-FlexEdge-Controller-Quick-Start-Guide--Rev--2- and https://redlion.net/node/39558
- "300+ Drivers", OPC UA, SQL Sync, Crimson Control (IEC 61131). "Convert up to 20 protocols simultaneously". [primary] https://www.redlion.net/node/39566
- The processor is ARM with 1 GB RAM. [secondary: Cumulocity listing] https://ecosystem.cumulocity.com/product/flexedge-da50a/
- Older Red Lion G3/Graphite hardware ran Red Lion's own executive on non-Linux firmware; FlexEdge marks the move to Linux. [inference, unverified]

### 2.12 Maple Systems

- Maple has been the #1 Weintek authority in North America for nearly 30 years. Its cMT and HMI5000 lines use EBPro (EZwarePlus). Built-in CODESYS is Weintek's runtime. [primary: Maple] https://maplesystems.com/support-center/controller-connection-guides/maple-systems-built-in-codesys-ebpro/
- EasyAccess 2.0 has a Linux client. [primary] https://maplesystems.com/support-center/software-downloads-upgrades/
- Maple hardware shares the Weintek Linux runtime. [inference]

### 2.13 AutomationDirect C-more

- The C-more EA9 programming software is a free download. Remote Access is included, with a free browser-based "Remote Access & Control" for PCs. [primary] https://automationdirect.com/c-more/ea9/features/remote-hmi-and-remote-access and https://cdn.automationdirect.com/static/specs/ea9pgming.pdf
- The OS and CPU were not found in official sources. A third-party blog says RTOS and TI ARM9. [secondary, low reliability]

### 2.14 Unitronics UniStream

- "Since UniStream is run by Linux, sockets are virtual; many dozens of virtual sockets are available." [primary FAQ] https://unitronicsplc.com/support-faq-unilogic/
- The VNC server is enabled in UniLogic. Clients connect to the panel IP on port 5900, with up to 8 connections. [primary] https://www.unitronicsplc.com/Download/SoftwareHelp/!UniLogicWebhelp2024/Communications/Remotely_Accessing_UniStream_via_VNC.htm
- A web server serves pages designed in UniLogic (model dependent). [primary] https://www.unitronicsplc.com/Download/SoftwareHelp/!UniLogicWebhelp2024/Communications/Web_Server.htm
- UniLogic, including updates, is free. [primary/secondary] https://www.unitronicsplc.com/software-unilogic-for-programmable-controllers-2

### 2.15 Exor (eX700, X platform; JMobile)

- eX707: "32-bit RISC dual core - 800 MHz", OS "Linux RT", 1 GB RAM, 4 GB flash, "industry standard Web engine powering state of the art Chromium browser", CODESYS V3, Corvina support. [primary] https://www.exorint.com/en/product/ex707
- An older datasheet names the CPU as "ARM Cortex-A9 dual core 800 MHz". [primary] https://scigate.com.sg/images/stories/scigate/products/EXOR/ptn0590-1%20eX707.pdf
- JMobile runtimes are built per Linux BSP family (UN60, UN65, UN70); 5.1.865 is dated 07/07/2026. [primary] https://www.exorint.com/support/jmobile-sw-downloads
- UN6x is likely TI AM335x / NXP i.MX6 class. [inference, unverified]
- JMobile PC Runtime (JM-RUNTIME, one activation) runs on Win32/Win64 for £456.26 ex-VAT. [secondary: Scatts UK] https://www.scatts.co.uk/exor-jm-runtime-jmobile-pc-runtime-1-activation-software-license-swlj00r000000-104soft00u-0022-01-21479200510054
- JMobile Studio development licences are paid; a 30-seat licence is listed at US$2,500. [secondary] https://automationdistribution.com/exor-jm-studio-30/
- Exor charges for development seats, unlike Weintek or Delta. [inference]

### 2.16 B&R (ABB) — Power Panel C/T, Automation Runtime, mapp View, Terminal OS

- Power Panel C70 needs Automation Runtime K4.08+. [primary] https://www.br-automation.com/en/products/4ppc70101g-20w/
- C80: "B&R's Automation Runtime real-time operating system runs parallel to an embedded Linux operating system for the HMI application… made possible by the hypervisor from B&R". [primary] https://cws.br-automation.com/en/products/hmi/innovations/power-panel-c80/
- The T-series has an embedded browser and can act as a VC client. [primary] https://www.br-automation.com/pl/produkty/hmi/power-panel-t-series-and-c-series/
- "Terminal OS" is an embedded OS for panel PCs that serves web-based HMI (mapp View), with VNC. It offers the T-series feature set on Intel Core-i hardware. [primary] https://www.br-automation.com/en/products/software/operating-systems/
- Linux for B&R is "based on the respective Debian version". [primary] same URL.
- mapp View is HTML5/CSS/JS generated without hand coding. [primary]
- Power Panel C650 is the successor, available from Q2/2026. [primary] https://www.br-automation.com/ja/products/hmi/power-panel-t-series-and-c-series/

### 2.17 CODESYS Visualization (TargetVisu, WebVisu)

- TargetVisu for Linux SL: €0.00. "No license is required", because licensing is done by application-based licences on the PLC side. It is a Debian package for x86_64, ARM and ARM64, needs glibc ≥ 2.36 (Debian 12) and Qt ≥ 6.2.4 including Qt WebEngine. [primary: CODESYS Store] https://store.codesys.com/en/codesys-targetvisu-for-linux.html
- The older hardware-bound TargetVisu SL was listed at US$264. [secondary: store search snippet]
- WebVisu SL renders the same visualisation in an HTML5 browser. [primary] https://store.codesys.com/media/n98_media_assets/files/2304000004-F/4/CODESYS%20WebVisu%20SL_en.pdf
- Weintek, Exor, Beijer (X2 control) and others embed CODESYS beside their own HMI runtime. [secondary]

### 2.18 Inductive Automation Ignition (Edge Panel, Perspective)

- Edge Panel = Edge IIoT plus local visualisation. Choose Perspective (2 sessions) or Vision (1 local plus 1 remote client). [primary] https://docs.inductiveautomation.com/docs/8.3/other-editions/ignition-edge
- Runs on Linux, Windows and macOS, and is ARM-compatible. [primary] same URL.
- Perspective Workstation is not supported on Linux/ARM; on a Raspberry Pi you use a browser kiosk. [secondary: IA forum] https://forum.inductiveautomation.com/t/running-ignition-edge-on-rpi-5-local-vision-client-fails/102895
- Price: Edge Panel about US$1,500–1,950 per device. [secondary/aggregator] https://inductiveautomation.com/pricing
- Prices changed with Ignition 8.3 on 16 Sept 2025. [primary] https://page.inductiveautomation.com/pricing-update-2025

### 2.19 Crank Storyboard

- Supports "real-time platforms like QNX, Zephyr, and FreeRTOS to open-source options like Linux and Android", plus Windows and iOS. [primary] https://www.cranksoftware.com/solutions/operating-systems-compatibility
- Render back ends: OpenGL ES 1.x/2.x, framebuffer/FBdev, DRM, X11, VGLite, DAVE2D, NXP G2D. [primary datasheet] https://www.cranksoftware.com/hubfs/Webpage%20Downloads/Storyboard%20Product%20Datasheet_EN.pdf
- Yocto images often use fbdev with GLES2; BSPs built only for X11 or Wayland may lack fbdev. [primary KB] https://support.cranksoftware.com/hc/en-us/articles/360045250652-Getting-Storyboard-Running-without-X11
- Storyboard Engine is integrated into Digi and Toradex Yocto/Torizon images and can launch at boot. [primary: Digi] https://digi.com/resources/documentation/digidocs/embedded/dey/4.0/cc8x/yocto-crank-create-image_t.html
- Architecture: the engine is a fixed runtime that loads a Storyboard model file plus Lua. This is the same "fixed runtime plus data" pattern as industrial HMIs, but sold as a toolkit. [inference]
- The current support-site footer reads "© 2026 Crank AMETEK", so Crank is now owned by AMETEK. [secondary, inference]
- A developer licence costs about US$4k a year. [secondary, unverified slide]

### 2.20 Qt for MCUs vs Qt on Linux

- Qt for MCUs (Qt Quick Ultralite) compiles QML to C++ and runs on bare metal, RTOS, Zephyr or Linux. Example targets: STM32N6570-DK (bare metal), i.MX RT595 (FreeRTOS), NXP FRDM-RW612 (Zephyr), i.MX 93 EVK (Linux). [primary] https://doc.qt.io/QtForMCUs/qtul-supported-platforms.html
- It is commercial only (Qt for Device Creation Professional/Enterprise). [primary] https://doc.qt.io/QtForMCUs/qtul-licensing.html
- Qt on embedded Linux (Boot2Qt / Device Creation) runs Qt Quick on EGLFS/KMS or Wayland as an interpreted QML application.
- Commercially, "Any Device built with or incorporating Qt is subject to a distribution license fee". The per-device price is not public. [primary] https://www.qt.io/faq/qt-commercial-licensing
- LGPL use on Linux is possible if obligations are met. [secondary: Qt mailing list] https://lists.qt-project.org/pipermail/interest/2023-December/039450.html
- A Qt-commissioned 2026 Spyrosoft comparison concedes that LVGL wins on runtime licensing. [secondary, vendor-commissioned] https://www.qt.io/hubfs/Spyrosoft-Qt-for-MCUs-vs-LVGL-Comparison-2026-Update.pdf
- Qt for MCUs is architecturally identical to EHS: generated code compiled into firmware. Qt on Linux is the NOVAKON/Weintek-class model, with a runtime loading assets. [inference]

### 2.21 LVGL on Linux and LVGL Pro XML runtime loading

- The Linux display drivers are fbdev (`LV_USE_LINUX_FBDEV`, default /dev/fb0), DRM/KMS (`LV_USE_LINUX_DRM`, default /dev/dri/card0, needs libdrm, optional GBM, and automatic EGL when OpenGL ES is enabled), Wayland (`LV_USE_WAYLAND`) and SDL. Input comes from evdev or libinput. [primary] https://docs.lvgl.io/9.5/integration/embedded_linux/drivers/drm.html and https://docs.lvgl.io/9.5/integration/pc/linux.html
- The reference project is lv_port_linux. [primary] https://github.com/lvgl/lv_port_linux
- LVGL Pro says: "LVGL supports loading XML files at runtime without recompiling your application". Delivery can be "via Bluetooth, WiFi, Serial port, SD card, or downloaded from a server". [primary] https://lvgl.io/docs/pro/integration/loading-xml-files.mdx
- However, the "engine that can load XML files at runtime is not publicly available"; contact LVGL Pro. [primary] same URL.
- Limits:
  - Widgets are compiled C and cannot be loaded from XML.
  - Events, subjects, images, fonts and similar resources must be registered from C.
  - Components with custom C logic cannot be loaded at runtime.
  [primary] same URL and https://lvgl.io/docs/open/xml/intro.mdx
- The MIT LVGL XML module exposes `lv_xml_component_register_from_file()`, `lv_xml_create()` and `lv_xml_load_all_from_path()` / `lv_xml_load_all_from_data()` (FrogFS packaging). How much of runtime loading is usable in the open-source build varies by version and needs checking against the release used. [primary/secondary]
- For EHS this is the obvious route to a "fixed firmware plus downloadable project" model on STM32 or Linux, but its availability depends on LVGL's commercial terms. [inference]

### 2.22 Context: Rockwell (not requested, useful for trends)

- PanelView Plus 7 Series B was created because "the preceding Series A graphic terminal ran on Windows CE 6.0 operating system, which is no longer fully supported by Microsoft". [primary: Rockwell press release, 2022] https://www.rockwellautomation.com/en-ie/company/news/press-releases/New-PanelView-Plus-7-Performance-Series-B-Protects-Customer-Investments.html
- The PanelView 5000 runs embedded Linux. [secondary: Rexel blog 2019] https://pages.rexelusa.com/blog/automation/windows-embedded-ce-6-0
- FactoryTalk Optix (Oct 2023) is browser-based and cloud-native. [secondary] https://emergenresearch.com/industry-report/human-machine-interface-market

---

## 3. Market trends 2020–2026

### 3.1 Windows CE / Embedded Compact end of support

- WEC 2013 (CE 8.0) mainstream support ended 9 Oct 2023 and extended support ended 10 Oct 2023. Microsoft has no plan for extended support beyond 2023. Devices may keep running indefinitely. OEM licence sales continue until 2028. [secondary citing Microsoft lifecycle; primary FAQ] https://learn.microsoft.com/windows/iot/windows_ce/windows-ce-migration-faq and https://www.theregister.com/software/2023/10/30/windows-ce-reaches-end-of-life-if-not-end-of-sales/1358506
- CE 6.0 extended support ended April 2018, and its end of licence was 22 Feb 2022. That is why Rockwell built PanelView Plus 7 Series B. [secondary] https://pages.rexelusa.com/blog/automation/windows-embedded-ce-6-0
- WEC7's extended support ended April 2021. [inference, unverified this session]
- Microsoft's suggested paths are the CE App Container on Windows 10 IoT Core (supported to 2029) or Windows IoT Enterprise. Industry consultants mostly recommend embedded Linux with Qt. [primary/secondary] https://www.scalosoft.com/blog/beyond-windows-ce-2013-transition-strategies-for-end-of-life/
- Observed vendor responses:
  - Siemens: Comfort (CE) → Unified Comfort (Linux-based "proprietary OS", web runtime), 2019–2020.
  - Rockwell: PV Plus 7 Series A (CE6) → Series B; PanelView 5000 on Linux.
  - MCGS: 嵌入版 on WinCE → McgsPro on Linux.
  - Beijer: X2 (WEC2013) → X3 / OS3 (containerised), 2024–2025.
  - Pro-face: Windows only for Open Box/IPC lines.
  - Already Linux before 2020: Weintek, Delta, Exor, Unitronics, Red Lion FlexEdge.
  [inference from sources above]

### 3.2 HTML5 / web runtimes

- ARC lists "Rising Linux Adoption Driven by Containerization Trends" among HMI software trends and notes that operating-system-independent HTML5 works on nearly every web-enabled device. [secondary: ARC press] https://arcweb.com/press/hmi-software-market-driven-demand-innovations-sustainability
- Meticulous Research expects the web-based HMI segment to show the highest CAGR. [secondary] https://meticulousresearch.com/reports/human-machine-interface-hmi-market-5805
- Product evidence:
  - Siemens WinCC Unified: HTML5/SVG/JS runtime.
  - B&R mapp View.
  - Beijer WebIQ and the X3 pro web.
  - Weintek cMT X Web Panels (Chrome-based, 2025) and WebView/EasyWeb 2.0.
  - Exor (Chromium engine).
  - CODESYS WebVisu.
  - Ignition Perspective.
  - Rockwell FactoryTalk Optix.
  - Mitsubishi GOT Mobile.
- Two patterns coexist [inference]:
  - **(a) Native runtime on the panel plus a web client for remote use.** Used by Weintek, Unitronics, C-more and Delta. This keeps low-end ARM panels fast.
  - **(b) The panel itself is a browser over a web runtime.** Used by Siemens Unified, B&R T-series, Weintek Web Panels and Beijer X3 web. It needs Cortex-A53/A72-class or x86 performance and 1–4 GB RAM.

### 3.3 x86 vs ARM share in HMIs

- No public share figure was found; paid reports (MarketsandMarkets, ARC, IHS) hold it.
- Observed product mix: the operator-panel segment is predominantly ARM Cortex-A, while x86 dominates Panel PCs/IPCs and "open" HMIs running Windows (SP5000 Open Box, Harmony iPC, Siemens IPC with WinCC Unified PC, B&R Panel PCs with Terminal OS). [inference]
- One driver of this mix is the Windows licence cost below; another is power and fanless thermal design. [inference]

### 3.4 Windows IoT Enterprise LTSC licence cost per device

- Editions are tied to CPU class: High End (Xeon, Core i7/i9), Value (Core i3/i5, Ryzen 5), Entry (Celeron, Atom), and Base (Arm, e.g. i.MX 8/93). "No changes in the pricing model for the 2024 LTSC release". [secondary: distributor FAQs] https://www.arrow.com/ais/msembedded/resources/windows-iot-enterprise-ltsc-frequently-asked-questions-faqs and https://my.avnet.com/msembedded/embedded-software/windows-11-iot-enterprise-2024/
- Reseller prices seen in 2025–2026 (one seller, one date each, single licence):

  | Edition | Price | Source |
  |---|---|---|
  | Windows 11 IoT Enterprise LTSC 2024 Entry | CAD 87.99 | cdw.ca https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-entry-license-1-license/7983139 |
  | Windows 11 IoT Enterprise LTSC 2024 Value | CAD 131.99 | cdw.ca https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-value-license-1-license/7983137 |
  | Windows 11 IoT Enterprise LTSC 2024 High End | CAD 237.99 (listing discontinued Apr 2025) | cdw.ca https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-high-end-license-1-license/7983138 |
  | Windows 10 IoT Enterprise 2021 LTSC Entry | US$53.99–66.99 | cdw.com https://www.cdw.com/product/windows-10-iot-enterprise-2021-ltsc-entry-license-1-license/8248540 |
  | Windows 10 IoT Enterprise 2021 LTSC Value | US$109.72 (Advantech via CDW) to CAD 361.99 | cdw.com / cdw.ca |
  | Windows 11 IoT Enterprise LTSC 2024 Value upgrade (Dell thin client) | US$109.99 | Dell https://www.dell.com/en-us/shop/p/APD/634-CZBD/ |

- The licence is sold through authorised embedded distributors under OEM/dedicated-use terms; volume pricing is not public.
- Rule of thumb: about US$50–90 per Atom/Celeron panel and US$100–140 per Core i3/i5 panel. That is significant against a US$65–125 ARM HMI BOM (see note 17). [inference]

### 3.5 Other 2020–2026 patterns [inference]

- **Containers and apps on the HMI:** Siemens Industrial Edge on Unified Comfort, Beijer OS3, CODESYS Virtual Control, and Weintek running CODESYS alongside EBPro.
- **Security hardening:** encrypted editor/viewer links (Weintek OS 20210518+), mandatory two passwords for Siemens SmartServer, and EU CRA readiness. A Linux runtime that can be updated in the field is becoming a requirement.
- **Cloud remote access as a paid add-on:** Weintek EasyAccess 2.0, Exor Corvina, Pro-face Connect, Mitsubishi GOT Mobile licence. Free LAN VNC/web is table stakes.

---

## 4. Direct implications for EHS [inference]

1. **Customers compare against Weintek/Delta/Kinco/MCGS.** For them, the "runtime" is a fixed program on a Linux Cortex-A7/A8/A17 panel, and the editor downloads a project. Recompiling firmware per project is foreign to them, unless it is hidden behind a one-click "download" that builds and flashes in the background.
2. **Option A: an EHS Linux runtime.** LVGL with DRM/fbdev on a Cortex-A7/A53 SoC, loading an EHS project, with drivers (Modbus first), VNC/web and a PC simulator (LVGL with SDL). This matches the NOVAKON/Weintek architecture, and LVGL's open-source Linux drivers make it feasible.
3. **Option B: "fixed MCU firmware plus project blob" on STM32.** This needs a runtime loader: LVGL XML loading (Pro engine not public; open-source subset to verify) or EHS's own binary format. It would be unique in the low-cost MCU space, but driver and feature parity are harder.
4. **Licensing:** panel makers give the runtime away free with their own hardware, and platform vendors charge for runtimes on third-party PCs and HMIs. A free EHS editor plus a runtime that is free on EDT displays and licensed elsewhere is a conventional model.
