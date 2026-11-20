# High-end MCU + RTOS path for a 7"–10" Ethernet industrial HMI

Notes date: 2026-11-20. Scope: the "high-end MCU + RTOS" option for an iFace-style industrial HMI built with EDT HMI Studio (EHS). EHS today generates LVGL C for STM32F746, H747 and U599 boards as a bare-metal super loop with no networking. The other two paths (Cortex-A + Linux, Cortex-A + RTOS) are covered elsewhere; Linux SoC/SoM prices and the 2026 memory shock are in study 17 (`notes/17-novakon-hardware-model/cost_estimation_and_model.md`) and study 18.

Method: web searches (standard mode, one extended) and page fetches. st.com timed out and several NXP/DigiKey pages refused fetches, so many specs come from vendor pages via search summaries, CNX Software launch articles, or a fetched NXP fact-sheet PDF. **Distributor prices are search-snippet snapshots with no visible date**, retrieved on 2026-11-20. Treat them as order-of-magnitude and requote before any BOM decision.

## Labels

- **[P]** primary: the vendor's own datasheet, fact sheet, product page, store page, SDK document or press release; the vendor's own forum staff answer.
- **[S]** secondary: trade press (CNX Software, Hackster, Elektor), distributor listings and snippets, user forum posts, third-party blogs.
- **[I]** inference: my own judgement or arithmetic. Also used for background knowledge I did not re-verify this session (marked "unverified").

---

## 0. Short answer [I unless tagged]

1. **The MCU path can carry a 7" 800×480 or 1024×600 Ethernet HMI.** A 10" 1280×800 panel is feasible but marginal. At 1280×800 the framebuffer, UI and networking all fight over external-memory bandwidth, and only three candidates have enough internal SRAM to keep one RGB565 1280×800 framebuffer (2.05 MB) on-chip: N6 (4.2 MB), Alif E7 (13.5 MB), and RA8D2/RA8P1 (2 MB, too tight). The MCU route is a cost and boot-time play for 4.3"–7", and a "good enough" play for 10".
2. **Shortlist.**
   - **STM32H747 / H7S7.** The lowest switching cost: EHS already runs on H747, and the H7S78-DK is a US$99 kit with Ethernet and PSRAM.
   - **STM32N6.** 800 MHz M55, 4.2 MB SRAM, NeoChrom, GbE with TSN; about US$13–14 at 500 units. The best ST part for 10", but it has no internal flash, its BGAs are 142–264 balls, and it is new.
   - **NXP i.MX RT1170.** 1 GHz M7, 2 MB SRAM, PXP + VGLite, 2×GbE (one with TSN), 3×CAN-FD; about US$12 at 600 units. The most proven "HMI + Ethernet MCU" with the most networking examples.
   - **Renesas RA8D2 / RA8P1.** 1 GHz M85, 2 MB SRAM, Dave2D, 2×GbE with a TSN switch, TrustZone; about US$16–20. The strongest new-generation alternative.
   - **ESP32-P4.** About US$4 for the chip with 32 MB PSRAM in the package, 1024×600 at about 55 fps in Espressif's own LVGL table, 10/100 Ethernet, external Wi-Fi via ESP-Hosted. The cheapest by far, but its security and industrial ecosystem are weaker, and a 2026 chip-revision change (v1.3 → v3.2) is not compatible in hardware or software.
3. **Drop or deprioritise.**
   - STM32U5G9: no Ethernet MAC found, and 160 MHz.
   - i.MX RT1180: no display controller. It is a good industrial-network companion chip, not an HMI MCU.
   - MCX N947: no RGB LCD controller (FlexIO/8080 only), 150 MHz.
   - PIC32CZ CA: no display controller or GPU.
   - Alif E7: interesting (13.5 MB SRAM, D/AVE 2D, MIPI DSI), but 10/100 only, one CAN-FD, a small vendor, and a thin industrial ecosystem.
   - Infineon PSOC Edge E84: display only up to 1024×768 over MIPI DSI/DBI, 10/100.
4. **Networking maturity is adequate, not luxurious.**
   - 100 Mbit: lwIP, NetX Duo and Zephyr reach about 75–95 Mbit/s TCP on M7-class parts when tuned [S].
   - Gigabit MACs on RT1170 reach about 130–430 Mbit/s with stock examples [S].
   - TLS handshakes cost about 0.2–1 s and about 40–80 KB of RAM per session.
   - MQTT and Modbus TCP are trivial.
   - An open62541 OPC UA server fits in about 100–256 KB RAM with a minimal namespace [P/S].
   - Remote view: no open-source VNC server works with LVGL on an MCU. EHS's WASM build is the better remote-view story.
5. **BOM.**
   - An MCU HMI core costs about US$12–30 in 2026: MCU + 32 MB PSRAM or SDRAM + 32–128 MB NOR.
   - A Linux core costs about US$35–75: SoC + 256 MB–1 GB DRAM + eMMC + PMIC, from study 17's 2026 numbers.
   - Memory is still exposed, but far less: PSRAM vendor AP Memory raised IoT-RAM prices in 2026, and SDR SDRAM is a legacy DRAM type.
   - Partly offsetting this, the MCU board needs more custom firmware engineering per feature (RTOS, drivers, OTA, security).

---

## 1. Candidate MCUs

Memory note: framebuffer arithmetic [I]:

| Resolution | RGB565, one buffer | RGB565, double buffer | RGB888 / XRGB8888, one buffer |
| --- | --- | --- | --- |
| 800×480 | 0.77 MB | 1.54 MB | 1.15 / 1.54 MB |
| 1024×600 | 1.23 MB | 2.46 MB | 1.84 / 2.46 MB |
| 1280×800 | 2.05 MB | 4.10 MB | 3.07 / 4.10 MB |

Display scan-out alone at 60 Hz RGB565 [I]:
- 800×480: about 46 MB/s;
- 1024×600: about 74 MB/s;
- 1280×800: about 123 MB/s.

Rendering, GPU blending and image fetches come on top. A 16-bit SDR SDRAM at 100 MHz peaks at 200 MB/s, and a 200 MHz DTR octal PSRAM peaks at 400 MB/s. Both are theoretical maxima that real systems reach only partly. This is why every forum report on large panels names external-RAM bandwidth as the bottleneck (§3).

### 1.1 Summary table

