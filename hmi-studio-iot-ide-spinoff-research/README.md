# EDT HMI Studio spin-off as an IoT development environment

Research date: November 19, 2026, Asia/Taipei. This is an English navigation overview; the full research is in Traditional Chinese.

The question: if EDT HMI Studio (EHS), today a free No Code LVGL tool that sells EDT display modules, were spun off into an independent software company, could it become an IoT integrated development environment? The study also covers three follow-up requests: Low Code is acceptable, strong multithreading and IDE features are wanted, and a paid VS Code extension is a candidate direction. It compares the product against LVGL Pro, MicroEJ and about twenty other products worldwide. Later additions cover paid dynamic-language MCU products, modern C/C++ RAD tools, a ranking of MCU development pain points, and an architecture proposal for an MCU unit bus with a programmable exchange hub.

## Findings

**Bottom line:** to compete as paid software, EHS has to stop selling a UI editor and become a tool that gets a whole connected device built and keeps it manageable after shipping. Without that shift, its features overlap LVGL Pro and SquareLine at a higher price than free tools. The [paid-product roadmap](research/10-ehs-paid-product-roadmap.zh-TW.md) sets out the changes, in order:

1. Remove the EDT-only licence and the ST/Windows lock-in.
2. Rebuild the core around the unit bus.
3. Charge for bus record and replay, connectivity units, and OTA plus SBOM.
4. Ship a hybrid product: a VS Code extension plus a No Code desktop app.
5. Price by seat and per product, with OEM and chip-vendor channels.

Customer interviews and prototype measurements must come before any investment.

- **Technically feasible.** The tie to EDT is mainly legal (EULA, in-app licence text) and branding. The code has no licence keys, device checks or EDT-only chips, and only one of the three boards is EDT hardware. The real technical lock-in is to ST tooling, Windows build scripts and LVGL.
- **Large gaps.** EHS has no device networking (TCP/IP, MQTT, HTTP), no OTA, no cloud accounts, no multithreading (the runtime is a bare-metal super-loop) and only STM32 targets. One engineer has built everything.
- **A general-purpose IoT IDE is not recommended.** Chip vendors give their IDEs and VS Code extensions away, Arduino is now part of Qualcomm, and IoT clouds are becoming free or open source.
- **Recommended position: a Low Code environment for connected devices with screens.** One tag model connects device UI, application logic, field protocols, cloud and OTA. The output is portable plain C, with whole-device simulation. Multithreading is designed in: a UI task that alone owns LVGL, tasks that exchange data through a tag store, design-time conflict detection and deterministic replay. Zephyr is the first RTOS target.
- **Delivery: a hybrid.** A paid VS Code extension serves engineers and reuses existing debugging and tracing tools. The No Code desktop app stays for OEM channels. Both sit on one shared engine and CLI. A VS Code-only product would lose the No Code audience. The Marketplace has no built-in payments, and its terms restrict extensions to Microsoft products, so the extension also needs Open VSX.
- **Against LVGL Pro, the value cannot be the UI editor.** LVGL Pro costs US$990 or US$3,990 per seat per year, or US$20,000 per product, and is free for Renesas customers. Its pages list no protocols, application logic flows, multithreading, OTA, board builds or shareable demos. EHS should sell outcomes: a working connected prototype in a day, concurrency bugs caught at design time, a demo customers can open, and OTA plus SBOM output for the EU CRA. It should also explore LVGL XML interoperability.
- **MicroEJ shows a complete MCU device platform can be sold.** It is Pro Code and VM-based, sold to large OEMs on unpublished multi-year contracts. It is a ceiling reference rather than a direct rival.
- **The worst MCU development pains match what EHS can charge for.** Across several surveys, the top pains are debugging (especially concurrency and timing bugs), complexity and schedule, connectivity, post-shipment OTA and field failures, and security and the CRA. EHS already covers or plans for concurrency bugs, testing before hardware exists and protocol integration. Field observability is better left to specialist services.
- **Other paid MCU tools charge for the runtime or production, not the tool.** Examples: Moddable (JavaScript) at US$0.50 per unit, Slint from about US$1 per unit, Quantum Leaps with a free QM tool but a commercial QP framework, and Toit with an open language and a paid OTA cloud (Toitware reportedly stopped operating in 2024, which suggests a language plus cloud alone does not sustain a company). QP's Active Object model is also a ready theory for the EHS multithreading design.
- **Architecture proposal: a unit bus with a programmable exchange hub.** Free of current EHS design limits, the MCU runs a software bus. Device units such as a touch GUI, a button interrupt, sensors, Modbus and MQTT attach to the bus, and Low Code rules in a programmable hub decide behaviour. The verdict is worth pursuing: Zephyr zbus, PX4 uORB, QP and ESPHome are precedents, and the design targets concurrency debugging, missing hardware and chip lock-in. The differentiation is the IDE: unit descriptors, whole-system simulation, bus record and replay, and cross-chip buses. Real-time behaviour, a hub bottleneck and memory use must first be measured on a minimal prototype.
- **No commercial product hot-reloads C code on an MCU.** Toit, MicroEJ, Moddable and LVGL Pro's runtime XML loading keep the C firmware fixed and swap bytecode, scripts, XML or containers. If EHS lets screens, hub rules and tag parameters take effect on real hardware on save, tied to simulation and bus replay, it fills a gap no rival covers. See [hot reload on MCUs](research/11-mcu-hot-reload.zh-TW.md).
- **No friendly whole-device simulator exists for MCUs.** Wokwi is friendly but covers few chips, Renode is capable but hard to set up, and Synopsys and Vector virtual prototypes are expensive. All simulate at the instruction-set level, and none puts the screen, external devices and cloud on one timeline. EHS could simulate behaviour at the level of bus units and leave driver-level testing to Renode. See [simulating an MCU and its external devices](research/12-mcu-device-simulation.zh-TW.md).
- **Borrow MicroEJ's boundary, not its VM, and run user extensions as C compiled to WebAssembly.** MicroEJ separates hardware, apps and its simulator with fixed APIs, fixed C prototypes and mocks on the same boundary. Python or TypeScript would also work as the app language, and TypeScript suits EHS better. The most practical "dynamic C" is C compiled to Wasm: it hot-loads, isolates modules from each other, and the same `.wasm` runs in the EHS WASM simulator and on the MCU. See [hardware/app separation and app languages](research/13-app-layer-separation-and-languages.zh-TW.md).
- **MCU firmware tools do not earn money from the editor.** IDEs, RTOSes, UI editors and languages are becoming free. Revenue sits in safety certification and compliance evidence, production licences, post-shipment operations, and debugging and porting services. ThreadX, now MIT-licensed with paid safety documentation, shows the pattern: free code, paid liability. EHS should charge for production licences, post-shipment management and whole-device debugging. See [where the money is](research/14-mcu-software-monetization.zh-TW.md).
- **New MCU software opportunities come from regulation, AI and test automation, not from nicer editors.** EU CRA vulnerability reporting started in September 2026, AI firmware startups raised funding in 2025 and 2026, and small teams still cannot afford hardware-in-the-loop testing. The best entry point for EHS is one chain: AI generation, verification in whole-device simulation, then SBOM and update records as compliance evidence. Extending it to EU RED cybersecurity checks (in force since August 2025), factory test generation and field service tools would make EHS a whole-lifecycle tool. See [MCU software opportunities](research/15-mcu-software-opportunities.zh-TW.md).
- **Global evidence supports the pivot but caps it: a small, profitable tool company whose most likely exit is a sale to a chip vendor or a tool consolidator.** From 2019 to 2026 Google, IBM, AWS, Microsoft, Arm and others retired the upper layers of their IoT platforms; survivors are local tools with portable output (Ignition) or were bought by chip or OS vendors (Memfault, Golioth, Particle). AI devalues drag-and-drop editing and raises the value of AI-readable project models, deterministic code generation and verification. EV chargers are the best-evidenced vertical, and Taiwan law offers a company split or technology-for-equity route. See [global evidence](research/16-global-evidence.zh-TW.md).
- **A spin-off is EDT's decision.** EDT owns the IP, and no document discusses a spin-off, IP transfer or licence-back. This study is decision material only.

