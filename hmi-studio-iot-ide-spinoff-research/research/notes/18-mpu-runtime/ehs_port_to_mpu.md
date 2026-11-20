# EHS on MPU: moving the runtime from STM32 firmware to Linux (and Windows) on Cortex-A / x86

Notes date: 2026-11-20. Scope: what it would take to turn EDT HMI Studio (EHS, local repo `edt-hmi-studio`, version 0.9.2-dev) into the designer plus runtime of an industrial HMI product in the style of NOVAKON iFace Designer, running on Cortex-A or x86 under Linux (optionally Windows).

Method: read-only exploration of the EHS repo (paths below are relative to its root), the earlier notes in this research folder (01, 09, 11, 13), and web searches. Prices come from distributor listings and press articles and change often; treat them as order-of-magnitude. **Everything labelled "Inference" is my judgement, not a measured or quoted fact.**

Legal reminder from note 01: the current EULA (`src/legal/legalText.ts`, `installer/licence/EULA.txt` §1(a)) limits the software and runtime to EDT-supplied hardware. A runtime sold for third-party or own MPU hardware needs that changed with EDT first. These notes do not repeat that analysis.

---

## 0. Short answer

1. **Most of EHS carries over.** The designer (React/Vite, about 594k lines incl. tests), the `.ehsp` project model and migrations, the widget set, the Logic graph, the C code generator (`src/codegen`, about 32.6k lines of non-test TS) and LVGL 9.5 itself are all platform-neutral. The proof is already in the repo: the **Emulator** compiles the generated C *plus the firmware runtime* (`firmware/common/src/*.c`) with emcc into WASM (`vite-plugin-emulator.ts:78-80`). A program that builds with emcc/clang against a POSIX-like host builds for Linux with little change.
2. **What must be redesigned** is not the UI path but the product shape around it: a per-project firmware image becomes a *fixed runtime* that loads a project; the bare-metal super loop becomes a multi-threaded Linux process; the float-only, one-device Modbus tag model becomes a typed, multi-device tag database; and MCU storage (external flash, SD, retained values) becomes files, SQLite and an A/B OS image with OTA.
3. **Runtime recommendation: option (c), "fixed runtime + precompiled project module".** The designer keeps generating C, but compiles it *on the PC* with a bundled cross compiler into a shared library (`.so` on Linux, `.dll` on Windows) for a stable runtime ABI. The project data (tags, devices, alarms, recipes, users, logging) moves out of C into a data file the runtime loads. This keeps EHS's single source of truth (generated C, the same code the Emulator runs), needs no toolchain on the device or in the user's face, downloads in seconds over Ethernet, supports hot swap, and the same runtime built for x86 *is* the PC simulator. A JSON/XML interpreter runtime (option b) is a long-term option for screens only, not the first step. Details in §2.
4. **Linux stack:** Buildroot (Rockchip, Allwinner) or Yocto (NXP, TI, ST vendor BSPs) for the product; vendor Debian only for prototyping. LVGL on DRM/KMS + evdev, software rendering with NEON first, 2D accelerators where LVGL has a draw unit. Read-only A/B rootfs with **RAUC**. Boot target: logo < 1 s, first screen < 8 s, stretch < 3 s.
5. **Windows:** build the runtime for Windows anyway, because it doubles as the offline PC simulator; sell it as a runtime licence for customer-supplied panel PCs. Do not design own Windows hardware: Windows IoT Enterprise LTSC 2024 adds roughly US$90–300 per unit (tier-dependent, see §3.6).
6. **Effort (Inference):** an MVP (one SoM, Modbus RTU/TCP + one more PLC family, alarms, trends, recipes, users, OTA, download) is about **9–14 person-months** for one experienced engineer with Claude Code; a product at iFace-like feature breadth is about **22–38 person-months**. The long poles that AI does not shorten are PLC interoperability testing on real hardware, certification (CE/UL/FCC, ODVA, CRA processes), and BSP bring-up and boot/thermal tuning (§6).

---

## 1. What carries over, what is ported, what is redesigned

### 1.1 What the code looks like today (verified in the repo)

| Area | Where | Facts relevant to an MPU port |
| --- | --- | --- |
| Designer | `src/` (React 19, Vite 7, TS 5.9), `server/` (Node 24 services), `desktop/` (.NET + NativeWebHost; WebView2 / WebKitGTK / WKWebView) | Already builds for Windows, Linux, macOS (`README.md` §desktop, `.github/workflows/desktop-packages.yml`). |
| Project file | `src/ehsp/` (container), `src/project/format/` (version gate, migrations) | `.ehsp` = 4 KB header + card + chunks + media. Inside: `project.ehsp.json` manifest, `design/project.json` (the whole design as JSON), `assets/index.json` + binary assets (`src/ehsp/projectCodec.ts:1-30`). `target {board, title, virtual}` is nearly board-neutral. |
| Code generator | `src/codegen/` (`generator.ts` emits `ui.h/.c`, `ui_events.h/.c`, `ui_logic.h/.c`) | Templates: `ui.c.ts` 10,116 lines, `ui_events.c.ts` 1,635, `ui_logic.c.ts` 2,020. Generated C talks to LVGL directly and to a small runtime API (`hmi_tag_read/write`, `hmi_video_attach`, label helpers). Very little board knowledge leaks into the templates; media go through hooks such as `hmi_video_attach`. |
| Logic | Logic graph compiled to `ui_logic.c`; node list in `src/project/format/saved-shape.txt` | Includes `custom/c_code_block` and `custom/call_function`: user C can be embedded. No VM on the device. `src/codegen/logicTrace.ts` states the design choice explicitly: debugging must be "the customer's code reporting what it did, not an interpreter in the editor guessing at it". |
| Runtime (shared) | `firmware/common/` about 8.9k lines C; `hmi_runtime.c` alone 4,864 | Table-driven: `hmi_runtime_init(transport, config, tag_descriptors, n, binding_descriptors, n)`. Port layer is thin: `hmi_platform.h` has one function (`hmi_platform_tick_ms`), `hmi_transport.h` has 7–9 byte-stream ops. Protocols: Modbus RTU client/server, a line-based "UART command" protocol, CAN (loopback only). |
| Tag values | `hmi_runtime.h`, `hmi_runtime.c:4784-4860` | Values are `float` (`hmi_tag_read(uint16_t tag, float *value)`); strings read-only; areas are Modbus coil/DI/HR/IR or `INTERNAL`. One link and one device per project (`src/types/connections.ts:4`). |
| Board layer | `firmware/<board>/` (F746, H747, EDT EVK043027B/STM32U599) | `main.c` = `for(;;){ lv_timer_handler(); hmi_memory_watch(); hmi_runtime_task(); }`. Board files: display/LTDC, touch, QSPI/OSPI flash, SD (FatFs), JPEG, MJPEG-AVI video (`firmware/common/video/hmi_avi.c`), audio, USB CDC. `lv_conf.h`: `LV_USE_OS LV_OS_NONE`, colour depth 16 (F746) or 32 (H747, EVK), FreeType/TinyTTF off. |
| LVGL | `firmware/*/scripts/bootstrap-deps.ps1:311` | **LVGL v9.5.0** pinned (commit 85aa60d). Same version on boards, Simulator and Emulator. |
| Build/flash | `server/hmi/service.ts` → `scripts/build.ps1`; STM32CubeCLT; `STM32_Programmer_CLI` over ST-LINK | Windows PowerShell-bound; minutes per deploy (`docs/preview-ladder.md` §1). |
| Previews ("preview ladder", `docs/preview-ladder.md`) | Rung 1 Prototype (Canvas 2D in TS), rung 2 Simulator (`wasm/src/ui_from_json.c`, 2,975 lines, real LVGL fed a UI JSON, **no actions run**), rung 3 Emulator (real LVGL + generated C + runtime, compiled on demand with emcc), rung 4 Deploy | The product chose the Emulator as the only rung users see, because rungs 1–2 are "the editor's opinion of the panel" (§1.1, §8). This matters for the runtime choice in §2. |