| MCU | Core / clock | Internal SRAM | External RAM interface | Display / max res | 2D GPU | Ethernet | USB host | CAN-FD | Security | Price (qty, source) | Dev board |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **STM32H743** | M7 480 MHz | ~1 MB [I, background] | FMC 32-bit SDRAM, Quad-SPI [I, background] | LTDC (ST: up to XGA) [I, background] | Chrom-ART DMA2D | 10/100 | HS (external ULPI PHY) + FS [I, background] | 2 [I, background] | no TrustZone; RDP + SBSFU | US$9.47 @540 (DigiKey US) [S] | Nucleo/eval boards |
| **STM32H747** (EHS board today) | M7 480 + M4 240 MHz [S] | ~1 MB | FMC SDRAM, Quad-SPI | LTDC + MIPI DSI | DMA2D + JPEG | 10/100 | HS (ULPI) + FS | 2 | as H743 | US$12.62 @595 (DigiKey US); Future US$10.87 @714 [S] | STM32H747I-DISCO ≈US$96 (ST eStore) [S] |
| **STM32H7S7 / H7R7** | M7 600 MHz [S] | 620 KB [S] | 2× xSPI (octal/hexa, HyperRAM/PSRAM, 200 MHz) + FMC with SDRAM [S, via ST page snippet] | LTDC | NeoChrom 2.5D (R7/S7) + DMA2D + JPEG [S] | 10/100 [S] | 2× USB HS/FS with PHY [S] | — (not confirmed) | H7S: on-the-fly memory encryption (MCE), STiRoT ROM boot, targets SESIP3/PSA L3 [P/S]; no TrustZone (M7) | H7S7L8H6H: DigiKey UK £5.91 @160; TME €7.22 @320 [S] | STM32H7S78-DK US$99, with 256 Mbit octal PSRAM, 1 Gbit octal NOR, WVGA LCD, Ethernet [S] |
| **STM32N6 (N657/N647/N645)** | M55 800 MHz + Helium; NPU on N6x7 only [S] | 4.2 MB contiguous [S] | FMC (SDRAM/PSRAM), hexa/octo-SPI [S] | LTDC up to XGA (vendor figure) [S] | NeoChrom 2.5D + DMA2D + JPEG/H.264 enc [S] | **10/100/1000 + TSN** [S] | 2× USB OTG; CNX says FS, other ST material says HS [S, conflicting] | 3× FDCAN [S] | TrustZone (Armv8.1-M) and secure boot [I, unverified] | ST eStore N657A0H3Q US$13.73 @500 (another eStore page: US$11.83) [P]; DigiKey N657X0H3Q ≈US$13.35 @595 [S] | STM32N6570-DK US$185; NUCLEO-N657X0-Q US$56.25 [S] |
| **STM32U5G9** | M33 160 MHz [S] | 3 MB [P via snippet] | OCTOSPI/HSPI [I, background] | LTDC + MIPI DSI [I, background] | NeoChrom (GPU2D, NeoChromVG) + DMA2D [P] | **none found** [S] | HS [I, background] | yes [S] | TrustZone | DigiKey DE €16 @1; aggregator ≈US$14.09 @10k [S] | STM32U5G9J-DK2 ≈US$52 (Future) / US$73.50 (eStore) [S] |
| **NXP i.MX RT1060/1062** | M7 600 MHz | 1 MB [I, background] | SEMC SDRAM, 2× FlexSPI [I, background] | eLCDIF up to WXGA [I, background] | PXP | 2× 10/100 [I, background] | 2× USB HS OTG with PHY | 3× CAN (1× FD) | HAB secure boot, BEE [I, background] | US$6.95 @1,200 (DigiKey MX) [S] | MIMXRT1060-EVKC (price not checked) |
| **NXP i.MX RT1170 (1176/1175/1173)** | M7 1 GHz (800 MHz industrial grade) + M4 400 MHz [P] | 2 MB (512 KB M7 TCM, 256 KB M4 TCM, 1 MB OCRAM) [P] | SEMC 8/16/32-bit SDRAM, 2× FlexSPI (HyperRAM/PSRAM/octal) [P] | Parallel LCD up to WXGA 1280×800 @60, 2-lane MIPI DSI [P/S] | PXP + 2D GPU with vector graphics (VGLite, "OpenVG up to 500 MHz") [P] | **1× GbE with AVB + 1× GbE with TSN + 1× 10/100** [P] | 2× USB 2.0 OTG with PHY [P] | 3× CAN-FD [P] | HAB keys for secure boot, OTFAD/IEE, ECC/RSA, EdgeLock Assurance; no TrustZone (M7) [P] | DVMAA US$12.11 @608, US$18.83 @1 (DigiKey US), flagged "not for new designs"; successor DVMAB listed [S] | MIMXRT1170-EVKB ≈US$190 [S] |
| **NXP i.MX RT1180** | M7 800 + M33 240 MHz [S] | up to 1.5 MB [S] | 2× FlexSPI, SDRAM [S] | **none** [S] | — | **5-port GbE TSN switch**, EtherCAT slave [S] | 2× OTG | 3 | EdgeLock Secure Enclave [S] | not checked | — |
| **NXP MCX N947** | 2× M33 150 MHz [S] | 512 KB [S] | FlexSPI | **no LCD controller**: FlexIO + SmartDMA to 8080/DBI panels [S] | — | 10/100 [S] | HS | 2 [I, background] | TrustZone, EdgeLock enclave [I, background] | DigiKey UK £6.94 @520 [S] | FRDM-MCXN947 ≈US$23 [S] |
| **Renesas RA8D1** | M85 480 MHz + Helium [S] | 1 MB [S] | 32-bit SDRAM, OSPI [S] | GLCDC up to WXGA, optional MIPI DSI [S] | Dave2D (DRW) [S] | 10/100 [S] | HS | 2 [I, background] | TrustZone, RSIP | Renesas 1ku US$11.51–11.83; launch US$17.37 @100 [P/S] | EK-RA8D1 US$332 at launch, US$500 on DigiKey [S] |
| **Renesas RA8D2** | M85 1 GHz + optional M33 250 MHz [S] | 2 MB, including TCM [S] | OSPI (SiP up to 8 MB flash), 0.5/1 MB MRAM; EK has 64 MB SDRAM, so a SDRAM bus exists [S/I] | GLCDC RGB888, 3 layers, MIPI DSI 2-lane, WXGA [S] | Dave2D (DRW) [S] | **2× GbE with TSN (L3 switch, 2-port TSN switch)** [P/S] | HS host/OTG | 2 [S] | TrustZone, secure boot, RSIP-E50D, tamper [S] | DigiKey US$24.46–26.55 @1, ≈US$16–18 in volume [S] | EK-RA8D2 (7" 1024×600): DigiKey UK £148, EU €167, Mouser ≈US$279 [S] |
| **Renesas RA8P1** | M85 1 GHz + M33 250 MHz + Ethos-U55 [S] | 2 MB [S] | 2× xSPI 333 MB/s, 16/32-bit external bus [S] | GLCDC RGB + MIPI DSI, WXGA [S] | Dave2D [S] | **2× GbE with TSN + 2-port switch** [S] | HS | 2 [S] | TrustZone, RSIP-E51A, immutable boot ROM, DPA protection [S] | Mouser US$20.34 @100 [S] | EK-RA8P1 US$195.98 (Mouser) [S] |
| **Microchip PIC32CZ CA80/CA90** | M7 300 MHz [S] | 512 KB / 1 MB [S] | 16-bit EBI (SRAM/PSRAM/LCD modules), 2× SQI; **no SDRAM controller listed** [S] | **no display controller** [S] | none | **10/100/1000 with AVB, 1588** [S] | 2× HS [S] | up to 6 [S] | HSM on CA90 (NDA datasheet) [S] | CA90 US$15.54 @10k (launch, 2023) [S] | Curiosity Ultra US$249 [S] |
| **Alif Ensemble E7** | 2× A32 800 MHz + M55 400 + M55 160, 2× Ethos-U55 [P] | 13.5 MB [P] | 2× octal SPI with HyperBus, up to 200 MB/s DDR; no SDRAM [P] | LCD controller + 24-bit DPI + 2-lane MIPI DSI [P] | D/AVE 2D [P] | 10/100 [P] | 1× HS host/device [P] | 1 [P] | secure enclave, TrustZone on both core types [P] | Mouser India ₹1,531 @3,000 reel (≈US$17–18 [I]) [S] | DK-E7 (price not found) |
| **Espressif ESP32-P4** | 2× RISC-V 400 MHz (rev v3.x) + LP core [P] | 768 KB + 8 KB TCM [P] | in-package PSRAM: 16/32 MB variants, e.g. P4NRW32 [S] | MIPI DSI up to 1080p, parallel RGB [P] | PPA + 2D-DMA [P] | 10/100 EMAC (RMII) [P/I] | USB 2.0 HS OTG [P] | TWAI; CAN-FD not stated [P] | secure boot, flash encryption, key manager, DS peripheral; **no TrustZone** [P] | ESP32-P4NRW32 LCSC US$4.13 @100 (US$5.21 @1) [S] | ESP32-P4-Function-EV-Board US$55 with 7" screen [S] |
| **Infineon PSOC Edge E84** | M55 + M33 + U55 NPU [S] | not checked | OSPI | display controller, MIPI DSI/DBI up to 1024×768 [P via snippet] | 2.5D GPU [P] | 10/100 [S] | — | — | — | not checked | — |
| **HPMicro HPM6880** (extra) | RISC-V 600 MHz [S] | not checked | **DDR2/DDR3 controller** [S] | 2× 4-lane MIPI DSI/LVDS, LCDC [S] | Vivante 2.5D (OpenVG) [S] | GbE [S] | — | — | — | chip price not found | HPM6800EVK ≈€200, China-focused [S] |

Notes on specific rows:
- **H743/H747 rows:** several cells are background knowledge marked [I]. st.com timed out; check them against DS12110 (H743) and DS12930 (H747).
- **LTDC "up to XGA" is conservative.** Riverdi ships 10.1" 1280×800 displays driven by the STM32H757's LTDC with 8 MB SDRAM [S], so WXGA works in practice.
- **RT1176DVMAA** shows "not recommended/not for new designs" on DigiKey because silicon revision B (DVMAB) replaces it [S]. Use the B part numbers.
- **ESP32-P4 chip revision.** PCN202600801 announces EOL of chip revision v1.3 and moves customers to v3.2. v3.2 is **hardware- and software-incompatible** with v1.3 [P]. Design only with v3.x parts.
- **RA8D2 SDRAM:** the evaluation kit has 64 MB SDRAM, which implies a SDRAM bus [S/I]. CNX did not list it; confirm in the datasheet.

### 1.2 Longevity

| Vendor | Programme | Evidence |
| --- | --- | --- |
| ST | 10-year rolling commitment, renewed annually; each part page shows an "available until" date | [S] ST staff on the ST Community; per-part dates not checked (st.com timed out) |
| NXP | 10- or 15-year programme; RT1060 (and RT1010/1020/1050) extended to 15 years from launch. RT1170 was not yet listed when NXP staff answered in 2021 | [P] NXP blog and forum staff. Check the live table |
| Renesas | RA longevity not found this session | — |
| Microchip | long-standing "customer-driven obsolescence" policy | [I, background, unverified] |
| Espressif | ESP32-P4: **12 years, 2025-01-01 to 2037-01-01** | [P] Espressif longevity page |
| Alif | not stated on the E7 page | [P] |

### 1.3 Ranking for EHS [I]

| Rank | MCU | Why |
| --- | --- | --- |
| 1 | STM32H747 → STM32N6 | Same vendor, CubeMX/HAL and existing EHS board support. H747 for a 7" 10/100 SKU now; N6 for a 10" GbE SKU once EHS adds an N6 board. The N6 has no internal flash, so it needs external octal NOR (XIP) plus a boot flow; that is new work for EHS |
| 2 | i.MX RT1170 | Most mature Ethernet + HMI MCU: SDK examples for lwIP iperf, MCUboot OTA and VGLite LVGL; Zephyr support. A second vendor reduces ST lock-in |
| 3 | RA8D2 / RA8P1 | Best paper specs per dollar (1 GHz M85, GbE + TSN switch, TrustZone, Dave2D with LVGL certification on RA8D1). The newest, with the thinnest community |
| 4 | ESP32-P4 | Lowest cost by far. 32 MB PSRAM in package removes the external-RAM routing. 10/100 Ethernet only, and Wi-Fi needs a second chip. The fit for "smart display / low tier", not premium industrial |

---

## 2. Networking maturity on MCUs

### 2.1 TCP/IP stacks and throughput

| Stack / board | Result | Source |
| --- | --- | --- |
| lwIP + FreeRTOS, NUCLEO-H723ZG (10/100) | iperf 91–95 Mbit/s | [S] ST Community user post |
| lwIP on H743 (another user) | stuck at about 10 Mbit/s until tuned. The usual causes are TCP_WND, pbuf counts, cache/MPU settings and the receive path | [S] ST Community |
| FreeRTOS+TCP on STM32H7 | others report about 80 Mbit/s | [S] FreeRTOS forum |
| NetX Duo, STM32H747-DISCO | TCP about 55 Mbit/s, UDP 95 Mbit/s (before tuning) | [S] ST Community |
| NetX Duo, older Express Logic claim | 93–95 Mbit/s on STM32F2 (M3) | [S] trade press |
| NetX Duo, Microsoft iperf documentation | TCP TX 91, RX 71 Mbit/s, hardware unspecified | [S] |
| Zephyr 3.6 zperf, MIMXRT1170-EVK, 100M port, code in ITCM | TCP RX 87.7, TX 73.6; UDP RX 95.1, TX 79.8 Mbit/s | [S] Zephyr GitHub discussion #72760 |
| lwIP, RT1170 GbE port (MCUXpresso examples) | TCP RX 127–390 Mbit/s, TX about 150; UDP up to about 430 Mbit/s. One NXP-side claim of about 950 Mbit/s UDP was not reproduced | [S] NXP Community |

Takeaways [I]:
- HMI traffic (polling a few hundred tags, MQTT telemetry, a web page, project download) needs well under 10 Mbit/s. 10/100 is enough.
- GbE matters only for:
  - fast project and firmware download (an 8–32 MB image in seconds rather than tens of seconds);
  - TSN or "same MAC as the controller" marketing.
- Every stack works on these chips. Getting from "works" to near wire speed costs about 1–3 engineer-weeks of tuning per board (buffers, cache coherency, ITCM placement).

**Stack choice for EHS [I]:**

| Stack | Fit |
| --- | --- |
| FreeRTOS + lwIP | The most examples across ST, NXP, Renesas and Espressif (ESP-IDF is FreeRTOS + lwIP). Default choice |
| ThreadX + NetX Duo | Integrated in STM32Cube and Renesas FSP. Very good, but a smaller community after the move to Eclipse. MIT-licensed since 2024 [I, background] |
| Zephyr | The best portability across all candidates (all four shortlisted families have boards). Has an LVGL module and MCUboot built in. Steepest learning curve for a team used to CubeMX |

### 2.2 TLS (mbedTLS / wolfSSL)

| Item | Figure | Source |
| --- | --- | --- |
| Software TLS 1.2 ECDHE-ECDSA handshake, M4 at 120 MHz | 1.1–2.2 s | [S] uncited guide |
| P-256 ECDHE, M4 at 120 MHz | 391 ms with mbedTLS; Curve25519-donna 58 ms | [S] 2015 IETF/NIST slides |
| STM32F417 at 168 MHz, full handshake | 2–3 s | [S] lwIP mailing list |
| wolfCrypt, STM32H753 at 480 MHz | ECDSA-256 sign 64 ops/s, verify about 31 ops/s, ECDHE about 49 ops/s (wolfSSL 4.4.1, software SP-math) | [S] wolfSSL commit notes |
| wolfSSL on STM32H7S | the hardware PKA was **slower** than wolfSSL's Cortex-M assembly | [P] wolfSSL blog |
| mbedTLS 2.16 on F437 | about 38 KB RAM per TLS connection; others report about 32 KB extra during handshake and about 50 KB private heap for an HTTPS client | [S] forum posts |
| wolfSSL TLS benchmark example (STM32Cube) | about 76 KB for tasks + peak heap | [S] |

Inference:
- On a 480–1000 MHz M7/M55/M85, one ECDHE-ECDSA P-256 handshake costs about 50–150 ms of crypto. With round trips and RSA certificate chains, expect about 0.2–1 s.
- Budget about 40–80 KB RAM per concurrent TLS session, and design for one or two concurrent sessions: MQTT to the cloud plus an HTTPS or OPC UA session. A browser opening four or more parallel HTTPS connections to an MCU web server is the RAM trap.

### 2.3 Industrial and IoT protocols

| Protocol | MCU status | Source / label |
| --- | --- | --- |
| Modbus TCP | Trivial on lwIP sockets or raw API. EHS already has Modbus RTU code; TCP framing is a small extension (study 18). Small MIT/BSD libraries exist (e.g. nanoMODBUS) | [I] |
| MQTT | coreMQTT (FreeRTOS, MIT), Paho Embedded-C, lwIP's built-in MQTT client, NetX Duo MQTT, Zephyr MQTT. Footprint is a few KB of code; TLS dominates RAM | [I, background, unverified footprints] |
| HTTP server | lwIP httpd (static and SSI/CGI), NetX Duo web server, Zephyr HTTP server, ESP-IDF esp_http_server. Enough for a config page and a REST/JSON tag API, not a heavy SPA with many parallel TLS connections | [I] |
| OPC UA server (open62541) | Docs: minimal servers "can be configured to need less than 100kB of RAM and ROM" with MINIMAL namespace 0 and MinSizeRel. A maintainer: "256kB RAM should be more than enough, 96kB is possible but needs some real work". Default 64 KB send/receive buffers were a common RAM trap (later about 16 KB). An F407 port saw about 2 KB stack and 6 KB heap before namespace initialisation. FreeRTOS support was removed in 1.4 with the new event loop, so stay on 1.3.x or use lwIP raw/POSIX on ≥1.4. The ESP32 port strongly recommends PSRAM | [P] docs; [S] forum |
| OPC UA, commercial | o6 Embedded claims under 100 kB RAM for a full server and complete namespace 0 under 300 kB (vendor claim). Renesas publishes OPC UA server + PubSub samples for RZ/T2 and RZ/N2 | [S] |

Inference: with 2–4 MB of internal SRAM, or PSRAM for heaps, a nano/micro-profile OPC UA server with a few hundred tag nodes is realistic on H7/N6/RT1170/RA8. Formal OPC Foundation certification is a separate cost.

### 2.4 Remote view (VNC-like)

| Option | Notes |
| --- | --- |
| SEGGER emVNC | [P] Commercial ANSI-C VNC server for embedded devices. Works with emWin or a plain pixel buffer, over TCP/IP or USB. Needs a multitasking OS. Source licence |
| STemWin VNC | [P/S] Was available on STM32F7. ST says it is not supported on STM32H7 and points to TouchGFX, which has no VNC |
| LVGL | [S] No built-in VNC. The LVGL team says the user must glue the flush buffer to a VNC/RFB server. A community remote-viewer library exists (custom protocol) |
| EHS-specific | [I] EHS already compiles the project to WASM for its Emulator (study 18). The cheapest "remote HMI" on an MCU is to serve the same WASM UI from the panel's HTTP server, or from a PC or cloud, and stream tag values over WebSocket or MQTT. That needs no framebuffer encoding, which an MCU does poorly at 1280×800. A minimal RFB server sending raw/RRE/hextile updates of LVGL's dirty areas is a possible second step (about 2–4 weeks) |

### 2.5 Wi-Fi via an external module

| Option | Notes |
| --- | --- |
| ESP-Hosted (Espressif) | [P] Newer releases (3.0.x/3.1.0) list "Other MCUs" as hosts, over SDIO or SPI, with a "network split" mode (separate lwIP stacks on host and co-processor). The ESP32-P4 product page names ESP-Hosted / ESP-AT with an external ESP32-C/S as its Wi-Fi path |
| Measured throughput | [S] P4 host + C6 over SDIO 4-bit 40 MHz: bus ceiling 79.5 Mbit/s, about 36 Mbit/s real Wi-Fi. One STM32H7 SDIO user saw only about 4 Mbit/s, suspecting signal integrity |
| ESP-AT over UART/SPI | Simplest to integrate. Enough for MQTT and telemetry, not for fast downloads [I] |
| Certification | A pre-certified module keeps RF certification mostly on the module, but the product still needs its own radio test file [I] |

---

## 3. LVGL performance on these MCUs

| Platform | Resolution | Result | Conditions | Source |
| --- | --- | --- | --- | --- |
| STM32H723 at 550 MHz, custom board | 800×480, LTDC | lv_demo_benchmark **82 FPS** | LVGL 8.3.11; "main bottleneck is SDRAM bandwidth" | [S] LVGL forum |
| STM32H7S78-DK | 800×480 | 27 FPS average; DMA2D render 3 ms vs software 4 ms; 18–19% CPU | DMA2D helps only fills and images | [S] LVGL forum |
| STM32H7B3I-DK | 480×272 | weighted 40 FPS software → 76 FPS with DMA2D | LVGL port README | [S] |
| STM32U5G9J-DK2 (NeoChrom) | 800×480 | about 25–29 FPS average (widgets demo 25) | double buffer, LTDC VSYNC, no OS; LVGL 9.3 dev made GPU2D work | [S] LVGL port README and forum |
| STM32N6570-DK | 800×480 | **no published LVGL FPS found**; a user reports the benchmark "runs well" with Nema GPU or software | ST markets NeoChrom "60 FPS" (TouchGFX) | [S]; [P] ST slides |
| i.MX RT1170 EVK | 720×1280 MIPI | weighted **24 FPS** without VGLite at default optimisation; **47 FPS** at -O2 (NXP staff answer) | other users report under 15 FPS before tuning. An NXP knowledge article shows Widgets 2 → 59 FPS and CPU 96% → 12% after optimisation | [S] NXP Community |
| Renesas EK-RA8D1 (Dave2D) | 480×854 | in the heaviest scene (several rotated ARGB images) **12 FPS, 66 ms render, CPU only about 10%**; FPS drops only in "highly complex" scenes | LVGL 9, two framebuffers in external SDRAM, direct mode; "external SDRAM limits performance" | [P] LVGL board-certification review |
| ESP32-P4 (MIPI DSI) | 1024×600 RGB565 | **55 FPS**, about 3.7 MB PSRAM | LVGL 9.4 benchmark via esp_lvgl_adapter | [P] Espressif LCD application note |
| ESP32-P4 | 800×1280 RGB565 | **40 FPS** (RGB888: 30), 6 MB PSRAM | same | [P] |
| ESP32-P4 | 800×480 RGB (parallel) | 60 FPS | same | [P] |
| ESP32-P4 | 1080×1920 RGB565 | 25 FPS | same | [P] |
| ESP32-P4 PPA in LVGL 9.4 | — | about 30% faster on many operations, up to 9× in fill-heavy cases; experimental | | [P] Espressif blog |
| Riverdi STM32H757 10.1" | 1280×800 | product exists (8 MB SDRAM, TouchGFX); no LVGL FPS | | [S] Riverdi |

Inference:
- **800×480:** every shortlisted MCU gives a smooth industrial UI at 30–60 FPS, which is the domain of typical HMI screens (buttons, gauges, trends, numeric fields).
- **1024×600:** comfortable on RT1170, N6, RA8D2/P1 and ESP32-P4, and acceptable on H747/H7S7 with partial refresh. Full-screen animations will stutter on the H7.
- **1280×800:** feasible (Riverdi H757, ESP32-P4 at about 40 FPS, RT1170 720×1280 at about 47 FPS). Expect 20–40 FPS on complex screens and keep animations local. Industrial HMI screens are mostly static, so this is acceptable for a 10" "Basic"-tier SKU, not a premium one.
- **No benchmark covers a concurrent network load.** Two tests should be EHS's own first benchmarks:
  - LVGL + lwIP iperf + a TLS session at once on the target board;
  - FMC SDRAM shared by the LTDC scan-out, the GPU and the Ethernet DMA, on H7 and RT1170.

---

## 4. Commercial MCU-based HMIs and smart displays

| Product | CPU | Ethernet | Source / notes |
| --- | --- | --- | --- |
| Siemens KTP400–1200 Basic (2nd gen) | datasheet says only "Processor type: ARM"; vendor and core not disclosed | PROFINET | [P] Siemens datasheet. Forum claims of custom Siemens silicon contradict each other [S]. MCU vs MPU class unknown |
| AutomationDirect C-more Micro (EA3) | not disclosed; no teardown found | built in on EA3-S3ML/T4CL; EA-ECOM module for 6"/8"/10" | [P] spec sheets. The 3"–10" range, low price and simple feature set suggest an MCU or ARM9 class [I] |
| Unitronics Samba / Vision | not disclosed | Vision 700: 1× Ethernet | [P] catalogue |
| Weintek lowest tier (MT8051iP, MT8071iP) | **Cortex-A8 600 MHz, 128 MB RAM, 128 MB flash**, Linux | yes | [P] Weintek datasheets via distributors. MT8051iP is discontinued and replaced by MT8052iP [S]. Weintek's cheapest tier is an MPU, not an MCU |
| Red Lion CR1000 | RS lists TI Sitara Cortex-A9 800 MHz, 512 MB; a 2017 Red Lion deck says "ARM9" | yes | [S] |
| NOVAKON N04/N07 | ARM9 400 MHz, 128 MB | 10/100 | [P/S] study 17 |
| DWIN T5L smart displays (DGUS) | DWIN's own T5L ASIC (8051 core + GUI core) [I, background] | **no Ethernet model found** (UART/RS232/RS485, one CAN option, an "IoT" variant) | [S]; 7" retail about ₹3,200–3,750 (about US$37–43) |
| Riverdi STM32 Embedded Display 10.1" | **STM32H757XIH6**, 8 MB SDRAM, 64 MB QSPI | via a PoE add-on board (10/100 + PoE) | [P/S] about US$243 (Riverdi), £181–207 (DigiKey/RS). The clearest example of a commercial 10" WXGA MCU display |
| Makerfabs MaTouch ESP32-P4 10.1" | ESP32-P4 | 10/100 + RJ45, 4G LTE | [S] CNX, US$79.80 |
| Waveshare ESP32-P4-WIFI6 HMI (8"/10.1", 1280×800) | ESP32-P4 + C6 | separate PoE-ETH board | [S] US$80–90 |
| Crank Storyboard / Qt for MCUs | RT1170 and STM32H7 demos | — | [P/S] Automotive and generic demos; no Ethernet-connected industrial product found |
| 7" UART smart display (generic, Alibaba) | MCU | no | [S] US$24–26 at 1–999 pcs (study 17) |

Inference:
- No major PLC-brand HMI publicly confirms an MCU-class CPU.
- The MCU-based market is "smart display modules" (DWIN, Riverdi, ESP32-P4 boards) and probably the cheapest compact panels (KTP Basic, C-more Micro).
- Every vendor that names a CPU in the "basic" tier (Weintek, NOVAKON, Red Lion) uses ARM9 or Cortex-A with Linux.
- So an MCU-based Ethernet HMI with real PLC drivers would sit in a thinly contested slot: below a Weintek iP in BOM, above smart displays in features.

---

## 5. OTA and secure update on MCUs

| Vendor / tool | Status |
| --- | --- |
| ST, STM32H7 (H743/H747) | X-CUBE-SBSFU (legacy) supports STM32H7 [P]. ST says SBSFU legacy "no longer evolves", only critical fixes, and is replaced by OEMiRoT/OEMuRoT based on MCUboot for all new series [P/S]. It is unclear whether the MCUboot flow is offered for plain H7 [S] |
| ST, STM32H7S | STiRoT (ST immutable ROM root of trust, MCUboot-based, targets SESIP3; needs the CRYP block, so H7S only, not H7R) plus an updatable OEMuRoT in external flash, and OEMiRoT examples in STM32CubeH7RS [P] |
| ST, STM32N6 | MCUboot-based OEMiRoT/STiRoT flow likely [I, unverified] |
| NXP, RT1170/RT1060 | MCUXpresso SDK ships `mcuboot_opensource` plus `ota_mcuboot_server/client_enet` and HTTPS OTA examples with imgtool signing [P]. Users report flash-layout pitfalls [S]. NXP SBL (secure bootloader) is also available for RT1170 [S]. ROM HAB/encrypted XIP gives the root of trust |
| Renesas, RA | FSP ships MCUboot integration; RA8 adds an immutable boot ROM and TrustZone [I, background; P for RA8P1 boot ROM] |
| Espressif, ESP32-P4 | ESP-IDF OTA (A/B app partitions), secure boot v2, flash encryption [P/I] |
| Zephyr | MCUboot is native and the same across vendors; mcumgr/SMP for updates [I, background] |

Inference: for EHS,
- **MCUboot + signed images** is the common denominator across ST, NXP, Renesas and Zephyr. It matches the CRA's expectation of authenticated, rollback-protected updates.
- Keep the "project" (screens, assets) separate from the "runtime firmware" so that a project download does not need a firmware signature cycle. On an MCU, though, generated C means a project change is a firmware change (§6).

---

## 6. BOM implications vs Cortex-A + Linux [I unless tagged]

### 6.1 Memory parts (snapshot prices, 2026-11-20)

| Part | Price | Source |
| --- | --- | --- |
| 256 Mbit (32 MB) octal PSRAM, AP Memory APS256XXN-OBR-WA | US$2.62 @100 (LCSC, out of stock) | [S] |
| Infineon S80KS2563 (256 Mbit octal HyperRAM-class) | about US$8–16 | [S] DigiKey |
| 256 Mbit SDR SDRAM, ISSI IS42S32800J (32-bit) | US$4.11–4.41 @100 (LCSC); JLCPCB up to US$8.30 for the industrial grade | [S] |
| ESP32-P4NRW32 (32 MB PSRAM in package) | US$4.13 @100 | [S] |
| Linux DRAM + eMMC (study 17) | 512 MB DDR3/4 + 4 GB eMMC about US$15–30; 1 GB + 8 GB about US$30–50 at 2026 prices (US$4–10 before the shock) | [S/I] study 17 |

### 6.2 Exposure to 2026 memory price rises

- **Mainstream DRAM.** TrendForce: conventional DRAM contract prices +90–95% QoQ in 1Q26 and +58–63% forecast for 2Q26 [S].
- **Legacy DRAM.** DDR2 +55–60% in 2Q26 and +35–40% in 3Q26 as suppliers exit types below DDR4 [S]. SDR SDRAM is an even older DRAM type, so the same legacy-exit risk applies [I].
- **PSRAM.** AP Memory (愛普, Taiwan) reports record 3Q26 revenue (+122% YoY), driven by strong IoT-RAM demand and **price increases**. IoT RAM is about 71% of its revenue. Its prices are described as "relatively stable compared with standard memory" [S] (TechNews, Fugle and other Taiwanese finance press). So PSRAM is not immune, but it is less volatile than DDR [I].
- Internal SRAM (N6 4.2 MB, RT1170 2 MB, RA8D2 2 MB, E7 13.5 MB) and in-package PSRAM (ESP32-P4) take the memory line out of the BOM entirely, or bundle it into the chip price.

### 6.3 Rough core-BOM comparison for a 7" 1024×600 panel

Excludes LCD, touch, enclosure, power and I/O. All figures [I].

| Item | MCU path (e.g. RT1170 or N6) | MCU path (ESP32-P4) | Linux path (e.g. i.MX93, AM62x, RK3568; study 18) |
| --- | --- | --- | --- |
| Processor | US$12–20 | US$4–5 | US$7–20 for a bare SoC; SoM US$40–60 |
| RAM | 0 (internal) to US$3–5 (32 MB PSRAM or SDRAM) | 0 (in package) | US$15–30 (512 MB–1 GB DDR, 2026) |
| Code/asset storage | 32–128 MB octal/QSPI NOR, about US$2–6 [I, unverified] | 16–32 MB NOR, about US$1–3 | 4–8 GB eMMC; partly counted above |
| PMIC / power tree | simple LDO/buck, about US$1–2 | about US$1 | PMIC or discrete multi-rail, about US$2–5 |
| Ethernet PHY | 10/100 PHY about US$1; GbE PHY about US$2–4 | 10/100 PHY | similar |
| PCB | 4–6 layers (BGA plus SDRAM/PSRAM), about 4 layers with internal SRAM only | 4 layers | 6–8 layers for DDR, or a SoM carrier |
| **Core total** | **about US$18–35** | **about US$8–14** | **about US$35–75 chip-down, or SoM US$45–90** |

### 6.4 Trade-offs not visible in the BOM

- **Boot time.** An MCU HMI shows its first screen in under 1 s, against about 3–8 s for tuned Linux (study 18). This is a visible selling point.
- **Feature ceiling.**
  - No filesystem-heavy features: SQLite alarm and trend history is possible with LittleFS/FatFS on SD, but not at Linux scale.
  - No browser.
  - Limited concurrent TLS sessions.
  - Fonts: CJK fonts in NOR flash are fine, but many languages plus large images push toward 64–128 MB NOR.
- **Engineering cost.**
  - Every driver, protocol and update path is integrated per chip family. On Linux, the same work is mostly packages.
  - Zephyr or a vendor SDK reduces this, but board bring-up per MCU family is about 1–3 person-months.
- **Product model.**
  - EHS's "compile project to firmware" model is natural on an MCU but is not how industrial HMIs work (study 18).
  - An MCU runtime with a data-driven project format is possible. Alternatives include:
    - position-independent screen modules loaded from NOR;
    - an LVGL XML/JSON loader;
    - WAMR or a similar interpreter.
  - All of these cost RAM and flash, and none is as simple as dlopen on Linux. A practical middle ground: the user's "Download" still produces a signed firmware image, pushed by MCUboot over Ethernet in seconds. Gigabit or even 100 Mbit makes this tolerable.

---

## 7. Open questions and suggested experiments

1. Measure LVGL 9.5 `lv_demo_benchmark` and a representative EHS project at 1024×600 and 1280×800 on the STM32H747I-DISCO (existing), H7S78-DK, N6570-DK and MIMXRT1170-EVKB, with and without concurrent lwIP iperf and one TLS session. This is the decisive data, and none of it is public.
2. Confirm in the datasheets: the STM32N6 USB speed (FS vs HS) and security features; the H7S LTDC maximum pixel clock; RA8D2 SDRAM support; PSOC Edge E84 Ethernet.
3. Get ST, NXP and Renesas distributor quotes at 1k and 5k pieces in Taiwan (Arrow, WPG/大聯大, Avnet). Snippet prices above lack dates.
4. Check the live longevity tables: ST per-part dates, NXP RT1170 entry, Renesas RA8.
5. Decide whether the MCU SKU is a "Basic" tier (7" 10/100, H747-class) under a Linux main line, or a stand-alone product line (study 17/18 context).

---

## Sources

### MCU specifications and prices
- ST STM32H7R7/7S7 product page (snippet): https://www.st.com/en/microcontrollers-microprocessors/stm32h7r7-7s7.html
- CNX, STM32H7R/S launch: https://www.cnx-software.com/2024/03/13/600-mhz-stm32h7r-s-cortex-m7-mcu-620kb-sram-64kb-boot-flash-neochrom-gpu/
- CNX, STM32N6 launch: https://www.cnx-software.com/2024/12/13/stmicro-releases-stm32n6-cortex-m55-mcu-series-with-in-house-npu-and-dedicated-computer-vision-pipeline/
- ST eStore STM32N657A0: https://estore.st.com/en/stm32n657a0h3q-cpn.html
- DigiKey STM32N657X0H3Q: https://www.digikey.com/en/products/detail/stmicroelectronics/STM32N657X0H3Q/25675974
- DigiKey UK STM32H7S7L8H6H: https://www.digikey.co.uk/en/products/detail/stmicroelectronics/STM32H7S7L8H6H/22485833
- TME STM32H7S7L8H6H: https://www.tme.eu/en/details/stm32h7s7l8h6h/st-microcontrollers/stmicroelectronics/
- DigiKey STM32H747XIH6: https://www.digikey.com/en/products/detail/stmicroelectronics/STM32H747XIH6/10244387
- DigiKey STM32H743ZIT6: https://www.digikey.com/en/products/detail/stmicroelectronics/STM32H743ZIT6/7809238
- ST STM32U5G9 campaign page: https://st.com/content/st_com/en/campaigns/ultra-low-power-mcu-for-graphic-gui-applications.html
- DigiKey STM32U5G9ZJT6Q: https://www.digikey.de/en/products/detail/stmicroelectronics/STM32U5G9ZJT6Q/21700744
- NXP i.MX RT1170 fact sheet (PDF, read in full): https://www.nxp.com/docs/en/fact-sheet/i.MX-RT1170-FS.pdf
- DigiKey MIMXRT1176DVMAA: https://www.digikey.com/en/products/detail/nxp-usa-inc/MIMXRT1176DVMAA/13536101
- DigiKey MIMXRT1062DVL6B: https://www.digikey.com.mx/en/products/detail/nxp-usa-inc/MIMXRT1062DVL6B/13635843
- CNX, i.MX RT1180: https://www.cnx-software.com/2022/05/04/nxp-i-mx-rt1180-cortex-m7-m33-crossover-mcu-integrates-gbe-tsn-for-industrial-iot-communication/
- Zephyr FRDM-MCXN947 board documentation: https://docs.zephyrproject.org/latest/boards/nxp/frdm_mcxn947/doc/index.html
- DigiKey MCXN947VDFT: https://www.digikey.co.uk/en/products/detail/nxp-usa-inc/MCXN947VDFT/22037698
- Renesas RA8D1 part page: https://www.renesas.com/en/products/ra8d1/part-details/r7fa8d1bhecbd-uc0
- CNX, RA8D1: https://www.cnx-software.com/2023/12/15/renesas-ra8d1-arm-cortex-m85-soc-features-lcd-display-interfaces-2d-graphics-accelerator/
- CNX, RA8D2/RA8M2: https://www.cnx-software.com/2025/10/27/1ghz-renesas-ra8d2-and-ra8m2-cortex-m85-mcus-feature-up-to-1mb-mram-2mb-sram/
- Renesas RA8D2/RA8M2 press release: https://www.renesas.com/en/about/newsroom/renesas-adds-two-new-mcu-groups-blazing-fast-ra8-series-1ghz-performance-and-embedded-mram
- CNX, RA8P1: https://www.cnx-software.com/2025/07/02/renesas-r8p1-a-1-ghz-arm-cortex-m85-microcontroller-with-ethos-u55-npu-for-audio-and-vision-ai-applications/
- CNX, PIC32CZ CA: https://www.cnx-software.com/2023/10/12/microchip-pic32cz-ca-300-mhz-arm-cortex-m7-mcu-hardware-security-module-hsm/
- Alif Ensemble E7 product page: https://alifsemi.com/ensemble-e7-series/
- Mouser Alif Ensemble family: https://www.mouser.in/en/c/semiconductors/embedded-processors-controllers/microcontrollers-mcu/?series=E7
- Espressif ESP32-P4 product page: https://www.espressif.com/en/products/socs/esp32-p4
- LCSC ESP32-P4NRW32: https://www.lcsc.com/product-detail/C22387510.html
- Espressif PCN202600801 (P4 v1.3 EOL, v3.2 migration): https://documentation.espressif.com/en/PCN202600801_ESP32-P4_Chip_Revision_v3.2_Upgrade_Chip_Revision_v1.3_Demand_Collection_and_EOL_Plan_Description.html
- Infineon PSOC Edge E84 product page: https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm/psoc-edge-e84
- CNX, PSOC Edge: https://www.cnx-software.com/2024/04/19/infineon-psoc-edge-e81-e83-e84-cortex-m55-m33-mcu-machine-learning-iot-consumer-industrial-applications/
- CNX, HPMicro HPM6800: https://www.cnx-software.com/2024/03/10/hpmicro-hpm6800-600-mhz-risc-v-mcu-vivante-2-5d-gpu-openvg/

### Development boards
- MIMXRT1170-EVKB (DigiKey UK): https://www.digikey.co.uk/en/products/detail/nxp-usa-inc/MIMXRT1170-EVKB/19528850
- EK-RA8D2 (DigiKey UK): https://www.digikey.co.uk/en/products/detail/renesas-electronics-corporation/RTK7EKA8D2S01001BE/27569005
- FRDM-MCXN947 (Future Electronics): https://futureelectronics.com/p/development-tools--microcontroller-microprocessor--32-bit-eval-board/frdm-mcxn947-nxp-3184836
- STM32U5G9J-DK2 (Future Electronics): https://www.futureelectronics.com/p/development-tools--microcontroller-microprocessor--32-bit-eval-board/stm32u5g9j-dk2-stmicroelectronics-9181492
- STM32H747I-DISCO (DigiKey UK): https://www.digikey.co.uk/en/products/detail/stmicroelectronics/STM32H747I-DISCO/10244393
- CNX, ESP32-P4-Function-EV-Board: https://cnx-software.com/?p=135374

### Longevity
- Espressif longevity commitment: https://www.espressif.com/en/products/longevity-commitment
- NXP, 15-year longevity on more than 100 MCUs: https://community.nxp.com/t5/NXP-Tech-Blog/Longevity-Extended-to-15-Years-on-More-than-100-MCU-Devices/ba-p/1131187
- NXP forum, RT1170 longevity: https://community.nxp.com/t5/i-MX-RT/Product-Longevity-for-RT1170/td-p/1224218
- ST Community, checking STM32 longevity: https://community.st.com/t5/stm32-mcus/how-to-check-stm32-part-numbers-longevity/ta-p/863108

### Networking
- ST Community, 10/100 lwIP throughput: https://community.st.com/t5/stm32-mcus-embedded-software/10-100-ethernet-maximum-throughput-with-lwip/td-p/573506
- FreeRTOS forum, TCP performance: https://forums.freertos.org/t/tcp-performance/13053
- ST Community, H747 NetX Duo iperf: https://community.st.com/t5/stm32-mcus-embedded-software/stm32h747-discro-netxduo-iperf-tcp-only-has-55mbits-but-udp-have/td-p/126507
- Express Logic NetX near wire speed: https://www.newelectronics.co.uk/content/news/netx-achieves-near-wire-speed-on-stm32-platform
- Zephyr discussion #72760 (RT1170 zperf): https://github.com/zephyrproject-rtos/zephyr/discussions/72760
- NXP Community, RT1170 GbE speed: https://community.nxp.com/t5/MCUXpresso-SDK/rt1170evkb-1G-ethernet-speed/td-p/1931972
- NXP Community, RT1170 slow GbE: https://community.nxp.com/t5/i-MX-RT/iMXRT-1170-Slow-Gigabit-Ethernet/td-p/1507077
- Embedded TLS/DTLS guide: https://applied-ee.github.io/embedded/docs/networking/protocol-selection/embedded-tls-dtls/
- IETF 92 LWIG slides: https://datatracker.ietf.org/meeting/92/materials/slides-92-lwig-3.pdf
- lwIP mailing list, TLS handshake time: https://lists.nongnu.org/archive/html/lwip-users/2017-06/msg00015.html
- wolfSSL, STM32H7S support: https://www.wolfssl.com/announcing-stm32h7s-support-in-wolfcrypt/
- wolfSSL STM32Cube IDE notes: https://gogs.librecmc.org/OWEALs/wolfssl/src/master/IDE/STM32Cube?lang=fa-IR
- Mbed forum, handshake allocation: https://forums.mbed.com/t/allocation-requirements-during-handshake/4610
- open62541 building documentation: https://open62541.org/doc/master/building.html
- open62541 issue #1790 (heap usage): https://github.com/open62541/open62541/issues/1790
- open62541 group, memory footprint: https://groups.google.com/d/topic/open62541/X18XqyTC70Q
- open62541 group, H7S3 bare metal: https://groups.google.com/g/open62541/c/eOo9n78MmtM
- open62541 ESP32 port: https://www.github.com/Pro/open62541-esp32
- o6 Embedded: https://www.o6-automation.com/o6-embedded
- SEGGER emVNC: https://segger.com/user-interface/emvnc/
- LVGL forum, VNC: https://forum.lvgl.io/t/lvgl-vnc-how-to-achieve/2600
- ST Community, H757 VNC: https://community.st.com/stm32-mcus-touchgfx-and-gui-33/hi-i-m-using-a-stm32h757-mcu-with-a-graphical-display-on-my-new-board-i-would-like-to-insert-on-it-a-vnc-server-using-touchgfx-tool-is-it-possible-thanks-6144
- ESP-Hosted MCU-host iperf example: https://components.espressif.com/components/espressif/esp_hosted/versions/3.1.0/examples/network_split/iperf/mcu_host
- ESP32-P4 + C6 Wi-Fi test repo: https://github.com/r4d10n/esp32p4-c6-wifi-test
- ESP-Hosted issue #367: https://github.com/espressif/esp-hosted/issues/367

### LVGL performance
- LVGL forum, benchmark results thread: https://forum.lvgl.io/t/whats-your-benchmark-result/12073
- LVGL forum, H7S7 port: https://forum.lvgl.io/t/stm32h7s7-lvgl-port/18538
- LVGL port for STM32H7B3I-DISCO: https://github.com/lvgl/lv_port_stm32h7b3i_disco
- LVGL forum, STM32U5 NemaGFX results: https://forum.lvgl.io/t/stm32u5-nema-gfx-expected-benchmark-results/18865
- LVGL forum, STM32N6 Helium: https://forum.lvgl.io/t/enabling-helium-acceleration-on-stm32n6-cm55/20489
- NXP Community, fastest LVGL on RT1176: https://community.nxp.com/t5/i-MX-RT/Need-fastest-possible-LVGL-graphics-VGLite-or-PXP-on-iMX-RT1176/m-p/1662826
- NXP knowledge article, LVGL benchmark optimisation: https://community.nxp.com/t5/i-MX-RT-Crossover-MCUs-Knowledge/LVGL-Benchmark-Performance-Optimization/ta-p/2252208
- LVGL showcase, EK-RA8D1: https://lvgl.io/blog/showcase-renesas-ek-ra8d1
- Espressif LCD application note: https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/advanced-development/lcd-application-note/overview.html
- Espressif blog, LVGL + PPA: https://preview-developer.espressif.com/pr597/blog/2025/11/lvgl-plus-pixel-processing-accelerator
- ST graphics solutions deck: https://www.st.com/resource/en/product_presentation/stm32-graphics-solutions-detailed.pdf

### Commercial HMIs
- Siemens KTP Basic datasheet: https://www.mouser.com/datasheet/2/1412/6AG11232GB032AX0_datasheet_en-3271057.pdf
- AutomationDirect EA3 overview: https://cdn.automationdirect.com/static/specs/ea3overview.pdf
- Weintek MT8071iP datasheet: https://www.rusavtomatika.com/manuals/MT8071iP.pdf
- Unipi, Weintek MT8051iP: https://unipi.technology/weintek-mt8051ip-p198
- RS, Red Lion CR1000: https://uk.rs-online.com/web/p/hmi-displays/2006440
- Riverdi STM32 Embedded Display: https://riverdi.com/?p=26943
- DigiKey UK, Riverdi SM-RVT101HVSFWN00: https://www.digikey.co.uk/en/products/detail/riverdi/SM-RVT101HVSFWN00/25855125
- CNX, Makerfabs MaTouch ESP32-P4: https://www.cnx-software.com/2026/06/26/makerfabs-matouch-esp32-p4-10-1-inch-hmi-display-features-4g-lte-and-ethernet-connectivity-a-2mp-camera/
- Hackster, ESP32-P4-WIFI6 HMI: https://www.hackster.io/news/bigger-faster-better-the-esp32-p4-wifi6-hmi-is-here-d088fda7f0c0
- Evelta, DWIN 7" display: https://evelta.com/7-0-inch-800x480-rs232-hmi-lcd-display-capacitive-touch-flash-16mb-uart-industrial-grade
- Qt, i.MX RT1170 HMI: https://www.qt.io/resources/documents/driving-down-automotive-costs-for-richer-hmis-with-qt-i-mx-rt1170
- NXP, Crank Storyboard on RT1170: https://www.nxp.com/company/about-nxp/smarter-world-videos/CRANK-SW-RT1170-VID

### OTA and secure boot
- ST wiki, STiRoT for STM32H7S: https://wiki.st.com/stm32mcu/wiki/Security:STiRoT_for_STM32H7S
- ST wiki, OEMiRoT/OEMuRoT for STM32H7S: https://wiki.st.com/stm32mcu/wiki/Security:OEMiRoT_OEMuRoT_for_STM32H7S
- ST wiki, SBSFU legacy: https://wiki.st.com/stm32mcu/wiki/Security:SBSFU_legacy
- ST Community, SBSFU vs MCUboot vs STiRoT: https://community.st.com/t5/stm32-mcus-security/clarification-needed-sbsfu-legacy-mcuboot-and-stirot/m-p/747513
- MCUXpresso SDK mcuboot_opensource example: https://docs.mcuxpresso.nxp.com/mcuxsdk/24.12.00/html/examples/ota_examples/mcuboot_opensource/readme.html
- NXP Community, RT1170-EVKB OTA example: https://community.nxp.com/t5/i-MX-RT-Crossover-MCUs/MIMXRT1170-EVKB-OTA-Update-Example/m-p/1882842

### Memory prices
- LCSC APS256XXN-OBR-WA: https://www.lcsc.com/product-detail/C42963059.html
- LCSC IS42S32800J-7BL: https://lcsc.com/product-detail/SDRAM_ISSI-Integrated-Silicon-Solution-IS42S32800J-7BL_C1349284.html
- iConnect007, DDR2 prices in 3Q26: https://iconnect007.com/article/150498/dram-shortages-hit-ddr2-prices-set-to-keep-rising-in-3q26/150495/smt
- Fortune, 2026 memory shortage: https://fortune.com/2026/02/15/ai-demand-memory-chip-shortage-crisis-dram-hbm-micron-skhynix-samsung
- TechNews, AP Memory September 2026 revenue: https://finance.technews.tw/2026/10/05/ap-memory-6531-202609-financial-report/
- Fugle, AP Memory earnings call 2026-11-10: https://blog.fugle.tw/post/earnings-call-6531-2026-11-10
- Study 17 cost model (DRAM/eMMC 2026): `../17-novakon-hardware-model/cost_estimation_and_model.md`
