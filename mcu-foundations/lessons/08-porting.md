# 第九課 換平台移植

這一課約 6 至 12 小時，可以分兩到三次完成。資料採集器在第一塊板子上已經完整；這一課把它搬到第二個平台，檢驗前八課學到的東西有哪些真的能帶走。讀完應該能解釋：為什麼電腦端的協定測試一個字都不用改；為什麼移植時改了大量應用邏輯，通常代表邊界畫錯了。

## 移植要證明什麼

| 要證明的事 | 怎麼證明 |
| --- | --- |
| 協定沒有變 | 電腦端的 rx_check、serial.ps1 與協定測試原封不動，在新平台上照樣通過 |
| 應用核心不依賴平台 | 只給核心自己的 include 路徑，它仍然能單獨編譯；兩個平台的核心原始碼相同 |
| 改動都在該改的層 | 每一層的改動列成表，每一項都說得出原因 |
| 新平台的時間行為符合需求 | 第四課到第八課的量測在新平台上重做一次 |

最後一項只能上板證明，桌機實驗證明的是前三項的邏輯部分。

## 選第二個平台

依 [選型與文件閱讀](../03-selection-and-documents.md) 的方法縮小範圍。第二個平台有兩種方向：

- 同一種核心、另一家廠商，例如兩家都是 Cortex-M。例外模型與 SysTick 大致相同，差異集中在周邊、時脈樹、SDK 與工具。適合第一次移植，一次只換一組變數。
- 另一種核心，例如 RISC-V。中斷控制器、啟動碼、工具鏈與呼叫慣例都換了，適合想進一步驗證核心支援層也能替換的時候；中斷與 timer 的細節要查該核心與晶片的文件，不能從 Cortex-M 推論。

拿到新板子先做 03 的「一小時分類」：核心與記憶體配置、LED 與有效電位、blink 專案裡的 startup、linker、BSP、driver 與應用檔案，追一個 GPIO 呼叫，列出三件還不確定的事。

## 分層與窄介面

| 層 | 內容 | 移植時 |
| --- | --- | --- |
| 應用 | 取樣與緩衝策略、協定、命令、監督規則、設定格式 | 不改；必須改時先懷疑邊界 |
| 平台介面 | 應用向平台要的函式，以及平台呼叫應用的入口 | 不改；它就是邊界 |
| BSP | 這塊板子的選擇：哪個 UART 接到 USB、感測器在哪條匯流排、LED 的有效電位、用哪個時脈 | 重寫 |
| driver | 把平台介面接到廠商的暫存器或 HAL：時間、UART、感測器、Flash、watchdog | 重寫 |
| 啟動與核心支援 | 向量表、Reset_Handler、SystemInit、中斷 handler 的名稱；RTOS 的移植層 | 換成新平台的 |
| build | 工具鏈、編譯旗標、include 路徑、linker script、燒錄檔轉換 | 換成新平台的 |

實驗裡的平台介面只有八個入口，型別只用 `<stdint.h>` 與 `<stdbool.h>`：

```c
typedef struct {
    uint32_t (*now_ms)(void);           /* 32 位元毫秒，允許回繞 */
    void (*tx_kick)(void);              /* TX 緩衝區有資料了 */
    void (*sensor_start)(void);         /* 開始一次量測，不等待 */
    int (*sensor_poll)(int16_t *value); /* 完成回傳 1，不等待 */
} platform_ops_t;

void app_init(const platform_ops_t *ops);
void app_poll(void);                              /* 主迴圈每圈一次 */
bool app_uart_rx(uint8_t byte);                   /* 中斷或 callback 裡呼叫 */
uint32_t app_uart_tx(uint8_t *dst, uint32_t max); /* 同一時間只能有一處在取 */
```

讓廠商型別與 callback 留在邊界外：

- 介面上不出現 handle、狀態碼、廠商的 typedef 或中斷向量名稱；廠商的錯誤碼由 BSP 轉成應用層的計數或狀態。
- callback 在中斷脈絡執行。BSP 在 callback 裡只把事件轉成應用看得懂的形式：放進 SPSC 緩衝區、設旗標、增加計數，不呼叫應用的邏輯，也不做會等待的事。
- 時間由平台換算成 32 位元毫秒，計數器的寬度、頻率與回繞週期不讓應用知道。
- 逾時、重試與策略屬於應用；收尾屬於 BSP，例如逾時後 abort 進行中的傳輸、重新開始接收。
- buffer 的所有權寫進介面說明：誰在什麼時候可以讀寫。第六課的 DMA 通道、請求來源、對齊與可存取的記憶體區域都屬於 driver。如果你在第七課加了 RTOS，它的移植層與中斷優先權上限屬於核心支援層，應用的 task 與 queue 不應該因為換平台而改。

