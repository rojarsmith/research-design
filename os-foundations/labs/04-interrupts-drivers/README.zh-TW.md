[English](README.md) | **繁體中文** | [實作建置計畫，階段 04](../../SYLLABUS.zh-TW.md)

# 實驗 04：中斷、計時器與裝置驅動程式

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A–C）：已建置並執行**，環境是 Windows 11 上 podman 中的 Ubuntu 24.04 容器（podman machine 是一台 WSL 2 虛擬機器；這不是 WSL 2 Ubuntu 本身，其中的時間都屬於虛擬機器的數字），原始碼是 xv6-riscv 的 `fa2ddcb7` commit，與[實驗 03](../03-virtual-memory/README.zh-TW.md#固定的-xv6-commit) 固定的版本相同。[`xv6/run-xv6.sh`](xv6/run-xv6.sh) 把參考解答（沒有放進專案）套用到原始碼的複本上，並用 expect 操作 QEMU 與 GDB。輸出：[`xv6/example-output.txt`](xv6/example-output.txt)。
> - **Linux 核心模組（練習 D）：未執行。** `tickdev.c` 沒有在任何地方編譯或載入過。容器共用 WSL 2 的核心，也沒有核心標頭檔，而且沒有可用的 Linux 虛擬機器。只有讀取程式 `tickread.c` 編譯過，用來確認它能建置。練習 D 中標為「*預期*」的內容，都來自閱讀核心文件與標頭檔。
> - **Linux 使用者空間的對應版（練習 E）：已建置並執行**，在同一個容器中。輸出：[`linux/example-output.txt`](linux/example-output.txt)。
> - **Windows 觀察（練習 F）：已建置並執行**，在 Windows 11 主機上，不需要系統管理員權限，以 WinLibs 的 gcc 16.2 完成。輸出：[`windows/example-output.txt`](windows/example-output.txt)。沒有建置或載入任何驅動程式，這裡也沒有任何步驟需要 `TESTSIGNING`。

## 目標

追蹤一次裝置中斷與一次計時器中斷，從硬體一路到處理它們的程式碼，並能用 xv6、Linux 與 NT 的術語說出那段程式碼能做什麼、不能做什麼。在 xv6 中，看著一個輸入的字元從 PLIC 經過 `devintr()` 進入 UART 驅動程式，依來源統計中斷次數，再拿掉計時器強迫讓出 CPU 的能力。在 Linux 上，寫一個字元裝置，它的 `read()` 會睡眠，直到核心計時器把它叫醒，這就是階段 12 DRM 事件串流的形狀。在 Windows 上只能觀察：計時器解析度、計時器喚醒，以及系統回報的中斷與 DPC 速率。

```
UART 收到一個位元組 ──► PLIC：來源 10 待處理；對這個 hart 的 S 模式開啟；優先權 1 > 門檻 0
                   ──► hart 設定 sip.SEIP；一旦中斷開啟就進入陷阱
來自使用者模式：      uservec（trampoline）──► usertrap()
來自監督者模式：      kernelvec           ──► kerneltrap()
兩者之後：            devintr()：scause = 0x8000000000000009（中斷，原因 9）
                        irq = plic_claim()        讀取 claim 暫存器：10
                        uartintr()                確認中斷、清空接收 FIFO
                          consoleintr(c)          編輯這一行，遇到 '\n' 喚醒 consoleread()
                        plic_complete(10)         PLIC 可以再次送出來源 10

計時器：time >= stimecmp ──► sip.STIP ──► scause = 0x8000000000000005（中斷，原因 5）
                        devintr() ──► clockintr()：hart 0 執行 ticks++ 與 wakeup(&ticks)；
                                       每個 hart 寫入 stimecmp = time + 1000000
                        回傳 2 ──► usertrap() 或 kerneltrap() 呼叫 yield()：也就是搶占
```

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/intrcount.c`](xv6/intrcount.c) | 統計一次 `write()`、一次讀取輸入行的 `read()` 與 `pause(10)` 期間的中斷 | 已在此建置並執行 |
| [`xv6/preempt.c`](xv6/preempt.c) | 一個 hart 上的兩個 CPU 密集子行程：各自何時執行、被搶占幾次 | 已在此建置並執行 |
| [`xv6/uart.gdb`](xv6/uart.gdb)、[`xv6/timer.gdb`](xv6/timer.gdb) | 針對一個收到的字元，以及搶占用的計時器中斷的 GDB 腳本 | 已在此以批次模式執行 |
| [`xv6/run-xv6.sh`](xv6/run-xv6.sh) | 把你的修補檔套用到原始碼的複本，執行以上全部內容與刻意弄壞的版本；使用實驗 03 的 expect 腳本 | 已在此搭配參考解答執行 |
| [`linux/tickdev.c`](linux/tickdev.c)、[`linux/tickdev.h`](linux/tickdev.h) | 模組：一個 misc 字元裝置，以及一個定期喚醒等待讀取者的核心計時器 | 未在此編譯 |
| [`linux/tickread.c`](linux/tickread.c) | 讀取程式：每次 `read()` 的序號、時間戳記、間隔、錯過的 tick 與喚醒延遲 | 已在此編譯，未執行 |
| [`linux/Makefile`](linux/Makefile)、[`linux/run-module.sh`](linux/run-module.sh) | 模組與讀取程式的 kbuild，以及在虛擬機器中載入、操作並卸載模組的腳本 | 未在此執行 |
| [`linux/timerfd_ticks.c`](linux/timerfd_ticks.c)、[`linux/run-linux.sh`](linux/run-linux.sh) | 以 `timerfd` 在使用者空間做出同樣的阻塞讀取模式，不需要 root | 已在此建置並執行 |
| [`windows/nt_timer_res.c`](windows/nt_timer_res.c) | API 回報的計時器資訊、有無 `timeBeginPeriod` 時的 `Sleep(1)`、週期性可等待計時器 | 已在此建置並執行 |
| [`windows/nt_intr_rates.c`](windows/nt_intr_rates.c) | 從 Processor 效能計數器讀取中斷與 DPC 速率，含每個 CPU | 已在此建置並執行 |
| `*/example-output.txt` | 輸出紀錄 | 2026-11-16 記錄 |

參考解答沒有放進專案：這裡記錄的計數與搶占結果，來自練習 B 的參考修補檔，它不在儲存庫中。請自己寫。要讓 `run-xv6.sh` 執行你的成果，把它存成針對固定 commit 的修補檔 `0001-intrstat.patch`，放在 git 會忽略的 `xv6/solution/`，或用 `SOLUTION_DIR` 指向它。沒有修補檔時，腳本仍會在原版原始碼上執行練習 A，其餘步驟則印出說明後略過。

## 環境設定

- 切到固定 commit 的實驗 00 工作環境（見[實驗 03](../03-virtual-memory/README.zh-TW.md#固定的-xv6-commit)），只用一個 hart：`make CPUS=1 qemu`。請針對乾淨的原始碼製作修補檔，不要疊在你實驗 03 的修改之上。
- 對應的 MIT 實驗是 `traps`（backtrace 與以計時器驅動的 `sigalarm`）與 `net`（E1000 網路驅動程式），都是很好的額外練習，本實驗不需要它們。
- 要像這裡一樣不必手動輸入就跑完所有 xv6 部分：`sudo apt-get install expect patch`，然後 `SOLUTION_DIR=<你的修補檔目錄> sh xv6/run-xv6.sh ~/xv6-riscv`（預設是 `xv6/solution/`）。它在 `/tmp/lab04-xv6/` 建置。
- **Linux 模組：** 一台 Linux 虛擬機器（QEMU 或其他 hypervisor），並安裝目前核心的標頭檔，例如 Ubuntu 上的 `sudo apt-get install build-essential linux-headers-$(uname -r)`。不要用 WSL 2：它的核心沒有附模組標頭檔，課程也沒有規劃在那裡寫模組。模組的錯誤可能讓整個核心當掉，所以請用可以隨時丟棄的虛擬機器。
- **Linux 使用者空間：** WSL 2 Ubuntu 或那台虛擬機器，然後 `sh linux/run-linux.sh`。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc，不需要系統管理員權限。**不要為了本實驗在主力桌機上開啟 `TESTSIGNING` 或關閉安全開機。** NT 驅動程式屬於選修支線，只在 Windows 虛擬機器中進行。

## 先讀

- xv6 課本第 5 章（中斷與裝置驅動程式），以及排程那一章中關於計時器中斷的部分。
- `kernel/start.c`：`w_mideleg(0xffff)`、`w_sie(... SIE_SEIE | SIE_STIE)`，以及 `timerinit()`；後者開啟 `stimecmp`（Sstc 擴充）並要求第一次計時器中斷。
- `kernel/plic.c` 與 `kernel/memlayout.h` 中的 `PLIC*`；`kernel/uart.c`；`kernel/console.c`。
- `kernel/trap.c`：`devintr()`、`clockintr()`，以及 `usertrap()` 與 `kerneltrap()` 中 `which_dev == 2` 的那幾行。`kernel/kernelvec.S`。

## 練習

執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 追蹤一個收到的字元（xv6、GDB，已在此執行）

[`uart.gdb`](xv6/uart.gdb) 會在你輸入一個字元時停在 `consoleintr()`，印出陷阱暫存器、PLIC 的待處理位元組與 UART 的線路狀態，再抓住對應的 `plic_complete()`。手動做法：在一個終端機執行 `make CPUS=1 qemu-gdb`，在另一個終端機的同一個目錄執行 `gdb-multiarch`，輸入 `source <路徑>/uart.gdb`，然後在 xv6 主控台輸入一個字元。

**預測：** backtrace 會顯示哪一條陷阱路徑，`usertrap()` 還是 `kerneltrap()`？`scause` 是多少？hart 當時在哪個模式（`sstatus.SPP`）？`consoleintr()` 執行時，PLIC 是否仍顯示來源 10 待處理？

在此觀察到的結果，輸入的是 `echo x`（第一個字元是 `e`，101）：

```
#0  consoleintr (c=101) at kernel/console.c:148
#1  0x0000000080000a1a in uartintr () at kernel/uart.c:164
#2  0x000000008000259c in devintr () at kernel/trap.c:211
#3  0x000000008000273a in kerneltrap () at kernel/trap.c:158
#4  0x00000000800056a8 in kernelvec () at kernel/kernelvec.S:38
scause  0x8000000000000009  (interrupt bit set, cause 9: supervisor external)
sstatus 0x0000000200000120  SPP=1, so the trap came from supervisor mode
sepc    0x000000008000437e
fileread + 16 in section .text
this hart's current process: 0x800128f8
PLIC pending word 0x0c001000: 0x00000400 (bit 10 is the UART)
UART LSR 0x10000005: 0x61 (bit 0: more input waiting, bit 5: transmitter idle)
#0  plic_complete (irq=irq@entry=10) at kernel/plic.c:45
#1  0x00000000800025ca in devintr () at kernel/trap.c:223
```

- **中斷在 hart 執行核心程式碼、中斷開啟時抵達。** 這次 `sepc` 位於 `fileread()`：shell 的行程正在執行它的 `read()` 系統呼叫，`kerneltrap()` 就在那個行程的核心堆疊上執行，儘管這個字元和那個行程毫無關係。當天其他幾次執行，分別在 `pop_off()` 與 `usertrap()` 中剛重新開啟中斷之後抓到它。輸出紀錄最後那次沒有設定 `SOLUTION_DIR` 的執行，則是在 `scheduler()` 中抓到它，當時根本沒有目前行程：hart 正閒置。
- **`scause` 的最高位元有設定。** 這表示是中斷，而不是例外。原因 9 是監督者外部中斷，在這台機器上的意思是「去問 PLIC」。
- **`consoleintr()` 執行時，來源 10 仍是待處理。** UART 仍保留著 `echo x` 的其餘字元（LSR 第 0 位元），所以它的中斷線維持在高電位。`devintr()` 已經 claim 了來源 10，要等 `uartintr()` 返回後才 complete。`devintr()` 中的註解說明了這條規則：每個裝置在被 complete 之前，一次最多只能發出一個中斷。
- `devintr()` 把 claim 到的編號放在暫存器裡（GDB 說 `irq` 被最佳化掉了），所以腳本在它被交回的地方讀取：`plic_complete (irq=10)`。
- **不要從 GDB 讀取 RHR、ISR 或 PLIC 的 claim 暫存器。** 讀取它們會改變裝置的狀態。LSR 與待處理位元組可以安全讀取。

**換你做（未在此執行）：** 用 `CPUS=3`，在 `plic_claim` 設中斷點並 `finish`，找出回傳 0 的那次 claim。`plicinithart()` 在每個 hart 上都開啟了 UART。說明 `devintr()` 如何處理 irq 0，以及為什麼需要這樣處理。

### B. 依來源統計中斷（xv6，已在此執行）

自己加上計數器。建立 `kernel/intrstat.h`，內容是 [`intrcount.c`](xv6/intrcount.c) 開頭列出的結構。在 `devintr()` 中計數（UART、virtio、計時器），在 `uartintr()` 中計數收到的字元，在 `usertrap()` 中計數在使用者模式抵達的計時器中斷。在 `struct proc` 加上 `preempted` 欄位，在 `usertrap()` 與 `kerneltrap()` 中統計每一次計時器讓出，並在 `freeproc()` 中歸零。好幾個 hart 會同時處理中斷，所以請用原子加法計數。接著新增 `intrstat()` 把計數複製出來，其中 `ticks` 在 `tickslock` 保護下讀取，`preempted` 取自呼叫者行程。把 [`intrcount.c`](xv6/intrcount.c) 與 [`preempt.c`](xv6/preempt.c) 複製到 `user/`，並把兩者加進 `UPROGS`。下面的計數來自參考解答（沒有放進專案）。

以一個 hart 執行 `intrcount` 之前，先**預測**每個數字。一次寫 64 位元組的 `write()` 會有幾次 UART 中斷？輸入一行 10 個字元呢？`pause(10)` 期間有幾次計時器中斷？

在此觀察到的結果（`hello xv6` 加 Enter 由 expect 腳本一口氣輸入）：

| 動作 | UART | virtio | 計時器（其中在使用者模式） | 收到的字元 | ticks |
| --- | ---: | ---: | ---: | ---: | ---: |
| 寫 64 位元組的 `write()` | 64 | 0 | 0（0） | 0 | 0 |
| 讀取一行輸入的 `read()` | 2 | 0 | 7（0） | 10 | 7 |
| `pause(10)` | 0 | 0 | 10（0） | 0 | 10 |

- **每寫一個位元組就一次中斷。** `uartwrite()` 把一個位元組放進傳送保持暫存器、設定 `tx_busy`，然後睡眠。UART 的傳送中斷清掉 `tx_busy`，叫醒它送下一個位元組。核心自己的 `printk` 不是這樣運作：`consputc()` 呼叫 `uartputc_sync()`，在關閉中斷的情況下輪詢線路狀態。
- **十個字元只有兩次中斷。** 接收 FIFO 可以存放好幾個位元組，`uartintr()` 會一直讀到 `LSR_RX_READY` 清除為止。中斷次數量的是處理批次，不是位元組數。
- **每個 tick 一次計時器中斷，沒有一次在使用者模式。** 只有一個 hart 時，每次計時器中斷都屬於 hart 0，而只有 hart 0 會推進 `ticks`。行程整段時間都睡在核心中。**先預測，再試試：** 用 `CPUS=3` 時，每個 tick 有幾次計時器中斷？

**刻意弄壞（未在此執行）：** 在 `uartinit()` 中拿掉 `IER_TX_ENABLE`。預測 shell 開機時會印出什麼，以及 `uartwrite()` 會卡在哪裡等待。

### C. 強迫搶占的計時器中斷（xv6，已在此執行）

`preempt` 先校正一個忙碌迴圈，再 fork 兩個子行程，各自在不做任何系統呼叫的情況下忙碌約 20 個 tick 的工作量，並印出各自執行的時間。以一個 hart 執行之前，先**預測**兩個子行程的開始與結束 tick，以及各自會被搶占幾次。

在此觀察到的結果：

```
$ preempt
calibrated: 65840000 iterations per tick; each child does 20 ticks of work
  child 0: ran from tick 0 to 34 (34 ticks), preempted 17 times
  child 1: ran from tick 1 to 34 (33 ticks), preempted 17 times
both done after 34 ticks
```

兩者重疊：每個子行程執行一個 tick，在使用者模式收到計時器中斷，`usertrap()` 就呼叫 `yield()`。「20 個 tick 的工作量」實際上只花了約 17 個 tick 的 CPU，因為校正迴圈也付出了 `uptime()` 呼叫的成本。各自被搶占 17 次也符合這一點：每拿到一個 tick 的 CPU 就被搶占一次。

[`timer.gdb`](xv6/timer.gdb) 停在 `preempt` 行程的那次 `yield()`，再停在下一次 `clockintr()`。在此觀察到的結果：

```
#0  yield () at kernel/proc.c:503
#1  0x000000008000270c in usertrap () at kernel/trap.c:94
#2  0x0000003ffffff09c in ?? ()
process preempt, pid 3, interrupted at user pc 0x476:
uptime + 6 in section .text of /tmp/l4/intr/user/_preempt
scause  0x8000000000000005  (interrupt bit set, cause 5: supervisor timer)
=== the next timer interrupt, inside clockintr() ===
#0  clockintr () at kernel/trap.c:179
#1  0x00000000800025e6 in devintr () at kernel/trap.c:229
#2  0x0000000080002628 in usertrap () at kernel/trap.c:75
time 5370211
sip 0x20 before clockintr() runs (bit 5, STIP: timer interrupt pending)
sip 0x0 after clockintr() wrote the next deadline into stimecmp
```

- 第 2 層的 `0x3ffffff09c` 位於 trampoline 分頁：`uservec` 跳進 `usertrap()` 的地方。GDB 在那個位址沒有符號（實驗 01）。
- 這次中斷點觸發時，父行程還在校正（到那時已被搶占兩次），中斷是在使用者模式、`uptime()` 的 stub 中抵達的。另一次執行則是在 `main` 中抓到子行程，因為編譯器把 `spin()` 內聯到 `main` 裡面。
- 在 `stimecmp` 超過 `time` 之前，`STIP` 會一直保持設定。寫入下一個期限才會清除這個請求，正如 `clockintr()` 中的註解所說。同一段註解也說明 1000000 大約是十分之一秒。

**刻意弄壞（已在此執行）：** 從 `usertrap()` 的計時器分支中拿掉 `yield()` 與 `preempted++`。如果你的修補檔裡有參考解答的那一行 `    yield(); // lab 04: the preemption`，`run-xv6.sh` 會替你做；否則它會說明原因並略過。先**預測**。觀察到的結果：

```
  child 0: ran from tick 0 to 17 (17 ticks), preempted 0 times
  child 1: ran from tick 17 to 35 (18 ticks), preempted 0 times
```

子行程 1 要等子行程 0 做完才開始。對一個不做系統呼叫的行程來說，沒有別的東西能拿走它的 CPU。計時器中斷照樣抵達，`ticks` 照樣前進，少掉的只有 `yield()`。`kerneltrap()` 在計時器中斷時仍會讓出，所以行程在核心中、中斷開啟時仍可能被搶占：xv6 的核心程式碼也是可搶占的。

### D. 阻塞到下一個 tick 的字元裝置（Linux 虛擬機器，未執行）

讀 [`tickdev.c`](linux/tickdev.c)。接著在 Linux 虛擬機器中：

```sh
cd linux
make
sudo insmod tickdev.ko period_ms=100
./tickread -n 10
sudo rmmod tickdev
```

或執行 `sh run-module.sh`，它還會同時跑兩個讀取者、一個慢速讀取者、非阻塞讀取，並顯示阻塞中的讀取者的狀態。它的設計，以及每個部分存在的理由：

- **misc 裝置**（`misc_register()`）是主要編號為 10 的字元裝置（[major.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/major.h) 中的 `MISC_MAJOR`）。註冊後會建立 `/sys/class/misc/tickdev`，udev（或 devtmpfs）再以驅動程式指定的 `mode` 建立 `/dev/tickdev`。這就是建置計畫指出的裝置模型：一個裝置、一個類別、sysfs 中的屬性，以及由 uevent 產生的 `/dev` 節點（[driver model](https://docs.kernel.org/driver-api/driver-model/overview.html)）。
- **週期性的 `timer_list`。** `tick_fn()` 在自旋鎖保護下更新序號與 `ktime_get_ns()` 時間戳記，喚醒等待佇列，並從上一次的期限重新設定自己，讓週期不會漂移。它從計時器 softirq 執行，[kernel hacking guide](https://docs.kernel.org/kernel-hacking/hacking.html) 把它描述為一種軟體中斷。在那裡不能呼叫任何可能睡眠的函式。
- **等待佇列。** `tick_read()` 在 `wait_event_interruptible()` 中睡眠，直到序號和這個開啟的檔案上次回傳的不同為止。訊號會以 `-ERESTARTSYS` 結束等待。加上 `O_NONBLOCK` 時改為回傳 `-EAGAIN`，而 `poll()` 回報是否就緒，所以階段 08 的 `epoll` 迴圈可以使用它。
- **`copy_to_user()` 在自旋鎖之外。** hacking guide 說明這些函式可能隱含地睡眠，不能在持有自旋鎖或關閉中斷時呼叫。
- **卸載。** `fops.owner = THIS_MODULE` 讓模組在任何檔案開著時保持載入。`tick_exit()` 在 Linux 6.2 以上用 `timer_shutdown_sync()` 停止計時器，之前的版本用 `del_timer_sync()`。2026-11-16 查核：v6.1 的 `include/linux/timer.h` 沒有 `timer_shutdown_sync()`，v6.2 有，而 master 已經沒有 `del_timer_sync()` 與 `from_timer()`。模組兩者都沒有使用。

*依原始碼的預期（未觀察）：* `dmesg` 顯示 `tickdev: /dev/tickdev, minor N, one tick every 100 ms (J jiffies at HZ=H)`。`tickread` 印出的序號每次加一，`dt` 接近 100,000 µs，並取整到 jiffy；喚醒延遲在虛擬機器中會較大、較不穩定。兩個讀取者印出相同的序號與時間戳記。每次讀取後睡 250 ms 的讀取者會看到序號跳 2 或 3，「missed」欄不是 0，因為每次 `read()` 回傳的是最新的 tick。加上 `-N` 時，tick 之間的 `read()` 回傳 `EAGAIN`。`ps` 顯示阻塞中的讀取者處於 `S` 狀態。讀取者還開著 `/dev/tickdev` 時，`rmmod` 會以「in use」失敗。

**刻意弄壞（只在虛擬機器中，未在此執行）：** 在 `tick_fn()` 中加上 `msleep(1)`。預測下一個 tick 時核心記錄會說什麼，以及為什麼被怪罪的是計時器回呼，而不是讀取者。

### E. 在使用者空間做同樣的模式（已在此執行）

`timerfd_ticks` 在使用者空間做 `tickread` 對模組做的事：一個被週期性計時器喚醒的阻塞 `read()`，並回報自上次讀取以來過了幾個 tick（[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)）。第二個執行緒透過 `/proc` 觀察那個阻塞中的執行緒。在此觀察到的結果：

```
  [observer] main thread 1284 during read(): state 'S', wchan 'do_wait_intr_irq'
     seq          wake ns      dt us  missed   latency us
       1 1996792204392544          -       0         53.1
       2 1996792304380934    99988.4       0         41.5
       ...
      10 1996793104375973    99973.8       0         36.5
softirqs on all CPUs during the run: TIMER +53, HRTIMER +0

=== timerfd_ticks -n 5 -s 250 (a slow reader) ===
       1 1996793206197892          -       0         50.4
       3 1996793456708635   250510.7       1      50561.1
       6 1996793707096642   250388.0       2        949.1
```

- 阻塞中的執行緒處於 `S`（可中斷的睡眠）狀態。它的等待通道是它睡在其中的核心函式，和模組中的 `wait_event_interruptible()` 是同一類的等待。
- timerfd 的讀取回傳到期次數，所以慢速讀取者可以算出自己錯過了多少，就像 `tickread` 可以從 `seq` 算出來。對慢速讀取者而言，「latency」是從它被告知的最新期限算起，所以包含讀取者自己的睡眠時間。
- 無 root 權限的容器遮蔽了 `/proc/interrupts`（在那裡它是 `/dev/null`），所以程式改讀 `/proc/softirqs`。那些是整台 WSL 2 虛擬機器的全機計數，不是這個程式的。在你自己的虛擬機器中，比較 `/proc/interrupts` 的 `LOC` 那一行在執行前後的差異。

### F. Windows 上的中斷、DPC 與計時器（只觀察，已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o build\nt_timer_res.exe nt_timer_res.c -lwinmm
build\nt_timer_res.exe
gcc -O2 -Wall -o build\nt_intr_rates.exe nt_intr_rates.c -lpdh -lwinmm
build\nt_intr_rates.exe
```

2026-11-16 的觀察結果（Windows 組建 26300、Intel Core i7-11700、有 hypervisor、podman 的 WSL 2 虛擬機器正在執行；[完整輸出](windows/example-output.txt)，第一個程式跑了兩次）：

| 量測 | 觀察值 |
| --- | --- |
| `GetSystemTimeAdjustment` 的時間增量 | 156250 × 100 ns = 15.625 ms |
| `timeGetDevCaps` | `wPeriodMin` 1 ms、`wPeriodMax` 1,000,000 ms |
| `Sleep(1)`，100 次的中位數 | 15.36 ms [15.42]；使用 `timeBeginPeriod(1)` 時 1.53 ms [1.53]；`timeEndPeriod` 之後 15.29 ms [15.36] |
| 可等待計時器，10 ms 週期，等待 200 次 | 經過 308 個週期 [308]；間隔中位數 15.37 ms [15.33] |
| 同上，加上 `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION` | 經過 205 個週期 [205]；間隔中位數 10.21 ms [10.21] |
| `Processor(_Total)`：Interrupts/sec，安靜時 | 116,808 |
| DPCs Queued/sec，安靜時 | 49,666 |
| % Interrupt Time、% DPC Time，安靜時 | 0.5 %、0.8 % |
| 同上，但有一個執行緒以 1 ms 解析度反覆 `Sleep(1)` | 每秒 113,062 次中斷、47,320 個 DPC |

這些執行顯示了什麼、沒有顯示什麼：

- **預設的等待大約持續 15.6 ms。** `Sleep(1)` 花了一整個計時器間隔，10 ms 的週期性計時器大約每 15.4 ms 觸發一次。[timeBeginPeriod](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod) 說明自 Windows 10 version 2004 起，這個呼叫只影響呼叫它的行程；也說明 Windows 11 對於被完全遮蔽、最小化或以其他方式不可見、不可聽的擁有視窗的行程，可能不保證比系統預設更高的解析度。Learn 把 `GetSystemTimeAdjustment` 的增量定義為系統把時間調整量加到時鐘上的間隔，並沒有說它就是計時器解析度，所以它和 15.6 ms 相符只能算觀察結果。
- **可等待計時器不會告訴你錯過了幾個週期。** 200 次喚醒涵蓋了 308 個週期，所以有 108 次到期沒有產生獨立的喚醒。不同於 `tickdev` 的 `seq` 或 timerfd 的計數，等待者無從得知。[CreateWaitableTimerExW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw) 記載了高解析度旗標（Windows 10 version 1803 以上），用於 "time-critical situations when short expiration delays on the order of a few milliseconds are unacceptable"。使用它時，間隔接近 10 ms，但 200 次等待仍花了 205 個週期。
- **安靜的桌機上每秒約 117,000 次中斷**，不均勻地分布在 16 個邏輯處理器上。計數器名稱來自 `PdhAddEnglishCounterW`，它接受與語言無關的路徑（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhaddenglishcounterw)）。1 ms 的 `Sleep` 迴圈在這樣的背景下看不出差異。光靠這些計數器，無法分辨是哪些裝置、hypervisor 又占了多少。含 CPU 與 DPC／ISR 事件的 WPR 追蹤可以看出來，但它需要系統管理員權限，所以這裡沒有做。

**IRQL 與 DPC：NT 這一邊的上半部與下半部。** NT 的程式碼在某個 IRQL 下執行。Learn 的 [Managing Hardware Priorities](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities) 由低到高列出：

| IRQL | 在該處理器上被遮蔽的中斷 | 在此執行的驅動程式常式 |
| --- | --- | --- |
| `PASSIVE_LEVEL` | 無 | `DriverEntry`、`AddDevice`、`Unload`、大多數 dispatch 常式、驅動程式建立的執行緒、work item |
| `APC_LEVEL` | `APC_LEVEL` 中斷 | 部分 dispatch 常式 |
| `DISPATCH_LEVEL` | `DISPATCH_LEVEL` 與 `APC_LEVEL` 中斷 | `StartIo`、`DpcForIsr`、`CustomDpc`、`CustomTimerDpc`、`IoTimer` |
| DIRQL | 等於或低於該裝置等級的中斷 | ISR（`InterruptService`）、`SynchCritSection` |

- ISR 必須快速結束，所以它排入一個 DPC，稍後在較低的 IRQL 完成其餘工作（[DPC objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-dpc-objects)）。核心計時器也是透過 DPC 執行驅動程式碼，也就是 `CustomTimerDpc`。所以 `tickdev` 的 `timer_list` 回呼，就是 Linux 上對應計時器 DPC 的東西。
- 在 `DISPATCH_LEVEL` 以上，以非零時間等待 dispatcher 物件是致命錯誤。高於 `APC_LEVEL` 時，分頁錯誤是致命錯誤，也不能存取 paged pool。只有在 `PASSIVE_LEVEL`、非任意執行緒情境中的常式才能等待。
- 裝置位於裝置堆疊中：匯流排驅動程式建立的 PDO、恰好一個功能驅動程式，以及可選的篩選驅動程式（[device stacks](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/device-nodes-and-device-stacks)）。新的驅動程式使用 KMDF 或 UMDF，而不是 WDM（[WDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/)）。載入測試簽章的驅動程式需要 `TESTSIGNING`，可能還得關閉安全開機（[Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)）。那只能在 Windows 虛擬機器中做，絕不在主力桌機上做。

## Linux／NT 對照

| | xv6 | Linux | NT |
| --- | --- | --- | --- |
| 最先執行的程式碼 | `devintr()`，該 hart 的中斷關閉，在當下的核心堆疊上 | 硬體中斷處理常式 | DIRQL 下的 ISR |
| 延後處理 | 沒有：`uartintr()` 全部做完，`wakeup()` 讓睡眠者變成可執行 | softirq（計時器 softirq 執行 `timer_list` 回呼）、執行緒化處理常式、workqueue | `DISPATCH_LEVEL` 的 DPC，再到 `PASSIVE_LEVEL` 的 work item |
| 週期性核心計時器 | 每個 hart 的 `stimecmp` 觸發 `clockintr()` | `timer_list`（softirq）或 hrtimer | 搭配 DPC 的計時器物件 |
| 讀取者在哪裡睡眠 | 在某個通道上 `sleep()`，例如 `&cons.r` | 等待佇列，`wait_event_interruptible()` | dispatcher 物件，在 `PASSIVE_LEVEL` |

- Linux 的執行緒化中斷處理常式把工作明確拆開。[genericirq](https://docs.kernel.org/core-api/genericirq.html) 說明 `request_threaded_irq()` 的主要處理常式仍在硬體中斷情境中被呼叫，並回傳 `IRQ_WAKE_THREAD` 喚醒執行 `thread_fn` 的處理執行緒。
- 三者都不讓中斷層級的程式碼睡眠，理由彼此相關。在 xv6 中，沒有屬於它自己的行程可以拿去睡：`devintr()` 在當時的任何核心堆疊上執行，可能是排程器的堆疊，而持有超過一把鎖時 `sched()` 會 panic。在 Linux 中，hacking guide 只允許在使用者情境、沒有持有自旋鎖且中斷開啟時睡眠。在 NT 中，上面的 IRQL 規則讓 `DISPATCH_LEVEL` 以上的等待成為致命錯誤。

## 檢核

在 `NOTES.md` 中回答。

1. **分別就 xv6、Linux 與 NT，說明中斷情境中什麼可以睡眠、什麼不行，以及原因。** 用你自己的證據：練習 A 的 backtrace（哪個堆疊、哪個行程）、`tickdev.c` 中的註解，以及 IRQL 表。
2. **展示你的模組在每次 tick 時喚醒阻塞的讀取者：** 在 Linux 虛擬機器中執行 `tickread` 的輸出，序號必須連續，再加上兩個讀取者與慢速讀取者的執行結果。在還沒有虛擬機器之前，先展示 `timerfd_ticks`，並註明模組的部分還沒完成。
3. 用你看到的數值畫出一個收到的字元走過的路徑：PLIC 待處理位元、`scause`、`sstatus.SPP`、`sepc`、claim 到的 irq，以及它被 complete 的時機。
4. 說明 xv6 的搶占：是哪個中斷、哪個函式讓出 CPU，以及沒有它時 `preempt` 的輸出有什麼改變。為什麼 `kerneltrap()` 仍能搶占行程？
5. 說明為什麼 Windows 上的 `Sleep(1)` 花了約 15 ms、`timeBeginPeriod(1)` 改變了什麼，以及對誰有效。
6. `tickdev` 的慢速讀取者會看到 `seq` 的缺口，而等待可等待計時器的執行緒無法得知自己錯過了什麼。DRM 的 vblank 事件（序號與時間戳記）採用哪一種設計？合成器為什麼需要它？階段 12 與 13 會回到這一點。

## 常見失敗情況

- **還沒輸入任何東西，`b uartintr` 就在開機時停下。** 傳送中斷也會進入 `uartintr()`。改在 `consoleintr()` 設中斷點，只有收到的字元才會到那裡。
- **GDB 顯示 `irq = <optimized out>`。** 改在 `plic_complete(irq)` 讀這個值，或從 `plic_claim()` 執行 `finish`。
- **`$stimecmp` 印出 void 或 "Value can't be converted to integer"。** 在這裡，QEMU 8.2 的 gdbstub 沒有提供這個 CSR。改讀 `$sip` 與 `$time`。
- **即使保留了 yield，`preempt` 也沒有重疊。** 你用了三個 hart，所以兩個子行程平行執行。改用 `make CPUS=1 qemu`。
- **`run-xv6.sh` 中的 `patch` 失敗。** 你的修補檔是針對別的 commit 做的，或疊在你實驗 03 的修改之上。請針對固定 commit 的乾淨原始碼產生 diff。
- **`run-xv6.sh` 印出 `SKIPPED: ... needs 0001-intrstat.patch`。** 修補檔不在 `SOLUTION_DIR` 中，或檔名不同。
- **模組：`insmod: ERROR: could not insert module tickdev.ko: Invalid module format`。** 建置時用的標頭檔和正在執行的核心不同。比較 `modinfo tickdev.ko` 的 vermagic 與 `uname -r`。
- **模組：`make` 找不到 `/lib/modules/.../build`。** 沒有安裝核心標頭檔，或你在 WSL 2 上。
- **模組：`rmmod: ERROR: Module tickdev is in use`。** 還有讀取者開著 `/dev/tickdev`。
- **模組：出現 `BUG: scheduling while atomic`。** `tick_fn()` 中有東西睡眠了。
- **Windows：`undefined reference to timeBeginPeriod` 或 `Pdh...`。** 連結時加上 `-lwinmm` 與 `-lpdh`。
- **Windows：沒有呼叫 `timeBeginPeriod`，`Sleep(1)` 就已經是 1 ms。** 在 Windows 10 version 2004 之前，任何行程的請求都會提高所有行程的解析度。確認你的組建版本，以及還有哪些程式在執行。

## 後續會用到的地方

- 階段 05：`acquire()` 會關閉自己 hart 上的中斷（`push_off`），因為同一個 hart 上的中斷處理常式可能需要同一把鎖。
- 階段 06：計時器驅動的搶占、執行佇列，以及用 `timerfd` 量測喚醒延遲。
- 階段 08：在同一個 `epoll` 迴圈中對 `tickdev` 或 `timerfd` 呼叫 `poll()`。
- 階段 12 與 13：DRM 的 page flip 與 vblank 事件帶有序號與時間戳記，和 `tickdev` 的一樣。
- 階段 16：再次用到 udev 與 sysfs，這次是輸入裝置與座位。

## 下一步

[建置計畫](../../SYLLABUS.zh-TW.md)的階段 05 處理同步：鎖競爭、`sleep` 與 `wakeup`，以及 futex。
