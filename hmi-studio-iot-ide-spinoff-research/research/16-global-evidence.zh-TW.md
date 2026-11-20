# EHS 分拆為 IoT IDE：全球證據縮小了市場，也指明了買家

研究日期：2026 年 11 月 20 日，Asia/Taipei；同日完成第二輪與第三輪查證（結果見「第二輪查證結果」與「第三輪查證結果」）。

本文用全球網路證據回答：「EDT HMI Studio（以下稱 EHS）目前是綁定 EDT 硬體的 No Code RAD 工具；如果分拆給一家純軟公司獨立經營，能不能改成 IoT 整合開發環境？」範圍包括市場規模與需求、併購／募資／分拆先例與出場路徑、IoT 平台的退場與存活者、開發者調查與 AI 寫程式的影響、區域市場，以及台灣的分拆機制。競品價格、程式碼耦合、MCU 研發痛點已在先前 15 份研究處理，本文不重複，只在第一節說明全球證據確認、修正與新增了哪些結論。

> **本文是給 EDT 經營層的研究材料，不是法律、稅務或投資意見。** 是否分拆、以什麼形式分拆，屬於 EDT 經營層與董事會的決定。文中的法條與稅務處理，執行前必須由台灣律師與會計師確認。

**標示說明**

| 標示 | 意思 |
| --- | --- |
| （無標示） | 有一手來源或可信報導的連結 |
| 單一來源 | 只找到一個來源 |
| 二手來源 | 搜尋摘要、第三方鏡像、廠商轉載或轉述，原文沒有直接讀到 |
| 未驗證 | 研究者背景知識，或來源互相矛盾，這次沒有確認 |
| 廠商調查 | 由有商業利益的廠商執行或贊助 |
| 市調區間 | Fortune、MarketsandMarkets 這類聯合市調，各家差距大、定義不明，只能看範圍 |
| 推論 | 本研究的判斷，不是來源的說法 |

## 結論

**可以改。全球證據支持先前「不要做通用 IoT IDE」的判斷，但也把天花板和結局講得更清楚：這會是一家小而能賺錢的工具公司，最可能的結局是被晶片商或工具整合商收購，而不是 VC 規模的平台公司。**

1. **市場是真的，但很窄。** 2025 年底全球約有 211 億台連網裝置，MCU 營收約 US$232 億。但真正付費買嵌入式開發工具的錢一直很少：VDC 估計 2009 年整個市場約 US$2.9 億，而且承受價格壓力。只有 13% 的嵌入式開發者經常做圖形化系統設計（JetBrains，廠商調查）。（推論）合理目標是年營收數百萬到一千多萬美元的利基公司。
2. **「IoT 平台」是負資產，「本機工具加上可帶走的輸出」才活得下來。** 2019 至 2026 年間，Google、IBM、AWS、Microsoft、SAP、Bosch、PTC、Arm、Samsung 都收掉或賣掉了 IoT 平台的上層與開發工具。IoT Analytics 統計的 620 家平台公司高峰再也沒有回來。活下來的是 Ignition 這種本機設計器按伺服器授權、ThingsBoard 這種可自架加付費授權，以及被晶片商或作業系統公司買走的 Memfault、Golioth、Particle。
3. **AI 寫程式不會消滅 Low Code，但會讓「拖拉編輯器」貶值，讓「AI 讀得懂的專案模型、確定性的程式碼產生器與驗證」升值。** ST、NXP、Nordic、Espressif、Microchip 在 2025 至 2026 年都推出給 AI agent 用的 MCP 伺服器或助手；ST 是讓 agent 去呼叫 CubeMX 產生程式碼，不是取代 CubeMX。EHS 應該成為 AI agent 呼叫的那一層。
4. **出場以策略收購為主，價格在數千萬美元級。** Particle 以 US$5,000 萬賣給 Digi，當時年經常性收入約 US$2,000 萬；Memfault 以 US$1.2 億賣給 Nordic，約為其 2024 年底年經常性收入 US$720 萬的 16.7 倍（推論計算）；froglogic 以 €3,000 萬賣給 Qt Group，約 4.6 倍營收；Qt Group 以約 €2.04 億買下 IAR，約 4.7 倍營收（推論計算）。被晶片商買走的工具，常常變成只支援自家晶片。
5. **區域差異很大。** 中國的嵌入式 GUI 工具多半免費，錢從晶片或雲端收，不適合賣授權；日本要透過代理商和晶片商生態，價格按專案報價；韓國大廠以 Qt 為標準，MCU 等級靠晶片商的免費工具；歐盟 RED 資安要求（EN 18031）已強制，CRA 已開始通報義務並將在 2027-12-11 接手 RED 的資安要求，美國有 FDA SBOM 要求、Cyber Trust Mark 與車聯網規則，讓「非中國供應鏈、交得出 SBOM」的台灣廠商有賣點（推論）。
6. **台灣有成熟的分拆路徑。** 企業併購法的「新設分割」可以讓員工年資延續，並免印花稅、營業稅等（第 39 條），友達、宏碁都用過。第 44 條的所得免稅要求把新公司股份全數轉給股東，EDT 自己保留股份時不適用；但財政部函釋讓分割給 100% 子公司時以帳面價值計算，2024 年的行政法院判決仍照此適用。自行研發的 IP 帳面價值低，分割所得可能接近零；代價是新公司承接這個低稅務成本，日後出售 IP 或事業時要就全部增值課稅（推論，需會計師確認）。產業創新條例第 12 條之 1 讓技術作價取得的股票延緩課稅，公司（EDT）也適用，到 2029 年底。國發基金天使方案對單一公司的初次投資上限仍是 NT$2,000 萬，NT$1 億是含後續增資的累計上限，而且 EDT 作為控股股東不能當搭配的天使。台灣種子輪中位數約 US$101 萬，資金多流向硬體與生技；純開發工具公司在台灣募資是逆風（推論）。
7. **分拆仍是 EDT 的決定，而且前提還沒成立。** 全球先例顯示，留在硬體母公司裡的工具，母公司策略一轉就可能被免費化、停產或捐出（Mbed、Azure RTOS）。但分拆要成功，新公司必須有不依賴母公司硬體的付費客戶（Qt 的前例）。台灣也有反例：研華與資策會合資的雲研物聯，不到兩年就因「短期獲利不易」收回研華。EHS 目前還沒有這樣的客戶。

## 一、和先前研究的對照

### 全球證據確認的結論

