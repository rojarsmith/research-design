# Cortex-A application processors running an RTOS (not Linux) with Ethernet and a display

Research date: 2026-11-20. Scope: which Cortex-A SoC families have **vendor-supported** RTOS or bare-metal software on the **Cortex-A cores themselves** (not only on companion Cortex-M/R cores), and whether that software includes what an iFace-style industrial HMI needs:
- Ethernet MAC + PHY driver with a TCP/IP stack (lwIP, FreeRTOS+TCP, NetX Duo, Zephyr net, vendor stack);
- LCD controller, touch, 2D GPU;
- USB host, SD/eMMC, Wi-Fi;
- SMP, MMU/cache setup, LVGL port.

Context: EDT HMI Studio (EHS) generates LVGL C for STM32 bare-metal super-loop firmware. The question is whether an RTOS on a Cortex-A chip can replace Linux for a NOVAKON iFace-style panel (Ethernet, Modbus TCP, MQTT, PLC drivers). Study 18 (`research/18-mpu-runtime.zh-TW.md`) assumed Linux; this note checks the RTOS alternative.

## Labels used

- **[primary]**: vendor SDK documentation, release notes, product pages, datasheets, vendor GitHub repositories, vendor press releases, and the official Zephyr / FreeRTOS / LVGL project docs for their own software.
- **[secondary]**: trade press (CNX Software, Hackster), distributor or board-maker pages, forums (including vendor forums where the poster is not clearly staff), search-engine summaries of pages I could not open.
- **[inference]**: my own judgement. Flagged "unverified" where it rests on background knowledge rather than a source read this session.

**Method note.** Evidence comes from WebSearch summaries and WebFetch of individual pages. Several pages timed out or were blocked: the ST wiki release notes for STM32CubeMP13, microchip.com software pages, the Microchip LVGL-for-Harmony page, the Xilinx wiki lwIP pages (Confluence shell only), and the XAPP1306 PDF. Facts from those come from search summaries and are marked [secondary] even when the underlying page is a vendor page.

---

## 1. Summary table

| Family | A-core | Vendor RTOS on the A core? | RTOS | TCP/IP on A core | Display on A core | 2D GPU | Vendor support level |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Renesas RZ/A1 (H/LU) | Cortex-A9, 1 core | Yes | FreeRTOS (10.4.3 LTS) | Ethernet + lwIP sample (RZ/A1LU); Ethernet fix in RZ/A1H pkg | Yes (VDC) | No | Legacy package v1.10 (Nov 2021) |
| Renesas RZ/A2M | Cortex-A9, 1 core | Yes | FreeRTOS (10.4.3 LTS) | FreeRTOS+TCP; SDIO Wi-Fi package on request | Yes (VDC, Guiliani) | No 2D GPU (DRP is an image-processing accelerator) | Legacy packages |
| Renesas RZ/A3UL | Cortex-A55, 1 core | **Yes, designed for it** | FreeRTOS and Azure RTOS / ThreadX | 2x GbE; r_gether + PHY; FreeRTOS+TCP or NetX Duo | Yes (r_lcdc, MIPI DSI) | No (no DRW on A3UL, inference from feature list) | RZ FSP v4.x (current) |
| Renesas RZ/A3M | Cortex-A55, 1 core, 128 MB DDR3L in package | **Yes, designed for it** | FreeRTOS and Azure RTOS (announcement / CNX) | **No Ethernet listed** in the announcement or CNX spec list | Yes (LCDC, MIPI DSI 4-lane, up to 1280x800) | Yes (2D drawing engine, D/AVE 2D `r_drw`) | RZ FSP v4.2/4.3; official LVGL port |
| AMD Zynq-7000 | Cortex-A9, 2 cores | Yes | FreeRTOS BSP in Vitis (single core) | GEM + lwIP (raw and socket) | **No display controller in the PS** (needs PL IP) | No | Vitis, current |
| AMD Zynq UltraScale+ MPSoC | Cortex-A53, 4 cores (+R5F) | Yes | FreeRTOS BSP in Vitis, 32/64-bit APU | GEM + lwIP | DisplayPort via standalone dppsu/avbuf/dpdma drivers | Mali-400 is Linux-only (inference) | Vitis, current; no SMP |
| TI AM64x | Cortex-A53, 2 cores (+R5F) | Partly | FreeRTOS, FreeRTOS SMP **experimental** | CPSW + lwIP tested on A53 | **No display subsystem on AM64x** (inference, unverified) | No | MCU+ SDK; A53 is experimental |
| TI AM243x | **No Cortex-A** (R5F + M4F only, inference) | n/a | n/a | n/a | n/a | n/a | n/a |
| TI AM62x | Cortex-A53, up to 4 cores | Yes, "as is" | FreeRTOS on A53 | ENET (CPSW) on A53 with FreeRTOS | DSS on A53 listed | GPU not listed | **Reference only; no E2E support** |
| TI AM62L | Cortex-A53, 2 cores | Yes, "as is" | FreeRTOS 11.1.0 on A53 | CPSW + lwIP 2.2.1 | DSS driver listed | Not listed | **Reference only; no E2E support** |
| Microchip SAMA5D2 | Cortex-A5, 1 core | Yes | Harmony v3 bare metal or FreeRTOS; ThreadX beta (2021) | Harmony TCP/IP stack (bare metal and RTOS demos) | Yes (Legato, "blank" apps for 3rd-party libs) | Yes (GFX2D on some parts; not confirmed for A5D2) | Harmony v3, current |
| Microchip SAMA5D3 / SAMA5D4 | Cortex-A5, 1 core | Not found in Harmony v3 | Legacy Atmel softpack (inference, unverified) | Ethernet HW exists (Oryx table, secondary) | HW exists | — | Legacy |
| Microchip SAMA7G5 | Cortex-A7, 1 core | Yes | Harmony v3 bare metal / FreeRTOS; ThreadX beta | Harmony TCP/IP; bare-metal Ethernet web-server app | Legato graphics (not confirmed this session) | — | Harmony v3, current |
| Microchip SAMA7D65 | Cortex-A7, 1 core | **Yes** | Harmony v3 bare metal and FreeRTOS; also upstream Zephyr board | Harmony TCP/IP, bare-metal Ethernet examples, iPerf2 | Yes (MGS Harmony, MIPI DSI / LVDS, LVGL guide) | "GPU" used by MGS Harmony | Harmony v3, current |
| NXP i.MX 6 / 6UL / 6ULL | Cortex-A9 / A7 | Old: i.MX 6ULL MCUXpresso SDK (2017) with FreeRTOS | FreeRTOS (6ULL SDK); QNX / VxWorks / INTEGRITY third-party | Not confirmed for 6ULL SDK | eLCDIF not confirmed | No | Legacy |
| NXP i.MX 7 | Cortex-A7 | No (RTOS only on the M4, inference) | — | — | — | — | — |
| NXP i.MX 8M Mini / Plus | Cortex-A53 | Yes, via Real-Time Edge | Zephyr; bare metal | lwIP on A core (ENET on 8M Mini, 8M Plus) | Not mentioned | Not mentioned | Real-Time Edge 3.5 (Jul 2026) |
| NXP i.MX 93 / 95 / 943 | Cortex-A55 | Yes, via Real-Time Edge; upstream Zephyr | Zephyr 4.4, FreeRTOS 11.2.0 (93, 95), bare metal | lwIP on A core (ENET on i.MX 93) | Zephyr board lists LCDIFv3 + MIPI DSI hardware; driver status unverified | Not mentioned | Real-Time Edge 3.5 |
| ST STM32MP13 | Cortex-A7, 1 core | **Yes** | STM32CubeMP13: Azure RTOS (ThreadX, NetX Duo, USBX, FileX, LevelX) and FreeRTOS (v1.2.0) | NetX Duo or lwIP; dual-Ethernet lwIP example | LTDC; TouchGFX via X-TOUCHGFX-MPU add-on | No | STM32CubeMP13 v1.2.0 |
| ST STM32MP15 | Cortex-A7, 2 cores | No (Cube package targets the M4) | — | — | — | — | — |
| ST STM32MP2 | Cortex-A35 | **Alpha only** (ST moderator, Mar 2026) | — | — | — | — | Cube package targets the M33 |
| Rockchip RK3506 | Cortex-A7, 3 cores (+M0) | Yes per board makers | RT-Thread 4.1 SMP; RK HAL bare metal | Board makers put Ethernet and display on the Linux side in AMP | Same | RGA 2D (Linux side) | Secondary sources only |
| Rockchip RK3568 | Cortex-A55, 4 cores | AMP: 1 core RT-Thread + 3 cores Linux | RT-Thread (Rockchip SDK); RT-Thread mainline `rk3500` BSP (RT-Smart); Zephyr board (minimal) | Not documented for the RTOS side | Not documented | — | Secondary / community |
| Allwinner T113 | Cortex-A7, 2 cores | Melis RTOS (RT-Thread based) per CNX | Melis | Not documented | Not documented | G2D (Linux evidence only) | Secondary only |
| Allwinner D1 / D1s | **RISC-V C906, not Cortex-A** | RT-Smart BSP in RT-Thread mainline | — | — | — | — | Out of scope |
| Raspberry Pi 1-5 (reference) | Cortex-A53/A72/A76 | No vendor RTOS; community Circle (bare metal C++) and Zephyr | Circle; Zephyr | Circle: own TCP/IP, MQTT client, Pi 5 GEM Ethernet, Pi 5 Wi-Fi; Zephyr: GENET on Pi 4 | Circle framebuffer, Pi 5 DSI touchscreens, LVGL | No GPU use | Community |