### 1.2 Classification

| Part | Verdict | Notes |
| --- | --- | --- |
| Designer UI (canvas, property editor, hierarchy, screens, themes, assistant) | **Carries over** | Add an MPU target type and remove MCU-only budget bars. |
| Desktop shell (.NET + WebView, Linux/Windows/macOS) | **Carries over** | |
| `.ehsp` container, manifest, migrations | **Carries over** | Add a target kind (`mpu-linux`, `windows`) and new sections (alarms, recipes, users, multi-device). Migration machinery exists. |
| Widget set and its codegen (`ui.c`, `ui_events.c`) | **Carries over** | Generated C is board-neutral; colour depth 32 is already supported (H747, EVK). |
| Logic graph and `ui_logic.c` codegen | **Carries over**, extended | Needs typed values (§1.3) and new nodes (alarm ack, recipe load, user login, log). |
| LVGL 9.5 rendering | **Carries over** | MIT. Change `lv_conf.h`: `LV_OS_PTHREAD`, NEON (`LV_DRAW_SW_ASM_NEON`), larger caches, Linux drivers, optional FreeType for full CJK fonts. |
| Font and image conversion | **Carries over**; optional upgrade | Trimmed bitmap fonts still work; on MPU, FreeType/TinyTTF with full fonts becomes affordable and removes charset trimming pain. Images can be files (PNG/JPEG decoded at run time) instead of flash layouts (`server/hmi/imageLayout.ts`). |
| Emulator (rung 3, WASM in browser) | **Carries over** | Remains the in-editor preview. A native Windows/Linux runtime adds a second, more faithful PC simulator (§2.4). |
| Simulator (rung 2, `ui_from_json.c`) | Carries over as is | Seed for a future data-driven screen loader (option b), see §2. |
| `firmware/common` runtime | **Port** | Already compiles with emcc. Linux: `hmi_platform_tick_ms` → `clock_gettime(CLOCK_MONOTONIC)`; transports → termios (with `TIOCSRS485` for RS-485), TCP sockets, SocketCAN. Keep it as the "MCU-compatible core" and grow it. |
| Board layer | **Replace with Linux drivers** | Display → LVGL DRM/KMS (fbdev fallback); touch → evdev/libinput; backlight → sysfs; buzzer/GPIO → libgpiod; audio → ALSA; video → FFmpeg (LVGL `lv_ffmpeg`, LGPL) or GStreamer with HW decode instead of `hmi_avi.c`; SD/QSPI → filesystem; RTC → `/dev/rtc`; watchdog → `/dev/watchdog`. |
| Build and deploy (`server/hmi`, PowerShell, STM32CubeCLT, ST-LINK) | **Redesign** | Becomes "compile project module on PC + transfer over Ethernet/USB stick". No JTAG on the main path. |
| Board list (`SUPPORTED_BOARDS` in `src/types/hmi.ts`, scattered board knowledge) | **Redesign** | MPU targets are described by runtime capability (resolution, rotation, ports, protocol licences), discovered from the device at connect time. |
| Per-project firmware vs fixed runtime | **Redesign** | §2. |
| Super loop vs multithreaded process | **Redesign** | §1.4. |
| Tag model (float, one device, Modbus areas) | **Redesign** | §1.3. |
| Storage (retained values, SD media, external flash) | **Redesign** | §1.5. |
| Industrial features (alarms, trends history, recipes, users, logging, remote) | **New** | §4. |
| OS image, OTA, security hardening, device licence keys | **New** | §3, §4.7. |
| EULA and branding | **Change** | Note 01. |

### 1.3 Tag model redesign

Inference, based on `hmi_runtime.h`/`.c` and what iFace-class products expose:

- Value type: `float` loses integer precision above 2^24 and cannot carry INT64, DOUBLE, strings to write, arrays, or PLC structures (S7 UDT, CIP UDT). Introduce a tagged variant (`bool, i16, u16, i32, u32, i64, u64, f32, f64, string, bytes`), with array length and optional struct path.
- Add **quality and timestamp** per value (good / bad-comm / uncertain / stale), which alarms, trends and OPC UA all need.
- **Many links, many devices per link**, each with its own driver, poll groups and scan rates. Logic stays protocol-agnostic through Read Tag / Write Tag (EHS Decision 1 in `docs/logic-node-taxonomy.md`), which is the most valuable existing design choice here.
- Keep the MCU path compiling: the generator can emit the old float API for MCU targets and the variant API for MPU targets, or the MCU runtime adopts the variant too (it fits in a few hundred bytes per tag).

