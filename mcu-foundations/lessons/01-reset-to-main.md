# 第二課 從 reset 走到 main

這一課約 4 至 6 小時，分成桌機實驗與上板實作。沒有板子可以先完成桌機部分。讀完應該能解釋：上電之後第一條指令從哪裡來，全域變數的初值又是誰放進 SRAM 的。

## 這一課要回答的四個問題

| 問題 | 機制 | 證據去哪裡找 |
| --- | --- | --- |
| 誰決定第一條指令在哪 | 向量表與 reset 流程 | core manual、啟動碼、debugger 的暫存器畫面 |
| 誰決定變數放在哪 | linker script 的記憶體區域與區段規則 | linker script、map file |
| 誰把初值搬進 SRAM | 啟動碼 | startup 原始碼、逐步執行 |
| 失敗時看起來像什麼 | 讀到殘留值、fault、數值錯位 | 冷開機與按 reset 各觀察一次 |

## 建置產物各自是什麼

| 產物 | 內容 | 從中要找什麼 |
| --- | --- | --- |
| object file | 每個 .c 的機器碼與尚未定址的區段 | 這個檔案定義了哪些符號、還缺哪些 |
| linker script | 記憶體區域與區段擺放規則 | Flash 與 SRAM 的起點和大小；.data 的兩個地址 |
| ELF | 定址後的程式、符號與除錯資訊 | debugger 用它把地址對回原始碼 |
| map file | 每個區段與符號的最終地址和大小 | 某個變數在哪裡、哪個檔案佔最多空間 |
| bin 或 hex | 只有要寫進 Flash 的位元組 | 燒錄工具寫入的內容，沒有符號 |

「從乾淨輸出目錄重建」是為了證明結果只來自原始碼與記錄下來的命令，而不是某個殘留的 object file。刪掉輸出目錄、重建、保存 build log 與工具鏈版本，之後出問題才有基準可比。

## Cortex-M 的 reset：硬體只做兩次讀取

