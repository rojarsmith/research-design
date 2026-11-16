# 第六課 ADC、DMA 與 buffer 所有權

這一課約 6 至 10 小時，可以分兩次完成。資料採集器的資料來源從「主迴圈每 10 ms 讀一次感測器」換成「timer 觸發 ADC、DMA 自己搬、CPU 一次處理一批」。讀完應該能解釋：為什麼 DMA 讓 CPU 少做很多事，卻多了一個每一刻都要回答的問題，這塊記憶體現在歸誰；為什麼 CPU 晚了資料一定會丟，而設計能決定的只是丟哪一份、有沒有被算到。

## 三條路徑換誰負責

[共通架構](../01-mental-model.md) 把定時取樣拆成控制、資料、通知三條路徑。第五課三條都由 CPU 負責；這一課把它們分給不同的硬體：

| 路徑 | 第五課 | 這一課 |
| --- | --- | --- |
| 控制：何時取樣 | 主迴圈比對 tick | timer 直接觸發 ADC 轉換，CPU 不參與 |
| 資料：結果搬到哪裡 | 主迴圈呼叫感測器讀取 | DMA 把 ADC 的資料暫存器搬進記憶體 |
| 通知：一批可以處理了 | 不需要，每次一筆 | DMA 寫到一半與寫到結尾時各發一次中斷 |

取樣時刻從此由 timer 決定，不再受主迴圈的抖動影響。以 4 kHz 取樣、每批 40 筆為例，CPU 每秒只被打斷 100 次，逐筆中斷則是 4,000 次。代價是 CPU 不再是唯一會寫這塊記憶體的一方。

## ADC 要先查的幾件事

| 項目 | 要回答的問題 | 常見的失敗 |
| --- | --- | --- |
| 解析度與參考電壓 | 12 位元的 0 到 4095 各代表多少伏特 | 用錯參考電壓，所有讀值等比例偏差 |
| 取樣時間 | 內部取樣電容要充電多久 | 訊號源阻抗高、取樣時間太短，讀值偏低或受前一個通道影響 |
| 觸發來源 | 軟體啟動、timer 或外部接腳 | 以為是 timer 觸發，其實是連續轉換，取樣率完全不對 |
| 通道與順序 | 掃描幾個通道、結果依什麼順序出現 | 多通道時順序弄錯，數值對到錯的感測器 |
| overrun | 新結果蓋掉沒被讀走的舊結果時如何通知 | 旗標沒清；有些晶片在旗標清除之前不再送出 DMA 請求 |
| 校正 | 上電後是否要先跑校正程序 | 跳過校正，讀值有固定偏移 |

換算公式、取樣時間與訊號源阻抗的關係、overrun 之後怎麼恢復，各家寫法不同，要查 datasheet 的電氣特性與 reference manual 的 ADC 章節。

## buffer 在每一刻歸誰

DMA 是另一個會寫記憶體的主人。CPU 和 DMA 之間沒有鎖，DMA 也不會等 CPU；唯一的保護是雙方都遵守的所有權規則。

| 時刻 | 這一半屬於誰 | CPU 可以做什麼 | DMA 在做什麼 |
| --- | --- | --- | --- |
| DMA 正在寫 | DMA | 不碰 | 一筆一筆寫進來 |
| 寫完、發出事件之後 | CPU | 讀、計算或複製出去 | 寫另一半 |
| CPU 歸還之後 | DMA | 不碰 | 下一圈會再寫進來 |

交接只發生在事件的那一刻；歸還是一個明確的動作，先歸還再讀，讀的就是 DMA 的東西。「完成」代表 DMA 已經把資料寫進記憶體，不代表 CPU 已經處理。