### 1.4 Concurrency model

Inference. LVGL is not thread-safe; keep all LVGL calls on one UI thread.

- **UI thread:** `lv_timer_handler()` loop with `LV_OS_PTHREAD`; drains a lock-free queue of tag changes and applies them to widgets (the existing binding tables do this already, just from the same thread today).
- **One I/O thread per link** (serial port or TCP endpoint), running poll groups; blocking I/O is fine here. The existing async Modbus RTU client (`modbus_rtu_async_client.c`) can run unchanged in such a thread.
- **Tag database** owned by a core thread or guarded by a short mutex; publishes change events to UI, alarms, logger, OPC UA/MQTT servers.
- **Worker threads:** alarm evaluation, history/logging writer (SQLite, WAL mode), OPC UA server (open62541 has its own loop), MQTT client, VNC server.
- **Logic execution:** generated `ui_logic.c` runs on the UI thread today (it calls LVGL directly). Keep that; long-running Logic (delays) already compiles to timer-driven state machines. Add a "background logic" variant later if needed.
- Process supervision: systemd service with `WatchdogSec`, restart on crash, core dumps to the data partition.

### 1.5 Storage redesign

| MCU today | MPU target |
| --- | --- |
| Images/fonts in external QSPI/OSPI flash at fixed addresses | Files in the project bundle on the data partition; or embedded in the project module |
| Video/audio on SD (FatFs) | Files; FFmpeg/GStreamer playback |
| Retained values (backup RAM/flash; `docs/hmi-memory-and-retention.md`) | SQLite (WAL, `synchronous=FULL` for retained values) or atomic write-rename + `fsync` |
| No history | SQLite for alarm history, trend history, audit trail, data logs; export CSV to USB/network |
| No OS | A/B rootfs (read-only), separate data partition (ext4 or f2fs), factory partition |

Power-fail behaviour is the main risk: choose eMMC with power-loss protection, journalled filesystem, and if possible a power-fail GPIO or supercap so the runtime can flush.

---

## 2. Runtime options

### 2.1 The options

| | (a) Generate C, cross-compile whole runtime per project | (b) Fixed runtime loads project JSON/XML at start (LVGL Pro XML style) | **(c) Fixed runtime + precompiled project module (`.so`/`.dll`) + data file** | (d) Fixed runtime hosts the Emulator's WASM (WAMR AOT) |
| --- | --- | --- | --- | --- |
| How it works | Like today's Deploy, but for Linux: generated C + runtime + LVGL linked into one executable per project | Device runtime parses screens/tags/logic from data and builds LVGL objects; Logic needs an interpreter | Runtime (LVGL, drivers, protocols, alarms, OTA) is installed once and versioned. Designer compiles the generated C into a small shared object against the runtime's exported ABI; tags/devices/alarms/recipes/users go in a data file | The same `.wasm` the browser Emulator runs (LVGL + generated C + runtime core) runs on the device under WAMR, with host imports for display, input, transports, files |
| Reuse of codegen | 100% | Low: the ~14k lines of template semantics must be reimplemented as a C loader, Logic as an interpreter | 100% | 100% |
| Semantic drift between preview and device | None | **High**: two engines (codegen for MCU/Emulator, loader for MPU). EHS already rejected this for rungs 1–2 (`docs/preview-ladder.md` §8) | None | None (identical binary) |
| Custom C blocks (`custom/c_code_block`) | Work | **Do not work** | Work | Work (sandboxed) |
| Download without rebuild | No: needs a full link per project (tens of MB) | Yes, data only | Yes for data edits; code edits need a PC-side compile of a few files (seconds) but no device toolchain | Yes for any change, one artifact for all CPU archs (AOT per arch) |
| Hot update | Restart process | Rebuild screens in place | Unload screens, `dlclose`/`dlopen`, rebuild (or restart process; target < 1 s) | Restart the WASM instance |
| Runtime upgrades independent of projects | No | Yes | Yes, with an ABI version check; designer re-compiles the module from the source embedded in the bundle when the ABI changes | Yes |
| GPU / 2D acceleration | Yes | Yes | Yes | **No** (rendering inside WASM is software only) |
| Licence / tooling risk | None | LVGL XML runtime loader licence is unclear (see 2.3) if LVGL's is used; own loader avoids it | Must ship a cross compiler with the designer (clang or `zig cc`, both permissive) | WAMR Apache-2.0; AOT compiler (wamrc, LLVM) on PC |
| Security | Native code | Data only (best) | Native code: sign bundles, run runtime unprivileged | Sandboxed (best for code) |

### 2.2 Recommendation: (c), with data moved out of C

Reasons (Inference, grounded in the repo):

1. **It keeps EHS's one source of truth.** Screens and Logic are what the generator produces and what the Emulator already runs. EHS deliberately killed "an interpreter guessing" (`logicTrace.ts`, preview-ladder §8). Option (b) would reintroduce it, on the device, which is worse than in the editor.
2. **Download without a device-side rebuild.** The heavy pieces (LVGL, drivers, protocol stacks) are in the runtime. The project module is only the generated `ui*.c` plus converted fonts; the Emulator already builds that incrementally in "seconds to tens of seconds" with emcc (`preview-ladder.md` §1). A native clang build of the same files is similar. The user never sees a toolchain.
3. **Hot update.** Data edits (tag addresses, scan rates, alarm limits, recipe values, users) need no compile at all once those tables are data. Screen/Logic edits swap the module.
4. **PC simulator reuse.** Build the same runtime for x86-64 Windows/Linux with the SDL or Windows LVGL driver; load the same module compiled for x86. That is a real offline simulator with real protocol drivers (it can poll a real PLC from the PC), which the browser Emulator cannot do.
5. **Keeps custom C as the optional escape hatch**, consistent with the CLAUDE.md principle that code may exist but never on the main path.

Concrete design (Inference):

