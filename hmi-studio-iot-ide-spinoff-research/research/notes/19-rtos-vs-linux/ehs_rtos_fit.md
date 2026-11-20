# EHS on an RTOS: networked industrial HMI without Linux, compared with the Linux path

Notes date: 2026-11-20. Scope: how EDT HMI Studio (EHS, local repo `edt-hmi-studio`, 0.9.2-dev) would extend to a networked industrial HMI that runs an RTOS instead of Linux, on (A) a high-end MCU such as STM32H7 with Ethernet, or (B) a Cortex-A part running an RTOS (Renesas RZ/A, AMD Zynq). Zephyr is assessed as a cross-cutting option. The baseline is the Linux path in study 18 (`../18-mpu-runtime/ehs_port_to_mpu.md`, MVP 9–14 person-months).

Method: read-only reading of the EHS repo (paths relative to its root), studies 09, 11 and 18 in this folder, and web searches. **Everything labelled "Inference" is my judgement, not a measured or quoted fact.** "Second-hand" marks forum posts, aggregators and trade press. Effort figures assume the same engineer profile as study 18 (one experienced embedded engineer, full time, with Claude Code).

Legal reminder (study 01, repeated in study 18): the EULA limits the runtime to EDT-supplied hardware. Nothing here changes that.

---

## 0. Short answer

