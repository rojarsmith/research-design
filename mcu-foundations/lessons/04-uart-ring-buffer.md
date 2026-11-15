# 第五課 UART、環形緩衝區與封包

這一課約 8 至 12 小時，可以分兩到三次完成。資料採集器在這一課第一次完整成形：定時取樣、放進緩衝區、經 UART 送出、接受設定命令，並在感測器或連線出問題時明確回報。讀完應該能解釋：為什麼一個環形緩衝區不用關中斷也安全；為什麼超載時加大緩衝區只能延後問題。

## UART 線上發生什麼

8N1 的每個 byte 是 1 個起始位、8 個資料位（最低位先送）、1 個停止位；線路閒置時維持高電位。115,200 baud 下每個 byte 約 86.8 µs。兩端各自用自己的時脈取樣，baud 的誤差會在一個 byte 內累積，允許多少誤差要查 reference manual。

| 接收錯誤 | 機制 | 通常代表什麼 |
| --- | --- | --- |
| framing error | 停止位該是高電位卻讀到低電位 | baud 不一致、雜訊，或對方送出 break |
| parity error | 同位檢查不符 | 設定不一致或雜訊；8N1 沒有同位位元 |
| overrun | 新的 byte 到了，上一個還沒被讀走 | 軟體太慢：ISR 延遲超過接收硬體的緩衝能力 |

名稱和旗標位置各家不同，機制相同。overrun 是時間預算問題：只有一個資料暫存器時，大約每 86.8 µs 就要讀走一個 byte；有 FIFO 能放寬期限，DMA 能讓 CPU 不必逐 byte 處理。錯誤旗標的清除方式也各家不同，有些晶片在錯誤旗標沒清之前會停止接收，要查手冊確認。

開發板上的 USB 轉序列通常由板載 debugger 提供。MCU 的 UART 不知道電腦有沒有打開終端機：沒有人聽的時候，TX 照樣送出，資料就這樣消失。所以「斷線」只能由協定偵測，例如電腦定期送命令、裝置在一段時間沒收到時改變狀態。MCU 的 UART 是邏輯電位；RS-232 的電壓不同，必須經過收發器，不能直接相接。

## 兩個方向，兩個環形緩衝區

```text
電腦 → RX 接收 → RX ISR（生產者）→ RX 環形緩衝區 → 主迴圈（消費者）→ 命令解析
取樣 → 主迴圈（生產者）→ TX 環形緩衝區 → TX ISR（消費者）→ TX 傳送 → 電腦
```

每個環形緩衝區只有一個生產者、一個消費者。生產者只寫 head，消費者只寫 tail，兩邊都只讀對方的索引；沒有任何變數被兩邊同時做讀-改-寫，所以不需要關中斷。若改成兩邊共用一個 `count` 並各自 `count++`、`count--`，就回到第三課的讀-改-寫問題：ISR 的遞增可能被主迴圈寫回的舊值蓋掉。

索引是一直往上加的 `uint32_t`，只在存取陣列時用 `index & (容量 − 1)` 取位置。`head - tail` 用 unsigned 減法算出目前筆數，回繞後仍正確；容量必須是 2 的次方。

