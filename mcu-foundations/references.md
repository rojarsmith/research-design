# MCU 官方文件入口

查閱日期：2026-10-18。以下入口用於理解架構與工具設計，不構成品牌或產品推薦。實作時應固定使用專案所採版本的文件；latest 頁面會隨時間變動。

| 來源 | 支援的學習內容 | 使用邊界 |
| --- | --- | --- |
| [Arm CMSIS-Core Overview](https://arm-software.github.io/CMSIS_5/Core/html/) | Cortex-M 核心介面、初始化慣例與核心文件入口 | 此入口是 CMSIS 5 文件；不能替代實際晶片的周邊手冊，也不適用所有 MCU 架構 |
| [Zephyr Devicetree](https://docs.zephyrproject.org/latest/build/dts/index.html) | 硬體描述及初始設定的用途 | 屬於 Zephyr 的具體實作，不是所有 SDK 的共同格式 |
| [Zephyr Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html) | 板級硬體與接腳配置、board port 的責任 | 實作時須對照採用的 Zephyr 版本 |

## 第二課到第五課新增

查閱日期：2026-11-15。Arm 的使用手冊以 PDF 核對，版本分別是 DUI 0553A、DUI 0662B、DUI 0497A；latest 頁面可能已有較新的版本。

| 來源 | 支援的學習內容 | 使用邊界 |
| --- | --- | --- |
| [Arm Cortex-M4 Devices Generic User Guide](https://support.arm.com/documentation/dui0553/latest/) | reset 時載入 MSP 與 PC、向量表與 Thumb 位元、VTOR、例外優先權、進入例外時推入的暫存器、NVIC 的 pending 與 level、pulse 中斷、SysTick、bit-band | 只描述處理器核心；晶片的周邊、時脈與 boot 對應要查廠商文件 |
| [Arm Cortex-M0+ Devices Generic User Guide](https://support.arm.com/documentation/dui0662/latest/) | VTOR 與 SysTick 為選配、優先權只實作 bits[7:6]、指令集沒有 LDREX 與 STREX | 選配功能是否實作，要查晶片文件 |
| [Arm Cortex-M0 Devices Generic User Guide](https://support.arm.com/documentation/dui0497/latest/) | 向量表固定在 0x00000000、系統控制區沒有 VTOR、SysTick 為選配 | 同上 |
| [CMSIS-Core Startup File](https://arm-software.github.io/CMSIS_6/latest/Core/startup_c_pg.html) | Reset_Handler 先呼叫 SystemInit 再交給 C runtime、弱定義的預設 handler | CMSIS 6 的範本；廠商啟動碼的順序與符號名稱可能不同 |
| [Open-CMSIS-SVD](https://open-cmsis-pack.github.io/svd-spec/main/index.html) | debugger 周邊暫存器畫面與裝置標頭檔的來源 | SVD 檔由晶片廠商維護，與 reference manual 不一致時以手冊與實測為準 |
| [GNU ld Basic Script Concepts](https://sourceware.org/binutils/docs/ld/Basic-Script-Concepts.html) | VMA 與 LMA，.data 從 ROM 複製到 RAM | latest 文件；以專案使用的 binutils 版本為準 |
| [GNU ld Command-line Options](https://sourceware.org/binutils/docs/ld/Options.html) | `-Map` 產生 map file | 同上 |
| [AAPCS32](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst) | 函式必須保存的暫存器、stack 的 8-byte 對齊 | 32 位元 Arm 的呼叫慣例，不適用其他架構 |
| [C11 委員會草案 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) | unsigned 運算的模數語意（6.2.5）、`<stdatomic.h>` 與記憶體順序（7.17） | 2011-04-12 的最終草案，不是正式出版的標準本文 |
| [Keil ULINKplus 與 Cortex-M0/M0+/M1/M23](https://www.keil.com/support/docs/3999.htm) | Cortex-M0、M0+ 沒有 DWT 週期計數器 | 工具廠商的支援說明；需要確切暫存器定義時查 Armv6-M 架構手冊 |

課文中標示「要查手冊」或「依晶片而定」的說法，是因晶片而異、或本課程沒有逐條核對官方文件的內容，例如 boot 時地址 0 的對應、沒開時脈時存取周邊的反應、UART 錯誤旗標的清除方式、各核心對哪些存取保證一次完成。第五課的 serial.ps1 尚未在實機上驗證。

選定第一塊開發板後，再補上該板的電路圖、晶片 datasheet、reference manual、errata、SDK 與 debugger 文件。課程階段、時數與專案規模是教學安排；資料率計算使用第一課明列的假設。
