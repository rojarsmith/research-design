# IoT Platform and IoT Development-Environment Retreats, Shutdowns and Consolidation (2018–Nov 2026): Evidence and Lessons for a Small IoT-IDE Vendor

Context: EDT HMI Studio (no-code embedded HMI tool) is considering a spin-off and repositioning as an IoT development environment. These notes collect what was retired or sold, and why; what happened to customers; which IoT businesses survived and how they make money; what analysts say; and lessons for a small developer-tool vendor. Research cut-off: 20 November 2026.

Source-quality note: dates below come from vendor notices or docs where possible. Some come only from community forums, third-party mirrors or trade press, and those are flagged. Several 2026 events (Azure IoT Central 2029 retirement, ThingsBoard BUSL, Nabu Casa rename, Particle/Golioth/Losant acquisitions) are recent; they were confirmed in search results but should be rechecked before publication.

---

## Q1. Verified dates and stated reasons for the major retreats

### Takeaway
Between 2019 and 2026, every hyperscaler and most industrial conglomerates cut back their horizontal IoT platforms. Google and IBM shut theirs down entirely. AWS retired its IoT analytics and event-processing layers but kept the core device gateway. Microsoft open-sourced Azure RTOS and set retirement dates for Azure Sphere and IoT Central. SAP folded its IoT platform into applications. PTC sold ThingWorx and Kepware. Arm archived Mbed. The stated reasons follow the same pattern: "customers are better served by partners," "focus on core platform services," or "focus on core business." What survived at the hyperscalers is the thin connectivity layer (IoT Hub, AWS IoT Core). The application, analytics and IDE layers above it were retired.

### Cited Findings