- **Bundle** (`.ehsr`, e.g. a signed zip): `manifest.json` (runtime ABI version, target arch, project id, hash), `project.so` per arch (aarch64, armhf, x86_64; `project.dll` for Windows), `project-data.json` (devices, tags, alarms, recipes, users, logging, schedules), assets (fonts, images, films, sounds), and the source `.ehsp` (for "upload from HMI" and for recompiling after runtime upgrades). Signature with Ed25519 or X.509.
- **ABI**: the module exports `ehs_module_info()`, `ui_init()`, `ui_deinit()`, Logic entry points; it imports only LVGL and the `hmi_*` API from the runtime executable (`-rdynamic` / exported symbol list). Pin LVGL version and `lv_conf` hash in the ABI id; refuse mismatches.
- **Cross compiler**: ship clang + a minimal sysroot (libc headers only; no linking against libc needed beyond what the runtime exports), or `zig cc`, which cross-compiles to `aarch64-linux-gnu`, `arm-linux-gnueabihf`, `x86_64-linux-gnu` and `x86_64-windows` from any host. The repo already installs emsdk for the Emulator (`npm run emulator:setup`), so bundling a compiler is a known pattern.
- **Transfer**: designer discovers runtimes on the LAN (mDNS), authenticates, uploads the bundle over HTTPS/gRPC; also USB stick import. Runtime keeps previous bundle for rollback.
- **Move the tables to data first**: `hmi_tag_descriptors` and `hmi_binding_descriptors` are already generated *tables*, so the step from "C array" to "JSON parsed at start" is small; bindings reference widgets by id, which the module registers at `ui_init`.

### 2.3 Where (b) and (d) fit

- **(b) data-driven screens, later.** If on-device editing, a web-based runtime editor, or "edit without compile" for screens becomes a sales requirement, grow `wasm/src/ui_from_json.c` (already covers ~40 widget kinds) into a full loader and verify it against the generated-C path with screenshot diffing in CI. Do not adopt LVGL Pro's XML runtime without written terms: LVGL's Pro docs say the runtime XML loading engine is not public, and the two published versions of the LVGL XML License contradict each other on whether runtime loading is allowed (see also note 11 §2). Logic would still need an interpreter or a bytecode VM (note 09 §5, note 13 §3).
- **(d) WASM on device: worth a 2-week spike.** It gives one artifact for all CPUs, sandboxing for customer code, and zero drift with the browser Emulator. The cost is losing 2D/GPU acceleration and some CPU (AOT is reported as near native; interpreted is 10–50x slower per note 13). On a 4x Cortex-A55 at 1024x600, software LVGL inside WAMR AOT may well be adequate; measure before deciding.

### 2.4 PC simulator

| Simulator | Fidelity | Use |
| --- | --- | --- |
| Browser Emulator (existing, WASM) | Same generated C, same runtime core; virtual devices | Inside the designer |
| Native runtime on Windows/Linux (new, from option c) | Same runtime binary as the device (other arch), real drivers | "Offline/online simulation" against real PLCs from the PC; also the Windows panel-PC runtime |

---

## 3. Linux stack choices

### 3.1 Build system

| | Buildroot | Yocto / OpenEmbedded | Vendor Debian/Ubuntu image |
| --- | --- | --- | --- |
| Fit | Single-purpose appliance, small, fast boot, simple to learn | Long-lived product lines, many SoMs, layered BSPs; NXP, TI, ST ship Yocto BSPs | Prototyping, internal tools |
| Boot time / size | Best (tens of MB) | Good when tuned | Poor (systemd + many services, GB-sized) |
| Update model | Whole image (pairs well with RAUC/SWUpdate) | Whole image; first-class RAUC/SWUpdate/Mender layers | Package manager (apt), not atomic |
| Compliance tooling | `make legal-info`, CVE reports | SPDX SBOM generation, `cve-check`, licence manifests | Weak for a product |
| Vendor support | Rockchip SDK, Allwinner (Tina), Bootlin `buildroot-external-st` for STM32MP1/MP2 | NXP `meta-imx`, TI Processor SDK, ST OpenSTLinux, Rockchip community layers | Rockchip, TI, MYIR, Forlinx offer Debian images |

Recommendation (Inference): prototype on the SoM vendor's Debian or SDK image; ship on **Yocto** if the SoC is NXP/TI/ST (that is where vendor BSP and security updates live) or **Buildroot** for Rockchip/Allwinner low-cost lines. Keep the runtime distro-agnostic (CMake, few deps: libdrm, libinput optional, sqlite, an SSL library), so the choice can differ per SoM.

### 3.2 LVGL Linux backends (LVGL 9.5)

LVGL 9.5 documents fbdev, DRM, Wayland, X11, SDL, evdev/libinput, and OpenGL/EGL integrations; the DRM driver can bring up EGL when `LV_LINUX_DRM_USE_EGL` is set, and the Wayland driver has SHM and EGL backends.

| Backend | Use |
| --- | --- |
| **DRM/KMS** (+ evdev) | Default for the product: page flipping, vsync, no tearing, no compositor, fastest boot |
| fbdev | Fallback for old BSPs (i.MX6ULL, T113 vendor kernels) |
| Wayland (Weston kiosk-shell) | Only if other apps must share the screen (browser, camera viewer, video overlay) |
| SDL / Windows driver | PC simulator and Windows runtime |
| X11 | Not needed |

### 3.3 GPU and 2D acceleration

Inference: for typical HMI screens at 800x480 to 1280x800, LVGL's software renderer with NEON on 2–4 Cortex-A53/A55 cores is usually sufficient; acceleration matters for large images with rotation/scaling, full-screen gradients, video overlays and 1920x1080. Order of preference:

1. Software + NEON + partial refresh (always available, identical on every SoC).
2. Vendor 2D engines that LVGL supports as draw units (NXP PXP, NXP G2D on i.MX8; VGLite where present). Rockchip RGA and Allwinner G2D need own integration or community code.
3. OpenGL ES draw unit (LVGL 9.x, still maturing; a forum report notes it can be slower for some UIs). Mali (Rockchip, T527) via Panfrost/Panthor or vendor blobs; Vivante on i.MX8M/STM32MP via etnaviv or vendor; PowerVR on AM62x (vendor driver; open Mesa driver still in progress).

### 3.4 Boot time

