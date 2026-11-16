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

## 第六課與第七課新增

查閱日期：2026-11-16。FreeRTOS Kernel Book 與 FreeRTOS-Kernel 標頭檔取自 GitHub main 分支；Zephyr 為 latest 頁面，實作時改用專案採用的版本。

| 來源 | 支援的學習內容 | 使用邊界 |
| --- | --- | --- |
| [Zephyr DMA API](https://docs.zephyrproject.org/latest/doxygen/html/group__dma__interface.html) | 半滿與完成的 callback 狀態、循環模式下 callback 的意義 | Zephyr 的抽象；硬體事件與旗標查晶片 reference manual |
| [Zephyr UART async API](https://docs.zephyrproject.org/latest/doxygen/html/group__uart__async.html) | 接收 buffer 的請求與釋放、沒及時提供下一塊 buffer 時停止接收、收到資料後的閒置逾時 | 同上；IDLE 偵測是否存在查晶片手冊 |
| [FreeRTOS Kernel Book 第 4 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md) | task 優先權數字越大越優先、idle 為 0、stack 深度以 word 計 | 以專案使用的 kernel 版本為準 |
| [FreeRTOS Kernel Book 第 7 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch07.md) | ISR 只用 FromISR API、不要求切換時最差等到下一個 tick、configMAX_SYSCALL_INTERRUPT_PRIORITY、Cortex-M 優先權數字越小越高 | 同上 |
| [FreeRTOS Kernel Book 第 8 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch08.md) | 優先權反轉、繼承只讓反轉有時間上限、mutex 有繼承而 binary semaphore 沒有、mutex 不可用於 ISR、gatekeeper task | 同上 |
| [FreeRTOS task.h](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/include/task.h) | uxTaskGetStackHighWaterMark 以 word 為單位、xTaskDelayUntil | main 分支 |
| [FreeRTOS semphr.h](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/include/semphr.h) | mutex 不能在 ISR 中使用 | 同上 |
| [Zephyr Interrupts](https://docs.zephyrproject.org/latest/kernel/services/interrupts.html) | 耗時或阻塞的工作交給 thread、k_is_in_isr、中斷有自己的 stack | latest 頁面 |
| [Zephyr API 術語](https://docs.zephyrproject.org/latest/develop/api/terminology.html) | 可能睡眠的函式只有在 no-wait 模式下才能從 ISR 呼叫 | 同上 |
| [Zephyr Mutexes](https://docs.zephyrproject.org/latest/kernel/services/synchronization/mutexes.html) | 優先權繼承、解鎖時交給等待最久的最高優先權 thread、不供 ISR 使用 | 同上 |
| [Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) | 優先權數字越小越優先、負數為 cooperative、k_thread_stack_space_get 回報剩餘 bytes | 同上 |

第六課的 HT、TC、IDLE 等事件名稱、ADC overrun 後的行為、cache 與 DMA 可存取的記憶體範圍依晶片而定，本課程沒有逐條核對特定廠商手冊。第七課 -fstack-usage 的數字是 Windows 上 x86-64 GCC 16.2 的結果，不代表 Cortex-M。

## 第八課與第九課新增

查閱日期：2026-11-16。Arm 使用手冊以先前從 PDF 擷取的文字核對，版本與第二課到第五課相同（DUI 0553A、DUI 0662B、DUI 0497A）；N1570 同上。

| 來源 | 支援的學習內容 | 使用邊界 |
| --- | --- | --- |
| [Arm Cortex-M4 Devices Generic User Guide](https://support.arm.com/documentation/dui0553/latest/) | WFI、WFE、SLEEPONEXIT、SCR 的 SLEEPDEEP；休眠模式由晶片實作決定、debug 操作可能喚醒處理器；AIRCR 的 VECTKEY 0x5FA 與 SYSRESETREQ | 只描述處理器核心；休眠模式、喚醒來源與重啟範圍要查晶片文件 |
| [Arm Cortex-M0+ Devices Generic User Guide](https://support.arm.com/documentation/dui0662/latest/) | 不支援非對齊存取、指令集沒有 SDIV 與 UDIV | 同上 |
| [Arm Cortex-M0 Devices Generic User Guide](https://support.arm.com/documentation/dui0497/latest/) | 不支援非對齊存取、指令集沒有 SDIV 與 UDIV | 同上 |
| [C11 委員會草案 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) | signed 左移超出範圍是未定義行為（6.5.7）、int 的最小範圍（5.2.4.2.1） | 2011-04-12 的最終草案，不是正式出版的標準本文 |

第八課關於 watchdog 的種類與限制、重啟原因旗標的累積與清除、Flash 的抹寫規則與抹除時 CPU 是否停住、休眠時 watchdog 與 tick 的行為，都標示為依晶片而定，沒有逐條核對任何廠商文件；第九課的 RISC-V 說明也是。兩課的桌機實驗只驗證邏輯，Flash 斷電後的位元狀態、抹除時間、watchdog 實際逾時與真實 HAL 的 callback 時機尚未上板驗證。

選定第一塊開發板後，再補上該板的電路圖、晶片 datasheet、reference manual、errata、SDK 與 debugger 文件。課程階段、時數與專案規模是教學安排；資料率計算使用第一課明列的假設。
