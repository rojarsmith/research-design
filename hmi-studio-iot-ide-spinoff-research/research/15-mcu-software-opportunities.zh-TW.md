# MCU 領域還有哪些軟體機會

查核日期：2026 年 11 月 19 日。本文接續 [14 號：錢收在哪裡](14-mcu-software-monetization.zh-TW.md)，回答：**MCU 領域還有哪些軟體機會，是現在才打開、還沒被佔滿的？** 排序是本研究的判斷，依據兩點：有沒有截止日期或資金在推；現有方案是不是太貴或太弱。AI 新創的資金與功能來自新聞稿與報導，沒有實際試用。第五節的補充只查證了 RED、電池護照、產線燒錄與 Edge Impulse 收購，其餘是依前面幾份研究推論。

## 結論

**還有機會，但新的錢不在「讓人寫程式更舒服」，而在三股外力剛打開的缺口：法規（歐盟 CRA）、AI、測試自動化。**

MCU 軟體還能賺錢的地方，是**幫設備商證明「這台東西是對的、是安全的、出貨後管得住」**，而不是幫他們把程式寫得更快。

對 EHS 來說，最好的切入點是把 AI、整機模擬和 CRA 證據綁在一起：**AI 產生 → 模擬驗證 → 一鍵輸出 SBOM 與更新紀錄。** 目前沒有任何一家工具完整覆蓋這條鏈。

再往外延伸（見第五節），EHS 可以從「開發工具」變成**涵蓋產品一生的工具**：開發 → 模擬驗證 → 出廠測試 → 現場設定與維修 → 更新與合規證據，每一段都用同一份專案與單元描述檔。

## 一、六個有機會的方向

