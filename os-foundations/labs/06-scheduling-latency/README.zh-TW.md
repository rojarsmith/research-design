[English](README.md) | **繁體中文** | [實作建置計畫，階段 06](../../SYLLABUS.zh-TW.md)

# 實驗 06：排程與延遲

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A、B）：已建置並執行**，於 2026-11-16 在 Windows 11 上以 podman 執行的 Ubuntu 24.04 容器中完成（podman machine 跑在 WSL 2 上，為 rootful 模式），使用 QEMU 8.2.2 與 riscv64-linux-gnu-gcc 13.3.0，xv6-riscv 的 commit 為 [`fa2ddcb7`](https://github.com/mit-pdos/xv6-riscv/commit/fa2ddcb7ca87a77af579380a6eafd5aff163b44f)（2026-11-05）。優先權排程器的結果來自一個不隨附的參考實作。輸出：[`xv6/example-output.txt`](xv6/example-output.txt)。彩票排程的變體（練習 C）沒有撰寫，也沒有執行。
> - **Linux 部分（練習 D）：已建置並執行**，在同一個容器中跑了三次：核心 6.6.87.2-microsoft-standard-WSL2、16 個 vCPU、gcc 13.3.0。這些是虛擬機器中的數字，而虛擬機器的 vCPU 由 Windows 主機排程。`SCHED_FIFO` 在加上 `--cap-add SYS_NICE` 時可以使用，沒有加時被以 `EPERM` 拒絕。輸出：[`linux/example-output.txt`](linux/example-output.txt)。
> - **Windows 部分（練習 E）：已建置並執行**，以 WinLibs 的 gcc 16.2 直接在本機跑了兩次。輸出：[`windows/example-output.txt`](windows/example-output.txt)。
> - **WPR／WPA 錄製（練習 F）：未執行。** `wpr -start` 需要提升權限的命令提示字元，這裡沒有使用；這台機器也沒有安裝 Windows Performance Analyzer。只執行了不需提升權限的 `wpr -profiles`，用來確認設定檔名稱。
> - **機器：** Windows 11 Pro 組建 26300、Intel Core i7-11700（16 個邏輯處理器）、有 hypervisor、「平衡」電源計畫。每次執行期間機器上都同時有其他工作；每份 Windows 紀錄都記下執行前後的整體 CPU 負載（延遲量測前後為 24–48 %）。所有數字都只屬於這台機器與這幾次執行。

## 目標

合成器每個影格醒來一次，完成合成，並且必須在下一次 vblank 之前把結果交給顯示器。醒得太晚，這個影格就錯過 vblank，螢幕上會再顯示一次前一個影格。本實驗量測週期性的喚醒實際上晚了多少，以及由什麼決定：排程器的策略、執行緒的優先權、計時器的解析度，以及機器上的其他負載。你要為 xv6 寫一個優先權排程器，看著互動式行程不再排在吃 CPU 的行程後面等待。接著在 Linux 上以 `timerfd`、在預設策略與 `SCHED_FIFO` 下量測 60 Hz 的喚醒，在 Windows 上以高解析度可等待計時器、在多種執行緒優先權下量測，並解釋每張分布圖的長尾。

```
vblank              vblank              vblank              （60 Hz：間隔 16.67 ms）
  |------- 影格 -------|------- 影格 -------|
       ^ 計時器到期
       |-- 喚醒延遲 --|-- 合成 --|-- 送出 --|              準時：在下一次 vblank 顯示
       ^ 計時器到期
       |------- 喚醒延遲（長尾）-------|-- 合成 --|         太晚：錯過，掉一個影格
```

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/lab06-schedstat.patch`](xv6/lab06-schedstat.patch) | 每個行程的喚醒延遲統計、一個優先權欄位，以及 `setprio()` 與 `schedinfo()` 系統呼叫；排程器仍是輪轉式 | 已在此建置並執行 |
| [`xv6/schedtest.c`](xv6/schedtest.c) | 三個吃 CPU 的行程與一個互動式行程跑固定時間，附喚醒延遲分布圖 | 已在此建置並執行 |
| [`xv6/run.sh`](xv6/run.sh) | 建出輪轉式核心，若 `SOLUTION_DIR` 指向你的優先權核心也一併建出，並在沒有人操作主控台的情況下執行 `schedtest` | 已在此執行 |
| 參考解答（不隨附） | 練習 B 的優先權排程器 | `prio` 的輸出紀錄來自放在本儲存庫之外的參考修補檔 |
| [`linux/latency.c`](linux/latency.c)、[`linux/run.sh`](linux/run.sh) | 60 Hz 的 `timerfd` 喚醒，閒置與有負載，`SCHED_OTHER` 與 `SCHED_FIFO`，附文字分布圖 | 已在此建置並執行 |
| [`windows/latency.c`](windows/latency.c) | 60 Hz 的可等待計時器喚醒，共七列：閒置時兩種計時器，以及有負載時五種執行緒設定，附文字分布圖 | 已在此建置並執行 |
| [`xv6/example-output.txt`](xv6/example-output.txt)、[`linux/example-output.txt`](linux/example-output.txt)、[`windows/example-output.txt`](windows/example-output.txt) | 輸出紀錄 | 2026-11-16 記錄 |

## 環境設定

- **xv6。** 和實驗 05 一樣，使用乾淨的 [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) 複本，切到 commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`。把 `lab06-schedstat.patch` 套在一份新的原始碼樹上，不要疊在實驗 05 的修補檔之上：兩者都使用系統呼叫號碼 23。把 `schedtest.c` 複製到 `user/`。`xv6/run.sh` 會在暫存目錄中做這些事，並使用實驗 05 的 `xv6run.exp`：

  ```sh
  sh xv6/run.sh ~/xv6-riscv rr                                   # 輪轉式核心
  SOLUTION_DIR=~/lab06-mine sh xv6/run.sh ~/xv6-riscv rr prio    # 再加上你的排程器
  ```

  要跑 `prio` 變體，在乾淨的原始碼樹套上 `lab06-schedstat.patch` 並 commit，寫好你的排程器，再用 `git diff > ~/lab06-mine/priority-sched.patch` 存下來。
- **Linux：** gcc，以 `sh linux/run.sh` 建置並執行每一列。`SCHED_FIFO` 需要 `CAP_SYS_NICE` 或非零的 `RLIMIT_RTPRIO`（[sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)）：`sudo`、`ulimit -r`，或在容器中加上 `--cap-add SYS_NICE`。這裡的執行使用實驗 05 所述的映像檔，並以 `podman run --cap-add SYS_NICE` 啟動。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc。練習 F 需要以提升權限的命令提示字元執行 `wpr.exe`（Windows 內建；`wpr -profiles` 會列出 `CPU` 與 `GeneralProfile`），以及 [Windows Performance Toolkit](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/) 中的 Windows Performance Analyzer。

## 先讀

- xv6 課本第 7 章（排程）。`kernel/proc.c` 的 `scheduler()`、`sched()`、`yield()` 與 `wakeup()`；`kernel/trap.c` 的 `clockintr()` 與兩處 `yield()` 呼叫。計時器中斷每隔 10 MHz `time` 計數器的 1,000,000 個週期發生一次，約 0.1 s。只有 hart 0 的中斷會推進 `ticks`。
- [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)：`SCHED_OTHER` 與 `SCHED_FIFO`、權限規則，以及即時排程的節流（`sched_rt_runtime_us`）。Linux 從 6.6 開始把公平排程器由 CFS 轉向 [EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)。[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)。
- Learn：[scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)（基本優先權表）、[priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)、[CreateWaitableTimerExW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw)（`CREATE_WAITABLE_TIMER_HIGH_RESOLUTION`，Windows 10 1803 以後）。

## 練習

執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 量測輪轉式排程器（xv6）

`lab06-schedstat.patch` 讓 `scheduler()` 維持輪轉，只加上量測：

- `wakeup()` 把行程從 `SLEEPING` 改成 `RUNNABLE` 時，記下 `p->readytime = r_time()`。
- 在 `scheduler()` 切換到某個行程之前，`lat_account()` 把 `r_time() - p->readytime` 加進該行程的統計：次數、總和、最大值，以及從 10 µs 以下到 200 ms 以上的八個區間分布。`r_time()` 讀取 `time` CSR，QEMU 的 `virt` 機器讓它以 10 MHz 運行。
- `setprio(n)` 存入一個 0 到 9 的優先權（預設 1），這個排程器會忽略它；`schedinfo()` 把統計複製出來。

`schedtest 3 50 same` 讓三個吃 CPU 的行程與一個互動式行程跑 50 個 tick。互動式行程反覆呼叫 `pause(1)`，也就是睡到下一個 tick，代表按影格節奏的喚醒。`inter` 讓互動式行程要求優先權 5，`starve` 另外讓第一個吃 CPU 的行程要求 9。

**預測**（`CPUS=1`）：互動式行程大約 50 次的喚醒中，有幾次能真正執行？每次要等多久才拿到 CPU？提示：想想計時器 tick 之後，排程器的迴圈先走到哪個行程。

觀察結果（`sh xv6/run.sh ~/xv6-riscv rr`，兩次執行；[輸出紀錄](xv6/example-output.txt)是第二次）：

| `CPUS`、模式 | 50 個 tick 內互動式行程的輪數 | 等待 CPU 的平均時間 | 最大值 | 吃 CPU 行程的工作單位，第二次 |
| --- | --- | --- | --- | --- |
| 1、`same` | 17／17 | 194.9／195.1 ms | 200.8／201.2 ms | 6,864／7,005／6,658 |
| 1、`inter` | 17／17 | 194.9／195.1 ms | 200.9／201.3 ms | 7,025／6,931／6,518 |
| 1、`starve` | 17／17 | 194.8／195.1 ms | 200.9／201.2 ms | 7,159／7,300／6,864 |
| 3、`same` | 50／50 | 105／120 µs | 497／643 µs | 20,132／19,336／19,888 |
| 3、`inter` | 49／49 | 115／145 µs | 305／497 µs | 19,232／18,599／18,802 |

單一 CPU 時，兩次執行都有 16／17 次喚醒等了超過 200 ms：大約排在其他吃 CPU 的行程後面整整兩個 tick。解釋這個 17，以及為何優先權毫無作用。有三顆 CPU 時，互動式行程在一毫秒內就執行了，因為很快就有某個 hart 的計時器 tick 到來，發現它可以執行。**解釋**如果有六個吃 CPU 的行程，為何這就不再成立。

### B. 優先權排程器（xv6）

改寫 `scheduler()` 中的迴圈：

- 執行 `p->prio` 最高的 `RUNNABLE` 行程。同一等級的行程輪流執行。
- 和原本一樣，只在查看某個行程時持有它的 `p->lock`；執行之前在持有鎖的情況下再確認一次 `RUNNABLE`：別顆 CPU 可能已經拿走它了。
- 保留迴圈開頭的 `intr_on(); intr_off();`，以及沒有可執行行程時的 `wfi`。
- 在 `swtch` 之前呼叫 `lat_account(p)`，讓統計繼續運作。

請自己寫；本實驗不附解答。下面 `prio` 的各列來自一個不隨附的參考實作：對 `proc[]` 走兩輪，第一輪找出可執行行程中最高的優先權，第二輪在該等級中、從這顆 CPU 上次執行的那個行程之後，執行下一個行程。

觀察結果，`CPUS=1`，兩次執行（`SOLUTION_DIR=... sh xv6/run.sh ~/xv6-riscv prio`）：

| 模式 | 互動式行程的輪數 | 平均等待 | 最大值 | 吃 CPU 行程的工作單位，第二次 |
| --- | --- | --- | --- | --- |
| `same` | 17／17 | 194.9／195.2 ms | 200.9／202.2 ms | 6,977／7,367／6,727 |
| `inter`（互動式行程為 5） | 50／50 | 209／246 µs | 382／628 µs | 7,052／7,310／7,057 |
| `starve`（一個吃 CPU 的行程為 9） | 1／1 | 4,914／4,920 ms | 相同 | 18,696／0／0 |

- **延遲。** 優先權 5 的互動式行程每個 tick 都能執行，喚醒後約 0.2 ms 就拿到 CPU；輪轉式下約為 195 ms。吃 CPU 的行程幾乎沒有損失，因為互動式行程只需要很少的 CPU。
- **飢餓。** 優先權 9 的吃 CPU 行程從不睡眠，所以整個測試期間比它低的都沒有執行。兩次執行中，互動式行程都等了 4.9 s 才跑到第一輪，也是唯一的一輪。加入老化機制（等得越久越提高優先權，或用掉 CPU 時降低它），再跑一次 `starve`。
- **一個值得找出來的錯誤。** 參考實作的第一版讓所有等級共用一個輪轉游標。在 `inter` 下，互動式行程的執行移動了游標，使得優先權 1 的搜尋每次都從它後面開始，找到同一個吃 CPU 的行程：那一個得到 20,338 個工作單位，另外兩個是 0（記錄在輸出檔中）。解釋為何每個等級一個游標就能修好。
- `CPUS=3` 時，`inter` 兩次都有 50 輪（平均等待 182–190 µs）。`same` 分別是 50 與 48 輪，第二次有一次喚醒等了 101 ms。優先權相同時，三顆 CPU 通常能掩蓋問題，但不是每次都能。
- xv6 只在計時器 tick 時搶占。pipe 寫入造成的喚醒不會搶占吃 CPU 的行程；被喚醒的行程即使優先權較高，也要等到下一個 tick。Linux 與 NT 會在喚醒時搶占（練習 D 與 E）。在 xv6 中，你會把這件事加在哪裡？

### C. 選做：彩票排程器（xv6，此處未做）

給每個行程彩票而不是優先權（`settickets(n)`，預設 10）。每次排程時，在可執行行程的彩票總數以下抽一個隨機數，執行號碼範圍包含它的那個行程。xv6 沒有亂數產生器；在 `proc.c` 中寫一個小型線性同餘產生器就夠了。接著讓兩個吃 CPU 的行程分別持有 10 與 30 張彩票，確認它們的工作單位接近 1:3，再跑 `schedtest`，看看彩票排程對互動式行程有什麼幫助、又有什麼幫不上。本草稿沒有撰寫或執行。

### D. Linux 上的 60 Hz 喚醒，`SCHED_OTHER` 對 `SCHED_FIFO`

[`linux/latency.c`](linux/latency.c) 在 `CLOCK_MONOTONIC` 上設定一個 `timerfd`，第一次到期時間為絕對時間，週期 1/60 s，讀取 300 次（5 s），量測每次喚醒比到期時間晚了多少。`read()` 回傳自上次讀取以來的到期次數，所以大於一代表錯過了整個週期。負載執行緒以預設策略自旋，每個上線的 CPU 一個。

```sh
sh linux/run.sh            # 閒置；16 個負載執行緒；16 個負載執行緒加上 SCHED_FIFO 50
```

**預測**每一列的中位數與最差的喚醒，以及有幾次喚醒晚於 4 ms。

在容器中跑三次的觀察結果（16 個 vCPU，`SCHED_FIFO` 優先權 50）：

| 列 | 中位數 | p99 | 最大值 | 晚於 4 ms（共 300 次） |
| --- | --- | --- | --- | --- |
| `SCHED_OTHER`，閒置 | 41／110／98 µs | 84／337／257 µs | 0.81／1.12／0.28 ms | 0／0／0 |
| `SCHED_OTHER`，16 個負載執行緒 | 3,026／20／22 µs | 13.9／2.4／4.9 ms | 14.7／3.8／7.4 ms | 127／0／5 |
| `SCHED_FIFO`，16 個負載執行緒 | 22／24／23 µs | 4.0／2.7／3.4 ms | 4.4／7.6／6.2 ms | 3／1／2 |

第一次有負載的 `SCHED_OTHER` 執行的分布圖，其中錯過了一整個週期：

```
       < 50 us     86  ##################################################
  50 - 100 us       8  #####
 100 - 200 us       1  #
 200 - 500 us       5  ###
0.5 - 1 ms         13  ########
  1 - 2 ms         20  ############
  2 - 4 ms         40  ########################
  4 - 8 ms         72  ##########################################
8 - 16.7 ms        55  ################################
    > 16.7 ms       0
```

這些執行顯示了什麼、沒有顯示什麼：

- **`SCHED_FIFO` 每次都把大部分喚醒移到約 20 µs，並把晚到的喚醒減少到每次 1–3 次**，但沒有消除長尾：每次都有 4–8 ms 的。sched(7) 說 `SCHED_FIFO` 執行緒「will always immediately preempt」`SCHED_OTHER` 執行緒，所以原因應該不在客體的排程器。一個可能的原因是主機：虛擬機器的 vCPU 和 Windows 主機自己的工作共用實體處理器，而這台機器當時很忙。這一點沒有追蹤，所以仍只是假設。硬體驗證會在 Linux 實體機上重做這項量測。
- **有負載時 `SCHED_OTHER` 的中位數**，在相隔幾分鐘的執行之間從 3 ms 變成 20 µs。請回報範圍，不要只報一次。EEVDF 執行虛擬截止時間最早的合格任務，截止時間較早的任務可以搶占正在執行的任務（[EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)）。被喚醒的執行緒為何在後兩次立刻拿到 CPU、在第一次卻要等好幾毫秒，沒有查明。
- **閒置那幾列的中段反而比有負載的 `SCHED_FIFO` 慢**（三次中有兩次中位數為 98–110 µs）。閒置的 vCPU 已經停機，喚醒它比搶占一個忙碌的 vCPU 更花時間。在實體硬體上，CPU 的閒置狀態（C-state）也會造成同樣的效果。
- **即時排程節流** 維持預設值（`sched_rt_runtime_us` 為 1,000,000 中的 950,000），所以一個失控的 `SCHED_FIFO` 執行緒最多只能占用一顆 CPU 的 95 %。
- 沒有 `--cap-add SYS_NICE` 時，容器的 `CapEff` 缺少 `CAP_SYS_NICE`，`ulimit -r` 為 0，`sched_setscheduler` 回傳 `EPERM`：程式印出 `SCHED_FIFO priority 50 refused: Operation not permitted`。podman 回報自己是 rootful（`podman info` 的 Rootless: false），所以在 rootless 的環境中請重新確認。

### E. Windows 上不同優先權的 60 Hz 喚醒（已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o latency.exe latency.c
.\latency.exe          # 全部七列，每列 4 s，共約 30 s
.\latency.exe 5 7      # 只跑一列（這裡是第 7 列）5 s，用於 WPR 追蹤
```

[`windows/latency.c`](windows/latency.c) 每個週期重新設定一個高解析度可等待計時器，時間是距離固定 1/60 s 網格上下一個到期時間的剩餘時間，然後等待它，並記錄醒得多晚。負載是每個邏輯處理器一個、優先權 `THREAD_PRIORITY_NORMAL` 的自旋執行緒，位於同一個 `NORMAL_PRIORITY_CLASS` 行程中，所以每個負載執行緒的基本優先權都是 8。依照 Learn 的[基本優先權表](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)，量測執行緒分別是 8（`NORMAL`）、9（`ABOVE_NORMAL`）、10（`HIGHEST`）或 15（`TIME_CRITICAL`）。

執行前先**預測**每一列。比負載高一級的優先權夠嗎？

兩次執行、每列 240 次喚醒的觀察結果：

| 列 | 中位數 | p99 | 最大值 | 晚於 4 ms（共 240 次） | 錯過的整個週期 |
| --- | --- | --- | --- | --- | --- |
| 1. 閒置，高解析度計時器 | 336／328 µs | 0.63／0.66 ms | 1.15／1.28 ms | 0／0 | 0／0 |
| 2. 閒置，一般可等待計時器 | 7.77／7.65 ms | 15.1／15.2 ms | 15.2／15.3 ms | 180／178 | 0／0 |
| 3. 有負載，`NORMAL`（基本 8） | 13.1／9.2 ms | 35.3／20.9 ms | 36.1／22.5 ms | 224／209 | 77／18 |
| 4. 有負載，`NORMAL`，停用提升 | 13.0／34.2 ms | 35.2／91.2 ms | 50.0／97.2 ms | 216／222 | 75／346 |
| 5. 有負載，`ABOVE_NORMAL`（9） | 276／303 µs | 0.51／0.52 ms | 0.52／0.88 ms | 0／0 | 0／0 |
| 6. 有負載，`HIGHEST`（10） | 275／271 µs | 0.51／0.52 ms | 0.52／0.52 ms | 0／0 | 0／0 |
| 7. 有負載，`TIME_CRITICAL`（15） | 262／288 µs | 0.52／0.52 ms | 0.89／0.53 ms | 0／0 | 0／0 |

第一次執行的第 3 列，依程式印出的樣子：

```
       < 50 us      0
  50 - 100 us       0
 100 - 200 us       1  #
 200 - 500 us       1  #
0.5 - 1 ms          1  #
  1 - 2 ms          1  #
  2 - 4 ms         12  ######
  4 - 8 ms         39  ##################
8 - 16.7 ms       113  ##################################################
    > 16.7 ms      72  ################################
```

這些執行顯示了什麼、沒有顯示什麼：

- **比負載高一級就夠了。** 基本優先權 9 以上時，不論有沒有負載，每次喚醒都在 0.9 ms 內執行，因為就緒的較高優先權執行緒會搶占正在執行的執行緒（[scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)）。在 8 時，量測執行緒要和 16 個同等級的執行緒爭奪 16 個邏輯處理器，錯過了大部分影格。
- **優先權提升並不能穩定地救它。** Learn 說等待條件滿足的執行緒會獲得優先權提升（[priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)）。以 `SetThreadPriorityBoost` 停用提升，在第一次執行中沒有差別，在第二次執行中卻讓結果糟很多。被提升的優先權 8 執行緒為何仍要在優先權 8 的執行緒後面等好幾毫秒，沒有查明；練習 F 的追蹤可以回答（看 `NewInPri` 與 `NewPriDecr`）。
- **一般的可等待計時器** 即使在閒置的機器上，喚醒也分散在 0 到約 15.3 ms 之間。這符合「到期只在約 15.6 ms 的時鐘 tick 時處理」，而 15.6 ms 是常見的 Windows 預設值；tick 間隔本身沒有在此量測。第二次執行中有一次喚醒比到期時間*早*了 5.6 µs。解釋這怎麼可能發生。
- **高解析度計時器的下限** 是中位數約 0.3 ms，不論閒置與否。這些執行沒有顯示那 0.3 ms 花在哪裡。

### F. 用 WPR 錄製一次執行，在 WPA 中檢視（此處未執行）

這些步驟依照 Learn 的 [WPR command-line options](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-command-line-options) 與 [CPU analysis](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/cpu-analysis) 頁面。它們沒有執行：`wpr -start` 需要提升權限的命令提示字元，而撰寫本實驗的機器沒有安裝 WPA。

1. 在提升權限的命令提示字元中：`wpr -start CPU -filemode`。WPA 的 CPU Usage (Precise) 圖是由情境切換事件建立的；用 `wpr -profiledetails CPU` 確認這個設定檔會記錄它們（此處未確認）。
2. 在一般的命令提示字元中：`latency.exe 5 3`（第 3 列，有負載的 `NORMAL`），再執行 `latency.exe 5 5`（第 5 列）。記下兩者各自印出的 tid。
3. 再回到提升權限的命令提示字元：`wpr -stop lab06.etl "lab 06: 60 Hz timer under load"`。
4. 在 WPA 中開啟 `lab06.etl`。加入 **Computation > CPU Usage (Precise)**，選擇 **Timeline by Process, Thread** 預設。以 `NewThreadId` 找到 `latency.exe` 與量測執行緒。
5. 加入欄位 `ReadyTime`、`Ready (µs)`（切入時間減去就緒時間）、`NewInPri`、`NewPriDecr`（提升量）、`ReadyingProcess`、`Readying ThreadId` 與 `OldThreadId`。
6. 針對第 3 列中一次晚到的喚醒，檢查：執行緒相對於到期時間何時變成就緒？以什麼優先權切入？取代了哪個執行緒？在就緒狀態等了多久？再和第 5 列的一次喚醒比較。

把發現記進 `NOTES.md`，再回答練習 E 留下的問題：提升為何不能穩定地發揮作用，以及高解析度計時器的 0.3 ms 花在哪裡。

## Linux／NT 對照

- **優先權。** NT 依 0 到 31 的優先權排程執行緒，同一等級輪流執行，較高等級會搶占較低等級（[scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)）。基本優先權在 0 到 15 的執行緒會獲得動態提升，包括等待完成時（[priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)）。Linux 把公平排程（`SCHED_OTHER`，自 6.6 起由 CFS 轉向 EEVDF）和即時策略分開，`SCHED_FIFO` 執行緒一定會搶占 `SCHED_OTHER` 執行緒（[sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)）。
- **權限。** 在 Linux 上，`SCHED_FIFO` 需要 `CAP_SYS_NICE` 或 `RLIMIT_RTPRIO`（練習 D）。在 Windows 上，任何行程都可以在自己的優先權類別內提高自己的執行緒，練習 E 就是在沒有提升權限的情況下做的。Learn 警告不要使用 `REALTIME_PRIORITY_CLASS`，因為它會打斷處理輸入與背景磁碟寫回的系統執行緒。
- **節流。** Linux 預設每秒保留 5 % 給非即時的工作（`sched_rt_runtime_us`）。這裡引用的兩個 Learn 頁面沒有描述 NT 有這樣的保留。
- **其他策略。** [`SCHED_DEADLINE`](https://docs.kernel.org/scheduler/sched-deadline.html) 是結合 EDF 與固定頻寬伺服器（CBS）的策略：任務為每個週期申請一段執行時間，合成器每個影格的工作就可以這樣表達。[`sched_ext`](https://docs.kernel.org/scheduler/sched-ext.html) 讓 BPF 程式定義排程器，ABI 不保證穩定。
- **xv6** 只有一種策略、一種優先權，而且只在 100 ms 的 tick 時搶占。它的長尾就是 tick（練習 A）。

## 檢核

在 `NOTES.md` 中回答。

1. 用你自己的執行畫出延遲分布圖，至少包括：xv6 輪轉式對優先權式、有負載時 Linux 的 `SCHED_OTHER` 對 `SCHED_FIFO`，以及有負載時 Windows 的 `NORMAL` 對 `ABOVE_NORMAL`。
2. 解釋每一張圖的長尾。為長尾的每一段指出原因：計時器的解析度（xv6 的 100 ms tick、Windows 的一般計時器）、排在同優先權的吃 CPU 執行緒之後等待、只在 tick 時搶占（xv6）、從閒置的 CPU 或 vCPU 喚醒，以及在虛擬機器中主機對 vCPU 的排程。說明哪些原因你證明了，哪些只是懷疑。
3. 把它和顯示連起來。在 60 Hz 下一個影格持續 16.7 ms。假設合成器在 vblank 前 4 ms 醒來，需要 2 ms 完成合成與送出，所以有 2 ms 的餘裕。根據你的分布圖，每種設定下每秒會掉幾個影格？這裡練習 E 的第 3 列在 240 次中有 231–236 次喚醒晚於 2 ms，也就是幾乎每個影格；第 5 列一次也沒有。練習 D 有負載的 `SCHED_OTHER` 各列在 300 次中有 5–167 次，`SCHED_FIFO` 各列有 4–9 次。醒得太晚的合成器會錯過 vblank，前一個影格留在螢幕上，所以排程也是顯示品質的一部分。
4. 你的 xv6 優先權排程器可能造成飢餓。用 `schedtest 3 50 starve` 展示它，再展示你的修正。
5. 解釋為何 WSL 2 或容器中的數字只能彼此比較，以及 Linux 實體機上的硬體驗證會改為量測什麼（階段 13 的翻頁時間戳記）。

## 常見失敗情況

- **`patch` 回報 `syscall.h` 衝突。** 你把 `lab06-schedstat.patch` 疊在實驗 05 的修補檔之上。改用一份新的原始碼樹。
- **`schedtest` 印出 `(only N of 4 results arrived)`。** 有子行程失敗：`schedinfo()` 回傳 -1，或你的核心沒有 `setprio()`。
- **在你的優先權核心上，`schedtest` 的三種模式看起來都一樣。** 你的排程器忽略了 `p->prio`，或 `setprio()` 存到了排程器不會讀的地方。
- **你的排程器以 `sched locks` panic，或開機時卡住。** 你在持有某個 `p->lock` 的情況下離開了 `scheduler()` 的迴圈，或同時持有兩把。
- **優先權相同時，一個吃 CPU 的行程拿走所有工作。** 你的輪轉游標會在別的等級的行程執行時移動（練習 B）。
- **Linux：`SCHED_FIFO priority 50 refused: Operation not permitted`。** 加上 `CAP_SYS_NICE`（`sudo`、`--cap-add SYS_NICE`），或提高 `ulimit -r`。
- **Linux：`SCHED_FIFO` 執行期間機器沒有回應。** 從不睡眠的即時執行緒，除了節流保留的部分之外，可以讓同一顆 CPU 上的其他一切都餓死。這個程式在兩次喚醒之間會睡眠；你自己的變體也應該如此。
- **Windows：第 1 列看起來和第 2 列一樣。** `CreateWaitableTimerExW` 忽略了高解析度旗標（Windows 10 1803 之前的版本），或沒有傳入這個旗標。
- **Windows：`wpr -start` 因存取錯誤而失敗。** 從提升權限的命令提示字元執行。
- **每次執行的數字差很多。** 其他程式、電源計畫、溫度限制，以及虛擬機器所在的主機，都會反映在這些數字上。像這裡的表格一樣，回報多次執行的範圍。

## 後續會用到的地方

- 階段 08 的事件迴圈在 `epoll_wait` 中睡眠，直到某個 `timerfd` 觸發：也就是這裡量測的喚醒。
- 階段 09 的伺服器以 `timerfd` 模擬 60 Hz 的 vblank，必須在用戶端行為異常時維持節奏。
- 階段 13 以真正的翻頁事件取代計時器，硬體驗證則搭配本實驗的排程選擇量測翻頁時序。

## 下一步

[建置計畫](../../SYLLABUS.zh-TW.md)的階段 07 擴充 xv6 的檔案系統，並加入以檔案為後盾的 `mmap`。