第五課的介面在移植時露出一個缺口：板級程式要知道 TX 有沒有資料，只能直接讀 `logger_t` 的欄位。實驗把這件事收進應用層的 app.c，平台只收到 `tx_kick`；logger.c 本身沒有改。

## 換平台清單

| 項目 | 會變的東西 | 去哪裡查 | 怎麼驗證 |
| --- | --- | --- | --- |
| 啟動碼 | 向量表與 handler 名稱、初始 SP、SystemInit、.data 與 .bss 的初始化、.noinit | 新平台的 startup 檔、core manual | 第二課：斷在 main，找到 .data 的 VMA 與 LMA |
| linker script | Flash 與 SRAM 的起點與大小、設定頁與 .noinit 的位置、stack 與 heap | reference manual 的記憶體配置、datasheet | map file：設定頁不在程式區內 |
| 時脈樹 | 振盪器、PLL、匯流排分頻；UART 的 baud 除數與 timer 的 reload 都由它決定 | reference manual 的時脈章節與時脈樹圖 | 第三課：獨立時間基準；UART 收到正確字元 |
| 接腳複用 | UART、LED、按鈕、感測器接哪支腳、替代功能編號、有效電位、除錯接腳 | 電路圖、datasheet 的接腳表 | 第三課：用 debugger 直接改暫存器 |
| 中斷 | handler 名稱與編號、優先權實作了幾個位元、哪些 API 能在 ISR 呼叫 | 啟動碼、core manual、SDK 文件 | 第四課：pending、清旗標、量延遲 |
| HAL 型別與語意 | handle、狀態碼、是否阻塞、callback 在哪個脈絡執行、buffer 要存活多久、忙碌時回傳什麼 | SDK 文件與原始碼 | 追到暫存器；故障注入 |
| 時間基準 | 計數器寬度、頻率、回繞週期、有沒有中斷、debugger 暫停時是否繼續 | reference manual、core manual | 回繞測試；第四課的週期量測 |
| Flash 與 watchdog | 頁大小、寫入單位、抹除時間；watchdog 的種類、時脈與休眠行為；重啟原因旗標 | reference manual、datasheet | 第八課的斷電與卡死注入 |
| 核心與 C 的假設 | int 的寬度、32 位元存取是否一次完成、原子操作是否 lock-free、硬體除法、非對齊存取 | core manual、編譯器文件 | 編譯警告、`atomic_is_lock_free`、測試 |
| build | 工具鏈與版本、`-mcpu` 等旗標、晶片型號巨集、C 函式庫、bin 或 hex 的轉換 | SDK 範例的建置設定 | 從乾淨目錄重建並保存 build log |

[Zephyr 的 board porting 文件](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html) 是另一種描述 BSP 的方式，可以對照上表的 BSP 與接腳兩列。

## 移植時才暴露的假設

