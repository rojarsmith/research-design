# Embedded/IoT Developer Surveys, Low-Code Acceptance, and the Impact of AI Coding (2024–2026)

Scope note: Research done 2026-11-20. Pain-point ranking is out of scope (covered by a prior study). Vendor-run or vendor-sponsored surveys are flagged **[VENDOR]**. Several full reports (Eclipse 2025, AspenCore, VDC) are gated or not indexed, so some figures come from press releases rather than the full PDFs.

## Q1. What survey data exists on embedded/IoT tool usage (IDEs, RTOSes, languages, GUI tools, purchasing)?

### Takeaway
Embedded is still a C/C++ world. Linux and FreeRTOS lead the OS charts, and Zephyr and ThreadX are growing on constrained devices. Public survey releases from 2024–2026 say almost nothing about IDE share (including VS Code), GUI tool share, or purchasing budgets. None of the summaries I could reach broke out VS Code share or low-code HMI tool share for embedded.

### Cited Findings
**Eclipse Foundation IoT & Embedded Developer Survey 2024** (10th edition, published 3 Dec 2024, ~750 respondents: developers, architects, decision-makers; run by a foundation, not a tool vendor, though it promotes open source)
- OS/RTOS: Linux 46%, FreeRTOS 29%, Zephyr 21%, ThreadX 13%. — [Eclipse newsroom](https://newsroom.eclipse.org/node/41237)
- Languages on constrained devices: C 55%, C++ 36%, Python 32%, Java 29%. Rust shows "steady growth" but no figure was published in the summaries. — [search summary of Eclipse coverage, i-programmer / ADTmag](https://i-programmer.info/news/91-hardware/17663-eclipse-iot-developer-survey-2024.html)
- Target industries: industrial automation 34%, automotive 29%, energy management 29%, environmental monitoring 23%, healthcare 18%. — [Eclipse newsroom](https://newsroom.eclipse.org/node/41237)
- 47% prioritize safety certifications such as IEC 61508 and ISO 26262. — [Eclipse newsroom](https://newsroom.eclipse.org/node/41237)
- MQTT 56% (+7 points vs 2023). Open-source use 75% (up from 63% in 2023). 24% are committers. — [Eclipse newsroom](https://newsroom.eclipse.org/node/41237)
- Top concerns: connectivity 48% (down from 52%), security 35% (up from 33%), data analytics 24%, deployment 24%. — [Eclipse newsroom](https://newsroom.eclipse.org/node/41237)
- The press release has no IDE, AI-tool, or budget data. — [Eclipse newsroom](https://newsroom.eclipse.org/node/41237)

**Eclipse 2025 IoT & Embedded Survey** (fielded from July 2025, report released at embedded world, March 2026)
- The question set was reworked. The report covers regulation, supply chain, "developer and engineering decision-maker technology preferences," and open source for compliance and digital sovereignty. The press release has no numbers. The full report is gated at outreach.eclipse.foundation. — [Eclipse newsroom 2026](https://newsroom.eclipse.org/node/43632); [Eclipse blog, July 2025](https://blogs.eclipse.org/post/fr%C3%A9d%C3%A9ric-desbiens/2025-embedded-and-iot-developer-survey-10th-time-charm)

**JetBrains "State of Embedded 2025"** (from the Developer Ecosystem Survey 2025. The sample is printed garbled on the page as "nearly 2,3000" across 28 countries. Mixed professional and hobbyist sample, 27% from mainland China. **[VENDOR]**, IDE maker)
- Languages: professionals C++ 37%, C 33%. Hobbyists Python 29%, C++ 28%, C 26%. — [JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)
- Systems: Embedded Linux 58%, Windows 22%, bare metal 21%, FreeRTOS 18%. Among RTOS users: FreeRTOS 82%, Zephyr 14%, VxWorks 4%, ThreadX 4%, NuttX 4%. — [JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)
- MCUs: ESP32 38%, ATmega328 30%, STM32F103 17%, other STM32 13%, RP2040 12%. Boards: ESP32-based 42%, Arduino 37%, STM32 Nucleo/Discovery 23%. — [JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)
- **Regular non-coding practices (relevant to low-code):** testing 54%, simulation 27%, emulation 22%, **graphical system design 13%**, virtual prototyping 12%, **modeling in high-level languages 10%**. — [JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)
- The public page shows no IDE share and no AI-assistant figures. — [JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)

**Memfault / VDC Research "State of IoT Software Development" 2024** (VDC online survey, 783 respondents involved in IoT/embedded product development, published April 2024. **[VENDOR]**: Memfault sells observability)
- Software is close to 60% of project development cost. Teams using third-party device-health tools reported 57% lower overall development cost than teams with in-house tools (a vendor-favorable finding). — [Memfault report page](https://memfault.com/iot-software-development-report/); [New Electronics](https://newelectronics.co.uk/content/news/state-of-iot-survey-highlights-the-importance-of-software-in-iot-development)
- Memfault "IoT Visibility Gap" 2025: 200 US IoT decision-makers, Q2 2025. 77% say insufficient device observability is a barrier to growth. No AI questions. **[VENDOR]** — [Memfault PDF](https://memfault.com/wp-content/uploads/2025/08/The-IoT-Visibility-Gap-Report-Memfault.pdf)

**AspenCore / EE Times Embedded Markets Study**
- The most recent edition with indexed data is 2019. The 2017 edition (1,234 respondents, 56% North America) had Embedded Linux at 22% and FreeRTOS at 20%. I found no 2023–2025 edition. — [CNX Software on 2017 study](https://cnx-software.com/2017/08/15/aspencore-2017-embedded-markets-study-programming-languages-operating-systems-mcu-vendors-and-more)

### Inferences
- The OS mix differs a lot by sample. Eclipse (professional IoT) puts FreeRTOS at 29% and Zephyr at 21% of all respondents. JetBrains (hobbyist-heavy) puts FreeRTOS at 82% of RTOS users. A Zephyr/FreeRTOS-first IoT IDE should cover both, plus ESP-IDF (ESP32 is the top MCU in JetBrains).
- C stays the lingua franca. A tool that generates portable C (as EHS does) fits current practice. Rust is growing but small, and I found no published 2024–2025 share.
- Only 10–13% of embedded developers regularly do "graphical system design" or "modeling in high-level languages" (JetBrains 2025). That is a useful ceiling for the regular low-code/model-based user base outside automotive and aerospace.

### Gaps
- **VS Code share among embedded developers**: none of the 2024–2026 public summaries (Eclipse, JetBrains embedded page) report it. The full Eclipse 2025 report is gated. I found no reliable figure, so none is quoted.
- **GUI/HMI tool usage share** (LVGL, TouchGFX, SquareLine, Qt, Crank, emWin): no survey data found. Only vendor or distributor marketing.
- **Who decides tool purchases, and typical budgets**: not in any accessible 2024–2026 survey summary. VDC Research covers this in paid reports that are not public.
- Stack Overflow 2024/2025 has no published embedded-segment breakdown for tools or AI. Raw data would need custom analysis.
- AspenCore has not published (or indexed) a post-2019 Embedded Markets Study.

## Q2. How do embedded engineers feel about low-code, model-based, and code-generation tools?

### Takeaway
Embedded engineers broadly use code *generators* that sit inside a code-first workflow: vendor configurators like STM32CubeMX, Simulink in automotive, and GUI designers like TouchGFX and SquareLine. Full no-code is not widely used. The evidence shows a split. Domain and system engineers rate model-based tools well. Software engineers rate them lower, mostly because of collaboration and Git friction. Quantified data on low-code acceptance specifically in embedded is very thin.

### Cited Findings
- Simulink survey at an automotive OEM (Cederbladh & Norin, MODELS 2025 companion, **n=23**, Likert 1–5): mostly positive reception. **Collaboration scored lowest, with the most 1-scores**: multiple users, "lack of Git," model sharing. Software engineers gave significantly lower ratings than other roles. Ratings of generated-code quality varied by role. The authors call Simulink "a divisive tool," valued and controversial "due to the same aspects of graphical coding." — [Cederbladh slides](https://copamo.github.io/presentations/2025/Cederbladh.pdf); [researchr](https://researchr.org/publication/CederbladhN25)
- An older industrial survey (112 embedded professionals, 2014) found model-based engineering used mainly for simulation, code generation, and documentation. Adoption was held back by high training effort and tool usability problems. — [Model Based Engineering for Embedded Systems in Practice](https://www.academia.edu/49326591/Model_Based_Engineering_for_Embedded_Systems_in_Practice)
- In JetBrains 2025, 13% regularly do graphical system design, 10% model in high-level languages, 27% use simulation, and 12% use virtual prototyping. **[VENDOR]** — [JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)
- SquareLine Studio is a drag-and-drop LVGL editor. A third-party tutorial says it "doesn't cover all components of LVGL or complex logic." — [search summary of DigiKey Maker.io / Elecrow tutorials](https://www.digikey.com/es/maker/blogs/2023/easily-design-beautiful-user-interfaces-for-embedded-projects-with-squareline-studio)
- ST keeps investing in TouchGFX Designer, a drag-and-drop PC tool, plus its engine. Version 4.25 and later added emulated framebuffers. — [ST TouchGFX Designer](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-configurators-and-code-generators/touchgfxdesigner.html); [ST blog TouchGFX 4.26](https://blog.st.com/?p=42447)
- LVGL moved its own editor (LVGL Pro) to a **fully text-based XML** model that exports C or loads at runtime. It is explicitly pitched as fitting "Git, automation, and AI tools." A CLI validates XML, generates C, and runs UI tests in CI/CD. Licensing is royalty-free, with a free start and paid seats. — [LVGL Editor page](https://lvgl.io/editor); [LVGL Pro XML docs](https://docs.lvgl.io/master/xml/index.html)

### Inferences
- The main objections to graphical tools (collaboration, Git, opaque generated code) are exactly what text-based project models address. LVGL's move to XML reads as a direct answer to the "lack of Git" complaint and to AI-readability, and it competes directly with EHS's positioning.
- Embedded engineers accept generators when the output is inspectable C and the tool is deterministic and vendor-trusted (CubeMX, TouchGFX, Simulink Embedded Coder). "No-code" framing is more likely to put off professional firmware engineers. The data shows software engineers are the harshest critics.

### Gaps
- No survey found that measures STM32CubeMX usage share, Node-RED use in industrial IoT, or SquareLine/TouchGFX user counts.
- No embedded-specific study of "willingness to adopt low-code" with a sample size larger than ~100 in 2024–2026.
- A 2018 Journal of Systems and Software survey (627 embedded engineers, 27 countries) exists, but its code-generation figures were not accessible. — [JSS 2018 PDF](https://gcris.iyte.edu.tr/bitstream/11147/7647/1/1-s2.0-S1383762118302455-main.pdf)

## Q3. How far has AI coding spread in embedded, and what are its limits?

### Takeaway
AI coding moved into embedded quickly in 2025–2026. Vendor surveys report 80–89% of organizations using AI assistants, and over 80% have shipped AI-generated code to production. Trust is low, and the best results come when agents get structured hardware context: skills, MCP servers, and deterministic generators. By late 2026 every major MCU vendor (ST, Nordic, Espressif, Microchip) and the IoT platform Tuya had shipped AI-agent tooling. Most of it is built on MCP and connects to existing deterministic code generators instead of replacing them.

### Cited Findings
**Adoption surveys**
- **RunSafe Security, "AI in Embedded Systems" 2025** (200+ embedded professionals in the US, UK and Germany, published Dec 2025. **[VENDOR]**, sells embedded security): 80.5% currently use AI tools in embedded development. 83.5% have deployed AI-generated code to production. 93.5% expect use to grow over two years. 53% name security as the top concern. 73% rate the risk moderate or higher. Use cases: testing and validation 28%, code generation 19%, security scanning 10%. 96% are confident their tools can find vulnerabilities in AI code. — [BusinessWire](https://www.businesswire.com/news/home/20251209931517/en/RunSafe-Security-Releases-2025-AI-in-Embedded-Systems-Report-Offering-New-Insight-Into-AI-Adoption-and-Security-Gaps); [Help Net Security](https://www.helpnetsecurity.com/2026/01/02/ai-embedded-systems-development/)
- **Black Duck, "State of Embedded Software Quality and Safety 2025"** (Censuswide, 785 development and security professionals. Black Duck's own blog says "over 750." **[VENDOR]**): 89.3% of organizations use AI coding assistants. 25% of organizations prohibit them, and of those, 75% (or 72% in another Black Duck-authored piece) admit developers use them anyway. — [Black Duck blog](https://www.blackduck.com/blog/embedded-software-survey-ai-adoption-code-quality.html); [Cybersecurity Insiders](https://www.cybersecurity-insiders.com/the-embedded-software-industry-in-2025-navigating-ai-adoption-and-supply-chain-transparency/)
- **Stack Overflow Developer Survey 2025** (all developers, no embedded breakdown): 84% use or plan to use AI tools. 46% distrust AI output (up from 31%). Only about 3% "highly trust" it. Favorability fell to 60% (from 72% in 2024 and 77% in 2023). 87% worry about the accuracy of AI agents. Trust-in-accuracy figures vary between summaries (29% vs 33%). — [Stack Overflow blog](https://stackoverflow.blog/2025/07/29/developers-remain-willing-but-reluctant-to-use-ai-the-2025-developer-survey-results-are-here); [TechRepublic](https://www.techrepublic.com/article/news-developer-trust-in-ai-declines/)

**Academic evidence on capability and limits**
- Englhardt et al. (arXiv 2307.03817, Univ. of Washington): on a hardware-in-the-loop platform, GPT-4 produced working I2C interfaces in 66% of **N=50** trials. It also wrote register-level drivers, LoRa code, and an nRF52 power optimization (740x current reduction to 12.2 µA). In a **15-user** study, success building a LoRa sensor rose from 25% to 100%, including for users with no hardware or C experience. — [arXiv 2307.03817](https://arxiv.org/abs/2307.03817v2)
- Li et al., "Skilled AI Agents for Embedded and IoT Systems Development" (arXiv 2603.19583, Duke, March 2026): code "can compile yet fail on real devices due to timing, peripheral initialization, or hardware-specific behavior." The IoT-SkillsBench benchmark covers 3 platforms, 23 peripherals, 42 tasks, and **378 hardware-validated experiments** across three setups (no skills, LLM-written skills, human-expert skills). **Concise human-expert skills gave "near-perfect success rates."** Experts wrote the skills from the failure modes of unassisted LLM code. — [arXiv 2603.19583](https://arxiv.org/abs/2603.19583)
- Sun & Staron, "Agentic Pipelines in Embedded Software Engineering" (arXiv 2601.10220, Chalmers/Gothenburg, Jan 2026, qualitative, **10 senior engineers from 4 companies**): 11 emerging practices, including multi-agent CI/CD, "AI-friendly artifacts," compiler-in-the-loop, and human-in-the-loop. 14 challenges, including **certification gaps for AI-generated artifacts** (IEC 61508, ISO 26262, DO-178C), non-determinism undermining reproducibility, and traceability over long lifecycles. — [arXiv 2601.10220](https://arxiv.org/html/2601.10220v1)
- A 2026 paper notes that compiler-in-the-loop and flash-in-the-loop methods still "fail to incorporate behavioral errors that emerge during physical execution." — [search summary citing arXiv 2603.19583 context](https://arxiv.org/pdf/2603.19583)

**AI firmware startups**
- **Embedder** (YC S25, San Francisco, 2025, pitched as "Claude Code for embedded"): its agent reads datasheets and schematics, writes code, flashes the board, runs tests, and self-corrects. A job post claims 5,000+ engineers and 100+ paying customers (unverified). Funding reports conflict ($500K vs an implausible $625M). Nominated for an Embedded Award 2026. — [YC profile](https://www.ycombinator.com/companies/embedder); [Launch HN](https://hn.svelte.dev/item/44915206); [StartupHub](https://startuphub.ai/startups/embedder-yc-s25)

**Chip-vendor and platform AI assistants**
- **Microchip MPLAB AI Coding Assistant** (Feb 19, 2025): a free VS Code extension built on the open-source Continue assistant with a Microchip-trained chatbot. Supports 8–32-bit MCUs, with paid tiers possible later. — [Microchip / GlobeNewswire](https://www.globenewswire.com/news-release/2025/02/19/3028656/0/en/artificial-intelligence-meets-embedded-development-with-microchip-s-mplab-ai-coding-assistant.html)
- **ST "STM32Cube for Agentic AI"** (announced Nov 4, 2026, beta. v1.1.0 adds STM32G5/H7P): bundles STM32 skills, MCP servers, and RAG over ST docs and validated reference code. **Agents call STM32CubeMX for "deterministic code generation."** An "Errata Hunter" MCP checks code against MCU errata, and the "STM32 Sidekick" MCP serves support info. ST says agents with this context produced working code where unassisted agents failed, using **80% fewer tokens** (ST internal test, **[VENDOR]**). — [ST page](https://www.st.com/content/st_com/en/stm32-mcu-developer-zone/stm32-cube-for-agentic-ai.html); [EIN Presswire](https://www.einpresswire.com/article/954394879/stmicroelectronics-sharpens-agentic-ai-coding-with-new-stm32-tools); [ST Community](https://community.st.com/stm32cube-for-agentic-ai-186/stm32cube-for-agentic-ai-1-1-0-now-supporting-stm32g5-and-stm32h7p-169402)
- **Nordic** (June 2026): the Nordic MCP server provides nRF Connect SDK and Zephyr docs, a development guide, and semantic search over docs and DevZone. A separate nRF Cloud MCP gives read-only fleet data. nRF Connect for VS Code sets it up automatically. Works with Claude Code, Cursor, and GitHub Copilot. — [Nordic AI-assisted development](https://www.nordicsemi.com/Products/Technologies/AI-assisted-development); [CNX Software](https://www.cnx-software.com/2026/06/02/nordic-adds-ai-assisted-development-to-the-nrf-connect-sdk-and-nrf-cloud/)
- **Espressif** (2026): the ESP-IDF v6.0 local MCP server is built into idf.py and exposes set-target, build, flash, and status to any MCP client. There is also a Documentation MCP server, and ESP-IDF VS Code extension v2.3.0 registers Espressif's remote MCP servers automatically. — [Espressif blog, Apr 2026](https://developer.espressif.com/blog/2026/04/esp-idf-tools-mcp-server/); [Espressif blog, Oct 2026](https://developer.espressif.com/blog/2026/10/esp-idf-vscode-extension-v230/)
- **Tuya**: "Tuya AI Coding" (2026) builds natural-language "lifestyle apps" that control real devices across 100,000+ SKUs. **TuyaOpen IDE** is a VS Code/Cursor extension covering firmware, connectivity, and cloud ("vibe coding comes to hardware"). Tuya Cobuilder is a "Physical AI Builder." Shown at IFA 2026. **[VENDOR]** press releases only. — [Tuya press release (ANP)](https://persportaal.anp.nl/artikel/CSN-050826021/tuya-smart-launches-tuya-ai-coding-enabling-users-to-build-their-own-ai-powered-lifestyle-apps-with-natural-language); [DEV Community TuyaOpen IDE](https://dev.to/tuyadeveloper/vibe-coding-comes-to-hardware-i-tried-tuyaopen-ide-the-ai-native-hardware-dev-tool-40o8)
- **LVGL**: LVGL Pro is XML-based, and the docs say AI tools can read, generate, and refactor the XML. LVGL runs an MCP server **for its docs**. On the LVGL forum, LVGL staff suggest AI should generate LVGL XML rather than SquareLine projects. A community-built Claude Code "skill" already generates SquareLine Studio projects. I found no official MCP server for the editor itself. — [LVGL forum thread](https://forum.lvgl.io/t/tool-ai-skill-that-generates-complete-squareline-studio-projects-to-export-as-lvgl-widget-trees/24322); [LVGL Pro docs](https://docs.pro.lvgl.io/xml/overview.html)
- **Memfault** launched an MCP server that exposes fleet data, coredumps, and crash traces to AI agents (product launch, not survey data). — [search summary of Memfault site](https://memfault.com/b)

### Inferences
- The chip vendors converged on one pattern in 2025–2026. They do not compete with general LLMs. They supply **context (docs/RAG, errata), deterministic generators (CubeMX), and actions (build/flash via MCP)** that general agents (Copilot, Claude Code, Cursor) call. Hardware-specific value has moved to the "tool layer behind the agent."
- Both the academic benchmark (expert skills give near-perfect success) and ST's claim (80% fewer tokens, success where bare agents fail) point the same way: structured, curated hardware knowledge is the main multiplier for AI in embedded. A low-code IDE's project model and HAL metadata can serve as that knowledge.
- The usage-vs-trust gap (80–89% use vs only about 3% high trust in Stack Overflow 2025, plus certification gaps) creates demand for verification, simulation, and traceability more than for more generation.

### Gaps
- **GitHub Copilot usage among embedded developers specifically**: no reliable figure found.
- **NXP** (MCUXpresso) AI-assistant or MCP offering: not found in searches. It may exist, so verify separately.
- Both RunSafe and Black Duck are security vendors with an interest in showing risk. No independent (non-vendor), large-sample embedded AI-adoption survey was found. The Eclipse 2025 report may have AI questions, but it is gated.
- IoT-SkillsBench per-configuration success rates (no-skills vs expert-skills) were not extractable from the abstract or the PDF text I could read.
- Embedder's user and revenue claims are unverified.

## Q4. What do analysts and the industry say: does AI make low-code obsolete or shift its value?

### Takeaway
Gartner's position, as quoted by vendors, is that AI adds to low-code rather than replacing it: "vibe coding accelerates typing; LCAPs accelerate engineering." Gartner still forecasts double-digit growth for the low-code market. At the same time, the big enterprise low-code vendors (OutSystems, Mendix) have repositioned around building and governing AI agents. The value is moving from "drag-and-drop instead of code" to governance, structured models, lifecycle control, and deterministic outputs. This is enterprise-app evidence. No analyst work specific to embedded low-code was found.

### Cited Findings
- Gartner forecast (Nov 4, 2025): the low-code development technologies market reaches **$58.2B by 2029, 14.1% CAGR**. — [Gartner abstract (via search)](https://www.gartner.com/en/documents/7146430). An earlier Gartner note (May 2024) put the LCAP market at $16.5B by 2027 (16.3% CAGR). A Kissflow-attributed "$44.5B by 2026" figure could not be verified. — [Kissflow](https://kissflow.com/low-code/future-of-low-code-application-development/)
- Gartner report "Why AI Won't Replace the Need for Low-Code Application Platforms" (Oct 2025): AI code assistants and agents have made some question the need for LCAPs, and Gartner concludes AI will enhance, not replace, them. The line "Ultimately, vibe coding accelerates typing. LCAPs accelerate engineering." is **vendor-quoted** (Jitterbit/Mendix distribute the report) and could not be checked against Gartner's own abstract. — [Jitterbit (Gartner reprint)](https://www.jitterbit.com/report/gartner-report-why-ai-wont-replace-low-code-application-platforms); [Mendix](https://www.mendix.com/?p=253477)
- Gartner (2026): software engineering leaders "must build low-code-like maturity in governance, reuse, integration and life cycle control to scale vibe coding." Another 2026 Gartner LCAP abstract says leaders pulled ahead by building agentic AI into their core development environments. — [Gartner abstracts (via search)](https://www.gartner.com/en/documents/8437013)
- Mendix cites Gartner (July 2025 MQ) that by 2028, agentic AI will be implemented through enterprise low-code platforms at 4 of 5 businesses. This is vendor framing of a Gartner claim. — [Siemens/Mendix blog](https://blogs.sw.siemens.com/news/mendix-roadmap-from-low-code-to-agentic-business-outcomes/)
- OutSystems launched Agent Workbench (Sep 2025) and now positions itself as an "open agentic systems platform." Diginomica (Dec 2025) said it went "all-in" on agentic AI, with little low-code messaging left. Its CEO describes a shift from code generation to autonomous agents. — [OutSystems news](https://www.outsystems.com/news/agent-workbench-accelerates-agentic-ai-adoption/); [Diginomica](https://diginomica.com/outsystems-one-low-code-vendor-remodels-appeal-ai-agent); [OutSystems platform page](https://www.outsystems.com/low-code-platform)
- Forrester: no standalone low-code-vs-AI report found. Analyst Craig Le Clair, keynoting at an OutSystems event, said low-code-heritage platforms now compete with other categories for agentic-AI spending. — [search summary of OutSystems event coverage](https://cybermagazine.com/news/q-a-woodson-martin-ceo-of-outsystems-on-agentic-ais-rise)
- In embedded, ST's agentic package keeps **CubeMX as the deterministic generator** that agents call. AI coordinates, and the low-code generator still produces the trusted code. — [ST page](https://www.st.com/content/st_com/en/stm32-mcu-developer-zone/stm32-cube-for-agentic-ai.html)

### Inferences
- Enterprise low-code did not die. It pivoted. The visual canvas is no longer the main selling point. Governance, structured models, and agent orchestration are. The pure "no-code for non-programmers" story is the part most eroded by AI, because natural-language generation now serves that user directly (Tuya AI Coding is an IoT example).
- For embedded HMI, AI erodes the value of: hand-placing widgets, boilerplate C generation, and "you don't need to know C." AI raises the value of: a **machine-readable project model** (XML/JSON scenes, bindings, HAL configs) that agents can generate and validate; **deterministic code generation** from that model (the CubeMX pattern); **simulation and on-host preview** (JetBrains: 27% already simulate, 22% emulate); **CI-based UI tests** (the LVGL CLI); and **traceability and compliance evidence** (the certification gap in Sun & Staron; 47% of Eclipse respondents prioritize IEC 61508/ISO 26262).
- Net assessment: AI is likely **neutral-to-negative for "no-code editor as product"** and **positive for "structured model + generator + verifier that AI agents target."** The market evidence (vendors adding MCP servers to existing generators rather than retiring them) supports the second reading. This is an inference, and no direct market-share data confirms it.

### Gaps
- No analyst (Gartner, Forrester, IDC, VDC) research found specifically on low-code or model-based tools in embedded/IoT after GenAI.
- No data on whether SquareLine, TouchGFX, or LVGL Pro seat sales rose or fell after AI assistants arrived.
- Gartner primary texts are paywalled. Quotes come from vendor reprints.

## Q5. Implications: which parts of an embedded low-code IDE keep their value as AI coding spreads?

### Takeaway
The durable parts are the ones AI agents *need* and cannot reliably produce themselves. Ranked by strength of evidence: (1) curated hardware and board context (HAL and pin maps, errata, LVGL and driver configs) exposed as skills or MCP; (2) a text-based, versionable project model as the AI target; (3) deterministic, inspectable C generation; (4) simulation, preview, and hardware-in-the-loop verification; (5) traceability and compliance artifacts. The drag-and-drop canvas becomes a review and adjust surface, no longer the main way to write code.

### Cited Findings
- Expert-curated, concise hardware "skills" took agents to near-perfect success on 378 real-hardware experiments. — [arXiv 2603.19583](https://arxiv.org/abs/2603.19583)
- ST: context plus a deterministic CubeMX generator gave working code where bare agents failed, with 80% fewer tokens. **[VENDOR]** — [ST page](https://www.st.com/content/st_com/en/stm32-mcu-developer-zone/stm32-cube-for-agentic-ai.html)
- Espressif's MCP exposes build and flash, and Nordic's exposes docs and fleet data. Hardware actions and knowledge are being packaged for agents. — [Espressif](https://developer.espressif.com/blog/2026/04/esp-idf-tools-mcp-server/); [Nordic](https://www.nordicsemi.com/Products/Technologies/AI-assisted-development)
- LVGL Pro: XML model, CLI validation, C generation, and CI UI tests, pitched as AI-friendly. This is the closest competitor following the same logic. — [LVGL Editor](https://lvgl.io/editor)
- Practitioners list "AI-friendly artifacts," compiler-in-the-loop, and human-in-the-loop as emerging practices, and certification and traceability as open problems. — [arXiv 2601.10220](https://arxiv.org/html/2601.10220v1)
- Code that compiles still fails on hardware because of timing and peripheral initialization, which argues for hardware or simulation verification loops. — [arXiv 2603.19583](https://arxiv.org/abs/2603.19583)
- Graphical MBD's weakest point for software engineers is collaboration and Git. — [Cederbladh slides](https://copamo.github.io/presentations/2025/Cederbladh.pdf)

### Inferences
- EHS's repositioning case is stronger if it is framed as an "AI-native, model-driven HMI/IoT IDE": a text-diffable project format, an MCP server that exposes the model, widget catalog, board profiles, and build/flash/preview, plus a deterministic C generator and simulator. That framing is stronger than "no-code editor." It turns AI from a substitute into a demand driver, because every AI-generated UI needs validation, preview, and deterministic output.
- Competitive risk: LVGL (LVGL Pro XML + CLI + docs MCP), ST (TouchGFX/CubeMX + agentic package), and Tuya (AI Coding + TuyaOpen IDE) are moving the same way with ecosystem advantages. Differentiation would have to come from multi-vendor HAL coverage (STM32 + ESP32 + Nordic/Zephyr), simulation fidelity, or compliance evidence. This is an inference, not tested against customer data.
- "Low-code for non-programmers" is the segment most exposed to natural-language generation (Tuya AI Coding, Embedder's 25%→100% novice success in Englhardt et al.). Professional firmware teams are the segment where structure and verification keep value.

### Gaps
- No customer or survey data on whether embedded teams would *pay* for an AI-target model or MCP layer, separately from free vendor tools (Microchip's is free, and ST's is a free beta).
- No benchmark data comparing AI-generated LVGL C against AI-generated XML plus a deterministic generator for HMI correctness or size.