---

## 2. Per family

### 2.1 Renesas RZ/A (the only family designed around "RTOS on Cortex-A for HMI")

**Positioning**
- RZ/A3UL: single Cortex-A55, 1 GHz. Renesas markets it to "achieve the full potential of a real-time operating system (RTOS)" and to "start up instantly, in less than a second after boot-up". Two Ethernet channels, 10M/100M/1G; graphics LCD controller; parallel display. Linux not mentioned on the page. [primary] ([Renesas RZ/A3UL page](https://www.renesas.com/jp/en/products/microcontrollers-microprocessors/rz-mpus/rza3ul-powerful-1ghz-64-bit-mpus-rtos-support-enables-high-definition-hmi-and-quick-startup))
- RZ/A3UL "supports the industry's two RTOSes: FreeRTOS and Azure RTOS"; free GUI tools Guiliani-Lite (FreeRTOS) and GUIX (Azure RTOS). Renesas also offers an "RTOS-Based RZ/A3UL HMI SMARC SoM" reference design for industrial terminals. [primary, via search summary] ([RTOS-based RZ/A3UL HMI SMARC SoM](https://www.renesas.com/us/en/applications/industrial/rtos-based-rza3ul-hmi-smarc-som), [press release](https://www.renesas.com/us/en/about/press-room/renesas-unveils-powerful-1ghz-rza3ul-64-bit-mpus-rtos-support-enable-high-definition-hmi-and-quick))
- RZ/A3M (May 2025): same A55 core, 128 MB DDR3L in a 17x17 mm 244-pin LFBGA, LCD up to 1280x800, parallel RGB and 4-lane MIPI DSI, 2D drawing engine, USB 2.0, SDHI, SPI, I2C, I2S. **Ethernet is not listed.** GUI partners: LVGL, Crank, SquareLine Studio, Envox. [primary] ([Renesas newsroom](https://www.renesas.com/en/about/newsroom/renesas-extends-rza-mpu-line-rza3m-cost-sensitive-advanced-hmi-solutions)); "FreeRTOS or Azure RTOS support" and "1x USB 2.0 host" [secondary] ([CNX Software](https://www.cnx-software.com/2025/05/19/1ghz-renesas-rz-a3m-cortex-a55-mpu-embeds-128mb-on-chip-ddr3l-for-hmi-applications/)).
  - [inference] RZ/A3M has no Ethernet MAC. This rests on absence from the announcement, CNX spec list and datasheet snippet; confirm in the RZ/A3M datasheet R01DS0482 pin-function tables. For a networked HMI, RZ/A3M would need a USB or SPI Ethernet chip, or a pairing with RZ/A3UL. The RZ/A3UL lacks the 2D engine. Neither Renesas part combines Ethernet + 2D GPU + in-package DRAM.

**Software (RZ/A FSP, now RZ FSP)**
- RZ/A FSP v3.6.0 module lists [primary] ([networking](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___n_e_t_w_o_r_k_i_n_g___m_o_d_u_l_e_s.html), [graphics](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___g_r_a_p_h_i_c_s___m_o_d_u_l_e_s.html), [RTOS](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___r_t_o_s___m_o_d_u_l_e_s.html), [connectivity](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___c_o_n_n_e_c_t_i_v_i_t_y___m_o_d_u_l_e_s.html), [storage](https://renesas.github.io/rza-fsp/group___r_e_n_e_s_a_s___s_t_o_r_a_g_e___m_o_d_u_l_e_s.html)):
  - RTOS: `rm_freertos_port`, `rm_threadx_port`.
  - Networking: Ethernet `r_gether`, Ethernet PHY `r_gether_phy`, FreeRTOS+TCP `rm_freertos_plus_tcp`, NetX Duo Ether driver `rm_netxduo_ether`. lwIP is not listed for RZ/A.
  - Graphics: LCD controller `r_lcdc`, MIPI DSI `r_mipi_dsi_b`, D/AVE 2D `r_drw`, image scaling `r_isu`, camera `r_cru`, GUIX port `rm_guix_port`.
  - USB: `r_usb_basic`, host HID `r_usb_hhid`, host mass storage `r_usb_hmsc`, peripheral CDC, USBX port.
  - Storage: SDHI `r_sdhi` (SD/MMC), QSPI flash `r_spibsc`, block media for SD and USB, FileX block media, FreeRTOS+FAT port.
  - Touch: input via I2C (`r_riic_master`); no Wi-Fi module found.
- The FreeRTOS+TCP network interface for RZ/A requires `g_ether_instance` to have `vEtherISRCallback` as callback; NetX Duo IP MTU must be 576-1500 bytes. [primary, via search summary] ([FreeRTOS+TCP module](https://renesas.github.io/rza-fsp/group___f_r_e_e_r_t_o_s___p_l_u_s___t_c_p.html))
- `renesas/rza-fsp` repository: current release v3.6.0; only RZ/A3UL EVK listed; "deprecated and development/maintenance ended on February 13, 2026", archived Feb 20, 2026. Successor is the unified RZ FSP v4.x. Legacy FSPs supported to end of January 2027. [primary] ([GitHub rza-fsp](https://github.com/renesas/rza-fsp))
- RZ FSP: current v4.3.0 (installer files dated 2026-10). Names "FreeRTOS, Azure RTOS, and other middleware stacks". v4.2.0 updated LVGL to v9.5 for RZ/A and added USB HCDC. v4.3.0 lets users override `d1_cacheflush()` / `d1_cacheblockflush()` in `r_drw`. Renesas says that from v4.x the RZ Smart Configurator path supports only RZ/T and RZ/N. [primary] ([RZ FSP page](https://www.renesas.com/software-tool/rz-flexible-software-package-fsp)); v4.2.0 details from search summary [secondary].
- LVGL: official LVGL docs list RZ/A3M as the supported RZ/A board. They say RZ/A "focus on bare metal and RTOS applications" and give the port repository `lvgl/lv_port_renesas-ek-rz_a3m`. [primary] ([LVGL RZ/A docs](https://lvgl.io/docs/open/integration/chip_vendors/renesas/rza_family.mdx))
  - Port README: RZ FSP v4.2.0, LVGL v9.5.0, 1280x720 MIPI panel, 16-bit colour, capacitive touch. GPU listed as "None", so the benchmark is unaccelerated. Average over all scenes is about 53% CPU, 28 FPS and 26 ms per frame (8 ms render, 18 ms flush). An empty screen already costs 50% CPU, with the flush dominating. [primary] ([lv_port_renesas-ek-rz_a3m](https://github.com/lvgl/lv_port_renesas-ek-rz_a3m))
  - [inference] Flush-bound numbers suggest the port copies a full frame each refresh. A direct-mode or double-buffer setup with the LCDC scanning LVGL's buffer, plus D/AVE 2D, should cut this; unverified.
- Older RZ/A parts (legacy, but show the pattern):
  - RZ/A2M FreeRTOS packages use FreeRTOS V10.4.3-LTS-patch-1, **FreeRTOS+TCP**, USB host HID/MSC, FatFS, camera/LCD/JPEG. Separate packages exist for an SDIO Wi-Fi demo (contact Renesas) and FreeRTOS OTA. Hardware GUI acceleration requires the full Guiliani version. [primary] ([RZ/A2M SDK page](https://www.renesas.com/tw/en/products/software-tools/software-os-middleware-driver/software-package/rza2-software-development-kit-free-rtos.html))
  - RZ/A1H FreeRTOS package v1.10 (Nov 2021, FreeRTOS V10.4.3-LTS-patch-1). It includes a fix for Ethernet transmission stalling during continuous sends. The RZ/A1LU package lists Ethernet with lwIP among its samples. [primary, via search summary] ([release notice](https://renesas.com/en/document/tnr/released-web-rz-family-rza1lu-software-package-v310-rza1h-software-package-v110-rza1h-software), [RZ/A1LU package](https://www.renesas.com/us/en/software-tool/rza1lu-freertos-software-package))
  - SEGGER offers embOS, emWin and emWeb evaluation for the RSK RZ/A1H. [primary for SEGGER] ([SEGGER RZ/A1H RSK](https://www.segger.com/evaluate-our-software/renesas/renesas-rza1h-rsk/))
- SMP: not applicable. RZ/A3UL and RZ/A3M are single-core.

### 2.2 AMD/Xilinx Zynq-7000 and Zynq UltraScale+

- Vitis ships a FreeRTOS BSP for the RPU (R5) and APU (A53, 32-bit and 64-bit) [primary] ([UG1137 2020.2 FreeRTOS stack](https://docs.amd.com/r/2020.2-English/ug1137-zynq-ultrascale-mpsoc-swdev/FreeRTOS-Software-Stack)):
  - The stack "is similar to the bare metal software stack, except that it contains the FreeRTOS library".
  - Standalone drivers "do not provide any support for mutexes"; they can be used under FreeRTOS "provided that only a single thread requires access to the device".
- **SMP:** UG1137 2023.1 describes the A53 cluster supporting SMP "using an OS (Linux or VxWorks)" and does not name FreeRTOS [primary, search summary] ([UG1137 SMP](https://docs.amd.com/r/2023.1-English/ug1137-zynq-ultrascale-mpsoc-swdev/Symmetric-Multiprocessing-SMP)). FreeRTOS forum: only a unicore Cortex-A9 port exists, and SMP in Vitis FreeRTOS is "probably not included" (Versal thread). A community "Working on Zynq SMP Port" thread was active June 2025. [secondary] ([Zynq-7000 SMP thread](https://forums.freertos.org/t/how-can-zynq-7000-support-freertos-smp-mode/21538), [Versal thread](https://forums.freertos.org/t/smp-on-the-xilinx-amd-versal/18676))
- **Networking:** GEM + lwIP in RAW (bare metal) and socket (FreeRTOS) modes. 2025.1 added lwIP GEM common MDIO and SGMII support [primary, search summary] ([AMD What's New 2025.1](https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/embedded-software/whats-new.html)). Throughput figures are in section 4.
- **Display:**
  - [inference, unverified] The Zynq-7000 PS has no display controller, so a panel needs a PL video pipeline (VDMA + timing controller IP). This adds FPGA design work and cost that a pure HMI does not need.
  - ZynqMP has a PS DisplayPort subsystem with standalone `dppsu`, `avbuf` and `dpdma` drivers and examples. Changelog dated 2018.1. [primary, search summary] ([ZynqMP Standalone DisplayPort Driver wiki](https://xilinx-wiki.atlassian.net/wiki/x/XoBaLQ))
  - [inference, unverified] The Mali-400 GPU has drivers only for Linux.
- **LVGL:** no vendor LVGL port found.
- **Verdict [inference]:** excellent, mature networking on the A cores, but the wrong cost and display profile for a panel HMI. Useful mainly as a throughput reference.

### 2.3 TI Sitara AM64x / AM243x / AM62x / AM62L

- **AM64x MCU+ SDK:**
  - "A53 NORTOS, A53 FREERTOS and A53 FREERTOS SMP support is a experimental feature" (11.01.00). FreeRTOS SMP kernel 202110.00-SMP on A53 (8.6/9.1). [primary, search summary] ([AM64x release notes](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/09_01_00_41/exports/docs/api_guide_am64x/RELEASE_NOTES_09_01_00_PAGE.html))
  - lwIP tested on A53 with FreeRTOS; LwIP 2.2.0 in 11.02. [primary, search summary]
  - Ethernet performance page (SDK 12.01.00): A53 at 1 GHz, DDR, cache enabled [primary] ([AM64x Enet performance](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/enetlld_performance.html)).
  - [inference, unverified] AM64x has no display subsystem, so it is a gateway/PLC part, not an HMI part. AM243x has no Cortex-A at all (R5F + M4F).
- **AM62x MCU+ SDK 12.00.00** [primary] ([AM62x release notes 12.00](https://software-dl.ti.com/mcu-plus-sdk/esd/AM62X/12_00_00_27/exports/docs/api_guide_am62x/RELEASE_NOTES_12_00_00_PAGE.html)):
  - Supported CPUs "M4F, R5F, A53". FreeRTOS kernel on A53.
  - "Ethernet driver (ENET) | A53 | YES | FreeRTOS". DSS (display) on A53 in the main domain.
  - Caveat: "MCU+SDK on A53 is provided as is for customers as a reference to implement/validate on their own SW Stack & OS. TI will not support these features on the E2E forum."
  - SMP not mentioned.
- **AM62L FreeRTOS SDK 11.02.00.23** (Dec 15, 2025) [primary] ([AM62L release notes](https://software-dl.ti.com/mcu-plus-sdk/esd/AM62LX/11_02_00_23/exports/docs/api_guide_am62lx/RELEASE_NOTES_11_02_00_PAGE.html)):
  - FreeRTOS Kernel 11.1.0 on A53, TF-A 11.00.13.
  - Drivers: DSS, MMCSD/eMMC, I2C, CPSW Ethernet. lwIP tested for TCP/UDP client/server, DHCP, ping and iperf. USB host, GPU, Wi-Fi and LVGL not mentioned.
  - Same "as is… TI will not support these features on the E2E forum" caveat.
  - Known issues include "Application crashes due to time() function". MCU+ SDK examples used "incorrect ARMv7 MPU attributes for the peripheral register region".
- DSS examples (display test, display share with Linux) run on R5F cores on AM62P, not on A53. LVGL not found in any TI MCU+ SDK result. [primary, search summary] ([AM62P DSS display test](https://software-dl.ti.com/mcu-plus-sdk/esd/AM62PX/latest/exports/docs/api_guide_am62px/EXAMPLES_DRIVERS_DSS_DISPLAY_TEST.html))
- **Verdict [inference]:** technically present (A53 + FreeRTOS + lwIP + DSS), but TI labels it reference code without forum support. TI's supported path for AM62x HMI is Linux.

### 2.4 Microchip SAMA5 / SAMA7 (MPLAB Harmony v3)

- Harmony v3 uses FreeRTOS through an OSAL; FreeRTOS "is enabled by default with Harmony v3 drivers, middleware and system services" (CMSIS-FreeRTOS). [primary, search summary] ([Harmony page](https://www.microchip.com/mplab-harmony))
- SAMA7D65 [primary] ([SAMA7D65 press release](https://www.microchip.com/en-us/about/news-releases/products/introducing-sama7d65-microprocessors-available-in-system-in-package?utm_bu=MPU32)):
  - "For customers developing RTOS or bare-metal systems, these MPUs are supported in MPLAB Harmony v3." Cortex-A7 up to 1 GHz; displays up to WXGA/720p.
  - The Curiosity board has on-board GbE RGMII plus a SODIMM PHY slot. The Harmony quick-start shows an iPerf2 Ethernet test. [primary, search summary]
  - LVGL guide covers "MPLAB Harmony-based bare-metal and FreeRTOS applications"; MGS Harmony "leverages the on-chip SAMA7D65 graphics peripherals, like the GPU and LCD controller". [primary, search summary] ([MGS SAMA7D65 guide](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/dev-kits/sama7dg65-mgsh-ug/), [LVGL for Harmony](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/lvgl/mplab-harmony/))
  - A PROFINET IO demo for Harmony v3 runs bare-metal on SAMA7D65 Curiosity. [primary, search summary] ([PROFINET demo](https://www.microchip.com/en-us/tools-resources/reference-designs/profinet-io-demo-for-mplab-harmony-v3))
- SAMA5D2 and SAMA7G5 [primary, search summary]:
  - Harmony 3 TCP/IP applications (Berkeley TCP client/server, UDP, web server with SD card FATFS) for SAMA5D27/D29, SAMA7G54 and SAMA7D65. Bare-metal variants exist; "For bare-metal (non-RTOS) FreeRTOS component should not be selected". ([TCP/IP apps for SAM A5D2](https://onlinedocs.microchip.com/oxy/GUID-DAF7292D-6EA7-4C78-B834-35AD4DAB086E-en-US-2/GUID-387D7670-DDFE-4EA4-ABE3-F55FAFE4BAC4.html), [SAMA7G54 web server](https://microchip-mplab-harmony.github.io/reference_apps/apps_mpu/sama7g54_ek/sama7g54_ek_ethernet_web_server_getting_started/readme.html))
  - The stack is Microchip's own Harmony TCP/IP, not lwIP or FreeRTOS+TCP.
- SAMA5D2 graphics: `gfx_apps_sam_a5d2` has Legato and "blank" (third-party library) quickstarts, all marked beta, at gfx v3.12.0. [primary] ([GitHub gfx_apps_sam_a5d2](https://github.com/Microchip-MPLAB-Harmony/gfx_apps_sam_a5d2))
- Harmony Azure RTOS v1.0.0 (Sep 2021, beta): "initial Azure RTOS ThreadX support for Cortex-A7 family of 32-bit MPUs"; kits SAM9X60-EK, SAMA5D2 Xplained Ultra, SAMA7G5-EK. No later release found. [primary] ([Harmony azure_rtos release notes](https://microchip-mplab-harmony.github.io/azure_rtos/release_notes.html))
- Upstream Zephyr `sama7d65_curiosity` (Cortex-A7) [primary] ([Zephyr board page](https://docs.zephyrproject.org/latest/boards/microchip/sam/sama7d65_curiosity/doc/index.html)):
  - Supported: GMAC Ethernet + MDIO + LAN8840 PHY, USART, I2C, GPIO, USB device (UDPHS), CAN, crypto, PWM, RTC.
  - **No display, touch, SD/SDHC or USB host entries.** Boots via at91bootstrap from SD.
- SAMA5D3 / SAMA5D4: no Harmony v3 support found. Oryx CycloneTCP lists their Ethernet MACs [secondary] ([Oryx CycloneTCP](https://oryx-embedded.com/cyclone_tcp.html)). [inference, unverified] They are left with the old Atmel bare-metal "softpack" and third-party stacks.
- Aside: SAM9X60 / SAM9X75 are ARM926EJ-S (ARMv5), not Cortex-A. Harmony supports them bare-metal with GFX2D + XLCDC acceleration for SAM9X75. [primary, search summary] ([SAM9X75 MGS guide](https://developerhelp.microchip.com/xwiki/bin/view/software-tools/mgs/dev-kits/sam9x75-ug/))
- **Verdict [inference]:** the most "MCU-like" Cortex-A vendor stack after Renesas. One configurator (MCC/Harmony) gives bare-metal or FreeRTOS, Ethernet + TCP/IP, LCD + GPU, LVGL. All parts are single-core, so no SMP question.

### 2.5 NXP i.MX

- i.MX 6 / 6ULL:
  - A 2017 "Getting Started with MCUXpresso SDK for i.MX 6ULL Derivatives" guide (MCIMX6ULL-EVK, FreeRTOS kernel layer) exists [secondary: NXP PDF hosted on the NXP community] ([PDF](https://community.nxp.com:443/pwmxy87654/attachments/pwmxy87654/imx-processors/146742/1/Getting%20Started%20with%20MCUXpresso%20SDK%20for%20i.MX%206ULL%20Derivatives.pdf)). lwIP/ENET/eLCDIF content not confirmed.
  - NXP marketing lists "QNX, VxWorks, Integrity" RTOS support for i.MX 6. [primary, search summary] ([NXP DWF14 slides](https://www.nxp.com/docs/en/supporting-information/DWF14_AMF_IND_T0039.pdf))
- i.MX 7: [inference, unverified] RTOS only on the Cortex-M4; NXP gives no A7 RTOS.
- i.MX 8M Mini / 8M Plus / 93 / 95 / 943, through **Real-Time Edge Software** [primary] ([Real-Time Edge page](https://www.nxp.com/design/design-center/software/development-software/real-time-edge-software:REALTIME-EDGE-SOFTWARE)):
  - Latest is 3.5 (July 2026): "Native Zephyr v4.4 and Native FreeRTOS v11.2.0 on Cortex-A cores".
  - Bare-metal images for i.MX 93, 8M Mini and 8M Plus EVKs.
  - "lwIP networking stack for A-Core RTOS: i.MX 943, i.MX 95". Earlier releases: "LwIP on Cortex-A Core (ENET on i.MX 93 and i.MX 8M Mini)" and ENET on i.MX8MP [search summary].
  - Jailhouse partitioning; RPMSG with Linux; "Multicore Fastboot for A-Core RTOS: i.MX 93".
  - **Display is not mentioned** for the A-core RTOS.
- Zephyr `imx93_evk/mimx9352/a55` [primary] ([Zephyr i.MX93 EVK](https://docs.zephyrproject.org/latest/boards/nxp/imx93_evk/doc/index.html)):
  - ENET (MAC, MDIO, PTP) + RTL8211F PHY; uSDHC (SD and eMMC); EHCI in device mode.
  - LCDIFv3 and MIPI DSI listed as on-chip hardware; Zephyr display driver status unverified.
  - Separate `/smp` target. Boot via U-Boot `go`, remoteproc, SPSDK/TF-A, or Jailhouse (SOF variant). The JLink runner needs D-cache disabled in U-Boot.
- **Verdict [inference]:** NXP treats A-core RTOS as a real-time partition next to Linux (industrial control, TSN, fast boot), not as a standalone HMI OS. No vendor display or LVGL path on the A core was found.

### 2.6 ST STM32MP1 / MP2

- **STM32MP13 (single Cortex-A7) has a real vendor bare-metal/RTOS package, STM32CubeMP13** [primary, search summaries of ST wiki pages that timed out] ([CubeMP13 release note v1.2.0](https://wiki.st.com/stm32mpu/wiki/STM32CubeMP13_Package_release_note_-_v1.2.0), [ecosystem release note](https://wiki.st.com/stm32mpu/wiki/STM32CubeMP13_bare_metal_-_RTOS_ecosystem_release_note)):
  - v1.2.0 middleware: ThreadX, NetX Duo, USBX, FileX v6.2.0; LevelX v6.4.0.
  - v1.2.0 "FreeRTOS as middleware (in addition to Azure-RTOS) and LwIp middleware".
  - Examples include "LwIp Dual ethernet" and "USB Host based on USB OTG IP".
  - ST calls it an alternative to OpenSTLinux.
- GitHub `STM32CubeMP13`: "projects are directly running on Cortex-A7 of the STM32MP13 series boards"; middleware ThreadX, FileX, USBX, NetX Duo, USB Host, LwIP, FreeRTOS; board STM32MP135F-DK. [primary] ([GitHub](https://github.com/STMicroelectronics/STM32CubeMP13))
- TouchGFX on MP13 A7 bare metal via the X-TOUCHGFX-MPU add-on (announced Sept 2024, TouchGFX 4.24.0) [primary/secondary mix] ([ST wiki: install X-TouchGFX MPU add-on](https://wiki.st.com/stm32mpu/wiki/How_to_install_X-TouchGFX_MPU_Addon), [ST community thread](https://community.st.com/stm32-mcus-touchgfx-and-gui-33/mp-series-and-gfx-140464)):
  - It is "based on STM32Cube BareMetal Ecosystem".
  - An ST reply says it was developed only for STM32MP13 bare metal. MP15/MP25 bare-metal code runs on the co-processor.
  - No LVGL port from ST found.
- STM32MP15: CubeMP1 targets the Cortex-M4. [inference, consistent with the ST reply above]
- STM32MP2 (Cortex-A35): STM32CubeMP2 targets the Cortex-M33; at runtime the A35 runs OpenSTLinux. [primary, search summary] ([CubeMP2 data brief](https://st.com/resource/en/data_brief/stm32cubemp2.pdf)) On Mar 19, 2026 an "ST Technical Moderator" wrote "We have launch the support of Baremetal and we are in Alpha phase". No GA by May 2026 in the thread. [secondary] ([ST community thread](https://community.st.com/stm32-mpus-embedded-software-and-solutions-46/stm32mp257f-dk-bare-metal-163756))
- **Verdict [inference]:** STM32MP13 is the most natural fit for an STM32-centred team. It has the same Cube HAL style and Azure RTOS/FreeRTOS/lwIP as STM32H7/H5 projects, plus dual Ethernet, LTDC and USB host. EHS already generates LVGL for STM32. Porting the EHS runtime to CubeMP13 is closer to "another STM32 target" than to a Linux port. Main risks: single A7 core with no 2D GPU (LVGL software rendering only), and no ST LVGL port.

### 2.7 Rockchip RK3506 / RK3568

- RK3506 (3x Cortex-A7 + Cortex-M0, up to 1.5 GHz) [secondary] ([ArmSoM RK3506 SoM](https://documentation-armsom.vercel.app/blog/rk3506-SoM), [CSDN/GitCode tutorial](https://gitcode.csdn.net/6a17db20662f9a54cb77edbc.html)):
  - Board makers list "Supports RT-Thread 4.1", "Bare-metal programming support" and AMP.
  - Splits: 2x A7 Linux + 1x A7 RTOS + M0 HAL, or 3x A7 RTOS + M0 HAL.
  - In their AMP description, Ethernet, USB, SDIO and display sit on the **Linux** side. The RTOS side does "data acquisition, interface control, and real-time applications".
  - LVGL with RGA acceleration is cited (Linux side).
  - The CSDN tutorial describes RT-Thread SMP with `RT_CPUS_NR 3`; the version differs from ArmSoM's.
- RK3568: AMP with three cores Linux and one core RT-Thread; RK HAL bare-metal library [secondary] ([Hackster YY3568](https://hackster.io/smartfly00/multi-core-boot-process-on-the-rk3568-development-board-873be6), [Forlinx RK3562J](https://www.forlinx.net/article_view_604.html)).
- RT-Thread mainline `bsp/rockchip/rk3500` (RT-Smart 5.2.1, AArch64, SMP boot) marks RK3568/RK3588/RK3576/RK3566/RK3528 as supported and RK3506 as "-". Only serial console documented. [primary for RT-Thread] ([GitHub rk3500 BSP](https://github.com/RT-Thread/rt-thread/tree/master/bsp/rockchip/rk3500))
- Zephyr `roc_rk3568_pc`: only A55 CPU, GICv3, PSCI, UART, timer; SMP target; no Ethernet, display, SD or USB. [primary] ([Zephyr ROC-RK3568-PC](https://docs.zephyrproject.org/latest/boards/firefly/roc_rk3568_pc/doc/index.html))
- No official Rockchip RT-Thread developer guide reached this session.
- **Verdict [inference]:** RTOS on Rockchip A cores is a real-time side partition. Networking and display are Linux jobs in every documented layout.

### 2.8 Allwinner T113 / D1

- T113-S3 (2x Cortex-A7, 128 MB DDR3 in package): "should support the usual Tina Linux … and Melis RTOS, which is based on the RT-Thread open-source RTOS"; the product brief lists no OS. [secondary] ([CNX T113-S3](https://www.cnx-software.com/2022/02/13/allwinner-t113-s3-dual-core-cortex-a7-soc-features-128mb-ddr3-targets-automotive-and-industrial-applications/))
- Sources conflict on Ethernet (GbE vs dual 100M) and GPU (Mali-400 vs none). LVGL + GT9xx touch + G2D rotation shown by a hobbyist, apparently on Linux. [secondary] ([Hackaday T113 log](https://hackaday.io/project/188048-sbc-t113-rgbmipilvdsamoledqspimcu-lcd/log/237379-running-lvgl-and-touch-functions))
- RT-Thread mainline `bsp/allwinner` covers only D1/D1s, which are **RISC-V64**, not Cortex-A. [primary for RT-Thread] ([GitHub](https://github.com/RT-Thread/rt-thread/tree/master/bsp/allwinner))
- **Verdict [inference]:** no public evidence of a Melis Ethernet + TCP/IP stack on T113. Treat as unverified.

### 2.9 Raspberry Pi (reference only)

- Circle (C++ bare metal) [primary for Circle] ([GitHub rsta2/circle](https://github.com/rsta2/circle)):
  - Supports Pi 1-5 (Pi 5 AArch64 only) and multi-core.
  - TCP/IP stack: ARP, IP, ICMP, IGMP, UDP, TCP, with DHCP, DNS, NTP, HTTP, Syslog, MQTT and mDNS clients. TCP congestion control added in Step 51.
  - Pi 5 MACB/GEM GbE and WLAN; USB host (HID, mass storage, touchscreen).
  - Framebuffer; Pi 5 DSI official 5"/7" touchscreens; XPT2046; **LVGL**. Latest 51.1.1.
- Zephyr `rpi_4b`: latest docs list GENET Ethernet + MDIO + PHY. Display hardware is not in the driver table. [primary] ([Zephyr rpi_4b](https://docs.zephyrproject.org/latest/boards/raspberrypi/rpi_4b/doc/index.html))
- [inference] Circle shows that a single-developer bare-metal stack can give Ethernet + MQTT + LVGL + touch on Cortex-A. It is hobby/community grade and has no industrial temperature or long-term supply story.

---

## 3. Commercial HMI and industrial products with an RTOS on Cortex-A-class parts

| Product | Evidence of RTOS | CPU | Label |
| --- | --- | --- | --- |
| Pro-face GP4000 | OS-vulnerability fixes in GP-Pro EX 4.09 are all VxWorks CVEs, including URGENT/11 (from study 18) | Not published; hardware manual and spec page not opened | [inference] from [primary] release notes ([Pro-face readme](https://www.proface.tech/otasuke/download/update/proex/proex/v409/Readme_E_GPProEX_v409450.txt)) |
| Mitsubishi GOT2000 (GT27/GT25/GT23) | The firmware component is called "CoreOS". The 2020 advisory covers the "TCP/IP function included in the firmware" (CVE-2020-5595…5600) rather than a Linux package | Not published | [primary] ([JVN](https://jvn.jp/en/vu/JVNVU95413676/index.html), [Mitsubishi PSIRT 2020-005](https://www.mitsubishielectric.com/psirt/vulnerability/pdf/2020-005_en.pdf)); "not Linux, proprietary RTOS" is [inference] |
| AutomationDirect C-more EA9 | Many CISA advisories (CVE-2025-0960 buffer overflow fixed in 6.80; CVE-2020-6969; ICSA-24-086-01), none naming the OS | Not found | OS unknown; [primary] advisories only ([CISA ICSA-25-035-08](https://www.cisa.gov/news-events/ics-advisories/icsa-25-035-08)) |
| Renesas RTOS-based RZ/A3UL HMI SMARC SoM | Vendor reference design for "real-time HMI … industrial terminals" on FreeRTOS/Azure RTOS | RZ/A3UL | [primary] |
| Microchip SAMA7D65 "high-resolution display solar management HMI" demo | HMI demo on SAMA7D65; OS not confirmed this session | SAMA7D65 | [primary] ([Microchip reference design](https://www.microchip.com/en-us/tools-resources/reference-designs/high-resolution-display-solar-management-hmi-demonstration-application)) |
| Rockwell ControlLogix (PLC, not HMI) | VxWorks on ARM (secondary blog) | ARM | [secondary] ([industrialmonitordirect](https://industrialmonitordirect.com/blogs/knowledgebase/controllogix-firmware-os-vxworks-architecture-deep-dive)) |

[inference] The Japanese incumbents (Pro-face, Mitsubishi) seem to keep proprietary or commercial RTOS firmware with their own TCP/IP stacks. Western and Chinese HMI makers moved to Linux (study 18). The RTOS approach is commercially proven, but those vendors own deep in-house BSPs and stacks.

---

## 4. Measured TCP/IP throughput on Cortex-A (RTOS or bare metal)

| SoC / core | OS / stack | Result | Label / source |
| --- | --- | --- | --- |
| Zynq-7000 ZC702, Cortex-A9 | Standalone lwIP **RAW** | 943 Mbps Rx / 948 Mbps Tx | [secondary] Xilinx wiki transcription of XAPP1026, SDK 2016.3 era ([search summary of wiki](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841917/xapp1026+lwIP+applications)) |
| Zynq-7000 ZC702, Cortex-A9 | lwIP **socket** (FreeRTOS) | 541 Mbps Rx / 572 Mbps Tx | same |
| Zynq-7000 | lwIP socket vs RAW per Xilinx app note | 542 vs 949 Mbps (quoted by a forum poster) | [secondary] ([FreeRTOS forum 2017](https://forums.freertos.org/t/tcp-zynq-performance/6834)) |
| Zynq-7000 | **FreeRTOS+TCP** | 523 Mbps Rx, 620 Mbps Tx (custom iperf); ~550 Mbps with iperf_task | [secondary] same thread, Oct 2017 |
| ZCU102, Cortex-A53 at 1.1 GHz | lwIP RAW | TCP 948/948 Mbps, UDP 944/956 Mbps | [secondary] search summary of Xilinx wiki "Standalone LWIP library" ([page](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842366/Standalone+LWIP+library)) |
| ZCU102, Cortex-A53 | lwIP socket (FreeRTOS perf apps) | TCP Rx 887 / Tx 948 Mbps | same; row labelling uncertain |
| Kria SOM, FreeRTOS lwIP TCP perf server | lwIP socket | 104-132 Mbps (iperf, 5 s intervals) | [primary] ([Kria doc](https://xilinx.github.io/kria-apps-docs/freertos/build/html/docs/freertos_kria_lwip_tcpperfserver.html)); target core not stated |
| TI AM64x, A53 at 1 GHz | FreeRTOS + lwIP, CPSW, DDR cached | TCP Rx 93.5 Mbps at 17% CPU, Tx 93 Mbps at 23%; UDP Tx max 652 Mbps (1470 B) at 100% CPU; UDP Rx 557 Mbps with 128 Rx buffers | [primary] ([AM64x Enet performance, SDK 12.01](https://software-dl.ti.com/mcu-plus-sdk/esd/AM64X/latest/exports/docs/api_guide_am64x/enetlld_performance.html)) |
| TI AM64x, R5F at 800 MHz (comparison) | FreeRTOS + lwIP | TCP Rx 93.3 Mbps at 43%, Tx 93 at 72% | same |
| TI AM62L, A53 at 1 GHz | FreeRTOS + lwIP 2.2.1, RGMII 1 Gbps, HW checksum offload | TCP 93 Mbps each way at 11-14% CPU; UDP Tx 957 Mbps (1470 B) at 100% CPU; UDP Rx max 184 Mbps | [primary] ([AM62L Enet performance](https://software-dl.ti.com/mcu-plus-sdk/esd/AM62LX/latest/exports/docs/api_guide_am62lx/enetlld_performance.html)) |

Notes [inference]:
- TI's TCP numbers sit at ~93 Mbps even on a 1 Gbps link, while UDP reaches 650-957 Mbps. The pages do not explain why, so it is probably a test-setup or window limit rather than a stack ceiling. Do not quote them as "A53 max TCP".
- An HMI doing Modbus TCP polling, MQTT and an occasional project download needs well under 10 Mbps. **Throughput is not the deciding factor.** Robustness under connection churn, TLS (mbedTLS/wolfSSL) and stack security maintenance matter more.

---

## 5. Typical pitfalls (with sources)

1. **Cache coherency between CPU and Ethernet DMA (descriptors and buffers).**
   - The FreeRTOS+TCP Zynq driver reserves 1 MB of SDRAM as uncached memory (`uncached_memory.c`, `pucGetUncachedMemory()`). Descriptors live there; packet buffers are cached or uncached via `nicUSE_UNCACHED_MEMORY`. [secondary] ([FreeRTOS forum: TCP/IP on Cortex-A53](https://forums.freertos.org/t/freertos-tcp-ip-on-arm-cortexa53/9898))
   - In 2024 the maintainer said the driver "works properly with caching enabled", controlled by `uncMEMORY_ATTRIBUTE`; older versions lacked the option. [secondary] ([forum 2024](https://forums.freertos.org/t/freertos-tcp-and-dma-cache-with-zynq7000/21663))
   - Unaligned Rx buffers: `Xil_DCacheInvalidateRange` works on whole cache lines, so a buffer that does not start on a line boundary corrupts packets. Fix by aligning buffers or mapping them non-cacheable through the MMU (1 MB section granularity on A9). [secondary] ([element14 thread](https://community.element14.com/products/devtools/avnetboardscommunity/avnetboard-forums/f/software-application-development/36153/invalidating-the-data-cache))
   - lwIP writing into Rx buffers (in-place byte swap) can race with cache maintenance and overwrite fresh DMA data (Zynq EMACPS, lwIP 1.4.0). [secondary] ([lwip-devel 2017](https://lists.gnu.org:443/archive/html/lwip-devel/2017-12/msg00072.html))
   - TI's A53 setup keeps "TCP/IP packet buffers … cached; Layer 2 buffers are uncached". Vendors choose this split explicitly. [primary] (AM62L performance page above)
   - The same issue applies to the 2D GPU: RZ FSP v4.3.0 makes `d1_cacheflush()` / `d1_cacheblockflush()` overridable for D/AVE 2D. [primary]
2. **MMU setup and memory map.**
   - On A53 the uncached DMA region was recommended to be 2 MB aligned because "the minimum block size in MMU is 2MB" in that port's translation tables. [secondary] (FreeRTOS forum thread above)
   - A DMA failure was traced to the uncached region not matching what FreeRTOS+TCP allocated. [secondary] (same)
   - TI known issue: MCU+ SDK examples used "incorrect ARMv7 MPU attributes for the peripheral register region". [primary] (AM62L notes)
   - Zephyr on i.MX 93 A55 with the JLink runner needs D-cache disabled in U-Boot. [primary] (Zephyr i.MX93 page)
   - [inference] On Cortex-A the RTOS/BSP must build page tables, set device vs normal memory, enable caches and set up the GIC and timers. MCU projects get this "for free" from the core's fixed memory map. Each vendor BSP does it differently, so moving between vendors means re-learning it.
3. **Single-core use of multi-core chips.**
   - Vitis FreeRTOS is unicore (Cortex-A9 port is unicore; SMP "probably not included" in Vitis). UG1137 names Linux or VxWorks for A53 SMP. [primary + secondary]
   - TI A53 FreeRTOS SMP is "experimental". [primary]
   - Zephyr has SMP targets for i.MX 93 and RK3568, but those boards have few drivers. [primary]
   - [inference] Single-core parts (RZ/A3UL/A3M, STM32MP13, SAMA7D65, SAMA5D2) avoid the problem and are the realistic RTOS-HMI targets.
4. **Missing or second-class drivers.**
   - TI: A53 features "provided as is … TI will not support these features on the E2E forum". AM62L notes do not mention USB host, GPU or LVGL. [primary]
   - Zephyr board tables: SAMA7D65 has no display/SD/USB host; RK3568 only UART/timer; RPi 4 no display. [primary]
   - NXP Real-Time Edge: no display on the A-core RTOS. [primary]
   - Renesas RZ/A3M: no Ethernet. RZ/A3UL: no 2D engine. [primary, partly inference]
   - Zynq: no PS display (7000) or DP-only (ZynqMP). [primary + inference]
   - ST MP2 bare metal is alpha. [secondary]
   - [inference, unverified] 3D GPUs on these SoCs (Mali, PowerVR/IMG, GC-series) have no RTOS drivers; only 2D blitters (D/AVE 2D, GFX2D, PXP/G2D) are usable.
   - Wi-Fi on the A-core RTOS is rare: only the Renesas RZ/A2M SDIO Wi-Fi package (on request) and Circle on Pi 5 were found.
5. **Vendor lifecycle churn.**
   - Renesas deprecated `rza-fsp` in Feb 2026 and requires migration to RZ FSP v4 (legacy support until end of Jan 2027). The RZ Smart Configurator path in v4 supports only RZ/T and RZ/N. [primary]
   - Harmony's Azure RTOS for Cortex-A7 is a 2021 beta with no later release found. [primary]
   - [inference] Microsoft handed Azure RTOS to the Eclipse Foundation as Eclipse ThreadX in 2023-24 (unverified this session). Renesas and ST still ship it under the Azure RTOS name, so check update cadence before choosing ThreadX/NetX Duo.
6. **Security updates for the network stack.**
   - FreeRTOS LTS releases get security and critical fixes "for at least two years". 202406 LTS (FreeRTOS-Plus-TCP v4.2.1) is therefore near or past its window in Nov 2026 [inference]. Paid FreeRTOS EMP extends a chosen LTS up to 10 years. [primary] ([AWS LTS announcement](https://aws.amazon.com/about-aws/whats-new/2024/07/freertos-long-term-support-version), [FreeRTOS LTS page](https://freertos.org/lts-libraries.html))
   - Zephyr: project LTS plus commercial options (Canonical offers up to 15 years for Zephyr LTS). [secondary/vendor] ([Canonical blog](https://canonical.com/blog/the-missing-piece-bringing-up-to-15-years-of-support-to-zephyr-for-microcontroller-based-ecus))
   - Embedded stacks do get critical CVEs:
     - AMNESIA:33 hit uIP, picoTCP, FNET and Nut/Net; lwIP, CycloneTCP and uC/TCP-IP had no findings. [secondary] ([Tenable summary of Forescout](https://www.tenable.com/blog/amnesia33-researchers-disclose-33-vulnerabilities-tcpip-libraries-uip-fnet-picotcp-nutnet), [CISA ICSA-20-343-01](https://us-cert.cisa.gov/ics/advisories/icsa-20-343-01))
     - URGENT/11 (VxWorks IPnet) forced Pro-face firmware updates (study 18).
     - Mitsubishi GOT2000's own TCP/IP stack needed CVE fixes. [primary]
   - [inference] With an RTOS, the HMI vendor owns patching of the TCP/IP stack, TLS library and drivers, and must produce its own SBOM and CRA vulnerability handling. On Linux these come from upstream/Yocto with a large community. This is the biggest long-term cost of the RTOS route.
7. **Vendor-specific stacks reduce portability.** Renesas uses FreeRTOS+TCP or NetX Duo (no lwIP on RZ/A). Microchip uses Harmony TCP/IP. TI, Xilinx and NXP use lwIP. ST offers NetX Duo or lwIP. [primary] [inference] An EHS runtime that targets several chips needs its own thin socket abstraction (BSD-socket-like). Modbus TCP / MQTT code should not bind to one stack's API.

---

## 6. What this means for EHS (all [inference])

- **Yes, an RTOS on a Cortex-A core can do networking properly** for HMI loads (Modbus TCP, MQTT, web/VNC-lite, project download). Vendor-supported paths exist today on:
  - Renesas RZ/A3UL (FreeRTOS/ThreadX, 2x GbE, LCDC);
  - ST STM32MP13 (ThreadX/NetX Duo or FreeRTOS/lwIP, 2x Ethernet, LTDC, TouchGFX);
  - Microchip SAMA7D65 / SAMA5D2 / SAMA7G5 (Harmony bare-metal/FreeRTOS, Harmony TCP/IP, LVGL guide).
  Measured lwIP throughput on Cortex-A runs from hundreds of Mbps to near line rate, far above HMI needs.
- **Shortlist for an "RTOS iFace":**
  1. STM32MP135 + CubeMP13: closest to the team's STM32 skills; no 2D GPU.
  2. RZ/A3UL + RZ FSP: Renesas explicitly designs for this; no 2D engine. Its sibling RZ/A3M has 2D + LVGL but no Ethernet.
  3. SAMA7D65 + Harmony: GPU + Ethernet + LVGL guide, all single-vendor.
  - TI AM62x/AM62L (unsupported "as is") and NXP i.MX 93 (A-core RTOS without display) are second tier.
  - Zynq, Rockchip and Allwinner are not recommended for an RTOS HMI.
- **Trade-off against study 18 (Linux):**
  - RTOS gains: sub-second boot (Renesas claims under 1 s), small RAM (RZ/A3M's 128 MB in package), MCU-like determinism, and reuse of EHS's existing C generator and super-loop knowledge.
  - RTOS losses: the large Linux ecosystem (OpenSSL, SQLite, VNC/web servers, OTA frameworks, USB/Wi-Fi drivers, CVE flow). Without it, EHS would own security maintenance of the stack, TLS and drivers per chip vendor (pitfall 6).
  - For an iFace-class product with remote access, alarm history, recipes and OTA, this maintenance load is the main argument for Linux.
  - A plausible split: RTOS-on-A for a low-cost "Basic" panel line (fast boot, Modbus TCP + MQTT only), Linux for the "Advanced" line. LVGL lets the same EHS projects run on both.

## 7. Open items

- Confirm RZ/A3M has no Ethernet (datasheet R01DS0482 pin table).
- Read the RZ FSP v4.3.0 release notes for RZ/A3UL/A3M module lists (lwIP, Wi-Fi, LVGL + DRW acceleration).
- Open the STM32CubeMP13 v1.2.0 package content page (timed out) for LTDC/touch/SDMMC examples and the v1.3 status.
- Check whether Zephyr has an LCDIFv3 display driver for the i.MX 93 A55 target.
- Find the Pro-face GP4000 / Mitsubishi GOT2000 CPU (hardware manuals).
- Find the C-more EA9 OS.
- Find measured lwIP/NetX Duo throughput on RZ/A3UL and STM32MP13 (none found).