## Documents

- [Full report in Traditional Chinese](README.zh-TW.md)
- [Codebase coupling and reusable assets](research/01-codebase-coupling.zh-TW.md)
- [Global competitor comparison](research/02-competitors.zh-TW.md)
- [Sources and open questions](research/03-sources-and-open-questions.zh-TW.md)
- [LVGL Pro and where paid value lies](research/04-lvgl-pro-and-paid-value.zh-TW.md)
- [MicroEJ comparison](research/05-microej-comparison.zh-TW.md)
- [Paid products for MCU development in dynamic languages](research/06-dynamic-language-mcu-products.zh-TW.md)
- [Modern paid RAD tools that output C/C++](research/07-c-cpp-rad-tools.zh-TW.md)
- [MCU firmware development needs and pain points](research/08-mcu-development-pain-points.zh-TW.md)
- [Architecture proposal: an MCU unit bus with a programmable exchange hub](research/09-mcu-unit-bus-architecture.zh-TW.md)
- [Paid-product roadmap for EHS](research/10-ehs-paid-product-roadmap.zh-TW.md)
- [Hot reload on MCUs: commercial products and the EHS opportunity](research/11-mcu-hot-reload.zh-TW.md)
- [Simulating an MCU and its external devices on a PC](research/12-mcu-device-simulation.zh-TW.md)
- [Separating hardware from applications: MicroEJ, and the choice of app language](research/13-app-layer-separation-and-languages.zh-TW.md)
- [Where the money is in MCU firmware development](research/14-mcu-software-monetization.zh-TW.md)
- [MCU software opportunities](research/15-mcu-software-opportunities.zh-TW.md)
- [Global evidence: market, exits, platform retreats, AI, regions and Taiwan spin-off mechanics](research/16-global-evidence.zh-TW.md) (notes in [research/notes/16-global-evidence](research/notes/16-global-evidence/))