以 Cortex-M 為例，處理器在 reset 時從地址 0x00000000 讀出初始 main stack pointer，再從 0x00000004 讀出 reset vector 作為 PC；這個值的 bit 0 必須是 1，表示 Thumb 狀態。向量表中每個 handler 地址的最低位元也都必須是 1。[Cortex-M4 Generic User Guide](https://support.arm.com/documentation/dui0553/latest/)

```text
偏移     內容
0x0000   初始 SP，通常是 SRAM 的結尾
0x0004   Reset_Handler 的地址 | 1
0x0008   NMI_Handler
0x000C   HardFault_Handler
...      其他系統例外
0x0040   第一個廠商中斷 IRQ0，之後每個中斷 4 bytes
```

reset 時向量表固定在 0x00000000；之後軟體可以寫 VTOR 把它移到別處。VTOR 在 Cortex-M0+ 是選配，在 Cortex-M0 上沒有，向量表永遠在 0x00000000。[Cortex-M0+ Generic User Guide](https://support.arm.com/documentation/dui0662/latest/)、[Cortex-M0 Generic User Guide](https://support.arm.com/documentation/dui0497/latest/)

架構只規定處理器讀哪裡。地址 0 實際對應到 Flash、系統記憶體或 SRAM，以及在處理器讀向量表之前是否有 boot ROM 先執行，都由晶片廠商決定，常和 boot 接腳或選項位元組有關，要查該晶片 reference manual 的 boot 章節。

## 啟動碼：把 C 的承諾變成事實

C 語言承諾：有初值的靜態儲存期物件在 main 之前已經有初值，沒寫初值的則是 0。桌機上由作業系統的載入器完成這件事；MCU 上沒有載入器，必須由啟動碼來做。

```text
Reset_Handler：
    必要時設定 stack
    SystemInit：時脈等最早期的硬體設定
    把 .data 的初值從 Flash 複製到 SRAM
    把 .bss 清零
    C 函式庫與 C++ 建構式的初始化
    呼叫 main
    main 若返回：進入無窮迴圈
```

CMSIS 風格的啟動碼由 Reset_Handler 先呼叫 SystemInit，再交給 C runtime 做初始化並呼叫 main。[CMSIS-Core Startup File](https://arm-software.github.io/CMSIS_6/latest/Core/startup_c_pg.html) 這代表 SystemInit 執行時 .data 還沒搬、.bss 還沒清，它不能依賴任何全域變數的初值。各家啟動碼的順序與符號名稱可能不同，以你專案裡的那一份為準。

有些專案會刻意保留一塊不清零的 SRAM，常命名為 .noinit，用來在 warm reset 之後留下重啟原因或故障紀錄。內容能不能保留取決於 reset 種類與晶片；讀取前要用 magic 值與檢查碼確認可信。第七階段的 watchdog 會用到這個機制。

## linker script 的兩個地址

GNU ld 的文件把每個輸出區段的地址分成兩種：VMA 是執行時的地址，LMA 是被載入的地址。.data 正是兩者不同的例子：初值存在 ROM（LMA），開機時再複製到 RAM（VMA）。[GNU ld Basic Script Concepts](https://sourceware.org/binutils/docs/ld/Basic-Script-Concepts.html)

```text
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}
SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
  .text       : { *(.text*) *(.rodata*) } > FLASH
  .data       : { _sdata = .; *(.data*) _edata = .; } > RAM AT > FLASH
  _sidata = LOADADDR(.data);
  .bss        : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

這是教學用的簡化版：語法已用 GNU ld 解析確認，但省略了對齊、C++ 與除錯區段，不能直接拿去連結真實專案。`> RAM AT > FLASH` 就是「VMA 在 RAM、LMA 在 Flash」；啟動碼用 `_sidata`、`_sdata`、`_edata` 這類符號知道要從哪裡搬到哪裡。符號名稱各家不同，重點是找到它們在啟動碼和 linker script 之間的對應。

`size` 會把用量分成 text、data、bss 三欄。Flash 用量約等於 text 加 data，因為 data 的初值也要存在 Flash；SRAM 用量約等於 data 加 bss，再加上 stack 與 heap。stack 和 heap 是否算在 bss 裡，要看 linker script 怎麼保留它們。

## 桌機實驗 A：reset 模擬器

[labs/01-reset-to-main/boot_sim.c](../labs/01-reset-to-main/boot_sim.c) 用兩個陣列代表 Flash 和 SRAM，模擬「硬體讀兩個 word、啟動碼搬資料、main 讀變數」。它不執行機器碼，只模擬誰在什麼時候寫了哪塊記憶體。

```sh
cd labs/01-reset-to-main
gcc -std=c11 -Wall -Wextra -Werror -O2 -o boot_sim boot_sim.c
./boot_sim
./boot_sim --test
```

先讀 `./boot_sim` 印出的模擬 map 與啟動追蹤，再對每一種故障寫下預測，然後執行 `./boot_sim --fault=名稱` 對照：

| 故障 | 你的預測 | 實際觀察 |
| --- | --- | --- |
| skip-data-copy | | |
| skip-bss-zero | | |
| wrong-lma | | |
| thumb-bit | | |
| sp-outside-ram | | |

特別注意 warm reset 那一段：SystemInit 讀到的 g_sample_period_ms 剛好是 10，看起來像已初始化，其實只是上一輪留在 SRAM 的殘留值。冷開機時同一個位置是亂數。只在冷開機出錯、按 reset 就正常的問題，常常就是這類「依賴殘留值」。

## 桌機實驗 B：讀 map file 與符號表

[labs/01-reset-to-main/globals.c](../labs/01-reset-to-main/globals.c) 有五個不同儲存方式的變數。先在原始碼註解寫下預測，再建置並觀察：

```sh
gcc -std=c11 -Wall -Wextra -Werror -O0 -Wl,-Map=globals.map -o globals globals.c
./globals
nm globals.exe
size globals.exe
```

Linux 上執行檔沒有 .exe。PowerShell 可用 `nm globals.exe | Select-String "g_|s_|k_"` 過濾，Git Bash 可用 `grep -E`。在 Windows 11、GCC 16.2 MinGW-w64 上得到：

```text
0000000140009080 B g_sample_count
0000000140004010 D g_sample_period_ms
0000000140009084 B g_zero_init
0000000140005050 R k_fw_name
00000001400091a0 b s_calls.0
00000001400090a0 b s_rx_buffer
```

D 是 .data、B 是 .bss、R 是唯讀資料，小寫表示只在檔案內可見。`g_zero_init = 0` 寫了初值卻在 .bss：GCC 預設把初值為 0 的物件放進 .bss，因為清零比從 Flash 複製便宜。static 變數在 nm 看得到，在 map 的符號清單裡卻找不到，這是這個 linker 的輸出方式；找 static 變數時改用 nm 或 debugger。

程式自己印出的地址和 nm 差了一個固定偏移，因為 Windows 載入器把程式放到另一個基底地址。MCU 沒有載入器，map 裡的地址就是執行時的地址；這也是 debugger 能直接依 map 讀變數的原因。

## 上板實作

先依 [選型與文件閱讀](../03-selection-and-documents.md) 準備好電路圖、reference manual 與 debugger。這一關只需要官方 blink 或空專案。

1. 記錄板子型號、完整晶片料號、工具鏈、SDK 與 debugger 版本。
2. 刪除輸出目錄後重建，保存完整 build log，確認產出 ELF、map 與燒錄檔。
3. 在 map 中找到向量表起點、Reset_Handler 的地址、一個 .data 變數的 VMA 與 LMA、一個 .bss 變數，以及初始 SP 的符號。
4. 讓 debugger 在 reset 後立刻停住，不要先跑到 main。讀向量表起點的前兩個 word，和 SP、PC 暫存器以及 map 對照；第二個 word 應該是 Reset_Handler 地址加 1。
5. 逐步執行 Reset_Handler。在記憶體視窗盯著那個 .data 變數的地址，看它在複製迴圈前後的變化；.bss 變數也一樣。再讀 Flash 上 LMA 處的內容，確認那就是初值。
6. 在 SystemInit 設斷點，讀同一個 .data 變數。冷開機（拔電）與按 reset 各做一次，比較讀到的值。
7. 斷在 main，記錄 call stack 和此時的 SP。

故障注入選可恢復的做法：在 .bss 清零迴圈之前，用 debugger 把某個 .bss 變數寫成 0x12345678，看它被清掉；或暫時把清零迴圈註解掉，比較冷開機和按 reset 時的計數初值。不要拿修改 Flash 裡的向量表做實驗，除非已確認 debugger 能在 reset 狀態下連線並抹除 Flash。

| 驗收條件 | 要留下的證據 |
| --- | --- |
| 從乾淨輸出目錄重建 | 刪除輸出目錄後的完整 build log、工具鏈版本 |
| 斷在 main | PC 位於 main、call stack 從 Reset_Handler 而來 |
| 找到全域變數地址與初始化來源 | map 中的 VMA 與 LMA，以及 debugger 在 LMA 讀到的初值 |
| 說明 reset 後的前兩次讀取 | 向量表前兩個 word 與 SP、PC 暫存器的對照 |

## 先自行回答

1. `uint8_t rx_buf[512];` 佔 Flash 還是 SRAM？各多少？
2. `const uint16_t table[256] = {...};` 在 Cortex-M 的 GCC 專案裡，佔 Flash 還是 SRAM？
3. `uint32_t g_config = 7;` 各佔多少 Flash 與 SRAM？
4. 燒錄後由 debugger 啟動一切正常，拔電再上電後某個計數器起始值是亂數。最可能缺了哪一步？
5. map 顯示 .data 的 VMA 是 0x20000000、大小 0x40，LMA 是 0x00003F00；燒錄檔卻只到 0x00003F00 為止。開機後全域變數的初值會是什麼？

### 解析

1. 它在 .bss，只佔 512 bytes SRAM，不佔 Flash 空間；開機時由清零迴圈處理。
2. 放在唯讀資料區，位於 Flash，佔 512 bytes，不佔 SRAM。換到 AVR 這類 Harvard 架構時，const 資料可能被複製到 RAM，除非使用特別的屬性；這正是換平台時要重查的假設。
3. 4 bytes Flash 存初值，4 bytes SRAM 存執行時的值。
4. 變數依賴了未初始化的內容：.bss 沒有清零、變數被放進不清零的區段，或在初始化前就被讀取。debugger 燒錄後的 SRAM 常保留著前一次的值，冷開機才露出問題。
5. 燒錄檔少了 .data 的初值，複製迴圈從 LMA 讀到的是抹除後的 Flash，常見是每個 byte 都是 0xFF。檢查轉出燒錄檔的命令有沒有包含 .data 的載入區段。

## 今日交付

在 [學習紀錄](../learning-log.md) 留下：向量表前兩個 word、Reset_Handler 地址、一個 .data 變數的 VMA 與 LMA、`size` 輸出，以及五個故障的預測與觀察。

最後用自己的話寫出「硬體做的事」與「啟動碼做的事」的分界，並列出換到另一個平台時要重查的項目：Flash 起點與 boot 對應、VTOR 是否存在、啟動碼的初始化順序、linker 符號名稱。下一課從這裡往外走一步，追一支 GPIO 從 API 到接腳。
