# Verification: MCU stack footprints, CRA classification, Taiwan hiring, stack support windows

Notes date: 2026-11-20. Scope: fact-check of open non-chip items in `ehs_rtos_fit.md` and `highend_mcu_path.md` (this folder). Method: web searches (standard mode, one extended) and page fetches. No browser tools. 104, 1111, Cake and LinkedIn refused automated fetches (403, bot check, 429), so job-count data could not be read directly.

Labels: **[P]** primary (vendor, regulator, project's own page or repo, official journal); **[S]** secondary (law firm, independent guide, forum, trade press, aggregator); **[I]** my inference.

Result codes: **confirmed**, **corrected**, **new**, **still unverified**.

---

## Summary

| # | Item | Result | One line |
| --- | --- | --- | --- |
| 1a | mbedTLS TLS session RAM/flash, handshake time on Cortex-M7 | still unverified (for M7); new vendor figures | No vendor publishes a Cortex-M7 TLS 1.3 heap peak. Best primary figures: Espressif 42.2 KB heap per HTTPS client (22.0 KB with dynamic buffers); TI 176 KB mbedTLS code+data on AM2x. The 40–80 KB planning band in the notes stands. |
| 1b | lwIP + FreeRTOS footprint | confirmed (order of magnitude) | lwIP README: "tens of kilobytes of free RAM", "around 40 kilobytes of code ROM"; FreeRTOS kernel 5–10 KB ROM. No M7-specific vendor table read. |
| 1c | coreMQTT / MQTT client | **corrected** | coreMQTT 5.x (MQTT 5, the version in 202604 LTS) is **19.1 KB (-Os) to 23.3 KB (-O1)** of code, not the ~7 KB in the old MQTT 3.1.1 table. No heap use. |
| 1d | MCUboot | still unverified (vendor M7 figure); new forum data | NCS default partition 48 KB, ~43 KB used; ST SBSFU (MCUboot-based) ~80 KB. |
| 1e | open62541 nano/micro | confirmed docs claim; **corrected** FreeRTOS status | Docs: "less than 100kB of RAM and ROM" for minimal servers. **v1.5.0 (2026-02-02) restored FreeRTOS support via the lwIP EventLoop**; the notes' "stay on 1.3.x" advice is outdated. No measured M7 figure found. |
| 2 | CRA class of an HMI panel | **new / resolved** | Final Annex III has **no IACS, PLC or HMI category** (the 2022 proposal had one; it was dropped). An HMI panel's core functionality is not listed, so it is **default category** (self-assessment). Integrated OS, boot manager, network interface or MCU do not change that (Art. 7(1)). No CRA harmonised standard is cited in the OJ yet. |
| 3 | Taiwan hiring counts | still unverified (counts); new salary data | No keyword counts obtainable. 104's 2026 tech report: firmware engineer median annual pay **NT$1,190,367** (rank 3 of non-managerial roles). Individual postings show FreeRTOS, Yocto, STM32 demand; Zephyr postings in Taiwan are rare. |
| 4a | FreeRTOS 202604 LTS window | confirmed, sharpened | **LTS until 04/30/2028** for all libraries; kernel 11.3.0, FreeRTOS-Plus-TCP 4.4.1, coreMQTT 5.0.2. |
| 4b | lwIP release status | confirmed: no new release | Last release still **2.2.1 (tag 2025-02-05)**. No 2.2.2 or 2.3.0. Heavy security-fix activity in git in November 2026 (TCP, SNMPv3, MQTT app, mDNS). |
| 4c | Zephyr LTS | **corrected** | 3.7 LTS EOL is **2029-07-27** (not January 2027). Next LTS is **4.6 (LTS4), April 2027**. Latest release 4.5.0 (2026-10-28). |

---

## 1. Measured footprints on Cortex-M7-class MCUs

### 1.1 What was searched for and not found

No vendor application note was found that gives, for a Cortex-M7 part (STM32H7, i.MX RT1060/1170), a measured table covering mbedTLS TLS 1.2/1.3 heap peak, handshake time, lwIP + FreeRTOS, coreMQTT/Paho, MCUboot and open62541. ST's UM2178 (X-CUBE-AWS) and AN3966 (lwIP on STM32F4x7) have footprint sections, but st.com timed out on both fetches. Renesas' OPC UA application note (R01AN8281EJ0120, Rev.1.20, 2026-08-28, RZ/T2 and RZ/N2) has a section "8.3 Footprint (ROM and RAM Usage)", but the PDF returned 404 to the fetch tool [P, table of contents only via search]. **Result: still unverified for M7. The project must measure on its own board (open question 1 in `ehs_rtos_fit.md` §7.3 stands).**

### 1.2 mbedTLS (TLS client session)

| Figure | Conditions | Label / source |
| --- | --- | --- |
| Heap **42,196 B** default; 42,120 B with variable buffer length; 38,533 B without keeping the peer certificate; **22,013 B** with dynamic TX/RX buffers and freeing CA/config data | ESP-IDF `protocols/https_request` example, server validation on, Mbed TLS; "subject to change with configuration and versions" | [P] Espressif ESP-IDF mbedTLS docs (Xtensa/RISC-V, not Cortex-M; heap figures are largely CPU-independent [I]) |
| Disabling cross-signed bundle verify cuts handshake peak heap by about 1 KB | same | [P] Espressif |
| mbedTLS library **176,436 B** (code 127,988 + RO 39,289 + RW 9,159); lwIP footprint change "almost negligible" | TI AM2x Sitara MCU (Cortex-R5F), lwIP altcp TLS, default config "which can further be reduced"; SPRADB2, August 2023 | [P] TI SPRADB2 |
| TLS 1.3 code depends on PSA crypto with "a little RAM overhead" (a few hundred bytes) vs legacy API | Mbed TLS maintainer | [P] Mbed TLS mailing list |
| Handshake 31,656 B (RSA endpoint) / 32,264 B (ECDSA endpoint); ~50 KB steady state after MQTT-over-TLS connect, reduced to 34–36 KB | unnamed MCU, older Mbed TLS | [S] Mbed OS forum |
| TLS 1.2 ECDHE-ECDSA AES-256 peaks 15 KB more heap (about 3.5×) than PSK AES-128 | low-power boards, Mbed TLS, 2020 | [S] academic (arXiv 2011.12035, Restuccia/Tschofenig/Baccelli) |
| wolfSSL tinytls13: TLS 1.3 PSK+X25519 client **30.8 KB flash** on Cortex-M33; +~6 KB for P-256; ~61 KB with X.509 verification; ~65 KB mutual TLS; no RAM figures | wolfSSL blog, July 2026 | [P] wolfSSL |
| wolfSSL forum estimate: TLS 1.3-only client ~70 KB flash, ~20 KB RAM | STM32 Cortex-M4, rough estimate by wolfSSL staff/user | [S] |

Handshake time on M7: still unverified. Data points found:
- wolfCrypt on STM32H753 at 480 MHz: ECDHE ~49 ops/s, ECDSA verify ~31 ops/s (already in `highend_mcu_path.md` §2.2) [S].
- STM32U585 + NetX Secure: about 1.5 s (secp256r1), 3 s (secp384r1), 10 s (secp521r1) full handshake, clock not stated [S, ST Community 2024].
- STM32F7 vs STM32MP13: RSA-2048 certificate handshake 1,400 ms on F7 vs 52 ms on MP13; secp384 4,000 ms vs 700 ms [S, ST Community].
- ST staff (2025-02-12): "on an average STM32 MCU, the TLS handshake with AWS IoT Core can take a couple of seconds" [P, vendor forum staff].

Inference: the planning band in the notes (40–80 KB RAM per session, 0.2–1 s handshake on a 480–1000 MHz core with P-256) is **consistent** with these figures; with dynamic buffers and P-256 only, ~22–35 KB heap per session is reachable. Use secp256r1, never secp384r1/secp521r1, on the panel side. Plan about 130–180 KB flash for mbedTLS with X.509 and TLS 1.2+1.3 unless trimmed [I from TI figure].

### 1.3 lwIP + FreeRTOS

| Figure | Label / source |
| --- | --- |
| lwIP suits systems "with tens of kilobytes of free RAM and room for around 40 kilobytes of code ROM" | [P] lwIP README (via Sming copy of the README) |
| FreeRTOS kernel "about 5 to 10 KBytes of ROM"; AWS docs say 4,000–9,000 bytes for a typical kernel image | [P] FreeRTOS FAQ; AWS FreeRTOS kernel guide |
| lwIP ROM 33–46 KB depending on configuration and RTOS (LPC builds) | [S] NXP Community |
| ST forum: ~50 KB figure for lwIP came from an STM32 app note demo; socket API sits on netconn on raw API, socket API needs an RTOS | [S] ST Community |
| Typical CubeMX lwIP heap `MEM_SIZE` ~32 KB in user projects; plus pbuf pools and Ethernet DMA buffers in non-cacheable SRAM | [S] ST Community |

Inference: lwIP sockets + FreeRTOS on an H7 lands around 50–80 KB flash and 50–100 KB RAM (heap + pools + DMA descriptors/buffers + task stacks), driven by `MEM_SIZE`, `PBUF_POOL_SIZE`, `TCP_WND`. Result: **confirmed as order of magnitude**, not measured on M7.

### 1.4 MQTT client

| Figure | Label / source |
| --- | --- |
| **coreMQTT v5 (MQTT 5.0) code size: 23.3 K (-O1) / 19.1 K (-Os)** total: core_mqtt.c 8.4/7.3 K, serializer 9.5/7.4 K, state 1.7/1.3 K, property (de)serialisers 1.6/1.4 K and 1.3/1.0 K, private serializer 0.8/0.7 K; "GCC for ARM Cortex-M" | [P] coreMQTT repo `docs/doxygen/include/size_table.md` |
| Older coreMQTT (MQTT 3.1.1) table still on the AWS user guide: 8.5 K (-O1) / 6.9 K (-Os); AWS page is flagged "may not be up-to-date" | [P] AWS FreeRTOS user guide |
| coreMQTT has "no heap allocation" (CBMC proofs); RAM = caller-supplied network buffer + ack-tracking arrays | [P] AWS user guide |
| coreMQTT 5.0.0 added MQTT 5 "with breaking API changes"; 5.0.2 is in 202604 LTS | [P] coreMQTT repo, FreeRTOS-LTS repo |

Result: **corrected.** The notes say MQTT is "a few KB of code" [`highend_mcu_path.md` §2.3]; with MQTT 5 it is ~19–23 KB. Still small next to TLS. Paho Embedded-C: no figure found (still unverified). lwIP's own MQTT app had a use-after-free fixed in git on 2026-11-08 ("mqtt: fix use-after-free when callback frees client") [P, lwIP git mirror], which is another reason to prefer coreMQTT over the lwIP MQTT app [I].

### 1.5 MCUboot

| Figure | Label / source |
| --- | --- |
| nRF Connect SDK default MCUboot partition 48 KB; ~43 KB used without external flash; +~9 KB when external SPI flash is declared; can be roughly halved by stripping logging etc. | [S] Nordic DevZone (Nordic staff and users) |
| nRF54L default bootloader partition 64 kB | [S] Nordic DevZone |
| Switching MCUboot crypto from mbedTLS to TinyCrypt saved ~30 KB in one build (76+ KB with USB DFU) | [S] Nordic DevZone user |
| ST SBSFU_Boot (MCUboot-based, mbedTLS) ~80 KB | [S] ST Community user |
| Arm TF-M slides (2018): mbedTLS ROM for RSA-3072 ~14 KB; RAM for ECC ~13 KB | [S] Arm/Linaro slides |

Inference: budget **48–64 KB flash** for MCUboot with ECDSA P-256 and swap-using-move or overwrite, ~80 KB for vendor-wrapped variants (OEMiRoT/SBSFU); RAM is only used at boot. Result: still unverified on M7 from a vendor table.

### 1.6 open62541 (nano/micro)

| Fact | Label / source |
| --- | --- |
| "it is possible to configure minimal servers that require less than 100kB of RAM and ROM"; `UA_NAMESPACE_ZERO=MINIMAL` can halve binary size but "does not pass the CTT"; RAM driven by information-model size, client count and preallocated max message size | [P] open62541 docs, building page |
| **v1.5.0 released 2026-02-02** with "arch: Support for FreeRTOS (using the lwip EventLoop)", "eventloop: Support for lwip-based EventLoop", "arch: Support for Zephyr" | [P] open62541 announcement list |
| v1.5.8 maintenance release 2026-09-07 (lwIP netif lookup fix; hardening of PubSub encoding, XML decoder size checks, a use-after-free fix) | [P] open62541 announcement list |
| Maintainer (late 2025): lwIP POSIX layer requires RTOS threading | [S] open62541 mailing list |

Result: **corrected.** `highend_mcu_path.md` §2.3 says FreeRTOS support was removed in 1.4, "so stay on 1.3.x". That was true for 1.4; since 1.5.0 FreeRTOS + lwIP is supported again. Footprint on M7: still unverified (the Renesas RZ/T2/N2 note has a table, not read).

### 1.7 Aggregate planning figure (Inference)

| Block | Flash | RAM |
| --- | --- | --- |
| FreeRTOS kernel | 5–10 KB | per task TCB + stacks |
| lwIP sockets + Ethernet driver | 50–80 KB | 50–100 KB |
| mbedTLS (X.509, TLS 1.2/1.3, P-256, AES-GCM) | 130–180 KB | 22–45 KB per session |
| coreMQTT 5 | 19–23 KB | a few KB of buffers |
| MCUboot (separate slot) | 48–64 KB | boot-time only |
| open62541 minimal (if ever) | ~100–300 KB | ~100–256 KB |

On the H747 (2 MB flash, ~1 MB SRAM, EHS already at 74 % of bank 1 and 55 % of AXI SRAM), networking without OPC UA is about 0.25–0.35 MB flash and 0.1–0.2 MB RAM [I]. This supports the notes' conclusion that it will not fit beside today's image in bank 1.

---

## 2. EU Cyber Resilience Act classification of an industrial HMI panel

### 2.1 Annex III and IV of Regulation (EU) 2024/2847 (final text)

Annex III **Class I** (19 entries) [P, OJ text via SpringLex mirror; cross-checked with cyberresilienceact.eu]: identity management/PAM systems; standalone and embedded browsers; password managers; anti-malware; VPN products; **network management systems**; SIEM; **boot managers**; PKI and certificate software; **physical and virtual network interfaces**; **operating systems**; **routers, modems intended for connection to the internet, and switches**; **microprocessors with security-related functionalities**; **microcontrollers with security-related functionalities**; ASICs and FPGAs with security-related functionalities; smart home virtual assistants; smart home security products; connected toys; personal wearable health monitors and children's wearables.

Annex III **Class II** (4 entries): hypervisors and container runtimes; firewalls and IDS/IPS; **tamper-resistant microprocessors**; **tamper-resistant microcontrollers**.

Annex IV **critical** (3 entries): hardware devices with security boxes; smart meter gateways and other devices for advanced security purposes including secure cryptoprocessing; smartcards and similar devices including secure elements.

**No entry names industrial automation and control systems, PLCs, SCADA or HMIs.** The Commission's 2022 proposal had "Industrial Automation & Control Systems (IACS) not covered by class II, such as programmable logic controllers (PLC), distributed control systems (DCS), computerized numeric controllers for machine tools (CNC) and supervisory control and data acquisition systems (SCADA)" in Class I, and IACS for NIS2 essential entities in Class II [P, proposal text via european-cyber-resilience-act.com mirror; S, Council documents]. These entries are absent from the adopted text [P]. Secondary sources that still call IACS "critical" products (e.g. an OT-security learning note) are wrong [S, flagged].

### 2.2 Implementing Regulation (EU) 2025/2392

- Adopted 28 November 2025, published OJ 1 December 2025, in force 21 December 2025 [S, TÜV and HSF Kramer, consistent]. It describes the categories; it does not amend the CRA [S, HSF Kramer].
- Technical descriptions relevant to an RTOS/Linux HMI [P, Annex I text via SpringLex mirror]:
  - **Operating systems**: products that "provide an abstract interface of the underlying hardware and control the execution of software"; examples include "real-time operating systems, general-purpose and special-purpose operating systems". So FreeRTOS, Zephyr and embedded Linux distributions, **when placed on the market as products in their own right**, are Class I.
  - **Boot managers**: "manage the process of initial system startup after power on/restart"; examples "UEFI firmware, single-stage and multi-stage boot loaders" (covers MCUboot, U-Boot as stand-alone products).
  - **Physical network interfaces**: devices that "directly connect a device to a network", e.g. "Wi-Fi, Ethernet, IrDA, USB, Bluetooth, NearLink, Zigbee, or Fieldbus" adapters.
  - **Microcontrollers / microprocessors with security-related functionalities**: CPU products "including microcode and other low-level firmware"; Class II only if tamper-resistant.
  - Network management systems: manage "servers, routers, switches, workstations, printers or mobile devices".
  - Examples are illustrative and non-exhaustive (recital 7) [P].
- No mention of industrial, PLC or HMI in these descriptions [P].

### 2.3 Core functionality rule

- **Article 7(1) CRA**: integrating a product whose core functionality is in Annex III "shall not in itself render the product" subject to the important-product conformity procedures [P, quoted by OSADL].
- Recital 4 of 2025/2392: carrying another category's functionality "does not in itself mean" the product loses its own core functionality [P via OSADL].
- OSADL worked example (FAQ updated June 2026, page modified 2026-10-28): a machine-control product bundling a container runtime is classified by "controlling machines", not as a container runtime [S].
- Commission CRA guidance, Communication **C(2026) 6013**, published 2026-07-27 (non-binding): covers scope, remote data processing solutions, FOSS, support periods, substantial modification, reporting, 67 examples [P, Commission page]. Law-firm summaries quote a smartphone example: a phone integrating an OS or password manager is "generally not" in those categories because that is not its core functionality [S]. One search summary gave the reference as C(2026) 5252; the Commission page says C(2026) 6013 [P wins].

### 2.4 Answer for an EHS-based HMI operator panel

| Product | Class | Conformity route | Label |
| --- | --- | --- | --- |
| HMI operator panel (screen + runtime + PLC drivers), any of the three platforms | **Default** (not listed; core functionality = operator visualisation/control) | Module A self-assessment (internal control), CE marking, Annex I essential requirements, SBOM, vulnerability handling, ≥5-year support period | [P] Annex III/IV text + Art. 7(1); [I] application to HMI |
| Same panel with an integrated VPN client for remote access, sold as an HMI | Default, provided VPN is ancillary. If marketed primarily as a remote-access/VPN gateway, Class I | as above / Class I route | [I] |
| EHS runtime sold separately as software for customer panel PCs | Default (application software; not an OS) | Module A | [I] |
| An RTOS, embedded Linux distro, or boot loader sold by EDT as a stand-alone product | Class I ("operating systems", "boot managers") | Module A only with a cited harmonised standard/common specification/certification; otherwise notified body (Module B+C or H) | [P] Annex III; [S] Art. 32 summary |
| MCU or SoC with security functions (the vendor's product, e.g. STM32H7S, RT1170) | Class I (Class II if tamper-resistant); the chip vendor's obligation, not EDT's | — | [P] |

Caveat (Inference): this is a reading of the regulation, not legal advice; confirm with counsel or a notified body, especially if an HMI variant is marketed as a remote-access gateway.

### 2.5 Harmonised standards status (as of 2026-11-20)

| Item | Status | Label / source |
| --- | --- | --- |
| Standardisation request M/606, C(2025) 618 of 2025-02-03 | Original deadlines: Type A horizontal + Type B vulnerability handling 2026-08-30; Type C product standards 2026-10-30 | [S] cyberresilienceact.eu (2026-07-30, updated 2026-09-11) |
| Amendment **C(2026) 7861 final, 2026-11-10** | EN 40000-1-2 and -1-3 now due **2026-11-30**; Type C product standards **2027-01-31**; EN 40000-1-4 unchanged at **2027-10-30**. Lapses if CEN/CENELEC/ETSI object within one month (to about 2026-12-10) | [S] cyberresilienceact.eu (2026-11-10), citing the decision, IBF (2026-11-16) and CRAevidence (2026-11-17); decision text itself not read |
| **EN 40000-1-1** (vocabulary), **EN 40000-1-2** (principles, risk management, lifecycle) | Vote closed 2026-10-01; ratified 2026-11-02; published as ENs 2026-11-18; CEN records: "no citation expected" in the OJ | [S] cyberresilienceact.eu, IBF |
| **prEN 40000-1-3** (vulnerability handling) | Draft; the horizontal part expected to be cited first | [S] |
| prEN 40000-1-4 (generic security requirements) | Drafting; due 2027-10-30 | [S] |
| Vertical (Type C): **prEN 50770-1…-6** (CENELEC, "Security for Operational Technologies") | Drafts for firewalls/IDS/IPS, network management, physical/virtual interfaces, VPN, routers/switches, SIEM; based on IEC 62443 | [S] IBF |
| ETSI EN 304 xxx product standards | 17 final drafts in public enquiry since 2026-08-13 (browsers, password managers, **operating systems**, firewalls, routers, VPN, PKI, anti-virus, SIEM …) | [S] IBF, cyberresilienceact.eu |
| IEC 62443-4-1 / 4-2 | Amendments in work; "unlikely" to be OJ-cited; 4-2 explicitly covers HMIs, PLCs, embedded devices as IACS components; expected to stay the technical basis in OT | [S] IBF |
| **OJ citations** | **None** for any CRA product category | [S] cyberresilienceact.eu (2026-11-10) |

Consequences (Inference):
- For a **default-category** HMI the missing standards do not block CE marking (Module A does not need a cited standard), but there is no presumption of conformity: the technical file must argue each Annex I requirement directly. Using IEC 62443-4-1 (process) and 62443-4-2 (component, which names HMIs) plus prEN 40000-1-3 is the practical route.
- No vertical CRA standard is planned for HMIs, PLCs or IACS as such; prEN 50770 covers only the Annex III OT categories.
- If EDT ever sells its RTOS/Linux platform or boot loader as a product, it is Class I and, without a cited standard by 2027-12-11, needs a notified body.

---

## 3. Taiwan hiring indicators

### 3.1 Job counts

- 104人力銀行 (`104.com.tw/jobs/search/?keyword=…`) returned HTTP 403; 1111 returned a failed security-verification page; Cake (cake.me) 403; LinkedIn 429 after a redirect to `tw.linkedin.com/jobs/<keyword>-職位`. Searches did not surface any indexed count pages. **Result: still unverified. No keyword counts for FreeRTOS, Zephyr, 嵌入式 Linux, Yocto or STM32 were obtained.** A manual count in a normal browser on one date is the remaining step.
- embedded.jobs "embedded systems engineer jobs in Taiwan": **4 jobs** total (HPE ×2, pSemi, Lenovo), none naming FreeRTOS/Zephyr/Yocto/STM32 [S, fetched 2026-11-20; aggregator, not representative].
- jobswithgpt: 105 embedded-software jobs in Taipei, last updated 2025-11-13 [S, stale].

### 3.2 Posting-level evidence (qualitative) [S, job boards and aggregators, retrieved 2026-11-20; dates where shown]

| Skill | Examples found |
| --- | --- |
| FreeRTOS | Axiado, Staff MCU/RTOS Engineer, Taipei (8+ years MCU FreeRTOS); Ubiquiti, Embedded Firmware Engineer, Taipei (RTOS such as FreeRTOS required; posting expired); 104 automotive postings listing FreeRTOS/OSEK/AUTOSAR OS; 104 "BMS資深韌體工程師", Taipei Nangang, 2026-05-14, RTOS + ESP32/STM32, salary ≥ NT$40,000/month |
| Zephyr | Ambiq, Staff Applications Software Engineer (Zephyr), Hsinchu/Singapore (Greenhouse); Augentix (Hsinchu) and Framework (Taipei) list Zephyr as secondary/plus. No 104 posting found that requires Zephyr |
| Embedded Linux / Yocto | 104: 5G Embedded Linux senior engineer (Yocto build and recipes preferred); FIH (Yocto BSP); several BSP/kernel/U-Boot roles; Advantech (Embedded Linux/MCU OOB firmware, Neihu); MediaTek (Embedded Linux SW, Hsinchu); Canonical (Taipei office, Yocto mentioned) |
| STM32 / Cortex-M | 104 firmware postings ask for ARM Cortex-M or 8051, I2C/SPI/UART; STM32 named in the BMS posting |
| Salary snippets | 104 FW engineer, Xizhi: NT$45,000–50,000/month; 104 junior embedded Linux, Sanchong: NT$35,000–80,000/month; many postings "面議" |

### 3.3 Salary and market surveys

| Fact | Label / source |
| --- | --- |
| **Firmware engineer (韌體工程師) annual median NT$1,190,367**, rank 3 of top-10 high-paying non-managerial roles | [P] 104 blog, 《2026科技業人才報告書》, published 2026-04-28 |
| Semiconductor hiring about 47,000 openings per month in 2026, +20 % YoY | [P] 104 blog (2026 semiconductor talent report) |
| 104 total openings 1.23 million in July 2026; electronics/IT/software/semiconductor about 194,000 | [P] 104 blog via search summary |
| Software/engineering openings in January 2026 −5 % YoY while semiconductor openings grew | [S] press summary of 104 data |
| 1111: tech industry monthly talent gap 161,000 in 2026 (+12.6 %); hardest to hire: equipment engineers 20.7 %, software engineers 16.6 %, hardware engineers 11.3 % | [S] press summary of 1111 survey |
| No survey breaks out FreeRTOS, Zephyr, Yocto or STM32 as skills | — |

Inference: the notes' statement (large pool for STM32/FreeRTOS firmware and BSP/Linux; Zephyr experience rare) is **consistent with posting-level evidence** but remains unquantified. Firmware engineers are expensive in Taiwan (median about NT$1.19 M/year) because they compete with semiconductor employers.

---

## 4. Stack support windows

### 4.1 FreeRTOS 202604 LTS — confirmed, sharpened

- AWS announcement posted **2026-05-01**: "feature stability, security updates, and critical bug fixes for two years"; kernel v11.3.0, coreMQTT v5.0.2 (MQTT 5.0), coreSNTP v2.0.0 (year-2038 readiness); EMP for projects needing fixes beyond expiry [P].
- FreeRTOS-LTS repo: **"LTS Until" 04/30/2028** for every library. Versions: Kernel 11.3.0, FreeRTOS-Plus-TCP 4.4.1, coreMQTT 5.0.2, coreHTTP 3.1.3, corePKCS11 3.6.4, coreJSON 3.3.1, coreSNTP 2.0.0, backoffAlgorithm 1.4.2, Cellular Interface 1.4.2, SigV4 1.3.1, Device Shadow 1.4.2, Device Defender 1.4.1, Jobs 2.0.1, Fleet Provisioning 1.2.2, MQTT File Streams 1.2.0 [P].
- Previous 202406 LTS ended 2026-06-30 [P/S, consistent with notes].
- Note: lwIP and mbedTLS are **not** part of FreeRTOS LTS [P, library list]. FreeRTOS-Plus-TCP 4.4.1 is in LTS and includes the fix for CVE-2026-7425 (fixed 4.2.6/4.4.1 per the notes).

### 4.2 lwIP — confirmed: no new release

- Git tags (GitHub mirror `lwip-tcpip/lwip`): newest **STABLE-2_2_1_RELEASE, 2025-02-05**; before that 2.2.0 (2023-09-25). **No 2.2.2, 2.3.0 or release candidate** [P mirror of upstream; Savannah news page did not render].
- Debian packages stay at 2.2.1; CVEs filed in 2026 (CVE-2026-8836 and others) name no fixed release [S].
- Master branch, November 2026 [P, git mirror commit list]: 2026-11-17 "tcp: fix Denial-of-Service via FIN-copy after window trimming in tcp_receive()", "makefsdata: prevent heap overflow …", SNMPv3/mbedTLS stack-overflow and division-by-zero fixes; 2026-11-16 TCP out-of-window and SYN_RCVD sequence-number checks; 2026-11-15/08 SNMP USM bounds checks and OOB read; 2026-11-08 "mqtt: fix use-after-free when callback frees client", PPPoL2TP double-free, mDNS short SRV record rejection.
- Inference: the **core TCP path** (not only optional apps) received DoS and state-machine fixes in November 2026. A product on lwIP must track master commits or a vendor fork (ST, NXP, TI ship their own), not the 2.2.1 tarball. This strengthens the CRA concern in `ehs_rtos_fit.md` §5.

### 4.3 Zephyr LTS — corrected

- Zephyr releases page [P]: supported releases **4.5.0 (2026-10-28, EOL 2027-10-28, latest)**, 4.4.0 (2026-04-14, EOL 2027-04-12), **3.7.0 LTS3 (2024-07-26, EOL 2029-07-27)**. Past LTS: 2.7.6 LTS2 EOL 2025-01-26; 1.14.1 LTS1 EOL 2022-01-01.
- Next release **4.6, targeted April 2027, marked LTS4**; projected 5.4 (October 2029) as LTS5 [P].
- Release process page [P]: LTS maintained "for approximately 5 years": first 2 years general bug and security fixes, then 3+ years "security and OS stability fixes only", then possibly third-party extended support; LTS published every 2.5–3 years.
- **Correction:** `ehs_rtos_fit.md` §4.1 and §5.2 say "upstream LTS still 3.7 (maintained to January 2027)". Per the current releases page, 3.7 is maintained (security/stability only since about mid-2026) until **2029-07-27**. The January 2027 date reflects the older policy [I on the reason].
- Canonical's commercial "Zephyr 26.04 LTS" claim in the notes was not re-checked here.

---

## Changes suggested for the study text (do not edit the prior notes; for the author)

1. `highend_mcu_path.md` §2.3 MQTT row: "a few KB of code" → coreMQTT 5 about 19–23 KB code, no heap.
2. `highend_mcu_path.md` §2.3 OPC UA row: replace "stay on 1.3.x" with "open62541 ≥ 1.5.0 (2026-02) supports FreeRTOS via the lwIP EventLoop; 1.5.8 current".
3. `ehs_rtos_fit.md` §4.1/§5.2: Zephyr 3.7 LTS EOL 2029-07-27 (security-only phase), next LTS 4.6 in April 2027; latest release 4.5.0.
4. `ehs_rtos_fit.md` §5.2 item 5: HMI panel = CRA default category (no IACS/HMI entry in final Annex III); Class I applies only if EDT sells an OS or boot loader as a product. No harmonised standard cited yet; EN 40000-1-1/-1-2 published 2026-11-18 without planned citation; prEN 40000-1-3 due 2026-11-30.
5. FreeRTOS 202604 LTS end date: 2028-04-30.
6. lwIP: still 2.2.1; November 2026 fixes in core TCP and the MQTT app exist only in git.

---

## Sources

### Item 1 (footprints)
- [P] Espressif ESP-IDF Mbed TLS docs (heap table): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mbedtls.html
- [P] TI SPRADB2, Integration of MbedTLS on Sitara MCU Devices (Aug 2023): https://www.ti.com/lit/pdf/spradb2
- [P] wolfSSL tinytls13 footprint post (July 2026): https://www.wolfssl.com/?p=48314
- [S] wolfSSL forum estimates: https://www.wolfssl.com/forums/post6803.html
- [P] Mbed TLS mailing list (PSA RAM overhead): https://lists.trustedfirmware.org/archives/list/mbed-tls@lists.trustedfirmware.org/message/KYJ75P4BP6XSI6AJFMIA3PHD2D4FW22W
- [S] arXiv 2011.12035 (DTLS/TLS 1.3 on low-power IoT): https://arxiv.org/pdf/2011.12035
- [S] Mbed OS forum, handshake allocation: https://forums.mbed.com/t/allocation-requirements-during-handshake/4610
- [P/S] ST Community, mbedTLS handshake time (ST staff reply 2025-02-12): https://community.st.com/t5/stm32-mcus-embedded-software/inquiry-on-mbedtls-handshake-time-and-optimization-for-aws-mqtt/td-p/758010
- [S] ST Community, NetX Secure handshake time per curve: https://community.st.com/t5/stm32-mcus-security/netx-secure-tls-handshake-time/m-p/648943
- [S] ST Community, STM32MP13 vs F7 TLS: https://community.st.com/t5/stm32-mpus-embedded-software-and/slow-tls-communication-with-netxduo-stm32mp13/m-p/715525
- [P] coreMQTT size table (v5): https://github.com/FreeRTOS/coreMQTT/blob/main/docs/doxygen/include/size_table.md
- [P] coreMQTT repo: https://github.com/FreeRTOS/coreMQTT
- [P] AWS coreMQTT user guide (older table, no-heap statement): https://docs.aws.amazon.com/freertos/latest/userguide/coremqtt.html
- [P] lwIP README copy: https://sming.readthedocs.io/en/latest/_inc/Sming/Components/lwip/lwip/README.html
- [P] FreeRTOS FAQ memory: https://FreeRTOS.org/FAQMem.html ; AWS kernel guide: https://docs.aws.amazon.com/freertos/latest/userguide/dev-guide-freertos-kernel.html
- [S] NXP Community, lwIP memory requirements: https://community.nxp.com/docs/DOC-330816
- [S] ST Community, lwIP on F417 memory: https://community.st.com/t5/stm32-mcus-embedded-software/lwip-32f417-how-to-optimise-network-stack-memory-allocation/td-p/102985
- [S] Nordic DevZone, MCUboot flash overflow: https://devzone.nordicsemi.com/f/nordic-q-a/116524/mcuboot-flash-overflow
- [S] Nordic DevZone, nRF54L15 bootloader partition: https://devzone.nordicsemi.com/f/nordic-q-a/128406/mcuboot-on-nrf54l15-can-not-fprotect-region-that-big
- [S] Nordic DevZone, MCUboot with USB DFU size: https://devzone.nordicsemi.com/f/nordic-q-a/116704/size-of-build-image-for-mcuboot-with-serial-or-usb-dfu-enabled/526424
- [S] ST Community, MCUboot size: https://community.st.com/t5/stm32-mcus-security/mcuboot-size/td-p/761326
- [S] Arm/Linaro TF-M trusted boot slides: https://www.slideshare.net/slideshow/hkg18223-trusted-firmwarem-trusted-boot/92871371
- [P] open62541 building docs: https://open62541.org/doc/master/building.html
- [P] open62541 v1.5.0 announcement (2026-02-02): https://groups.google.com/g/open62541/c/cvcvu80QJ5Q
- [P] open62541 v1.5.8 announcement (2026-09-07): https://groups.google.com/g/open62541/c/AjPlL-KDF7c
- [S] open62541 list, H7S3 bare metal: https://groups.google.com/g/open62541/c/eOo9n78MmtM
- [P, not read] Renesas R01AN8281EJ0120 RZ/T2 RZ/N2 OPC UA sample (§8.3 footprint): https://www.renesas.com/en/document/apn/rzt2-rzn2-group-opc-ua-server-and-pubsub-sample-software
- [P, not read: timeout] ST AN3966: https://www.st.com/resource/en/application_note/an3966-lwip-tcpip-stack-demonstration-for-stm32f4x7-microcontrollers-stmicroelectronics.pdf ; ST UM2178: https://www.st.com.cn/resource/en/user_manual/um2178-getting-started-with-xcubeaws-stm32cube-expansion-package-for-amazon-web-services-iot-core-stmicroelectronics.pdf

### Item 2 (CRA)
- [P] CRA Annex III text (OJ L 2024/2847 mirror): https://www.springlex.eu/en/packages/cra/cra-regulation/annex-3/
- [P] CRA Annex IV text (mirror): https://www.springlex.eu/en/packages/cra/cra-regulation/annex-4/
- [P] Implementing Regulation (EU) 2025/2392, EUR-Lex: https://eur-lex.europa.eu/eli/reg_impl/2025/2392/oj/eng (did not render); Annex I mirror: https://www.springlex.eu/en/packages/cra/tdpc-regulation/annex-1/
- [P] 2022 proposal Annex III (mirror): https://www.european-cyber-resilience-act.com/Cyber_Resilience_Act_Annex_3.html
- [S] HSF Kramer, Commission clarifies categories (2026): https://www.hsfkramer.com/notes/cybersecurity/2026-posts/cyber-resilience-act-commission-clarifies-important-and-critical-product-categories
- [S] TÜV Rheinland on 2025/2392: https://www.tuv.com/regulations-and-standards/en/eu-commission-implementing-regulation-eu-2025-2392-on-the-technical-description-of-the-categories-of-important-and-critical-products-with-digital-elements.html
- [S] OSADL CRA FAQ (core functionality, machine-control example): https://www.osadl.org/?id=4445
- [P] European Commission, CRA guidance C(2026) 6013 (2026-07-27): https://digital-strategy.ec.europa.eu/en/library/commission-publishes-new-guidance-support-timely-cyber-resilience-act-implementation
- [S] Orrick on the guidance (2026-09): https://www.orrick.com/fr-FR/Insights/2026/09/EU-Cyber-Resilience-Act-New-Guidance-Clarifies-Scope-Reporting
- [S] cyberresilienceact.eu, deadlines pushed back (2026-07-30, upd. 2026-09-11): https://www.cyberresilienceact.eu/news/cra-standardisation-deadlines-pushed-back-two-months.html
- [S] cyberresilienceact.eu, Commission extends deadlines (2026-11-10): https://www.cyberresilienceact.eu/news/commission-extends-cra-standardisation-deadlines-10-november-2026.html
- [S] IBF Solutions, standardisation status (upd. 2026-11-16): https://www.ibf-solutions.com/en/news-and-knowledge/technical-papers-and-news-on-ce-marking/current-status-of-standardisation-for-the-cyber-resilience-act
- [S] DKE CRA standardisation projects (June 2026 PDF): https://www.dke.de/resource/blob/2435170/a4c8225091ac18023516487e3e55cb23/normungsprojekte-zum-cra-data.pdf

### Item 3 (Taiwan hiring)
- [P] 104, 2026 tech industry salary (2026-04-28): https://blog.104.com.tw/104data-tech-industry-salary-2026/
- [P] 104, semiconductor hiring 2026: https://blog.104.com.tw/?p=247701
- [S] embedded.jobs Taiwan: https://embedded.jobs/embedded-systems-engineer-jobs-in-taiwan
- [S] Axiado Staff MCU/RTOS (Taipei): https://resumegeni.com/jobs/staff-mcurtos-engineer-at-axiado-taipei-tw
- [S] Ubiquiti Embedded Firmware Engineer: https://www.foundit.hk/job/embedded-firmware-engineer-connect-ubiquiti-inc-taiwan-35976137
- [S] Augentix Embedded Linux Platform Engineer: https://embedded.jobs/job/Embedded-Linux-Platform-Engineer-up-to-Staff-with-Augentix-Inc--ec101b
- [S] Ambiq Zephyr role (Hsinchu): https://job-boards.greenhouse.io/ambiqmicroinc/jobs/4136498009
- [S] 104 5G Embedded Linux (Yocto) posting: https://www.104.com.tw/job/8pufu
- [S] 104 hunter BMS firmware posting: https://hunter.104.com.tw/en/job/F000013947
- [S] Canonical Taipei role: https://canonical.com/careers/5140562
- [S] Taiwan Gold Card firmware job digest: https://goldcard.nat.gov.tw/zh/badges/%E9%9F%8C%E9%AB%94%E5%B7%A5%E7%A8%8B%E5%B8%AB/page/2
- [S] 1111 survey press coverage: https://udn.com/news/story/7240/9667066
- Not obtainable (403/bot check/429): 104 keyword search, 1111 search, cake.me search, LinkedIn jobs search.

### Item 4 (support windows)
- [P] AWS, FreeRTOS 202604 LTS (2026-05-01): https://aws.amazon.com/about-aws/whats-new/2026/04/freertos-lts/
- [P] FreeRTOS-LTS repo: https://github.com/FreeRTOS/FreeRTOS-LTS
- [P] lwIP tags (GitHub mirror): https://github.com/lwip-tcpip/lwip/tags ; commits: https://github.com/lwip-tcpip/lwip/commits/master
- [S] Debian lwIP tracker: https://tracker.debian.org/pkg/lwip
- [P] Zephyr releases: https://docs.zephyrproject.org/latest/releases/index.html
- [P] Zephyr release process: https://docs.zephyrproject.org/latest/project/release_process.html
