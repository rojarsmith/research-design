# NOVAKON 模式：軟體免費、靠硬體賺錢，適合 EHS 嗎？

研究日期：2026 年 11 月 20 日，Asia/Taipei。

本文回答：「如果 EHS 轉作 [NOVAKON iFace Designer](https://www.novakon.com.tw/tw/software/detail/iFace-Designer) 這種產品（設計軟體免費、靠自有品牌 HMI 硬體賺錢、硬體委外生產），NOVAKON 的硬體價格與毛利率有沒有情報？沒有的話，同業怎麼推估？」範圍包括 NOVAKON 的公司與財務、iFace Designer 與硬體、價格、同業毛利率、推估方法，以及對 EHS 的含意。新產品要從 MCU 改到 Cortex-A 或 x86 執行環境的技術問題，另見 [18 號研究](18-mpu-runtime.zh-TW.md)。

> **本文是研究材料，不是投資或財務意見。** NOVAKON 的毛利率沒有公開，文中的毛利率與台數都是推論，用來判斷量級，不能當成事實引用。

**標示說明**

| 標示 | 意思 |
| --- | --- |
| （無標示） | 有一手來源（政府登記、上市公司年報、原廠頁面）或可信報導的連結 |
| 二手來源 | 搜尋摘要、經銷商或拍賣頁面、資料彙整網站 |
| 廠商說法 | 公司自己的行銷文字 |
| 推論 | 本研究的判斷或計算，不是來源的說法 |

## 結論

1. **NOVAKON 不是獨立的純軟公司，而是上櫃工業電腦廠廣積（iBASE，8050）持股 70% 的子公司。** 速博康科技股份有限公司 2010 年設立，實收資本 NT$8,880 萬，約 20 至 30 人，總部和廣積在同一棟樓。廣積在 2022 與 2026 年都增資。這個模式背後有母公司的資本、工廠與品管。
2. **規模小但賺錢。** 廣積年報揭露 NOVAKON 2025 年營收 NT$2.31 億（約 US$720 萬，年增 61%），營業利益 NT$6,681 萬，營業利益率約 29%；2023 年曾虧損 NT$2,520 萬。毛利率、出貨台數、客戶都沒有公開。
3. **硬體大概不是委外給外部代工廠，而是由母公司生產（推論）。** NOVAKON 自稱「臺灣研發、生產」，廣積年報列出對 NOVAKON 的銷貨（2022 年 NT$4,839 萬、2023 年 NT$2,666 萬，約佔 NOVAKON 營收三到四成），廣積在三重、新莊、平鎮有工廠。所以它比較像「集團內代工」，和完全委外不同。**EDT 的角色正好對應廣積：EDT 有面板，可以當自家供應商。**
4. **價格：7 吋約 US$250 至 370，10 吋約 US$480 至 530，比 Weintek 便宜三到五成。** 原廠不公布價格表，全部「請洽詢」；數字來自塞爾維亞代理商與歐美經銷商（二手來源）。同級 Weintek 7 吋在美國約 US$390 至 710。台灣零售通路只找到二手品。
5. **毛利率有兩種推估，結果不同：**
   - 用同業類比（中國上市 HMI 廠毛利 30% 至 42%，扣掉外包代工的利潤），得到約 25% 至 40%。
   - 用 NOVAKON 自己的營業利益反推：營業利益率 29%，加上 20 至 30 人公司的營業費用約佔營收 13% 至 28%，2025 年毛利率約 **42% 至 57%**（推論）。
   - 後者用的是 NOVAKON 的實際數字，比較可信。這也符合「品牌 HMI 售價約是物料成本 4 至 6 倍」的經驗法則。
6. **HMI 在同業裡是低毛利產品，靠量撐。** 步科（Kinco）HMI 每年約 35 萬台、均價約 RMB 480、毛利約 41%；信捷（Xinje）約 65 萬台、均價約 RMB 313、毛利約 33%，而信捷 PLC 毛利 57%。兩家都自己組裝測試，只外包 PCBA，物料約佔 HMI 成本的 87%。
7. **EDT 本身的毛利率約 23%（面板模組的零組件區間），品牌 HMI 約 40% 至 55%。** 做自有品牌 HMI 等於往價值鏈上游走（推論）。不過 EDT 近年營收從 NT$43.9 億（2023）降到 NT$30.1 億（2025），營業利益率從 10% 降到約 3%，新事業要能自負盈虧。
8. **iFace 的技術已經老舊，這是 EHS 的機會。** HMI 跑嵌入式 Linux 加 Qt 4.8，用 TI AM335x 的參考設計；工業電腦跑 Windows 10 IoT 或 Debian 11。2025 年有五個重大 CVE（root 空密碼、未驗證的 UDP 服務可遠端以 root 執行程式），原廠未回應。EHS 若用現代、低記憶體的 LVGL 執行環境，並內建 CRA 要求的資安與更新機制，可以做出差異（推論）。
9. **對 EHS 的判斷（推論）：NOVAKON 模式適合 EDT 或 EDT 持股的子公司，不適合沒有硬體的純軟公司。** 硬體損益大約每年 2,500 至 4,000 台才打平，還沒算軟體團隊。NOVAKON 花了約 13 年、靠母公司才做到 NT$2.3 億。最接近的做法是「EDT 持股約 70% 的子公司，EDT 供面板、外部代工廠或 EDT 組裝」，正好對應 [16 號研究](16-global-evidence.zh-TW.md)第七節「新設分割、EDT 保留股份」那條路。

## 一、NOVAKON 是誰

| 項目 | 內容 |
| --- | --- |
| 公司 | 速博康科技股份有限公司（NOVAKON Co., Ltd.），統編 29176695，2010-08-17 設立（[商工登記公開資料](https://data.gcis.nat.gov.tw/od/data/api/5F64D864-61CB-4D0D-8AD9-492047CC1EA6?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50)） |
| 資本 | 實收 NT$8,880 萬（2026-05-26 從 NT$6,880 萬增資），資本總額 NT$1.688 億 |
| 股東 | 廣積科技持有 70.0%，四席法人董事都代表廣積（[董監事資料](https://data.gcis.nat.gov.tw/od/data/api/4E5F7653-1B91-4DDC-99D5-468530FAE396?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50)）；廣積與研揚（華碩集團）交叉持股 |
| 上市 | 沒有上市、上櫃或興櫃，財務只出現在廣積年報 |
| 人數 | 約 20 至 30 人（104 顯示 20 人、518 顯示 30 人，二手來源） |
| 市場 | 自稱銷往 18 國、外銷超過 90%（廠商說法）；美國經 Novatek Controls（海關資料唯一收貨人，二手來源），德國經 Bressner（2024 年合作），塞爾維亞經 EP Solutions；首頁仍在招募全球代理商 |
| 獎項 | 2022 年玉山獎、2023 年小巨人獎（[工商時報](https://www.ctee.com.tw/news/20231124700537-431202)）；工商時報稱它也是「國內外知名的 HMI 代工廠」，代表也接 ODM 案 |
| 和其他 HMI 廠的關係 | 沒有找到和 Weintek、研華、台達的淵源；自稱有「20 年以上 HMI 經驗」，團隊可能來自廣積與研揚集團（推論） |

**財務（廣積英文年報「各關係企業營業狀況」，單位 NT$ 千元）**

| 年度 | 營收 | 營業利益 | 稅後淨利 | 營業利益率（推論，計算） |
| --- | --- | --- | --- | --- |
| 2022 | 132,128 | 29,200 | 32,003 | 22.1% |
| 2023 | 94,274 | (25,198) | (24,045) | −26.7% |
| 2024 | 142,806 | 25,993 | 60,026 | 18.2% |
| 2025 | 230,628 | 66,811 | 49,954 | 29.0% |

來源：[廣積 2025 年報（英文）](https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=F&co_id=8050&filename=2025_8050_20260626FE4.pdf)，以及 2022 至 2024 年的同類年報（見來源）。NOVAKON 約佔廣積合併營收（約 NT$53.6 億）的 4%。2024 年淨利高於營業利益約 NT$3,400 萬，是未說明的業外收益。

## 二、iFace Designer 與硬體

| 項目 | 內容 |
| --- | --- |
| 授權 | 原廠稱「FREE HMI development software」（[NOVAKON 2024-07](https://www.novakon.com.tw/en/news/detail/Novakon_s_Versatile_Human_Machine_Interface__HMI__Solutions_with_iFace_Designer)），要註冊會員才能下載，沒有公開授權條款；塞爾維亞代理商標價 0（[EP Solutions](https://www.ep-solutions.rs/en/novakon-software-for-hmi-and-gateway)） |
| 可能收費的部分 | x86 工業電腦的「iFace-Designer Runtime」列為軟體選配，價格不公開；ODM 軟體客製是服務；沒有雲端訂閱（推論，依產品頁） |
| 綁硬體 | 執行環境只跑在 NOVAKON 自己的 ARM HMI 與 x86 工業電腦上 |
| 編輯器 | Windows 10、11（64 位元）；最新 2.0.05（2026-02-09 版），可免編譯模擬 |
| HMI 執行平台 | 嵌入式 Linux 加 Qt 4.8（Qt/Embedded、LinuxFb），以 TI AM335x 參考板設計為基礎（[CyberDanube 資安公告](https://www.mail-archive.com/fulldisclosure@seclists.org/msg09126.html)，二手來源）；資料表寫 Linux 核心 3.14 |
| 工業電腦平台 | Windows 10 IoT 或 Debian 11；Intel J6412、N97、Core i5；部分機型通過 Siemens Industrial Edge 認證 |
| 協定 | Modbus RTU/TCP/ASCII 主從站、CANopen、BACnet IP、OPC UA 伺服器（2.0.05 新增，[NOVAKON 2025-10](https://www.novakon.com.tw/en/news/detail/iFace_Designer_V2_0_05_Software_Update)）、MQTT、REST API；約 25 個 PLC 品牌，自稱涵蓋「90% 以上主流 PLC」（廠商說法） |
| 功能 | 趨勢、警報、16 組資料記錄、配方、巨集、排程、16 種語言、密碼、CNC G-code |
| 遠端 | VNC、FTP、PLC 穿透、VPN |
| 資安 | 2025 年 CVE-2025-9962 至 9966：UDP 60681 埠未驗證可上傳下載專案、堆疊溢位可以 root 遠端執行、root 空密碼、USB 路徑穿越、程序以 root 執行；發布時沒有修補，原廠未回應（同上公告） |

**硬體**

| 系列 | 規格 | 認證 |
| --- | --- | --- |
| N04、N07 | 4.3 吋、7 吋，ARM9 400 MHz，128 MB DRAM，電阻式觸控 | 頁面列 CE、FCC、UL；沒有 KC，也查不到公開的 UL 檔案號碼 |
| P07、P10、P12、P15 | 7 至 15.6 吋，TI AM3352 Cortex-A8 1 GHz，512 MB DRAM，4 GB eMMC | 同上 |
| 工業電腦 NPP、NFP、NOF、NRP | 12 至 21.5 吋，Intel Atom、N97、Core i5 | CE、FCC |
| 其他 | NWH10（Cortex-A55）、GW-01 協定閘道器（iM Connect 軟體） | |

有線 HMI 依 FCC 規則走自我宣告（SDoC），沒有 FCC ID，所以查不到製造廠的 FCC 申請資料（[eCFR 47 CFR 2.1077](https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-2/subpart-J/subject-group-ECFR64d0de5de218f0c/section-2.1077)）。

**誰生產（推論）：** 營業項目有電腦與電子設備製造，但沒查到工廠登記。廣積對 NOVAKON 有銷貨，廣積在台灣有三個工廠、越南有新廠，而且 2023 年溫室氣體盤查邊界含 NOVAKON。最可能的情況是 NOVAKON 設計、廣積生產。這和「完全委外給外部代工廠」不同：集團內代工有工業電腦等級的品管與認證經驗，外部代工則要自己管。

## 三、價格情報

原廠沒有價格表。以下是 2026-11-20 看到的新品價格（二手來源；部分只看到搜尋摘要）。

| 機型 | 賣家（國家） | 價格 | 約合 US$ |
| --- | --- | --- | --- |
| N07F-N（7 吋） | EP Solutions（塞爾維亞），不含稅 | RSD 28,748 | 265 |
| N07F-N | PLC-City（灰色市場） | US$251 | 251 |
| N07F-N | Automation Database（英國），不含稅 | £257 | 335 |
| P07F-N（7 吋） | EP Solutions 特價，不含稅 | RSD 36,831 | 340 |
| P07F-N | 美國、墨西哥經銷商 | US$249 至 371（含稅） | 250 至 320 |
| P10F-N（10.2 吋） | EP Solutions 特價、PLC-City | RSD 56,488、US$507 | 507 至 520 |
| P15F-N（15.6 吋） | EP Solutions 特價 | RSD 132,477 | 1,220 |

來源：[EP Solutions](https://www.ep-solutions.rs/en/p-series-hmi-operator-panel-standard-hmi-panel)、[PLC-City](https://plc-city.com/shop/en/novakon-hmi-panel/n07f-n-nfs.html)。中國的 OKmarts 標 US$692 至 1,346，是中間商加價，不採用。台灣露天只有二手 N07（NT$1,000 至 1,500），PChome、momo、蝦皮、Amazon、AliExpress 都沒有找到。

**同業比較（7 吋，新品）**

| 品牌 | 價格 | 來源 |
| --- | --- | --- |
| Weintek MT8071iP、MT8072iP | US$345 至 398；歐盟不含稅 €231 | [Wolf Automation](https://www.wolfautomation.com/products/hmi/mt8072ip/)、[Unipi](https://unipi.technology/weintek-mt8072ip-p539)（二手來源） |
| Weintek cMT 電容式 | US$587 至 844 | Wolf Automation（二手來源） |
| Maple Systems（Weintek 代工的美國品牌） | 7 吋定價 US$450，基本系列 US$345 起，軟體免費 | [Maple Systems](https://maplesystems.com/product/modelname/hmi5070b/) |
| Kinco GL070 | US$131 至 168 | [Anaheim Automation](https://anaheimautomation.com/KNC-HMI-G070.html)（二手來源） |
| 昆侖通態 TPC7062 | 中國 B2B 約 RMB 450 至 750 | 搜尋摘要，日期不明 |
| 中國白牌（Mochuan、Coolmay） | 7 吋 US$90 至 160，10.1 吋 US$185 至 190，附免費軟體，保固 36 個月 | [Alibaba](https://www.alibaba.com/product-introduction/Mochuan-hmi-free-software-touch-screen_60477361034.html)（二手來源） |

（推論）NOVAKON 的定位是「比 Weintek 便宜、比中國品牌貴的台灣品牌」。新進者沒有 UL、沒有幾百種驅動、沒有現場口碑，在美國通路很難賣到 Weintek 的價格；7 吋街價 US$180 至 280、10 吋 US$300 至 450 比較實際。

## 四、毛利率：同業財報怎麼說

只有兩家上市公司揭露 HMI 的台數、均價與毛利率，都是中國廠商（年報與招股書，一手來源）。

| 公司 | 年度 | HMI 台數 | 均價（不含稅，出廠到通路） | 毛利率 |
| --- | --- | --- | --- | --- |
| 步科 Kinco 人機界面 | 2017 至 2020 上半年 | 每年 23.8 萬至 24.6 萬 | RMB 540 至 580 | 35.0% 至 40.2% |
| 步科 控制系統（HMI 佔台數 82% 至 86%） | 2025 | 42.4 萬（HMI 34.8 萬） | 約 RMB 481（推論，計算） | 41.33% |
| 信捷 Xinje 人機界面 | 2025 | 65.6 萬 | 約 RMB 313（推論，計算） | 32.78% |

來源：[步科招股書](https://pdf.dfcfw.com/pdf/H2_AN202011051426899033_1.pdf)、[步科 2025 年報](https://pdf.dfcfw.com/pdf/H2_AN202604021820984739_1.pdf)、[信捷 2025 年報](https://pdf.dfcfw.com/pdf/H2_AN202604161821274535_1.pdf)。

- **物料約佔 HMI 銷貨成本 87%。** 信捷 2025 年每台成本約 RMB 211，其中物料約 RMB 184；步科 2017 至 2020 年每片 LCD 進價 RMB 78 至 97、觸控面板 RMB 25 至 31。組裝測試的附加價值很低，毛利主要來自品牌、通路與軟體（推論）。
- **HMI 是控制產品線裡毛利最低的。** 信捷 PLC 毛利 57%、HMI 33%；匯川 HMI 加 PLC 為 45% 至 54%（步科招股書引用）。
- **兩家都自己組裝測試，只外包 PCBA。** Weintek 在台灣與中國有自有工廠。查到的同業中，只有 Maple Systems 這類「經銷商品牌」是完全委外。
- **市占（MIR，2019 年中國 HMI 產值）：** Siemens 25.6%、Weintek 14.2%、昆侖通態 12.3%、步科 4.6%。以步科當年 HMI 營收反推，Weintek 中國營收約 RMB 4.3 億、昆侖通態約 RMB 3.7 億（推論，計算）。Weintek 沒有公開財務。

**其他同業（全公司或集團毛利率，不是 HMI 專屬）**

| 公司 | 毛利率 | 說明 |
| --- | --- | --- |
| Ependion（原 Beijer 集團） | 集團 53.5%（2025，推論，計算） | HMI 事業 Beijer Electronics 營收 858 MSEK，營業利益率只有 9.2%（[Ependion 2025 Q4 報告](https://storage.mfn.se/34508b36-12ac-48ab-88c2-6d59bd59f427/report-in-pdf-format.pdf)） |
| Red Lion（HMS 收購） | 約 55%，調整後 EBIT 約 21% | 有自有工廠，產品含網路設備（[HMS](https://www.hms-networks.com/news/news-details/10-12-2023-hms-networks-acquires-red-lion-controls-and-significantly-expands-its-presence-in-north-america)） |
| 研華（2395） | 約 40% | 二手來源（stockanalysis） |
| 威強電（3022） | 33% 至 37% | 同上 |
| 廣積（8050，NOVAKON 母公司） | 21% 至 25% | 同上 |
| **全台晶像 EDT（3038）** | 約 23%，營業利益率從 10.3%（2023）降到 3.0%（近四季） | [stockanalysis](https://stockanalysis.com/quote/tpe/3038/financials/)（二手來源） |
| 代工廠 Plexus、Benchmark、Kimball | 7% 至 10% | 純代工；小型工控代工與台灣工業 ODM 約 15% 至 25% |

## 五、推估方法與 NOVAKON 的推估

### 五種方法

| 方法 | 怎麼做 | 這次的結果 |
| --- | --- | --- |
| 1. 由售價往下扣 | 街價 → 扣稅 → 扣通路 20% 至 30% → 扣品牌毛利 → 出廠價 → 除以 1.15 至 1.35 得物料成本 | Weintek MT8072iP 街價 US$398，反推物料約 US$95 至 130 |
| 2. 由物料往上加 | 逐項估面板、觸控、SoC、記憶體、電源、外殼，加組裝測試與代工利潤 | 見下表 |
| 3. 上市同業類比 | 用步科、信捷的毛利與均價，或台灣工業電腦廠的毛利帶 | 自有工廠 30% 至 42%；完全委外要再扣 5 至 10 個百分點（推論） |
| 4. 從母公司年報反推 | NOVAKON 的營業利益率加上估計的營業費用率 | 2025 年毛利率約 42% 至 57%（見下） |
| 5. 貿易資料與訪談 | 海關提單（Panjiva 有 157 筆 NOVAKON 對美出貨，品名遮蔽）、認證資料庫、代工廠報價、代理商折扣 | 這次沒有實際報價，是下一步 |

**由物料往上加（2026 年價格，年量 1,000 至 5,000 台，推論）**

| 項目（US$） | 7 吋入門（T113 或 STM32MP13、電阻式、塑膠殼） | 7 吋高階（AM62x 或 RK3566、1 GB、電容式、鋁前框） | 10.1 吋入門 | 10.1 吋高階 |
| --- | --- | --- | --- | --- |
| 面板 | 14 至 22 | 18 至 30 | 30 至 45 | 40 至 65 |
| 觸控 | 4 至 7 | 9 至 18 | 8 至 14 | 15 至 30 |
| SoC | 5 至 8 | 8 至 20 | 5 至 8 | 8 至 20 |
| DRAM 加 eMMC | 5 至 20 | 23 至 48 | 5 至 20 | 23 至 48 |
| 電源、通訊、PCB、外殼、包裝 | 17 至 31 | 29 至 53 | 21 至 37 | 36 至 65 |
| **物料合計** | **約 45 至 88** | **約 87 至 169** | **約 69 至 124** | **約 122 至 228** |
| **出廠價（加組裝測試與 8% 至 25% 代工利潤）** | **約 55 至 115** | **約 100 至 220** | **約 80 至 165** | **約 140 至 295** |

- **2026 年的記憶體漲價是最大變數。** eMMC 合約價 2026 年第一季漲 147% 至 183%（[TechNews](https://technews.tw/?p=1526986)），TrendForce 估第二季 DRAM 再漲 58% 至 63%（[TrendForce](https://www.trendforce.com/presscenter/news/20260331-12995.html)）。記憶體從約佔 7 吋 Linux HMI 物料的 10% 升到 25% 至 35%。DRAM 封裝在 SoC 內的晶片（T113-S3、S4）加上省記憶體的 LVGL 執行環境，可以部分避開（推論）。
- **面板加觸控約佔物料 25% 至 40%，是 EDT 唯一能自己供應的大項（推論）。**
- **品牌售價約是物料的 4 至 6 倍，中國品牌約 2 至 3 倍（推論）。** 每 US$100 街價：通路 20 至 30、品牌毛利 35 至 50、出廠價 20 至 35（其中物料 16 至 28）。

### NOVAKON 的推估（全部是推論）

- **毛利率：方法 4 最可信。** 2025 年營業利益率 29%。20 至 30 人公司的人事費用每年約 NT$2,000 萬至 4,500 萬，加上行銷、認證、差旅、折舊約 NT$1,000 萬至 2,000 萬，營業費用合計約 NT$3,000 萬至 6,500 萬，佔營收 13% 至 28%。所以毛利率約 **42% 至 57%**，中間值約 50%。方法 3 的 25% 至 40% 偏低，因為它假設外部代工並用中國的低價；NOVAKON 賣歐美、比中國貴，而且由母公司生產。
- **台數：** 營收 US$720 萬包含 HMI 與單價高得多的工業電腦（US$800 至 2,000）。若 HMI 佔一半、平均出廠價 US$150 至 300，HMI 約每年 1 萬至 2.5 萬台。這只是量級，約為步科的十分之一以下。
- **2023 年虧損**（營收掉三成、費用上升）顯示小規模硬體事業的波動很大。

## 六、對 EHS 的含意

**和使用者前提的差別。** NOVAKON 的硬體不是完全委外給外部代工廠，而是很可能由母公司廣積生產；軟體也不是完全不收費，x86 執行環境與客製服務可能收費。它是「上市工業電腦廠的子公司」，不是「純軟公司」。

**EDT 的位置比純軟公司好（推論）：**

| 條件 | NOVAKON | EDT 版本 |
| --- | --- | --- |
| 母公司資本 | 廣積持股 70%，兩次增資 | EDT 可持股（16 號研究的新設分割路徑） |
| 生產 | 母公司工廠 | 面板與觸控由 EDT 供應；主板與組裝找代工廠，或 EDT 自己組裝 |
| 最大的物料項目 | 外購 | 面板加觸控約佔物料 25% 至 40%，EDT 自己有 |
| 軟體 | Qt 4.8、AM335x、2025 年 5 個重大 CVE | EHS 已有 LVGL 設計器與模擬器，可以做現代、低記憶體、合乎 CRA 的執行環境 |
| 低階市場 | 只有 Linux 與 x86 | LVGL 也能跑 MCU，可以做比 Linux HMI 更便宜的機型 |

**經濟面（推論）：**

| 項目 | 估計 |
| --- | --- |
| 單台硬體毛利 | 7 吋街價 US$240、通路 25%、到岸成本 US$110 時約 US$65（36%）；搭配 10 吋後平均約 US$84 |
| 一次性固定成本 | 認證（CE、UL/cUL 61010-2-201、KC）US$3 萬至 7.5 萬，加模具與代工開發，合計約 US$15 萬至 35 萬 |
| 每年固定成本 | UL 追蹤、維修、代工管理、庫存約 US$15 萬至 25 萬 |
| 硬體損益兩平 | 每年約 2,500 至 4,000 台，還沒算軟體團隊 |
| 每年 2 萬台 | 硬體毛利約 US$130 萬，一年內回收 |

**主要風險：**

- **和 EDT 面板客戶衝突。** 有些 EDT 客戶自己做 HMI，EDT 推自有品牌就和客戶競爭。緩解方式：主要賣終端用戶與機械廠；對面板客戶改賣「HMI Studio 內建」的執行環境授權；分開代理商區域。
- **驅動缺口。** NOVAKON 約 25 個 PLC 品牌，Weintek 超過 400 種協定（[Weintek](https://www.weintek.com/Software/EasyBuilder/EasyBuilderPro)）。驅動是持續成本，要接真機測試。
- **中國價格地板。** 中國 7 吋白牌 US$90 至 160，也附免費軟體、保固 36 個月。免費軟體只是基本條件，不是差異化。
- **零件停產。** Toppan 工業 LCD 的最後採購期 2026 年 10 月已截止；客戶要求 5 至 10 年供貨，每次換料都可能要重測 EMC 與 UL。
- **記憶體漲價。** 代工合約要有記憶體價格調整條款。

**守住毛利的方法（推論）。** 同業的做法有三種：

- 軟體只跑自家硬體：Weintek、NOVAKON 都這樣做。
- 付費遠端存取：Weintek EasyAccess 2.0 啟用卡約 US$104 至 122，每年流量卡約 US$268。
- 擴大產品線：控制器、閘道器、內建 CODESYS。

EHS 還多一條路：把執行環境授權給第三方硬體，尤其是 EDT 的面板模組客戶與 MCU 等級的機型。這是只做 Linux 的 HMI 廠做不到的。

**決策含意（推論）：**

| 決策 | 建議 |
| --- | --- |
| 要不要學 NOVAKON | 只在 EDT 或 EDT 持股的子公司做；純軟公司沒有硬體毛利，免費軟體沒有收入來源 |
| 組織 | 仿 NOVAKON：EDT 持股約 70%、團隊與外部投資人持其餘；EDT 供面板，主板與組裝委外或自製 |
| 第一批產品 | 7 吋與 10 吋 Linux HMI，加一款 MCU 等級的低價機型；用 DRAM 封裝在內的 SoC 降低記憶體風險 |
| 差異化 | 現代資安（不用 root、簽章更新、SBOM）、LVGL 低記憶體、整機模擬、AI 可讀的專案模型（15、16 號研究） |
| 定價 | 7 吋街價約 US$180 至 280，比 Weintek 低，比中國品牌高 |
| 先做的事 | 向 3 至 5 家代工廠詢價（7 吋與 10 吋，500、2,000、5,000 台）；訪談台灣、歐美代理商的折扣與必要驅動；內部確認 EDT 面板轉撥價與長期供貨承諾 |

## 待驗證

- NOVAKON 毛利率、出貨台數、HMI 與工業電腦的營收比例：都沒有公開；本文的 42% 至 57% 是由營業利益率反推。
- 廣積是否實際為 NOVAKON 生產：只有銷貨金額與工廠位置，沒有直接說明。
- iFace Designer 授權條款與 x86 執行環境價格：要註冊會員或詢價。
- NOVAKON 台灣新品價格：台灣零售通路沒有新品，要向原廠詢價。
- 白牌 HMI 的實際出廠報價、代理商折扣表、台灣實驗室的 UL 61010-2-201 認證報價：都要直接詢價。
- 步科、信捷 2026 年上半年報告的 HMI 數字沒有擷取；昆侖通態、顯控、Weintek 沒有公開財務。
- TI AM62x、STM32MP2、RK3566 的 2026 年晶片價格沒有查到最新值。

## 來源

### NOVAKON

- 原廠：[iFace Designer](https://www.novakon.com.tw/tw/software/detail/iFace-Designer)、[關於我們](https://www.novakon.com.tw/tw/about)、[HMI 系列](https://www.novakon.com.tw/en/hardware/series/HMI)、[工業電腦系列](https://www.novakon.com.tw/en/hardware/series/Industrial_Panel_PC)、[免費軟體新聞（2024-07）](https://www.novakon.com.tw/en/news/detail/Novakon_s_Versatile_Human_Machine_Interface__HMI__Solutions_with_iFace_Designer)、[2.0.05 版（2025-10）](https://www.novakon.com.tw/en/news/detail/iFace_Designer_V2_0_05_Software_Update)
- 登記：[商工登記基本資料](https://data.gcis.nat.gov.tw/od/data/api/5F64D864-61CB-4D0D-8AD9-492047CC1EA6?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50)、[董監事](https://data.gcis.nat.gov.tw/od/data/api/4E5F7653-1B91-4DDC-99D5-468530FAE396?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50)、[營業項目](https://data.gcis.nat.gov.tw/od/data/api/236EE382-4942-41A9-BD03-CA0709025E7C?$format=json&$filter=Business_Accounting_NO%20eq%2029176695&$skip=0&$top=50)
- 財務：廣積英文年報 [2022](https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=F&co_id=8050&filename=2022_8050_20230620FE4.pdf)、[2023](https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=F&co_id=8050&filename=2023_8050_20240619FE4.pdf)、[2024](https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=F&co_id=8050&filename=2024_8050_20250620FE4.pdf)、[2025](https://doc.twse.com.tw/server-java/t57sb01?step=9&kind=F&co_id=8050&filename=2025_8050_20260626FE4.pdf)（公開資訊觀測站，連結會轉到一次性下載頁）
- 報導與其他：[工商時報 2023 小巨人獎](https://www.ctee.com.tw/news/20231124700537-431202)、[工商時報 2022 玉山獎](https://www.ctee.com.tw/news/20221021700544-431202)、[CyberDanube 資安公告](https://www.mail-archive.com/fulldisclosure@seclists.org/msg09126.html)、[Panjiva](https://cn.panjiva.com/Novakon-Co-Ltd/45298805)、[Bressner 合作](https://pressebox.com/pressrelease/bressner-technology-gmbh/Collaboration-for-Siemens-Industrial-Edge-Certified-Panel-PCs/boxid/1213753)、[廣積越南廠](https://uanalyze.com.tw/articles/3571312901)
- 價格：[EP Solutions](https://www.ep-solutions.rs/en/p-series-hmi-operator-panel-standard-hmi-panel)、[EP Solutions N07](https://www.ep-solutions.rs/en/novakon-n07-touch-screen-hmi-n07f-n)、[PLC-City N07](https://plc-city.com/shop/en/novakon-hmi-panel/n07f-n-nfs.html)、[PLC-City P10](https://www.plc-city.com/shop/en/novakon-hmi-panel/p10f-n-nfs.html)、[OKmarts](https://okmarts.com/product/hmi/novakon.html)、[露天搜尋](https://www.ruten.com.tw/find/?q=novakon)

### 同業財務

- 一手：[步科招股書](https://pdf.dfcfw.com/pdf/H2_AN202011051426899033_1.pdf)、[步科 2024 年報](https://pdf.dfcfw.com/pdf/H2_AN202504241661694668_1.pdf)、[步科 2025 年報](https://pdf.dfcfw.com/pdf/H2_AN202604021820984739_1.pdf)、[信捷 2024 年報](https://pdf.dfcfw.com/pdf/H2_AN202504251662306008_1.pdf)、[信捷 2025 年報](https://pdf.dfcfw.com/pdf/H2_AN202604161821274535_1.pdf)、[Ependion 2025 Q4](https://storage.mfn.se/34508b36-12ac-48ab-88c2-6d59bd59f427/report-in-pdf-format.pdf)、[HMS 收購 Red Lion](https://www.hms-networks.com/news/news-details/10-12-2023-hms-networks-acquires-red-lion-controls-and-significantly-expands-its-presence-in-north-america)、[Maple Systems](https://maplesystems.com/product/modelname/hmi5070b/)、[Maple 價格 FAQ](https://maplesystems.com/faq/what-is-the-upfront-cost-of-buying-an-hmi/)
- 二手：stockanalysis.com 各公司損益表（[全台晶像 3038](https://stockanalysis.com/quote/tpe/3038/financials/) 等）、[Weintek 簡介（ipc2u）](https://ipc2u.com/about-us/vendors/weintek/)

### 成本與通路

- 記憶體：[TrendForce 2026-03-31](https://www.trendforce.com/presscenter/news/20260331-12995.html)、[TechNews eMMC](https://technews.tw/?p=1526986)、[Raspberry Pi 漲價](https://raspberrypi.com/news/more-memory-driven-price-rises)、[CNX 零件漲價](https://www.cnx-software.com/2026/01/29/electronic-components-price-and-lead-time-increases-announced-across-the-board-and-not-only-because-of-ram/)
- 零件：[LCSC T113-S3](https://www.lcsc.com/product-detail/C5197687.html)、[ST eStore STM32MP135](https://estore.st.com/en/stm32mp135fag7-cpn.html)、[TI AM62x 發表價（CNX）](https://www.cnx-software.com/2022/06/07/ti-sitara-am623-am625-cortex-a53-socs-offer-low-power-ai-for-hmi-and-iot-applications/)、[Forlinx RK3568 SoM](https://www.digikey.com.mx/en/products/detail/forlinx-embedded/FORLINX-FET3568-C-202GSE16GCA10/21810672)、[Toradex i.MX8M Mini](https://www.toradex.com/computer-on-modules/verdin-arm-family/nxp-imx-8m-mini)、[7 吋面板（Global Sources）](https://www.globalsources.com/Modul-LCD/Modul-LCD-TFT-1237031013p.htm)、[7 吋電容觸控（BuyDisplay）](https://www.buydisplay.com/chinese/7-inch-capacitive-touch-panel-with-controller-ft5316-for-800x480)
- 代工與通路：[Lincoln International EMS 季報 2026 Q2](https://www.lincolninternational.com/wp-content/uploads/2026-Q2_EMS-Quarterly-Review_vF.pdf)、[Wesco 2025](https://wesco.gcs-web.com/news-releases/news-release-details/wesco-international-reports-fourth-quarter-and-full-year-2025)、[Predictable Designs 定價](https://predictabledesigns.com/how-to-price-your-product-for-various-distribution-channels/)
- 同業售價與服務：[Wolf Automation MT8072iP](https://www.wolfautomation.com/products/hmi/mt8072ip/)、[Unipi MT8072iP](https://unipi.technology/weintek-mt8072ip-p539)、[Anaheim Automation Kinco](https://anaheimautomation.com/KNC-HMI-G070.html)、[Mochuan（Alibaba）](https://www.alibaba.com/product-introduction/Mochuan-hmi-free-software-touch-screen_60477361034.html)、[Weintek EasyBuilder Pro](https://www.weintek.com/Software/EasyBuilder/EasyBuilderPro)、[EasyAccess 2.0 啟用卡](https://wolfautomation.com/easyaccess-2-0-activation-card)
- 零件停產：[Toppan 工業 LCD 停產](https://embeddedcomputing.com/application/industrial/industrial-computing/data-modul-supports-customers-as-toppan-display-solutions-ends-industrial-lcd-display-production)

研究筆記在 [notes/17-novakon-hardware-model](notes/17-novakon-hardware-model/)：`novakon_company_and_prices.md`、`peer_financials.md`、`cost_estimation_and_model.md`。
