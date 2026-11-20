# Verification of open chip items (study 19: Cortex-A + RTOS vs high-end MCU + RTOS vs Linux)

Date: 2026-11-20. This note checks the open chip questions left in `cortex_a_rtos_support.md` and `highend_mcu_path.md`. Those two notes are unchanged; corrections are listed here.

## Labels and method

- **[primary]**: the vendor's own document. This covers datasheets (text extracted from the PDF), product and part pages, press releases, release notes, the vendor's GitHub repository, and ST's CubeMX MCU database (`STMicroelectronics/STM32_open_pin_data`, the per-package XML files that STM32CubeMX uses). Official LVGL port repositories and docs count as [primary] for LVGL's own software and for the benchmarks LVGL publishes.
- **[secondary]**: distributor listings, forum posts (including ST Community posts by non-staff), trade press, and search-engine summaries of pages I could not open.
- **[inference]**: my own reasoning.

Access notes:
- st.com, wiki.st.com and the Arrow and microchip.com mirrors timed out or returned 403.
- For ST parts I therefore used the CubeMX XML database on GitHub (ST's own data, per package) and a DS14791 copy hosted by RS (the ST datasheet PDF, text extracted).
- I extracted the Renesas datasheets (RZ/A3UL R01DS0487EJ0110, RZ/A3M R01DS0482EJ0110) and the RZ/A intro flyer (R01PF0230EU0100) to text and read them.
- Prices are web snapshots taken 2026-11-20. Quantity tiers are given where the source showed them.

## Summary

| # | Item | Result |
| --- | --- | --- |
| 1 | RZ/A3M Ethernet MAC | **Confirmed: no Ethernet MAC** (datasheet, product page parameter "Ethernet speed: No", EK-RZ/A3M connectivity list) |
| 1 | RZ/A3M price | **New:** Renesas product page US$13.52; part-details page US$15.62; DigiKey US$21.35 @1, US$13.94 @500 |
| 1 | RZ/A3M LVGL draw unit | **Corrected / refined:** the D/AVE 2D driver (`r_drw`) is supported on RZ/A3M in RZ FSP. The official LVGL port does **not** use it: software rendering with NEON, partial mode, 180° rotation. No published Dave2D-accelerated LVGL result on RZ/A3M |
| 2 | RZ/A3UL max resolution | **Resolved:** LCDC hardware spec is WXGA 1280×800 at 60 fps (datasheet). "HD 1280×720" is Renesas marketing wording tied to the DDR option. No published figure for OctaRAM-only builds |
| 2 | RZ/A3UL LVGL | **Confirmed:** LVGL is supported on RZ/A3UL in RZ FSP v4.2.0 (software rendering; no 2D engine, `r_drw` not available on A3UL) |
| 2 | RZ FSP v4 RTOS status | **Confirmed:** FreeRTOS and ThreadX supported on RZ/A3UL and RZ/A3M. FreeRTOS+TCP and NetX Duo are on A3UL only. v4.3.0 released 12 Nov 2026 |
| 3 | STM32H7R/S Ethernet | **Corrected:** all four lines have an Ethernet MAC, but **not every package**. No ETH on VFQFPN68, LQFP100, TFBGA100 or WLCSP101 (these are H7R3/H7S3 packages only). Also new: H7R3/S3 have **no LTDC**, and H7R7/S7 have USB OTG HS only in TFBGA225 |
| 3 | STM32H7R/S prices | **New:** about US$6–9 @1 and US$4–7 in hundreds (DigiKey snapshots) |
| 4 | STM32N6 USB | **Resolved:** 2× USB 2.0 OTG **high-speed with embedded PHYs** (USBPHYC1/2). No ULPI |
| 4 | STM32N6 security | **Confirmed with a correction:** TrustZone on all parts; secure boot ROM ("STiRoT") that authenticates an OEMuRoT; SESIP L3 / PSA certified. **STM32N647 has no SAES/CRYP/PKA/MCE**; N657 has them |
| 4 | STM32N6 LVGL benchmark | **Still unverified for N6.** No published FPS. Nearest published result: STM32H7S78-DK with NeoChrom, 52 FPS average at 800×480 (official LVGL port) |
| 5 | RA8D2 SDRAM / display | **Confirmed:** 32-bit external bus with SDRAM; GLCDC up to 1280×800 (WXGA), parallel RGB + 2-lane MIPI DSI; 2D engine |
| 6 | LVGL + network load benchmark | **Still unverified:** none published. Only qualitative ST staff guidance (Jan 2026) and forum anecdotes of LTDC/Ethernet contention |
| 7 | Cortex-A RTOS boot time | **Still unverified:** no vendor-measured boot-to-display figure for RZ/A3UL, RZ/A2M, STM32MP13 bare metal or SAMA7 Harmony. Only claims ("< 1 s") and partial TI SBL timings |

---

## 1. Renesas RZ/A3M

### 1a. Ethernet MAC: confirmed absent

- **Datasheet** R01DS0482EJ0110 Rev.1.10 (Mar 31, 2025) [primary]:
  - The feature list's "Various communication/storage/network interfaces" are USB2.0 ×1 (Host-Function), I2C ×2, SCI ×2, SCIF ×5 and RSPI ×2.
  - There is no Ethernet, GbE or GMAC entry anywhere in the document; I searched the extracted text.
  - Other items: built-in DDR3L-SDRAM 128 MB, SD host ×1, 2D drawing engine, MIPI DSI ×1 *or* parallel output ×1.
  - Note 1: "This LSI is a SiP … with build-in DDR3L SDRAM, so customers cannot use DDR interface."
  - Product lineup: one part, R9A07G066M04GBG, 128 MB, no security.
  - Source: https://www.renesas.com/en/document/dst/rza3m-group-datasheet
- **Product page** [primary]: the parameter table lists "Ethernet speed: No" with no Ethernet channels. It lists USB 2.0 ×1 (host/device), SD ×1, 7 SCI/UART, 2 SPI, 2 I2C, a 2D graphics drawing engine and no 3D GPU. https://www.renesas.com/en/products/rz-a3m
- **EK-RZ/A3M** [primary]: the LVGL port README for the board lists audio codec, micro-SD, USB host/device, two 40-pin headers, a MIPI display connector and Arduino/Qwiic/Grove/mikroBUS/Pmod connectors. **No Ethernet.** https://github.com/lvgl/lv_port_renesas-ek-rz_a3m
  - Search summaries of distributor and Renesas kit pages also show no RJ45. The RJ45 hits belonged to the RZ/A3UL kit [secondary].
- Result: **confirmed**. The earlier [inference] in `cortex_a_rtos_support.md` §2.1 is now [primary]. A networked RZ/A3M HMI needs a USB-Ethernet or SPI-Ethernet chip (for example W5500/ENC424J600 class) [inference].

### 1b. Price

| Source | Price | Label |
| --- | --- | --- |
| Renesas RZ/A3M product page (structured data), R9A07G066M04GBG#AC0 and #BC0 | US$13.52 (quantity not stated) | [primary] |
| Renesas part-details page, #AC0 | US$15.62 (quantity not stated) | [secondary, search summary] https://www.renesas.com/en/products/rz-a3m/part-details/r9a07g066m04gbg-ac0 |
| DigiKey R9A07G066M04GBG#AC0 | US$21.35 @1, US$13.94 @500 (tray) | [secondary, search summary] https://www.digikey.co.uk/en/products/detail/renesas-electronics-corporation/R9A07G066M04GBG-AC0/26459403 |
| Farnell IE | €16.62 @1+ | [secondary] |
| EK-RZ/A3M (RTK9EKZA3MS10001BE) | Renesas budgetary US$274.12 @1; DigiKey US$246.15 | [secondary, search summary] https://www.renesas.com/en/design-resources/boards-kits/ek-rz-a3m |

A search summary also says Renesas lists #AC1 as the recommended replacement for #AC0. I did not open the page to confirm this [secondary].

[inference] Roughly US$14–16 in volume for an A55 with 128 MB DRAM in the package. For comparison, RZ/A3UL is US$6.91–7.38 on the Renesas page, plus external DRAM or OctaRAM.

### 1c. LVGL draw-unit support for the 2D engine

- **Hardware** [primary, datasheet §1.1.5]: the "2D drawing engine (DRW)" supports frame-buffer formats a8, RGB565, aRGB4444, aRGB1555 and aRGB8888, texture formats from CLUT1 up to aRGB8888 (RLE RGB888), and anti-aliased or blurred edges on any geometry. This is the TES D/AVE 2D core, the same family as the RA "Dave2D".
- **RZ FSP v4.2.0 release** (28 Jul 2026) supported-components table for RZ/A [primary] (https://github.com/renesas/rz-fsp/releases/tag/v4.2.0):

  | Component | RZ/A3UL | RZ/A3M |
  | --- | --- | --- |
  | D/AVE 2D Port Interface (`r_drw`) | – | ✓ |
  | LVGL | ✓ | ✓ |

  - Third-party versions: TES D/AVE 2D 3.8.0, LVGL v9.5.
  - v4.3.0 (12 Nov 2026) made `d1_cacheflush()` / `d1_cacheblockflush()` overridable in `r_drw`.
- **LVGL side** [primary]:
  - The LVGL Renesas "Built-in Drivers" doc describes the Dave2D draw unit (`LV_USE_DRAW_DAVE2D`) as "the GPU of Renesas" but does not name RZ/A3M. https://lvgl.io/docs/open/integration/chip_vendors/renesas/built_in_drivers.mdx
  - The LVGL RZ/A family page lists RZ/A3M as the only supported RZ/A board and does not mention GPU acceleration. https://lvgl.io/docs/open/integration/chip_vendors/renesas/rza_family.mdx
  - Per the docs, the LVGL Dave2D draw unit needs FSP's `bsp_api.h` and `dave_driver.h`. It is therefore technically wireable on RZ/A3M, where `r_drw` exists [secondary for the header detail, via search summary].
- **The official port does not use it** [primary, repository files read via GitHub raw]:
  - `src/lv_conf_user.h` sets `LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NEON` and has no `LV_USE_DRAW_DAVE2D`.
  - `configuration.xml` (FSP 4.2.0, LVGL 9.5.0) has no `r_drw` module. `rm_lvgl_port` is configured with `render_mode_partial` (2 buffers, size = screen/10, in `CACHED_BSS`) and **`display_rotation_180`**.
  - README: "GPU: None"; all-scenes average 28 FPS at 1280×720.
- [inference] The heavy flush time (18 ms of 26 ms) noted in `cortex_a_rtos_support.md` is very likely software 180° rotation plus partial-buffer copies, not a chip limit. Expected gains from direct/double-buffer mode, a panel mounted the right way up and `LV_USE_DRAW_DAVE2D` are unmeasured.
- Result: **refined**. The DRW is supported by RZ FSP on RZ/A3M, but LVGL has no published Dave2D-accelerated configuration or benchmark on RZ/A3M. The README's "GPU: None" result is from software rendering, and that is a choice made in the port.

---

## 2. Renesas RZ/A3UL

### 2a. Maximum display resolution (1280×720 vs 1280×800)

- **Datasheet** R01DS0487EJ0110 Rev.1.10 (Jun 16, 2025), §1.1.7 Display Interface [primary]:
  - LCD controller: one parallel output channel, 2-plane blending, dither (RGB666), clipping and gamma LUT.
  - Input formats: RGB565/666/888, ARGB1555/4444/8888, YCbCr.
  - Parallel output: "**Support WXGA (1280 × 800), 60 fps**", RGB666/RGB888.
  - The datasheet ties this spec to no memory option.
  - https://www.renesas.com/en/document/dst/rza3ul-group-datasheet
- **Product lineup** (datasheet Table 1.1) [primary]:
  - R9A07G063U02GBG: DDR SDRAM interface "Available".
  - R9A07G063U01GBG: DDR "Not supported" (OctaRAM/Octa-flash only).
  - DDR: 1 channel, DDR3L-1333 or DDR4-1600, 16-bit.
  - Octa memory controller: "One each of an OctaFlash device and an OctaRAM device".
- **Where 1280×720 comes from** [primary]:
  - The press release (Aug 2022) says the DDR3L/DDR4 interface is about 10× faster than Octal-SPI, "maximizing the performance needed to enable an HD (1280x720) class display". https://www.renesas.com/us/en/about/press-room/renesas-unveils-powerful-1ghz-rza3ul-64-bit-mpus-rtos-support-enable-high-definition-hmi-and-quick
  - The RZ/A series flyer R01PF0230EU0100 (2022.8) positions "RZ/A3UL External DDR3L/4 RAM: Support HD (1280 x 720)". It describes Octal-SPI RAM as "for simple and low cost PCB design" and DDR3L/DDR4 as "for high resolution HMI and camera use cases". https://www.renesas.com/en/document/fly/introduction-renesas-rza-series
  - The same flyer's "Support WVGA (800 × 640)" box refers to on-chip-RAM parts (RZ/A1/A2M), not to RZ/A3UL with OctaRAM.
- **1280×800** for RZ/A3UL also appears in CNX Software [secondary]. https://www.cnx-software.com/2022/08/05/renesas-rz-a3ul-cortex-a55-cpu-runs-rtos-for-hmi-with-quick-startup-times/
- Result: **resolved**.
  - The hardware maximum is WXGA 1280×800 at 60 fps (datasheet).
  - "HD 1280×720 class" is Renesas's marketing statement for the DDR variant.
  - **No Renesas document found states a maximum resolution for the OctaRAM-only part (U01).**
- [inference] Octal DDR at 200 MHz gives a theoretical 400 MB/s. A 1280×800 RGB565 scan-out at 60 Hz needs about 123 MB/s, before CPU rendering traffic and code/data fetches. OctaRAM-only builds are therefore plausible for WVGA/1024×600 static HMI screens but tight at WXGA. Confirm with Renesas or measure.

### 2b. LVGL support for its graphics

- RZ FSP v4.2.0: LVGL ✓ for RZ/A3UL; `r_drw` "–" for RZ/A3UL. Release note: "Fixed an issue that prevented LVGL stacks from being added to RZ/A3UL projects." [primary] https://github.com/renesas/rz-fsp/releases/tag/v4.2.0
- Datasheet block diagram: Display Interface = LCD Controller (FCPVD, VSPD, DU); Video Processing = ISU (image scaling unit). **No 2D drawing engine.** Product page: "3D GPU: No". [primary]
- No LVGL-published RZ/A3UL board port or benchmark was found. The LVGL RZ/A page lists RZ/A3M only. [primary]
- Result: **confirmed with a qualification**.
  - LVGL is supported through RZ FSP, with software rendering only (NEON assembly available, as on the A3M port).
  - The LCDC's 2-plane blending can composite an overlay layer [inference].

### 2c. RZ FSP v4 RTOS support status

From the GitHub release notes of `renesas/rz-fsp` [primary] (https://github.com/renesas/rz-fsp/releases):

| Release | Date | RZ/A items |
| --- | --- | --- |
| v4.0.0 | 13 Feb 2026 | "Support LVGL" (RZ/A). NEON Q-register save/restore added in FIQ/IRQ handlers. ThreadX port fix. NetX Duo Ether reopen fix. `r_gether` read-overflow fix. ThreadX/NetX Duo 6.4.0, D/AVE 2D 3.8.0 |
| v4.1.0 | 27 Apr 2026 | FreeRTOS+TCP updated to v4.3.3. FreeRTOS static-task stack-size fix. LVGL port tick-callback fix |
| v4.2.0 | 28 Jul 2026 | LVGL v9.5. RZ/A3UL LVGL stack fix. `r_gether` build fix. FreeRTOS kernel V11.1.0 |
| v4.3.0 | 12 Nov 2026 | `r_drw` cache-flush override. FreeRTOS kernel updated to a newer commit. Release page truncated, so RZ/A details may be incomplete |

v4.2.0 RZ/A component table [primary]:

| Component | RZ/A3UL | RZ/A3M |
| --- | --- | --- |
| FreeRTOS | ✓ | ✓ |
| Azure RTOS ThreadX | ✓ | ✓ |
| FreeRTOS+TCP | ✓ | – |
| FreeRTOS+FAT | ✓ | ✓ |
| FreeRTOS+POSIX | – | ✓ |

Ethernet on RZ/A is `r_gether` / `r_gether_phy`.

- Result: **confirmed**.
  - RZ FSP v4.x actively supports FreeRTOS and ThreadX on both RZ/A parts.
  - The networking stacks are present only on RZ/A3UL, which matches item 1a.
  - Legacy `rza-fsp` was archived Feb 2026 (see the earlier note).

### 2d. Prices (for context)

- Renesas RZ/A3UL product page [primary]: R9A07G063U02GBG (DDR) US$7.38; R9A07G063U01GBG (no DDR) US$6.91. Quantity not stated.
- Search summaries [secondary]:
  - Renesas part-details page for U02 #AC0: US$10.23.
  - Future Electronics: US$9.08 @1, US$8.65 @300.
  - DigiKey Canada: CAD 22.54 @1.
  - Lead times 12–14 weeks.

---

## 3. STM32H7R/S family: Ethernet per part number and prices

### 3a. Ethernet MAC and other per-package differences

Source: ST's CubeMX MCU database XML files, one per package [primary]: https://github.com/STMicroelectronics/STM32_open_pin_data/tree/master/mcu. Columns show which IP instances the file declares and the number of ETH pin signals.

| Package (suffix) | H7R3 | H7S3 | H7R7 | H7S7 |
| --- | --- | --- | --- | --- |
| VFQFPN68 (R8V) | **no ETH** | **no ETH** | n/a | n/a |
| LQFP100 (V8T), TFBGA100 (V8H), WLCSP101 (V8Y) | **no ETH** | **no ETH** | n/a | n/a |
| LQFP144 (Z8T), UFBGA144 (Z8J) | ETH | ETH | ETH (Z8J only) | ETH (Z8J only) |
| UFBGA169 (A8I) | ETH | ETH | ETH | ETH |
| LQFP176 (I8T), UFBGA176 (I8K) | ETH | ETH | ETH | ETH |
| TFBGA225 (L8H, L8HxH) | ETH | ETH | ETH | ETH |

Other per-line facts from the same files [primary]:
- **LTDC and GPU2D (NeoChrom) exist only on H7R7/H7S7.** H7R3/H7S3 have DMA2D, JPEG and GFXMMU but **no LTDC**, so they cannot drive an RGB panel directly. ST's AN5996 summary also says LCD-TFT and NeoChrom are R7/S7 only [secondary, search summary].
- USB:
  - H7R3/H7S3 declare USB_OTG_HS in every package except VFQFPN68.
  - **H7R7/H7S7 declare USB_OTG_HS only in TFBGA225**. In UFBGA144/169/176 and LQFP176 they have USB OTG FS only. [inference] The LTDC probably takes the shared pins.
- Security: CRYP, SAES, PKA and MCE1–3 appear only on the **S** parts (H7S3/H7S7). The R parts have HASH only.
- The Ethernet MAC is 10/100. Product pages say "100Mbps (IEEE 1588)" and the H7RS flyer says "1x 10/100 ethernet" [secondary, search summaries].

Corrections to earlier notes:
- `highend_mcu_path.md` §1.1 lists "2× USB HS/FS with PHY" for H7S7/H7R7. That is true only for the TFBGA225 package; other R7/S7 packages have FS only.
- The H7S78-DK uses STM32H7S7L8H6H (TFBGA225, hexa-SPI), so the kit shows the full feature set.
- Distributor attributes are unreliable here. DigiKey lists "Ethernet" for STM32H7S3R8V6 (VFQFPN68), but ST's own database shows no ETH signals for that package [secondary vs primary].

[inference] For an Ethernet + LCD HMI, the usable H7RS parts are H7R7/H7S7 in UFBGA144 (Z8J), UFBGA169, LQFP176/UFBGA176 or TFBGA225. TFBGA225 is needed if USB HS is also required.

### 3b. Prices

Snapshots, all [secondary]:

| Part | Package | Ethernet + LTDC | Price |
| --- | --- | --- | --- |
| STM32H7R7L8H6H | TFBGA225 | yes + yes | DigiKey US: US$8.65 @10, US$6.71 @1,120; lead time 15 weeks. https://www.digikey.com/en/products/detail/stmicroelectronics/STM32H7R7L8H6H/22485807 |
| STM32H7S7L8H6H | TFBGA225 | yes + yes (+ crypto) | DigiKey IE: €10.44 @1, about €6.38 @1,120. https://www.digikey.ie/en/products/detail/stmicroelectronics/STM32H7S7L8H6H/22485833. Earlier note: DigiKey UK £5.91 @160, TME €7.22 @320 |
| STM32H7S7L8H6 | TFBGA225 | yes + yes | DigiKey US: US$9.16 @10, US$7.16 @960 |
| STM32H7R3L8H6H | TFBGA225 | ETH, no LTDC | DigiKey US: US$7.91 @10, US$6.11 @1,120 |
| STM32H7R3V8H6 | TFBGA100 | no ETH, no LTDC | DigiKey CH: US$7.47 @1 |
| STM32H7S3Z8T6 | LQFP144 | ETH, no LTDC | DigiKey UK: £6.65 @1, about £4.06 @540 |
| STM32H7S3I8T6 | LQFP176 | ETH, no LTDC | DigiKey AT: €8.65 @1, about €5.22 @1,000 |
| STM32H7S3L8H6 | TFBGA225 | ETH, no LTDC | DigiKey: €9.09 @1, about €5.52 @960 |
| STM32H7S3R8V6 | VFQFPN68 | no ETH, no LTDC | DigiKey UK: £7.09 @1, £4.30 @500; 0 stock, 52-week lead time |

---

## 4. STM32N6 (N647 / N657)

### 4a. USB speed

- Datasheet DS14791 Rev 2 (Dec 2024), "STM32N647xx STM32N657xx" [primary, ST PDF via RS mirror https://docs.rs-online.com/a1e6/A700000012968342.pdf]:
  - Features list: "2x USB 2.0 high-speed/full-speed device/host OTG controllers (one with UCPD USB Type-C Power Delivery)".
  - §3.52, Table 14: OTG supports HS 480 Mbit/s and FS in host and device mode.
  - §3.53 "USB HS PHY controller (USBPHYC)": "two near-identical instances, namely USBPHYC1 (associated with OTG1), and USBPHYC2 (associated with OTG2)". §5.3.36 gives "Embedded PHY characteristics".
- CubeMX DB, STM32N657X0HxQ [primary]:
  - Declares USB1_OTG_HS and USB2_OTG_HS with dedicated DM/DP/ID pins and OTG1_TXRTUNE / OTG2_TXRTUNE pins.
  - **No ULPI signals.**
  - Both HS instances are also present on N647 and on the VFBGA169/198 packages.
- Result: **resolved**. Both ports are **USB 2.0 high-speed with on-chip HS PHYs**; no external ULPI PHY is needed. The CNX "FS" wording in the earlier note was wrong.
- Also from the datasheet:
  - Ethernet is "10-Mbit, 100-Mbit, and 1-Gbit Ethernet with TSN" (the CubeMX DB shows RGMII signals).
  - LTDC is "up to XGA resolution" (1024×768).
  - Packages: VFBGA142/169/178/198/223/264.

### 4b. Security features

- Datasheet front page [primary]:
  - "Arm TrustZone and securable I/Os memories and peripherals".
  - "SESIP Level 3 …, Arm PSA … certified".
  - RDP life-cycle with password-protected debug.
  - Customer keys in OTP fuses; 1.5 KB OTP.
  - "Secure boot code in ROM, decrypting and authenticating customer uRoT (updatable root-of-trust)".
  - Secure storage with HUK.
  - TF-M secure firmware upgrade support.
  - Two AES coprocessors (one DPA-resistant), DPA-resistant PKA, on-the-fly encryption of external memories, HASH, TRNG (SP800-90B), active tampers.
- **Correction: crypto is not on every part.**
  - DS14791 Table 2 "STM32N647xx features and peripheral counts" lists SAES, CRYP, PKA and MCE1..4 as **"No"** for N647 [primary].
  - CubeMX DB agrees: N647 and N645 have HASH but no CRYP/SAES/PKA/MCE; N657 and N655 have all of them [primary].
  - So N647 = NPU without crypto accelerators; N657 = NPU with crypto. TrustZone is present on both (SAU, Cortex-M55 S/NS contexts in CubeMX).
- ST wiki, from search summaries [secondary, wiki.st.com timed out]:
  - The N6 boot ROM "can be classified as an ST immutable Root of Trust (STiROT) and acts as a first boot stage".
  - The documented boot path is boot ROM → **OEMuRoT** (MCUboot-based, stored in external flash, code in STM32CubeN6) → application.
  - OEMs provision a hash of their public signing keys into OTP words 160–167.
  - No N6 page using the name "OEMiRoT" was found. On N6 the immutable stage is the ROM itself, and the OEM stage is updatable.
  - https://wiki.st.com/stm32mcu/wiki/Security:Security_features_on_STM32N6_MCUs, https://wiki.st.com/stm32mcu/wiki/Security:OEMuRoT_for_STM32N6
- ST publishes UM3451 "STM32N6xx security guidance for SESIP level 3 certification" [primary, title only]. https://www.st.com/resource/en/user_manual/um3451-stm32n6xx-security-guidance-for-sesip-level-3-certification-stmicroelectronics.pdf
- Result: **confirmed** (TrustZone, ROM secure boot, MCUboot-based OEMuRoT, SESIP3/PSA). **Corrected**: N647 lacks the crypto accelerators and memory encryption. Choose N657 if encrypted external flash (XIP) and hardware crypto are needed.

### 4c. Published LVGL benchmark on N6

- No published LVGL FPS figure for STM32N6 was found:
  - The official LVGL STM32 ports (lvgl GitHub org) cover F429/F469/F746/F769, H745, H7B3, H7S78-DK, U5G9J-DK2, two Riverdi H7 boards, Riverdi U5 and **STM32MP135F-DK bare metal**. There is **no N6 port** [primary].
  - The LVGL NeoChrom doc names H7S78-DK, U5G9J-DK2 and Riverdi U5 as ready ports, plus a `cortex_m55` Nema library build. That build is what an N6 project would link. N6 is not named [primary]. https://lvgl.io/docs/open/integration/chip_vendors/stm32/neochrom.mdx
  - LVGL forum: a user runs LVGL 9.5 + FreeRTOS on the N6570-DK, single 800×480×2 framebuffer, and reports that the benchmark runs but tears. No FPS given [secondary]. https://forum.lvgl.io/t/how-to-enable-freertos-with-lvgl-9-5-running-on-stm32n6570-dk-board/23839
  - Community GitHub repos (Zephyr + LVGL on N6570-DK, Nov 2026) publish no numbers [secondary].
  - ST markets NeoChrom as giving "60 FPS" (TouchGFX) and says N6 can "run up to 1280 x 720 GUIs using internal SRAM only" [primary marketing, via search summary]. The datasheet LTDC spec is XGA.
  - The LVGL blog "LVGL vs TouchGFX vs Qt for MCUs" (July, © 2026) is on STM32U5G9J-DK2 at 800×480. It reports CPU % and render ms, not FPS, and runs no network load [primary]. https://lvgl.io/blog/insight-gui-framework-benchmark-lvgl-touchgfx-qt
- **Closest published NeoChrom LVGL figure (new):** the official `lv_port_stm32h7s78-dk` README [primary] (https://github.com/lvgl/lv_port_stm32h7s78-dk):
  - Setup: 800×480, RGB565, double-buffered direct mode, LTDC driver, NeoChrom; framebuffers in 32 MB external PSRAM.
  - All-scenes average **52 FPS**, 65% CPU, 11 ms (7 ms render + 4 ms flush). Widgets demo 33 FPS. Screen-sized text 29 FPS. Containers with overlay 30 FPS.
  - This **supersedes** the 27 FPS forum figure in `highend_mcu_path.md` §3, which was for DMA2D-only rendering.
- Result: **still unverified for N6**. [inference] An N6 at 800 MHz with framebuffers in internal SRAM should match or beat the H7S7's 52 FPS, but this is untested.

---

## 5. Renesas RA8D2: SDRAM and display resolution

- RA8D2 product page [primary] (https://www.renesas.com/en/products/ra8d2):
  - External memory: "32-bit external memory I/F (CS/SDRAM)"; parameter "DRAM I/F: SDRAM"; "xSPI-compliant Octal SPI with XIP & DOTF".
  - Graphics: "GLCDC w/ RGB & MIPI DSI I/Fs", RGB888, 0 or 1 DSI channel depending on part, "2D drawing engine", "high-resolution TFT-LCD displays up to WXGA".
  - Networking: "Gigabit Ethernet, TSN switch, USB2.0 HS/FS, CAN FD"; Ethernet 10M/100M/1G.
  - Prices, quantity not stated: R7KA8D2KFLCAB#UC0 (224-LFBGA) US$17.37; R7KA8D2KFLCAC#UC0 (289-LFBGA) US$19.63; other variants listed at US$9.53–12.18 (out of stock).
- Press release, Oct 22, 2025 [primary]: "supports up to 1280x800 displays with both parallel RGB and 2-lane MIPI DSI interfaces"; dual GbE MACs with DMA plus a 2-port TSN switch; 224- and 289-pin BGA. https://www.renesas.com/en/about/newsroom/renesas-adds-two-new-mcu-groups-blazing-fast-ra8-series-1ghz-performance-and-embedded-mram
- Result: **confirmed**. A real SDRAM controller exists (32-bit CS/SDRAM bus), so the EK's 64 MB SDRAM is not a workaround. Maximum display is WXGA 1280×800 over RGB or 2-lane DSI.
- Open: whether the 224-pin package brings out the full 32-bit SDRAM bus and DSI together. Check the datasheet pin tables.

---

## 6. LVGL (or TouchGFX) with simultaneous network load on an MCU

- **No published benchmark found** that measures GUI frame rate and TCP/IP throughput at the same time on STM32, i.MX RT, RA8 or ESP32 [searches: standard + two extended].
- Closest vendor material:
  - **ST Community knowledge article** "LwIP integration in a TouchGFX project: recommendations and pitfalls". Author labelled "ST Employee", Jan 29, 2026, STM32H750B-DK [primary-ish: ST staff, community platform]. https://community.st.com/t5/stm32-mcus/lwip-integration-in-a-touchgfx-project-recommendations-and/ta-p/866743
    - Memory layout:
      - lwIP heap in SRAM2.
      - ETH DMA descriptors at the start of SRAM3 (0x30040000 / 0x30040100 / 0x30040200), non-cacheable, shareable, bufferable.
      - A dedicated `.lwip_sec` linker section in RAM_D2 ("CubeMX alone is insufficient").
      - 32-byte aligned and kept apart from framebuffers.
    - lwIP thread priorities set deliberately against TouchGFX. TCP_WND/SND_BUF 5840, MSS 1460.
    - It states that "Ethernet DMA and LTDC share the memory bus, which can cause contention" but gives **no throughput or FPS numbers**.
  - **ST AN4861** (LTDC):
    - The LTDC is an autonomous AXI/AHB master.
    - On H7, LTDC bursts that cross 1 KB boundaries are split, which lowers SDRAM efficiency and can cause underrun.
    - It recommends spreading masters across memories.
    - [primary, via search summary of st.com chunks] https://www.st.com/content/st_com/en/technical-documents/an4861-introduction-to-lcdtft-display-controller-ltdc-on-stm32-mcus-stmicroelectronics_asset/chunk-007-topic-an4861-ltdc-synchronization-with-dma2d-and-cpu.html
  - **TouchGFX docs for STM32H7R7/S7** give the LTDC bandwidth formula (pixel clock × bpp × layers) and point to optimisations when external-RAM bandwidth is near its limit [primary, search summary]. https://support.touchgfx.com/docs/development/scenarios/running-graphics-on-stm32h7r
- Field anecdotes [secondary]:
  - **STM32F767 + 800×480 LTDC** (2020): iperf about 200 kbit/s with LTDC on vs about 50 Mbit/s with LTDC off. Lowering the pixel clock from 26 to 7 MHz restored about 50 Mbit/s. The poster later suspected 25 MHz crystal EMI. No ST answer and no confirmed root cause. https://community.st.com/t5/stm32-mcus-touchgfx-and-gui/problem-with-ethernet-bandwidth-on-custom-board-with-lcd/td-p/156000
  - **STM32H743 + TouchGFX 4.25 + lwIP + SDRAM framebuffer** (2025): Ethernet unstable only after adding TouchGFX; AXI contention suspected. https://community.st.com/t5/stm32-mcus-embedded-software/ethernet-communication-unstable-after-integrating-touchgfx-on/td-p/829177
  - ESP32-S3 + W5500 (Hackster): under artificial CPU stress, lwIP throughput fell about 49% vs about 25% for a hardware TCP offload chip. No GUI in the test. https://hackster.io/tjr0927/esp32-s3-wiznet-w5500-toe-vs-lwip-iperf-performance-test-274510
- Result: **still unverified**. This is the gap the earlier note flagged, and it is now confirmed as a gap.
- [inference] EHS's planned "LVGL benchmark + iperf + TLS" test would be the first public data point. Put ETH descriptors and buffers in a different SRAM bank from the framebuffers, and measure LTDC underrun flags as well as FPS.

---

## 7. Measured boot time of a Cortex-A RTOS HMI

| Platform | What was found | Label |
| --- | --- | --- |
| Renesas RZ/A3UL | Only the claim "start up instantly, in less than a second after boot-up" (product page and press release); the Renesas blog says it is "expected to reduce startup time". No measured ms figure, test conditions or app note found | [primary claim] https://www.renesas.com/en/products/rz-a3ul |
| Renesas RZ/A2M / RZ/A1 | Flyer claims "fast boot time"; boot app notes (serial flash R01AN4333, HyperFlash) describe the mechanism but give no timings | [primary claim] |
| ST STM32MP13 bare metal (CubeMP13) | No boot-time figure. ST's boot-time wiki covers Linux and says figures are "illustrative". An ST community post (Aug 2024) said display drivers were missing for MP13 bare metal | [secondary, search summaries] |
| Microchip SAMA7/SAMA5 Harmony | No bare-metal boot-to-first-frame figure. Microchip's Linux Ultra-Fast Boot claims a cold start under 5 s with snapshot (vs 5–10 s optimised standard); the page returned 403 | [secondary, search summary] |
| TI AM62x MCU+ SDK (A53 FreeRTOS) | The SDK datasheet page has SBL timing tables. Search summaries quote SBL Stage1 totals of 45.6 ms and 75.1 ms in different versions. One E2E user measured about 600 ms from OSPI SBL boot to A53 "hello world" (TF-A runs before the RTOS) | [primary page, numbers via search summary; E2E secondary, page 403] |
| TI AM62L | TI E2E reply: "at present, we do not have a benchmark for FreeRTOS boot time"; expected to be lower than other AM6x because the ROM loads the application directly (two-stage) | [secondary, search summary] |
| NXP i.MX 93 | No RTOS-on-A55 fast-boot figure found. Linux reduced to 1.8 s by a third party (Hackster/Elektor) | [secondary] |

Result: **still unverified**. No vendor publishes a measured power-on-to-first-frame time for an RTOS HMI on a Cortex-A part.

[inference] The best public anchor is "sub-second" (Renesas claim), with ROM + bootloader stages around 50–600 ms on TI parts before the application starts. A GPIO-toggle measurement on EK-RZ/A3M or STM32MP135F-DK would close this item. ST's boot-time wiki describes that method for Linux.

---

## Additional findings relevant to the two earlier notes

1. **The LVGL bare-metal port for STM32MP135F-DK exists** [primary] (https://github.com/lvgl/lv_port_stm32mp135f_disco_bm). It corrects `cortex_a_rtos_support.md` §2.6, "No LVGL port from ST found": the port is LVGL's, not ST's.
   - Cortex-A7, double-buffered direct LTDC driver, software rendering, 480×272 RGB565.
   - All-scenes average 55 FPS, 45% CPU. The binary is under 2 MB and boots from SD.
2. **Official LVGL port for the 10.1" Riverdi STM32H757 (1280×800)** [primary] (https://github.com/lvgl/lv_port_riverdi_101-stm32h7):
   - Direct mode, two full-screen RGB565 framebuffers in 8 MB external SDRAM (32-bit), DMA2D.
   - The README has a benchmark video but no FPS table in text. It does confirm that a WXGA LVGL port on an H7 exists.
3. **RZ/A3M vs RZ/A3UL split is confirmed in primary sources.** A3M has the 2D engine + 128 MB DRAM in package and no Ethernet. A3UL has 2× GbE + DDR/OctaRAM and no 2D engine. No RZ/A part combines Ethernet with the 2D engine.
4. **STM32N6 LTDC is "up to XGA"** in the datasheet. Treat 1280×800 on N6 as unconfirmed by the datasheet, even though ST marketing mentions 1280×720 GUIs.