- The question's assumption (industrial HMIs take ~10–30 s) is plausible for Windows CE/Embedded and Debian-based panels; I did not find measured figures for named competitors, so treat it as unverified.
- Published embedded Linux data points: Forlinx reports LVGL on i.MX93 starting in 3.1 s; Elektor/Hackster report an i.MX93 graphical app brought from 13.2 s to 1.8 s (U-Boot delay removal, starting the app before systemd); NXP's community reports ~1.9 s on i.MX6ULZ; an older ST wiki example on STM32MP1 shows 7.1 s to a Weston UI.
- **Targets (Inference):** splash in U-Boot < 1 s; first interactive screen < 8 s on Yocto/systemd without heroic tuning; < 3 s with Buildroot, falcon mode or trimmed U-Boot, initramfs-less boot and the runtime started early. Faster than every 10–30 s panel is a marketing point worth having.

### 3.5 Read-only rootfs and OTA

Layout (Inference): `boot` (A/B or in rootfs), `rootfs-A`, `rootfs-B` (read-only, squashfs or ext4 ro, optionally dm-verity), `data` (rw: project bundles, SQLite, logs), `factory` (device keys, serial, calibration). `/etc` writable bits via overlay or bind mounts into `data`.

| Updater | Licence | Strengths | Weaknesses |
| --- | --- | --- | --- |
| **RAUC** | LGPL-2.1 | Small, simple A/B, signed bundles (X.509), adaptive/delta (casync), good Yocto and Buildroot integration | No server; bring your own or use hawkBit |
| SWUpdate | GPL-2.0 (core) | Very flexible handlers, Lua hooks, delta, hawkBit and Mender-like suricatta client | More configuration to get right |
| Mender | Client Apache-2.0; open-source server plus paid hosted/enterprise | Full device management server included | Heavier; server operation or subscription cost |

Recommendation (Inference): RAUC for the OS; project bundles updated separately by the runtime (atomic rename + keep previous); Eclipse hawkBit or a simple own update server later for fleet management. CRA note: the EU Cyber Resilience Act's vulnerability-reporting obligations already apply from September 2026 and the main obligations from December 2027, so a signed update path and an SBOM are not optional for EU sales.

### 3.6 Windows on x86

- Windows 11 IoT Enterprise LTSC 2024 (there is no Windows 10 LTSC 2024; Windows 10 IoT Enterprise LTSC 2021 is the last Windows 10). Support runs to January 2034 for 2024 (per secondary source; check with Microsoft).
- Licences are tiered by CPU (Entry / Value / High End). Reseller listings seen: about US$88 (Entry), US$203 (standard); a secondary source quotes US$295 per device. OEM pricing comes from authorised distributors and is not public. An Intel N100/N97 panel would usually fall in the Entry tier (Inference; confirm with a distributor).
- **Worth supporting? (Inference)** Yes as software, no as own hardware:
  - The Windows build of the runtime is nearly free once option (c) exists, because it is the PC simulator.
  - Many machine builders already own Windows panel PCs; a runtime licence for those (iFace sells its runtime for panel PCs too) is extra revenue with no BOM.
  - For own hardware, Linux on N100 adds no OS licence, boots faster, and uses the same OTA design. Windows adds US$90–300, UWF/servicing work and slower boot.

---

## 4. Industrial runtime features and open-source building blocks

Licence notes: LGPL libraries must be dynamically linked (or object files provided) and replaceable; MPL-2.0 is file-level copyleft (changes to MPL files must be published); GPL must be avoided in the runtime process. Verify each licence file before use.

### 4.1 Communication

| Need | Candidate | Licence | Notes (Inference unless cited) |
| --- | --- | --- | --- |
| Modbus RTU/TCP client and server | **libmodbus**; or extend EHS's own `modbus_rtu*.c` | LGPL-2.1+; own code | EHS already has RTU client/server; adding TCP to own code is small and avoids LGPL. |
| Siemens S7 (S7-300/400/1200/1500 via PUT/GET, ISO-on-TCP) | **snap7** | LGPL-3.0 | Mature but quiet upstream; S7-1200/1500 need PUT/GET enabled and non-optimised DBs. S7CommPlus (optimised access, symbolic) has no free C library; an own driver is a large project. |
| Mitsubishi MC protocol / SLMP (3E/4E, binary/ASCII) | No established C library found; MIT-licensed C#/Rust/Python/Node implementations exist (e.g. `melsec_mc`, PlcComm.Slmp); Apache PLC4X documents SLMP | — | Write own in C (protocol is small and documented by Mitsubishi); use those projects as references. |
| Omron FINS (UDP/TCP) | No well-known C library confirmed; small community libraries exist | — | Write own (small protocol). Omron NJ/NX also speak EtherNet/IP CIP. |
| EtherNet/IP | **libplctag** (client: CIP explicit messaging to ControlLogix/CompactLogix/Micro800, also Omron NJ/NX, Modbus TCP) | MPL-2.0 or LGPL-2+ | Use for the HMI-as-client case. **OpENer** (adapted BSD, Rockwell copyright) is an *adapter/server* stack; selling an EtherNet/IP product also needs an ODVA licence and vendor ID. |
| OPC UA client/server | **open62541** | MPL-2.0 | Certified profiles possible but OPC Foundation certification costs money and time. |
| MQTT | **Eclipse Paho C** client; **Mosquitto** (libmosquitto client or broker) | EPL-2.0 / EDL-1.0 (BSD-style) dual | Paho C or libmosquitto for publish of tags/alarms, Sparkplug B optional. |
| CAN | SocketCAN | kernel | EHS has CAN plumbing (loopback only on MCU). |

### 4.2 Alarms and events

Own code: alarm definitions per tag (limits, deadband, delay, bit alarms), states (active/ack/cleared, latched), priority, groups, message text with translations (EHS already has multi-language texts), history in SQLite, alarm banner/list widgets, acknowledge from Logic. Inference: 1–1.5 PM including editor UI.

### 4.3 Trends and data logging

Real-time trend exists (`HMI_WIDGET_TREND`, `trendCode.ts`). Add historical trends: sampled or on-change logging into **SQLite** (public domain), retention policies, export CSV to USB, zoom/pan history widget built on `lv_chart` or a custom canvas.