還有一層是記憶體順序。生產者要先寫資料、再更新 head；消費者看到 head 前進時，必須保證資料已經寫好。C11 的 acquire/release 原子操作同時約束編譯器與硬體的重排；在單核心 MCU 上，ISR 與主迴圈之間主要靠它阻止編譯器重排，多核心或有 DMA 參與時還需要對應的硬體屏障。[C11 N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf) 這個設計只用到原子的讀和寫，不需要讀-改-寫指令；Cortex-M0+ 的指令集沒有 LDREX 與 STREX，Cortex-M4 有，所以這點在換核心時很重要。[Cortex-M0+ Generic User Guide](https://support.arm.com/documentation/dui0662/latest/)

TX 方向還有一個細節。TX 資料暫存器空的旗標只要沒有資料可送就一直成立；緩衝區空了還開著這個中斷，ISR 會不停重入。TX ISR 發現沒資料時要關掉這個中斷，主迴圈放入資料後再打開。開關中斷致能位元本身也是對共享暫存器的讀-改-寫，要用第三課的方法檢查。

## 封包格式

```text
位移   0      1     2      3 .. 3+LEN-1   3+LEN    4+LEN
       SYNC   LEN   TYPE   PAYLOAD        CRC 低   CRC 高
```

| 欄位 | 防範什麼 |
| --- | --- |
| SYNC = 0xA5 | 從位元組流裡找出封包起點；中途才開始接收時重新同步 |
| LEN，上限 32 | 雜訊造成的離譜長度可以立刻判定錯誤，不必等滿 255 bytes |
| TYPE | 區分取樣、回應與命令；未知類型明確拒絕 |
| CRC-16/CCITT-FALSE | 傳輸中的位元錯誤；涵蓋 LEN、TYPE 與 PAYLOAD |

取樣封包的 payload 是 7 bytes：序號、毫秒時間戳的低 16 位、數值、狀態，整包 12 bytes，與第一課的計算相同。多位元組欄位一律 little-endian，由程式逐 byte 組裝，不直接傳送 C struct，因為 struct 的填充和位元組順序由編譯器與平台決定。

SYNC 值也可能出現在 payload 裡。解析器在失去同步時可能把它誤認成起點，讀完一個假的封包才發現 CRC 不符。這個實作在出錯時把假起點之後收到的位元組全部重新掃描一次，所以假 SYNC 吞掉的真封包還能找回來。CRC-16 對隨機錯誤仍有大約 1/65,536 的機率誤判為正確，長度與類型檢查再擋掉一部分；對需要更高可靠度的資料，要改用更長的檢查碼。

封包收到一半就停住超過 20 ms，解析器放棄這半包。這個逾時是在主迴圈處理位元組時判斷的；主迴圈若可能卡住超過逾時，會把正常的封包誤判成停頓，此時要讓 RX ISR 記錄每個 byte 的到達時間，或把逾時設得比主迴圈的最差延遲更長。

命令有 SET_PERIOD 與 GET_STATUS 兩種。每個命令都回 ACK：成功、長度錯誤、超出範圍或未知類型。無效設定保留原值並明確回報，不默默接受，也不默默忽略。

## 緩衝區滿時怎麼辦

這一版的策略：

- TX 緩衝區放不下時，丟棄整個新的取樣封包，絕不寫入半包。
- 序號照樣遞增，電腦看到跳號就知道少了幾筆；下一個送出的封包帶 OVERFLOW 旗標；裝置的丟棄計數可用 GET_STATUS 查詢。
- 取樣封包只在剩餘空間至少有「一個封包加 16 bytes 保留量」時放入，保留量留給命令回應，所以超載時電腦仍收得到 ACK。

第一課算過：每秒 1,000 筆、每筆 12 bytes，比 115,200 baud 的上限多出 480 bytes/s。穩定狀態下每秒會丟大約 480 ÷ 12 = 40 筆。緩衝區越大，只是第一次丟棄來得越晚。

## 感測器與逾時

感測器存取拆成兩段：`start` 開始一次量測或傳輸，`poll` 檢查是否完成。主迴圈每次只呼叫一次 `poll`，超過 2 ms 沒完成就判定逾時，送出帶 SENSOR_TIMEOUT 的封包，下一個週期再試。任何 `while (!ready) {}` 都要有上限。

I2C 的位址沒有回應時會收到 NACK，可以直接回報；匯流排卡住時的恢復程序要查 I2C 規格與晶片手冊。SPI 沒有應答機制，拔掉感測器時可能讀到全 0 或全 1，要靠讀 ID 暫存器或檢查數值範圍才能發現。這兩種差異只影響板級的 `start` 與 `poll`，應用核心不需要知道。

## 桌機實驗

[labs/04-uart-pipeline](../labs/04-uart-pipeline/) 把資料採集器拆成可以在桌機上測試的幾個部分：

| 檔案 | 內容 |
| --- | --- |
| [ring.c](../labs/04-uart-pipeline/ring.c) | SPSC 環形緩衝區，C11 原子操作 |
| [frame.c](../labs/04-uart-pipeline/frame.c) | CRC、封包編碼、逐 byte 解析器 |
| [logger.c](../labs/04-uart-pipeline/logger.c) | 裝置端應用核心：取樣、命令、緩衝策略；不含任何暫存器存取 |
| [host_rx.c](../labs/04-uart-pipeline/host_rx.c) | 電腦端接收器：序號連續性與統計 |
| [test_pipeline.c](../labs/04-uart-pipeline/test_pipeline.c) | 整合測試，含模擬 UART 與模擬中斷 |
| [rx_check.c](../labs/04-uart-pipeline/rx_check.c) | 分析從序列埠存下的檔案，也能產生命令封包 |
| [budget.c](../labs/04-uart-pipeline/budget.c) | 資料率、緩衝區與時間預算計算器 |

```sh
cd labs/04-uart-pipeline
gcc -std=c11 -Wall -Wextra -Werror -O2 -DRING_SIMULATE_PREEMPTION -o test_pipeline test_pipeline.c ring.c frame.c logger.c host_rx.c
./test_pipeline
gcc -std=c11 -Wall -Wextra -Werror -O2 -o budget budget.c -lm
./budget --self-test
gcc -std=c11 -Wall -Wextra -Werror -O2 -o rx_check rx_check.c host_rx.c frame.c
./rx_check --self-test
```

`-DRING_SIMULATE_PREEMPTION` 讓 ring.c 在每個可能被中斷打斷的位置呼叫測試程式，由測試決定要不要插入一次模擬 ISR。上板時不要加這個定義。

執行 `test_pipeline` 之前先預測：

- 1 ms 週期時，第一次丟棄發生在第幾秒？用 `./budget` 或第一課的算法估計。測試裡的實測是 2.08 秒，和扣掉保留量後的預測一致。
- 錯誤示範的共用 `count` 會出什麼錯？測試讓同一份資料流同時跑正確與錯誤的兩種緩衝區。
- 從封包中途才開始接收，會損失幾個封包？

`rx_check` 可以在沒有板子時練習：`./rx_check --make-demo demo.bin` 產生一份含丟棄、位元錯誤與雜訊的資料，再用 `./rx_check demo.bin` 分析，對照 rx_check.c 開頭描述的故障，逐一找出它們在報告裡的位置。

## 上板實作

1. 先用輪詢方式送出固定字串，用一般序列終端機確認 115,200 8N1 正確。收到亂碼時，先檢查 baud、時脈設定與接腳複用。
2. 把 ring.c、frame.c、logger.c 原封不動加入韌體專案，另外寫板級程式。下面是虛擬碼，暫存器與函式名稱依晶片而定：

```c
static logger_t g_logger;

void UART_IRQHandler(void)
{
    if (/* RX 有資料 */) {
        uint8_t b = /* 讀 RX 資料暫存器；是否同時清除旗標，查手冊 */;
        logger_uart_rx_isr(&g_logger, b);
    }
    if (/* overrun 或 framing error */) {
        /* 計數，依手冊清除錯誤旗標 */
    }
    if (/* TX 資料暫存器空，且 TX 空中斷已開 */) {
        uint8_t b;
        if (logger_uart_tx_isr(&g_logger, &b)) {
            /* 寫入 TX 資料暫存器 */
        } else {
            /* 關掉 TX 空中斷，否則 ISR 會不停重入 */
        }
    }
}

int main(void)
{
    /* 時脈、GPIO、UART、1 ms tick 初始化 */
    sensor_port_t port = {board_sensor_start, board_sensor_poll, NULL};
    logger_init(&g_logger, port, tick_ms(), 0);
    /* 開 RX 中斷 */
    for (;;) {
        logger_poll(&g_logger, tick_ms());
        /* TX 緩衝區有資料時打開 TX 空中斷 */
    }
}
```

3. 感測器先用模擬資料實作 `board_sensor_start` 與 `board_sensor_poll`；資料流穩定後，再換成 I2C 或 SPI 感測器。第一版可以用 HAL 的阻塞式讀取，但一定要設逾時，並量出最長花了多久。
4. 電腦端用 [serial.ps1](../labs/04-uart-pipeline/serial.ps1) 存下原始位元組，再用 rx_check 分析。這個腳本只用 Windows 內建的 .NET 序列埠類別，已確認能解析、能列出序列埠、在埠不存在時正確報錯，但尚未在實機上收過資料。

```sh
powershell -ExecutionPolicy Bypass -File serial.ps1 -List
powershell -ExecutionPolicy Bypass -File serial.ps1 -Port COM5 -Seconds 60 -Out capture.bin
./rx_check capture.bin
./rx_check --make-cmd set-period 20 cmd.bin
powershell -ExecutionPolicy Bypass -File serial.ps1 -Port COM5 -Seconds 5 -Out reply.bin -Send cmd.bin
./rx_check reply.bin
```

`-ExecutionPolicy Bypass` 只影響這一次執行，不改變系統設定。

| 驗收條件 | 操作 | 要留下的證據 |
| --- | --- | --- |
| 主機能辨識完整封包 | 10 ms 週期收 60 秒 | 約 6,000 筆、沒有跳號、tick 間隔都是 10 ms |
| 命令可以調整週期 | 送 SET_PERIOD 20 | 收到 ACK，之後 tick 間隔變成 20 ms |
| 無效設定被拒絕 | 送 0 與 5000 | 收到 NACK，GET_STATUS 顯示週期未變 |
| 超載行為可預測 | 設 1 ms 週期 | 第一次丟棄的時間接近預測；電腦算出的遺失數等於裝置的丟棄計數 |
| 拔除感測器有逾時與明確錯誤 | 執行中拔掉、再接回感測器 | 期間封包照常送出並帶 SENSOR_TIMEOUT；接回後不必重開機即恢復 |
| 停止讀取時有明確結果 | 關掉電腦端 10 秒再重開 | 裝置持續運作；電腦看到序號跳號；解析器從中途重新同步 |

電腦端看到的時間包含 USB 與作業系統的緩衝延遲，不能拿來量 MCU 的取樣抖動；抖動用第四課的 GPIO 加邏輯分析儀方法量。

## 先自行回答

1. 115,200 baud、8N1，接收端只有一個資料暫存器。RX ISR 最晚要在多久內讀走資料？若某段程式關中斷 200 µs，會發生什麼？
2. TX 緩衝區 1,024 bytes，每秒 1,000 筆、每筆 12 bytes。第一次丟棄大約在何時？之後每秒丟幾筆？把緩衝區加大到 4 KiB 有什麼改變？
3. 電腦端用 `uint16_t` 序號，為什麼要寫成 `(uint16_t)(seq - last)`？不轉型會怎樣？
4. 失去同步時，SYNC 值出現在 payload 中。解析器怎麼恢復？代價是什麼？
5. 裝置要怎麼知道電腦已經不再接收？

### 解析

1. 約一個 byte 的時間，86.8 µs，實際期限還取決於硬體的緩衝方式。關中斷 200 µs 期間可能到達兩個以上的 byte，很可能發生 overrun；要縮短關中斷時間、使用 FIFO 或 DMA。
2. 約 1,024 ÷ 480 ≈ 2.1 秒，扣掉保留量會稍早；之後每秒丟約 40 筆。4 KiB 只把第一次丟棄延後到約 8.5 秒，穩定後的丟棄率不變。
3. `uint16_t` 相減前會先提升成 `int`，序號從 65,535 回繞到 0 時結果是負數，被當成很大的跳號。轉回 `uint16_t` 才得到模數語意下的差。
4. 解析器讀完假封包後 CRC 不符，丟掉假起點，從它後面一個 byte 開始重新掃描已收到的資料。代價是多一點運算與延遲，以及統計裡多一筆 CRC 錯誤；資料本身不會遺失。
5. 從 UART 本身無法得知。要在協定裡加上心跳：電腦定期送命令，裝置在一段時間沒收到時改變狀態，例如降低輸出或標示連線遺失。這部分留到第七階段和重啟處理一起設計。

## 今日交付

在 [學習紀錄](../learning-log.md) 留下：預測與實測的第一次丟棄時間、60 秒正常資料的 rx_check 報告、每一項驗收條件的操作與結果、感測器逾時期間的封包內容、你遇到的 UART 錯誤旗標。

最後用自己的話回答：logger.c 裡有沒有任何一行在換晶片時必須修改？如果有，它是不是應該移到板級程式？到這裡，資料採集器的主線已經完整；下一階段把輪詢搬運換成 ADC 與 DMA，重新回答「誰擁有這塊 buffer」。