1. **The EHS runtime is already shaped for an RTOS.** The board loop is a bare-metal super loop (`firmware/stm32h747i-disco/src/main.c:48-54`), `lv_conf.h` sets `LV_USE_OS LV_OS_NONE` on all three boards, but the runtime below it is non-blocking by design: transports are "poll and drain" (`firmware/common/include/hmi_transport.h:33-37`), protocol drivers are start/poll/cancel state machines (`firmware/common/include/hmi_driver.h:41-59`), and the platform layer is one function (`hmi_platform.h:16`). Dropping that loop into one FreeRTOS task, with LVGL in another, is small work. The repo's own docs already plan Ethernet as lwIP `NO_SYS` polled from the super loop on the H747 (`docs/protocol-connections.md:807`, spikes R3/R4 at `:836-837`), and note that the UI-thread rule carries to "a future RTOS port" (`docs/hmi-themes.md:1633`).
2. **Path A (STM32H7-class + FreeRTOS + lwIP) is not much cheaper to build than Linux; it is cheaper to make.** MVP about **8–13 PM** stand-alone (Inference), against 9–14 PM for Linux. If the Linux path (or this one) is built first with a portable runtime core, the second target costs about **5–8 PM** more, because the typed tag database, PLC drivers written against `hmi_driver.h`, alarms logic and designer changes are shared. What does not get cheaper on an MCU: OTA plus secure boot, project loading without reflashing, and fitting TLS, several sockets and LVGL into on-chip RAM.
3. **Project loading without reflashing on an MCU should be a split image:** a fixed, signed runtime (RTOS, lwIP, LVGL, drivers) plus a project slot in QSPI holding (i) data tables and assets and (ii) the generated UI/Logic code compiled on the PC against a versioned jump table exported by the runtime. This is study 18's option (c) adapted to an MCU, and keeps study 11's split of "hot-reloadable layer" vs "fixed layer". A device-side interpreter (option b) or WASM (option d) is worse on an MCU than on Linux.
4. **Path B (Cortex-A + RTOS) adds about 3–6 PM per SoC family on top of path A** for MMU/cache setup, vendor SDK, display and 2D engine drivers, boot chain and an own A/B boot loader. It pays only where sub-second boot or a DRAM-in-package BOM really matters. Check the part: the **RZ/A3M has no Ethernet MAC** listed (CNX); the RZ/A3UL has two Gigabit Ethernet ports but needs external DDR for HD resolution.
5. **Zephyr is the strongest single RTOS for networking and CRA paperwork** (native IP stack, TLS via mbedTLS, MQTT, HTTP/1.1 and HTTP/2 server, LwM2M, Modbus RTU and TCP helpers, MCUboot, LLEXT loadable modules, `west spdx` SBOMs, a PSIRT and CNA since 2017, and runs on i.MX93 Cortex-A55 too). But EHS is built on STM32Cube HAL bare-metal, ST moved Cube back to **FreeRTOS + lwIP as its main stack in May 2025**, and STM32 under Zephyr is maintained by ST as a silver member with uneven board coverage. Recommendation (Inference): FreeRTOS + lwIP for the first STM32 networked product; keep Zephyr as the planned second platform, and as the way to put an RTOS HMI on a Cortex-A55.
6. **Product mapping (Inference):** MCU + RTOS for a low-cost networked "Lite" line (4.3"–7", Modbus RTU/TCP plus one or two PLC families, MQTT, web remote); Linux for the main line (7"–15", many PLC drivers, OPC UA, VNC, SQLite history, process isolation); Cortex-A + RTOS only on customer demand. **One designer and one code generator can target all three** if four things are true: one portable runtime core in C with platform layers, project data separated from code on every target, a capability profile per target in the designer, and one pinned LVGL version.
7. **Security:** the brief's premise needs a correction. AMNESIA:33 (2020) did **not** hit FreeRTOS+TCP or lwIP; Forescout analysed lwIP and found nothing. The 2018 FreeRTOS+TCP disclosure was Zimperium's (13 CVEs). NAME:WRECK (2021) hit **NetX** (ThreadX's stack), FreeBSD, Nucleus NET and IPnet. FreeRTOS+TCP has kept collecting CVEs in DNS and IPv6 code (2024–2026); lwIP has 2026 CVEs in SNMPv3 and mDNS that are fixed in git but not in a release. Under the CRA an RTOS HMI is one signed binary, so every stack fix is a full firmware update that must be shipped, and actively exploited vulnerabilities must be reported to ENISA within 24 hours since 11 September 2026.

---

## 1. What the EHS code says about an RTOS port (verified in the repo)

| Fact | Where | Why it matters |
| --- | --- | --- |
| Board entry is a super loop: `for (;;) { lv_timer_handler(); hmi_memory_watch(); hmi_runtime_task(); HAL_Delay(1U); }` | `firmware/stm32h747i-disco/src/main.c:48-54`; same shape in `stm32f746g-disco/src/main.c:44-48`, `edt-evk043027b/src/main.c:57-61` | Each line becomes a task body. Nothing in `hmi_runtime_task` blocks. |
| `hmi_runtime_init(transport, config, tag_descriptors, n, binding_descriptors, n)` is called once with generated tables | `stm32h747i-disco/src/main.c:37-46`; `hmi_runtime.c:3209`; header `hmi_runtime.h:762` | The tables are already data-shaped; moving them to a project blob is the same step study 18 proposes for Linux. |
| No OS in LVGL config: `LV_USE_OS LV_OS_NONE` | `stm32f746g-disco/include/lv_conf.h:45`, `stm32h747i-disco/include/lv_conf.h:47`, `edt-evk043027b/include/lv_conf.h:50` | Switch to `LV_OS_FREERTOS`; LVGL then takes its own mutex inside `lv_timer_handler()` and other tasks must bracket LVGL calls with `lv_lock()/lv_unlock()` (LVGL docs). |
| LVGL heap 4 MB on F746/H747, 1 MB on the EVK; FreeType and TinyTTF off | `lv_conf.h` lines 39/41/44 and 224/220/221 | The 4 MB heap sits in external SDRAM (H747: FMC SDRAM bank 2, 32 MB, `stm32h747i-disco/src/board.c:87`). An MCU HMI still uses DRAM-class memory, just small SDR SDRAM. |
| Platform layer = one function `hmi_platform_tick_ms` | `firmware/common/include/hmi_platform.h:16`; implementation 8 lines (`src/hmi_platform.c`) | Maps to `xTaskGetTickCount()` or `k_uptime_get_32()`. |
| Transport = 8 function pointers, "poll and drain", comment already says "later a socket" | `hmi_transport.h:26-37`, ops at `:67-104` | A non-blocking lwIP socket (or raw-API pcb with a ring buffer) fits without changing the client. |
| Protocol driver = `start_read / start_write / poll / cancel / reply_has` | `hmi_driver.h:41-59` | The right shape for PLC drivers that must run on MCU *and* Linux: write them once in C against this interface. |
| Tag values are `float` only | `hmi_runtime.h:807-808`, `hmi_runtime.c:4784`, `:4814` | Same redesign as study 18 §1.3; on an MCU the typed variant must stay small. |
| Static, fixed-size runtime state (`g_tag_text[...]`, `g_watches[...]`) | `hmi_runtime.c:70`, `:98` | Good for an RTOS: no heap churn, bounded RAM. |
| H747 flash and RAM already busy: about 74 % of flash bank 1, 55 % of the 512 KB AXI SRAM | `docs/stm32h747i-disco-dual-core.md:93-100` | lwIP + TLS + MQTT + MCUboot will not fit in bank 1 beside today's image. Needs bank 2, XIP from QSPI, or a bigger part. |
| Ethernet on H747: LAN8742 PHY present, `HAL_ETH_MODULE_ENABLED`, **no lwIP in the tree**; planned as lwIP `NO_SYS` polled from the super loop, DMA descriptors in non-cached SRAM ("the known H7 trap") | `docs/protocol-connections.md:807`; "link only what is enabled" rule at `:1453` | EHS has already chosen the first, RTOS-less step. |
| Spikes R3 (Ethernet bring-up), R4 (Modbus TCP via an MBAP framer on a socket), R8 (Packet over TCP) | `docs/protocol-connections.md:836-841` | Modbus TCP reuses the Modbus driver over a new framer (`hmi_framer.h`, `:1445-1449`). |
| EVK043027B (STM32U599) has no Ethernet MAC | `docs/protocol-connections.md` §8 text after the table | Path A needs a different board (H7 or N6 class). |
| Vendor EVK code uses FreeRTOS (TouchGFX app, `edt_bsp_lcd.c` FreeRTOS task) but EHS deliberately did not vendor it | `docs/edt-evk043027b.md:220`; `firmware/edt-evk043027b/include/main.h:7`, `:57`; `docs/display-sleep.md:509` | EDT's firmware department already knows FreeRTOS; EHS chose bare metal for simplicity. |
| Threading rule for an RTOS port already written: requests are one atomic byte, apply runs on the UI thread | `docs/hmi-themes.md:1633-1634` | Same pattern as study 18 §1.4. |
| Images and fonts linked into fixed QSPI sections at `0x90000000` | `docs/images-external-flash.md:41`, `:227` | For project loading without reflash, assets must move from linker sections to an indexed blob. |
| Zephyr already referenced as prior art (west manifest, `BUILD_OUTPUT_META`, NVS) | `docs/project-storage-architecture.md:348`, `:360`, `:453`; `docs/hmi-memory-and-retention.md:249` | The team has looked at Zephyr's tooling ideas, not adopted Zephyr. |

**Conclusion of §1 (Inference):** the hard part of an RTOS port is not threading; the runtime is already a cooperative scheduler. The hard parts are memory budget, network-stack robustness, secure update, and the move from "one firmware per project" to "fixed firmware + project".

---

## 2. Path A: high-end MCU + RTOS (STM32H7-class with Ethernet)

### 2.1 Choice of RTOS and stack on STM32

| Option | For | Against |
| --- | --- | --- |
| **Bare metal + lwIP `NO_SYS`** (EHS's current plan) | No RTOS; matches today's loop; enough for Modbus TCP client/server and a simple HTTP status page | TLS handshakes, MQTT keep-alive, OTA download and several PLC links in one loop make UI jitter likely (Inference). Stop here only if the product never needs TLS. |
| **FreeRTOS + lwIP (sockets) via STM32Cube** | ST said in May 2025 it is "moving back to FreeRTOS as the main kernel", with USBX, FileX and lwIP (ST Community). Largest pool of examples and engineers. FreeRTOS LTS 202604 (kernel 11.3.0, coreMQTT 5.0.2 with MQTT 5) with two years of fixes; paid EMP extends LTS up to 10 years | lwIP has no LTS and slow releases (2.2.0 in 2023-09, 2.2.1 on 2025-02-06); fixes sit in git. H7 Ethernet + cache + MPU setup is a known source of bugs (ST Community threads). |
| FreeRTOS + FreeRTOS+TCP | Same kernel vendor, coreMQTT/coreHTTP integration, STM32H7 network driver exists | Not ST's default in Cube; steady CVE flow in DNS/IPv6 code (§5). |
| ThreadX + NetX Duo (Eclipse ThreadX, MIT) | Integrated TCP/IP, TLS, MQTT, HTTP; ST packages exist (X-CUBE-AZRTOS-H7 6.4.0) | ST is no longer making it the main kernel; Eclipse ThreadX 6.4.2 (2025-02) but certified versions lag (second-hand); NetX was in NAME:WRECK. Not recommended for a new line (Inference). |
| Zephyr | See §4 | See §4 |

Recommendation (Inference): **FreeRTOS + lwIP from STM32Cube**, IPv6 off, mDNS/SNMP/LLMNR off, mbedTLS for TLS, coreMQTT (or lwIP's MQTT app) for MQTT. This keeps EHS on Cube HAL, which all current board files use.

### 2.2 Task layout (Inference)

| Task | Priority | Content |
| --- | --- | --- |
| `ui` | normal | `lv_timer_handler()`, generated `ui_logic.c`, binding updates; drains a queue of tag changes. Only task that calls LVGL without `lv_lock`. |
| `runtime` | above normal | `hmi_runtime_task()` as today: scheduler, cache, staleness, write queue, drivers' `poll`. One task can run all links because drivers are non-blocking; split per link only if a driver blocks. |
| `tcpip` | high | lwIP core thread (`tcpip_thread`) |
| `net_app` | below normal | MQTT client, TLS sessions, HTTP server for web remote, OTA download, project download |
| `storage` | low | Alarm/trend log writer to SD (FatFs) or a flash ring buffer; retained values |
| idle hook | — | Watchdog kick only when all tasks have checked in |

Memory budget questions to answer by measurement (open): lwIP pools and Ethernet DMA descriptors in non-cacheable SRAM; mbedTLS heap for one certificate-based TLS 1.3 session, planning figure about 40–50 KB (unmeasured; Espressif reports about 42 KB heap for an HTTPS client); per-task stacks; LVGL draw buffers; open62541 if ever wanted (no reliable RAM figure found).

### 2.3 Project loading without reflashing (relation to studies 11 and 18)

Study 11 split a project into a hot-reloadable layer (screens, rules, tags, unit parameters, communication mapping) and a fixed layer (drivers, unit tasks, bus, RTOS). Study 18 recommended, for Linux, option (c): fixed runtime + precompiled project module + data file. On an MCU the same options look like this:

| | (a) Per-project firmware (today), delivered by OTA | (b) Fixed firmware + data-driven project (screens from data, Logic by bytecode VM) | **(c-MCU) Split image: fixed runtime + project slot with code module + data** | (d) WASM project under WAMR |
| --- | --- | --- | --- | --- |
| How | Designer builds the whole image as today; boot loader (MCUboot) swaps it in | Grow `wasm/src/ui_from_json.c` into a device loader; Logic via VM (study 09/11) | Runtime image exports a versioned function table (LVGL + `hmi_*` API) at a fixed address. Designer compiles generated `ui*.c` on the PC, linked at the project slot address in QSPI (XIP), calling the runtime only through the table. Tags, devices, bindings, alarms, assets go into a data section of the same blob | Same `.wasm` as the browser Emulator, AOT-compiled for Cortex-M |
| Download size / time | Whole image (about 0.8 MB today on H747) plus assets; tens of seconds over Ethernet plus internal flash write (Inference) | Small; seconds | Project code tens to hundreds of KB plus assets; seconds to write QSPI (Inference) | Medium |
| Drift vs Emulator | None | High (two engines; rejected in `docs/preview-ladder.md` §8 per study 18) | None (same generated C) | None |
| Custom C blocks | Work | Do not | Work | Work, sandboxed |
| Runtime update independent of project | No | Yes | Yes, with ABI check; designer recompiles from the `.ehsp` embedded in the blob | Yes |
| Risks on MCU | Every project download is a firmware update: secure boot covers it for free | Device-side loader and VM cost RAM and CPU; LVGL XML runtime loading licence issues (study 18) | QSPI cannot be written while XIP-executing from the same chip (study 11 §4): write the inactive A/B project slot, then restart the UI (or the MCU). ABI discipline needed. Signature check of the project slot needed | LVGL renders in software inside WASM; on a Cortex-M7 this is likely too slow and RAM-hungry (Inference) |

Recommendation (Inference): **start with (a) for the MVP** — it needs nothing new beyond the OTA/secure boot work that path A must do anyway, and project download over Ethernet replaces ST-LINK. **Move to (c-MCU) in the product phase**, because it gives the same user experience as Linux runtimes (download in seconds, runtime updated separately, "upload project from HMI"). Zephyr's LLEXT (Apache-2.0; ELF extensions loaded at run time on ARM, RISC-V, Xtensa) is a ready-made loader for (c-MCU) if the Zephyr route is taken; on FreeRTOS a fixed-address link plus jump table avoids writing an ELF loader.

Asset handling must change in both (a)-with-separate-assets and (c): images and fonts are linker sections at `0x90000000` today (`docs/images-external-flash.md:41`); they need an index table in the blob so the runtime resolves them at load time.

### 2.4 Other building blocks

| Need | MCU choice (Inference unless cited) |
| --- | --- |
| Ethernet driver | STM32Cube `ETH` HAL + lwIP `ethernetif.c` from CubeMX; LAN8742 component; DMA descriptors and Rx buffers in a non-cacheable MPU region (`docs/protocol-connections.md:807`) |
| Modbus TCP | Own MBAP framer over the existing driver (R4 in EHS docs); client and server |
| MQTT | coreMQTT (MIT; v5.0.2 in FreeRTOS 202604 LTS) or lwIP MQTT app; Sparkplug B later |
| TLS | mbedTLS (Apache-2.0) with hardware RNG; SNTP for certificate time (coreSNTP 2.0.0 adds year-2038 readiness) |
| OTA + secure boot | Classic STM32H7: X-CUBE-SBSFU is legacy with "no longer evolutions"; ST's MCUboot-based OEMiRoT/OEMuRoT ships for STM32H5 and **STM32H7R/S** (ST wiki). So either pick H7R/S (boot flash + external flash XIP, matches the split-image model) or port MCUboot to classic H7 on FreeRTOS yourself. Check which H7R/S parts have Ethernet. |
| PLC drivers in C | Own minimal clients: Mitsubishi MC/SLMP, Omron FINS, Siemens S7 PUT/GET (ISO-on-TCP), EtherNet/IP explicit messaging. Libraries used on Linux (snap7, libplctag) assume threads, heap and sockets in ways that fit badly on an MCU (Inference); write against `hmi_driver.h` so the same C runs on Linux. |
| USB stick import | H7 USB OTG host + MSC class + FatFs (Cube USB Host or USBX standalone) |
| Remote view | Option 1: minimal VNC/RFB server fed from LVGL flush areas (raw/hextile; 800x480x16 bpp is 768 KB per full frame, fine with dirty rectangles on 100 Mbit/s, Inference). Option 2 (differentiator): device serves the project's WASM Emulator build plus a WebSocket tag feed; the browser renders the HMI. Needs the WASM in QSPI or served from the PC/cloud. |
| Storage | Alarm/trend history as a ring log in QSPI or on SD (FatFs); no SQLite on the MCU (Inference). Retained values as today (`docs/hmi-memory-and-retention.md`). |

### 2.5 Effort estimate, path A (all Inference)

Same engineer profile as study 18 §6. "Shared" marks work that is the same code on Linux if written portably.

| # | Task | PM (low–high) | Shared with Linux? | Comment |
| --- | --- | --- | --- | --- |
| A1 | FreeRTOS integration: CubeMX/CMSIS-RTOS2, task split (§2.2), `LV_OS_FREERTOS`, `lv_lock`, tag-change queue, stack sizing, watchdog, fault capture | 0.5–1 | No | Runtime is already non-blocking |
| A2 | Ethernet + lwIP on H7: descriptors, cache/MPU, PHY, DHCP + static fallback, link events, cable-pull and broadcast-storm robustness | 0.5–1 | No | EHS spike R3 is the first third of this |
| A3 | Modbus TCP client/server via MBAP framer | 0.25–0.5 | Yes (framer/driver) | EHS spike R4 |
| A4 | Typed, multi-link, multi-device tag database; data tables out of C | 1–2 | Yes | Same as study 18 row 3, MCU-sized variant |
| A5 | Designer: RTOS-MCU target, capability profile, network discovery and download, remove ST-LINK from main path | 0.5–1 | Mostly | Overlaps study 18 row 4 |
| A6 | TLS (mbedTLS) + MQTT (coreMQTT) + SNTP + certificate/key provisioning | 1–1.5 | Partly (MQTT topic model) | |
| A7 | OTA + secure boot: MCUboot (or OEMiRoT on H7R/S), A/B slots, signing in the designer, rollback, anti-rollback, key provisioning at factory | 1.5–2.5 | Signing/tooling only | Linux uses RAUC instead |
| A8 | Project loading without reflash: (a) for MVP is covered by A7; (c-MCU) split image, ABI table, asset index, A/B project slot, hot UI restart | 0.25 (a) / 1.5–3 (c) | ABI ideas only | |
| A9 | PLC drivers in C: MC/SLMP 0.5–1, FINS 0.5–0.75, S7 PUT/GET 0.75–1.5, EtherNet/IP explicit client 0.75–1.5 | 2.5–4.75 | **Yes** if written to `hmi_driver.h` | Code only; lab testing in A12 |
| A10 | Alarms, trend logging, recipes, users on MCU (reduced: ring-log storage, no SQL) | 2–3.5 | Logic yes, storage no | |
| A11 | Remote: VNC-lite 0.75–1.5, or web remote (WASM HMI + WebSocket tags) 1–2 | 0.75–2 | Web remote yes | |
| A12 | Test: PLC simulators in CI, network fuzzing of exposed parsers, soak, power-cut, real-PLC interop | 1.5–2.5 | Partly | Calendar-bound, as study 18 |
| A13 | CRA: SBOM for Cube + middleware (manual or script; no Yocto tooling), vulnerability monitoring, documentation | 0.5–1 | Process yes | |
| A14 | USB host stick import | 0.5–1 | No | |
| | **MVP** (A1–A5, A6 partial, A7, A8(a), one PLC family from A9, basic alarms from A10, part of A12, A13) | **8–13** | | Stand-alone |
| | **MVP as second target** after Linux (shared rows already done) | **5–8** | | |
| | **Full product** (all rows, A8(c)) | **15–26** | | vs 22–38 for Linux full; no OPC UA, no SQLite history, narrower driver list |

Same long poles as study 18: real-PLC interoperability, certification (CE/UKCA, FCC, UL), and hardware validation. On the MCU path add RAM tuning and EMC of a fast parallel/DSI display, which are calendar-bound.

---

## 3. Path B: Cortex-A + RTOS

### 3.1 Candidates

| Part | Facts found | Fit |
| --- | --- | --- |
| **Renesas RZ/A3M** | 1x Cortex-A55 1 GHz with NEON; **128 MB DDR3L in package**; 128 KB SRAM with ECC; MIPI-DSI 4-lane or parallel RGB up to 1280x800; 2D drawing engine; 1x USB 2.0 host; boot from QSPI NOR/NAND; FSP with FreeRTOS or Azure RTOS; LVGL listed as GUI partner; about US$13.94 at 500 units; EK-RZ/A3M about US$285 (Renesas, CNX). **No Ethernet listed** (CNX). | Excellent BOM and memory-price story (DRAM inside a fixed-price chip), but not for a networked HMI unless Ethernet comes via an external SPI/USB MAC (Inference) |
| **Renesas RZ/A3UL** | 1x Cortex-A55 1 GHz; 2x Gigabit Ethernet; optional external DDR3L-1333/DDR4-1600 16-bit; FreeRTOS and Azure RTOS; Renesas Confluence says up to 1280x720 with DDR, VGA with OctoRAM only (sources conflict with 1280x800 in CNX) | Networked RTOS HMI candidate; external DDR brings the memory-price exposure back |
| Renesas RZ/A2M | Older Cortex-A9 with large on-chip SRAM and two Ethernet ports (general knowledge, not re-verified) | Legacy designs |
| **AMD Zynq-7000** | Dual Cortex-A9 + FPGA; Vitis standalone and FreeRTOS BSPs with lwIP; FreeRTOS has an official Zynq-7000 port; LVGL only via community templates (second-hand) | Only when the FPGA is needed anyway (custom I/O, motion) |
| NXP i.MX93 A55 under Zephyr | Zephyr board `imx93_evk/mimx9352/a55` (also FRDM-IMX93 since Zephyr 4.1, Variscite DART/VAR-SOM, phyBOARD-Nash); boot via U-Boot, J-Link or remoteproc; cold reboot only | The only mainstream Cortex-A55 + open RTOS path with a full net stack; same SoM family as the Linux shortlist in study 18 |

### 3.2 Extra work vs path A (Inference)

| # | Extra task | PM | Why |
| --- | --- | --- | --- |
| B1 | Vendor SDK and boot chain: FSP (e² studio) or Vitis; boot loader stages from QSPI; DDR init when external; clocks; GIC interrupts | 0.75–1.5 | New vendor, new tools; less community help than STM32 |
| B2 | MMU and caches: page tables, cacheable/non-cacheable regions, L1/L2 maintenance around DMA (Ethernet, display, 2D engine, SD), SMP if two cores | 0.5–1 | The H7 cache trap, but larger |
| B3 | Display, 2D engine and touch drivers into LVGL (flush, vsync, draw unit for the vendor 2D engine; check whether LVGL already has a draw unit for it) | 0.75–1.5 | LVGL software rendering with NEON is the fallback |
| B4 | Ethernet driver + lwIP port on the vendor SDK (FSP/Vitis provide drivers; robustness work as A2) | 0.25–0.5 | Mostly integration |
| B5 | Own A/B boot and signature check (no ST-style MCUboot package for these parts found; MCUboot has no out-of-the-box RZ/A or Zynq port as far as I found) | 0.75–1.5 | CRA needs signed updates |
| B6 | USB host, SD/eMMC filesystem on the vendor stack | 0.25–0.5 | |
| | **Extra over path A, per SoC family** | **3–6** | And a second toolchain to maintain |

What path B buys: about 2x CPU of an H7 plus NEON, 1280x800 with a 2D engine, and boot to first screen in about a second (Renesas markets "quick startup"; no measured figure found). What it does not buy compared with Linux on the same class of chip: process isolation, ready drivers (OPC UA, VNC, SQLite, Wi-Fi supplicant), Yocto SBOM tooling. Recommendation (Inference): **do not start a path B line**. Build it only for a named customer who needs sub-second boot at 1280x800, or a DRAM-free BOM, and then prefer Zephyr on i.MX93 A55 (same SoM as the Linux line) or RZ/A3UL.

---

## 4. Zephyr as a cross-cutting option

### 4.1 What Zephyr provides for EHS

| Area | Zephyr | Source |
| --- | --- | --- |
| Licence | Apache-2.0 | — |
| Networking | Native IPv4/IPv6 stack, TCP client/server, sockets; MQTT and MQTT-SN; HTTP client (1.1) and server (1.1 and HTTP/2); LwM2M 1.0.2 (1.1.1 by Kconfig); CoAP; TLS/DTLS sockets on mbedTLS | Zephyr networking docs |
| Modbus | RTU and ASCII client/server; TCP is "helper functions", with a Modbus TCP server sample and a TCP-to-serial gateway sample; raw ADU callbacks | Zephyr Modbus docs |
| GUI | LVGL as a Zephyr module; Zephyr 4.2 moved it to LVGL 9.3 with multi-display support. EHS pins **9.5.0** (study 18) — EHS would need its own LVGL module revision in the west manifest | Zephyr 4.2 release notes |
| Boot/update | MCUboot first-class (sysbuild), image signing in the build | — |
| Loadable code | LLEXT: load, link, unload ELF extensions at run time (ARM, RISC-V, Xtensa) | study 11 |
| Bus | zbus (study 09 proposed it as the bus implementation on Zephyr targets) | study 09 §8 |
| Storage | NVS/ZMS, FatFs, LittleFS (EHS already studied NVS: `docs/hmi-memory-and-retention.md:249`) | — |
| Isolation | Userspace threads with MPU/MMU memory domains (stronger than FreeRTOS-MPU; weaker than Linux processes) | general knowledge |
| Hardware | 220+ STM32 boards and 50+ STM32 peripherals supported, built on STM32Cube HAL/LL, nightly tests by ST (ST JP page); i.MX93 Cortex-A55 boards (§3.1) | ST, Zephyr docs |
| Releases | 4.4.0 released 2026-04-14 (EOL 2027-04-12); upstream LTS still 3.7 (maintained to January 2027); Canonical announced a commercial "Zephyr 26.04 LTS" with up to 15 years of support (announcement dated 2026-09-21, one copy differs) | Zephyr releases page, Canonical |
| CRA/SBOM | `west spdx` SBOM (SPDX 2.3; SPDX 3 work in progress); PSIRT, CNA since 2017; targets reporter feedback in 7 days, manufacturer notification and fix in 30 days, disclosure at most 90 days; project is working on its Article 24 steward obligations | Zephyr CRA page |

### 4.2 Pros and cons for EHS (Inference unless cited)

Pros:
- One RTOS for MCU and Cortex-A55, and one networking/TLS/MQTT/HTTP/OTA stack maintained upstream rather than assembled from Cube middleware.
- Best CRA story of the RTOS options: SBOM generator, CNA, published advisories, manufacturer pre-notification, optional 15-year commercial support.
- Devicetree + Kconfig describe boards as data, which suits EHS's "target by capability" idea (study 18 §1.2) and the designer's capability profile.
- LLEXT and zbus map directly to studies 09 and 11.

Cons:
- **Migration cost of the existing boards.** EHS board files are Cube HAL code (F746: 12 source files incl. display, QSPI, SD, software JPEG, audio, video; H747: 9). Under Zephyr each must become a devicetree node plus Zephyr driver, or a wrapped HAL call. The `stm32h747i_disco` Zephyr page needs a display shield config, documents Ethernet mainly as solder-bridge changes, and says other features are "not yet supported" (Zephyr docs; a 2023 discussion reported the NT35510 panel revision unsupported, later PR not confirmed merged).
- **Vendor direction:** ST's Cube strategy (May 2025) is FreeRTOS + lwIP; ST is a Zephyr silver member and maintainer, but a user on ST Community calls STM32 support "not first class like Nordic's" (second-hand opinion).
- Learning curve: west, devicetree, Kconfig, sysbuild; six-month release cadence with API changes.
- LVGL version lag (9.3 in Zephyr 4.2 vs 9.5 in EHS).
- Visible CVE flow in the net stack (DNS CVE-2025-1673, ICMP CVE-2025-12899, HTTP server CVE-2024-10395, DTLS CID CVE-2026-8718). This is partly a sign of good disclosure, but each one is a firmware update obligation.
- Smaller hiring pool in Taiwan than for FreeRTOS/Cube or embedded Linux (Inference; no data found).

Effort delta (Inference): Zephyr instead of FreeRTOS + Cube for path A adds about +2–4 PM on the first board (board port, LVGL module at 9.5, learning) and saves about 1.5–3 PM (MQTT, TLS, HTTP, MCUboot, SBOM, LLEXT come ready). Roughly even on the first board; positive from the second board or the first Cortex-A55 RTOS target.

---

## 5. Security note: RTOS TCP/IP stack history and the CRA

### 5.1 History

| Event | Stack(s) | What | Relevance |
| --- | --- | --- | --- |
| Zimperium, Oct–Dec 2018 | **FreeRTOS+TCP** (FreeRTOS ≤ 10.0.1, AWS FreeRTOS ≤ 1.3.1); also WHIS Connect TCP/IP in OpenRTOS/SafeRTOS | 13 CVEs: CVE-2018-16522 to 16528 and CVE-2018-16598 to 16603; four RCE, one DoS, seven info leaks (second-hand counts); fixed in AWS FreeRTOS 1.3.2 | Shows the parsers (DNS, DHCP, TCP options, NBNS, LLMNR) are where bugs live |
| AMNESIA:33, Dec 2020 (Forescout) | uIP/Contiki, picoTCP, FNET, Nut/Net | 33 CVEs in DNS, IPv6, IPv4, TCP, ICMP, LLMNR, mDNS; worst CVE-2020-25111 (9.8) | **Not FreeRTOS+TCP, not lwIP.** Forescout analysed lwIP, CycloneTCP and uC/TCP-IP and found none (Black Hat EU 2020 slides) |
| NAME:WRECK, Apr 2021 (Forescout, JSOF) | FreeBSD, Nucleus NET, **NetX**, IPnet | Nine DNS flaws; NetX 6.0.1 affected, fixed in 6.1, no CVE for NetX | Relevant if ThreadX/NetX Duo is chosen |
| FreeRTOS+TCP 2024–2026 | FreeRTOS+TCP | CVE-2024-38373 (DNS response over-read, 4.0.0–4.1.0, fixed 4.1.1); CVE-2025-5688 (out-of-bounds write, AWS-2025-012); CVE-2025-11616/11617/11618 (IPv6/ICMPv6, fixed 4.3.4); CVE-2026-7425 (IPv6 Router Advertisement DoS, fixed 4.2.6/4.4.1). Aggregator mentions of an AWS bulletin 2026-139 with odd CVE ids are unverified | Steady flow, mostly DNS and IPv6 |
| lwIP | lwIP | CVE-2020-22283/22284 (older, Debian tracker); 2026: CVE-2026-8836 SNMPv3 USM stack overflow in ≤ 2.2.1 (CVSS 9.3 in some databases), fixed by upstream commit, no release named; VulnCheck lists an mDNS search-response out-of-bounds write and an SNMP agent over-read in ≤ 2.2.1 | Optional apps (SNMP, mDNS) carry the risk; last release 2.2.1 on 2025-02-06, so a product must track git commits |
| Zephyr 2024–2026 | Zephyr net | CVE-2024-10395 (HTTP server), CVE-2025-1673 (DNS), CVE-2025-12899 (ICMP), CVE-2026-8718 (DTLS CID) | Published through Zephyr's PSIRT pages |

Rules for EHS (Inference): compile out what is not used (IPv6, mDNS, LLMNR, NBNS, SNMP, DNS if static IPs suffice); keep the HMI as a client by default and put services (Modbus TCP server, HTTP, VNC) behind an explicit setting; fuzz every parser exposed to the LAN, including EHS's own (MBAP, SLMP, FINS, S7, CIP); pin and record exact stack versions or commits; plan a signed OTA from day one.

### 5.2 What CRA vulnerability handling means for an RTOS product

From the Zephyr CRA page, the Commission/ENISA pages and law-firm summaries:

- **Dates:** reporting of actively exploited vulnerabilities and severe incidents through ENISA's Single Reporting Platform applies since **11 September 2026**; full application **11 December 2027**.
- **Reporting:** early warning within **24 hours**, notification within **72 hours**, final report within **14 days** after a fix is available (severe incidents: 24 h, 72 h, one month).
- **Support period:** security updates for the expected lifetime, at least five years; vulnerability handling, SBOM in the technical documentation, secure-by-default configuration, conformity assessment and CE marking.
- **Components:** the manufacturer is responsible for vulnerabilities in integrated open source, and must report vulnerabilities found in components to their maintainers and share fixes (Article 13(6), per the Zephyr page).
- **Penalties:** up to EUR 15 million or 2.5 % of worldwide turnover for Article 13/14 breaches.

Consequences specific to an RTOS HMI (Inference):

1. **One binary, so every fix is a whole-firmware update.** On Linux a libc or OpenSSL fix is a package in an A/B image built by Yocto with `cve-check`; on an RTOS the kernel, IP stack, TLS, drivers and the runtime are one signed image. Secure boot + A/B OTA + rollback (row A7) is a CRA prerequisite, not a feature.
2. **SBOM is manual on Cube.** Yocto produces SPDX and CVE reports; Zephyr has `west spdx`; STM32Cube middleware has neither, so EHS must generate an SBOM from its own manifest (EHS already pins component revisions: `docs/project-storage-architecture.md:453`).
3. **Upstream support windows must cover five years or more.** FreeRTOS 202406 LTS ended on 2026-06-30; 202604 LTS gives two years, with a paid EMP for up to ten. lwIP has no LTS or advisories. Zephyr upstream LTS is 3.7 to January 2027, plus Canonical's commercial offer. Budget a yearly stack rebase.
4. **The project blob is in scope.** If projects are downloaded over the network (path A §2.3), the download service is an attack surface: authenticate, sign, version-check, and allow it to be switched off in production (study 11 §5 already says so).
5. **CRA classification of an HMI panel** (default vs Annex III Class I) was not checked here; most HMIs are likely default category with self-assessment (Inference; confirm with counsel).

---

## 6. Feature matrix: Linux / Cortex-A RTOS / MCU RTOS

| Feature | Linux on Cortex-A (study 18) | Cortex-A + RTOS (path B) | MCU + RTOS (path A) |
| --- | --- | --- | --- |
| Networking protocols | Everything: Modbus TCP, S7 (snap7), EtherNet/IP (libplctag), MQTT, OPC UA, HTTP(S), SNMP, NTP | Modbus TCP, own PLC drivers, MQTT, HTTP via lwIP/Zephyr; each extra protocol is own C | Same as Cortex-A RTOS, limited by RAM (sockets, TLS sessions) |
| Wi-Fi | SDIO/USB modules with mainline drivers + wpa_supplicant | Vendor-specific or hosted module (e.g. ESP32 as network co-processor); Zephyr has Wi-Fi management | External module with on-module stack; pre-certified modules avoid radio certification (Inference) |
| USB host (stick import) | Native (udev, vfat/exFAT) | Vendor USB host stack (FSP, Vitis) | H7 OTG host + MSC + FatFs; 0.5–1 PM |
| VNC / web remote | neatvnc; web view of WASM HMI | VNC-lite or web remote; own code | VNC-lite (bandwidth-bound) or device serves WASM HMI + WebSocket tags |
| OPC UA | open62541 full server/client | open62541 possible, own porting | Possible in principle (ports exist for ESP32, Renesas RZ/T2/N2 samples); RAM-tight, unmeasured; not for MVP |
| Multi-PLC concurrency | Thread per link; tens of devices | Several links; bounded by RAM | 2–4 links practical (Inference); drivers are non-blocking so one task suffices |
| Process isolation | Yes: processes, unprivileged user, seccomp, namespaces | No (single address space; Zephyr userspace partial) | No (FreeRTOS-MPU or Zephyr userspace partial) |
| OTA | RAUC A/B, signed bundles; project bundles separate | Own A/B boot; whole image | MCUboot / OEMiRoT A/B; whole image or split image |
| Secure boot | SoC ROM + U-Boot verified boot + dm-verity (per SoC) | SoC ROM secure boot (per vendor) + own chain | MCUboot / ST iRoT; RDP/TrustZone on newer parts |
| CRA / SBOM | Yocto SPDX + `cve-check`; Buildroot `legal-info`; many packages to track | Manual SBOM (or Zephyr `west spdx`); fewer components | Manual SBOM on Cube, `west spdx` on Zephyr; fewest components, but every fix is a full update |
| Boot time to first screen | < 8 s planned, < 3 s with tuning (study 18) | About 1 s (Renesas "quick startup"; Inference) | Well under 1 s typical (Inference; not measured in the repo) |
| BOM (SoC/SoM + memory, Inference) | SoM US$15–60 (study 18) plus panel | RZ/A3M chip about US$14 at 500 with DRAM inside; RZ/A3UL + DDR more | H7-class MCU + SDRAM + QSPI; cheapest only below about 7" and 800x480 |
| Memory price exposure | High: DDR3/DDR4/LPDDR4 contract prices rose sharply through 2026 (DDR4 +24 % in three months to Oct 2026 per Barclays via trade press; LPDDR4 short) | Low with DRAM-in-package (price fixed in the chip); high with external DDR | Low to medium: SDR SDRAM 8–32 MB, or none on parts with large SRAM (STM32N6: up to 4.2 MB, enough for 1280x800 double buffer per ST) |
| Dev tools / debugging | gdb, perf, ASan on host, strace, journald; BSP/kernel debugging is the hard part | Vendor IDE (e² studio, Vitis) + J-Link; little community help | CubeIDE/CLT, ST-LINK, SWO, SEGGER SystemView, Percepio Tracealyzer; EHS already uses CubeCLT |
| Hiring pool in Taiwan (Inference) | Large for BSP/Linux (ODM, NAS, industrial PC makers); app-level C/C++ easy | Small; vendor-specific | Large for STM32/Keil/IAR/FreeRTOS firmware; firmware and driver roles are listed as a talent gap in Taiwan's IC-design survey (2023 estimate, second-hand); Zephyr experience rare |

---

## 7. Recommendation for EHS

### 7.1 Product lines (Inference)

| Line | Screen / use | Platform | Protocols and features |
| --- | --- | --- | --- |
| **EHS Lite (networked MCU HMI)** | 4.3"–7", 480x272 to 800x480, cost-driven OEM panels, machine builders with one PLC | STM32H7 (or H7R/S, N6) + FreeRTOS + lwIP | Modbus RTU/TCP, one or two PLC families (MC/SLMP, FINS), MQTT with TLS, alarms with ring-log history, recipes, web remote, USB stick import, signed OTA. No OPC UA, no SQLite, no multi-user audit trail |
| **EHS main line** | 7"–15" | Linux on RK3568/i.MX93 class (study 18) | Full driver list, OPC UA, VNC + web, SQLite history, users/audit, RAUC, process isolation |
| **EHS Fast-boot (optional)** | 7"–10" where sub-second boot or DRAM-free BOM is a contract requirement | Zephyr on i.MX93 A55, or RZ/A3UL + FreeRTOS | Lite feature set at higher resolution; only on demand |
| Windows runtime | Customer panel PCs, PC simulator | Windows build of the Linux runtime (study 18) | As main line |

Sequencing (Inference): EHS's own plan already does lwIP `NO_SYS` + Modbus TCP on the H747 (spikes R3/R4, about 1 PM). That gives an Ethernet Modbus HMI without an RTOS and should go ahead. The decision between "Lite first" and "Linux main line first" depends on customers, not technology: Lite is the shorter path from today's code and keeps the "LVGL also runs on MCUs" advantage that Linux-only rivals lack (study 18 conclusion 9); Linux is the larger market (study 18 §1).

### 7.2 One designer + codegen for all three?

Yes, with these conditions (Inference):

1. **One runtime core in portable C**, as today's `firmware/common` already is (it compiles for MCUs and with emcc). Platform layers: `hmi_platform` (tick, later mutex/queue), `hmi_transport` (UART, CDC, socket), storage, display/input. Linux, FreeRTOS, Zephyr and the WASM Emulator are four implementations of the same small interfaces.
2. **PLC drivers written once against `hmi_driver.h`** (non-blocking), never against libraries that only exist on Linux. Linux can additionally offer library-based drivers (snap7, libplctag, open62541) as Linux-only capabilities.
3. **Project data out of C on every target** (tags, devices, alarms, recipes, users, assets index); generated C only for screens and Logic. Same `.ehsr`-style bundle format; per-target code payload (`.so`, `.dll`, MCU project slot).
4. **A capability profile per target** in the designer (RAM, flash, protocols, max links, history storage, remote options) that greys out what a target cannot do and shows budgets, like today's MCU budget bars.
5. **One pinned LVGL version** on all targets (9.5.0 today); on Zephyr, override the module.
6. **A CI matrix** that builds every example project for every target and screenshot-diffs against the Emulator. The test matrix, not the code, is the main recurring cost of three targets; expect 15–25 % extra effort on every later feature for the second and third target (Inference).

What breaks the "one designer" idea: adding a device-side interpreter for one target only (option b), or using Linux-only libraries inside the core.

---

### 7.3 Open questions

1. Measure on the H747: flash/RAM delta of lwIP + mbedTLS + coreMQTT + MCUboot, and UI frame time during a TLS handshake with and without FreeRTOS.
2. Which STM32H7R/S or N6 parts have Ethernet, and their prices, as the base of a Lite board (H7R/S give MCUboot-based OEMiRoT and external-flash XIP).
3. Prototype the (c-MCU) split image: jump-table ABI, XIP project slot, A/B swap of the project without touching the runtime.
4. Zephyr spike: EHS runtime + LVGL 9.5 module on `stm32h747i_disco` and on `imx93_evk/mimx9352/a55`; count board-file work.
5. RZ/A3M Ethernet: confirm in Renesas's datasheet; if absent, check RZ/A3UL pricing and LVGL draw-unit support for its 2D engine.
6. CRA class of an HMI panel; whether the project download service counts as a "remote data processing solution".
7. Taiwan hiring data for FreeRTOS, Zephyr and embedded Linux roles (104/1111 job counts).

---

## Sources

Local (read-only):

- `edt-hmi-studio`: `firmware/common/include/hmi_platform.h`, `hmi_transport.h`, `hmi_driver.h`, `hmi_runtime.h`; `firmware/common/src/hmi_runtime.c`; `firmware/{stm32f746g-disco,stm32h747i-disco,edt-evk043027b}/src/main.c` and `include/lv_conf.h`; `firmware/stm32h747i-disco/src/board.c`; `firmware/edt-evk043027b/include/main.h`; `docs/protocol-connections.md`, `docs/stm32h747i-disco-dual-core.md`, `docs/edt-evk043027b.md`, `docs/display-sleep.md`, `docs/hmi-themes.md`, `docs/images-external-flash.md`, `docs/project-storage-architecture.md`, `docs/hmi-memory-and-retention.md`.
- This research folder: `18-mpu-runtime.zh-TW.md`, `notes/18-mpu-runtime/ehs_port_to_mpu.md`, `09-mcu-unit-bus-architecture.zh-TW.md`, `11-mcu-hot-reload.zh-TW.md`.

Web:

- LVGL threading and FreeRTOS: [LVGL 9.5 FreeRTOS](https://docs.lvgl.io/9.5/integration/rtos/freertos.html); [LVGL 9.4 threading](https://docs.lvgl.io/9.4/details/integration/overview/threading.html); [LVGL 9.2 OS and interrupts](https://docs.lvgl.io/9.2/porting/os.html)
- ST direction: [ST Community, New strategic directions for STM32Cube (2025-05-15)](https://community.st.com/t5/developer-news/new-strategic-directions-for-stm32cube/ba-p/799537); [ST Community, newer ThreadX versions](https://community.st.com/stm32-mcus-embedded-software-32/newer-threadx-versions-150225); [X-CUBE-AZRTOS-H7](https://github.com/STMicroelectronics/x-cube-azrtos-h7)
- ST secure boot: [ST wiki, SBSFU by MCUboot](https://wiki.st.com/stm32mcu/wiki/Security:SBSFU_by_MCUboot); [ST wiki, SBSFU legacy](https://wiki.st.com/stm32mcu/wiki/Security:SBSFU_legacy); [ST wiki, OEMiRoT/OEMuRoT for STM32H7S](https://wiki.st.com/stm32mcu/wiki/Security:OEMiRoT_OEMuRoT_for_STM32H7S)
- FreeRTOS LTS: [FreeRTOS LTS libraries](https://freertos.org/lts-libraries.html); [AWS, FreeRTOS 202604 LTS](https://aws.amazon.com/about-aws/whats-new/2026/04/freertos-lts/); [AWS, 202406 LTS](https://aws.amazon.com/about-aws/whats-new/2024/07/freertos-long-term-support-version)
- lwIP: [Savannah lwIP news (2.2.1)](https://savannah.nongnu.org/news/?group=lwip); [Ubuntu CVE-2026-8836](https://ubuntu.com/security/CVE-2026-8836); [Debian tracker CVE-2026-8836](https://security-tracker.debian.org/CVE-2026-8836); [VulnCheck mDNS advisory](https://www.vulncheck.com/advisories/lwip-through-2.2.1-stack-out-of-bounds-write-via-mdns-search-response-handler); [VulnCheck SNMP advisory](https://www.vulncheck.com/advisories/lwip-2.0.0-through-2.2.1-snmp-agent-out-of-bounds-read-via-snmp-pbuf-stream-writeto); [Debian CVE-2020-22284](https://security-tracker.debian.org/tracker/CVE-2020-22284)
- FreeRTOS+TCP CVEs: [Zimperium 2018 details](https://zimperium.com/blog/freertos-tcpip-stack-vulnerabilities-details); [NVD CVE-2018-16522](https://nvd.nist.gov/vuln/detail/CVE-2018-16522); [Security Affairs 2018](https://securityaffairs.com/?p=77294); [NVD CVE-2024-38373](https://nvd.nist.gov/vuln/detail/CVE-2024-38373); [AWS-2025-012](https://aws.amazon.com/security/security-bulletins/AWS-2025-012/); [NVD CVE-2025-11617](https://nvd.nist.gov/vuln/detail/CVE-2025-11617); [OSV CVE-2025-11616](https://osv.dev/vulnerability/CVE-2025-11616); [SentinelOne CVE-2026-7425](https://www.sentinelone.com/vulnerability-database/cve-2026-7425/)
- AMNESIA:33 and NAME:WRECK: [Tenable AMNESIA:33](https://www.tenable.com/blog/amnesia33-researchers-disclose-33-vulnerabilities-tcpip-libraries-uip-fnet-picotcp-nutnet); [Forescout Black Hat EU 2020 slides](https://i.blackhat.com/eu-20/Wednesday/eu-20-dosSantos-How-Embedded-TCPIP-Stacks-Breed-Critical-Vulnerabilities.pdf); [CISA NAME:WRECK](https://cisa.gov/news-events/alerts/2021/04/15/namewreck-dns-vulnerabilities); [Tenable NAME:WRECK](https://tenable.com/blog/namewreck-nine-dns-vulnerabilities-found-in-four-open-source-tcpip-stacks); [CSA Singapore AL-2021-024](https://www.csa.gov.sg/alerts-and-advisories/alerts/al-2021-024)
- Zephyr: [Networking overview](https://docs.zephyrproject.org/latest/connectivity/networking/overview.html); [Networking protocols](https://docs.zephyrproject.org/latest/connectivity/networking/api/protocols.html); [Modbus](https://docs.zephyrproject.org/latest/services/modbus/index.html); [Zephyr 4.2 release notes (LVGL 9.3)](https://docs.zephyrproject.org/4.2.0/releases/release-notes-4.2.html); [Releases](https://docs.zephyrproject.org/latest/releases/index.html); [i.MX93 EVK A55](https://docs.zephyrproject.org/latest/boards/nxp/imx93_evk/doc/index.html); [NXP, Zephyr on A55 with FRDM-IMX93](https://community.nxp.com/t5/i-MX-Processors-Knowledge-Base/Run-Zephyr-on-A55-with-FRDM-IMX93-and-FRDM-IMX91/ta-p/2143151); [STM32H747I Discovery board page](https://docs.zephyrproject.org/latest/boards/st/stm32h747i_disco/doc/index.html); [Zephyr discussion #60888](https://de.github.com/zephyrproject-rtos/zephyr/discussions/60888); [Zephyr CRA page](https://docs.zephyrproject.org/latest/security/standards/cyber-resilience-act.html); [Zephyr vulnerabilities 2025](https://docs.zephyrproject.org/latest/security/vulnerabilities/2025.html); [Zephyr vulnerabilities 2024](https://docs.zephyrproject.org/latest/security/vulnerabilities/2024.html); [Canonical Zephyr LTS announcement](https://canonical.com/blog/zephyr-lts-announcement); [LF, three projects and CRA](https://www.linuxfoundation.org/blog/pathways-to-cybersecurity-best-practices-in-open-source-how-three-linux-foundation-projects-are-leading-the-way-in-cra-compliance); [ST, Zephyr ecosystem page](https://st.com/content/st_com/en/ecosystems/stm32-mcus-in-the-open-source-software-ecosystem/zephyr.html); [ST blog, Zephyr 4.4.0](https://blog.st.com/zephyr/); [LF press, ST/Renesas join Zephyr](https://www.linuxfoundation.org/press/renesas-stmicroelectronics-and-ac6-join-the-zephyr-project-as-it-launches-the-3.6-release); [ST Community, Zephyr support opinion](https://community.st.com/t5/stm32-summit-q-a/is-st-planning-to-invest-more-in-zephyr/td-p/652339)
- CRA: [European Commission, CRA reporting](https://digital-strategy.ec.europa.eu/en/policies/cra-reporting); [ENISA, SRP launched](https://www.enisa.europa.eu/news/the-cra-single-reporting-platform-is-launched); [Skadden, reporting FAQs (2026-09)](https://www.skadden.com/insights/publications/2026/09/get-ready-reporting-faqs-and-checklist)
- Cortex-A RTOS parts: [CNX, RZ/A3M](https://www.cnx-software.com/2025/05/19/1ghz-renesas-rz-a3m-cortex-a55-mpu-embeds-128mb-on-chip-ddr3l-for-hmi-applications/); [Renesas RZ/A3M](https://www.renesas.com/en/products/rz-a3m); [Renesas RZ/A3UL](https://www.renesas.com/en/products/rz-a3ul); [Mouser RZ/A3UL](https://www.mouser.ca/renesas-rz-a3ul-mpu); [CNX, RZ/A3UL](https://www.cnx-software.com/2022/08/05/renesas-rz-a3ul-cortex-a55-cpu-runs-rtos-for-hmi-with-quick-startup-times/); [FreeRTOS Zynq-7000 port](https://blog.freertos.org/RTOS-Xilinx-Zynq.html); [AMD UG1137 FreeRTOS stack](https://docs.amd.com/r/2020.2-English/ug1137-zynq-ultrascale-mpsoc-swdev/FreeRTOS-Software-Stack); [LVGL forum, Zedboard LVGL + FreeRTOS template](https://forum.lvgl.io/t/new-zedboard-lvgl-freertos-tcp-fat-and-usb-template-project-available-on-github/10245)
- MCU parts and footprints: [ST STM32N6 product page](https://st.com/en/microcontrollers-microprocessors/stm32n645b0.html); [CNX, STM32N6](https://www.cnx-software.com/2024/12/13/stmicro-releases-stm32n6-cortex-m55-mcu-series-with-in-house-npu-and-dedicated-computer-vision-pipeline/); [Mbed TLS footprint reduction](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/reduce-polarssl-memory-and-storage-footprint/); [ESP-IDF mbedTLS heap figures](https://github.com/espressif/esp-idf/blob/v5.2.1/docs/en/api-reference/protocols/mbedtls.rst); [TI SPRADB2 lwIP + mbedTLS](https://ti.com/lit/pdf/spradb2); [open62541 on STM32H7S3 (mailing list)](https://groups.google.com/g/open62541/c/eOo9n78MmtM); [Renesas RZ/T2 RZ/N2 OPC UA sample](https://www.renesas.com/en/document/apn/rzt2-rzn2-group-opc-ua-server-and-pubsub-sample-software); [ST Community, Ethernet lwIP FreeRTOS issues on H753](https://community.st.com/t5/stm32-mcus-embedded-software/ethernet-lwip-freertos-issues-on-nucleo-h753zi/td-p/830649)
- Memory prices (trade press): [The Register, DRAM prices expected to double (2026-02)](https://www.theregister.com/2026/02/02/dram_prices_expected_to_double/); [Wccftech, DDR4 +50 % Q3 2026](https://wccftech.com/memory-shortages-drive-ddr4-prices-over-50-in-q3-2026-ddr3-also-impacted-by-higher-costs/); [Guru3D, DDR4 shortage through 2028](https://www.guru3d.com/story/ddr4-memory-prices-now-also-rise-as-dram-supply-shortages-intensify-through-2028/)
- Taiwan labour signals (second-hand): [Business Next, Taiwan tech jobs shift](https://www.bnext.com.tw/article/90804/taiwan-tech-jobs-shift-from-software-to-chip-design-and-ai); [NDC talent survey PDF](https://ws.ndc.gov.tw/001/administrator/18/relfile/6037/9935/91b50cfd-34f7-4c7f-839e-0c089f5e2747.pdf)
- Not re-verified in this session (general knowledge): RZ/A2M specifics; Zephyr userspace isolation details; coreMQTT and mbedTLS licences (MIT, Apache-2.0); Percepio/SEGGER tool support.