### 4.4 Recipes

Recipe tables (sets of tag values), load/save/download to PLC, edit on HMI, import/export CSV; stored in SQLite. Own code.

### 4.5 Users, permissions, audit trail

Users and roles with per-widget/per-screen permissions, login/logout, auto-logout, password hashing (Argon2id/bcrypt via libsodium, ISC), audit log of operator actions (FDA 21 CFR Part 11-style is a later premium option). Own code.

### 4.6 Remote access

| Option | Licence | Notes |
| --- | --- | --- |
| **neatvnc** server fed from LVGL's draw buffer (or wayvnc if running under Wayland) | ISC | Recommended; avoid **libvncserver**, which is GPL-2.0. |
| Web remote: stream the framebuffer to a browser (WebSocket + canvas, or noVNC in front of VNC) | noVNC MPL-2.0 | Good for "view on phone" features. |
| Web runtime (run the project in a browser via the existing WASM Emulator, fed by the device's tag server) | own | Interesting differentiator: EHS already compiles projects to WASM, so a "web HMI" view of the same screens is close at hand. |

### 4.7 Other runtime services

Scheduler (time-based actions), e-mail/notification, printing/PDF reports (later), screenshot to USB, multi-language switching (exists), system settings screen (network, time, brightness), device licence keys (runtime options such as OPC UA or VNC unlocked per device: new, ties to the paid roadmap in note 10), watchdog, crash dumps, remote log retrieval.

---

## 5. Candidate hardware

Prices are 1-unit or small-volume module prices from listings or launch articles (2023–2026), in USD. "Est." means I found no listing and give an inference.

| SoC | CPU / graphics | Typical HMI size | Module price seen | Linux BSP quality (Inference unless cited) |
| --- | --- | --- | --- | --- |
| **Rockchip RK3506(J)** | 3x Cortex-A7 + M0, 2D, no 3D GPU | 4.3"–7" low cost | ~$15–20 (ArmSoM CM1 $15; Forlinx FET3506J-C reported $15–20 via reader comments) | Rockchip SDK (Buildroot), newer part, documentation improving; LVGL demos common |
| **Rockchip RK3566 / RK3568(J)** | 4x Cortex-A55 2.0 GHz, Mali-G52, RGA2 | 7"–15.6" | ~$54 (Forlinx FET3568-C 2/16 GB at DigiKey); $60–65 marketplace; industrial RK3568J similar (~€56) | Vendor kernel 5.10/6.1 SDK with Buildroot/Debian; mainline support good for RK3568; widely used in Chinese HMIs |
| **NXP i.MX6ULL** | 1x Cortex-A7 528–900 MHz, PXP, no GPU | 4.3"–7" | Est. $20–35 | Excellent, mainline, long longevity; slow for 1024x600 animation |
| **NXP i.MX8M Mini** | 4x Cortex-A53, GC NanoUltra 3D + GC320 2D | 7"–10" | From $44.90 at volume (Toradex Verdin) | Mature Yocto BSP, mainline good |
| **NXP i.MX93** | 2x Cortex-A55 + M33 + NPU, PXP, no 3D GPU | 7"–10" | ~$39–59 (Variscite DART/VAR-SOM-MX93), $43–49 (MYIR at launch) | Good Yocto BSP; 3.1 s LVGL boot reported by Forlinx |
| **Allwinner T113-S3** | 2x Cortex-A7, 128 MB DDR3 in package, G2D | 4.3"–7" cheapest | ~$14 (MYIR MYC-YT113X at launch) | Tina Linux vendor BSP weaker; good linux-sunxi mainline community |
| **Allwinner T527** | 8x Cortex-A55, Mali-G57 | 10"–15" | Est. $40–60 | Vendor BSP; mainline in progress |
| **ST STM32MP13 / MP15** | 1–2x Cortex-A7 (+M4 on MP15), GC400T on MP157 | 4.3"–7" | Est. $30–60 | Excellent docs (OpenSTLinux, Yocto; Bootlin Buildroot external); same vendor as today's EHS MCUs; slower boot in ST's own Weston example (7.1 s) |
| **ST STM32MP2 (MP25/MP23/MP21)** | 1–2x Cortex-A35 + M33, 3D GPU on MP25 | 7"–10" | Est. $60–100 (chip from ~$15.6 for MP215) | Good, newer; Debian 12 on MYIR MYC-LD25X |
| **TI AM62x** | 1–4x Cortex-A53 + M4F, PowerVR GPU | 7"–15" | Est. $40–80 | TI SDK (Yocto/Debian), strong mainline focus, long longevity; GPU driver vendor-specific |
| **Intel N100 / N97** (x86) | 4 E-cores, UHD graphics | 10"–21" panel PCs | Bare industrial boards ~$90–200; fanless systems ~$225–285 | Linux mainline; Windows option |

Shortlist (Inference):

- **Low-cost line (4.3"–7"):** RK3506 or T113-S3. Cheapest BOM; check that software LVGL at 1024x600 is smooth enough.
- **Main line (7"–10"):** RK3568/RK3566 (price, GPU, availability in Asia) or i.MX93 (NXP longevity, Western customers' trust, PXP only). Pick one first; the runtime stays portable.
- **Keep STM32MP2 in view** as the "same vendor as our MCUs" story and for customers already on ST tooling.
- **High end / Windows:** N100 panel PCs.

Heterogeneous cores (M4/M33/M0 next to the A cores on MP1/MP2, i.MX93, AM62x, RK3506) could run the existing MCU runtime for deterministic serial protocols. Not needed for an HMI; do not add this complexity early.

---

## 6. Effort estimate

**All figures in this section are Inference.** One experienced embedded/Linux engineer, full time, using Claude Code for most code writing, review and tests. Ranges assume the designer and codegen are reused as described and that a SoM with a working vendor BSP is bought, not designed.

| # | Task | PM (low–high) | Comment |
| --- | --- | --- | --- |
| 1 | Linux host runtime skeleton: port `firmware/common` (tick, termios/TCP transports), LVGL DRM/fbdev + evdev, `lv_conf` for Linux, systemd service, watchdog, logging, config | 0.5–1 | The Emulator proves the code compiles off-MCU |
| 2 | Project module pipeline: ABI and export list, bundled cross compiler (clang or zig) for aarch64/armhf/x86_64/win64, bundle format + signing, `dlopen` hot swap, rollback | 1–2 | |
| 3 | Move tags/devices/bindings to a data file; typed tag database with quality/timestamps; multi-link, multi-device; threading model (§1.4); MCU compatibility | 1.5–3 | Touches editor, codegen, runtime |
| 4 | Designer changes: MPU target type, device discovery and download panel (replaces ST-LINK path), multi-device tag sheet, remove MCU budgets for MPU | 1–2 | |
| 5 | Protocols: Modbus TCP (+RTU over Linux serial) 0.5; S7 via snap7 0.5–1; MC/SLMP own 0.5–1; FINS own 0.5–1; EtherNet/IP client via libplctag 0.5–1; MQTT 0.5; OPC UA server/client 1–2 | 4–7.5 | Code only; lab testing is row 12 |
| 6 | Alarms (editor + runtime + history) | 1–1.5 | |
| 7 | Historical trends and data logging (SQLite, export) | 1–1.5 | |
| 8 | Recipes | 0.5–1 | |
| 9 | Users, permissions, audit trail | 1–1.5 | |
| 10 | Remote: VNC (neatvnc) 0.5–1; optional web view of the WASM HMI 1–2 | 0.5–3 | |
| 11 | OS image for the first SoM: Yocto or Buildroot, A/B read-only rootfs, RAUC, boot-time tuning, factory provisioning, secure boot (optional) | 2–4 | Plus 0.5–1.5 per additional SoM |
| 12 | Test infrastructure: PLC simulators in CI, soak tests, power-cut tests, interop runs on real PLCs | 1.5–3 | Calendar-bound, see below |
| 13 | Windows runtime build and PC simulator packaging | 0.5–1 | |
| 14 | Documentation, licences/SBOM, CRA processes, device licence keys | 1–2 | |
| 15 | Spike for option (d) WASM-on-device | 0.5 | Optional |
| | **MVP** (rows 1–4, Modbus + one PLC family from row 5, 6–9 basic, 11 for one SoM, part of 12) | **9–14** | |
| | **Full iFace-class product** (all rows) | **22–38** | |

What Claude Code accelerates well: porting C to POSIX, protocol framing code from published specs, editor UI in the existing React patterns, SQLite schemas, tests, Yocto/Buildroot recipes, documentation. A rough guess is 2–3x on those parts compared with working alone.

What it does **not** accelerate (calendar time and money):

- **PLC interoperability testing.** Needs real hardware: Siemens S7-1200/1500, Mitsubishi FX5U/iQ-R, Omron NX/NJ/CP, Rockwell CompactLogix/Micro800, plus Modbus devices from several vendors. Firmware variants, PUT/GET and optimised-block settings, timeouts, multi-connection limits and edge cases only show on the real devices. Budget weeks per family and a hardware lab (Inference: US$10k–30k of PLCs and licences).
- **Certification and conformance.** CE/UKCA (EMC, LVD or IEC 61010-2-201), FCC, UL/cUL for North America, marine or hazardous-area variants if targeted; ODVA conformance and licence for EtherNet/IP adapters; optional OPC UA certification; EU CRA conformity (reporting obligations active since September 2026, full obligations December 2027). Lab lead times are months.
- **BSP bring-up and hardware validation.** Display timing and touch tuning per panel, backlight/EMI, boot-time tuning, thermal and long-run tests (e.g. 1000 h), power-fail behaviour of eMMC, vendor kernel quirks and their security-update cadence. Mostly waiting on hardware and vendor support.
- **Customer field trials.** Machine builders adopt an HMI line slowly; first real projects expose missing features.

---

## 7. Open questions

1. Is EDT willing to relicense EHS for a non-EDT runtime product (note 01)? Everything above depends on it.
2. Which first SoM: RK3568 (price) or i.MX93 (longevity, Western trust)? Decide with the first target customers.
3. Measure software LVGL performance on the chosen SoM at 1024x600 and 1280x800 with real EHS projects (e.g. `examples/` washer demos) before buying into GPU paths.
4. Spike option (d): EHS Emulator `.wasm` under WAMR AOT on Cortex-A55, frame time vs native.
5. Exact LVGL XML licence terms, if option (b) is ever pursued.
6. Windows IoT OEM price for the Entry tier from a distributor.
7. Confirm boot times of competing panels (iFace hardware, Weintek cMT, Delta DOP) to set a credible marketing target.

---

## Sources

Local (read-only):

- `edt-hmi-studio`: `CLAUDE.md`; `README.md`; `package.json`; `docs/preview-ladder.md` §1; `docs/logic-node-taxonomy.md`; `docs/hmi-memory-and-retention.md`; `src/codegen/generator.ts`, `src/codegen/templates/*.ts`, `src/codegen/logicTrace.ts`; `src/ehsp/format.ts`, `src/ehsp/projectCodec.ts`; `src/project/format/saved-shape.txt`; `src/types/connections.ts`; `firmware/common/include/hmi_runtime.h`, `hmi_platform.h`, `hmi_transport.h`; `firmware/common/src/hmi_runtime.c`; `firmware/stm32f746g-disco/src/main.c`, `scripts/bootstrap-deps.ps1`; `firmware/*/include/lv_conf.h`; `wasm/src/main.c`, `wasm/src/ui_from_json.c`, `wasm/build.sh`; `server/emulator/panelBuild.ts`, `mainWrapper.ts`; `vite-plugin-emulator.ts`.
- This research folder: `01-codebase-coupling.zh-TW.md`, `09-mcu-unit-bus-architecture.zh-TW.md`, `11-mcu-hot-reload.zh-TW.md`, `13-app-layer-separation-and-languages.zh-TW.md`.

Web:

- LVGL XML licence: [LVGL 9.3 docs, XML License](https://docs.lvgl.io/9.3/details/auxiliary-modules/xml/license.html); [LVGL Pro, XML License](https://docs.pro.lvgl.io/xml/license.html); [LVGL Pro, Loading XML at runtime](https://lvgl.io/docs/pro/integration/loading-xml-files.mdx)
- LVGL Linux: [LVGL 9.5 embedded Linux](https://docs.lvgl.io/9.5/integration/embedded_linux/index.html); [LVGL 9.5 DRM driver](https://docs.lvgl.io/9.5/integration/embedded_linux/drivers/drm.html); [LVGL 9.5 Wayland driver](https://docs.lvgl.io/9.5/integration/embedded_linux/drivers/wayland.html); [LVGL forum, OpenGL ES performance in 9.3](https://forum.lvgl.io/t/what-is-the-performance-of-lvgl-v9-3-opengl-es-rendering/21917); [LVGL 9.5 changelog](https://docs.lvgl.io/9.5/CHANGELOG.html)
- Boot time: [Forlinx, 3.1 s LVGL on i.MX93](https://www.forlinx.net/article_view_652.html); [Elektor, i.MX93 boot to 1.8 s](https://www.elektormagazine.com/labs/how-we-reduced-linux-boot-time-on-imx93-to-18s); [Hackster, i.MX93 1.8 s](https://www.hackster.io/the-embedded-kit/how-we-reduced-linux-boot-time-on-imx93-to-1-8s-c32ecf); [NXP community, i.MX6ULZ under 2 s](https://community.nxp.com/t5/i-MX-Processors-Knowledge-Base/i-MX-6ULZ-How-to-boot-Linux-in-under-2s/ta-p/2196625); [ST wiki, boot time optimisation](https://wiki.st.com/stm32mpu/wiki/How_to_optimize_the_boot_time); [Bootlin buildroot-external-st](https://bootlin.com/blog/tag/buildroot/page/2/)
- Windows IoT: [Microsoft, IoT Enterprise licensing](https://learn.microsoft.com/windows/iot/iot-enterprise/commercialization/licensing); [CDW, Win 11 IoT Enterprise LTSC 2024](https://www.cdw.com/product/windows-11-iot-enterprise-ltsc-2024-license-1-license/8025005); [CDW.ca Entry licence](https://cdw.ca/product/windows-11-iot-enterprise-2024-ltsc-entry-license-1-license/7983139); [o365hq LTSC editions and cost](https://o365hq.com/blog/windows-enterprise-ltsc-vs-n-ltsc-vs-iot-enterprise-ltsc-which-edition-who-can-buy-it-and-what-it-costs); [Arrow, IoT Enterprise LTSC FAQ](https://www.arrow.com/ais/msembedded/resources/windows-iot-enterprise-ltsc-frequently-asked-questions-faqs/windows-iot-enterprise-ltsc)
- Hardware: [DigiKey, Forlinx FET3568-C](https://www.digikey.com.mx/en/products/detail/forlinx-embedded/FORLINX-FET3568-C-202GSE16GCA10/21810672); [CNX, MYIR MYC-LR3568](https://www.cnx-software.com/2024/07/17/rk3568-based-myc-lr3568-system-on-module-comes-with-up-to-8gb-ram-32gb-storage-for-cost-sensitive-edge-ai-applications/); [Variscite DART-MX93](https://shop.variscite.com/product/system-on-module/dart-mx93-system-on-module/); [CNX, i.MX93 SoMs](https://www.cnx-software.com/2024/05/13/nxp-i-mx-93-based-system-on-modules-launched-by-myir-variscite-and-compulab/); [CNX, $14 T113-S3 module](https://cnx-software.com/2023/05/11/14-allwinner-t113-s3-cpu-module-128mb-ram-256mb-nand-flash-4gb-emmc-flash); [CNX, ArmSoM CM1 RK3506J $15](https://www.cnx-software.com/2025/08/15/armsom-cm1-a-15-compute-module-sbc-hybrid-powered-by-rockchip-rk3506j-soc/); [CNX, Forlinx FET3506J-C](https://www.cnx-software.com/2025/08/25/forlinx-fet3506j-c-ultra-small-40-x-29mm-rockchip-rk3506j-som-with-board-to-board-connectors/); [Toradex Verdin iMX8M Mini](https://www.toradex.com/computer-on-modules/verdin-arm-family/nxp-imx-8m-mini); [CNX, MYIR MYC-LD25X STM32MP25](https://cnx-software.com/2024/09/26/myir-tech-myc-ld25x-a-compact-stm32mp25-system-on-module-running-debian-12/); [CNX, VAR-SOM-AM62](https://www.cnx-software.com/2023/02/07/var-som-am62-system-on-module-features-ti-am625x-cortex-a53-m4-soc/); [CNX, N100 fanless appliance](https://www.cnx-software.com/2024/03/30/inctel-n100-fanless-mini-pc-and-micro-firewall-appliance-comes-with-four-2-5gbe-ports-using-intel-i226v-controllers/)
- Libraries: [libplctag](https://libplctag.github.io); [OpENer](https://github.com/EIPStackGroup/OpENer); [OpENer ODVA licensing discussion](https://groups.google.com/g/eip-stack-group-opener-developers/c/5i1u4HxK234); [snap7 licence (Quanser)](https://docs.quanser.com/quarc/documentation/snap7_license.html); [neatvnc](https://github.com/any1/neatvnc); [Apache PLC4X SLMP](https://plc4x.apache.org/plc4x/pre-release/users/protocols/slmp.html); [melsec_mc crate](https://docs.rs/crate/melsec_mc/0.4.13/source/Cargo.toml)
- NOVAKON: [DirectIndustry, NOVAKON products](https://www.directindustry.com/prod/novakon-co-ltd/product-222514-2273366.html); [NOVAKON catalogue](https://pdf.directindustry.com/pdf/novakon-co-ltd/novakon-catalog/222514-911814.html)
- Licences stated from general knowledge and not re-verified in this session: libmodbus (LGPL-2.1+), open62541 (MPL-2.0), Eclipse Paho C and Mosquitto (EPL-2.0/EDL-1.0), RAUC (LGPL-2.1), SWUpdate (GPL-2.0), Mender client (Apache-2.0), SQLite (public domain), libvncserver (GPL-2.0), noVNC (MPL-2.0), libsodium (ISC), WAMR (Apache-2.0), FFmpeg (LGPL-2.1 with optional GPL parts). CRA dates are also from general knowledge. Check each before relying on it.
