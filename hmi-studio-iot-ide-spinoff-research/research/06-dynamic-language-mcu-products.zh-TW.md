# 用動態語言開發 MCU 的付費產品

查核日期：2026 年 11 月 19 日。價格多取自廠商頁面，部分頁面沒有日期，引用前請向原廠確認。

## 結論

**幾乎沒有產品是「賣動態語言 IDE 本身」。** 付費點落在三處：

1. 裝置端 runtime 的商業授權或每台權利金。
2. 雲端裝置群管理與 OTA。
3. 綁在硬體或整體平台上。

動態語言在 MCU 上最能收錢的理由，是**出貨後不必重新燒錄就能更新邏輯**，而不是語言本身。

## 一、runtime 要付費（商業授權或權利金）

| 產品 | 語言 | 收費 | 備註 |
| --- | --- | --- | --- |
| [Moddable SDK](https://moddable.com/license) | JavaScript（XS 引擎） | 開源授權（工具 GPLv3，XS 部分 Apache 2.0），另有商業授權：**每台一次性 US$0.50**，量大可議價；須在出貨前取得 | 商業授權涵蓋 XS 引擎、Piu UI 框架、Commodetto 圖形、網路與 TLS；文件 2026 年仍有更新。最接近「付費的動態語言 MCU 開發套件」 |
| [Mongoose OS](https://mongoose-os.com/licensing.html) 與 [mJS](https://github.com/cesanta/mjs/blob/master/README.md)（Cesanta） | JavaScript（mJS） | 社群版用 Apache 2.0、功能受限；企業版付費，按台一次性計價、依數量分級；mJS 是 GPLv2 或商業授權二選一 | 商品化之後，GPL 等於逼你買商業授權 |
| [Zerynth](https://zerynth.com/license) | Python（編譯後在自家 Zerynth OS 上執行） | 開源與閉源元件混合，授權頁 2026-03 更新；有免費方案 | 已轉向工業 IoT 平台；有[評論](https://rkblog.pages.dev/posts/electronics/zerynth-custom-python-implementation-programming-iot-devices)認為會被鎖在它的平台，不適合大量生產 |

## 二、語言開源，付費在雲端與 OTA

| 產品 | 語言 | 收費 |
| --- | --- | --- |
| [Toit](https://toit.io/)（丹麥 Toitware） | Toit（自家動態語言，用於 ESP32） | 語言、VM 與編譯器開源（LGPL）；[商業產品是雲端的裝置群管理與 OTA](https://embeddedcomputing.com/technology/software-and-os/ides-application-programming/product-of-the-week-toit-software-platform)。有[一則來源](https://esp32.co.uk/?p=20261017056)說 Toit 2.0 在 2026-10-31 發布，目前只見到這一個來源。**Toitware ApS 據報已在 2024 年停止營運**，見第五節 |
| [Atym](https://www.zephyrproject.org/ocre-a-tiny-open-source-container-runtime-for-embedded-systems/) | WebAssembly 容器（用 C、C++、Go、Rust 寫） | runtime [Ocre](https://lfedge.org/ocre-comes-to-linux-extended-reach-for-portable-lightweight-containers-powered-by-webassembly/) 以 Apache 2.0 開源，付費的是 Atym Hub 編排平台；價格未公開。不是動態語言，但同樣能在 MCU 上熱部署應用 |

## 三、動態語言包含在付費的平台或硬體裡

| 產品 | 語言 | 說明 |
| --- | --- | --- |
| [MicroEJ](https://www.microej.com/product/sdk/) | 以 Java 為主，也列出 JavaScript | 多年期合約，見 [MicroEJ 比較](05-microej-comparison.zh-TW.md) |
| [Crank Storyboard](https://www.cranksoftware.com/products/pricing) | Lua | 付費 UI 工具，應用邏輯用 Lua 寫 |
| [Digi XBee 3](https://www.digi.com/blog/post/employing-wireless-edge-intelligence-with-digi-xbe) | MicroPython | 內建在付費的無線模組裡，另有 PyCharm 外掛；錢從模組收 |
| [OpenMV](https://openmv.io/pages/partners) | MicroPython | 部分驅動與函式庫是專有的，只限非商業使用；商用要另外洽談授權 |

## 四、免費或已停止

- **免費開源**：MicroPython 靠[贊助](https://fr.github.com/sponsors/micropython)維持，沒找到專門賣它支援合約的公司；CircuitPython、NodeMCU 的 Lua、Tasmota 的 Berry 也都免費。Espruino 的商業條款這次沒查到。
- **已停止**：Microsoft 的 TypeScript 方案 DeviceScript 是研究專案，[repo 已在 2025-08-13 封存](https://github.com/microsoft/jacscript)。

## 五、深入：Toit 與 Jaguar

### 為什麼要另外發明 Toit

Toitware 2017 至 2018 年成立於丹麥奧胡斯，核心成員包括 Lars Bak 與 Kasper Lund。兩人在 Google 做過 Chrome 的 V8 JavaScript 引擎，也共同設計了 Dart（見 [HandWiki](https://handwiki.org/wiki/Toit)、[eeNews](https://www.eenewseurope.com/en/toit-opens-up-its-iot-programming-language/)）。Lund 說，成立公司的目的是讓人能**安全地**把程式碼部署到連網的嵌入式裝置上（見 [Instantiations](https://www.instantiations.com/news/bridging-the-gap-with-toit-a-rich-dev-experience-for-microcontrollers/)、[‹Programming› 2019 主題演講](https://2019.programming-conference.org/event/programming-2019-keynotes-keynote-turning-iot-devices-into-robust-and-safe-computers)）。

他們要解決的是 C 和 MicroPython 各自的缺點：

| 問題 | C | MicroPython | Toit 的做法 |
| --- | --- | --- | --- |
| 記憶體安全 | 野指標、溢位能弄壞整台設備 | 安全 | 安全：記憶體由系統管理，加上垃圾回收 |
| 一個應用出錯，會不會拖垮整台設備 | 會 | 通常整個直譯器一起掛 | 不會：每個應用跑在自己的容器裡，有獨立的記憶體（見 [EOC 2021 演講](https://embeddedonlineconference.com/session/Containers_and_managed_software_on_micro-controllers)） |
| 速度 | 最快 | 慢 | 廠商宣稱在 ESP32 上[比 MicroPython 快約 30 倍](https://www.cnx-software.com/2021/11/28/toit-open-source-language-claims-to-be-30x-faster-than-micropython-on-esp32/?amp=1)，是廠商自己的數字 |
| 更新程式 | 重新燒錄整份韌體 | 換 `.py` 檔 | 單獨安裝或更新某個容器，系統其他部分不動 |
| 學習成本 | 高 | 低 | 低：抽象掉記憶體配置，Lund 說 Python 開發者幾小時就能上手 |

簡單說，Toit 把瀏覽器「多個分頁互相隔離、一頁當掉不影響其他頁」的 VM 思路，縮小搬到 ESP32 上。現有語言做不到「快、安全、可隔離、可單獨更新」四件事同時成立，所以他們自己做了一個。語言在 2021 年 11 月以 LGPL 開源。

### Jaguar 是什麼

[Jaguar](https://github.com/toitlang/jaguar/) 是 Toit 的開源開發工具，負責即改即跑的開發迴圈：

1. **先燒一次**：用 USB 執行 `jag flash`，把 Toit 執行環境和 Jaguar 寫進 ESP32，之後不必再接線燒錄。
2. **裝置端**：Jaguar 本身是跑在 ESP32 上的 Toit 程式，透過 Wi-Fi 等待電腦傳來新程式碼。
3. **電腦端**：`jag run app.toit` 把程式編譯成 bytecode，透過 Wi-Fi 推到裝置執行；`jag watch` 在存檔時自動重推，[官方說不到 2 秒](https://docs.toit.io/getstarted)。
4. **為什麼不會推壞**：使用者的程式跑在另一個容器裡，當掉時 Jaguar 的容器還活著，可以馬上再推一次。這是容器隔離帶來的好處。
5. **兩種部署**：`jag run` 是暫時執行，重開機就消失；`jag container install` 會安裝成具名容器，開機自動啟動。

所以 [熱重載研究](11-mcu-hot-reload.zh-TW.md)說 Jaguar 是最接近真正熱重載的工具。它做得到，是因為底層有 VM 和容器隔離；換成 C 就做不到。

### 公司已經停止營運

[HandWiki 的 Toit 條目](https://handwiki.org/wiki/Toit)寫道：Toitware ApS 依丹麥商業登記，已在 2024 年停止營運；2025 年起，專案由兩位前員工維護。

- 這是維基類來源，不是官方公告，丹麥商業登記沒有直接查證。
- 若屬實，第二節「語言開源、付費在雲端與 OTA」的模式**沒能撐住公司**。
- 第二節提到的 Toit 2.0 只有單一來源，和「兩人維護」不矛盾，但說明 Toit 現在是社群專案，不是有廠商支援的商業產品。

### 啟示

- **技術上值得學**：容器隔離加上無線即改即跑，正是 [13 號研究](13-app-layer-separation-and-languages.zh-TW.md)建議用 WebAssembly 達成的事。
- **商業上是警訊**：只賣「更好的語言與開發體驗」再搭配雲端 OTA，連 V8 與 Dart 的原班人馬都沒能撐起一家公司。這支持 [收費化路線圖](10-ehs-paid-product-roadmap.zh-TW.md)的判斷：錢要收在「整台設備做得出來、出貨後管得住」，不是收在語言本身。

## 對 EHS 的意義

主報告建議後期評估裝置端腳本層，讓 UI 與 Logic 不必重新燒錄就能更新。這幾家的做法可作為收費模式參考：

- **Moddable 的每台 US$0.50**：可行的量產權利金基準，比 LVGL Pro 每產品 US$20,000 更適合中小型 OEM。
- **Toit 的開源語言加付費 OTA 雲端**：動態語言最能收錢的地方是出貨後的遠端更新與裝置群管理，和 Bitdove OTA 的方向一致。但 Toitware 據報已在 2024 年停止營運（見第五節），單靠語言加雲端不足以撐起公司。
- **Mongoose 的 GPL 加商業授權**：開源可以換來採用度，但這種雙授權常被視為「逼人付費」，口碑有風險。