| 排名 | 方向 | 為什麼現在有機會 | 缺口在哪 |
| --- | --- | --- | --- |
| 1 | **給中小型設備商的 CRA 合規工具** | CRA 的弱點通報義務已在 [2026-09-11 開始](https://www.telit.com/blog/cra-vulnerability-reporting-september-2026/)，2027-12-11 全面適用。ENISA 調查顯示中小企業「聽過」CRA 和「準備好」之間有落差（見 [TechTarget](https://www.techtarget.com/cybersecurity/feature/EU-Cyber-Resilience-Act-reporting-What-CISOs-need-to-know)） | MCU 的 C 專案沒有套件管理器，函式庫多半直接複製進專案，SBOM 很難自動產生；舊產品連元件清單都沒有。Black Duck 這類工具的價格是給大企業的 |
| 2 | **可驗證的 AI 韌體工具** | 2025 到 2026 年有一批新創拿到資金：[Embedder](https://ycombinator.com/companies/embedder)（YC S25）、[CraftifAI](https://yourstory.com/2026/10/tech30-craftifai-agentic-ai-software-development-physical-devices)（US$3M）、[Embedd](https://www.supplychaindive.com/press-release/20260901-embedd-raises-27m-to-tackle-the-infrastructure-challenge-behind-physical)（US$2.7M 預種子輪，和 Microchip 合作）、[Code Metal](https://www.therobotreport.com/code-metal-secures-seed-funding-llm-agents-edge/)（累計 US$16.45M） | AI 產生的 C 碼在時序與記憶體上不可靠。Black Duck 調查：超過 21% 的公司沒把握擋住 AI 引入的缺陷。值錢的不是「會產生程式碼」，而是「**產生之後能在模擬器或真機上自動驗證**」 |
| 3 | **小團隊負擔得起的 HIL 測試與 CI** | 只有 27% 的團隊用模擬（見 [12](12-mcu-device-simulation.zh-TW.md)）。新產品陸續出現：[BootLoop Test](https://embeddedcomputing.com/technology/debug-and-test/launching-bootloop-test-why-hardware-in-the-loop-testing-stays-out-of-reach-for-most-embedded-teams)（2026 年 6 月推出）、[Linaro 代管服務](https://www.linaro.org/services/testing-automation)、[Lynx 板卡農場](https://www.lynx.com/solutions/embedded-board-farm)；開源的有 [HWaaS](https://cyberus-technology.de/post/announcing-hwaas-open-source-hardware-in-the-loop-test-automation) 與 [Jumpstarter](https://next.redhat.com/project/jumpstarter-hardware-in-the-loop-testing/) | 現有方案偏向 Linux 等級的板子，或要自己架硬體。缺的是「模擬加少量真機」的混合方案，以及沒有專職測試工程師也能設定的方式 |
| 4 | **垂直領域的完整解決方案** | 充電樁（OCPP）、Matter 家電、醫療（IEC 62304）、電池管理都有領域標準與認證要求 | 通用工具不懂領域。客戶願意為「範本、協定堆疊、認證準備」一起付錢；MicroEJ 就是靠電表、手錶這類垂直市場打進去的（見 [05](05-microej-comparison.zh-TW.md)） |
| 5 | **晶片遷移與舊程式現代化** | 缺料時要換晶片、8 位元 MCU 停產、晶片商綁定（[困擾排行](08-mcu-development-pain-points.zh-TW.md)第 6 名） | 很花工時、沒有差異化，適合用「AI 加工具加服務」一起做。Embedd 從 datasheet 產生驅動，做的就是這件事 |
| 6 | **特定市場的現場維運** | Memfault 證明「按台收費的可觀測性」行得通（見 [14](14-mcu-software-monetization.zh-TW.md)） | 一般市場 Memfault 已經佔住；機會在它沒覆蓋的地方，例如用 Modbus、CAN 的工業設備群，或改裝舊設備 |

## 二、機會不大的方向

| 方向 | 理由 |
| --- | --- |
| 編輯器、IDE、單純的 UI 編輯器 | 晶片商免費，LVGL Pro 有免費的 Community 版 |
| 一般的 RTOS 與程式語言 | ThreadX 改為 MIT 授權；Toitware 據報已停止營運（見 [06](06-dynamic-language-mcu-products.zh-TW.md)） |
| 一般的 IoT 雲端與單獨的 OTA | 雲端正在免費化，Memfault 也把 OTA 內含在每個方案裡 |
| 通用的 AI 寫程式助手 | 晶片商和 IDE 廠商一定會把 AI 內建進免費工具；新創也已經很擁擠 |

## 三、和 EHS 的契合度

| 方向 | 契合度 | 理由 |
| --- | --- | --- |
| 可驗證的 AI | **很高** | EHS 已有 AI 助理與 WASM 模擬器，也規劃了匯流排錄製重播。AI 產生單元或規則之後，可以馬上在[整機模擬](12-mcu-device-simulation.zh-TW.md)裡跑測試情境，通過了才推到真機。純 AI 新創大多缺這個「驗證迴圈」 |
| CRA 合規 | **很高** | EHS 掌控整個建置流程，知道專案用了哪些元件與版本，**SBOM 幾乎可以免費產生**。再加上 MCUboot 的簽章更新與更新紀錄，就能交出「安全更新」的證據 |
| HIL 與 CI | 高 | 匯流排錄製檔本身就是測試情境；模擬單元可以直接替換真機單元。可以做成「先在模擬器上跑，再拿少量真機抽驗」 |
| 垂直方案 | 中到高 | EDT 本身就是工業與家電面板的供應商，有現成的客戶場景可以做範本 |
| 晶片遷移 | 中 | 單元匯流排讓應用不綁晶片（見 [09](09-mcu-unit-bus-architecture.zh-TW.md)、[13](13-app-layer-separation-and-languages.zh-TW.md)），但驅動層仍要人做；適合當成服務 |
| 現場維運 | 低 | 交給 Memfault 這類專業服務，EHS 只要做好串接 |

## 四、對收費化路線圖的影響

- [收費化路線圖](10-ehs-paid-product-roadmap.zh-TW.md)第三節的優先 1（錄製重播與整機模擬）、優先 3（OTA 加 SBOM）、優先 5（AI 助理）應該**合成一條賣點**，而不是三個獨立功能：AI 產生的東西要經過模擬驗證，驗證結果和 SBOM、更新紀錄一起成為客戶交得出的證據。
- 優先 3 的 SBOM 不只是附加功能：CRA 通報義務已經開始，這是管理層會撥預算、而且有截止日期的項目。
- 客戶訪談（路線圖第七節）要加問：是否已在準備 CRA、目前怎麼產生 SBOM、有沒有用 AI 寫韌體，以及 AI 產生的程式怎麼驗證。

## 五、補充：其他七個可能性

### 法規還有兩個缺口

| 可能性 | 現況 | 缺口 | 和 EHS 的契合度 |
| --- | --- | --- | --- |
| **歐盟無線設備指令（RED）的資安要求** | 比 CRA 更早生效：所有無線產品從 [2025-08-01 起](https://www.bsigroup.com/en-US/products-and-services/assessment-and-certification/product-testing-certification/radio-equipment-directive-cybersecurity-testing-for-radio-products/)要符合 EN 18031-1/-2/-3；[西班牙政府的指引](https://digital.gob.es/content/dam/portal-mtdfp/avance-digital/telecomunicacion-e-infraestructuras-digitales/areas_interes/equipos-telecomunicacion/documents/Guia-aplicacion-EN-18031.pdf)指出這些標準被引用時附有限制條件 | 有 Wi-Fi 或藍牙的 MCU 產品現在就要交證據，但市面上多半是驗證實驗室的顧問服務，少有給工程師用的檢查工具 | **高**。可和 CRA 做成同一套功能：依專案設定逐項檢查（預設密碼、加密通訊、更新機制），再產生技術文件草稿 |
| **電池數位護照** | [2027-02-18 起強制](https://single-market-economy.ec.europa.eu/batteries_en)，適用電動車電池、輕型交通工具電池，以及容量大於 2 kWh 的工業電池 | 健康度、充放電紀錄等部分資料要由電池管理系統（BMS）的韌體提供 | **低到中**。只有 EDT 有電池或儲能類客戶時才值得做 |

### 生產與出貨後

| 可能性 | 現況 | 缺口 | 和 EHS 的契合度 |
| --- | --- | --- | --- |
| **產線燒錄、金鑰注入與出廠測試** | 晶片商各有服務：[NXP](https://www.nxp.com/applications/technologies/security/trust-provisioning-service-for-secure-manufacturing:TRUST-PROVISIONING)、[Microchip](https://www.microchip.com/en-us/products/security/trust-platform)、[Silicon Labs](https://www.silabs.com/services/custom-part-manufacturing-service?tab=why)；第三方有 [Data I/O](https://dataio.com/Solutions/SentriX)、[SEALSQ](https://sealsq.com/managed-pki-for-iot/secure-factory-provisioning) | 這些服務各自綁定晶片或燒錄機；中小廠的出廠測試程式多半手寫，也沒有序號與測試結果的追溯紀錄 | **高**。EHS 知道整機有哪些單元，可以自動產生出廠測試畫面與流程；EDT 自己的產線就能先用 |
| **現場維修與設定工具** | 技術人員常要用筆電加專用軟體改參數、看故障碼 | 每家廠商都自己寫一套，品質參差 | **很高**。依單元描述檔自動產生手機或網頁的設定與診斷介面，透過藍牙或 Modbus 連線，等於「HMI 的第二個螢幕」，是 EHS 本業的自然延伸 |
| **設備端功能解鎖與授權** | 廠商想要硬體相同、按功能收費 | MCU 上缺少現成且安全的功能授權機制 | **中**。和 OTA、簽章機制一起做；要先確認客戶真有這種商業模式 |

### 知識與人才

| 可能性 | 說明 | 和 EHS 的契合度 |
| --- | --- | --- |
| **給 AI 用的結構化晶片知識** | AI 韌體工具的瓶頸在 datasheet 與暫存器資料的品質；整理並驗證過的周邊與元件模型可以授權（Embedd 走的就是這條路） | **中**。EHS 的單元描述檔累積多了，本身就是這類資料 |
| **教育訓練與認證課程** | 嵌入式人才不足（見 [08](08-mcu-development-pain-points.zh-TW.md)）；課程可帶動工具普及 | **中**。適合當推廣管道，不適合當主要營收 |

### 這次確認機會不大的

- **邊緣 AI 與 TinyML 平台**：最大的獨立平台 Edge Impulse 已被 [Qualcomm 收購](https://www.hackster.io/news/qualcomm-to-acquire-edge-impulse-as-it-seeks-to-gain-ground-in-the-artificial-intelligence-of-things-2db3bb6f0d63)，晶片商都會自己做；EHS 只需能串接模型。
- **功耗量測與最佳化**：主要靠量測硬體，軟體很難單獨收費。

### 補充的重點

- **RED 比 CRA 更急**：CRA 是「快要來」，RED 是「現在就要交證據」。兩者應做成同一套合規功能。
- **現場維修與設定工具**開發成本最低，因為直接重用 EHS 的畫面與單元描述能力。
- 和出廠測試合起來看，同一份專案與單元描述檔可以貫穿開發、模擬、出廠測試、現場維修與更新，這是單點工具做不到的。

## 六、待驗證

- CRA 對 SBOM 的要求，哪些是法定義務、哪些只是實務上需要，各家來源說法不同，要以法規原文與調和標準為準。
- AI 韌體新創的實際功能與客戶群；資金數字只來自新聞稿與報導。
- BootLoop Test 等 HIL 產品的資料有些來自廠商自己撰寫的文章，價格與支援的 MCU 範圍未查證。
- 工業設備群（Modbus、CAN）的現場維運是否真的沒有專門的商業方案。
- RED 的 EN 18031 引用限制條件細節，以及 2025-08 之後的實際執法情況。
- 電池護照的 QR code 要求範圍：CMS 與 Scantrust 寫所有電池都要，比完整護照的適用範圍更廣，要以法規原文為準。
- 現場設定工具、出廠測試產生器、功能解鎖三項是否有既有商業產品，這次沒有查證。
