# 第七課 RTOS：task、queue、mutex 與 ISR 邊界

這一課約 6 至 10 小時。這是資料採集器的第四版：把超級迴圈裡的工作拆成幾個 task，再比較延遲、記憶體與新出現的失敗方式。桌機實驗用一個模擬的小 kernel，不必安裝任何 RTOS；上板時才選 FreeRTOS 或 Zephyr。讀完應該能解釋：RTOS 解決了超級迴圈的哪個問題、沒有解決哪個；為什麼加了 mutex 之後，最高優先權的 task 反而可能等得更久；ISR 為什麼只能呼叫特定的 API。

## RTOS 提供什麼，不提供什麼

RTOS 的核心是一個排程器：在所有 ready 的 task 中，執行優先權最高的那一個。每個 task 有自己的 stack，等待時可以阻塞而不必輪詢；kernel 另外提供 queue、semaphore、mutex、timer 這類讓 task 互相通知的物件。

| 問題 | 超級迴圈的做法 | RTOS 的做法 | RTOS 沒有改變的 |
| --- | --- | --- | --- |
| 長工作擋住短工作 | 手動把長工作切成小段的狀態機 | 長工作放進低優先權 task，被高優先權 task 搶占 | CPU 總量：25 ms 的運算仍然要 25 ms |
| 等待事件 | 每一圈檢查旗標 | 在 queue 上阻塞，不耗 CPU | 事件的到達率與緩衝區大小 |
| 共用資料 | 只有 ISR 與主迴圈兩方，SPSC 就夠 | 多個 task 可能同時寫同一份資料 | 所有權仍然要自己設計 |
| 記憶體 | 一個 stack | 每個 task 一個 stack，加上 kernel 的控制區塊 | buffer 的大小 |

適合加 RTOS 的情況：有幾件期限不同的工作，其中有長時間的運算，又不想手動切小段；要用預期在 thread 裡執行的 middleware，例如網路協定堆疊或檔案系統；要同時等好幾種事件並各自設逾時。

不適合的情況：每件工作都短而且週期固定，超級迴圈已經滿足期限；問題其實是 CPU 不夠，RTOS 不會讓運算變快；需要微秒等級的反應，那仍然屬於 ISR、DMA 或 timer 硬體。RTOS 也帶來新的失敗方式：競態、優先權反轉、死結與 stack 溢位。

## task 的數量由執行與等待決定

一件工作要不要獨立成 task，看它等的事件是不是不同、期限是不是不同，或是會不會阻塞在無法改成非阻塞的呼叫上。不要每個函式都包成 task：解析器、CRC 與封包編碼都只是 cmd task 裡的函式。

| task | 等什麼 | 期限 | 優先權 | 為什麼獨立 |
| --- | --- | --- | --- | --- |
| sample | DMA 中斷送來的批次號 | DMA 繞回之前，10 ms | 4，最高 | 期限最短 |
| cmd | UART 中斷送來的位元組 | 電腦等 ACK，數十 ms | 3 | 等的事件不同，要及時回應 |
| stats | 每 100 ms 一次 | 下一次之前 | 2 | 25 ms 的純運算，不能擋住別人 |
| diag | 每 100 ms 一次 | 盡力而為 | 1 | 可以和 stats 合併；這裡分開，是為了重現優先權反轉 |
| idle | 沒有別人要跑時 | | 0 | kernel 提供 |

期限越短、優先權越高，是排定優先權的常見起點。優先權的數字方向各家不同。FreeRTOS 的數字越大越優先，idle 是 0。[FreeRTOS Kernel Book 第 4 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md) Zephyr 的數字越小越優先，負數是不會被其他 thread 搶占的 cooperative thread。[Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) Cortex-M 的中斷優先權也是數字越小越優先；同一個專案裡同時有三種慣例，每次換平台都要重查。

## queue：ISR 與 task 之間的邊界

ISR 只做最少的事：把事件複製進 queue 就離開。DMA 中斷送出的是批次號，不是資料本身；資料仍在第六課的 DMA buffer 裡，所有權規則不變，queue 只負責通知路徑。