**Google Cloud IoT Core**
- Announced 16 August 2022. The service was discontinued on 16 August 2023, after which the IoT Core Device Manager APIs and MQTT/HTTP bridges stopped working and existing device connections were cut. — [RCR Wireless, 16 Aug 2022](https://www.rcrwireless.com/20220816/internet-of-things/google-cloud-to-shut-down-iot-core-service-next-year); [InfoQ](https://www.infoq.com/news/2022/08/google-iot-core-discontinued/)
- The Register quotes the notice as "discontinued on August 15 2023," a one-day discrepancy with the widely cited 16 August 2023. — [The Register, 15 Nov 2022](https://www.theregister.com/2022/11/15/ibm_set_to_retire_watson/)
- Stated reason: a spokesperson said customers' needs "could be better served by our network of partners that specialize in IoT applications and services." There was no first-party replacement. — [ITPro](https://www.itpro.com/network-internet/internet-of-things-iot/368845/google-cloud-to-retire-iot-core-service-in-2023); [SiliconANGLE](https://siliconangle.com/2022/08/17/google-cloud-discontinue-iot-core-service-2023/)

**IBM Watson IoT Platform (IBM Cloud)**
- Customer notice reported 15 November 2022. IBM would "sunset the Watson IoT Platform service on IBM Cloud effective December 1st, 2023 without a direct replacement," after which devices lose MQTT and HTTP connectivity. IBM gave no reason and declined further comment. — [The Register, 15 Nov 2022](https://www.theregister.com/2022/11/15/ibm_set_to_retire_watson/)
- Conflicting end date: TechRadar gives 31 December 2023. — [TechRadar](https://techradar.com/news/ibm-is-retiring-its-watson-iot-cloud-management-platform)
- Earlier partial withdrawals: support for IBM IoT MessageSight and Watson IoT Platform Message Gateway was discontinued in a notice of 8 September 2020, and the Watson IoT Platform QuickStart service was sunset on 31 May 2021. — [IBM Support](https://www.ibm.com/support/pages/watson-iot-platform-quickstart-service-sunset); [IBM Support node 6340723](https://www.ibm.com/support/pages/node/6340723)

**Microsoft Azure IoT Central**
- February 2024: an Azure console message announced retirement on 31 March 2027. On 16 February 2024, Microsoft's IoT blog called that message erroneous and said IoT Central was not being retired. — [The Register, 15 Feb 2024](https://www.theregister.com/2024/02/15/microsoft_retires_azure_iot_central/); [Microsoft Tech Community, "Microsoft's commitment to Azure IoT"](https://techcommunity.microsoft.com/blog/iotblog/microsofts-commitment-to-azure-iot/4059725); [The Register, 21 Feb 2024](https://www.theregister.com/2024/02/21/microsoft_azure_iot_central/)
- 29 September 2026: Message Center item AZ569914 set formal retirement for 20 September 2029. Reason given: Microsoft is "focusing future investment on Azure IoT platform services." Recommended migration is to IoT Hub + Device Provisioning Service + Azure Device Registry, with Microsoft Fabric for analytics, plus a migration playbook and migration partners. This was seen only via a third-party Message Center mirror. — [mc.merill.net AZ569914](https://mc.merill.net/message/AZ569914)

**Microsoft Azure Sphere**
- Announced 20 March 2026. The service retires 31 July 2031, when DAA certificates and application, OS and security updates stop. The MT3620 silicon reaches end of life on 31 July 2026, and that is also the last date to renew OEM agreements and register purchasing intent through Avnet. Stated reason: secure MCUs are now available from many silicon vendors, so customers can pair them with newer Microsoft IoT services. Products that must run past 2031 need a hardware redesign. — [Microsoft Tech Community, 20 Mar 2026](https://techcommunity.microsoft.com/blog/iotblog/azure-sphere-is-retiring-in-2031---what-you-need-to-know/4504225); [mc.merill.net AZ557123](https://mc.merill.net/message/AZ557123)
- The Azure Sphere Classic CLI was retired on 30 September 2024. — [Azure Updates](https://azure.microsoft.com/it-it/updates/important-notice-azure-product-retirement-end-of-life-announcement-of-azure-sphere-classic-cli-on-30-september-2024-replace-w/)

**Microsoft Azure RTOS → Eclipse ThreadX**
- Announced 21 November 2023: Azure RTOS (ThreadX, NetX Duo, FileX, GUIX, USBX, etc.) contributed to the Eclipse Foundation as Eclipse ThreadX under the MIT licence. The first release, Eclipse ThreadX 6.4.1 (functionally equivalent to Azure RTOS 6.4), shipped in April 2024. — [Eclipse Foundation blog, 21 Nov 2023](https://eclipse-foundation.blog/2023/11/21/introducing-eclipse-threadx/); [Microsoft Tech Community](https://techcommunity.microsoft.com/blog/iotblog/microsoft-contributes-azure-rtos-to-open-source/3986318); [Microsoft, "transition to open source is now complete"](https://techcommunity.microsoft.com/blog/iotblog/azure-rtos-transition-to-open-source-is-now-complete/4105027)

**AWS IoT Analytics**
- Closed to new customers (accounts created after July 2024). End of support was 15 December 2025, after which console and resources became inaccessible. Recommended migration: Amazon Kinesis Data Streams and Amazon S3, among others. — [AWS IoT Analytics end-of-support doc](https://docs.aws.amazon.com/iotanalytics/latest/userguide/iotanalytics-end-of-support.html)

**AWS IoT Events**
- Closed to new customers on 20 May 2025. End of support was 20 May 2026, after which console and resources became inaccessible. — [AWS IoT Events end-of-support doc](https://docs.aws.amazon.com/iotevents/latest/developerguide/iotevents-end-of-support.html); [AWS IoT Events features page](https://aws.amazon.com/iot-events/features)

**AWS IoT Greengrass V1**
- V2 launched December 2020. The V1 Core software entered the "extended life phase" (no patches) on 30 June 2023. The policy promised 12 months' notice before turning off endpoints. The current banner on the V1 maintenance-policy page says "On October 7th, 2026, AWS will discontinue support for AWS IoT Greengrass Version 1. After October 7th, 2026, you will no longer be able to access the AWS IoT Greengrass V1 resources." — [AWS Greengrass V1 maintenance policy](https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html)
- Conflict: search results showed the Greengrass product page giving 1 June 2026. The maintenance-policy page (fetched directly) gives 7 October 2026, which is treated here as authoritative. — [aws.amazon.com/greengrass](https://aws.amazon.com/greengrass)
- Earlier staged cuts: the APT repository was removed 11 February 2022, ML and third-party connectors ended maintenance 31 March 2022, Docker images 30 June 2022, and the Snap 31 December 2023. — [AWS Greengrass V1 maintenance policy](https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html)

**SAP Leonardo IoT / SAP IoT**
- SAP Leonardo IoT was announced at MWC 2019 and became generally available 25 March 2019 as PaaS. — [SAP blog, Apr 2019](https://blogs.sap.com/2019/04/12/upgrading-all-iot-application-enablement-subscriptions-to-leonardo-iot-this-week-get-ready-for-major-improvements/)
- SAP IoT as a standalone PaaS was retired (2208 release note). Community posts cite a retirement date of 31 January 2023, and the service could no longer be contracted under CPEA or Pay-As-You-Go. IoT capabilities moved into SAP Asset Performance Management. This is community-sourced, not a formal SAP notice. — [SAP Community, "SAP IoT retirement and SAP APM"](https://groups.community.sap.com/t5/supply-chain-management-blogs-by-sap/sap-iot-retirement-and-sap-asset-performance-management/bc-p/13533348); [SAP Community Q&A "Retiring SAP IoT"](https://community.sap.com/t5/supply-chain-management-q-a/retiring-sap-iot/qaq-p/12620965)
- The Leonardo umbrella brand (AI, IoT, blockchain, analytics) was dropped. E3 Magazine calls it "Das Leonardo-Desaster." — [E3 Magazine](https://e3mag.com/en/das-leonardo-desaster/)

**GE Predix / GE Digital**
- In mid-2017, Predix took a "time out" for stability. Predix chief Harel Kodesh and CCO Kate Johnson left in 2017. Under CEO Flannery, GE cut about $400M of GE Digital costs and refocused Predix on selling to existing GE energy, aviation and oil-and-gas customers. The original target had been $15B of platform revenue by 2020. — [IndustryWeek](https://www.industryweek.com/technology-and-iiot/article/22024583/ge-predix-platforms-focus-narrows-as-flannery-cuts-digital-spending); [CIO Dive](https://www.ciodive.com/news/ge-to-scale-back-predix-amid-companywide-restructure/510758/)
- 13 December 2018: GE announced plans to set up its digital business as a standalone industrial IoT company and to sell a majority stake in ServiceMax. — [SiliconANGLE, 13 Dec 2018](https://siliconangle.com/2018/12/13/ge-spin-off-digital-business-standalone-industrial-iot-company/)
- IoT Analytics: GE pivoted to internal, vertical deployments in November 2017, and its "$5 billion Predix revenue claim largely reflected internal billing." — [IoT Analytics, 17 Sep 2025](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)

**Siemens MindSphere → Insights Hub**
- Siemens product pages say MindSphere "has evolved into Insights Hub." A Siemens blog of 17 April 2023 placed Insights Hub inside Siemens Xcelerator. Newer pages reference "Industrial Operations X." IoT Analytics dates the rebrand to 2023. — [Siemens Insights Hub](https://www.siemens.com/en-us/products/insights-hub/resources/); [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)

**PTC ThingWorx and Kepware → TPG**
- Sale to TPG announced 5 November 2025 and completed 16 March 2026. PTC's stated rationale was to focus on its core CAD/PLM offerings. — [PTC press release](https://www.ptc.com/en/news/2025/tpg-to-acquire-ptcs-industrial-connectivity-and-iot-businesses); [ARC Advisory, completion](https://www.arcweb.com/blog/ptc-completes-sale-kepware-thingworx-tpg)
- Reported value is about $600M cash plus up to $125M earnout (about $730M). This figure is secondary and unconfirmed against PTC filings. — [MarketScreener](https://in.marketscreener.com/news/tpg-global-llc-completed-the-acquisition-of-kepware-industrial-connectivity-and-thingworx-interne-ce7e5edbdb8ef220)
- Engineering.com's headline: "PTC's IoT dream ends with sale of ThingWorx and Kepware." PTC had bought ThingWorx in 2013 and Kepware in 2016. — [Engineering.com](https://www.engineering.com/ptcs-iot-dream-ends-with-sale-of-thingworx-and-kepware/)

**Arm Mbed OS and online tools; Pelion**
- Announced July 2024: Mbed OS and the Mbed platform reached end of life in July 2026. The website is archived, online builds stopped, and Mbed projects can no longer be built in Keil Studio Cloud or Mbed Studio. Active maintenance and CI had already stopped. Arm recommends Keil MDK v6 Community Edition with CMSIS/RTX, FreeRTOS or Zephyr. A community fork, Mbed CE, continues. — [Arm Mbed blog, "Important Update on Mbed"](https://os.mbed.com/blog/entry/Important-Update-on-Mbed/); [Hackaday, 16 Jul 2024](https://hackaday.com/2024/07/16/times-up-for-mbed); [eeNews Europe](https://www.eenewseurope.com/en/end-of-life-for-mbed-development-tools/)
- Pelion was spun out as an Arm subsidiary (launched as a separate brand 19 November 2020), then sold to SoftBank in November 2021 for about $12M. Izuma Networks announced acquisition of Pelion Device Management on 4 October 2022, funded by SoftBank, and the Pelion client became the Izuma client. — [The Register, 7 Jul 2020](https://www.theregister.com/2020/07/07/arm_iot_cloud_softbank/); [Embedded Computing Design](https://embeddedcomputing.com/technology/iot/device-management/izuma-networks-acquires-pelions-device-management-iot-services-platform); [Edge IR](https://www.edgeir.com/izuma-networks-completes-acquisition-of-pelions-device-management-business-20221010); [eeNews Europe, "Softbank pays US startup to take on Pelion"](https://www.eenewseurope.com/en/softbank-pays-us-startup-to-take-on-pelion)

**Samsung ARTIK and SmartThings Groovy IDE**
- Samsung decided in December 2018 to discontinue the ARTIK hardware line. ARTIK Cloud closed on 15 May 2019 for both paid and free accounts, with less than a month's notice for paid users. — [CNX Software, 14 Mar 2019](https://www.cnx-software.com/2019/03/14/samsung-artik-discontinued-iot-graveyard/); [SmartThings Community](https://community.smartthings.com/t/samsung-discontinuing-artik-cloud-may-15-2019-both-paid-and-free-accounts/161403)
- The SmartThings Groovy IDE and Groovy SmartApps were retired in phases during 2022, with dates moved several times. Cloud hosting of custom Groovy SmartApps was turned off in January 2023, which broke third-party integrations. Exact phase dates are forum-sourced. — [SmartThings Community, "Why Did My Integration Stop Working in January 2023?"](https://community.smartthings.com/t/why-did-my-integration-smartapp-stop-working-in-january-2023/255923/); [Hackaday, 19 Jul 2021](https://hackaday.com/2021/07/19/samsung-shuttering-original-smartthings-hubs)

**Google Android Things (IoT OS and developer console)**
- Announced December 2020. The console stopped accepting new projects on 5 January 2021. Full shutdown was 5 January 2022, when "all project data will be permanently deleted — including build configurations and factory images." — [9to5Google, 17 Dec 2020](https://9to5google.com/2020/12/17/android-things-shutdown/); [Engadget](https://www.engadget.com/google-reveals-timeline-android-things-shutdown-120504721.html)

**Bosch IoT Suite / Bosch IoT Device Management**
- A Bosch notice dated 20 October 2022 said Bosch IoT Device Management "will be discontinued by mid 2024," with customers moved toward major cloud providers' ecosystems. Earlier, Bosch IoT Permissions was ended and Bosch IoT Things on Azure was discontinued. IoT Analytics lists Bosch as exiting in 2024. — [Bosch IoT Suite docs](https://docs.bosch-iot-suite.com/device-management/Other-Bosch-IoT-Suite-services.html); [Bosch IoT Suite news](https://bosch-iot-suite.com/news/bosch-iot-device-management-boschio-to-support-device-management-together-with-major-cloud-providers/); [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)

**Other retreats and consolidations (2018–2026)**
- Relayr (Berlin IIoT): Munich Re/HSB acquired it in 2018 at a $300M valuation, later exited ("withdrawn from initiatives to focus on our core business"), and per IoT Analytics Relayr ceased operations in July 2025. — [EU-Startups, 2018](https://www.eu-startups.com/2018/09/big-exit-berlin-based-iot-startup-relayr-acquired-by-munich-re-for-300-million/); [Coverager](https://coverager.com/munich-res-acquisition-and-exit-from-relayr/); [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)
- Cisco Kinetic and SAP Leonardo were "shut down or absorbed." — [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)
- Blynk Legacy (1.0) was a hobbyist IoT app builder and cloud. Apps were pulled from the stores 30 June 2022, in-app purchases deprecated 30 September 2022, and the legacy server shut down 31 December 2022. There was no automated migration to Blynk IoT, and legacy "Energy" purchases had no value in the new product. — [Blynk legacy FAQ](https://blynk.io/legacy)
- Toit (Danish ESP32 language, IDE and cloud): the cloud service was described as "deprecated." A Wikipedia draft mirror says Toitware ApS ceased operating in 2024 and that two ex-employees maintain the code as volunteers. No primary shutdown announcement was found, so treat this as low-confidence. — [codecentric](https://www.codecentric.de/en/knowledge-hub/blog/toit-will-bring-your-iot-projects-up-to-speed); [toit.io about](https://toit.io/company/about); [Wikipedia draft mirror](https://wikiless.lunar.icu/wiki/Draft:Toit?lang=en)
- Electric Imp (impOS + impCentral IDE/cloud): Twilio acquired it, then sold its IoT business (Super SIM, Programmable Wireless, Microvisor, Electric Imp) to KORE, effective 1 June 2023. On 27 February 2026, KORE Group agreed to be taken private by Searchlight and Abry for about $726M ($9.25/share). impCentral is still listed on the Electric Imp site, but no roadmap was found. — [Twilio blog](https://www.twilio.com/en-us/blog/company/news/electric-imp-is-now-part-of-twilio); [Electric Imp forum](https://forums.electricimp.com/t/kore-acquired-twilio-who-acquired-electric-imp/6800); [Finviz/KORE press release](https://finviz.com/news/324868/kore-announces-agreement-to-be-acquired-by-searchlight-capital-partners-and-abry-partners); [Connect Money](https://www.connectmoney.com/stories/kore-group-to-go-private-in-726m-all-cash-buyout/)
- Losant (low-code IIoT application platform): acquired by SUSE in February 2026 for an undisclosed sum, to become part of SUSE Edge. — [SUSE](https://www.suse.com/news/suse-acquires-losant/); [The Stack](https://www.thestack.technology/suse-targets-tiny-edge-with-losant-acquisition/)
- Golioth (Zephyr-centric device cloud): acquired by Canonical on 3 March 2026 for an undisclosed sum. — [Golioth blog](https://blog.golioth.io/?p=10529); [Hackster](https://www.hackster.io/news/iot-specialist-golioth-gets-snapped-up-by-linux-goliath-canonical-4c738a5a2935)
- Memfault (embedded observability/OTA): acquired by Nordic Semiconductor, announced 24 June 2025, for $120M cash and debt-free. Nordic committed to keep supporting Memfault customers on any hardware. — [Nordic press release](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-acquires-Memfault)
- Particle: acquired by Digi International on 27 January 2026 for $50M cash, with about $20M ARR growing at double digits. Particle had earlier cut about 10% of staff (TechCrunch; date not pinned down). — [Business Wire, 27 Jan 2026](https://www.businesswire.com/news/home/20260127820853/en/Digi-International-Acquires-Particle-to-Accelerate-ARR-Growth-and-Strengthen-Digis-Embedded-as-a-Service-Offering); [TechCrunch](https://techcrunch.com/?p=1951557)
- Cumulocity: Software AG sold it during the 2024 break-up, and a management buyout closed on 20 January 2025, backed by Schroders Capital, Verso Capital and Avedon. — [Cumulocity press release](https://cumulocity.com/press-releases/cumulocity-completes-management-buyout-to-become-the-leading-independent-iot-platform); [RCR Wireless, 20 Jan 2025](https://www.rcrwireless.com/20250120/internet-of-things/cumulocity-software-ag)
- Zerynth (Italy) began in 2015 as a spin-off focused on simplifying embedded programming (Python on MCUs, Zerynth Studio IDE). It is now positioned as an industrial IoT and AI "manufacturing intelligence" vendor for SMEs, with 40+ staff, 200+ customers and about €7.3M raised. — [Zerynth company page](https://www.zerynth.com/company/); [Wikipedia: Zerynth](https://en.wikipedia.org/wiki/Zerynth); [Zerynth 2025 deck](https://www.sigep.it/restricted-assets/digital-profiles/01_ATTACHMENTS/SIG26/Zerynth-Deck-2025-ENG-69301cb02f127.pdf.pdf)

### Inferences
- A small vendor's experience inside a hyperscaler is "thin core survives, upper layers are cut." AWS kept IoT Core and Greengrass V2 but removed Analytics, Events and Greengrass V1. Microsoft kept IoT Hub and DPS but is retiring IoT Central and Sphere and gave away RTOS. The layers that went are the ones that look most like an "IoT IDE": low-code app builders, rules engines and dashboards.
- Developer IDEs and online build tools are the easiest thing for a large vendor to drop. Arm (Mbed online compiler/Studio), Samsung (Groovy IDE), Google (Android Things console) and Toit all shut theirs. Ignition and Node-RED, which run locally, are the counter-examples.
- The independent device-cloud and dev-tool startups that "survived" (Memfault, Golioth, Particle, Losant) mostly exited by being acquired by silicon or OS vendors in 2025–2026: Nordic, Canonical, Digi, SUSE. The prices were modest (Particle $50M on about $20M ARR; Memfault $120M). That points to a capped standalone ceiling for horizontal IoT dev-tool businesses, with strategic sale to a chip or OS vendor as the typical endgame. This is an inference from four data points.
- Zerynth's arc, from embedded-Python IDE to vertical manufacturing-analytics SaaS, is a near-exact precedent for an embedded tool vendor repositioning, but it went the opposite way from "IoT IDE." It moved toward a vertical application to find revenue.

### Gaps
- No primary Toit shutdown announcement and no exact date for the toit.io cloud going offline.
- Cayenne/myDevices: no evidence found either way.
- No exact official dates for each SmartThings Groovy phase. Forum sources disagree on dates.
- The exact AWS announcement date for the Analytics/Events end of support was not retrieved (only the docs with effective dates).
- The Greengrass V1 conflict (1 June vs 7 October 2026) was not fully resolved, though the maintenance-policy page gives 7 October 2026.
- The final fate of GE Digital after 2018 (it stayed inside GE and moved to GE Vernova) was not verified in this pass.
- The Azure IoT Central 2029 notice was seen only through a third-party Message Center mirror.
- Twilio's acquisition date for Electric Imp was not confirmed.

---

## Q2. What happened to customers (migration paths, open-sourcing, partner hand-offs)

### Takeaway
There were four exit patterns, in rough order of how much customer pain they caused:
- **Hard kill with partner hand-off:** Google IoT Core, IBM Watson IoT, ARTIK Cloud, Android Things. Typically 0–12 months' notice, no first-party replacement, and data deleted.
- **Fold into the hyperscaler's own primitives:** AWS Analytics/Events → Kinesis/S3; IoT Central → IoT Hub + Fabric; Greengrass V1 → V2.
- **Divest to a new owner:** ThingWorx/Kepware to TPG; Pelion to SoftBank to Izuma; Electric Imp to Twilio to KORE; Cumulocity MBO.
- **Open-source the code:** Azure RTOS to Eclipse ThreadX; Mbed CE community fork; Toit volunteer-maintained.

Only open-sourcing left customers able to keep running unchanged without a vendor.

### Cited Findings
- Google IoT Core customers were told to "take action early to migrate…to an alternative service" and to contact account managers about partners. The listed partners included SoftServe, SOTEC, ClearBlade (which launched an IoT Core-compatible replacement with a migration path) and EMQ (EMQX), which called itself an official migration partner. — [Wayback snapshot of cloud.google.com/iot-core](https://webcf.waybackmachine.org/web/20230103220349/https://cloud.google.com/iot-core); [EMQ](https://emqx.com/en/blog/why-emqx-is-your-best-google-cloud-iot-core-alternative); [Altoros](https://medium.com/altoros-blog/alternatives-to-google-cloud-iot-core-where-to-migrate-792fe7023c9e)
- Hacker News commenters were "highly critical" and "questioned Google Cloud's commitment to its customers." TechCrunch said customers felt "left in the lurch" despite Google's written deprecation policy. InfoQ described the thread as ranging "from distrust to somewhat understanding." — [TechCrunch, 17 Aug 2022](https://techcrunch.com/2022/08/17/google-cloud-will-shutter-its-iot-core-service-next-year/); [InfoQ](https://www.infoq.com/news/2022/08/google-iot-core-discontinued/)
- TechRadar headline: "Google Cloud is closing IoT Core, leaving user devices stranded." — [TechRadar](https://techradar.com/news/google-cloud-is-closing-iot-core-leaving-devices-stranded)
- Analyst counter-view (MachNation): the shutdown was "not expected to create significant disruption for a great number of IoT enterprise customers," which implies the user base was small. — [Fierce Network](https://www.fierce-network.com/tech/google-cloud-iot-shutdown-likely-have-minimal-impact-csps-machnation)
- IBM Watson IoT customers got roughly 12 months and "should plan to move to alternative services," with no named replacement. — [The Register](https://www.theregister.com/2022/11/15/ibm_set_to_retire_watson/)
- AWS IoT Analytics was steered to Kinesis Data Streams and S3. AWS IoT Events got one year of no new customers and then a hard stop. — [AWS IoT Analytics EoS](https://docs.aws.amazon.com/iotanalytics/latest/userguide/iotanalytics-end-of-support.html); [AWS IoT Events EoS](https://docs.aws.amazon.com/iotevents/latest/developerguide/iotevents-end-of-support.html)
- Greengrass V1 users got a five-year tail: V2 in December 2020, extended life June 2023, endpoints off October 2026, with a written commitment of 12 months' notice. — [AWS Greengrass V1 maintenance policy](https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html)
- Azure IoT Central customers get about three years of notice (to September 2029), a migration playbook, and partner and field-team help. The 2024 false start, a console banner later retracted, created confusion and press coverage of the service's "retirement" two years before any formal decision. — [mc.merill.net AZ569914](https://mc.merill.net/message/AZ569914); [The Register](https://www.theregister.com/2024/02/21/microsoft_azure_iot_central/); [Windows Report](https://windowsreport.com/azure-iot-central-will-be-retired-in-march-despite-microsoft-still-sending-invite-notifications/)
- Azure Sphere customers face a forced hardware redesign, since the MT3620 reaches end of life in July 2026 and the service ends in 2031. Recommended replacements are PSA/SESIP Level 3+ certified MCUs plus IoT Hub, ADR and Device Update. — [Microsoft Tech Community](https://techcommunity.microsoft.com/blog/iotblog/azure-sphere-is-retiring-in-2031---what-you-need-to-know/4504225)
- Azure RTOS users kept a maintained codebase under the MIT licence (Eclipse ThreadX 6.4.1, April 2024), backed by an Eclipse working group. — [Microsoft Tech Community](https://techcommunity.microsoft.com/blog/iotblog/azure-rtos-transition-to-open-source-is-now-complete/4105027); [InfoQ](https://www.infoq.com/news/2023/12/threads-rtos-open-sourced)
- Mbed users: the codebase was archived on GitHub, online and IDE builds ended, and users were told to move to Keil MDK v6 / CMSIS / Zephyr / FreeRTOS. The community fork Mbed CE is active. Arduino's Mbed-based cores were also affected. — [Arm Mbed blog](https://os.mbed.com/blog/entry/Important-Update-on-Mbed/); [Arduino blog](https://blog.arduino.cc/2024/page/12); [PlatformIO community](https://community.platformio.org/t/mbed-eol-announced/41674)
- SAP IoT customers were moved into SAP Asset Performance Management, where IoT became an embedded capability. — [SAP Community](https://groups.community.sap.com/t5/supply-chain-management-blogs-by-sap/sap-iot-retirement-and-sap-asset-performance-management/bc-p/13533348)
- Bosch IoT Device Management customers were moved toward hyperscaler ecosystems. Managed Bosch IoT Things instances were migrated to Eclipse Ditto, Bosch's open-source digital-twin project, as a device-management backend. — [Bosch IoT Suite news](https://bosch-iot-suite.com/news/bosch-iot-device-management-boschio-to-support-device-management-together-with-major-cloud-providers/); [Bosch IoT Suite, Ditto migration](https://bosch-iot-suite.com/news/migration-of-managed-bosch-iot-things-instances-to-ditto-as-device-management-backend/)
- Android Things: project data, build configurations and factory images were permanently deleted on 5 January 2022. — [9to5Google](https://9to5google.com/2020/12/17/android-things-shutdown/)
- ARTIK Cloud paid users got less than a month's notice. — [SmartThings Community](https://community.smartthings.com/t/samsung-discontinuing-artik-cloud-may-15-2019-both-paid-and-free-accounts/161403)
- Blynk Legacy: no automated migration. Commercial customers on legacy Startup/Business plans were exempted and kept their business servers. Hobbyists lost purchased "Energy" credit, and many objected to subscription-only pricing. — [Blynk legacy FAQ](https://blynk.io/legacy); [Blynk community](https://community.blynk.cc/t/blynk-2-0-pricing-model-not-friendly-to-hobbyists/67191)
- Pelion device-management customers moved from Arm to SoftBank to Izuma, where the client SDK was renamed Izuma client. — [Embedded Computing Design](https://embeddedcomputing.com/technology/iot/device-management/izuma-networks-acquires-pelions-device-management-iot-services-platform)
- Acquired-startup customers got continuity commitments: Nordic will support Memfault "regardless of their choice of hardware," and Digi said Particle tools and kits continue. — [Nordic](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-acquires-Memfault); [Electronic Design](https://electronicdesign.com/technologies/communications/iot/article/55354275/electronic-design-digi-acquires-particle-for-50-million)

### Inferences
- Notice periods ranged from under one month (ARTIK Cloud) through about 12 months (Google, IBM, AWS Events) to 3–5 years (Azure IoT Central, Greengrass V1, Azure Sphere). Microsoft and AWS lengthened notice periods after the 2022 Google/IBM backlash, which suggests the backlash had an effect.
- Where firmware was involved (Sphere, Mbed, Android Things, Electric Imp), customers faced the costliest migration: reflashing or redesigning fielded hardware. Cloud-only migrations (IoT Core to EMQX/ClearBlade) were cheaper. For an IoT IDE that generates device firmware, this is the most important risk to de-risk for customers.
- Open-sourcing (ThreadX, Ditto, Mbed CE) was the only path that removed vendor-continuity risk entirely. A small vendor can make the same promise in advance through a source escrow or open-core commitment.

### Gaps
- No quantitative data on how many customers churned or how many devices were stranded in any shutdown.
- No primary customer case studies of migration cost (e.g., per-device cost to move off IoT Core or Sphere) were retrieved.

---

## Q3. Survivors and why: business models

### Takeaway
The durable IoT businesses share one or more of these traits:
- **Revenue tied to something physical or per-site, not per-message cloud usage:** Ignition per-server unlimited licence; Particle, Nabu Casa and Espressif selling hardware/modules; Tuya PaaS bundled with modules.
- **Self-hostable or local-first, with a paid managed option:** ThingsBoard, Node-RED/FlowFuse, Home Assistant, ESP RainMaker in the customer's own AWS account.
- **Owned by or sold to a silicon/OS company that uses the cloud to sell chips:** Espressif RainMaker, Nordic/Memfault, Canonical/Golioth, Digi/Particle.
- **Narrow vertical or job focus:** Memfault (observability), Ignition (SCADA/HMI), Tuya (consumer smart home OEMs).

Pure-play horizontal "IoT PaaS" for the general enterprise is the shape that failed.

### Cited Findings

**ThingsBoard**
- On 29 September 2026, ThingsBoard 4.4 merged the Community and Professional Editions into one source-available product under the Business Source License 1.1. The free licence covers commercial use up to 100 devices on one server with branding, and non-commercial use up to 1,000 devices. A commercial licence is needed beyond that or for white-labelling. Each release converts to Apache 2.0 after four years (4.4.0 converts on 29 September 2030). Existing CE users get a perpetual no-charge "Community Grant" at current scale. The company says "The new license is what keeps ThingsBoard's development sustainable." Revenue comes from commercial licences, white-label, support, ThingsBoard Cloud (managed), development services and training. — [ThingsBoard blog, 29 Sep 2026](https://thingsboard.io/blog/one-thingsboard-source-available/); [ThingsBoard Community Grant](https://thingsboard.io/community-grant-program/)

**Particle**
- Integrated hardware + connectivity + cloud platform, with subscription revenue of about $20M ARR at acquisition. Acquired by Digi International for $50M on 27 January 2026 to lift Digi's IoT ARR by more than 60%. Total funding was $75–90M+ (including a $40M Series C in 2019 led by Qualcomm Ventures), so the exit was below capital raised. — [Business Wire](https://www.businesswire.com/news/home/20260127820853/en/Digi-International-Acquires-Particle-to-Accelerate-ARR-Growth-and-Strengthen-Digis-Embedded-as-a-Service-Offering); [Particle blog, Series C](https://www.particle.io/blog/particle-raises-40m-series-c/)

**Blynk**
- Moved from one-off in-app "Energy" purchases (Legacy) to subscription tiers plus an Enterprise white-label tier with private or dedicated cloud, custom-branded apps and SLAs, priced by device count, messaging and branding. Forum posts say one-off purchases "didn't cover their ongoing cloud server costs." That is user commentary, not an official statement. — [Blynk business plan](https://blynk.io/pricing/business-plan); [Blynk legacy FAQ](https://blynk.io/legacy); [Blynk community](https://community.blynk.cc/t/blynk-1-0-legacy-retirement-timeline-announced/61193)

**Golioth**
- Zephyr-focused device-management/OTA cloud. Raised a $2.5M seed (2021) and $4.6M. Expanded free-tier device management and OTA. Acquired by Canonical on 3 March 2026. — [PR Newswire](https://www.prnewswire.com/news-releases/golioth-a-cloud-platform-for-iot-announces-private-beta-and-2-5m-seed-301304014.html); [Golioth blog $4.6M](https://blog.golioth.io/?p=4412); [Golioth blog, Canonical](https://blog.golioth.io/?p=10529)

**Memfault**
- Device observability, crash reporting and OTA SaaS, hardware-agnostic. Acquired by Nordic for $120M on 24 June 2025, to be integrated into nRF Cloud to make Nordic a "chip-to-cloud" vendor. — [Nordic](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-acquires-Memfault)

**Tuya**
- Consumer-IoT PaaS bundled with modules for OEM brands, plus SaaS. FY2025 revenue was $321.8M (+7.8%), with PaaS at $231.2M (48.3% gross margin) and SaaS and others at about $44.9M (73.2% gross margin in H1 2025). First operating profit was $11.5M, against a $47.6M loss in 2024. PaaS growth is slowing (DBNER 102%; premium PaaS customers down to 291). — [Tuya FY2025 results](https://tuya.gcs-web.com/news-releases/news-release-details/tuya-reports-fourth-quarter-and-fiscal-2025-unaudited-financial)

**Espressif ESP RainMaker**
- A "private IoT cloud" deployed as CloudFormation stacks in the customer's own AWS account and region, licensed by Espressif, with paid deployment and customisation services. The public instance is for evaluation only. Espressif's materials say "all data belongs to the customers' enterprise." — [RainMaker docs: public vs private](https://docs.rainmaker.espressif.com/docs/deploy/deployment-overview/public-vs-private-rainmaker); [RainMaker deployment architecture](https://docs.rainmaker.espressif.com/docs/deploy/deployment-overview/deployment-architecture); [ESP RainMaker product brief](https://www.espressif.com/sites/default/files/RainMaker/ESP%20RainMaker%20Product%20Brief_EN.pdf)

**Inductive Automation Ignition**
- Sold per server with "unlimited" clients, tags, designers and connections. The founder recalls that many said the unlimited model "would cause us to go bust." Privately owned for 20+ years with installs in 140+ countries (company-sourced). No audited revenue was found. — [Inductive Automation, Founder's message](https://inductiveautomation.com/the-new-scada); [Inductive Automation Unlimited](https://inductiveautomation.com/ignition/unlimited); [Wikipedia](https://Www.Wikipedia.org/wiki/Inductive_Automation)

**Node-RED / FlowFuse**
- Open-core: open-source Node-RED plus a commercial FlowFuse platform (cloud or self-hosted) that adds fleet management, RBAC, SSO and an industrial focus. Raised a $7.25M seed in October 2022 (Cota Capital, Open Core Ventures) and about $7.2M in June 2025 to build an end-to-end industrial-data platform. — [FlowFuse blog, Oct 2022](https://flowfuse.com/blog/2022/10/seed-round-bring-node-red-to-enterprise/); [Newswire, 2025](https://www.newswire.com/news/flowfuse-secures-7-2-million-investment-to-drive-end-to-end-industrial-22589812)

**Home Assistant / ESPHome / Nabu Casa**
- Nabu Casa was founded in 2018 to fund Home Assistant through a $5/month cloud subscription, with no investor money. Revenue comes from cloud subscriptions plus official hardware. Since April 2024, the Open Home Foundation (non-profit) owns Home Assistant, ESPHome, Zigpy and 240+ projects, with the majority of Nabu Casa profits funding it. — [Home Assistant blog, 2018](https://www.home-assistant.io/blog/2018/09/17/thinking-big/); [Open Home Foundation](https://openhomefoundation.org/organization); [CNX Software, Apr 2024](https://www.cnx-software.com/2024/04/23/open-home-foundation-manage-home-assistant-esphome-zigpy-over-240-open-source-smart-home-projects/)
- Reported scale: "2 million homes" (State of the Open Home, April 2025). — [Home Assistant blog](https://home-assistant.io/blog/2025/04/16/state-of-the-open-home-recap/)
- On 2 October 2026, Nabu Casa announced it was renaming "Home Assistant Cloud" to "Home Assistant Link" under the title "Big Tech ruined the cloud, so we're renaming ours." It positions the paid service as an optional add-on, with the home continuing to work if the subscription lapses. — [Home Assistant blog, 2 Oct 2026](https://home-assistant.io/blog/2026/10/02/big-tech-ruined-the-cloud-so-were-renaming-ours/)

**Others still operating**
- Cumulocity is independent after a January 2025 MBO, has 270+ employees, and Forrester names it a Leader in industrial IoT. — [Wikipedia: Cumulocity](https://en.wikipedia.org/wiki/Cumulocity)
- Losant was recognised in the 2025 Gartner Magic Quadrant for Global Industrial IoT Platforms, then acquired by SUSE in 2026. — [SUSE](https://www.suse.com/c/suse-acquires-losant-to-complete-edge-vision)

### Inferences
- **Self-host plus paid managed** is the dominant surviving shape (ThingsBoard, FlowFuse, Home Assistant, RainMaker-in-your-account). It removes the "what if you shut down" objection that Google and IBM made salient.
- **Licence tightening** is the stress signal. ThingsBoard's move to BUSL with a 100-device commercial threshold (2026) and Blynk's move to subscriptions (2022) show that free and open tiers had to be monetised harder to stay viable. A small vendor should design the paid boundary from day one, not retrofit it.
- **Silicon is the patient capital.** Espressif, Nordic, Digi and Canonical fund or buy dev clouds because cloud and tooling sell chips, modules or OS subscriptions. A small IoT-IDE vendor's realistic partners or acquirers are MCU, module and display vendors, not hyperscalers.
- **Ignition is the closest analogue to EDT HMI Studio** (visual HMI/SCADA designer, sold per server, unlimited tags and clients). It is the strongest evidence that a locally run designer and runtime with simple, predictable licensing can be durable without owning an IoT cloud.

### Gaps
- No verified revenue or headcount figures for Inductive Automation, ThingsBoard, Blynk, FlowFuse, Nabu Casa or Golioth.
- RainMaker licence pricing was not found.
- Particle's layoff date was not pinned.
- Blynk's ownership and funding were not checked in this pass.

---

## Q4. Analyst commentary on why horizontal IoT platforms failed

### Takeaway
Analysts agree on the reasons. Generic, horizontal IoT platforms were too abstract to deliver operational value, too hard to integrate in brownfield sites, and commoditised by hyperscalers. Demand also arrived far below 2015-era forecasts, and most enterprise projects stalled in "pilot purgatory." The market concentrated: the top five hyperscalers' share of agnostic IoT platforms rose from 39% (2020) to 60% (2024). Value shifted to device management, data harmonisation, OEM-specific workflows and vertical applications.

### Cited Findings
- IoT Analytics counted about 450 IoT platform companies in 2017, 620 in 2019, and 613 in 2021, when "for the first time, more IoT platform companies discontinued their operations than new ones were created." Of the 620 from 2019, 426 were still active in 2021 and 182 new ones had appeared. About 30 pivoted to an application or vertical solution and 10 to security. — [IoT Analytics, 2019/2020 landscape](https://iot-analytics.com/iot-platform-companies-landscape-2020); [IoT Analytics, 2021/2022 landscape](https://iot-analytics.com/iot-platform-companies-landscape/); [eeNews Europe](https://www.eenewseurope.com/en/consolidation-shrinks-iot-platform-players)
- IoT Analytics (17 September 2025, "What happened to IoT platforms"):
  - The 620+ peak "was never reached again."
  - The top five hyperscalers (Microsoft, AWS, Huawei, Alibaba, Oracle) held 60% of the agnostic IoT platform market in 2024, up from 39% in 2020.
  - There were 12.2B IoT connections in 2021, against Ericsson's forecast of 50B.
  - McKinsey's projected $11T by 2025 "fell short."
  - The exits it lists: GE (2017 pivot), Honeywell to verticals (2019), Google (August 2022), SAP (2022), IBM (2023), Bosch (2024), Siemens rebrand (2023), Relayr (July 2025), Cisco Kinetic and SAP Leonardo.

  — [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)
- IoT Analytics' explanation of the failure:
  - Generic platforms were "too broad, rigid, or abstract" and delivered middleware and APIs while leaving integration to customers.
  - Brownfield complexity made standardised onboarding unrealistic.
  - All-in-one application-enablement platforms backfired because developers preferred mainstream tools.
  - Maintaining full stacks became financially unsustainable.
  - Hyperscalers commoditised the MQTT broker, rules engine and digital-twin graph.

  Its outlook is a hybrid edge-cloud model ("cloud for scale, edge for intelligence") with value pockets in device management, data harmonisation, OEM-specific workflows and vertical apps. — [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)
- McKinsey (November 2021):
  - IoT could enable $5.5–12.6T of value by 2030, up from $1.6T in 2020.
  - The 2025 estimate was cut from the 2015 range of $3.9–11.1T to $2.8–6.3T because of headwinds.
  - 70% of manufacturers had been unable to scale beyond pilots ("pilot purgatory").
  - Up to three-quarters of the high-end value depends on interoperability.
  - Factories are the largest setting, at $1.4–3.3T (26%).

  — [McKinsey, "IoT value set to accelerate through 2030"](https://www.mckinsey.com/capabilities/tech-and-ai/our-insights/iot-value-set-to-accelerate-through-2030-where-and-how-to-capture-it); [McKinsey PDF](https://mckinsey.com/~/media/mckinsey/business%20functions/mckinsey%20digital/our%20insights/iot%20value%20set%20to%20accelerate%20through%202030%20where%20and%20how%20to%20capture%20it/the-internet-of-things-catching-up-to-an-accelerating-opportunity-final.pdf); [Axios](https://www.axios.com/internet-of-things-mckinsey-study-1b11baae-8521-4a5d-8f44-14ad136bc3da.html)
- Gartner placed IoT platforms near the "peak of inflated expectations" and heading into the trough in its 2017 Emerging Technologies cycle. In its 2020 supply-chain cycle, IoT sat at the bottom of the trough of disillusionment. The 2026 Hype Cycle for IoT (22 June 2026) frames IoT platforms as something to "integrate…into AI/digital business initiatives." The platform position is not visible in the abstract. — [iothub.com.au, "Crunch time for IoT platforms: Gartner"](https://www.iothub.com.au/news/crunch-time-for-iot-platforms-gartner-471341); [Gartner press release, 9 Sep 2020](https://www.gartner.com/en/newsroom/press-releases/2020-09-09-gartner-2020-hype-cycle-for-supply-chain-strategy-shows-internet-of-things-is-two-to-five-years-away-from-transformational-impact); [Gartner Hype Cycle for IoT abstract](https://www.gartner.com/en/documents/8032333)
- Hyperscaler marketplaces listed nearly 3,000 IoT offerings combined (Microsoft and AWS, September 2025). Partner solutions on top of thin hyperscaler primitives, not first-party application platforms, now carry the application layer. — [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)

### Inferences
- "IoT platform" as a horizontal category is now a commodity substrate (hyperscaler IoT Hub/IoT Core plus open-source brokers). Durable value sits either below it (device firmware, observability, OTA, security; the Memfault/Golioth layer) or above it (vertical apps, OEM workflows, HMI/SCADA; the Ignition/Tuya layer). A new "IoT development environment" should be positioned explicitly in one of those layers, not as another horizontal platform.
- The pilot-purgatory finding argues for tools that shorten the path from prototype to production device, not tools that add yet another cloud console.

### Gaps
- No post-2021 IoT Analytics platform-company count was retrieved (the 2025 article says the peak was never regained but gives no new number in the free section).
- No current Gartner hype-cycle phase for IoT platforms. The full 2025/2026 reports are paywalled.
- No Stacey on IoT or EE Times retrospective was retrieved in this pass.

---

## Q5. Lessons for developer tools vs. cloud platforms: does owning the cloud help or hurt a small vendor? Lock-in fear and demand for portable/self-hostable stacks

### Takeaway
For a small vendor, running the device cloud is a liability more often than an asset:
- It creates hosting costs that one-off tool revenue does not cover (Blynk Legacy).
- It is the first thing buyers ask "what if you disappear?" about, after Google, IBM, Samsung, Arm and Toit.
- It competes directly with hyperscaler primitives that analysts say are commoditised.

The evidence favours a locally running designer/IDE with portable output (standard RTOS/Zephyr/FreeRTOS firmware, MQTT, open formats). Monetisation should come from tool licences, per-device or per-site runtime licences, and an optional, self-hostable or bring-your-own-cloud backend. Partnering with MCU and display vendors is more realistic than building a proprietary cloud.

### Cited Findings
- **Cloud-dependent tools die with the vendor's cloud.**
  - Mbed's online compiler and Keil Studio Cloud builds stopped in July 2026.
  - Android Things project data and factory images were deleted.
  - ARTIK Cloud closed with under a month's notice.
  - Groovy IDE closure broke integrations.

  — [Arm Mbed blog](https://os.mbed.com/blog/entry/Important-Update-on-Mbed/); [9to5Google](https://9to5google.com/2020/12/17/android-things-shutdown/); [SmartThings Community](https://community.smartthings.com/t/samsung-discontinuing-artik-cloud-may-15-2019-both-paid-and-free-accounts/161403)
- **Hosting costs outran one-off pricing.** Blynk Legacy's one-off "Energy" purchases reportedly "didn't cover their ongoing cloud server costs" (community claim). The fix was subscription plus white-label private cloud. — [Blynk community](https://community.blynk.cc/t/blynk-1-0-legacy-retirement-timeline-announced/61193); [Blynk legacy FAQ](https://blynk.io/legacy)
- **Customer reaction to IoT Core showed trust loss.** Commenters "questioned Google Cloud's commitment to its customers" and felt "left in the lurch" despite a written deprecation policy. — [TechCrunch](https://techcrunch.com/2022/08/17/google-cloud-will-shutter-its-iot-core-service-next-year/)
- **Vendors now market exit-safety explicitly.**
  - Home Assistant renamed its cloud to "Link" (October 2026), saying users "won't be stuck," the home keeps working if they unsubscribe, and contrasting this with Big Tech devices that stop working when support is dropped.
  - Espressif markets RainMaker as deployable in the customer's own AWS account, with data owned by the customer.
  - ThingsBoard gives existing CE users a perpetual free grant and converts each release to Apache 2.0 after four years.

  — [Home Assistant blog](https://home-assistant.io/blog/2026/10/02/big-tech-ruined-the-cloud-so-were-renaming-ours/); [RainMaker docs](https://docs.rainmaker.espressif.com/docs/deploy/deployment-overview/public-vs-private-rainmaker); [ThingsBoard blog](https://thingsboard.io/blog/one-thingsboard-source-available/)
- **Open-sourcing is the accepted way to retire responsibly.** Microsoft moved Azure RTOS to Eclipse ThreadX (MIT) and Bosch moved Things to Eclipse Ditto, while Mbed CE was a community fork that followed Arm's archive. — [Eclipse Foundation](https://eclipse-foundation.blog/2023/11/21/introducing-eclipse-threadx/); [Bosch IoT Suite](https://bosch-iot-suite.com/news/migration-of-managed-bosch-iot-things-instances-to-ditto-as-device-management-backend/); [Hackaday](https://hackaday.com/2024/07/16/times-up-for-mbed)
- **Hyperscalers commoditised the IoT cloud core.** IoT Analytics says broker, rules engine and twin graph were commoditised, pushing smaller vendors to verticals or cloud integration. It also says developers "preferred mainstream tools" over all-in-one application-enablement platforms. — [IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)
- **Cloud-plus-tools startups' best exits were to silicon/OS vendors at modest multiples.**
  - Particle sold for $50M on about $20M ARR, after raising $75M+.
  - Memfault sold for $120M.
  - Golioth and Losant sold for undisclosed sums.

  — [Business Wire](https://www.businesswire.com/news/home/20260127820853/en/Digi-International-Acquires-Particle-to-Accelerate-ARR-Growth-and-Strengthen-Digis-Embedded-as-a-Service-Offering); [Nordic](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-acquires-Memfault)
- **A locally installed designer with server licensing has proven durable.** Ignition has sold per-server unlimited licences for 20+ years, without depending on a vendor-hosted cloud. — [Inductive Automation](https://inductiveautomation.com/the-new-scada)
- **Even hyperscalers now give long notice and migration playbooks.** Azure IoT Central gets three years and a playbook, and Greengrass V1 got a 12-month notice commitment and about three years of extended life. A small vendor will be benchmarked against these terms. — [mc.merill.net AZ569914](https://mc.merill.net/message/AZ569914); [AWS Greengrass V1 policy](https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html)

### Inferences
1. **Don't make the cloud the product.** For an EHS-derived IoT IDE, the defensible asset is the designer plus code generator plus device runtime, the same niche as Ignition, SquareLine and Node-RED. A vendor-hosted device cloud adds fixed cost and buyer risk, and competes with commodity hyperscaler services and with the open-source stacks (ThingsBoard, EMQX, Node-RED) that buyers already trust.
2. **Make exit-safety a sales feature from day one.** This could include:
   - output that builds with standard toolchains (Zephyr, FreeRTOS, ThreadX, ESP-IDF) without the IDE;
   - an offline/local IDE;
   - a self-hostable or bring-your-own-cloud backend (the RainMaker pattern);
   - published data and project formats;
   - a written deprecation policy with a 12-month minimum notice;
   - a source-escrow or delayed-open-source clause (the ThingsBoard BUSL four-year pattern).

   The post-2022 buyer has seen Google, IBM, Samsung, Arm and Microsoft retire IoT tools and will ask.
3. **Monetise what is tied to shipping devices, not cloud consumption.** Options are per-seat tool licences, per-device or per-SKU runtime royalties, or per-site/server licences (the Ignition pattern), plus white-label and support. Blynk and ThingsBoard both show that free tiers must have a deliberately designed commercial threshold.
4. **The realistic strategic home is silicon or OS vendors.** Espressif, Nordic, Digi, Canonical and SUSE bought or built IoT dev tooling in 2025–2026. A spin-off should cultivate MCU, display-module and RTOS partners as distribution channels and potential acquirers, rather than position against hyperscalers.
5. **Avoid the "horizontal platform" label.** Analysts associate it with the 2017–2025 failure wave. Positioning should name a concrete job (e.g., "HMI + connectivity for MCU-based devices, prototype to production") and a vertical or OEM segment, consistent with IoT Analytics' value pockets (device management, OEM workflows, vertical apps).
6. **Firmware-coupled lock-in is the most feared.** Customers in Azure Sphere, Mbed, Android Things and ARTIK faced hardware redesigns or reflashing. An IoT IDE that emits firmware should guarantee that generated code and runtime libraries remain buildable and licensed in perpetuity for shipped products.

### Gaps
- No survey data quantifying enterprise lock-in fear or the share of buyers requiring self-hosting after 2022 was found. Evidence is qualitative: press coverage, HN reaction and vendor positioning.
- No direct data on the profitability of self-hosted vs SaaS models for small IoT vendors.
- Sources on SquareLine Studio, Arduino Cloud and other HMI-adjacent tool vendors' cloud strategies were outside this pass's scope and may be covered by other researchers.
