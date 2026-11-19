# MCU 軟韌體研發的需求與困擾排行

查核日期：2026 年 11 月 19 日。

## 先說明限制

MCU 軟韌體研發**目前沒有一份權威的單一排名**。公開調查大多把嵌入式 Linux 混在一起：[JetBrains 2025](https://lp.jetbrains.com/the-state-of-embedded-2025) 的受訪者有 58% 以 Linux 為主，[Eclipse 2024](https://newsroom.eclipse.org/node/41237) 是 Linux 46%、FreeRTOS 29%。專門的 Zephyr 開發者調查 2026 年才開始做，尚未查到結果。

下表是**綜合多份調查得出的判斷**，不是單一調查的原始排序：

- 唯一有排序的老調查是 AspenCore 2017 年版；2023 與 2025 年版的結果沒找到。
- Memfault 與 Black Duck 是賣相關產品的廠商，調查有立場。
- embedded world 2026 的樣本只有 100 多人。

排名方向可以參考，百分比不宜跨調查直接比較。

## 困擾排行（綜合判斷）

| 排名 | 困擾 | 依據 | MCU 端的具體樣貌 |
| --- | --- | --- | --- |
| 1 | **除錯與找出根因** | AspenCore 2017 年的調查中，除錯與趕時程並列最大挑戰，各約 23%（[CNX](https://cnx-software.com/2017/08/15/aspencore-2017-embedded-markets-study-programming-languages-operating-systems-mcu-vendors-and-more/?amp=1)）；Beningo 估計團隊花 20% 至 40% 的時間在除錯（[Beningo](https://www.beningo.com/the-embedded-frontier-podcast-episode-006-decreasing-debugging-increasing-productivity/)，屬專家估計） | 時序與並行 bug：中斷、RTOS 優先權讓 Task 餓死、堆疊溢位、競態。難以重現，硬體上也看不到發生了什麼 |
| 2 | **複雜度與時程** | embedded world 2026 現場調查，100 多位工程師中「開發複雜度」排第一（[Electronic Specifier](https://www.electronicspecifier.com/?p=185036)）；VDC 2024 調查，軟體約占專案開發成本 60%（[Memfault／VDC](https://go.memfault.com/hubfs/ExecutiveSummaryStateOfIOT.pdf)） | 功能愈加愈多，MCU 資源卻沒變；原本「只是控制器」的產品現在要有 UI、連網、OTA、資安 |
| 3 | **連網** | Eclipse 2024 年調查，連網是第一大顧慮，占 48%（[Eclipse](https://newsroom.eclipse.org/node/41237)） | Wi-Fi、BLE、蜂巢網路的協定堆疊、斷線重連、低功耗，還有和通訊 Task 搶 CPU |
| 4 | **出貨後的問題：現場故障、OTA、可觀測性** | Memfault 2025 年調查（200 家美國 IoT 公司）：38% 一年內發生過召回；77% 認為看不到裝置狀態阻礙了成長；中型公司有 57% 曾因此延後上市（[Memfault](https://memfault.com/wp-content/uploads/2025/08/The-IoT-Visibility-Gap-Report-Memfault.pdf)） | OTA 更新到一半斷電變磚、太晚才加 OTA 只好改版電路板、現場故障無法重現 |
| 5 | **資安與法規** | Eclipse 2024 年調查中資安占 35%，排第二且在上升；歐盟 CRA 2027-12 全面適用 | 安全開機、韌體簽章、金鑰保存、SBOM、弱點處理流程，對小型 MCU 團隊是全新負擔 |
| 6 | **硬體與晶片商綁定、BSP 移植** | 換 MCU 供應商時，已驗證的工具鏈通常要重新驗證（[Embedded.com](https://www.embedded.com/where-developer-choice-broke-down-in-embedded-software-development)）；QNX 2026 年調查顯示車用開發者有 35% 時間花在底層整合（[QNX](https://qnx.software/en/reports/under-the-hood-2026)，車用、非 MCU 專屬） | 缺料或換晶片時，HAL、驅動、工具鏈幾乎要重來 |
| 7 | **測試與「硬體還沒到」** | JetBrains 2025 年調查：54% 會定期測試，但只有 27% 用模擬、22% 用 emulation（[JetBrains](https://lp.jetbrains.com/the-state-of-embedded-2025)） | 軟體團隊等板子、在 PC 上跑不起來、CI 難以自動化 |
| 8 | **人才不足** | UST 調查 200 家嵌入式公司，65% 難以補足 MCU、IoT 等相關職缺（[Electronics Weekly](https://www.electronicsweekly.com/news/why-embedded-engineering-is-becoming-a-bottleneck-for-industrial-automation-2026-10/)） | 會 RTOS、低階除錯與連網的人很少，也很貴 |
| 上升中 | **AI**：邊緣 AI 與 AI 寫程式 | Avnet 調查 1,200 位工程師，57% 優先導入邊緣 AI（[EE Journal](https://www.eejournal.com/industry_news/avnet-insights-engineers-get-behind-ai/)）；Black Duck 2025 年調查：89.3% 的公司已用 AI 寫程式，但超過 21% 沒把握能擋住 AI 引入的缺陷（[Black Duck](https://blackduck.com/content/dam/black-duck/en-us/reports/state-of-embedded-software-quality-and-safety.pdf)） | 模型塞不進 Flash 與 RAM；AI 產生的 C 碼在時序與記憶體上不可靠 |

## 最困擾、最願意花錢解決的三件事

這是判斷，不是調查結果：

1. **並行與時序 bug 難以重現**：花最多時間的地方，硬體上幾乎看不見。
2. **出貨後的事**：OTA 的風險、現場故障、CRA 法規。這直接關係到召回、上市時程和能否賣進歐洲，管理層有預算。
3. **整合的雜事**：BSP、協定堆疊、晶片商工具鏈。花很多工時，但不產生差異化。

## 對 EHS 的意義

| 困擾 | EHS 已有或規劃中的對應 | 判斷 |
| --- | --- | --- |
| 並行 bug | Task 加 Tag Store 模型、設計期衝突偵測、確定性模擬重播（規劃中） | **最高價值**，也是最少工具處理的一層 |
| 硬體還沒到、測試 | WASM 整機模擬器、模擬 Modbus 裝置（已有） | 現成強項，可以接上 CI |
| 協定整合 | Link → Device → Tag，免寫程式（Modbus 與 UART 已有） | 現成強項，要補 MQTT 與 CAN |
| 出貨後與 CRA | OTA 串接與 SBOM 輸出（規劃中） | 管理層願意付費的理由 |
| 晶片綁定 | 經 Zephyr 支援多晶片（規劃中） | 有價值，但工程量大 |
| 現場可觀測性 | 無 | 交給 Memfault 這類專業服務，不要自己做 |