| 先前結論 | 出處 | 全球證據 |
| --- | --- | --- |
| 不做通用 IoT IDE，不自己經營雲 | [README](../README.zh-TW.md) 結論第 3 點、[10 號](../research/10-ehs-paid-product-roadmap.zh-TW.md)第六節 | 大廠一路退場（見第三節）；前五大雲端廠商在 2024 年拿下通用 IoT 平台市場的 60%，2020 年只有 39%（[IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)） |
| 錢不收在編輯器上 | [14 號](../research/14-mcu-software-monetization.zh-TW.md) | VDC 說獨立嵌入式工具市場「持續承壓」；中國工具免費；只有 13% 的開發者常做圖形化設計（見第二、六節） |
| 晶片商會把能幫它賣晶片的工具做成免費 | 14 號規律 4 | 2025 至 2026 年晶片商的 AI 工具全部免費或免費測試版（NXP 是「實驗性測試版」）；emWin 在 Renesas MCU 上量產免費；TouchGFX 免費但限 ST 晶片（主要是 STM32）（見第四、五、六節） |
| CRA 是有截止日、管理層會撥預算的項目 | [15 號](../research/15-mcu-software-opportunities.zh-TW.md)第 1 名 | 通報義務 2026-09-11 已生效，罰款最高 €1,500 萬或全球營收 2.5%；機器可讀 SBOM 2027-12-11 強制（見第六節） |
| AI 的價值在「產生之後能驗證」 | 15 號第 2 名 | 378 次真機實驗：專家撰寫的硬體 skills 讓首次成功率（pass@1）從 69.0% 提高到 95.2%，由 LLM 自己寫的 skills 反而降到 66.7%；ST 的 agent 套件保留 CubeMX 做確定性產生（見第四節） |
| Particle 被 Digi 收購（先前只有單一來源） | [02 號](../research/02-competitors.zh-TW.md)、03 號 | 已由 [Business Wire](https://www.businesswire.com/news/home/20260127820853/en/Digi-International-Acquires-Particle-to-Accelerate-ARR-Growth-and-Strengthen-Digis-Embedded-as-a-Service-Offering) 證實：2026-01-27，US$5,000 萬現金 |

### 全球證據修正或講得更精確的結論

| 先前說法 | 修正後 |
| --- | --- |
| 雲端正在免費化（02、14 號） | 更精確地說：大廠只留下薄薄的連線層（Azure IoT Hub、AWS IoT Core），上層被砍；同時獨立廠商在**收緊**免費版。ThingsBoard 4.4（2026-09-29）改為 BUSL 授權，商業免費限 100 台；Blynk 2022 年改成訂閱制。新公司的免費版要從第一天就設計好付費門檻 |
| ThingsBoard 是開源 CE，可當上雲對接目標（02 號） | 仍可對接，但授權已變：客戶超過 100 台要買商業授權。對接時要讓客戶知道 |
| VS Code 付費擴充是主要付費介面（README 結論第 5 點） | AI agent（Claude Code、Cursor、Copilot）已經是工具的新使用者。核心資產應是**引擎、CLI、MCP 伺服器與可 diff 的文字專案格式**，VS Code 擴充與桌面版都是外殼（推論） |
| No Code 是賣點 | 對工程師不是。汽車廠的 Simulink 調查（n=23）中，軟體工程師評分最低，最弱的一項是協作與 Git。對工程師要講「結構化模型加上看得懂的 C」，No Code 只留給 OEM 通路（推論） |
| 充電樁、家電、工控、醫療並列為垂直市場 | 有一手數據證明在成長的是公共充電樁（IEA：2024 年新增逾 130 萬個，年增逾 30%；2025 年新增近 180 萬個，年增逾 33%，二手來源）；智慧家庭量大但成長慢（IDC：2025 年約 9.3 億台，+4.4%）；醫療與工控面板沒有可用的台數資料。充電樁應排第一（推論） |
| 新公司 4 至 6 人，12 個月內有經常性收入（10 號） | 人力與時程仍是假設。全球先例顯示獨立工具公司天花板低，VC 支持的 IoT 工具新創多在成立 5 至 7 年後被收購。財務計畫應以「小而賺錢」或「策略收購」為基準，不以 VC 式成長為前提（推論） |
| Toitware 據報 2024 年停止營運（06 號） | 第二輪查證：Toitware ApS（CVR 38984594）2024-08-20 進入破產，2025-10-28 破產後解散（[丹麥 CVR 第三方鏡像](https://danskcvrapi.dk/en/news/38984594-ophor-2025-10-28)，二手來源；第三輪官方 CVR 仍被機器人驗證擋住）。Toit 語言本身仍由原創辦人以開源維護，2026-10-31 發布 v2.0.0（[GitHub](https://github.com/toitlang/toit/releases)） |

### 全球證據新增的結論

1. 出場的買家類型、價格與時間（第五節）。
2. 平台退場的教訓可以轉成產品功能，也就是「退場安全」（第三節）。
3. AI 對 Low Code 的影響：哪些貶值、哪些升值（第四節）。
4. 各區域的進入方式（第六節）。
5. 台灣分拆的法律路徑、稅務、員工、資金與先例（第七節）。
6. 新公司的股權結構最好避免中國資金，才能保住在美國市場的「非中國供應鏈」定位（推論，第六節）。
7. 分拆協議要保護 EDT：如果新公司日後被晶片商收購並改成只支援自家晶片，EDT 的模組客戶會受影響（推論，第五節）。
8. 原始碼與 know-how 必須以書面讓與或授權移轉，否則有營業秘密法的民刑事風險（第七節）。

## 二、市場：真的存在，但用對的尺量就很小

**大數字都在上游，真正付錢買工具的那一層很薄。** 連網裝置與 MCU 的數字很大，但「有螢幕的 MCU 連網裝置」沒有任何一手資料直接量過，嵌入式開發工具的市場規模也只有互相矛盾的市調。

| 指標 | 數字 | 來源與可信度 |
| --- | --- | --- |
| 連網 IoT 裝置 | 2025 年底 211 億台（+13%），2030 年約 390 億 | [IoT Analytics](https://iot-analytics.com/number-connected-iot-devices/) |
| 連線方式 | Wi-Fi 32%、藍牙 24%、蜂巢 22% | 同上 |
| 預測下修 | 2025 年比前一年的預測少約 3 億，原因是資本支出延後與中國需求疲弱 | 同上 |
| MCU 營收 | 2024 年 US$232 億；其中 IoT 用途 US$51 億（約 22%，推論計算） | [IoT Analytics](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/) |
| MCU 營收預測 | 2030 年 US$294 億（年增 3.9%）；Yole 則估 2030 年超過 US$340 億（年增 6%），Infineon、NXP、Renesas、ST 合計近七成 | IoT Analytics；[Yole（經 Edge AI and Vision Alliance）](https://www.edge-ai-vision.com/2025/12/microcontrollers-enter-a-new-growth-cycle-as-the-market-targets-us34-billion-in-2030/) |
| MCU 出貨量 | 2021 年約 280 億顆 | [Statista 引 Yole](https://statista.com/statistics/1327567/worldwide-microcontroller-unit-shipments-forecast)（二手來源） |
| Espressif | 2025 年營收約 CNY 25.65 億；2025 上半年 +35% | [Espressif 2025 年報](https://www.espressif.com/sites/default/files/financial/Espressif%20Systems%202025%20Annual%20Report.pdf) |
| Low Code（企業應用） | 2029 年 US$582 億，年增 14.1% | [Gartner 摘要](https://www.gartner.com/en/documents/7146430) |
| 嵌入式開發工具 | 2025 年 US$22.9 億至 76.5 億，相差 3.3 倍 | [Grand View Horizon](https://www.grandviewresearch.com/horizon/statistics/microcontroller-market/embedded-systems/software-embedded-development-tools/global)、[Wise Guy](https://www.wiseguyreports.com/reports/embedded-software-tool-market)（市調區間） |
| 嵌入式開發工具（歷史） | 2009 年「超過 US$2.9 億」，比 2008 年少約 10% | [Military & Aerospace Electronics 引 VDC](https://www.militaryaerospace.com/commercial-aerospace/article/14224846/slow-and-steady-growth-expected-for-embedded-software-development-tools-market)（舊資料） |
| HMI 軟體 | 2025 年約 US$58 億至 62 億 | [Marketintelo](https://marketintelo.com/report/hmi-software-market)、[TechSci](https://techsciresearch.com/report/human-machine-interface-hmi-software-market/25451.html)（市調區間；多半指工業 SCADA，不是 MCU GUI 工具） |
| 嵌入式開發者人數 | VDC 2010 年 88 萬；Ganssle 估 50 萬韌體開發者；Maxfield 估 200 至 400 萬「嵌入式專業人員」；SlashData 2023 年約佔全體開發者 5% | [Percepio 引 VDC](https://percepio.com/tag/max-maxfield/)、[Ganssle](https://www.ganssle.com/rants/wearenotthem.html)、[SlashData](https://slashdata.co/embedded-software-dei/embedded-developers-population-forecast)（舊資料與個人意見） |

三個判讀：

- **市調數字不能當成工具市場。** VDC 的「數億美元、持續承壓」和市調的「數十億美元」不可能描述同一件事。（推論）市調大概把 IDE、編譯器、除錯器、RTOS、測試與服務都算進去；純粹的 MCU IDE 與 GUI 工具只佔其中很小一部分。
- **Gartner 的 Low Code 成長屬於企業應用。** Mendix、OutSystems、Power Apps 那個市場的 14.1% 成長，不能拿來推估嵌入式 Low Code。
- **連網 MCU 正在搶份額。** 2025 上半年 IoT MCU 營收只成長 1.8%，Espressif 卻成長 35%。（推論）以 Wi-Fi 為核心、有螢幕型號的 MCU（ESP32-S3、P4）在平穩的市場中擴張，EHS 支援 ESP32 很重要。

**會經常用圖形化工具的工程師是少數。** JetBrains 2025 年的嵌入式調查中，經常做「圖形化系統設計」的只有 13%，「用高階語言建模」10%，「模擬」27%（[JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)，廠商調查，樣本含業餘者，27% 來自中國大陸）。Eclipse 2024 年調查（約 750 人）中，Linux 佔 46%、FreeRTOS 29%、Zephyr 21%（[Eclipse](https://newsroom.eclipse.org/node/41237)）。（推論）很多有螢幕的連網產品跑在 Linux 上，只做 MCU 的工具只覆蓋其中一部分。

**粗估付費規模（推論，純算術，未經驗證）。** 若專業嵌入式軟體開發者約 100 萬至 250 萬人，其中約 36% 至 49% 的設計有 GUI（36–37%（能力題）至 49%（直接詢問），2015–2017 年調查：[UBM 2015](https://webpages.charlotte.edu/~jmconrad/ECGR4101-2016-08/notes/UBM%20Tech%202015%20Presentation%20of%20Embedded%20Markets%20Study%20World%20Day1.pdf)、[AspenCore 2017](https://staff.emu.edu.tr/mehmetbodur/Documents/courses/CMPE423/Notes/2017-embedded-market-study.pdf)；這是受訪者的設計比例，不是台數），約 36 萬至 120 萬人。假設其中 1% 付費、每席每年 US$790 至 1,490（README 的建議價），一年席位營收約 US$280 萬至 1,790 萬。量產授權與服務可以再往上加，但量級和 froglogic（2020 年營收 €650 萬）相近，不是平台公司的量級。

### 垂直市場：充電樁的證據最強

| 垂直 | 數字 | 判讀 |
| --- | --- | --- |
| 公共充電樁 | 2024 年新增逾 130 萬個，年增逾 30%；歐洲存量增加逾 35% 到 100 萬出頭，美國增加 20% 到將近 20 萬（[IEA GEVO 2025](https://www.iea.org/reports/global-ev-outlook-2025/electric-vehicle-charging)）。2025 年新增近 180 萬個（年增逾 33%），年底全球存量超過 700 萬（[IEA GEVO 2026](https://www.iea.org/reports/global-ev-outlook-2026/electric-vehicle-charging-chap-6-and-10)，二手來源：頁面被機器人驗證擋住，第三輪仍無法讀取，數字來自 IEA 頁面的搜尋摘要） | 同時需要螢幕、OCPP 連線與遠端更新，最符合定位。市場以 OEM 設計案計，不以台數計（推論） |
| 智慧家庭 | 2024 年 8.923 億台（+0.6%），2025 年預測 9.311 億台（+4.4%）；IDC 2021 年曾預測 2025 年 14 億台，結果落空（[IDC](https://www.idc.com/resource-center/press-releases/smart-home-market-on-track-for-rebound-thanks-to-emerging-regions-according-to-idc/)） | 量大但成長慢；成長在 Wi-Fi 與 Matter 裝置（ABI 預估 2025 至 2030 年年增 16%，[ABI](https://www.abiresearch.com/news-resources/chart-data/forecast-wifi-device-shipments-by-connectiivty-technology)） |
| 工業顯示面板 | Omdia 2025 年 7 月的**預測**：2025 年 2.906 億片（+3.4%），其中「智慧家庭與辦公」佔 64%（[Communications Today](https://www.communicationstoday.co.in/?p=360711)）。Omdia 2026 年 1 月公布的**實際結果**：面板廠 2025 年營收 US$34 億（+24%），出貨量 −0.4%，主因歐洲淘汰拋棄式電子菸（[Display Daily](https://displaydaily.com/?p=385009)，二手來源：Omdia 原文網址第三輪已回傳 404，也不在 Omdia 2026 年 1 月的新聞列表，原文已下架） | 最接近「手機、電腦、汽車以外有螢幕的裝置」的量，但有一部分由 MPU 驅動；2025 年台數持平，營收成長主要來自遊戲掌機與運動相機面板 |
| 樓宇自動化 | 只有市調金額 | 沒有可用的台數 |
| 醫療、工控操作面板 | 沒找到資料 | 待查 |

## 三、IoT 平台退場潮：被砍的正是最像「IoT IDE」的那一層

**2019 至 2026 年，大廠留下最底層的連線服務，把應用開發、分析、規則引擎與開發工具一層層收掉。** 這些被收掉的層，正是一般人想像中「IoT 整合開發環境」的樣子。

| 廠商與產品 | 宣布 | 結束 | 官方理由 | 客戶處境 |
| --- | --- | --- | --- | --- |
| Samsung ARTIK Cloud | 2018-12 決定停產硬體 | 2019-05-15 | 停產 ARTIK 硬體 | 付費用戶不到一個月通知（[CNX Software](https://www.cnx-software.com/2019/03/14/samsung-artik-discontinued-iot-graveyard/)） |
| Google Android Things | 2020-12 | 2022-01-05 | 未說明 | 專案資料、建置設定與出廠映像永久刪除（[9to5Google](https://9to5google.com/2020/12/17/android-things-shutdown/)） |
| Blynk Legacy | 2022 | 2022-12-31 | 未說明 | 沒有自動遷移，已購點數作廢（[Blynk](https://blynk.io/legacy)） |
| Google Cloud IoT Core | 2022-08-16 | 2023-08-16（The Register 寫 08-15） | 「合作夥伴更能服務客戶」 | 沒有自家替代，裝置連線中斷（[RCR Wireless](https://www.rcrwireless.com/20220816/internet-of-things/google-cloud-to-shut-down-iot-core-service-next-year)） |
| IBM Watson IoT Platform | 2022-11 | 2023-12-01（TechRadar 寫 12-31） | 未說明 | 約 12 個月，沒有替代（[The Register](https://www.theregister.com/2022/11/15/ibm_set_to_retire_watson/)） |
| SAP IoT | — | 2023-01-31 | 併入 Asset Performance Management | 社群來源，不是正式公告（[SAP Community](https://groups.community.sap.com/t5/supply-chain-management-blogs-by-sap/sap-iot-retirement-and-sap-asset-performance-management/bc-p/13533348)） |
| Azure RTOS | 2023-11-21 | 改為 Eclipse ThreadX，MIT 授權 | — | 程式碼可以繼續用（[Eclipse](https://eclipse-foundation.blog/2023/11/21/introducing-eclipse-threadx/)） |
| Bosch IoT Device Management | 2022-10-20 | 2024 年中 | 改與大型雲端廠商合作 | 部分移到開源的 Eclipse Ditto（[Bosch](https://bosch-iot-suite.com/news/bosch-iot-device-management-boschio-to-support-device-management-together-with-major-cloud-providers/)） |
| AWS IoT Analytics | 2024-07 停收新客 | 2025-12-15 | — | 改用 Kinesis 與 S3（[AWS](https://docs.aws.amazon.com/iotanalytics/latest/userguide/iotanalytics-end-of-support.html)） |
| AWS IoT Events | 2025-05-20 停收新客 | 2026-05-20 | — | 一年後硬停（[AWS](https://docs.aws.amazon.com/iotevents/latest/developerguide/iotevents-end-of-support.html)） |
| Arm Mbed | 2024-07 | 2026-07 | — | 線上建置停止，社群分支 Mbed CE 延續（[Arm](https://os.mbed.com/blog/entry/Important-Update-on-Mbed/)） |
| AWS Greengrass V1 | 2020-12 推出 V2 | 2026-10-07，已停止服務 | — | 約五年過渡期，承諾 12 個月預告（[AWS 維護政策](https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html)、[AWS 遷移指南](https://docs.aws.amazon.com/greengrass/v2/developerguide/migrate-from-v1.html)） |
| PTC ThingWorx、Kepware | 2025-11-05 | 2026-03-16 賣給 TPG | 專注 CAD 與 PLM | US$6 億現金，加上最多 US$1.25 億的或有對價（TPG 日後轉售時才支付），最高 US$7.25 億；交割時實收 US$5.23 億（[SEC 8-K](https://www.sec.gov/Archives/edgar/data/857005/000119312525267120/ptc-20251105.htm)、[Digital Engineering 24/7](https://www.digitalengineering247.com/article/ptc-finalizes-divestiture-of-kepware-thingworx-businesses/internet-of-things)） |
| Azure Sphere | 2026-03-20 | 服務 2031-07-31；MT3620 晶片 2026-07-31 停產 | 安全 MCU 已普及 | 2031 年後要用的產品必須重新設計硬體（[Microsoft](https://techcommunity.microsoft.com/blog/iotblog/azure-sphere-is-retiring-in-2031---what-you-need-to-know/4504225)） |
| Azure IoT Central | 2026-09-25（2026-09-23 起已不能建立新應用） | 2029-09-20 | 集中投資 IoT 平台服務 | 三年預告與遷移手冊，建議改用 IoT Hub 加 DPS（[Microsoft IoT 部落格](https://techcommunity.microsoft.com/blog/iotblog/azure-iot-central-is-evolving-what-you-need-to-know/4554570)） |

**分析師的解釋一致。** IoT Analytics 統計平台公司在 2019 年達到約 620 家，2021 年首次「停業的比新成立的多」，高峰此後再也沒有回來。它的解釋是：通用平台「太廣、太僵、太抽象」，把整合留給客戶；舊廠房的整合太複雜；開發者偏好主流工具，不想用一體式平台；而大廠把 MQTT broker、規則引擎與數位孿生都變成商品（[IoT Analytics](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)）。McKinsey 2021 年指出 70% 的製造商無法走出試點階段（[McKinsey](https://www.mckinsey.com/capabilities/tech-and-ai/our-insights/iot-value-set-to-accelerate-through-2030-where-and-how-to-capture-it)）。IoT Analytics 也指出 GE 宣稱的 Predix 營收「大多是內部計費」。

**活下來的有共同點：錢綁在實體或站點上、可以自架、或背後有晶片商。**

| 存活者 | 怎麼收錢 | 為什麼活下來 |
| --- | --- | --- |
| Inductive Automation Ignition | 按伺服器授權，客戶端、tag、設計器數量不限 | 本機執行、價格好預測；私有公司 20 多年，140 多國有安裝（[公司自述](https://inductiveautomation.com/the-new-scada)） |
| ThingsBoard | 4.4 版改為 BUSL：商業免費 100 台，超過要授權；另收白標、代管雲、服務 | 可自架；每個版本四年後轉為 Apache 2.0（[ThingsBoard](https://thingsboard.io/blog/one-thingsboard-source-available/)）；2025 年約 110 人，營收不公開（[公司自述](https://thingsboard.io/company/)） |
| Home Assistant／Nabu Casa | 每月 US$5 的雲端訂閱加官方硬體，不拿投資人的錢 | 不訂閱家裡照樣能用；2026-10 把雲端服務改名為 Link，強調不會被綁住（[Home Assistant](https://home-assistant.io/blog/2026/10/02/big-tech-ruined-the-cloud-so-were-renaming-ours/)） |
| ESP RainMaker | 部署在客戶自己的 AWS 帳號，Espressif 收授權與服務 | 資料屬於客戶（[RainMaker 文件](https://docs.rainmaker.espressif.com/docs/deploy/deployment-overview/public-vs-private-rainmaker)） |
| Tuya | 模組綁 PaaS，加上 SaaS；2025 年營收 US$3.218 億，首次營業利益 US$1,150 萬 | 收入綁在模組出貨上（[Tuya](https://tuya.gcs-web.com/news-releases/news-release-details/tuya-reports-fourth-quarter-and-fiscal-2025-unaudited-financial)） |
| FlowFuse | Node-RED 開源，加上商業平台 | Open core；2022 年募 US$725 萬，2025 年再募約 US$720 萬（[FlowFuse](https://flowfuse.com/blog/2022/10/seed-round-bring-node-red-to-enterprise/)） |
| Cumulocity | 2025-01 管理層收購後獨立 | 270 多人（[Cumulocity](https://cumulocity.com/press-releases/cumulocity-completes-management-buyout-to-become-the-leading-independent-iot-platform)） |

**對 EHS 的意義：把「退場安全」做成產品功能（推論）。** 2022 年之後的買家都看過 Google、IBM、Samsung、Arm、Microsoft 收掉 IoT 工具，一定會問「你們倒了怎麼辦」。證據顯示，最貴的遷移是和韌體綁在一起的（Azure Sphere、Mbed、Android Things 都要重燒或重新設計硬體），只有開源（ThreadX、Ditto、Mbed CE）能讓客戶不靠廠商繼續用。EHS 可以先承諾：

| 承諾 | 依據 |
| --- | --- |
| 產生的 C 不用 IDE 也能以標準工具鏈建置（Zephyr、FreeRTOS、ESP-IDF） | 韌體綁定的遷移最貴 |
| 專案格式公開、文字化 | 開源與公開格式是唯一沒有斷頭的出路 |
| IDE 可離線在本機執行 | Mbed 線上建置、Android Things 主控台都隨服務消失 |
| 雲端只做連接器，或部署在客戶自己的帳號 | RainMaker、ThingsBoard、Home Assistant 的做法 |
| 書面停產政策，至少 12 個月預告 | AWS 已經做到，買家會拿來比 |
| 原始碼託管，或「若停止經營則開源」條款 | ThingsBoard 四年轉 Apache 2.0 的模式 |
| 已出貨產品的執行期函式庫永久授權 | 避免出貨後被收回授權 |

## 四、AI 寫程式：拖拉編輯器貶值，結構化模型升值

**AI 已經大量進入嵌入式開發，但信任度低，最好的結果都來自「AI 加上結構化的硬體知識與確定性工具」。**

| 調查 | 樣本 | 結果 | 標示 |
| --- | --- | --- | --- |
| [RunSafe 2025](https://www.businesswire.com/news/home/20251209931517/en/RunSafe-Security-Releases-2025-AI-in-Embedded-Systems-Report-Offering-New-Insight-Into-AI-Adoption-and-Security-Gaps) | 美、英、德 200 多位嵌入式專業人員 | 80.5% 在用 AI；83.5% 已把 AI 產生的程式碼送上產品；53% 最擔心資安 | 廠商調查（資安廠商） |
| [Black Duck 2025](https://www.blackduck.com/blog/embedded-software-survey-ai-adoption-code-quality.html) | 785 位開發與資安人員 | 89.3% 的組織在用 AI 寫程式；25% 禁用，但禁用的組織中有 75% 承認員工照用 | 廠商調查（資安廠商） |
| [Stack Overflow 2025](https://stackoverflow.blog/2025/07/29/developers-remain-willing-but-reluctant-to-use-ai-the-2025-developer-survey-results-are-here) | 全體開發者，沒有嵌入式拆分 | 84% 在用或計畫用；46% 不信任 AI 輸出；只有約 3% 高度信任；好感度從 72% 降到 60% | — |

學術研究說明了界線在哪裡：

- **AI 能讓新手做出東西。** 華盛頓大學的研究中，GPT-4 在 50 次試驗裡有 66% 做出可用的 I2C 介面；15 位使用者的實驗中，做出 LoRa 感測器的成功率從 25% 提高到 100%，包括沒有硬體或 C 經驗的人（[arXiv 2307.03817](https://arxiv.org/abs/2307.03817v2)）。
- **能編譯不代表能在真機上跑。** Duke 的 IoT-SkillsBench 用 Claude Sonnet 4.5，在 Arduino、ESP-IDF、Zephyr 三個平台做了 378 次真機驗證實驗，指出程式碼常因時序、周邊初始化而在真機上失敗。**由專家寫的精簡硬體 skills 讓成功率接近完美**：首次成功率（pass@1）沒有 skills 時 69.0%，LLM 自己寫的 skills 66.7%，專家寫的 skills 95.2%；五次內成功率（pass@5）分別是 80.2%、75.4%、98.4%。LLM 自寫的 skills 在 Zephyr 上反而從 57.1% 降到 45.2%（[arXiv 2603.19583](https://arxiv.org/html/2603.19583v1)，表 1、2；百分比由本研究依次數計算）。（推論）硬體知識要由懂硬體的人整理，不能交給 AI 自己寫。
- **認證與追溯是缺口。** 對 4 家公司 10 位資深工程師的訪談，列出 AI 產物在 IEC 61508、ISO 26262 認證上的缺口，以及非確定性破壞可重現性（[arXiv 2601.10220](https://arxiv.org/html/2601.10220v1)）。

**晶片商在 2025 至 2026 年走向同一個模式：提供知識、確定性產生器與動作，讓通用 AI agent 呼叫。**

| 時間 | 廠商 | 內容 |
| --- | --- | --- |
| 2025-02-19 | Microchip | MPLAB AI Coding Assistant，免費 VS Code 擴充（[GlobeNewswire](https://www.globenewswire.com/news-release/2025/02/19/3028656/0/en/artificial-intelligence-meets-embedded-development-with-microchip-s-mplab-ai-coding-assistant.html)） |
| 2026-04 | Espressif | ESP-IDF v6.0 內建本機 MCP 伺服器，提供 build、flash（[Espressif](https://developer.espressif.com/blog/2026/04/esp-idf-tools-mcp-server/)） |
| 2026-06 | Nordic | MCP 伺服器提供 nRF Connect SDK 與 Zephyr 文件；另有唯讀的 nRF Cloud 裝置群資料（[Nordic](https://www.nordicsemi.com/Products/Technologies/AI-assisted-development)） |
| 2026 | NXP | Agentic AI 開發，「實驗性測試版」：MCUXpresso for VS Code 提供 Copilot agent 與 MCUXpresso SDK、Zephyr 專案 skills；S32 Design Studio 有連到 IDE、編譯器、設定與除錯工具的 MCP gateway；另有 FreeMASTER 與文件的 MCP 伺服器（[NXP](https://www.nxp.com/design/design-center/software/agentic-ai-development:AGENTIC-AI)） |
| 2026 | Tuya | Tuya AI Coding、TuyaOpen IDE（[Tuya 新聞稿](https://persportaal.anp.nl/artikel/CSN-050826021/tuya-smart-launches-tuya-ai-coding-enabling-users-to-build-their-own-ai-powered-lifestyle-apps-with-natural-language)，只有廠商新聞稿） |
| 2026-11-04 | ST | STM32Cube for Agentic AI 測試版：skills、MCP、文件檢索；**agent 呼叫 CubeMX 做確定性產生**；宣稱少用 80% token（[ST](https://www.st.com/content/st_com/en/stm32-mcu-developer-zone/stm32-cube-for-agentic-ai.html)，ST 內部測試） |
| 持續 | LVGL | LVGL Pro 改為文字化 XML，CLI 可驗證、產生 C、在 CI 跑 UI 測試，明確主打適合 Git 與 AI（[LVGL](https://lvgl.io/editor)） |

**企業 Low Code 沒有死，而是轉向。** Gartner 2025 年 11 月仍預估 Low Code 到 2029 年達 US$582 億。Gartner 報告「Why AI Won't Replace the Need for Low-Code Application Platforms」（2025-10-07，Adrian Leow、Kyle Davis）的公開摘要說，AI 寫程式助手與 agent 讓一些人質疑 Low Code 平台的必要，報告說明為什麼這些平台仍然關鍵（[Gartner 摘要](https://www.gartner.com/en/documents/7033498)）。摘要裡沒有「vibe coding」字樣。常被引用的「vibe coding 加速打字，LCAP 加速工程」一句，以及「把 vibe coding 限於範圍明確、有開發者監督的用途」的建議，只見於 Jitterbit 與 Mendix 的轉載頁，兩頁現已 404（未驗證，只有廠商轉載），本文不據此做判斷。OutSystems 則幾乎全面改講 AI agent（[Diginomica](https://diginomica.com/outsystems-one-low-code-vendor-remodels-appeal-ai-agent)）。嵌入式領域沒有任何分析師的專門研究。

**什麼貶值、什麼升值（推論，依上述來源）：**

| 貶值 | 升值 |
| --- | --- |
| 手動擺放元件 | AI 讀得懂、能產生、可 diff 的專案模型（畫面、綁定、板級設定） |
| 樣板 C 碼產生 | 確定性、看得懂的程式碼產生器（CubeMX 模式） |
| 「不用懂 C」的訴求 | 整機模擬與 PC 預覽 |
| 給非程式背景者的 No Code | 在 CI 上跑的 UI 與行為測試 |
| — | 追溯與合規證據 |
| — | 經驗證的板級與周邊知識，以 skills 或 MCP 提供 |

**對 EHS 的意義（推論）：** 給工程師的產品應該是「AI 原生、模型驅動的 HMI／IoT 開發環境」：文字化專案格式；一個 MCP 伺服器，把專案模型、元件目錄、板級設定、建置、燒錄與預覽開放給 AI agent；確定性的 C 產生器與模擬器。這樣 AI 會變成需求來源，因為每一個 AI 產生的畫面都需要驗證、預覽與確定性的輸出。

**風險：** LVGL、ST、NXP、Tuya 走的是同一個方向，而且有生態優勢。EHS 的差異只能來自跨晶片（STM32、ESP32、Nordic／Zephyr）、模擬的真實度與合規證據。目前**沒有任何資料**顯示嵌入式團隊願意為 MCP 或 AI 層另外付費；Microchip 的工具免費，ST 的是免費測試版，NXP 的是實驗性測試版。AI 韌體新創 Embedder（YC S25）目前也提供免費版；它唯一可查的募資是 YC 標準的 US$50 萬（[YC deal](https://www.ycombinator.com/deal)、[VCBacked](https://www.vcbacked.co/company/embedder)），曾流傳的 US$6.25 億是資料彙整網站的錯誤。YC 公司頁目前只顯示 6 人、位於舊金山，沒有用戶、客戶或募資說法（[YC](https://ycombinator.com/companies/embedder)）。「5,000 多位工程師使用、100 多家付費客戶、CRV、BoxGroup、Emerson Collective 投資」只見於徵才網站鏡像保留的 YC 徵才文，原徵才頁已 404（[riseme.ai 鏡像](https://riseme.ai/jobs/embedder/dda9239e-aae2-4f11-9a47-29994bba7452/founding-ai-engineer)，未驗證，徵才文宣）。

## 五、出場路徑：買家是晶片商與工具整合商，價格在數千萬美元級

### 2016 至 2026 年的主要交易

| 年 | 買方 | 標的 | 價格 | 備註 |
| --- | --- | --- | --- | --- |
| 2016 | ThunderSoft（中國） | Rightware（Kanzi 車用 HMI） | 約 €6,400 萬 | 保留獨立經營；2025 年仍是 ThunderSoft 子公司（[Tech.eu](https://tech.eu/2016/12/19/rightware-thundersoft/)、[MarkLines 2025-05](https://www.marklines.com/cn/news/325959)；完成日期各來源不一）。ThunderSoft 2024 年報：100% 間接持有，Rightware 商譽 RMB 2.02 億，未減損（[ThunderSoft 2024 年報](https://pdf.dfcfw.com/pdf/H2_AN202504221660624764_1.pdf)） |
| 2018 | ST | Draupner Graphics（TouchGFX） | 未公開 | 理由是「加速 STM32 路線圖」；當時媒體質疑是否還支援其他晶片（[ST](https://www.st.com/content/st_com/en/about/media-center/press-item.html/c2854.html)、[eeNews Europe](https://www.eenewseurope.com/en/st-acquires-embedded-gui-specialist-2/)）。後來確實改為免費、限 ST 晶片（主要是 STM32）：附加授權條款（v4.27，含 SLA0048）規定只能用在「由 ST 或為 ST 製造」的裝置上，也禁止任何會讓它受開源條款約束的使用（[ST 授權條款](https://www.st.com/resource/en/additional_license_terms/additional-license-terms-x-cube-touchgfx-v4-27.html)）；函式庫執行時會檢查是否為 STM32（[ST 社群](https://community.st.com/stm32-mcus-touchgfx-and-gui-33/porting-touchgfx-code-to-non-st-mcus-129161)） |
| 2018 | Siemens | Mendix（Low Code） | €6 億 | Forrester 估至少 7 至 8 倍營收（[Forrester](https://www.forrester.com/blogs/siemens-snaps-up-mendix-low-code-platforms-enter-new-phase/)） |
| 2019 | Microsoft | Express Logic（ThreadX） | 未公開 | 2023 年捐給 Eclipse |
| 2021 | AMETEK | Crank Software（Storyboard） | 未單獨公開 | 三家公司合計約 US$2.7 億，合計年營收約 US$1.2 億（[MarketScreener](https://www.marketscreener.com/quote/stock/AMETEK-INC-11635/news/AMETEK-Inc-acquired-Magnetrol-International-Incorporated-and-Crank-Software-Inc-33461213/)） |
| 2021 | Qt Group | froglogic（Squish GUI 測試） | €3,000 萬，部分依業績 | 2020 年營收 €650 萬、EBIT €270 萬，約 4.6 倍營收（[Qt](https://www.qt.io/stock/qt-strengthens-its-market-leader-position-with-acquisition-of-froglogic-gmbh-1618320900000-2894368)） |
| 2021 | FSN Capital | TASKING（Altium 剝離） | US$1.1 億（US$1 億現金加 US$1,000 萬業績款） | Altium 稱「非核心」；2021-02 完成（[FSN Capital](https://www.fsncapital.com/en/news/fsn-capital-v-has-signed-an-agreement-to-acquire-100-of-tasking)、[Altium 公告（iConnect007 轉載）](https://iconnect007.com/index.php/article/125627/altium-divests-non-core-tasking-to-support-enable-investment-in-altium-365/125630)）。另一來源的 1.46 億很可能是澳幣金額（推論） |
| 2021 | ST | Cartesiam（NanoEdge AI） | 未公開 | 同樣被質疑是否綁 STM32 |
| 2022 | Aptiv | Wind River（TPG 出售） | US$43 億 | 2021 年營收約 US$4 億，約 10.8 倍（推論計算；[PE Hub](https://pehub.com/tpg-to-sell-wind-river-to-aptiv-for-4-3bn)） |
| 2024 | Espressif | M5Stack 多數股權 | 未公開 | Espressif 從 2018 年起是天使投資人（[Espressif](https://www.espressif.com/en/news/Espressif_Acquires_M5Stack)） |
| 2024 | Renesas | Altium | 約 US$59 億 | （[Renesas](https://www.renesas.com/en/about/newsroom/renesas-completes-acquisition-altium)） |
| 2025-03 | Qualcomm | Edge Impulse | 未公開 | 累計募資約 US$5,400 萬；承諾繼續支援其他硬體（[SiliconANGLE](https://siliconangle.com/2025/03/10/qualcomm-acquires-edge-ai-development-startup-edge-impulse/)） |
| 2025-06 | Nordic | Memfault | US$1.2 億，現金、無負債 | 2024 年底年經常性收入 US$720 萬，約 16.7 倍（推論計算）；2025-07-01 完成；累計募資約 US$3,500 萬（[Nordic 交易所公告](https://www.inderes.dk/releases/nod-nordic-semiconductor-acquires-memfault-launches-first-complete-chip-to-cloud-platform-for-lifecycle-management-of-connected-products)、[Nordic](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-acquires-Memfault)、[eeNews Europe](https://www.eenewseurope.com/en/nordic-semi-buys-memfault-for-120m)） |
| 2025-06 | Nordic | Neuton.AI 的 IP 與技術資產 | 未公開 | 品牌只在「初期整合階段」保留（[Nordic](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-accelerates-edge-AI-leadership-with-acquisition-of-Neuton-AI)） |
| 2025-10 | Qt Group | IAR Systems | 約 SEK 22.93 億（約 €2.04 億） | IAR 2024 年營收 SEK 4.872 億、EBITDA 35.8%；約 4.7 倍營收（推論，匯率估算）；Qt 說要「進入更大的 MCU 市場」（[Qt](https://www.qt.io/stock/inside-information-qts-recommended-public-cash-offer-for-iar-systems-group-1751607060000-3623512)、[IAR](https://www.iar.com/investors/reports-and-news/press-releases/iar-systems-group-ab-publ-interim-year-end-report-2024-690e5c85-6e0f-4a55-8ed5-2c300d831814)） |
| 2025-10 | Qualcomm | Arduino | 未公開 | 3,300 萬以上活躍用戶；2025-10-07 宣布，2025-11-05 的財報會議前已完成交割；FY2025 10-K（至 2025-09-28）沒有提到 Arduino，Q1 FY26 10-Q 只說該季（自 2025-09-29 起）另買了四家未具名公司，合計 US$8.46 億，Arduino 是否在內是推論（[Qualcomm FY2025 10-K](https://s204.q4cdn.com/645488518/files/doc_financials/2025/q4/QCOM-09-28-25-FY2025-10-K-Final.pdf)、[Q1 FY26 10-Q（StockTitan 鏡像）](https://www.stocktitan.net/sec-filings/QCOM/10-q-qualcomm-inc-de-quarterly-earnings-report-cebab8b158aa.html)）；11 月因服務條款修改引發社群反彈（[Qualcomm](https://www.qualcomm.com/news/releases/2025/10/qualcomm-to-acquire-arduino-accelerating-developers--access-to-i)、[Qualcomm Q4 FY25 財報會議](https://s204.q4cdn.com/645488518/files/doc_financials/2025/q4/Q4FY25-Earnings-Call-Transcript_11-6-25_Final.pdf)、[The Register](https://www.theregister.com/2025/11/21/adafruit_makers_unhappy_with_arduino)） |
| 2025-10 | NXP | Kinara | US$3.07 億 | （[NXP](https://www.nxp.com/company/about-nxp/newsroom/NW-NXP-COMPLETES-ACQUISITIONS-AVIVA-LINKS-KINARA)） |
| 2026-01 | Digi | Particle | US$5,000 萬 | 年經常性收入約 US$2,000 萬；光 2019 年 C 輪就募了 US$4,000 萬，出場價接近或低於累計募資（累計金額未逐輪查證） |
| 2026-02 | SUSE | Losant | 未公開 | （[SUSE](https://www.suse.com/news/suse-acquires-losant/)） |
| 2026-02 | TI | Silicon Labs（同意收購） | 約 US$75 億 | 預計 2027 上半年完成（[Silicon Labs](https://news.silabs.com/2026-02-04-Texas-Instruments-to-acquire-Silicon-Labs)） |
| 2026-03 | Canonical | Golioth | 未公開 | 2026-03-03 宣布；累計募資 US$710 萬（pre-seed US$250 萬加 seed US$460 萬，公司自述）（[Golioth](https://blog.golioth.io/golioth-is-now-a-part-of-canonical)、[Golioth 種子輪](https://blog.golioth.io/may-2023-golioth-seed-round/)） |

**四種買家。** 一是晶片商，用工具帶晶片（ST、Nordic、Qualcomm、NXP、Espressif、Renesas、Infineon）；二是工業集團（AMETEK、Siemens）；三是工具整合商（Qt Group 先買 froglogic 再買 IAR；ThunderSoft）；四是作業系統與平台公司（Microsoft、Canonical、SUSE）。（推論）對 LVGL 系的 HMI 工具來說，最可能的買家是第一類（ST 已有 TouchGFX；NXP 有 LVGL 系的 GUI Guider；Renesas、Infineon、Espressif、Nordic、Microchip 都是候選）與第三類的 Qt Group。

**被晶片商買走的工具常變成只支援自家晶片。** TouchGFX 與 NanoEdge 被收購時都被質疑會綁 STM32，TouchGFX 後來確實改為免費但限 ST 晶片（主要是 STM32）；Imagimob 被 Infineon 收購後改名 DEEPCRAFT，整合進 ModusToolbox（[Shawn Hymel](https://shawnhymel.com/3125/state-of-edge-ai-on-microcontrollers-in-2026/)，二手來源）。買下大型社群的買家會公開承諾中立（Edge Impulse、Arduino），但 Arduino 在幾週內就因服務條款引發反彈。Memfault 被收購後是「Nordic 客戶享優惠價」的軟綁定。

**分拆先例：Qt 是成功的模板，條件很明確。** Nokia 轉向 Windows Phone 後，Digia 在 2012 年買下整個 Qt 業務，2016 年把 Qt 分拆成 Qt Group 在赫爾辛基上市（[Digia](https://www.digia.com/en/investors/demerger)）。Qt Group 2025 年營收 €2.163 億，EBITA 率 24%，自己也成了收購者（[Qt Group 2025 Q4](https://storage.mfn.se/da194bf9-6209-480c-b461-84fedd21e1cf/qt-group-plc-2025-q4-en.pdf)）。（推論）成功的條件是：有不依賴母公司硬體的商業授權客戶、跨平台中立、母公司真的願意放手。反面例子是當作生態補貼的工具：Mbed 停產、Azure RTOS 捐出。另外，LVGL 原廠在 2024 年初與 SquareLine 分手，改做自己的 LVGL Pro（[LVGL](https://lvgl.io/blog/announcement-squareline-breakup)），說明平台擁有者可以隨時收回合作。

**募資規模與結局。**

| 公司 | 募資 | 結局 |
| --- | --- | --- |
| Memfault | B 輪 US$2,400 萬，累計約 US$3,500 萬（[Tracxn](https://tracxn.com/d/companies/memfault/__Po211QCPDkRaPzfw2x3GD0B2PWSTJsnkWXtpDka8IlM/funding-and-investors)） | 2025 年賣給 Nordic |
| Edge Impulse | A 輪 US$1,500 萬；B 輪 US$3,400 萬，估值 US$2.34 億（[EE Asia](https://www.eetasia.com/edge-impulse-raises-34m-triples-valuation/)） | 2025 年賣給 Qualcomm |
| Blues | 種子 US$1,100 萬、A 輪 US$2,200 萬、A1 輪 US$3,200 萬（Qualcomm 參與）（[GeekWire](https://www.geekwire.com/2021/ray-ozzies-telecom-startup-blues-wireless-raises-22m-bill-gates-others/)） | 仍獨立 |
| Golioth | pre-seed US$250 萬、seed US$460 萬，合計 US$710 萬（[Golioth](https://blog.golioth.io/may-2023-golioth-seed-round/)） | 2026 年賣給 Canonical |
| FlowFuse | US$725 萬、US$720 萬 | 仍獨立 |
| Slint | 「六位數」種子，其餘靠 NLnet 補助（[Slint](https://slint.dev/blog/slint-funding-and-hiring)） | 仍獨立，規模小 |
| LVGL Kft | 沒找到股權募資；創辦人曾說「還不確定商業模式」（[LVGL 論壇](https://forum.lvgl.io/t/business-model-for-lvgls-editor/14939)）；2025-12-08 另在英國設立 LVGL LTD 負責商業營運，營收仍不公開（[UK Companies House](https://find-and-update.company-information.service.gov.uk/company/16897003/officers)、[LVGL](https://lvgl.io/about)） | 仍獨立 |

（推論）三個規律：

1. **VC 要的是雲端或 SaaS 成分。** 拿到美國種子輪 US$400 萬至 1,100 萬的，都有裝置雲或可觀測性服務（Memfault、Golioth、Blues、FlowFuse）。單純的桌面 GUI 編輯器公司（LVGL Kft、Slint、SquareLine）都很小，靠自籌或補助。
2. **出場時間約 5 至 7 年，幾乎都是賣給晶片商。** Golioth 2020 年成立、2026 年賣出；Cartesiam 2016 年成立、2021 年賣出。2015 至 2026 年沒有找到嵌入式工具公司 IPO，除了 Qt 2016 年的分拆上市。
3. **價格區間。** 小型工具與測試公司約 4 至 5 倍營收（froglogic、IAR）；Low Code 平台或稀缺資產 7 至 11 倍（Mendix、Wind River）；VC 支持的 IoT 工具，出場價從低於累計募資（Particle）到約 3.4 倍（Memfault）不等。以營收計，Memfault 約 16.7 倍年經常性收入，是第五節已揭露交易中最高的倍數；但它是預期 2025 年成長逾 50% 的雲端服務，不能套用在授權型工具上。若新公司做到年營收 €500 萬，以 4 至 5 倍計約 €2,000 萬至 2,500 萬。

**對 EDT 的提醒（推論，需律師確認）：** 若新公司日後被某家晶片商收購並改成只支援自家晶片，EDT 搭配其他晶片的模組客戶會受影響。分拆協議應包含 EDT 的永久 OEM 授權，以及控制權變更時授權繼續有效的條款。

## 六、區域市場：中國免費、日本走代理、歐美靠法規

| 區域 | 證據 | 進入方式（推論） |
| --- | --- | --- |
| 中國 | ZLG 的 AWTK 以 LGPL 開源，官方說商業使用免費（[AWTK FAQ](https://awtk.zlg.cn/docs/awtk_docs/FAQ/1.AWTK.html)）；ZLG 人員在論壇說設計器「不靠它賺錢，只靠它吸引客戶」（[WhyCan 論壇](https://whycan.com/t_5688.html)，非正式說法）；RT-Thread Studio 社群版「永久免費」，RT-Thread 宣稱跑在 25 億台以上裝置（[Embedded Computing Design](https://embeddedcomputing.com/technology/software-and-os/ides-application-programming/rt-thread-studio-ide-now-available-for-open-source-iot-rtos)、[鳳凰網](https://news.ifeng.com/c/8jCGykOulxm)）；塗鴉與阿里雲按台收啟用碼，阿里雲每台 RMB 1.60 至 5.00（[阿里雲](https://help.aliyun.com/zh/document_detail/150384.html)） | 不在中國賣授權。把中國晶片與開發板（ESP32、CYD 板）當作接觸全球長尾客戶的通路 |
| 日本 | SEGGER emWin 由 EmbITeK（エンビテック）代理（[SEGGER 代理商](https://www.segger.com/purchase/distributors)），在 Renesas RX、RA 上量產免費（[Renesas](https://renesas.com/ja/document/prb/segger-emwin)）；SEGGER 的全球牌價是單一產品 €2,980 至 11,980（[SEGGER](https://www.segger.com/pricelist-emwin.html)）。Crank Storyboard 由大阪エヌデーエス代理（[Crank 2022 新聞稿](https://cranksoftware.com/press-release-crank-expands-support-in-japan-with-osakands)），在 IPROS 標價帶 100 萬至 500 萬日圓，沒有授權細節，這是唯一查到的日圓價格（[IPROS（Storyboard）](https://mono.ipros.com/product/detail/2000808564)）；Embedded Wizard 由テガラ代理，依目標 MCU、年產量、色彩格式按專案報價（[IPROS](https://mono.ipros.com/product/detail/2000859227)），原廠結帳頁另列 Dioiz 為日韓代理商（[Embedded Wizard](https://www.embedded-wizard.de/pricing)）；Qt 也只接受詢價。本土廠商有 HI Corporation 的 exbeans UI Conductor，用在 Epson 印表機（[MONOist](https://monoist.itmedia.co.jp/mn/articles/1611/02/news027.html)），以及 CRI Middleware 的車用 HMI 工具 CRI Glassco，已量產用於儀表（[CRI](https://www.cri-mw.co.jp/business/product/automobile/criglassco/)）；CRI 的 Mobility 事業營收在 2026 年 9 月期從 6.33 億日圓增至 9.32 億日圓，其中約 3 億來自 Glassco 被印度機車採用（目前經日本車廠出口）（[logmi 法說會紀錄](https://finance.logmi.jp/articles/385976)） | 找日本代理商，加入 Renesas 生態；日文文件；認證週期長；準備依 MCU 與產量報價的執行期授權條款 |
| 韓國 | 本土 HMI 龍頭 M2I（엠투아이）做工控面板 HMI，50% 至 60% 的 HMI 營收來自三星、SK 海力士的晶圓廠設備（[韓國經濟](https://www.hankyung.com/article/2025120785641)），屬於 Weintek、Pro-face 這一類，不是 MCU GUI 工具。Qt 是大廠標準：現代汽車集團 2021 年選 Qt 為 HMI 技術夥伴（[Qt](https://qt.io/press/hyundai-motor-group-selects-qt-as-their-key-hmi-technology-partner)，二手來源：內文未顯示，日期來自搜尋摘要），LG 的 webOS 用 Qt 開發（[Qt](https://qt.io/lg-electronics-built-with-qt)）。ST 韓國直接推廣免費的 TouchGFX，用在家電、門鎖、鍋爐、穿戴與醫療裝置（[Digital Daily](https://www.ddaily.co.kr/page/view/2023112814485539624)）。GUI 工具的韓國代理商：Embedded Wizard 經 Dioiz（[Embedded Wizard](https://www.embedded-wizard.de/pricing)），SEGGER 經 In Semiconductor（[SEGGER 代理商](https://www.segger.com/purchase/distributors)），Qt 經 OCUBE（[Qt 夥伴名錄](https://qt.io/contact-us/partners)，韓文搜尋沒有獨立佐證）；沒有找到韓元價格 | （推論）Linux 等級的大廠 HMI 已由 Qt 佔據；MCU 等級走晶片商生態（ST、Renesas、NXP、Espressif）與嵌入式工具代理商，鎖定家電與工控的二線供應商 |
| 東南亞 | 越南 2026 上半年新登記的製造業外資，近 66%（逾 US$70.3 億）是電腦、電子與光學產品（[VOV 引 Savills](https://english.vov.vn/en/economy/northern-vietnam-attracts-805-of-new-manufacturing-fdi-in-first-half-post1330684.vov)）；越南 FPT 自稱支援 160 多個數位座艙、HMI 與車聯網專案（[TechNode Global](https://technode.global/2026/01/02/fpt-collaborates-with-telechips-to-introduce-ai-powered-intelligent-cockpit-platform/)，二手來源）；馬來西亞檳城：congatec 2026-01-27 設研發子公司，從 23 位嵌入式工程師開始，目標約 70 人（[MIDA](https://www.mida.gov.my/media-release/congatec-expands-penang-rd-presence-anchoring-high-value-embedded-computing-capabilities-in-malaysia/)）；泰國 BOI 2025 年電子電機投資申請 THB 2,776 億、PCB 相關 THB 2,492 億（[Nation Thailand](https://www.nationthailand.com/business/investment/40061707)）；菲律賓 2025 年電子出口 US$496.4 億（+16.1%），以封測與 EMS 為主（[SEIPI](https://www.seipi.org.ph/?p=7660)，二手來源）；台灣的文曄科技 2022 年完成收購新加坡通路商 Excelpoint（[MarketScreener](https://ca.marketscreener.com/quote/stock/EXCELPOINT-TECHNOLOGY-LTD-6811928/news/WT-Semiconductor-Pte-Ltd-completed-the-acquisition-of-Excelpoint-Technology-Ltd--41845347/)，二手來源）。GUI 工具的東南亞代理商：SEGGER 經新加坡的 Gopalam Embedded Systems 與 Testech Electronics 涵蓋新加坡、泰國、越南、馬來西亞、菲律賓、印尼（[SEGGER 代理商](https://www.segger.com/purchase/distributors)）；Qt 經新加坡 TECHKINETICS（[Qt 夥伴名錄](https://qt.io/contact-us/partners)）；沒有找到當地幣別價格 | （推論）東南亞主要是製造基地，GUI 工具多半仍由品牌商決定採購。可接觸的是 FPT 這類設計服務公司，以及文曄／Excelpoint 這類台資通路，把工具和 MCU、面板一起賣 |
| 歐盟 | CRA 2024-12-10 生效；2026-09-11 起須通報被利用的弱點與嚴重事件，不溯及既往；違反通報義務最高罰 €1,500 萬或全球營收 2.5%；2027-12-11 全面適用，含機器可讀 SBOM（[歐盟執委會](https://digital-strategy.ec.europa.eu/en/policies/cra-reporting)、[CMS](https://cms.law/en/bel/legal-updates/ready-respond-report-the-cyber-resilience-act-s-reporting-regime-is-here)、[Bootlin](https://bootlin.com/blog/cyber-resilience-act-cra-obligations-for-manufacturers/)）；Microchip 已發布給客戶的 CRA 指引。RED 資安要求（授權規則 2022/30）自 2025-08-01 適用；EN 18031-1、-2、-3 於 2025-01-30 刊登公報（實施決定 2025/138），但附限制：允許使用者不設密碼的設計，以及 EN 18031-3 的安全更新評估條款，都不能推定符合（[EUR-Lex 2025/138](https://eur-lex.europa.eu/eli/dec_impl/2025/138/oj/eng)）。2026 年沒有找到修改 2025/138 的決定；最新的 RED 標準修正 (EU) 2026/2003（2026-09-04）沒有涉及 EN 18031（[EUR-Lex 2026/2003](https://eur-lex.europa.eu/eli/dec_impl/2026/2003/oj)），所以限制推定仍有效（推論，依未找到修正）。授權規則 2026/339 規定 2022/30 自 2027-12-11 廢止，由 CRA 接手（[EUR-Lex 2026/339](https://eur-lex.europa.eu/eli/reg_del/2026/339/oj/eng)） | SBOM、弱點追蹤與簽章 OTA 做成預設功能；義務落在客戶（製造商）身上，所以客戶有預算。現在到 2027-12 先賣符合 EN 18031 的強制密碼設定與安全更新範本，之後延伸到 CRA；距 2027-12 約 12 個月的銷售窗口 |
| 美國 | Cyber Trust Mark：原主管機構 UL 因與中國企業的合資關係在 2025-12 退出，FCC 在 2026-04-13 改選 ioXt；2026-10-27 才徵詢標籤實作意見，產品尚未上架（[Nextgov](https://www.nextgov.com/cybersecurity/2026/04/fcc-selects-ioxt-alliance-lead-cyber-labeling-program/412800)、[Cooley](https://www.cooley.com/news/insight/2026/2026-11-10-fcc-advances-us-cyber-trust-mark-program)）。EO 14306（2025-06）保留了聯邦採購要求：FAR 須讓機關在 2027-01-04 前要求消費性 IoT 供應商貼標（[Federal Register](https://www.govinfo.gov/content/pkg/FR-2025-06-11/html/2025-10804.htm)）；但 FCC DA 26-834（2026-08-11）說明，標籤管理機構尚未獲授權核准使用標籤（[FCC](https://docs.fcc.gov/public/attachments/DA-26-834A1.pdf)）；國防部的 Open FAR Cases 報告（截至 2026-11-06）沒有任何實施這項聯邦採購要求的 FAR 案件（[Open FAR Cases](https://www.acq.osd.mil/dpap/dars/opencases/farcasenum/far.pdf)）；BIS 車聯網規則：與中、俄相關的車聯與自駕軟體從 2027 年式起禁止，只限乘用車（[BIS](https://www.bis.gov/press-release/commerce-finalizes-rule-secure-connected-vehicle-supply-chains-foreign-adversary-threats)）；FDA §524B 要求「cyber device」送件附 SBOM，2025-06-27 公布最終指引，並要求寫進供應商合約（[Federal Register](https://www.govinfo.gov/content/pkg/FR-2025-06-27/html/2025-11669.htm)、[King & Spalding](https://www.kslaw.com/insights/articles/fda-finalizes-premarket-cybersecurity-guidance-for-medical-devices)） | 台灣註冊、沒有中國股權的工具商，可以把「工具與執行期的來源可稽核」當賣點 |

補充判讀：

- **中國同時是機會與波動來源。** IoT Analytics 預估中國是到 2030 年成長最快的 IoT MCU 市場，但 2025 年的預測下修也是因為中國需求疲弱。
- **西方調查低估亞洲。** AspenCore 2017 年的嵌入式調查有 56% 來自北美，亞洲只有 10.6%（[CNX Software](https://cnx-software.com/2017/08/15/aspencore-2017-embedded-markets-study-programming-languages-operating-systems-mcu-vendors-and-more)）。
- **股權結構會影響美國市場（推論）。** FDA 要求供應商合約涵蓋 SBOM，BIS 規則看軟體與中、俄的關聯，Cyber Trust Mark 換掉了和中國有關的主管機構。新公司若引進中國資金，可能失去這個定位。BIS 規則針對車聯網與自駕軟體，不是一般的儀表板 HMI 工具，所以這是訊號，不是直接的強制要求。
- **歐盟 RED 資安要求已查證**（第二輪）：2025-08-01 起適用，EN 18031 附限制，2027-12-11 由 CRA 接手（見上表）。第三輪沒有找到 2026 年解除限制的決定（推論：限制仍有效）。（推論）IDE 現在內建符合 EN 18031 的認證與更新範本，2027 年後可以直接沿用到 CRA。
- **Cyber Trust Mark 時程對不上（推論）。** 行政命令仍要求聯邦機關在 2027-01-04 前要求貼標，但到 2026-11 還沒有任何產品能貼標，Open FAR Cases 報告（截至 2026-11-06）也沒有實施這項要求的 FAR 案件。照目前狀況，2027-01-04 的聯邦採購要求無法如期生效。這是 2027 至 2028 年的中期訊號，不是馬上會帶來採購的理由。
- **Rightware 與 BIS 規則（推論，需律師確認）。** Rightware 仍是 ThunderSoft 100% 持有的子公司；ThunderSoft 2024 年報中，Rightware 商譽 RMB 2.02 億沒有減損，歐美佔營收 19.18%，年報沒有提到美國車聯網規則（[ThunderSoft 2024 年報](https://pdf.dfcfw.com/pdf/H2_AN202504221660624764_1.pdf)）。Kanzi 若跑在具連網功能的座艙單元上，可能落入 BIS 規則的「covered software」，從 2027 年式起不能用在美國市場車輛（[Federal Register](https://www.govinfo.gov/content/pkg/FR-2025-01-16/html/2025-00592.htm)）。這是本研究對條文的解讀；車廠、律師事務所與產業媒體都沒有公開點名 Kanzi 或 ThunderSoft。
- **亞洲代理商與價格（第三輪）。** Embedded Wizard 在台灣與中國經 Simplify、在韓國與日本經 Dioiz，原廠標價每席每月 €199（[Embedded Wizard](https://www.embedded-wizard.de/pricing)）；SEGGER 在台灣經 Microtime（[SEGGER 代理商](https://www.segger.com/purchase/distributors)）；LVGL Pro 由原廠以美元直接銷售，沒有列出經銷商（[LVGL Pro 價格](https://lvgl.io/pro/pricing)）。（推論）亞洲的嵌入式工具代理商名單已經存在，新公司可以先找這些代理商談，不必自建通路；Simplify、Microtime 這類台灣代理商是最近的起點。

## 七、台灣分拆機制：三條法律路徑、稅務、員工與資金

> 本節是研究材料，不是法律或稅務意見。所有法條適用，須由台灣律師與會計師確認。

### 三條法律路徑

| 路徑 | 怎麼做 | 好處 | 代價 |
| --- | --- | --- | --- |
| **1. 新設分割**（[企業併購法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0080041)） | EDT 把 EHS 業務分割到一家 100% 持股的新公司，再讓團隊與投資人入股 | 員工年資必須承認（第 16 條）；合約、IP、人員整包移轉；第 39 條：對價中有表決權股份達 65% 以上時，免印花稅、契稅、證券交易稅，貨物或勞務移轉不課營業稅，母子型分割也適用（[財政部 2017 年令，經理律](https://www.leeandli.com/TW/Newsletters/5960.htm)）；第 44 條：有表決權股份達 80% 以上，**並且**把取得的股份全數轉給股東時，所得才免稅，EDT 自己保留新公司股份時不適用（[企併法第 44 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080041&flno=44)）。不過財政部 2007 年函釋（台財稅字第 09604545320 號）規定分割給 100% 子公司時以帳面價值計算（[理律](https://www.leeandli.com/TW/Newsletters/3679.htm)）。臺北高等行政法院 113 年度訴字第 948 號判決（2024-12-19）認為函釋合法可用，而且「不因採何種分割模式有別」，維持新公司以帳面價值 NT$3.37 億、而非原始成本 NT$5.46 億作為稅務成本（[判決全文](https://judgment.judicial.gov.tw/FILES/TPBA/113%2c%e8%a8%b4%2c948%2c20241219%2c1.pdf)）。自行研發的 IP 帳面價值低，分割所得可能接近零；但新公司承接這個低稅務成本，日後出售 IP 或事業時要就全部增值課稅（推論，需會計師確認） | EHS 必須是「得獨立營運之營業」（第 4 條第 6 款）；原則上要開股東會，三分之二股份出席、過半同意（第 35 條）；債權人至少 30 天異議期；新公司在出資範圍內對分割前債務負連帶責任，2 年後消滅 |
| **2. 技術作價入股**（[產業創新條例](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0040051)第 12 條之 1） | 成立新公司，EDT 以 EHS 的 IP 讓與或授權作價，取得新股 | 取得的股票可延緩到實際轉讓時才課稅，第 2 項明定「個人、公司或有限合夥事業」都可選擇，所以 EDT 也適用（[產創第 12 條之 1](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0040051&flno=12-1)）；適用到 2029-12-31（第 72 條）；結構簡單，適合小型軟體單位 | 員工須離職再受雇，年資不會自動延續（推論）；技術抵充的數額要經新公司董事會決議（[公司法第 156 條第 5 項](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080001&flno=156)；發起設立時見第 131 條第 3 項），公開發行新股只能收現金（第 272 條）；法律沒有明定鑑價，但會計師查核資本額時須取得專家的鑑定價格意見書（[會計師查核簽證公司登記資本額辦法第 7 條](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0080021)）；第 3 項的優惠計稅基礎只適用個人，公司作價不適用（[KPMG](https://kpmg.com/tw/zh/home/insights/2019/06/e-tax-alert-125.html)） |
| **3. 獨家授權** | EDT 保留 IP，給新公司全球獨家授權，第 12 條之 1 第 2 項也涵蓋授權 | EDT 保有控制權 | 投資人通常要求新公司擁有核心 IP；日後海外架構或併購都需要移轉（推論） |

其他要點：

- **簡化程序。** 分割到既有公司且價值不超過淨值 20% 時，可只經董事會決議（第 36 條）；母公司與持股 90% 以上的子公司之間，可只經董事會決議（第 37 條）。第 36 條第 3 項另有被分割公司為新設公司唯一股東的規定，適用條件要由律師確認。
- **員工。** 新公司須在基準日前 30 天書面通知留用條件，員工 10 天內不回覆視為同意（第 16 條）。不留用或不同意的員工，由原雇主依勞基法第 16 條預告終止並給資遣費（第 17 條）。退休準備金先支付不留用員工，餘額移轉新公司（第 15 條）。
- **競業禁止。** [勞基法第 9 條之 1](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=N0030001&flno=9-1) 要求四個條件同時成立（雇主有正當利益、員工接觸營業秘密、期間與範圍合理、雇主給合理補償），缺一無效，最長 2 年。每月補償不低於離職時一個月平均工資的 50%，須一次預付或按月給付（[施行細則第 7 條之 3](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=N0030002&flno=7-3)）；約定必須書面（[第 7 條之 1](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=N0030002&flno=7-1)）。（推論）分拆協議應明訂，轉到新公司的 EDT 員工不受 EDT 競業條款限制。
- **不延緩課稅的代價。** 技術作價的所得在取得股票當年課稅，個人最高 40%，會出現「股票沒變現卻要繳現金稅」的問題（[EY](https://www.ey.com/zh_tw/insights/tax/pcs-technology-contribution-strategies-for-pre-ipo-startups)）。
- **兩個稅務問題已有條文答案（第二輪查證）。** 第 44 條第 3 項要求「將取得之股份全數轉予股東」，所以 EDT 保留新公司股份時不適用；實際路徑是財政部函釋的帳面價值計算（見上表），2024 年的行政法院判決仍照此適用（第三輪查證）。帳面價值是雙面刃：分割當下所得低，新公司日後出售時稅負高（推論）。公司（EDT）以第 12 條之 1 第 2 項作價可以延緩課稅；第 3、4 項只適用個人。第 12 條之 1 最後一次修正是 2019-07-24，2026-09-30 的修法只改第 19 條之 1、第 23 條之 1、第 23 條之 2 與第 72 條（[產創沿革](https://law.moj.gov.tw/LawClass/LawHistory.aspx?pcode=J0040051)）。執行前仍要會計師確認。
- **程式碼與 know-how 的歸屬。** 員工職務上開發的營業秘密歸雇主（[營業秘密法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0080028)第 3 條），程式的著作財產權原則上也歸雇主（[著作權法第 11 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0070017&flno=11)）。團隊沒有 EDT 的書面讓與或授權就把原始碼帶進新公司，可能構成侵害（第 10 條），並有刑責：5 年以下徒刑，得併科 NT$100 萬至 1,000 萬元罰金（第 13 條之 1）；意圖在境外使用的刑度更重（第 13 條之 2）。（推論）防護做法：分割計畫書或 IP 讓與契約列明移轉清單（repo、分支、文件、商標、網域、客戶資料）；EDT 保留的顯示模組客戶資料與硬體 know-how，新公司不能使用，反之亦然；依智慧局手冊做營業秘密盤點、分級與存取紀錄（[智慧局《營業秘密保護實務教戰手冊 3.0》](https://www.tipo.gov.tw/wSite/public/Attachment/0/f1742977090931.pdf)）。

### 資金來源

| 資源 | 內容 |
| --- | --- |
| 國發基金創業天使投資方案 | 2025-11 從 NT$50 億擴大到 NT$100 億（[聯合報](https://udn.com/news/story/7238/9115341)）。單一公司的**初次投資上限仍是 NT$2,000 萬**；有累計募資或管理資產達 US$10 億以上的機構承諾投資時可提高到 NT$3,000 萬；NT$1 億是含後續增資的累計上限。受理到 2030-12-31；公司設立未逾 8 年、實收資本額或累計募資不超過 NT$1 億（[國發基金方案](https://www.df.gov.tw/cp.aspx?n=D96B18C3BC73D5B5&s=B97E7588060C2084)）。新創的股東及其具實質控制權的公司不得擔任搭配投資的天使，所以 EDT 不能當搭配方（[方案申請須知](https://www.angelinvestment.org.tw/download)）。到 2025 年底累計投資 299 家、NT$43.05 億 |
| 產創第 23 條之 2 | 個人以現金投資成立未滿 8 年的高風險新創，單一公司單年 NT$50 萬以上、持有 3 年，可抵減 50% 綜合所得；每年上限 NT$500 萬（非國家重點產業 NT$300 萬）；適用到 2029-12-31 |
| 產創第 19 條之 1 | 員工獎酬股票延緩課稅，每年 NT$1,000 萬，有 2 年服務條件；2026-09-30 修正，施行期間 2027-01-01 至 2029-12-31（[產創沿革](https://law.moj.gov.tw/LawClass/LawHistory.aspx?pcode=J0040051)） |
| 2025 年台灣新創投資 | US$27.74 億、555 件，集中在健康、生技、能源、硬體、工業製造（[中小及新創企業署](https://wwwcdn.sme.gov.tw/article-tw-2276-15174)） |
| 輪次中位數 | 種子輪 US$101 萬、早期（A、B）US$331 萬、後期 US$808 萬（[FINDIT 2026 年報](https://findit.sme.gov.tw/tw/Res/2693)） |
| SBIR | 115 年度上限：Phase 1 為 NT$150 萬／6 個月，Phase 2 為 NT$600 萬／1 年或 NT$1,200 萬／2 年，Phase 2+ 為 NT$600 萬；補助不超過總經費 50%，且補助款 ≦ 自籌款 ≦ 實收資本額；**有陸資投資者不得申請**；新公司與 EDT 是關係人，不宜一起組聯盟（[115 年 SBIR 申請須知](https://sbir.sme.gov.tw/download/downloadarea/115%E5%B9%B4SBIR%E7%94%B3%E8%AB%8B%E9%A0%88%E7%9F%A5ctYsBK.pdf)） |
| A+ 企業創新研發淬鍊 | 前瞻計畫補助不超過總經費 50%，公司淨值須為正，**不得為陸資**，已有產品或已開發的技術不符合（[前瞻計畫申請須知](https://service.moea.gov.tw/EE514/wSite/public/Attachment/00106/f1786506767729.pdf)）。（推論）較適合 EDT 或已有營收的新公司，初期小公司較適合 SBIR |
| AppWorks Accelerator | 不收股份與費用，計畫 3 個月；AW#34 於 2026-11-29 與 2026-12-27 截止；沒有說明企業分拆的公司能否申請（[AppWorks FAQ](https://appworks.tw/accelerator-faq/)） |
| 青年創業貸款（信保基金保證） | 原「青年創業及啟動金貸款要點」已改名為「青年創業貸款要點」（[要點全文含沿革](https://0800280280.sme.gov.tw/upload/1150701青年創業貸款要點%28含沿革%29.pdf)）。經濟部 2026-06-29 公布：2026-07-01 起的新增貸款，每人前 NT$150 萬由信保基金 9.5 成保證，原為 NT$100 萬（[經濟部新聞稿](https://www.moea.gov.tw/MNS/populace/news/News.aspx?kind=1&menu_id=40&news_id=123170)）。主要條件：設立未滿 8 年的中小企業；負責人 18 至 45 歲，個人持有實收資本額 20% 以上，3 年內修過 20 小時或 2 學分的創業課程；營運週轉金最高 NT$600 萬，資本性支出最高為計畫經費 80%、上限 NT$1,200 萬；利率為中華郵政二年期定儲機動利率加 0.575%；利息補貼最多以 NT$100 萬貸款額計算、最長 5 年（保證額度提高了，補貼基數沒變），預算用完即止（同上要點）。（推論）EDT 持有多數股份時，要有一位 18 至 45 歲、個人持股 20% 以上的創辦人擔任負責人，新公司才能申請 |
| TTA（Taiwan Tech Arena） | 南部基地（台南沙崙）公布入駐條件：公司設立未滿 8 年並願意以實證推動商品化，或是經政府創業計畫或競賽選出的新團隊；領域為智慧科技、精準健康、淨零與循環；新創進駐 2 年、可延 1 年；共同工作空間每月 NT$2,000（開放座位）或 NT$2,500（攤位），含稅；可在基地辦公司登記（[TTA 南部基地申請說明](https://apply2ttas.taiwanarena.tech/)）。台北基地沒有公布入駐條件，空間價格要洽詢（[TTA](https://www.taiwanarena.tech/about/)）。（推論）新公司屬智慧科技、設立未滿 8 年，符合南部基地的形式條件 |

### 台灣先例

| 先例 | 內容 |
| --- | --- |
| 友達 → 達擎 | 2020-05 董事會通過，把一般與公用顯示事業分割到 100% 子公司，2021-01-01 生效；這是友達第一次分割事業單位（[TechNews](https://finance.technews.tw/2020/06/17/auo-passed-the-daqing-division-base-day-january-1-2021/)） |
| 友達 → AUO Mobility Solutions | 2024-11 董事會通過，基準日 2026-01-01（[自由時報](https://ec.ltn.com.tw/article/breakingnews/4871165)） |
| 友達 → 達耀能源 | 2026-04 董事會通過，基準日約 2026-08-01（[Newtalk](https://newtalk.tw/news/view/2026-04-13/1029575)） |
| 宏碁 | 2017 年分割智聯服務並上市；2023-11 已有 11 家上市子公司（[數位時代](https://www.bnext.com.tw/article/60715/acer-group-esg-policy-ast)、[鉅亨網](https://news.cnyes.com/news/id/4534161)） |
| 研華 → 雲研物聯（反例） | 2018-01 研華與資策會各出資 50% 成立，提供以 WISE-PaaS 為基礎的工業設備聯網雲服務（[數位時代](https://www.bnext.com.tw/article/47729/advantech-and-iii-set-up-a-new-joint-venture-company-focus-on-iot)）。2019 年因「短期獲利不易」改以 spin-in 讓 10 位同仁回到研華，公司現列非營運中（[FINDIT](https://findit.sme.gov.tw/tw/Startup/cExMcG9EQ2lyMGViYmpOUFRKYWc1QT09/Basic)）。沒有找到 WISE-PaaS 平台本身分拆的來源 |
| Gogolook（Whoscall） | 第一天就決定走出台灣；泰國月活躍用戶超過台灣；雇用東南亞在地主管補足人脈；之後上市（[TechNews](https://technews.tw/?p=1086155)） |
| Appier | 在東京證券交易所上市（[中央社](https://www.cna.com.tw/news/afe/202501310025.aspx)） |

（推論）四點判讀：

1. **路徑選擇。** 如果要把團隊、合約和 IP 一起帶走，新設分割最乾淨，而且 EDT 的董事會與會計師熟悉這個做法（友達、宏碁都用過）。如果新公司初期只有少數人和 IP，技術作價入股比較簡單。
2. **先例的差別。** 友達、宏碁分出去的都是硬體或服務單位，賣給既有客戶。最接近 EHS 的台灣先例是研華的雲研物聯：工控大廠把 IoT 平台軟體放進獨立公司，不到兩年就收回。這支持本文的判斷：新公司要有不依賴母公司硬體的付費客戶與獲利路徑，否則很容易被收回母公司。開發工具公司需要全球的自助購買與開發者通路（英文文件、社群、線上付款），Gogolook 與 Appier 的經驗顯示要雇在地主管，甚至海外上市。
3. **募資與時程。** 台灣資金偏硬體與生技，開發工具公司要找外國或策略投資人（晶片商、通路商）；晶片商常先少數投資再收購（Qualcomm 投資 Particle 與 Blues、Espressif 先投資 M5Stack 再取得多數股權）。產創第 12 條之 1 與第 23 條之 2 都在 2029-12-31 到期，若要分拆，應在此之前完成。美國種子輪 US$400 萬至 1,100 萬的規模，和台灣中位數 US$101 萬差很多。國發天使不能由 EDT 當搭配方，SBIR 與 A+ 排除陸資，所以新公司一開始就要找外部天使或創投，而且不要引進中國資金。
4. **US flip 的組成要件。** 美國種子輪與 A 輪創投常要求外國公司改由 Delaware 母公司持有（[Wilson Sonsini](https://www.wsgr.com/a/web/26603/revisiting-the-delaware-flip.pdf)）。在台灣要處理：外國母公司取得台灣公司股份，須依[外國人投資條例](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0040002)第 8 條事前向經濟部投資審議司申請核准；EDT 作為公司對外投資，逾 NT$15 億須事前核准，其餘在實行後 6 個月內申報（[公司國外投資處理辦法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0040052)），個人不在此辦法內；個人以未上市股票換外國母公司股票的所得計入基本所得額（[所得基本稅額條例第 12 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=G0340115&flno=12)）；母公司設在 Cayman 這類低稅負地區時，可能觸發 CFC（[臺北國稅局](https://www.ntbt.gov.tw/singlehtml/41ae3594197f4f69b47753ce08188516?cntId=b5a5fb32200c4151b8a77f363ce4d16b)）。法律上，企併法第 31 條第 3 項准許台灣公司與外國公司進行股份轉換，並準用第 21 條（外國公司所屬國法律允許、依該國法律核准、在台灣指定代理人），所以台灣公司可以用換股成為 Delaware 公司的 100% 子公司（[企業併購法](https://law.moj.gov.tw/LawClass/LawAll.aspx?PCODE=J0080041)、[第 21 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080041&flno=21)）。2025 至 2026 年的修法草案讓股份轉換的證券所得延緩計入基本所得額，但只限國發會認定的控股公司，關係企業不適用（[KPMG](https://kpmg.com/tw/zh/media/press-releases/2025/12/draft-amendments-insights-from-kpmg1.html)）；三讀原排在 2026 年 8 月（[聯合報](https://udn.com/news/story/6656/9705721)），結果未確認，法規資料庫仍顯示最後修正是 2022-06-15。（推論）這個優惠針對國內產業控股公司，不是為了 flip 到外國母公司。flip 的步驟與典型費用，沒有找到台灣的可信來源。

## 八、對 EDT 經營層的決策含意

| 決策 | 全球證據指向 | 標示 |
| --- | --- | --- |
| 要不要分拆 | 只有在新公司能取得不依賴 EDT 硬體的付費客戶時才划算。先完成 10 至 15 家客戶訪談，再決定 | 推論 |
| 分拆形式 | 要帶走團隊與合約，用新設分割；只帶 IP 與少數人，用技術作價入股。EDT 保留新公司股份時，企併法第 44 條的所得免稅不適用，要靠帳面價值計算（2024 年判決仍適用）與第 39 條的其他稅負優惠；帳面價值低使分割所得低，但新公司日後出售 IP 或事業時要就全部增值課稅，出場規劃要先算進去 | 推論，需律師與會計師 |
| IP 移轉 | 分割計畫書或 IP 讓與契約列明移轉清單；沒有書面讓與就帶走原始碼有營業秘密法刑責風險 | 推論，需律師 |
| EDT 的保障 | 永久 OEM 授權；控制權變更時授權繼續有效；EDT 保留股份 | 推論，需律師 |
| 財務目標 | 以「小而賺錢」或「5 至 7 年後策略收購」為基準 | 推論 |
| 產品定位 | 有螢幕的連網裝置；AI 可呼叫的模型、產生器與模擬器；退場安全承諾 | 推論 |
| 優先市場 | 歐盟（RED 與 CRA）、美國（非中國供應鏈）、日本（代理商）；韓國、東南亞走晶片商生態與台資通路；中國只當硬體通路 | 推論 |
| 股權 | 避免中國資金（SBIR、A+ 也排除陸資）；考慮晶片商或通路商的策略投資；國發天使的搭配方要另找外部天使，EDT 不能擔任 | 推論 |

客戶訪談建議再加三個問題（15 號已加入 CRA 與 AI 的問題）：

- 工具廠商若被某家晶片商收購，你會不會停用？你要求哪些退場保障？
- 你會不會為「讓 AI agent 能正確產生與驗證韌體」的工具層付費？
- 你們的產品跑 MCU 還是 Linux？有螢幕的產品中，各佔多少？

## 第二輪查證結果

2026-11-20 針對第一輪的待驗證項目做第二輪查證，查證紀錄在 [notes/16-global-evidence](notes/16-global-evidence/) 的四份 verification 筆記。

| 項目 | 結果 | 一句話 |
| --- | --- | --- |
| Omdia 工業顯示面板 | 更正 | 2.906 億片（+3.4%）與 64% 是 2025 年 7 月的預測；2026 年 1 月的實際結果是營收 US$34 億（+24%），出貨量 −0.4% |
| AspenCore／UBM 的 GUI 比例 | 更正 | 改為 36–37%（能力題）至 49%（直接詢問），2015–2017 年調查；粗估席位營收上限從 US$1,190 萬調為 1,790 萬，結論的「數百萬到一千多萬美元」不變 |
| IEA GEVO 2026 充電樁 | 確認 | 2025 年新增近 180 萬個（+33%），年底存量逾 700 萬；仍是二手來源（IEA 頁面無法直接讀取） |
| IoT-SkillsBench | 確認 | pass@1：無 skills 69.0%、LLM 自寫 66.7%、專家 95.2%；pass@5：80.2%、75.4%、98.4%；Claude Sonnet 4.5；Zephyr 上 LLM 自寫 skills 只有 45.2% |
| Gartner Low Code 與 AI 報告 | 確認（書目）／仍未驗證（引句） | 報告是「Why AI Won't Replace the Need for LCAPs」，2025-10-07，Leow、Davis；「vibe coding 加速打字」一句仍只見廠商轉載；第三輪確認公開摘要沒有此句 |
| Embedder 募資 | 更正 | US$50 萬是 YC 標準條件；US$6.25 億是資料彙整網站的錯誤；用戶與客戶數仍未驗證 |
| NXP 的 AI 工具 | 新增 | MCUXpresso for VS Code 有實驗性測試版的 Copilot agent 與 SDK、Zephyr skills；S32 Design Studio、FreeMASTER、文件都有 MCP 伺服器 |
| Azure IoT Central | 確認 | 2029-09-20 退役，2026-09-23 起不能建立新應用；改引 Microsoft IoT 部落格（2026-09-25） |
| Greengrass V1 | 確認 | AWS 文件寫 2026-10-07 停止支援，已停止；產品頁的 06-01 是過時資料，刪除 |
| PTC → TPG 價格 | 更正 | US$6 億現金加最多 US$1.25 億或有對價，最高 US$7.25 億（不是約 7.3 億）；交割實收 US$5.23 億（SEC 8-K） |
| Memfault 價格 | 確認 | US$1.2 億（Nordic 交易所公告）；年經常性收入 US$720 萬，約 16.7 倍；刪除無來源的 1.38 億 |
| TASKING 價格 | 確認 | US$1.1 億（1 億加 1,000 萬業績款）；1.46 億很可能是澳幣（推論） |
| Golioth | 更正（募資）／確認（交易） | 募資合計 US$710 萬（pre-seed 250 萬加 seed 460 萬）；2026-03-03 被 Canonical 收購，條件未公開 |
| Qualcomm–Arduino 交割 | 確認 | 在 2025-10-07 與 2025-11-05（Q4 FY25 財報會議）之間完成 |
| TouchGFX 授權 | 確認 | 免費、只限 STM32（SLA0048，函式庫執行時檢查），經 ST 社群確認；第三輪讀到條款原文，更正為限 ST 晶片（主要是 STM32） |
| Toitware | 更正 | 2024-08-20 破產、2025-10-28 解散（第三方 CVR 鏡像，二手來源）；Toit 語言仍在維護，v2.0.0 於 2026-10-31 發布 |
| ThingsBoard、LVGL 規模 | 新增（部分） | ThingsBoard 2025 年約 110 人；LVGL LTD 於 2025-12-08 在英國設立；營收仍不明 |
| Rightware | 確認（持股）／仍未驗證（客戶疑慮） | 仍是 ThunderSoft 子公司；沒找到客戶疑慮；BIS 車聯網規則是否適用是推論，需律師確認 |
| 韓國、東南亞、日本 | 新增／更正引用 | 新增韓國（M2I、Qt 為現代與 LG 標準、ST 韓國推 TouchGFX）、東南亞（越南外資、檳城 congatec、泰國 BOI、菲律賓出口、文曄–Excelpoint）、日本（按專案報價、emWin €2,980–11,980、CRI Glassco）；原 IPROS 連結其實是 Tegara 的 Embedded Wizard，改引 Crank 2022 新聞稿 |
| 歐盟 RED | 確認＋新增 | EN 18031-1/-2/-3 於 2025-01-30 刊登公報（2025/138）並附限制；2026/339 規定 2022/30 自 2027-12-11 廢止，由 CRA 接手 |
| 美國 Cyber Trust Mark | 確認＋新增 | EO 14306 保留 2027-01-04 的聯邦採購要求；ioXt 自 2026-04-13 為主管機構；FCC DA 26-834 說標籤管理機構尚未獲授權；沒找到 FAR 規則 |
| 企併法第 44、39 條 | 更正 | 第 44 條要求有表決權股份達 80% **且**股份全數轉給股東，EDT 保留股份時不適用；帳面價值函釋需會計師確認；第 39 條 65% 確認，含母子型分割。**結論第 6 點已改寫** |
| 產創第 12 條之 1 | 確認 | 第 2 項涵蓋公司，EDT 可緩課；第 3 項只適用個人；到 2029-12-31；2026-09-30 修法確認，但沒有修第 12 條之 1 |
| 公司法技術出資 | 確認＋新增 | 第 156 條第 5 項要董事會決議；第 131 條第 3 項適用發起設立；第 272 條公開發行只收現金；會計師須取得專家鑑價意見 |
| 勞基法施行細則第 7 條之 3 | 確認 | 補償不低於月平均工資 50%；第 7 條之 1 要求書面 |
| 營業秘密法、著作權法 | 新增 | 程式碼與 know-how 原則上歸 EDT，移轉要書面；新增第七節段落與第八節「IP 移轉」一列 |
| 國發基金天使方案 | 更正 | 初次投資上限 NT$2,000 萬（有大型機構時 3,000 萬），NT$1 億是累計上限；到 2030-12-31；8 年、資本額 NT$1 億以下；EDT 不能當搭配天使。**結論第 6 點已改寫** |
| SBIR、A+、AppWorks、青創貸款 | 新增（部分） | SBIR 與 A+ 都排除陸資；AppWorks 不收股份；青創貸款與 TTA 細節仍未取得 |
| US flip | 新增（部分） | 外國人投資條例第 8 條事前核准、公司對外投資申報、換股所得計入基本所得額、CFC；成本不明 |
| 研華 | 新增 | 雲研物聯（與資策會合資，2018-01）2019 年因短期獲利不易收回研華；支持「分拆需要獨立付費客戶」的判斷，結論第 7 點加入此例 |

**對核心結論的影響：** 沒有推翻任何核心結論。結論第 6 點的兩個事實（第 44 條所得免稅、國發天使 NT$1 億上限）有錯，已改寫；改寫後「台灣有成熟的分拆路徑」仍成立，但 EDT 保留股份時的稅務處理與找外部天使變成要先處理的事。

## 第三輪查證結果

2026-11-20 針對第二輪仍未驗證的項目做第三輪查證：一份用桌面版內建瀏覽器唯讀開啟一手頁面，一份用網路搜尋查申報文件、在地語言與代理商頁面。查證紀錄在 [notes/16-global-evidence](notes/16-global-evidence/) 的 `verification_round3_primary_pages.md` 與 `verification_round3_search.md`。

| 項目 | 結果 | 一句話 |
| --- | --- | --- |
| Gartner 7033498 摘要 | 確認（書目）／更正（引句） | 公開摘要確認標題、2025-10-07、Leow 與 Davis；摘要沒有「vibe coding」字樣，引句與相關建議只見於已 404 的廠商頁，正文改標未驗證、不據此判斷 |
| TouchGFX 授權 | 更正 | 附加授權條款 v4.27 限用於「由 ST 或為 ST 製造」的裝置，並禁止讓它受開源條款約束的使用；改寫為「限 ST 晶片（主要是 STM32）」 |
| Cyber Trust Mark 的 FAR 案件 | 確認 | Open FAR Cases 報告（截至 2026-11-06）沒有實施這項聯邦採購要求的案件；照目前狀況 2027-01-04 無法如期生效（推論） |
| 青年創業貸款 | 確認＋新增 | 經濟部 2026-06-29 新聞稿與要點全文確認前 NT$150 萬 9.5 成保證；新增額度、利率、利息補貼與負責人資格；改引官方來源，刪除新聞與大學轉載來源 |
| TTA 入駐條件 | 新增 | 南部基地公布條件（設立未滿 8 年或政府計畫選出；智慧科技、精準健康、淨零；2+1 年；每月 NT$2,000–2,500）；台北基地沒有公布條件 |
| IEA GEVO 2026 | 仍未驗證 | 仍被機器人驗證擋住，維持二手來源 |
| Omdia 2026-01 新聞稿 | 仍未驗證 | 原文網址已 404，也不在 Omdia 新聞列表；維持 Display Daily 為來源 |
| Toitware 官方 CVR | 仍未驗證 | 仍被機器人驗證擋住，維持第三方鏡像 |
| Qualcomm–Arduino | 新增／仍未驗證（交割日） | FY2025 10-K 沒提 Arduino；Q1 FY26 10-Q 列出該季四家未具名收購，合計 US$8.46 億，Arduino 在內是推論；交割日仍在 2025-10-07 至 11-05 之間 |
| EN 18031 限制 | 仍未驗證 | 2026 年沒有修改 2025/138 的決定，最新 RED 修正 2026/2003（2026-09-04）不涉及 EN 18031；推定限制仍有效（推論） |
| 財政部 09604545320 號函釋 | 確認＋新增 | 臺北高等行政法院 113 年度訴字第 948 號（2024-12-19）照此適用、不分分割模式，維持帳面價值 NT$3.37 億的稅務成本；新增「新公司承接低稅務成本」的推論。**結論第 6 點、第七節、第八節已補充** |
| 韓國、東南亞、台灣代理商 | 新增 | Embedded Wizard 經 Dioiz（韓、日）與 Simplify（台、中），每席每月 €199；SEGGER 經 In Semiconductor、Microtime、Gopalam、Testech；Qt 經 OCUBE、TECHKINETICS；LVGL Pro 原廠直銷 |
| SEGGER 日本代理商 | 更正 | 正式名稱是 EmbITeK（エンビテック） |
| 日本日圓價格 | 新增（部分） | 大阪エヌデーエス在 IPROS 列 Storyboard 100 萬至 500 萬日圓，其餘只接受詢價；CRI Mobility 營收 6.33 億增至 9.32 億日圓，約 3 億來自 Glassco 用於印度機車 |
| Embedder | 仍未驗證 | YC 頁顯示 6 人、沒有用戶或募資說法；5,000 多位工程師、100 多家付費客戶與投資人名單只見於徵才文鏡像，標為徵才文宣 |
| Rightware | 新增／無公開資料（客戶疑慮） | ThunderSoft 2024 年報：100% 持有、商譽 RMB 2.02 億未減損、歐美佔營收 19.18%、沒提 BIS 規則；沒找到第三方疑慮 |
| US flip | 新增（部分） | 企併法第 31 條第 3 項準用第 21 條，可與外國公司換股；換股緩課草案只限國發會認定的控股公司，2026 年 8 月三讀結果未確認；台灣沒有步驟或費用來源 |
| 開發者人數、VS Code 使用率、有螢幕的比例 | 無公開資料 | 最後一輪搜尋沒有新資料 |

**對核心結論的影響：** 沒有推翻任何核心結論。結論第 6 點補上帳面價值的另一面：分割時所得低，但新公司日後出售時稅負高，出場規劃要先算進去。TouchGFX 的描述改得更精確，不影響「晶片商把工具做成只支援自家晶片」的判斷。Cyber Trust Mark 沒有 FAR 案件，加強「中期訊號，不是馬上帶來採購」的判斷。Gartner 的 vibe coding 引句已不作為依據，結論第 3 點原本就不依賴它。

## 待驗證

### (a) 仍可再查

- IEA GEVO 2026、Toitware 官方 CVR：兩者都被機器人驗證擋住；人工用一般瀏覽器開啟即可確認。
- Omdia 2025 年工業顯示實績：原新聞稿已下架，只剩 Display Daily 轉述；可等 Omdia 重新發布或找其他轉述。
- Qualcomm–Arduino 確切交割日：只能確定在 2025-10-07 至 11-05 之間；10-Q 的四家未具名收購是否包含 Arduino 未確認。
- EN 18031 限制：沒有找到 2026 年的修正，但沒有讀到 2025-12 之後的 EUR-Lex 合併版本。
- Gartner「vibe coding 加速打字」引句：只有讀付費正文才能確認；本文已不據此判斷。
- Embedder 的用戶、客戶與投資人：只見於徵才文宣，等公司或投資人正式公布。
- Rightware：ThunderSoft 2025 年報全文沒有找到；BIS 規則是否適用要律師判斷。
- 醫療、工控操作面板、樓宇控制器的台數：只找到市調估工控操作面板每年約 140 萬台、ABI 估 2030 年智慧溫控器 6,720 萬台，都沒有區分有沒有螢幕。
- 韓國、東南亞的當地幣別價格，以及日本其他工具的日圓價格：都只接受詢價；可透過第六節列出的代理商詢價。
- TTA 台北基地的入駐條件：沒有公布，要直接洽詢。
- 財政部帳面價值函釋：2024 年判決沒有查到上訴結果；對 EHS 的實際稅負仍要會計師計算。
- 企併法換股緩課修正是否已三讀，以及 US flip 的步驟與費用：可請律師確認。

### (b) 結構上無公開資料（不再重查）

- 私有公司營收：Inductive Automation、ThingsBoard、LVGL 都不公開財報；英國 LVGL LTD 的首份財報約 2027 年才到期。
- VDC 付費牆後的資料：包括 600 人的嵌入式 AI 調查採用率與工具市場規模。
- Eclipse 2025 年 IoT 與嵌入式調查：要註冊登入才能下載，是開發者 IDE、作業系統分布最可能的來源。使用者可以自行註冊下載。
- AI 或 MCP 層的付費意願：Microchip、NXP、ST、Nordic、Espressif、Embedder 的工具全部免費或測試版；只能靠客戶訪談取得。
- 有螢幕的 MCU 或 IoT 裝置佔比：分析師報告與市調都沒有台數比例；Qt Group 2025 年調查是 420 位嵌入式 UI／UX 設計師的樣本，不是比例（[Qt Group](https://www.qt.io/hubfs/Designing-For-Embedded_Qt-Group-Market-Research.pdf?hsLang=en)，廠商調查）。
- 嵌入式 GUI 工具市場的一手規模：VDC、Omdia、IDC 都沒有數字；最接近的只有 MRFR 估 2024 年約 US$2.4 億（[MRFR](https://marketresearchfuture.com/reports/embedded-graphical-user-interface-gui-development-software-market/market-size)，市調區間，方法未驗證）。

## 來源

### 市場規模與需求

- IoT Analytics：[連網裝置數](https://iot-analytics.com/number-connected-iot-devices/)、[IoT MCU 市場](https://iot-analytics.com/iot-mcu-market-7-billion-opportunity-by-2030-driven-by-industrial-edge-ai/)、[嵌入式系統市場（2026-09）](https://iot-analytics.com/embedded-systems-market-opportunities-and-challenges-for-developers/)
- [Yole（經 Edge AI and Vision Alliance）](https://www.edge-ai-vision.com/2025/12/microcontrollers-enter-a-new-growth-cycle-as-the-market-targets-us34-billion-in-2030/)、[Statista 引 Yole](https://statista.com/statistics/1327567/worldwide-microcontroller-unit-shipments-forecast)、[TechInsights](https://techinsights.com/blog/mcclean-report-february-2025-microcontroller-market-update)
- [Espressif 2025 年報](https://www.espressif.com/sites/default/files/financial/Espressif%20Systems%202025%20Annual%20Report.pdf)
- [Gartner Low Code 預測（2025-11）](https://www.gartner.com/en/documents/7146430)
- 市調區間：[Grand View Horizon](https://www.grandviewresearch.com/horizon/statistics/microcontroller-market/embedded-systems/software-embedded-development-tools/global)、[Wise Guy](https://www.wiseguyreports.com/reports/embedded-software-tool-market)、[Marketintelo HMI](https://marketintelo.com/report/hmi-software-market)、[TechSci HMI](https://techsciresearch.com/report/human-machine-interface-hmi-software-market/25451.html)
- VDC：[Military & Aerospace Electronics](https://www.militaryaerospace.com/commercial-aerospace/article/14224846/slow-and-steady-growth-expected-for-embedded-software-development-tools-market)、[VDC 部落格](https://www.vdcresearch.com/News-events/iot-blog/2012/software-development-tools-market-update.html)
- 開發者人數：[SlashData](https://slashdata.co/embedded-software-dei/embedded-developers-population-forecast)、[Percepio 引 VDC 與 Maxfield](https://percepio.com/tag/max-maxfield/)、[Ganssle](https://www.ganssle.com/rants/wearenotthem.html)、[UBM 2015](https://webpages.charlotte.edu/~jmconrad/ECGR4101-2016-08/notes/UBM%20Tech%202015%20Presentation%20of%20Embedded%20Markets%20Study%20World%20Day1.pdf)、[AspenCore 2017](https://staff.emu.edu.tr/mehmetbodur/Documents/courses/CMPE423/Notes/2017-embedded-market-study.pdf)
- 垂直市場：[IEA GEVO 2025](https://www.iea.org/reports/global-ev-outlook-2025/electric-vehicle-charging)、[IEA GEVO 2026](https://www.iea.org/reports/global-ev-outlook-2026/electric-vehicle-charging-chap-6-and-10)、[IDC 智慧家庭](https://www.idc.com/resource-center/press-releases/smart-home-market-on-track-for-rebound-thanks-to-emerging-regions-according-to-idc/)、[ABI Research](https://www.abiresearch.com/news-resources/chart-data/forecast-wifi-device-shipments-by-connectiivty-technology)、[Omdia 工業顯示（原文已 404，第三輪）](https://omdia.tech.informa.com/pr/2026/jan/industrial-display-revenues-grow-24percent-yoy-in-2025-led-by-auo-display-plus)、[Display Daily（Omdia 2025 實績）](https://displaydaily.com/?p=385009)、[Communications Today（Omdia 2025-07 預測）](https://www.communicationstoday.co.in/?p=360711)
- 第二、三輪查證（待驗證項目用）：[MRFR 嵌入式 GUI 開發軟體](https://marketresearchfuture.com/reports/embedded-graphical-user-interface-gui-development-software-market/market-size)、[Qt Group 嵌入式設計調查 2025](https://www.qt.io/hubfs/Designing-For-Embedded_Qt-Group-Market-Research.pdf?hsLang=en)

### IoT 平台退場與存活者

- [IoT Analytics：What happened to IoT platforms](https://iot-analytics.com/what-happened-to-iot-platforms-whats-next/)、[IoT 平台公司版圖 2021/2022](https://iot-analytics.com/iot-platform-companies-landscape/)
- [McKinsey 2021](https://www.mckinsey.com/capabilities/tech-and-ai/our-insights/iot-value-set-to-accelerate-through-2030-where-and-how-to-capture-it)
- Google：[RCR Wireless](https://www.rcrwireless.com/20220816/internet-of-things/google-cloud-to-shut-down-iot-core-service-next-year)、[TechCrunch](https://techcrunch.com/2022/08/17/google-cloud-will-shutter-its-iot-core-service-next-year/)、[9to5Google（Android Things）](https://9to5google.com/2020/12/17/android-things-shutdown/)
- IBM：[The Register](https://www.theregister.com/2022/11/15/ibm_set_to_retire_watson/)
- AWS：[IoT Analytics 終止](https://docs.aws.amazon.com/iotanalytics/latest/userguide/iotanalytics-end-of-support.html)、[IoT Events 終止](https://docs.aws.amazon.com/iotevents/latest/developerguide/iotevents-end-of-support.html)、[Greengrass V1 維護政策](https://docs.aws.amazon.com/greengrass/v1/developerguide/maintenance-policy.html)、[Greengrass V1 遷移指南](https://docs.aws.amazon.com/greengrass/v2/developerguide/migrate-from-v1.html)
- Microsoft：[Azure Sphere 退役](https://techcommunity.microsoft.com/blog/iotblog/azure-sphere-is-retiring-in-2031---what-you-need-to-know/4504225)、[IoT Central 退役（Microsoft IoT 部落格，2026-09-25）](https://techcommunity.microsoft.com/blog/iotblog/azure-iot-central-is-evolving-what-you-need-to-know/4554570)、[Eclipse ThreadX](https://eclipse-foundation.blog/2023/11/21/introducing-eclipse-threadx/)
- [Arm Mbed](https://os.mbed.com/blog/entry/Important-Update-on-Mbed/)、[SAP Community](https://groups.community.sap.com/t5/supply-chain-management-blogs-by-sap/sap-iot-retirement-and-sap-asset-performance-management/bc-p/13533348)、[Bosch IoT Suite](https://bosch-iot-suite.com/news/bosch-iot-device-management-boschio-to-support-device-management-together-with-major-cloud-providers/)、[PTC](https://www.ptc.com/en/news/2025/tpg-to-acquire-ptcs-industrial-connectivity-and-iot-businesses)、[PTC SEC 8-K（2025-11-05）](https://www.sec.gov/Archives/edgar/data/857005/000119312525267120/ptc-20251105.htm)、[Digital Engineering 24/7（PTC 交割）](https://www.digitalengineering247.com/article/ptc-finalizes-divestiture-of-kepware-thingworx-businesses/internet-of-things)、[CNX Software（ARTIK）](https://www.cnx-software.com/2019/03/14/samsung-artik-discontinued-iot-graveyard/)、[Blynk Legacy](https://blynk.io/legacy)
- 存活者：[ThingsBoard](https://thingsboard.io/blog/one-thingsboard-source-available/)、[ThingsBoard 公司頁（人數）](https://thingsboard.io/company/)、[Inductive Automation](https://inductiveautomation.com/the-new-scada)、[Home Assistant](https://home-assistant.io/blog/2026/10/02/big-tech-ruined-the-cloud-so-were-renaming-ours/)、[RainMaker](https://docs.rainmaker.espressif.com/docs/deploy/deployment-overview/public-vs-private-rainmaker)、[Tuya FY2025](https://tuya.gcs-web.com/news-releases/news-release-details/tuya-reports-fourth-quarter-and-fiscal-2025-unaudited-financial)、[FlowFuse](https://flowfuse.com/blog/2022/10/seed-round-bring-node-red-to-enterprise/)、[Cumulocity](https://cumulocity.com/press-releases/cumulocity-completes-management-buyout-to-become-the-leading-independent-iot-platform)

### 開發者調查與 AI

- [Eclipse 2024 IoT & Embedded 調查](https://newsroom.eclipse.org/node/41237)、[JetBrains State of Embedded 2025](https://lp.jetbrains.com/the-state-of-embedded-2025)
- [RunSafe 2025](https://www.businesswire.com/news/home/20251209931517/en/RunSafe-Security-Releases-2025-AI-in-Embedded-Systems-Report-Offering-New-Insight-Into-AI-Adoption-and-Security-Gaps)、[Black Duck 2025](https://www.blackduck.com/blog/embedded-software-survey-ai-adoption-code-quality.html)、[Stack Overflow 2025](https://stackoverflow.blog/2025/07/29/developers-remain-willing-but-reluctant-to-use-ai-the-2025-developer-survey-results-are-here)
- [Cederbladh & Norin（Simulink 調查）](https://copamo.github.io/presentations/2025/Cederbladh.pdf)、[arXiv 2307.03817](https://arxiv.org/abs/2307.03817v2)、[arXiv 2603.19583（IoT-SkillsBench）](https://arxiv.org/abs/2603.19583)、[IoT-SkillsBench 全文（表 1、2）](https://arxiv.org/html/2603.19583v1)、[arXiv 2601.10220](https://arxiv.org/html/2601.10220v1)
- 晶片商 AI 工具：[ST](https://www.st.com/content/st_com/en/stm32-mcu-developer-zone/stm32-cube-for-agentic-ai.html)、[NXP Agentic AI Development](https://www.nxp.com/design/design-center/software/agentic-ai-development:AGENTIC-AI)、[Nordic](https://www.nordicsemi.com/Products/Technologies/AI-assisted-development)、[Espressif MCP](https://developer.espressif.com/blog/2026/04/esp-idf-tools-mcp-server/)、[Espressif VS Code v2.3.0](https://developer.espressif.com/blog/2026/10/esp-idf-vscode-extension-v230/)、[Microchip](https://www.globenewswire.com/news-release/2025/02/19/3028656/0/en/artificial-intelligence-meets-embedded-development-with-microchip-s-mplab-ai-coding-assistant.html)、[Tuya](https://persportaal.anp.nl/artikel/CSN-050826021/tuya-smart-launches-tuya-ai-coding-enabling-users-to-build-their-own-ai-powered-lifestyle-apps-with-natural-language)、[LVGL Editor](https://lvgl.io/editor)、[LVGL Pro XML 文件](https://docs.lvgl.io/master/xml/index.html)
- Low Code 與 AI：[Gartner 7033498 公開摘要](https://www.gartner.com/en/documents/7033498)、[OutSystems 轉載的 Gartner 報告（書目）](https://www.outsystems.com/1/low-code-ai-platforms-gartner/)、[Jitterbit 轉載的 Gartner 報告（已 404）](https://www.jitterbit.com/report/gartner-report-why-ai-wont-replace-low-code-application-platforms)、[Diginomica（OutSystems）](https://diginomica.com/outsystems-one-low-code-vendor-remodels-appeal-ai-agent)
- AI 韌體新創：[YC 標準投資條件](https://www.ycombinator.com/deal)、[VCBacked（Embedder）](https://www.vcbacked.co/company/embedder)、[Embedder](https://www.embedder.com/)、[YC 公司頁（Embedder）](https://ycombinator.com/companies/embedder)、[riseme.ai（Embedder 徵才文鏡像）](https://riseme.ai/jobs/embedder/dda9239e-aae2-4f11-9a47-29994bba7452/founding-ai-engineer)

### 併購、募資與分拆先例

- GUI／HMI 工具：[ST 收購 Draupner](https://www.st.com/content/st_com/en/about/media-center/press-item.html/c2854.html)、[eeNews Europe](https://www.eenewseurope.com/en/st-acquires-embedded-gui-specialist-2/)、[Crank 部落格](https://www.cranksoftware.com/resources/blogs/AMETEK-acquires-Crank-Software)、[MarketScreener（AMETEK）](https://www.marketscreener.com/quote/stock/AMETEK-INC-11635/news/AMETEK-Inc-acquired-Magnetrol-International-Incorporated-and-Crank-Software-Inc-33461213/)、[Tech.eu（Rightware）](https://tech.eu/2016/12/19/rightware-thundersoft/)、[MarkLines（Rightware 仍屬 ThunderSoft，2025）](https://www.marklines.com/cn/news/325959)、[ThunderSoft 2024 年報（Rightware 商譽與地區營收）](https://pdf.dfcfw.com/pdf/H2_AN202504221660624764_1.pdf)、[ST 社群（TouchGFX 執行期檢查）](https://community.st.com/stm32-mcus-touchgfx-and-gui-33/porting-touchgfx-code-to-non-st-mcus-129161)、[ST TouchGFX 附加授權條款 v4.27](https://www.st.com/resource/en/additional_license_terms/additional-license-terms-x-cube-touchgfx-v4-27.html)、[Espressif 收購 M5Stack](https://www.espressif.com/en/news/Espressif_Acquires_M5Stack)
- 工具整合：[Qt 收購 IAR](https://www.qt.io/stock/inside-information-qts-recommended-public-cash-offer-for-iar-systems-group-1751607060000-3623512)、[Qt 完成收購](https://www.qt.io/stock/qt-completes-the-recommended-public-cash-offer-to-the-shareholders-of-iar-systems-group-1760351460000-3668995?hsLang=en)、[IAR 2024 年報](https://www.iar.com/investors/reports-and-news/press-releases/iar-systems-group-ab-publ-interim-year-end-report-2024-690e5c85-6e0f-4a55-8ed5-2c300d831814)、[Qt 收購 froglogic](https://www.qt.io/stock/qt-strengthens-its-market-leader-position-with-acquisition-of-froglogic-gmbh-1618320900000-2894368)、[Qt Group 2025 Q4](https://storage.mfn.se/da194bf9-6209-480c-b461-84fedd21e1cf/qt-group-plc-2025-q4-en.pdf)、[Forrester（Mendix）](https://www.forrester.com/blogs/siemens-snaps-up-mendix-low-code-platforms-enter-new-phase/)、[Renesas 收購 Altium](https://www.renesas.com/en/about/newsroom/renesas-completes-acquisition-altium)
- 晶片商收購：[Nordic 收購 Memfault](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-acquires-Memfault)、[Nordic 交易所公告（Memfault 價格與 ARR）](https://www.inderes.dk/releases/nod-nordic-semiconductor-acquires-memfault-launches-first-complete-chip-to-cloud-platform-for-lifecycle-management-of-connected-products)、[eeNews Europe（US$1.2 億）](https://www.eenewseurope.com/en/nordic-semi-buys-memfault-for-120m)、[Qualcomm Q4 FY25 財報會議（Arduino 完成收購）](https://s204.q4cdn.com/645488518/files/doc_financials/2025/q4/Q4FY25-Earnings-Call-Transcript_11-6-25_Final.pdf)、[Qualcomm FY2025 10-K](https://s204.q4cdn.com/645488518/files/doc_financials/2025/q4/QCOM-09-28-25-FY2025-10-K-Final.pdf)、[Qualcomm Q1 FY26 10-Q（StockTitan 鏡像）](https://www.stocktitan.net/sec-filings/QCOM/10-q-qualcomm-inc-de-quarterly-earnings-report-cebab8b158aa.html)、[Nordic 收購 Neuton](https://www.nordicsemi.com/Nordic-news/2025/06/Nordic-Semiconductor-accelerates-edge-AI-leadership-with-acquisition-of-Neuton-AI)、[SiliconANGLE（Edge Impulse）](https://siliconangle.com/2025/03/10/qualcomm-acquires-edge-ai-development-startup-edge-impulse/)、[Qualcomm 收購 Arduino](https://www.qualcomm.com/news/releases/2025/10/qualcomm-to-acquire-arduino-accelerating-developers--access-to-i)、[The Register（Arduino 條款爭議）](https://www.theregister.com/2025/11/21/adafruit_makers_unhappy_with_arduino)、[NXP（Kinara）](https://www.nxp.com/company/about-nxp/newsroom/NW-NXP-COMPLETES-ACQUISITIONS-AVIVA-LINKS-KINARA)、[Silicon Labs（TI）](https://news.silabs.com/2026-02-04-Texas-Instruments-to-acquire-Silicon-Labs)、[Shawn Hymel（Imagimob）](https://shawnhymel.com/3125/state-of-edge-ai-on-microcontrollers-in-2026/)
- IoT 雲與工具：[Business Wire（Digi 收購 Particle）](https://www.businesswire.com/news/home/20260127820853/en/Digi-International-Acquires-Particle-to-Accelerate-ARR-Growth-and-Strengthen-Digis-Embedded-as-a-Service-Offering)、[Particle C 輪](https://www.particle.io/blog/particle-raises-40m-series-c/)、[Golioth 加入 Canonical](https://blog.golioth.io/golioth-is-now-a-part-of-canonical)、[Golioth 種子輪（累計 US$710 萬）](https://blog.golioth.io/may-2023-golioth-seed-round/)、[SUSE 收購 Losant](https://www.suse.com/news/suse-acquires-losant/)
- Toitware：[丹麥 CVR 第三方鏡像（解散）](https://danskcvrapi.dk/en/news/38984594-ophor-2025-10-28)、[丹麥 CVR 第三方鏡像（破產）](https://danskcvrapi.dk/nyt/38984594-kredit-2024-08-20-under-konkurs)、[Toit 發布紀錄](https://github.com/toitlang/toit/releases)
- 分拆與剝離：[Digia 分拆 Qt](https://www.digia.com/en/investors/demerger)、[PE Hub（Wind River）](https://pehub.com/tpg-to-sell-wind-river-to-aptiv-for-4-3bn)、[FSN Capital（TASKING）](https://www.fsncapital.com/en/news/fsn-capital-v-has-signed-an-agreement-to-acquire-100-of-tasking)、[Altium 出售 TASKING（iConnect007 轉載）](https://iconnect007.com/index.php/article/125627/altium-divests-non-core-tasking-to-support-enable-investment-in-altium-365/125630)、[LVGL 與 SquareLine 分手](https://lvgl.io/blog/announcement-squareline-breakup)
- 募資：[Tracxn（Memfault）](https://tracxn.com/d/companies/memfault/__Po211QCPDkRaPzfw2x3GD0B2PWSTJsnkWXtpDka8IlM/funding-and-investors)、[EE Asia（Edge Impulse）](https://www.eetasia.com/edge-impulse-raises-34m-triples-valuation/)、[GeekWire（Blues）](https://www.geekwire.com/2021/ray-ozzies-telecom-startup-blues-wireless-raises-22m-bill-gates-others/)、[Slint](https://slint.dev/blog/slint-funding-and-hiring)、[LVGL 論壇](https://forum.lvgl.io/t/business-model-for-lvgls-editor/14939)、[LVGL 公司介紹](https://lvgl.io/about)、[UK Companies House（LVGL LTD）](https://find-and-update.company-information.service.gov.uk/company/16897003/officers)

### 區域市場

- 中國：[AWTK FAQ](https://awtk.zlg.cn/docs/awtk_docs/FAQ/1.AWTK.html)、[WhyCan 論壇](https://whycan.com/t_5688.html)、[RT-Thread Studio](https://embeddedcomputing.com/technology/software-and-os/ides-application-programming/rt-thread-studio-ide-now-available-for-open-source-iot-rtos)、[鳳凰網（RT-Thread）](https://news.ifeng.com/c/8jCGykOulxm)、[阿里雲啟用碼計費](https://help.aliyun.com/zh/document_detail/150384.html)、[塗鴉支援中心](https://support.tuya.com/cn/help/_detail/K8sdy16gicd9t)
- 日本：[Renesas（emWin）](https://renesas.com/ja/document/prb/segger-emwin)、[SEGGER emWin 價目](https://www.segger.com/pricelist-emwin.html)、[Crank 2022 新聞稿（大阪エヌデーエス）](https://cranksoftware.com/press-release-crank-expands-support-in-japan-with-osakands)、[IPROS（大阪エヌデーエス，Storyboard 價格帶）](https://mono.ipros.com/product/detail/2000808564)、[IPROS（テガラ，Embedded Wizard）](https://mono.ipros.com/product/detail/2000859227)、[MONOist（exbeans）](https://monoist.itmedia.co.jp/mn/articles/1611/02/news027.html)、[CRI Glassco](https://www.cri-mw.co.jp/business/product/automobile/criglassco/)、[logmi（CRI 2026 年 9 月期法說會）](https://finance.logmi.jp/articles/385976)
- 亞洲代理商與價格（第三輪）：[SEGGER 代理商列表](https://www.segger.com/purchase/distributors)、[Embedded Wizard 價格與代理商](https://www.embedded-wizard.de/pricing)、[Qt 夥伴名錄](https://qt.io/contact-us/partners)、[LVGL Pro 價格](https://lvgl.io/pro/pricing)
- 韓國：[韓國經濟（M2I）](https://www.hankyung.com/article/2025120785641)、[Qt（現代汽車集團）](https://qt.io/press/hyundai-motor-group-selects-qt-as-their-key-hmi-technology-partner)、[Qt（LG）](https://qt.io/lg-electronics-built-with-qt)、[Digital Daily（ST 韓國 TouchGFX）](https://www.ddaily.co.kr/page/view/2023112814485539624)
- 東南亞：[VOV 引 Savills（越南外資）](https://english.vov.vn/en/economy/northern-vietnam-attracts-805-of-new-manufacturing-fdi-in-first-half-post1330684.vov)、[TechNode Global（FPT）](https://technode.global/2026/01/02/fpt-collaborates-with-telechips-to-introduce-ai-powered-intelligent-cockpit-platform/)、[MIDA（congatec 檳城）](https://www.mida.gov.my/media-release/congatec-expands-penang-rd-presence-anchoring-high-value-embedded-computing-capabilities-in-malaysia/)、[Nation Thailand（BOI）](https://www.nationthailand.com/business/investment/40061707)、[SEIPI（菲律賓電子出口）](https://www.seipi.org.ph/?p=7660)、[MarketScreener（文曄–Excelpoint）](https://ca.marketscreener.com/quote/stock/EXCELPOINT-TECHNOLOGY-LTD-6811928/news/WT-Semiconductor-Pte-Ltd-completed-the-acquisition-of-Excelpoint-Technology-Ltd--41845347/)
- 歐盟：[歐盟執委會 CRA 通報](https://digital-strategy.ec.europa.eu/en/policies/cra-reporting)、[CMS](https://cms.law/en/bel/legal-updates/ready-respond-report-the-cyber-resilience-act-s-reporting-regime-is-here)、[Bootlin](https://bootlin.com/blog/cyber-resilience-act-cra-obligations-for-manufacturers/)、[Microchip CRA 指引](https://onlinedocs.microchip.com/oxy/GUID-72031DB3-5FCC-44C9-9944-75A35D247D97-en-US-1/GUID-A875D3A7-4405-4943-9224-3D9B1D046910.html)、[EUR-Lex 實施決定 2025/138（EN 18031）](https://eur-lex.europa.eu/eli/dec_impl/2025/138/oj/eng)、[EUR-Lex 授權規則 2026/339（廢止 2022/30）](https://eur-lex.europa.eu/eli/reg_del/2026/339/oj/eng)、[EUR-Lex 實施決定 2026/2003（最新 RED 標準修正）](https://eur-lex.europa.eu/eli/dec_impl/2026/2003/oj)
- 美國：[Nextgov（ioXt）](https://www.nextgov.com/cybersecurity/2026/04/fcc-selects-ioxt-alliance-lead-cyber-labeling-program/412800)、[TechRadar（UL 退出）](https://www.techradar.com/pro/security/firm-overseeing-fccs-cyber-trust-mark-program-withdraws-over-ties-to-china)、[Cooley](https://www.cooley.com/news/insight/2026/2026-11-10-fcc-advances-us-cyber-trust-mark-program)、[Federal Register（EO 14306）](https://www.govinfo.gov/content/pkg/FR-2025-06-11/html/2025-10804.htm)、[FCC DA 26-834](https://docs.fcc.gov/public/attachments/DA-26-834A1.pdf)、[國防部 Open FAR Cases 報告（截至 2026-11-06）](https://www.acq.osd.mil/dpap/dars/opencases/farcasenum/far.pdf)、[BIS 車聯網規則](https://www.bis.gov/press-release/commerce-finalizes-rule-secure-connected-vehicle-supply-chains-foreign-adversary-threats)、[Federal Register（BIS 規則全文）](https://www.govinfo.gov/content/pkg/FR-2025-01-16/html/2025-00592.htm)、[Federal Register（FDA §524B）](https://www.govinfo.gov/content/pkg/FR-2025-06-27/html/2025-11669.htm)、[King & Spalding](https://www.kslaw.com/insights/articles/fda-finalizes-premarket-cybersecurity-guidance-for-medical-devices)

### 台灣分拆機制

- 法規：[企業併購法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0080041)、[企併法第 44 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080041&flno=44)、[產業創新條例](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0040051)、[產創第 12 條之 1](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0040051&flno=12-1)、[產創沿革](https://law.moj.gov.tw/LawClass/LawHistory.aspx?pcode=J0040051)、[公司法第 156 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080001&flno=156)、[會計師查核簽證公司登記資本額辦法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0080021)、[勞基法第 9 條之 1](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=N0030001&flno=9-1)、[勞基法施行細則第 7 條之 1](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=N0030002&flno=7-1)、[第 7 條之 3](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=N0030002&flno=7-3)、[營業秘密法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0080028)、[著作權法第 11 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0070017&flno=11)、[智慧局營業秘密保護實務教戰手冊 3.0](https://www.tipo.gov.tw/wSite/public/Attachment/0/f1742977090931.pdf)、[數位發展部技術作價申報範例](https://www-api.moda.gov.tw/File/Get/adi/zh-tw/sS8Q5fkBA6FXbzu)、[KPMG e-Tax alert 125](https://kpmg.com/tw/zh/home/insights/2019/06/e-tax-alert-125.html)、[EY 技術作價策略](https://www.ey.com/zh_tw/insights/tax/pcs-technology-contribution-strategies-for-pre-ipo-startups)、[理律（分割帳面價值函釋）](https://www.leeandli.com/TW/Newsletters/3679.htm)、[臺北高等行政法院 113 年度訴字第 948 號判決（帳面價值函釋仍適用）](https://judgment.judicial.gov.tw/FILES/TPBA/113%2c%e8%a8%b4%2c948%2c20241219%2c1.pdf)、[理律（母子型分割適用第 39 條）](https://www.leeandli.com/TW/Newsletters/5960.htm)
- US flip：[企併法第 21 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=J0080041&flno=21)、[KPMG（股份轉換緩課草案）](https://kpmg.com/tw/zh/media/press-releases/2025/12/draft-amendments-insights-from-kpmg1.html)、[聯合報（企併法修正三讀排程）](https://udn.com/news/story/6656/9705721)、[外國人投資條例](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0040002)、[公司國外投資處理辦法](https://law.moj.gov.tw/LawClass/LawAll.aspx?pcode=J0040052)、[所得基本稅額條例第 12 條](https://law.moj.gov.tw/LawClass/LawSingle.aspx?pcode=G0340115&flno=12)、[臺北國稅局（個人 CFC）](https://www.ntbt.gov.tw/singlehtml/41ae3594197f4f69b47753ce08188516?cntId=b5a5fb32200c4151b8a77f363ce4d16b)、[Wilson Sonsini《Revisiting the Delaware Flip》](https://www.wsgr.com/a/web/26603/revisiting-the-delaware-flip.pdf)
- 資金：[聯合報（國發基金天使方案）](https://udn.com/news/story/7238/9115341)、[國發基金創業天使投資方案](https://www.df.gov.tw/cp.aspx?n=D96B18C3BC73D5B5&s=B97E7588060C2084)、[天使方案作業要點與申請須知](https://www.angelinvestment.org.tw/download)、[115 年 SBIR 申請須知](https://sbir.sme.gov.tw/download/downloadarea/115%E5%B9%B4SBIR%E7%94%B3%E8%AB%8B%E9%A0%88%E7%9F%A5ctYsBK.pdf)、[A+ 前瞻計畫申請須知](https://service.moea.gov.tw/EE514/wSite/public/Attachment/00106/f1786506767729.pdf)、[AppWorks Accelerator FAQ](https://appworks.tw/accelerator-faq/)、[經濟部新聞稿（青創貸款 9.5 成保證提高至 NT$150 萬，2026-06-29）](https://www.moea.gov.tw/MNS/populace/news/News.aspx?kind=1&menu_id=40&news_id=123170)、[青年創業貸款要點全文含沿革（2026-07-01 施行）](https://0800280280.sme.gov.tw/upload/1150701青年創業貸款要點%28含沿革%29.pdf)、[TTA 南部基地申請說明](https://apply2ttas.taiwanarena.tech/)、[TTA](https://www.taiwanarena.tech/about/)、[中小及新創企業署 2025 新創投資](https://wwwcdn.sme.gov.tw/article-tw-2276-15174)、[FINDIT 2026 年報](https://findit.sme.gov.tw/tw/Res/2693)
- 先例：[數位時代（研華與資策會合資雲研物聯）](https://www.bnext.com.tw/article/47729/advantech-and-iii-set-up-a-new-joint-venture-company-focus-on-iot)、[FINDIT（雲研物聯收回研華）](https://findit.sme.gov.tw/tw/Startup/cExMcG9EQ2lyMGViYmpOUFRKYWc1QT09/Basic)、[TechNews（達擎）](https://finance.technews.tw/2020/06/17/auo-passed-the-daqing-division-base-day-january-1-2021/)、[自由時報（AMS）](https://ec.ltn.com.tw/article/breakingnews/4871165)、[Newtalk（達耀能源）](https://newtalk.tw/news/view/2026-04-13/1029575)、[數位時代（宏碁）](https://www.bnext.com.tw/article/60715/acer-group-esg-policy-ast)、[鉅亨網（宏碁子公司）](https://news.cnyes.com/news/id/4534161)、[TechNews（Gogolook）](https://technews.tw/?p=1086155)、[中央社（Appier）](https://www.cna.com.tw/news/afe/202501310025.aspx)
