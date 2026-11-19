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
| [Toit](https://toit.io/)（丹麥 Toitware） | Toit（自家動態語言，用於 ESP32） | 語言、VM 與編譯器開源（LGPL）；[商業產品是雲端的裝置群管理與 OTA](https://embeddedcomputing.com/technology/software-and-os/ides-application-programming/product-of-the-week-toit-software-platform)。有[一則來源](https://esp32.co.uk/?p=20261017056)說 Toit 2.0 在 2026-10-31 發布，目前只見到這一個來源 |
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

## 對 EHS 的意義

主報告建議後期評估裝置端腳本層，讓 UI 與 Logic 不必重新燒錄就能更新。這幾家的做法可作為收費模式參考：

- **Moddable 的每台 US$0.50**：可行的量產權利金基準，比 LVGL Pro 每產品 US$20,000 更適合中小型 OEM。
- **Toit 的開源語言加付費 OTA 雲端**：動態語言最能收錢的地方是出貨後的遠端更新與裝置群管理，和 Bitdove OTA 的方向一致。
- **Mongoose 的 GPL 加商業授權**：開源可以換來採用度，但這種雙授權常被視為「逼人付費」，口碑有風險。