這兩個事件在不少廠商文件裡叫 half transfer（HT）與 transfer complete（TC）；有些 DMA 控制器沒有半滿事件，要用兩個通道或兩個描述區塊互相接續來做乒乓。Zephyr 的 DMA API 也把它們抽象成半滿與完成兩種 callback 狀態。[Zephyr DMA API](https://docs.zephyrproject.org/latest/doxygen/html/group__dma__interface.html)

## 循環模式與乒乓 buffer

```text
dma_buf[80]：前半 0..39，後半 40..79；timer 每 250 µs 觸發一次 ADC，DMA 寫一筆

時間   0 ms        10 ms          20 ms          30 ms
DMA    寫前半：批0  寫後半：批1    寫前半：批2    寫後半：批3
事件               HT：批0交給CPU  TC：批1交給CPU  HT：批2交給CPU
CPU                處理批0 ……………… 最晚 20 ms 讀完，DMA 要回到前半了
```

循環模式下 DMA 寫完結尾自動回到開頭，不需要 CPU 重新設定，也不會停下來等。每一批從交出到被覆寫，CPU 只有半個 buffer 的時間，這裡是 10 ms。

乒乓 buffer 其實就是第五課容量為 2 的 SPSC 環形緩衝區：DMA 和它的中斷是生產者，CPU 是消費者。

- head：DMA 已寫完的批次數，只有 DMA 中斷寫。
- tail：CPU 已歸還的批次數，只有主迴圈寫。
- 批次 k 放在 k % 2 那一半；tail ≤ k < head 的批次屬於 CPU；DMA 正在寫 head % 2。
- head − k ≥ 2 表示 DMA 已經繞回批次 k 那一半，資料至少有一部分被覆寫。

和第五課唯一的差別是：這個生產者滿了也不會停。第五課的「滿了丟最新」在這裡做不到，除非真的把 DMA 停下來。主迴圈的檢查寫成這樣，順序就是所有權交接的順序：

```c
uint32_t head = atomic_load_explicit(&g_head, memory_order_acquire);
if (head - tail >= 2u) {              /* 還沒開始就被繞過：只剩 head-1 完整 */
    lost += (head - 1u - tail) * HALF;
    tail = head - 1u;
}
uint32_t k = tail;
batch_t b = summarize(&dma_buf[(k % 2u) * HALF]);  /* 就地讀取與計算 */
head = atomic_load_explicit(&g_head, memory_order_acquire);
if (head - k >= 2u) lost += HALF;     /* 算的途中被繞過：整批作廢 */
else publish(k, &b);
atomic_store_explicit(&g_tail, k + 1u, memory_order_release);  /* 最後才歸還 */
```

這個檢查假設 DMA 中斷在下一筆取樣寫入之前就執行完，也就是中斷延遲小於 250 µs。關中斷更久時，DMA 可能已經寫進新的一半而 head 還沒更新；要再讀 DMA 的剩餘計數暫存器確認它的位置，暫存器名稱查 reference manual。

## CPU 晚了：定義滿了怎麼辦

CPU 晚到有兩種：偶發的，例如等 Flash 寫入卡了 35 ms；長期的，例如每批要算 12 ms，但每 10 ms 就來一批。兩種都一定會丟資料，[實作路線](../02-learning-path.md) 要求的是明確選一種策略，不能默默覆寫：

| 策略 | 做法 | 結果 |
| --- | --- | --- |
| 丟舊 | DMA 不停；被繞過的批次整批作廢並計數 | 取樣時刻維持在格點上，遺失以整批為單位，數量精確 |
| 停住 | 中斷發現下一半還沒歸還就停住 DMA；歸還時重新啟動 | 不會覆寫，停住期間的取樣遺失，時間軸出現缺口，要記下缺口從哪裡開始 |
| 不檢查 | 照樣發布 | 回報零遺失，內容卻可能是另一批，或讀到一半被蓋掉的資料 |

資料採集器選丟舊：時間戳由批次號直接算出，被作廢的批次照樣佔一個序號，電腦端看到跳號就知道少了哪幾批；遺失數併入 GET_STATUS 的 missed 欄位。停住也是合理的選擇，Zephyr 的非同步 UART API 就是這樣規定的：下一塊 buffer 沒有及時提供，接收就停止。[Zephyr UART async API](https://docs.zephyrproject.org/latest/doxygen/html/group__uart__async.html)

長期超載還有一個陷阱。就地計算時，CPU 整段處理時間都佔著那一半；只要處理一批超過 10 ms，每一批都會在讀完之前被繞過，一筆也發布不了。先把 40 筆複製出來就歸還，佔用時間縮到複製的那幾微秒，超載才變成「依比例遺失」：每批 12 ms、每 10 ms 一批，大約每 6 批丟 1 批。這仍然是超載；加大 buffer 只延後第一次遺失，解法是加快處理、降低取樣率，或先抽取再處理。

## 沒有適當的 ADC：先做 UART 接收 DMA

實作路線允許先用 UART 接收練習同一組觀念。DMA 把每個收到的 byte 寫進 64 bytes 的循環 buffer，CPU 只在事件發生時把新到的一段搬進第五課的 RX 環形緩衝區。ring.c 與 frame.c 完全不用改，DMA 換掉的只是「每個 byte 進一次中斷」。

```text
電腦 → UART → DMA → dma_buf[64] → 事件時 service() 複製 [last_pos, pos) → RX 環形緩衝區 → 解析器
```

和 ADC 不同的地方有三個：

- 命令長度不固定。半滿與全滿只在第 32 與 64 個 byte 觸發，一個 5-byte 的命令可能永遠停在 buffer 裡。有些 UART 能偵測線路閒置一個字元時間，常稱為 IDLE；沒有這個功能時，就用 1 ms 的 tick 定時檢查。Zephyr 的非同步 UART 用「收到至少一個 byte 之後的閒置時間」產生同樣的事件。
- DMA 只回報目前位置，不回報繞了幾圈。兩次搬運之間到了 64 個以上的 byte，位置看起來和沒收到東西一樣，資料就無聲消失。所以搬運的間隔必須小於 63 個 byte 的時間，115,200 baud 下約 5.5 ms；有 1 ms 定時檢查時，間隔超過這個值就代表中斷被擋太久，整段丟掉並計數，交給協定層重新同步。
- 搬運可能被半滿、全滿、IDLE 與 tick 四種中斷觸發，它們共用 last_pos。這些中斷要設成相同優先權，或在搬運時短暫關中斷，否則彼此打斷就回到第三課的讀-改-寫問題。

## 這一課不處理的邊界

- cache 與 DMA 的一致性。有 data cache 的 MCU，例如部分 Cortex-M7 晶片，CPU 可能讀到 cache 裡的舊值，DMA 可能讀到還沒寫回記憶體的資料，需要 cache 維護操作或不經 cache 的記憶體區域。實作路線把它留到實際平台需要時再展開；桌機模型完全不模擬 cache。
- DMA 能不能存取那塊記憶體。有些晶片的部分 SRAM 不在 DMA 能到達的匯流排上，要查 reference manual 的記憶體與匯流排章節，必要時用第二課的 linker script 把 buffer 放到指定區段。
- buffer 的生命週期。DMA buffer 不能放在會返回的函式 stack 上；函式返回後 DMA 照樣在寫，寫到的是別人的 stack。用 static 或全域變數。傳輸寬度也要對上：12 位元結果用 16 位元傳輸寫進 `uint16_t` 陣列，寬度設錯，數值會錯位。

## 桌機實驗

[labs/05-dma-buffers](../labs/05-dma-buffers/) 有兩個程式：

| 檔案 | 內容 |
| --- | --- |
| [adc_dma.c](../labs/05-dma-buffers/adc_dma.c) | timer、ADC、循環 DMA 與主迴圈的模型；三種策略、就地與複製兩種處理方式；每次 CPU 讀取都檢查那一半是不是正被 DMA 寫入 |
| [uart_dma.c](../labs/05-dma-buffers/uart_dma.c) | UART 接收循環 DMA，加上半滿、全滿、IDLE 與 1 ms 定時檢查；沿用第五課的 ring.c 與 frame.c |

```sh
cd labs/05-dma-buffers
gcc -std=c11 -Wall -Wextra -Werror -O2 -o adc_dma adc_dma.c
./adc_dma
./adc_dma --test
gcc -std=c11 -Wall -Wextra -Werror -O2 -I../04-uart-pipeline -o uart_dma uart_dma.c ../04-uart-pipeline/ring.c ../04-uart-pipeline/frame.c
./uart_dma
./uart_dma --test
```

模型另外記下韌體看不到的「真相」：每一批實際是哪些取樣、CPU 每次讀取時 DMA 正在寫哪一半。測試用它判斷韌體的回報是否誠實。執行 `adc_dma` 之前，先預測：B 的主迴圈在 3 秒時卡住 35 ms，會作廢幾批？C 停住 DMA 會遺失多少筆？D 不檢查，回報的遺失是多少？E 與 F 每批要算 12 ms，各能發布幾批？

```text
case policy   mode     published lost_rep  missing  bad wrong_rd bad_own max_wait
A    drop-old in-place      1000        0        0    0        0       0        4
B    drop-old in-place       997      120      120    0        0       0    35009
C    stop     in-place       997      102      102    0        0       0    35005
D    none     in-place      1000        0        0    3       80       2    35009
E    drop-old in-place         1    39960    39960    0    18526       0    11996
F    drop-old copy           833     6680     6680    0        0       0    12000
```

`lost_rep` 是韌體自己回報的遺失取樣數，`missing` 是真相；`bad` 是發布了卻和真實取樣不符的批次；`wrong_rd` 是 CPU 讀到 DMA 正在寫的那一半的次數；`bad_own` 是發布的批次裡有這種讀取的批數。讀表時回答：

- B 卡住的 35 ms 裡累積了 4 批沒處理，為什麼只作廢 3 批？哪一批還完整，為什麼？
- C 遺失的是 102 筆，不是 40 的倍數。停住是從哪一刻到哪一刻？
- D 回報零遺失，卻有 3 批內容錯誤。用 `wrong_rd` 與 `bad_own` 推回去：哪一批是讀到一半被蓋掉，哪一批是用舊批號發布了新資料？
- E 一共讀到 DMA 正在寫的那一半 18,526 次，卻沒有發布任何錯誤批次。代價是什麼？那些被作廢的批次，內容真的都壞了嗎？
- F 的 833 和「10 ÷ 12」的預測差多少？

再試四個故障，先寫預測：

| 故障 | 你的預測 | 實際觀察 |
| --- | --- | --- |
| `./adc_dma --fault=wrong-half` | | |
| `./adc_dma --fault=early-release` | | |
| `./uart_dma --fault=no-wrap` | | |
| `./uart_dma --fault=no-gap-check` | | |

wrong-half 在事件後處理了 DMA 正在寫的那一半；丟舊策略的 head 檢查完全沒發現，因為它比對的是批次號，不是你讀了哪一半。early-release 在讀之前就歸還，停住的 DMA 立刻寫進 CPU 正在讀的那一半；這次內容碰巧還沒被蓋到，`bad` 是 0，但 `bad_own` 是 1。比對內容靠運氣，所有權檢查不靠運氣。

`uart_dma` 的三組實驗：

```text
1. 一個 5-byte 命令，之後線路安靜 100 ms
   mode       收到   最後一個 byte 到解析完成（us）
   ht+tc      否      卡在 DMA buffer 裡 5 bytes
   idle       是      86.8
   tick       是      566.0
   idle+tick  是      86.8

2. 300 個 7 或 12 bytes 的封包，間隔 0 到 3 個 byte 時間
   mode       frames seq_gaps crc_err  stuck isr_runs
   ht+tc         297        0       0     20       90
   idle          300        0       0      0      316
   tick          300        0       0      0     3090
   idle+tick     300        0       0      0     3316

3. 2000 個 12-byte 封包連續送；idle+tick；0.5 秒時擋中斷 3 ms，1.0 秒時擋 8 ms
   frames=1991 seq_gaps=9 crc_err=0 lap_events=1 suspect_max=104 truth_lost=104 max_gap=9.00 ms
```

擋 3 ms 時，被擋住的幾次事件合併成一次搬運，仍然沒有遺失；擋 8 ms 超過一圈，韌體發現並丟掉整段。改用 `--fault=no-gap-check` 再跑一次：真正遺失的 bytes 變少了，因為還在 buffer 裡的那一段被搬了出來，但 DMA 這一層回報零遺失，只有協定層的序號看得出來。

自我檢查全部通過之後，改壞一處，看哪一項檢查失敗：

- 在 adc_dma.c 的 `dma_isr` 把停住的條件 `head - tail >= 2u` 改成 `>= 3u`：C 的「內容正確而且遺失全部被計數」會失敗，表格裡 C 出現 1 批錯誤，因為 DMA 寫進了還沒歸還的那一半。
- 把主迴圈處理後的第二次檢查 `if (head - k >= 2u)` 改成 `if (0)`：E 的兩項檢查失敗，發布 834 批，其中 142 批內容錯誤。
- 把 `dma_isr` 發布 head 的 `memory_order_release` 改成 `memory_order_relaxed`：全部檢查照樣通過。單執行緒的模型看不出編譯器重排，這也是一個要記下來的發現。
- 在 uart_dma.c 把一圈檢查的門檻 `(DMA_N - 1u)` 改成 `(2u * DMA_N)`：擋 8 ms 那一項會失敗，lap_events 變成 0。

## 上板實作

有可用的 ADC 就走第一條路；沒有就走第二條。兩條都要做到「量出遺失」那一步。

ADC 路徑：

1. 從電路圖與 datasheet 找出一個可用的 ADC 通道接腳，接可變電阻或兩顆電阻分壓。先用輪詢方式讀一次，和三用電錶的電壓換算比對，記下參考電壓、解析度與取樣時間設定。
2. 設定 timer 觸發 ADC（先用 4 kHz 或更低）、DMA 循環模式寫進 `static uint16_t dma_buf[80]`、打開半滿與全滿中斷。在兩個中斷裡各切換一支 GPIO，用邏輯分析儀量間隔；應該是 10 ms，偏差代表 timer 設定錯誤。
3. 主迴圈依上面的 head 與 tail 規則處理每一批，把平均值當成第五課 `sensor_port_t` 的讀值；logger.c 不必修改。遺失數併入 GET_STATUS，批次號當作序號，讓 rx_check 統計跳號。
4. 故障注入：按一次按鈕就讓主迴圈 busy wait 35 ms，比較裝置回報的遺失、電腦看到的跳號與模型的預測。再讓每批的處理延長到 12 ms，就地與複製各跑 60 秒。
5. 用 wrong-half 的錯法跑一次。接在固定電壓上看不出差別；要轉動可變電阻，或用 PWM 加 RC 濾波產生慢速變化的訊號，才看得到批次內容前後跳動。
6. 用 debugger 停住 1 秒再繼續，記錄遺失計數的變化；DMA、ADC 與 timer 在除錯暫停時是否繼續跑，要查 reference manual。

UART 路徑：

1. 設定 UART 接收 DMA 循環模式寫進 64 bytes 的 buffer，打開半滿、全滿中斷；晶片有 IDLE 偵測就一併打開，沒有就在 1 ms tick 裡呼叫 service。所有呼叫 service 的中斷設成相同優先權。
2. 把第五課逐 byte 的 `logger_uart_rx_isr` 換成 service：複製 [last_pos, pos) 進 RX 環形緩衝區，跨過結尾時分兩段。在 service 開頭切換一支 GPIO，量 RX 線最後一個停止位到 GPIO 變化的時間。
3. 把 rx_check 產生的命令檔重複串接成幾百個命令，以 serial.ps1 連續送，同時讓主程式關中斷 8 ms 一次，記錄裝置的 lap 計數、解析器的錯誤計數，以及之後的命令是否照常回應。

| 驗收條件 | 操作 | 要留下的證據 |
| --- | --- | --- |
| 說明誰可讀寫每個 buffer | 畫出事件、處理、歸還三個時刻的所有權 | 圖、head 與 tail 的定義、對應的程式行 |
| 取樣時間由硬體決定 | 量半滿與全滿中斷的 GPIO | 間隔的最小、平均、最大值與樣本數 |
| 量出偶發超載的遺失 | 主迴圈卡 35 ms | 裝置回報的遺失、電腦看到的跳號、模型預測，三者對得上 |
| 量出長期超載 | 每批處理 12 ms，就地與複製各一次 | 兩種做法的發布數與遺失數，和模型比較 |
| 滿了的策略是明確的 | 寫下選了哪一種、為什麼 | 文件與程式註解；GET_STATUS 能查到遺失數 |
| UART DMA 替代路徑 | IDLE 或 1 ms 檢查；擋中斷超過一圈 | 短命令的延遲、lap 計數、解析器恢復後的回應 |

模型證明的是策略與計數邏輯。真實的中斷延遲、ADC 的雜訊與精度、時脈誤差，只能在板子上量。

## 先自行回答

1. 4 kHz 取樣、2 × 40 筆。CPU 最晚要在交出前半之後多久讀完？改成 2 × 100 筆呢？代價是什麼？
2. DMA 中斷設 `g_ready = 1`，主迴圈處理完清成 0，用在乒乓 DMA 上會出什麼問題？
3. 有人在全滿 callback 一開始就重新啟動 DMA，再處理資料。為什麼這是錯的？什麼情況下看起來完全正常？
4. 每批處理 12 ms，每批只有 10 ms。把 buffer 加大到 2 × 400 筆能解決嗎？
5. UART 接收 DMA 的 buffer 是 64 bytes，115,200 baud。中斷最長可以被擋多久？為什麼 DMA 的位置暫存器不能告訴你有沒有超過？

### 解析

1. 半個 buffer 的時間，10 ms。2 × 100 筆時是 25 ms；代價是每一批晚 25 ms 才能處理，RAM 從 160 bytes 變成 400 bytes。期限放寬，延遲也跟著變長。
2. 第四課的老問題：主迴圈卡住期間來了兩次事件，只看得到一次；它也不知道是哪一批、那一半有沒有已經被繞過。改用只由中斷增加的 head，主迴圈自己記住 tail。
3. 重新啟動 DMA 等於歸還，DMA 立刻開始寫 CPU 還沒讀的那一半。處理很快、DMA 第一筆還沒寫到你正在讀的位置時，結果碰巧正確，所以測試常常抓不到；桌機模型的 early-release 故障就是這個情況。
4. 不能。每批變成 100 ms、處理要 120 ms，仍然是長期超載；就地計算時每一批都會被繞過，先複製時仍然大約每 6 批丟 1 批。要讓處理速度高於來源速率。
5. 少於 63 個 byte 的時間，約 5.5 ms，而且還要扣掉上次搬運之後已經到的 bytes。位置是對 64 取餘數的值，繞一圈加幾個 byte 和只收到那幾個 byte，看起來一模一樣；只能靠時間預算保證，再用經過時間或協定層的序號檢查。

## 今日交付

在 [學習紀錄](../learning-log.md) 留下：ADC 的參考電壓、解析度、取樣時間與觸發設定；半滿與全滿中斷的間隔量測；35 ms 卡住時預測與實測的遺失；12 ms 處理時就地與複製的發布數與遺失數；選了哪一種滿了的策略與理由。走 UART 路徑的，留下短命令延遲與擋中斷超過一圈時的計數。

最後用自己的話回答：在事件、處理、歸還這三個時刻，這塊 buffer 分別歸誰？換到另一個平台時，要重查 DMA 事件的名稱、是否有循環模式、DMA 能不能存取這塊記憶體，以及有沒有 cache。下一課把資料採集器拆成幾個 RTOS task，再回答一次：這份資料現在歸誰，[第七課](06-rtos-tasks-and-queues.md)。