- 整數寬度。C 標準只要求 int 至少能表示 −32767 到 +32767，有些 8 位元與 16 位元 MCU 的 int 就是 16 位元。第五課 frame.h 的 `get_u16` 寫成 `p[0] | (p[1] << 8)`：`p[1]` 先提升成 int，在 16 位元 int 上 0xFF × 256 超出範圍，依 C11 6.5.7 是未定義行為；frame.c 的 CRC 裡 `data[i] << 8` 也一樣。在 32 位元平台上正確，要搬到 16 位元 int 的平台，先轉成 `uint16_t` 再移位。[C11 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- 一次完成的存取。第四課的撕裂讀取在 8 位元或 16 位元核心上會發生在任何 32 位元變數上，包括第五課環形緩衝區的索引。用 `atomic_is_lock_free` 確認，或改用較窄的索引。
- 非對齊存取與硬體除法。Cortex-M0 與 M0+ 不支援非對齊存取，任何一次都進 HardFault；它們的指令集也沒有 SDIV 與 UDIV，Cortex-M4 有。[Cortex-M0+ Generic User Guide](https://support.arm.com/documentation/dui0662/latest/)、[Cortex-M0 Generic User Guide](https://support.arm.com/documentation/dui0497/latest/) 把 byte 陣列轉型成 `uint32_t *` 再讀的寫法換核心就可能壞；第五課逐 byte 組裝欄位，不受影響。除法會變成函式庫呼叫，實驗的平台 B 換算時間只除以 32,768，編譯器能用位移完成。
- 時間基準。平台 B 的 16 位元計數器約 2 秒回繞一次，主迴圈卡住超過 2 秒，時間就少算。
- stack 與位元組順序。同一段程式在不同編譯器與核心上的 stack 用量不同，中斷巢狀與每個 task 的 stack 都要重算。協定逐 byte 組裝，不受位元組順序影響；直接傳 C struct 的寫法換平台就可能壞。

## 怎麼發現改了太多應用邏輯

訊號：應用的原始碼出現 `#ifdef BOARD_X`、廠商標頭、handle 或狀態碼；應用在等 HAL 的 busy 旗標，或應用邏輯直接寫在 callback 裡；時間常數由時脈頻率算出來；buffer 的大小或對齊被 DMA 的限制綁死在應用裡；為了新平台改了協定格式，連電腦端的測試也得改。

檢查方法：兩個平台的應用核心用 diff 比對，應該完全相同，任何差異都要說得出為什麼它不屬於 BSP 或 driver；只給核心自己的 include 路徑單獨編譯；數一數每一層改了幾行，應用層應該接近零。

有些差異是資源而不是邏輯，例如新晶片的 SRAM 只放得下較小的 TX 緩衝區。第五課的 logger.h 把容量寫死成常數，只能直接改標頭；比較好的做法是讓這類常數能由 build 覆寫，記在 build 那一層，並重算第五課的第一次丟棄時間。

## 桌機實驗

[labs/08-port-layers](../labs/08-port-layers/) 把資料採集器拆成應用核心與窄介面，再提供兩個行為真的不同的虛構平台。同一份協定測試和兩個平台各連結一次，都必須通過。

| 路徑 | 內容 |
| --- | --- |
| [core/platform.h](../labs/08-port-layers/core/platform.h)、[core/app.c](../labs/08-port-layers/core/app.c) | 平台介面與應用層，加上直接引用、一個字都沒改的第五課 logger.c、frame.c、ring.c |
| [tests/protocol_tests.c](../labs/08-port-layers/tests/protocol_tests.c)、[tests/sim.h](../labs/08-port-layers/tests/sim.h) | 電腦端協定測試與測試台介面，兩個平台共用 |
| [plat_a/](../labs/08-port-layers/plat_a/) | 虛構廠商 A 的暫存器標頭、BSP 與 driver、硬體模型 |
| [plat_b/](../labs/08-port-layers/plat_b/) | 虛構廠商 B 的 HAL 標頭、BSP 與 driver、HAL 實作與硬體模型 |

| | 平台 A | 平台 B |
| --- | --- | --- |
| 時脈 | BSP 自己開 PLL、等鎖定、切換，baud 除數自己算 | `vndb_system_init()` 包辦，BSP 只給 baud |
| 時間 | SysTick 每 1 ms 中斷，ISR 數 32 位元毫秒 | 16 位元、32,768 Hz 自由計數，沒有中斷，約 2 秒回繞；BSP 自己延伸 |
| UART 接收 | 一個 IRQ handler 讀狀態暫存器，讀 DATA 清旗標 | HAL 收滿 1 byte 呼叫 callback，callback 裡要立刻重新開始接收 |
| UART 傳送 | 有資料時打開 TX 空中斷，每次送 1 byte，沒資料就關掉 | 一次交給 HAL 最多 32 bytes，送完的 callback 接著送；送完前那塊記憶體屬於 HAL |
| 感測器 | 寫 START、輪詢 DONE | 非阻塞讀取加 callback；逾時後要先 abort 才能重來 |

```sh
cd labs/08-port-layers
gcc -std=c11 -Wall -Wextra -Werror -O2 -Icore -Itests -Iplat_a -I../04-uart-pipeline -o test_a tests/protocol_tests.c plat_a/sim_a.c plat_a/board_a.c core/app.c ../04-uart-pipeline/logger.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c ../04-uart-pipeline/host_rx.c
./test_a
gcc -std=c11 -Wall -Wextra -Werror -O2 -Icore -Itests -Iplat_b -I../04-uart-pipeline -o test_b tests/protocol_tests.c plat_b/sim_b.c plat_b/board_b.c core/app.c ../04-uart-pipeline/logger.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c ../04-uart-pipeline/host_rx.c
./test_b
```

兩行只差平台目錄與兩個檔名，這是這個實驗 build 層的全部改動；真實專案還有啟動碼、linker script 與編譯旗標。證明應用核心不依賴任何平台標頭：

```sh
gcc -std=c11 -Wall -Wextra -Werror -fsyntax-only -Icore -I../04-uart-pipeline core/app.c ../04-uart-pipeline/logger.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c
gcc -std=c11 -MM -Icore -I../04-uart-pipeline core/app.c ../04-uart-pipeline/logger.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c
```

第二行列出的標頭只有 core 與 04-uart-pipeline 底下的檔案。這個檢查擋不住 `#include "../plat_a/vendor_a.h"` 這種相對路徑，所以也要搜尋核心裡的每一行 `#include`。

協定測試沿用第五課整合測試的情境：10 秒正常資料流、有效與無效命令、超載、感測器斷線、半個命令與接收溢位、140 秒長時間測試。不同的是它完全不讀裝置內部的變數，裝置端的計數一律用 GET_STATUS 從線路上問；第五課讓 TX 索引從接近回繞處開始，電腦看不到這個索引，所以這裡改成讓毫秒計數從 0xFFFFF000 開始。

執行之前先預測：兩個平台在超載時第一次跳號的時間、遺失的筆數會一樣嗎？平台 B 的計數器在全部測試中大約回繞幾次，時間戳為什麼仍然連續？平台 A 的 UART 中斷次數和平台 B 的傳送完成 callback 次數，差多少倍？以下是兩個平台各一次執行的部分輸出：

```text
平台：A：暫存器層級 UART、SysTick 中斷數毫秒、PLL 倍頻到 48 MHz
  電腦在 2.17 s 第一次看到跳號（裝置約在 2.08 s 開始丟，之後還要等緩衝區送完）
  電腦算出遺失 117 筆，裝置回報丟棄 117 筆，帶 OVERFLOW 的封包 117 個
  SysTick 中斷 169340 次、UART 中斷 997183 次（單一毫秒最多 19 次）

平台：B：HAL callback 式 UART 與感測器、16 位元 32,768 Hz 自由計數 timer
  電腦在 2.16 s 第一次看到跳號（裝置約在 2.08 s 開始丟，之後還要等緩衝區送完）
  電腦算出遺失 118 筆，裝置回報丟棄 118 筆，帶 OVERFLOW 的封包 118 個
平台 B 統計（全部測試累計）：低功耗 timer 回繞 90 次
  callback：接收 201 次、傳送完成 74016 次、感測器 77063 次
```

遺失筆數差了一筆，兩個平台卻都通過：協定測試檢查的是契約，遺失要被計數，而且電腦算出的數字等於裝置回報的數字，而不是某個只有特定平台才會得到的固定值。傳送的節奏不同，丟棄就可能早一筆或晚一筆。

改壞一處。以下每一項都在 Windows 11、GCC 16.2 上實際改過，並確認右欄的結果：

| 改壞一處 | 結果 | 對應清單 |
| --- | --- | --- |
| board_a.c：baud 除數用內部振盪器的 8 MHz 計算 | 23 項檢查全部失敗：雙方 baud 不同，收到的 byte 全錯 | 時脈樹 |
| board_a.c：沒資料可送時不關 TXIE | 只剩「沒有解析錯誤」通過，因為電腦什麼都沒收到：中斷風暴，主迴圈得不到時間 | 中斷 |
| board_b.c：把重新開始接收從 `on_rx_done` 移到 `board_main_once` | 需要送命令的檢查全部失敗，只看資料流的第一項仍通過 | HAL 語意 |
| board_b.c：直接把 16 位元計數換算成毫秒，不做延伸 | 10 秒資料流、感測器斷線、長時間測試失敗 | 時間基準 |
| board_b.c：`b_tx_kick` 不檢查 `s_tx_busy` | 命令、超載、接收錯誤的檢查失敗 | buffer 所有權 |
| core/app.c：加上 `#include "vendor_a.h"` | 核心單獨編譯失敗，平台 B 也建置失敗 | 邊界 |

第三項值得多看一眼：只跑正常資料流的測試，抓不到 callback 語意的錯誤，因為那段時間電腦沒有送任何東西。

這個模型不能證明的事：兩家的 HAL 都是虛構的，真實 HAL 的 callback 時機、忙碌狀態與錯誤處理要讀它的原始碼確認；電氣訊號、真實的中斷延遲、時脈精度與 Flash 行為都要上板量測。

## 上板實作

1. 在新板子上做「一小時分類」，記錄板子型號、完整晶片料號、工具鏈、SDK 與 debugger 版本。
2. 從乾淨目錄建立新平台的官方 blink 或空專案，確認能燒錄、能斷在 main。
3. 把應用核心原封不動加入：第五課的 logger.c、frame.c、ring.c，第八課的 supervisor.c、boot_guard.c、config_store.c，以及你的平台介面與 app.c。先只給它們自己的 include 路徑編譯一次。
4. 依換平台清單寫 BSP 與 driver：時間、UART、感測器、Flash 的三個函式、watchdog 與重啟原因、.noinit 區與設定頁。每完成一項就用對應課程的方法驗證，不要全部寫完才一起測。
5. 電腦端的 serial.ps1、rx_check 與協定測試原封不動，重跑第五課與第八課的驗收表。
6. 重做量測：取樣週期的抖動、第一次丟棄的時間、卡死到重啟的時間、Flash 抹除時間，填進學習紀錄的「第二平台」一欄，和第一塊板對照。
7. 填寫各層的改動表：

| 層 | 改了什麼 | 為什麼 | 怎麼驗證 |
| --- | --- | --- | --- |
| 應用 | | | |
| BSP | | | |
| driver | | | |
| build | | | |

| 驗收條件 | 要留下的證據 |
| --- | --- |
| 主機端協定測試沿用 | rx_check、serial.ps1 與協定測試的版本或雜湊和第一塊板相同；第五課的驗收表在新平台全部通過 |
| 應用核心沒有改 | 兩個平台的核心檔案 diff 結果；只用核心 include 路徑的編譯紀錄 |
| 列出各層改動及原因 | 上面的改動表，每一列都有原因與驗證方法 |
| 新平台的時間行為 | 第四課、第五課、第八課的量測數字，與第一塊板對照 |

## 先自行回答

1. 平台 B 的計數器是 16 位元、32,768 Hz。主迴圈最長可以多久不呼叫 `now_ms`？超過會怎樣？
2. 新平台的 HAL 是在呼叫傳送完成 callback 之前還是之後，才把狀態設回空閒？為什麼這件事決定你能不能在 callback 裡開始下一次傳送？
3. 某平台的 int 是 16 位元。`uint8_t b = 0xFF; uint16_t v = b << 8;` 有什麼問題？怎麼改？
4. 新晶片的 SRAM 放不下 1,024 bytes 的 TX 緩衝區，只好改成 256 bytes。這算應用的改動嗎？還要重新檢查什麼？
5. 新平台的 SDK 範例在 UART 接收 callback 裡直接解析命令。照著寫有什麼風險？

### 解析

1. 65,536 ÷ 32,768 = 2 秒。兩次讀取之間回繞超過一次時，延伸的算法只看得到一次，時間少算 2 秒的整數倍，取樣期限與所有逾時都跟著錯。主迴圈的最差延遲要小於 2 秒；做不到就改用有溢位中斷或回繞週期更長的計數器。
2. 若 HAL 在 callback 返回之後才設回空閒，callback 裡呼叫傳送會得到「忙碌」，那一批資料就卡住或遺失。要讀 HAL 的原始碼確認順序；不能在 callback 裡開始時，改成設旗標、由主迴圈開始下一次。
3. `b` 先提升成 16 位元的 int，0xFF × 256 = 65,280 超出 int 的範圍，是未定義行為。寫成 `(uint16_t)((uint16_t)b << 8)`：在 16 位元 int 的平台上 `uint16_t` 提升成 unsigned int，移位有模數語意；在 32 位元平台上提升成 int，結果也放得下。
4. 這是資源設定，不是應用邏輯，記在 build 那一層，並讓容量能由 build 覆寫，而不是每個平台改一份標頭。要重算第一次丟棄的時間與停頓能撐多久，確認保留量仍放得下命令回應，再重跑超載的驗收。
5. callback 在中斷脈絡執行：解析命令、寫設定甚至寫 Flash 都會拉長中斷時間，也會和主迴圈同時改同一份狀態。依第五課的規則，callback 只把 byte 放進環形緩衝區，解析留在主迴圈。

## 今日交付

在 [學習紀錄](../learning-log.md) 填好「第二平台」一欄的數字，附上各層改動表、核心檔案的 diff 結果、電腦端工具的版本，以及查過的文件與章節。

最後用自己的話回答：從第一課到這裡，哪些知識不依賴任何一家的晶片？哪些每換一次平台都要重查？

這是課程的最後一課。之後依需要展開 [實作路線](../02-learning-path.md) 提到的進階主題：cache 與 DMA 一致性、無線、安全開機與更新流程；或回頭補 [課程首頁](../README.md) 列為第一輪暫緩的圖形介面、USB、馬達控制、多核心與自製 PCB。每個新主題都用同樣的方式開始：先預測，做最小實作，量測，注入一個可恢復的錯誤，並把證據記進 [學習紀錄](../learning-log.md)。