ISR 用的送出 API 不會等待：queue 滿了就立刻失敗，這次失敗要計數。它還會回報「是否喚醒了比目前 task 更高優先權的 task」，ISR 結尾據此要求切換。FreeRTOS 的寫法是 `xQueueSendFromISR` 搭配 `portYIELD_FROM_ISR`；不要求切換的話，被喚醒的 task 要等到下一次排程器執行，最差是下一個 tick。[FreeRTOS Kernel Book 第 7 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch07.md) Zephyr 沒有另外的 FromISR 版本，ISR 把工作交給 thread 時，通常在 ISR 結束時切換一次。[Zephyr Interrupts](https://docs.zephyrproject.org/latest/kernel/services/interrupts.html)

queue 的長度由突發決定，不是越大越好；模型會印出每個 queue 的最高佔用。

## 哪些 API 可以在 ISR 裡呼叫

- 不能阻塞：不能等 queue、semaphore、mutex，也不能 delay。ISR 沒有自己的 task 可以讓出 CPU。
- 不能用 mutex。mutex 有擁有者，優先權繼承也只對 task 有意義。FreeRTOS 與 Zephyr 的文件都明確規定 mutex 不能在 ISR 裡使用。[FreeRTOS semphr.h](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/include/semphr.h)、[Zephyr Mutexes](https://docs.zephyrproject.org/latest/kernel/services/synchronization/mutexes.html)
- FreeRTOS：名稱裡沒有 FromISR 的 API，一律不能在 ISR 裡呼叫。就算是 FromISR，也只能從邏輯優先權不高於 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 的中斷呼叫；Cortex-M 上數字越小越優先，0 最高。比這個設定更高的中斷不會被 kernel 延遲，代價是不能呼叫任何 FreeRTOS API。[FreeRTOS Kernel Book 第 7 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch07.md)
- Zephyr：可能睡眠的函式只有在不等待的模式下，例如逾時參數給 `K_NO_WAIT`，才能從 ISR 呼叫；同一個函式可能被 thread 與 ISR 共用時，用 `k_is_in_isr()` 判斷。[Zephyr API 術語](https://docs.zephyrproject.org/latest/develop/api/terminology.html)

真正的 RTOS 不一定能在執行時發現這類誤用；違反時的行為沒有保證，可能剛好能動，也可能毀掉 kernel 的資料。桌機模型把每次誤用記下來並讓呼叫失敗，是為了讓你看見它。

## 兩個 task 寫同一個 TX：競態

第五課的 TX 環形緩衝區只有一個生產者。拆成 task 之後，sample、cmd 與 diag 都要寫 TX，前提就不成立了。寫一個封包分成三步：讀 head、逐 byte 寫入、發布新的 head。在第一步與第三步之間被搶占，另一個 task 就會讀到同一個 head：

```text
diag    讀 head=H，寫第 0、1 個 byte …（被搶占）……………… 繼續寫第 2 到 14 個 byte，發布 H+15
sample             讀 head=H，寫 12 bytes，發布 H+12
TX DMA                                  從 H 開始送：前面是 sample 的，後面被 diag 蓋掉
```

更糟的是 diag 很晚才發布 H+15 時，head 可能已經被別人推到更後面，這一步等於把 head 往回拉，後面幾個封包直接消失。裝置本身完全不知道，所有計數都顯示正常。

修正有兩種：用 mutex 把「讀 head 到發布」包成臨界區；或讓 TX 只有一個擁有者，其他 task 把封包放進它的 queue，FreeRTOS 的書稱這種 task 為 gatekeeper。[FreeRTOS Kernel Book 第 8 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch08.md) 第二種不會有優先權反轉，代價是多一個 task 和一次複製。桌機模型示範第一種。

## mutex 與優先權反轉

加了 mutex 之後，資料不再損壞，卻出現另一個問題：

```text
 9.0 ms   diag（1）上鎖，在鎖裡收集資料並逐 byte 格式化，共 2.5 ms
10.25 ms  DMA 事件：sample（4）搶占 diag，算完平均，要上鎖 → 等 diag
11.0 ms   stats（2）醒來，比 diag 高，搶占 diag，一跑就是 25 ms
          sample 等 diag，diag 等 stats：最高優先權的 task 被中間的 stats 擋住
約 36 ms  stats 跑完，diag 才寫完、解鎖，sample 才繼續；20.25 ms 那一批已經被 DMA 繞過
```

高優先權 task 被低優先權 task 擋住，就是優先權反轉。優先權繼承的做法是：高優先權 task 開始等某個 mutex 時，持有者暫時提升到等待者的優先權，解鎖時恢復。stats 因此無法插隊，sample 最多只等 diag 剩下的臨界區。FreeRTOS 的書強調，繼承並不消除反轉，只讓它的時間有上限。FreeRTOS 的 mutex 有繼承，binary semaphore 沒有。[FreeRTOS Kernel Book 第 8 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch08.md) Zephyr 的 mutex 也有繼承，解鎖時交給等待最久的最高優先權 thread。[Zephyr Mutexes](https://docs.zephyrproject.org/latest/kernel/services/synchronization/mutexes.html)

繼承設定好之後，等待的上限就是臨界區的長度。真正的改善是讓臨界區只做搬運：diag 先在鎖外把封包準備好，鎖裡只花 30 µs 寫進 TX。

## stack 與記憶體

每個 task 的 stack 都要容得下三樣東西：最深呼叫鏈上每個函式的 frame；被中斷打斷時硬體推入的例外框架，Cortex-M 是 8 個 word，有 FPU 且正在使用浮點時還有浮點暫存器；kernel 切換時另外保存的暫存器，數量依移植層而定。Cortex-M 有 MSP 與 PSP 兩個 stack pointer。[Cortex-M4 Generic User Guide](https://support.arm.com/documentation/dui0553/latest/) 常見的 RTOS 移植讓 task 用 PSP、中斷用 MSP，所以中斷的 stack 要另外估；Zephyr 的文件也說明中斷有自己專用的 stack 區域。

stack 大小的單位也各家不同。FreeRTOS 建立 task 時給的 stack 深度以 word 計，32 位元核心上 128 代表 512 bytes；`uxTaskGetStackHighWaterMark` 回傳的最小剩餘空間同樣以 word 計。[FreeRTOS task.h](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/include/task.h) Zephyr 開啟 `CONFIG_INIT_STACKS` 與 `CONFIG_THREAD_STACK_INFO` 後，`k_thread_stack_space_get()` 回報的是剩餘的 bytes。[Zephyr Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)

## 對應到真實的 RTOS

桌機模型的 API 名稱是自訂的，概念可以一對一換過去。下表只列對應關係；參數與設定以你使用的版本文件為準。

| 模型 | FreeRTOS | Zephyr |
| --- | --- | --- |
| task 與優先權 | `xTaskCreateStatic`；數字越大越優先 | `K_THREAD_DEFINE` 或 `k_thread_create`；數字越小越優先 |
| `q_recv(t, q, &x, 1)` | `xQueueReceive(q, &x, portMAX_DELAY)` | `k_msgq_get(&q, &x, K_FOREVER)` |
| `q_send_from_isr` 加 `isr_exit` | `xQueueSendFromISR(q, &x, &woken)`，結尾 `portYIELD_FROM_ISR(woken)` | ISR 中 `k_msgq_put(&q, &x, K_NO_WAIT)` |
| `mtx_lock`、`mtx_unlock`，有繼承 | `xSemaphoreCreateMutex`、`xSemaphoreTake`、`xSemaphoreGive` | `k_mutex_lock`、`k_mutex_unlock` |
| `k_delay_until` | `xTaskDelayUntil` | `k_timer`，或在 thread 裡自己算期限再 `k_sleep` |
| 誤用偵測 | 文件規定只用 FromISR；可定義 `configASSERT` 增加檢查 | `k_is_in_isr()`；ISR 中只用不等待的模式 |
| stack 剩餘量 | `uxTaskGetStackHighWaterMark`，單位 word | `k_thread_stack_space_get`，單位 byte |

## 桌機實驗 A：模擬 kernel

[labs/06-rtos-model/rtos_sim.c](../labs/06-rtos-model/rtos_sim.c) 是一個用虛擬時間執行的優先權搶占式 kernel：1 ms tick、ready 清單、會阻塞的 queue、可選優先權繼承的 mutex，以及 ISR 誤用的偵測。每個 task 寫成一步一步的狀態機，步與步之間和計算途中都可能被搶占。封包格式與解析器直接用第五課的 frame.c；所有成本數字都是假設值，用來比較設計。

```sh
cd labs/06-rtos-model
gcc -std=c11 -Wall -Wextra -Werror -O2 -I../04-uart-pipeline -o rtos_sim rtos_sim.c ../04-uart-pipeline/frame.c
./rtos_sim
./rtos_sim --test
```

同一組工作跑六種設計。執行之前先預測：A 的超級迴圈會丟幾批？C 沒有 mutex，裝置自己回報的遺失是多少？D 的 sample 最久等多久？E 的上限由什麼決定？

```text
case smp_tx smp_rx  gaps  lost  crc st_tx/rx    smp_lat    cmd_lat max_block switches
A       899    899   100   100    0  345/345       7473      25425         0        1
B       999    999     0     0    0  345/345       1632       2586         0        1
C       999    672   326     0  104  345/200        356        463         0     4774
D       899    899   100   100    0  345/345      26848      28321     28211     4750
E       999    999     0     0    0  345/345       1773       2650      2540     4830
F       999    999     0     0    0  345/345        358        455        11     4778
```

A 與 B 是超級迴圈，stats 一次做完或切成 1 ms 小段；C 是 RTOS 但寫 TX 沒有 mutex；D 有 mutex 沒有繼承；E 加上繼承；F 再把 diag 的慢工作移出臨界區。`gaps` 是電腦看到的序號跳號，`lost` 是裝置自己回報的遺失；smp_lat、cmd_lat 與 max_block 的單位都是微秒。讀表時回答：

- A 的 smp_lat 只有 7.5 ms，為什麼仍然丟了 100 批？被丟掉的那些批次為什麼不算進延遲？
- B 沒有 RTOS 也不丟資料。它付出的代價在哪裡？
- C 的 lost 是 0，電腦卻看到 326 個跳號、少收 145 個 STATUS。為什麼裝置自己完全不知道？
- D 加了 mutex，反而和 A 一樣丟 100 批。畫出 sample、stats、diag 三條時間線。
- E 的 max_block 是 2,540 µs，對應 diag 臨界區的哪一段？為什麼 E 的延遲沒有比 B 好？
- 真正讓延遲降一個數量級的是 F。它改了什麼？

輸出的後半段是代價：

```text
kernel 與中斷的時間（假設每次切換 4 us、每次中斷 2 us）
  A：切換 1 次，kernel 0.0 ms，中斷 24.4 ms，idle 0.0 ms，共 10000 ms
  E：切換 4830 次，kernel 21.8 ms，中斷 24.4 ms，idle 6828.3 ms，共 10000 ms

    loop     rtos  RAM（bytes）
    1024     3072  stack（設定值，不是量到的用量）
      48       48  queue 內容（項目大小 x 長度）
    1184     1184  TX 與 DMA buffer
       0        ?  TCB、queue 與 mutex 的控制區塊

task    prio  stack   cpu_ms     in preempted  max_wait  max_block
sample     4    512    341.7   1102         0         6       1424
cmd        3    768     31.6   1181         7      1698       2540
stats      2    512   2500.2    583       482      1082          0
diag       1    512    252.0    312       211     26183          0
idle       0    256   6828.3   1652      1651     29030          0
queue 最高佔用：batch_q 1/4，rx_q 5/32
```

stack 欄是設定值，模型量不到 C 程式真正用了多少 stack；cpu、切換次數與等待時間是模型量到的，但建立在假設的成本上。控制區塊大小依 RTOS 與設定而定，所以模型印問號，不猜數字。

再試兩個故障，先寫預測：

| 故障 | 你的預測 | 實際觀察 |
| --- | --- | --- |
| `./rtos_sim --fault=isr-blocking` | | |
| `./rtos_sim --fault=no-yield` | | |

isr-blocking 讓 UART 中斷改用 task 版本的 q_send；kernel 記下 1,225 次誤用，245 個命令一個也沒有回應。no-yield 讓 ISR 喚醒 task 時不要求切換；F 的 smp_lat 從 358 µs 變成 1,105 µs，正好多出一段等下一個 tick 的時間。

自我檢查全部通過之後，改壞一處，看哪一項檢查失敗：

- 在 `mtx_lock` 刪掉優先權繼承那一行：E 的遺失、等待上限與延遲三項檢查都會失敗。
- 在 `tx_step` 讀完 head 之後就先發布 `txr.head = j->w + j->n`，再寫資料：連超級迴圈 A、B 都會失敗。TX DMA 也是讀者，發布之後它就可能去讀還沒寫好的 byte，這和有沒有 RTOS 無關。
- 把 stats 的優先權改成 5，高過 sample：E 與 F 的遺失檢查都會失敗。優先權排錯，繼承也救不回來。

## 桌機實驗 B：估計 stack 的靜態方法

GCC 的 `-fstack-usage` 會為每個函式寫出它自己的 frame 大小。拿第五課的應用核心試：

```sh
cd labs/06-rtos-model
gcc -std=c11 -O2 -fstack-usage -c ../04-uart-pipeline/logger.c ../04-uart-pipeline/frame.c ../04-uart-pipeline/ring.c
```

每個 .c 會得到一個 .su 檔。在 Windows 11、GCC 16.2 x86-64 上，`logger_poll` 是 96 bytes、`parser_feed` 128、`on_command` 128、`ring_write_all` 24。最深的呼叫鏈是 logger_poll → parser_feed → on_command → ring_write_all，約 376 bytes；其中 `on_command` 是透過函式指標被呼叫的，工具不會替你把它接上，要自己沿著程式找。這些是 x86-64 的數字，它的呼叫慣例和 Cortex-M 不同，不能直接搬；換成目標平台的交叉編譯器再做一次，加上例外框架與 kernel 保存的暫存器，才是 stack 設定的起點，最後以上板量到的剩餘量確認。

## 上板實作

1. 依板子的 SDK 選 FreeRTOS 或 Zephyr，記下版本、移植層、tick 頻率、是否允許搶占。FreeRTOS 在 Cortex-M 上要確認 `configMAX_SYSCALL_INTERRUPT_PRIORITY`，以及每個會呼叫 FromISR API 的中斷都明確設了優先權。
2. 把第六課的資料採集器拆成 sample、cmd、stats、diag 四個 task。DMA 與 UART 中斷只用 FromISR 或不等待的 API 送進 queue。RX 環形緩衝區仍是一個生產者、一個消費者，可以沿用；TX 有多個生產者，要加 mutex 或改成單一擁有者。
3. 量延遲：在 DMA 中斷開頭與取樣封包進入 TX 時各切換一支 GPIO，用邏輯分析儀量兩者的間隔；命令也一樣，從 RX 線的最後一個停止位量到 ACK 進入 TX。同一組負載在第六課的超級迴圈版本上再量一次，比較最大值與分布。
4. 量記憶體：比較加入 RTOS 前後的 map file 與 `size` 輸出；全負載跑 10 分鐘，包含下面的故障注入，記下每個 task 的 stack 剩餘量。開啟 RTOS 的 stack 溢位檢查。
5. 重現競態：讓兩個 task 不經 mutex 寫 TX，用 rx_check 看 CRC 錯誤與跳號，再加回 mutex 確認歸零。
6. 重現優先權反轉：FreeRTOS 用 binary semaphore 代替 mutex，Zephyr 用 `k_sem`，兩者都沒有繼承；量 sample 的最長等待，再換回 mutex 比較。
7. 在一個中斷裡故意呼叫非 ISR 版本的 API，記錄發生什麼事。沒有當機不代表正確。

| 驗收條件 | 操作 | 要留下的證據 |
| --- | --- | --- |
| 對比加入前後的延遲 | 同一組負載跑超級迴圈與 RTOS | 兩版的最大延遲與分布、量測方法、樣本數 |
| 對比加入前後的記憶體 | map file、stack 剩餘量 | text、data、bss 的前後差異；各 task 的 stack 設定與實際最高用量 |
| 重現並修正一個競態 | 拿掉 TX 的 mutex | rx_check 的 CRC 錯誤與跳號；修正後歸零 |
| 說明優先權反轉與修正 | 換成沒有繼承的同步物件 | 最長等待的前後對照 |
| 知道 ISR 可呼叫哪些 API | 列出每個用到的 API | 是否 ISR 安全、出處文件與章節 |

模型證明的是排程規則與同步邏輯；真實的切換時間、中斷延遲與 stack 用量，只能在板子上量。

## 先自行回答

1. 三個 task 的優先權是 H、M、L。L 持有 mutex 2 ms，M 每次運算 25 ms。沒有繼承時，H 最多可能等多久？有繼承呢？
2. ISR 用 `xQueueSendFromISR` 喚醒了高優先權 task，卻沒有呼叫 `portYIELD_FROM_ISR`。那個 task 什麼時候才會執行？
3. 為什麼不能在 ISR 裡 lock mutex？
4. 有人把每個函式都包成 task：解析、CRC、編碼、送出各一個。代價有哪些？
5. FreeRTOS 的 `uxTaskGetStackHighWaterMark` 回傳 20，在 32 位元核心上代表什麼？
6. 加了 RTOS 之後，第五課的 ring.c 還能用在哪個方向？

### 解析

1. 沒有繼承時，等待時間沒有固定上限：L 剩下的臨界區，加上 M 和其他中間優先權 task 插隊的全部時間，這裡至少是 25 ms 再加上 L 剩下的部分。有繼承時，M 無法插隊，H 最多等 L 剩下的臨界區，也就是不到 2 ms，再加上切換成本。
2. 等到下一次排程器執行，最差是下一個 tick。桌機模型裡多出約 0.75 ms，因為 DMA 事件和 tick 錯開了 0.25 ms。
3. ISR 不是 task，沒有辦法阻塞等待；mutex 的擁有者與優先權繼承也只對 task 有意義。ISR 要通知 task，用 queue 或 semaphore 的 ISR 版本。
4. 每個 task 都要一個 stack 與控制區塊；每段資料都要經過 queue 複製；每次交接都是一次切換。更嚴重的是延遲變得難以預測，而且多了很多要保護的共用狀態。task 的數量應由等待的事件與期限決定。
5. 這個 task 開始執行以來，stack 的剩餘空間最少只剩 20 個 word，也就是 80 bytes。這已經很接近溢位了。
6. RX 方向：UART 中斷是唯一的生產者，cmd task 是唯一的消費者，SPSC 的前提仍然成立。TX 方向有好幾個 task 寫，不能直接用，要加 mutex 或讓單一 task 擁有它。

## 今日交付

在 [學習紀錄](../learning-log.md) 留下：選用的 RTOS 與版本、task 的劃分與優先權理由、加入前後的延遲與記憶體對照、競態與優先權反轉各自的重現方式與修正前後的數字、每個用在 ISR 裡的 API 與出處。

最後用自己的話回答：你的資料採集器真的需要 RTOS 嗎？哪一個問題是 RTOS 解決的，哪一個是把臨界區縮短才解決的？換到另一個 RTOS，要重查優先權的數字方向、stack 大小的單位、ISR 可呼叫的 API，以及中斷優先權的限制。下一課處理系統卡住之後怎麼恢復：watchdog、重啟原因與設定儲存，[第八課](07-watchdog-and-recovery.md)。
