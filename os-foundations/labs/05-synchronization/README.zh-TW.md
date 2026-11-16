[English](README.md) | **繁體中文** | [實作建置計畫，階段 05](../../SYLLABUS.zh-TW.md)

# 實驗 05：同步

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A–D）：已建置並執行**，於 2026-11-16 在 Windows 11 上以 podman 執行的 Ubuntu 24.04 容器中完成（podman machine 跑在 WSL 2 上，為 rootful 模式），使用 QEMU 8.2.2 與 riscv64-linux-gnu-gcc 13.3.0，xv6-riscv 的 commit 為 [`fa2ddcb7`](https://github.com/mit-pdos/xv6-riscv/commit/fa2ddcb7ca87a77af579380a6eafd5aff163b44f)（2026-11-05）。輸出在 [`xv6/example-output.txt`](xv6/example-output.txt)。練習 C 的 GDB 步驟沒有執行。
> - **Linux 部分（練習 E–G）：已建置並執行**，在同一個容器中完成：核心 6.6.87.2-microsoft-standard-WSL2、16 個 vCPU、gcc 13.3.0。這些是虛擬機器中的數字。練習 G 的 ThreadSanitizer 在該容器中執行失敗（見該練習），所以那一步算未執行。輸出：[`linux/example-output.txt`](linux/example-output.txt)。
> - **Windows 部分（練習 H–J）：已建置並執行**，以 WinLibs 的 gcc 16.2 直接在同一台機器上完成。輸出：[`windows/example-output.txt`](windows/example-output.txt)。
> - **機器：** Windows 11 Pro 組建 26300、Intel Core i7-11700（16 個邏輯處理器）、有 hypervisor、「平衡」電源計畫。執行期間機器上同時有其他工作；每份 Windows 紀錄都記下執行前後的整體 CPU 負載（13–88 %）。下文所有數字都只屬於這台機器與這幾次執行。請自己執行這些程式，記下你自己的數字。

## 目標

量測一把有競爭的鎖、讓競爭消失，並能精確說出本實驗中每一把鎖為何正確或為何有缺陷。在 xv6 中，你要計算各 CPU 在分頁配置器的鎖上自旋了多少次、把這把鎖拆成每顆 CPU 一把、讀懂 `acquire()` 與 `push_off()` 並刻意弄壞它們，然後執行一組會遺失喚醒的 `sleep`／`wakeup`，再自己寫第二組。在 Linux 上，你讓一個以 `futex(2)` 實作的 mutex 和 `pthread_mutex` 比賽。在 Windows 上，你用 SRW lock、critical section 與 `WaitOnAddress` mutex 跑同樣的測試，並說明兩個行程共用的鎖為何不能用 `WaitOnAddress`。

```
                  快速路徑，沒有等待者              慢速路徑，必須等待                         誰能叫醒你
xv6 自旋鎖        對 lk->locked 做 amoswap.w.aq     繼續自旋，中斷關閉                         （沒有人睡眠）
xv6 sleep()       -                                 sleep_prepare(chan); release(lk); sleep()  核心中的 wakeup(chan)
Linux futex       原子的 compare-and-swap           FUTEX_WAIT(addr, 預期值)                   FUTEX_WAKE(addr)；FUTEX_*_PRIVATE：只限本行程
WaitOnAddress     interlocked compare-exchange      WaitOnAddress(addr, &預期值, 大小, ms)     WakeByAddress*：「同一行程中的另一個執行緒」
具名 mutex        沒有：每次取得都是一次呼叫        WaitForSingleObject(mutex)                 ReleaseMutex，任何持有 handle 的行程都可以
```

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/lab05-lockstat.patch`](xv6/lab05-lockstat.patch) | 在 `acquire()` 中加入每把鎖的計數器，以及 `lockstat()` 系統呼叫；把 `kmemstress` 與 `lostwake` 加進 `UPROGS` | 已在此建置並執行 |
| [`xv6/kmemstress.c`](xv6/kmemstress.c) | 多個行程盡快以 `sbrk` 取得一頁再歸還，最後印出鎖的計數 | 已在此建置並執行 |
| [`xv6/pushoff-nointr.patch`](xv6/pushoff-nointr.patch)、[`xv6/pushoff-nonest.patch`](xv6/pushoff-nonest.patch) | 弄壞 `push_off()`／`pop_off()` 的兩種方法 | 已在此建置並執行 |
| [`xv6/lostwake.c`](xv6/lostwake.c) | 以 pipe 來回傳遞，加上一個看門狗，回報遺失的喚醒 | 已在此建置並執行 |
| [`xv6/lostwake-a.patch`](xv6/lostwake-a.patch)、[`xv6/lostwake-window.patch`](xv6/lostwake-window.patch) | 一組供執行與閱讀、用於 pipe 的有缺陷 `sleep`／`wakeup`，以及一個對照組：同樣的競態視窗，搭配正確的一組 | 已在此建置並執行 |
| [`xv6/run.sh`](xv6/run.sh)、[`xv6/xv6run.exp`](xv6/xv6run.exp) | 從乾淨的原始碼樹建出每個變體，在沒有人操作主控台的情況下執行 | 已在此執行 |
| 參考解答（不隨附） | 練習 B 的每 CPU 配置器，以及練習 D 的第二組有缺陷配對 | 這兩項的輸出紀錄來自放在本儲存庫之外的參考修補檔 |
| [`linux/futex_mutex.c`](linux/futex_mutex.c) | 在競爭下比較 futex mutex、`pthread_mutex`、adaptive mutex 與自旋鎖 | 已在此建置並執行 |
| [`linux/lostwake.c`](linux/lostwake.c) | 用條件變數與直接使用 futex 時的遺失喚醒 | 已在此建置並執行 |
| [`linux/lockorder.c`](linux/lockorder.c) | AB-BA 的取鎖順序：分開跑、一起跑，以及在 ThreadSanitizer 下跑 | 已在此建置並執行；ThreadSanitizer 那一步失敗 |
| [`linux/run.sh`](linux/run.sh) | 建置並執行 Linux 部分 | 已在此執行 |
| [`windows/lockbench.c`](windows/lockbench.c) | 以 SRW、兩種 critical section、`WaitOnAddress` mutex 與自旋鎖跑同樣的競爭測試 | 已在此建置並執行 |
| [`windows/lostwake.c`](windows/lostwake.c) | 用條件變數與 `WaitOnAddress` 時的遺失喚醒 | 已在此建置並執行 |
| [`windows/crossproc.c`](windows/crossproc.c) | 跨兩個行程的 `WaitOnAddress`、具名 event、具名 mutex 與 benaphore | 已在此建置並執行 |
| [`xv6/example-output.txt`](xv6/example-output.txt)、[`linux/example-output.txt`](linux/example-output.txt)、[`windows/example-output.txt`](windows/example-output.txt) | 輸出紀錄 | 2026-11-16 記錄 |

## 環境設定

- **xv6。** 使用乾淨的 [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) 複本，切到 commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`；修補檔就是對它做的。每一組修補檔各用一份新的原始碼樹，例如 `patch -p1 < lab05-lockstat.patch`（或 `git apply`），再把使用者程式複製到 `user/`。[`xv6/run.sh`](xv6/run.sh) 會在暫存目錄中為每個變體做完這些步驟，不會動到你的複本：

  ```sh
  sh xv6/run.sh ~/xv6-riscv                     # 所有變體
  sh xv6/run.sh ~/xv6-riscv base lostwake-a     # 其中幾個
  SOLUTION_DIR=~/lab05-mine sh xv6/run.sh ~/xv6-riscv percpu lostwake-b
  ```

  `percpu` 與 `lostwake-b` 兩個變體執行的是你自己在練習 B 與 D 寫的東西。在乾淨的原始碼樹套上 `lab05-lockstat.patch` 並 commit，做出你的修改，用 `git diff > ~/lab05-mine/kalloc-percpu.patch`（或 `lostwake-b.patch`）存下來，再讓 `SOLUTION_DIR` 指向那個目錄。沒有設定時，腳本會說明它略過了這兩個變體。

  它需要實驗 00 中 MIT 的工具套件，再加上 `expect` 與 `patch`。對應的 MIT 實驗是 [`lock`](https://pdos.csail.mit.edu/6.1810/2025/labs/lock.html)（在 `xv6-labs-2025` 中執行 `git checkout lock`）。那份原始碼樹來自 2025 年，有自己的統計程式碼，用的是較舊的 `sleep(chan, lk)`；本實驗的修補檔不適用於它。
- **這裡使用的容器。** 下文的紀錄使用一個裝好上述套件、並把釘選的原始碼樹放在 `/opt/xv6-riscv` 的映像檔，從 Git Bash 這樣啟動，實驗資料夾以唯讀方式掛載：

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm -v "C:/path/to/research-design:/work:ro" oslab:24.04 \
    sh /work/os-foundations/labs/05-synchronization/xv6/run.sh /opt/xv6-riscv
  ```

  Linux 部分使用同一個映像檔，加上 `--cap-add SYS_PTRACE`，讓 `strace` 能運作。
- **Linux：** gcc，練習 E 還需要 `strace`。實驗 00 的 WSL 2 Ubuntu 也可以，它的數字同樣是虛擬機器中的數字。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc。`WaitOnAddress` 在 `API-MS-Win-Core-Synch-l1-2-0.dll` 中，所以要以 `-lsynchronization` 連結（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)）。

## 先讀

- xv6 課本：第 6 章（鎖），以及第 7 章中 sleep 與 wakeup 的小節。
- `kernel/spinlock.c` 與 `kernel/spinlock.h`；`kernel/kalloc.c`；`kernel/proc.c` 的 `sleep_prepare()`、`sleep()` 與 `wakeup()`；`kernel/pipe.c` 的 `piperead()` 與 `pipewrite()`。
- **釘選版本的 sleep API 是新的。** 自 commit [`68337b6`](https://github.com/mit-pdos/xv6-riscv/commit/68337b6fea42dfd6c6215e9e0f269d989438f349)（「more modular API and implementation for sleep/wakeup」，2026-08-11）起，呼叫者在仍持有自己的鎖時以 `sleep_prepare(chan)` 登記，接著釋放那把鎖，再呼叫不帶參數的 `sleep()`。`wakeup(chan)` 會清除每個已登記行程的 `p->chan`，不論它是否已經睡著；如果 `p->chan` 已被清除，`sleep()` 就不會睡。在那之前的原始碼樹，包括 MIT 2025 年的實驗原始碼樹與當年的 xv6 課本，用的是 `sleep(chan, lk)`。練習 D 兩種都會談到。
- [futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)，以及 Ulrich Drepper 的論文〈Futexes Are Tricky〉。`linux/futex_mutex.c` 的上鎖路徑依照論文中的第三個 mutex；解鎖時改用 exchange，論文用的是 decrement。
- Learn：[WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)、[SRW lock](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)、[critical section](https://learn.microsoft.com/en-us/windows/win32/sync/critical-section-objects)、[條件變數](https://learn.microsoft.com/en-us/windows/win32/sync/condition-variables)。

## 練習

執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 量測分頁配置器上的競爭（xv6）

`lab05-lockstat.patch` 在 `struct spinlock` 中加入兩個計數器。`acquire()` 計算成功取得鎖的呼叫次數（`#acquire()`），以及發現鎖已被持有的 `amoswap` 次數（`#spin`）；兩者都在持有鎖時更新，所以不需要原子操作。`initlock()` 會記錄每一把位於核心靜態資料區（`end` 以下）的鎖；pipe 的鎖位於 `kalloc()` 配置的分頁中，會被釋放，所以不記錄。`lockstat(1)` 把計數歸零，`lockstat(0)` 把同名的鎖加總後印出。

`kmemstress` 會 fork 出多個行程，每個行程呼叫 `sbrk(4096)` 與 `sbrk(-4096)` 各 20,000 次。前者經由 `uvmalloc()` 映射一個新分頁，而 `uvmalloc()` 會呼叫 `kalloc()`；後者解除映射，會呼叫 `kfree()`。兩者都要取得 `kmem.lock`。

```sh
sh xv6/run.sh ~/xv6-riscv base           # 或在 xv6 shell 中手動執行：
$ kmemstress                             # 以 make CPUS=3 qemu 開機
$ kmemstress 1 60000                     # 同樣的 60,000 輪，在一個行程中
```

**預測：** 有三顆 CPU 時，120,000 次 `kmem` 取得中，有幾次會發現鎖已被持有？只有一顆 CPU 時呢？還有哪一把鎖會出現自旋？為什麼？

同一個核心開機三次、每次各執行兩次 `kmemstress` 的觀察結果。[輸出紀錄](xv6/example-output.txt)是第三次開機：

| 執行 | `kmem` `#acquire()` | `kmem` `#spin`，第 1／2／3 次開機 | Tick（每個約 0.1 s） |
| --- | ---: | --- | --- |
| CPUS=3，3 個行程 | 120,061 | 99,831、56,969／54,094、45,433／58,225、43,481 | 17、15／15、14／15、15 |
| CPUS=3，1 個行程 × 60,000 | 120,021 | 0／0／0 | 36／33／34 |
| CPUS=1，3 個行程 | 120,061 | 0／0／0 | 35／32／33 |

這些結果顯示了什麼、沒有顯示什麼：

- **競爭需要兩顆 CPU 同時進入鎖。** 一個行程跑在三顆 CPU 上，或三個行程跑在一顆 CPU 上，都做了同樣的 120,000 次取得，沒有任何自旋。在單一 CPU 上，持有 `kmem.lock` 的行程不會被搶占，因為 `acquire()` 已經關閉了中斷。
- **`#spin` 計算的是迴圈次數，不是等待時間。** 一次自旋就是一次 `amoswap`，它在 QEMU 下的成本取決於 QEMU 如何在主機上模擬原子操作。只在相同環境的執行之間比較自旋次數。
- **`uart` 鎖** 在每次多 CPU 的執行中，約 165 次取得就有 5,000–70,000 次自旋；單一 CPU 時則沒有。`uartwrite()` 在把一個位元組寫進 UART 暫存器時持有 `tx_lock`，另一顆 CPU 上的 `uartintr()` 則在等它。在 `kernel/uart.c` 中找出這兩個函式，解釋為何取得次數少，自旋次數卻可能很多。
- **`proc`**（64 把鎖）每次執行約有 250,000 次取得。`scheduler()` 每走一輪都會取得每個行程的鎖。實驗 06 的練習 B 會改寫這個迴圈。

### B. 把鎖拆成每顆 CPU 一把（xv6）

依照 MIT 的 lock 實驗，把單一的空閒串列換成每顆 CPU 一個串列、一把鎖：

- `kfree()` 把分頁放到它正在執行的那顆 CPU 的串列上。
- `kalloc()` 從自己的串列取用；自己的串列空了，就從別顆 CPU 的串列偷一頁。
- `cpuid()` 只有在中斷關閉時才有意義，所以使用它的地方要包在 `push_off()`／`pop_off()` 之間。
- 絕不同時持有兩把 `kmem` 鎖。持有自己的鎖、同時去取另一顆 CPU 的鎖，可能和那顆 CPU 做同樣的事而形成死結。
- 每一把每 CPU 鎖都命名為 `"kmem"`，讓 `lockstat` 把它們加總成一行，可以直接和練習 A 比較。

請自己寫；本實驗不附解答。下面的數字來自一個一次偷一頁、不隨附的參考實作。用 `SOLUTION_DIR=... sh xv6/run.sh ~/xv6-riscv percpu` 以同樣方式執行你的版本。

參考實作的觀察結果：

| 執行 | `kmem` 鎖數 | `kmem` `#acquire()` | `kmem` `#spin`，第 1／2／3 次開機 | Tick |
| --- | ---: | --- | --- | --- |
| CPUS=3，3 個行程 | 8 | 120,068 到 120,126 | 六次執行全部為 0 | 15、15／15、15／15、16 |
| CPUS=3，1 個行程 × 60,000 | 8 | 120,028 | 0／0／0 | 36／33／43 |

自旋次數從 43,000–100,000 降到 0，但一把鎖時耗時 14–17 個 tick，八把鎖時是 15–16 個 tick。tick 很粗（0.1 s），而每一輪大部分時間都花在鎖外：`kalloc()` 與 `kfree()` 各自把分頁填滿垃圾值，`uvmalloc()` 再把它清零，每輪寫入 12 KiB。**解釋**為何消除所有競爭並沒有讓這個工作負載在這裡明顯變快，並設計一個讓差異看得出來的 `kmemstress` 修改。MIT 2025 年實驗的第二部分是一個寫入者優先的讀寫自旋鎖（`rwlktest`），適合當延伸練習；請在 MIT 的原始碼樹中做。

### C. 讀 `acquire()`，說明 `push_off()`（xv6）

在釘選的版本中，`acquire()` 依序是 `push_off()`、一個會以 `"acquire"` panic 的 `holding()` 檢查、一個對 `__atomic_exchange_n(&lk->locked, 1, __ATOMIC_ACQUIRE)` 的迴圈，以及把 `mycpu()` 存進 `lk->cpu`。`release()` 清除 `lk->cpu`，以 `__atomic_store_n(..., __ATOMIC_RELEASE)` 存入 0，再呼叫 `pop_off()`。`push_off()` 用一道 `csrrc` 指令（`rc_sstatus`）讀取並清除 `sstatus.SIE`；如果這是最外層的 `push_off()`，就把原本的狀態存進 `mycpu()->intena`，然後把 `mycpu()->noff` 加一。`pop_off()` 在中斷開著時會 panic，接著把 `noff` 減一，只有在 `noff` 回到 0 且 `intena` 顯示之前中斷是開的時候，才重新開啟中斷。

先從原始碼回答，再檢查：

1. 在 `kernel/kernel.asm` 中找到 `acquire`。`spinlock.c` 的註解說 exchange 會變成 `amoswap.w.aq`，release 的儲存會變成 `fence rw,w` 加上 `sw`。你的編譯器產生的是這樣嗎？
2. 如果中斷處理常式也會取得某把自旋鎖，為何 CPU 持有那把鎖時必須關閉中斷？在 `trap.c` 中舉出一把這樣的鎖。
3. 為何 `push_off()` 要在呼叫 `mycpu()` *之前*關閉中斷？（註解有說明。）
4. 為何用計數器加上一個保存的旗標，而不是在 `acquire()` 中 `intr_off()`、在 `release()` 中 `intr_on()`？找出 xv6 同時持有兩把自旋鎖的一個地方。
5. `sched()` 除非 `mycpu()->noff == 1`，否則會 panic。為何剛好是一？

用 GDB 與 `CPUS=1`（此處沒有執行）：`b wakeup`、`c`，在其中執行 `p cpus[0].noff` 與 `p cpus[0].intena`，並在一次 `push_off()` 前後執行 `p/x $sstatus`。

**刻意弄壞，兩次。** 先預測會發生什麼，再手動套用各修補檔，或執行 `sh xv6/run.sh ~/xv6-riscv pushoff-nointr pushoff-nonest`：

- [`pushoff-nointr.patch`](xv6/pushoff-nointr.patch)：`push_off()` 讀取 `sstatus`，但不再清除 `SIE`。
- [`pushoff-nonest.patch`](xv6/pushoff-nonest.patch)：`pop_off()` 每次釋放都重新開啟中斷，即使外層的鎖還被持有。

接著也刪掉 `pop_off()` 中的 `intr_get()` 檢查，兩者各再跑一次（`pushoff-nointr-nocheck` 與 `pushoff-nonest-nocheck` 兩個變體用 `sed` 做這件事）。以 `kmemstress` 作為第一個指令、做兩組執行的觀察結果：

| 修補檔 | CPUS=1，第 1 組／第 2 組 | CPUS=3，第 1 組／第 2 組 |
| --- | --- | --- |
| `pushoff-nointr` | `panic: pop_off - interruptible`／相同 | 相同／相同 |
| `pushoff-nonest` | `ppanic: pop_off - interruptible`／相同 | 相同／相同 |
| `pushoff-nointr`，移除檢查 | `ppanic: acquire`／相同 | `panic: sched interruptible`／`panic: acquire` |
| `pushoff-nonest`，移除檢查 | `panic: acquire`／`ppanic: acquire` | `ppanic: acquire`／相同 |

沒有一次執行到達 shell 提示字元；每次 panic 都發生在開機期間。xv6 在三個地方檢查這個不變條件（`pop_off`、`acquire` 的 `holding()` 測試、`sched`），所以壞掉的核心會 panic，而不是卡住。對每一個 `panic: acquire`，描述是哪個中斷在它的 CPU 持有某把鎖時到來，接著又去取那把鎖。`ppanic` 中重複的 `p` 沒有深入調查。一個可能的解讀是：第一次 panic 印出訊息時來了一個中斷，引發第二次 panic，把訊息接在第一次的 `p` 之後。想確定的話，用 GDB 檢查。

### D. 遺失一次喚醒，再解釋真正的配對為何不會（xv6）

[`xv6/lostwake.c`](xv6/lostwake.c) 讓兩個行程透過兩條 pipe 來回傳遞 2,000 輪。每次讀取發現 pipe 是空的，就在 `piperead()` 中睡眠，直到另一端的 `pipewrite()` 呼叫 `wakeup(&pi->nread)`。只要遺失一次喚醒，兩個行程就會永遠互相等待。一個看門狗行程在 100 個 tick 後把它們殺掉，它們的結束狀態 -1 表示看門狗不得不出手。

這些修補檔在 `pipe.c` 中加入 `race_window()`：在「讀取者已決定睡眠」與「讀取者已經睡著」之間自旋 100 µs，讓競態頻繁到看得見：

- **配對 A**（[`lostwake-a.patch`](xv6/lostwake-a.patch)）：讀取者是正確的，但 `pipewrite()` 呼叫 `wakeup_asleep()`，它只喚醒已經是 `SLEEPING` 的行程。處於 `sleep_prepare()` 與 `sleep()` 之間的讀取者保留著 `p->chan`，照樣睡下去。這個檢查正是舊版原始碼樹中 `wakeup()` 的做法。
- **配對 B，由你來寫：** 讓 `wakeup()` 保持正確，改成讓 `piperead()` 在呼叫 `sleep_prepare()` *之前*就釋放 pipe 鎖，兩者之間同樣放 `race_window()`。在這段期間執行的寫入者找不到任何已登記的行程。把它存成 `lostwake-b.patch` 給 `run.sh` 使用，做法如環境設定所述。下面配對 B 的結果來自一個不隨附的參考版本。
- **對照組**（[`lostwake-window.patch`](xv6/lostwake-window.patch)）：同樣 100 µs 的視窗，搭配正確的配對。

先讀 `lostwake-a.patch`。**預測**三者中哪些會卡住，再執行 `SOLUTION_DIR=... sh xv6/run.sh ~/xv6-riscv window lostwake-a lostwake-b`。以 `CPUS=3`、每個核心開機三次、每次開機執行三次 `lostwake`（第一列除外）的觀察結果：

| 核心 | 第 1／2／3 次開機 |
| --- | --- |
| 只有 `lab05-lockstat.patch`（每次開機執行一次） | 分別在 6／5／5 個 tick 內完成 |
| ＋對照視窗 | 11、10、10／10、9、10／10、10、10 個 tick 內完成 |
| ＋配對 A | 九次全部卡住；一次撐過第 1,000 輪、一次撐過第 500 輪才卡住，其餘七次都在第 500 輪之前卡住 |
| ＋配對 B（參考版本，不隨附） | 九次全部卡住，每次都在第 500 輪之前 |

接著解釋兩種正確的設計：

- **釘選的版本：** 讀取者在持有 `pi->lock` 時呼叫 `sleep_prepare()`。寫入者需要 `pi->lock` 才能放入資料，所以它無法在讀取者檢查與登記之間執行。讀取者釋放 `pi->lock` 之後，寫入者隨時可能執行，但它的 `wakeup()` 會在持有 `p->lock` 時清除讀取者的 `p->chan`，而 `sleep()` 在睡眠前也是在持有 `p->lock` 時檢查 `p->chan`。配對 A 弄壞了後半段，配對 B 弄壞了前半段。
- **舊版原始碼樹的 `sleep(chan, lk)`：** `sleep()` 在釋放 `lk` *之前*先取得 `p->lock`，再設定 `p->chan` 與 `SLEEPING`。`wakeup()` 需要 `p->lock` 才能查看這個行程，所以它看到的若不是還沒決定睡眠的行程（它會在持有 `lk` 時看到新資料），就是已經 `SLEEPING` 的行程。如果你的原始碼樹是這個 API，就在那裡寫有缺陷的配對：複製一份 `sleep()`，讓它在取得 `p->lock` 之前就釋放 `lk`，只給 `piperead()` 用，並加上同樣的視窗。先預測，再確認它會卡住。

兩種設計遵循的規則，和練習 F 與 I 中的 futex 與 `WaitOnAddress` 一樣：「我還該睡嗎？」的檢查與實際睡下的動作，相對於喚醒者必須是不可分割的。

### E. futex mutex 對上 `pthread_mutex`（Linux）

[`linux/futex_mutex.c`](linux/futex_mutex.c) 讓每個執行緒跑同樣的迴圈：上鎖、對共享資料做一段短的臨界區（約占每輪的五分之一）、解鎖，再做私有的工作。它的 futex mutex 有三種狀態：0 空閒、1 被持有、2 被持有且可能有等待者。`lock` 先嘗試把 0 compare-and-swap 成 1；失敗的話就把這個字設為 2，並呼叫 `FUTEX_WAIT(addr, 2)`，只有在核心檢查時這個字仍為 2 才會睡眠。`unlock` 把這個字 exchange 成 0，只有舊值為 2 時才呼叫 `FUTEX_WAKE(addr, 1)`（[futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)）。

```sh
sh linux/run.sh
```

如果想自己實作這個 mutex，先根據 futex(2) 寫出你自己的 `lock` 與 `unlock`，再去讀檔案中的 `futex_lock()`，然後換上你的版本比較。

**預測：** 在 16 個執行緒時，哪一把鎖最快？每一把的時間花在哪裡（`sys` 或 `user`）？有多少次 `FUTEX_WAKE` 找不到人可以喚醒？

在容器中（16 個 vCPU）第二次執行、每格 0.25 s 的觀察結果；第一次執行在[輸出檔](linux/example-output.txt)中：

| 鎖，16 個執行緒 | Mops/s | sys ms | user ms | 自願情境切換 | 每千次操作的 `FUTEX_WAIT` | 每千次操作的 `FUTEX_WAKE` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `pthread_mutex`（預設） | 4.04 | 2,756 | 372 | 103,040 | | |
| `PTHREAD_MUTEX_ADAPTIVE_NP` | 4.58 | 1,432 | 1,772 | 102,044 | | |
| futex mutex | 4.71 | 3,213 | 401 | 80,333 | 398 | 483 |
| futex mutex，先自旋 50 次 | 4.03 | 371 | 3,129 | 37,453 | 199 | 231 |
| 自旋鎖 | 5.39 | 3 | 3,934 | 3 | | |
| 一個執行緒，任何鎖 | 6.26–6.54 | 0 | 約 250 | 2 | 0 | 0 |

- **沒有競爭，就沒有系統呼叫。** 只有一個執行緒時，每一把鎖在整格中都只有 2 次自願情境切換，futex mutex 一次 futex 呼叫都沒有。
- **會睡眠的鎖把競爭的代價花在核心裡**：預設 mutex 與 futex mutex 花了 2.8–3.2 s 的系統時間，使用者時間約 0.4 s。先自旋（adaptive、futex + spin）會把這段時間移到使用者模式。純自旋鎖從不睡眠，它最慢的執行緒做的工作不到最快者的一半（min/max 0.48）。
- **`FUTEX_WAKE` 比 `FUTEX_WAIT` 多。** 狀態 2 代表「可能有等待者」：被喚醒的執行緒取得鎖時會把這個字設回 2，因為它無法知道是否還有其他人在等，所以它自己解鎖時即使已經沒人可叫醒，也會發出一次喚醒呼叫。
- **以 `strace -c` 在 8 個執行緒下跑 0.5 s**，`pthread_mutex` 有 19,228 次 futex 呼叫（5,930 次錯誤），futex mutex 有 20,792 次（7,654 次錯誤）。這些錯誤大多是 `FUTEX_WAIT` 因為那個字已經改變而回傳 `EAGAIN`。解釋為何這個錯誤正是防止遺失喚醒的機制在運作。
- **拆鎖**：像練習 B 那樣每個執行緒一把鎖，16 個執行緒時 `pthread_mutex` 從 4.10 提高到 63.41 Mops/s，futex mutex 從 4.77 提高到 70.41。
- 大多數鎖的吞吐量在兩個執行緒時達到最高，之後就下降，即使臨界區只占迴圈的五分之一。**解釋**每次取得鎖時，不論誰搶到，有什麼東西要在核心之間搬移。

### F. 條件變數與 futex 的遺失喚醒（Linux）

[`linux/lostwake.c`](linux/lostwake.c) 是練習 I 中 [`windows/lostwake.c`](windows/lostwake.c) 的 Linux 雙胞胎。兩個執行緒把一個輪次來回交接 20,000 次。每次等待都有 50 ms 的逾時；等待逾時時，如果輪次已經屬於等待者，就算一次遺失喚醒。一個模式累計 20 次後就停止。模式 1 與 4 有缺陷，2 與 3 沒有：

1. `pthread_cond`，在取得 mutex *之前*測試輪次。
2. `pthread_cond`，在持有 mutex 時測試輪次。
3. 以剛看到的值呼叫 `FUTEX_WAIT`。
4. 一個 futex，通知者在存入新輪次*之前*就呼叫 `FUTEX_WAKE`。

觀察結果（第二次執行；競態視窗是測試與等待之間的忙碌自旋）：

| 模式 | 5 µs 視窗：輪數、遺失 | 沒有視窗：輪數、遺失 |
| --- | --- | --- |
| 1. 在 mutex 外測試 | 146 輪，遺失 20 次，停止 | 1,025 輪，遺失 20 次，停止 |
| 2. 在 mutex 內測試 | 20,000 輪，遺失 0 次 | 20,000 輪，遺失 0 次 |
| 3. futex，使用看到的值 | 20,000 輪，遺失 0 次 | 20,000 輪，遺失 0 次 |
| 4. 先喚醒再發布 | 70 輪，遺失 20 次，停止 | 699 輪，遺失 20 次，停止 |

模式 3 和練習 D 中 xv6 的 `sleep()` 是同一條規則。futex(2) 說明 `FUTEX_WAIT` 讀取那個字、與預期值比較、開始睡眠這三件事，相對於同一個字上的其他 futex 操作是不可分割的。所以在你的測試與呼叫之間發生的儲存與喚醒，會讓這次呼叫立即返回。模式 4 顯示了比較也保護不了的情況：喚醒比它所宣告的改變更早送出。

### G. 取鎖順序與 lockdep（Linux）

[`linux/lockorder.c`](linux/lockorder.c) 在一個執行緒中先取鎖 A 再取 B，在另一個執行緒中先取 B 再取 A。

- 分開執行時什麼事都沒有：`thread 1: took A then B`、`thread 2: took B then A`。
- 一起執行時（`./lockorder together`），每個執行緒持有自己的第一把鎖，以 `pthread_mutex_timedlock` 等第二把鎖 1 s 後放棄：兩者都印出 `deadlock`。
- **未執行：** 在 ThreadSanitizer（`-fsanitize=thread`，gcc 13.3）下，程式以 `FATAL: ThreadSanitizer: unexpected memory mapping` 中止。容器的核心設定 `vm.mmap_rnd_bits = 32`，而 gcc 13 的 ThreadSanitizer 無法應付這麼大的位址空間隨機化。以 `setarch -R` 關閉隨機化，被容器的 seccomp 設定拒絕（`Function not implemented`）。在一台能把 `vm.mmap_rnd_bits` 降到 28、或能執行 `setarch -R` 的 Linux 機器上，預期 ThreadSanitizer 會對第一次、無害的那次執行回報 `lock-order-inversion (potential deadlock)`。

這個回報正是重點：[lockdep](https://docs.kernel.org/locking/lockdep-design.html) 與 ThreadSanitizer 都會記錄取鎖的順序，在第二種順序第一次出現時就回報循環，即使兩個執行緒實際上從未相遇。以 xv6 來說，`wait_lock` 必須在任何 `p->lock` 之前取得（見 `proc.c` 中 `wait_lock` 上方的註解）。寫下如果 `kwait()` 先取 `p->lock` 再取 `wait_lock` 會造成的死結，以及在以 `CONFIG_PROVE_LOCKING` 建置的 Linux 核心中，你預期 lockdep 會給出的回報。

### H. 在 Windows 上做同樣的競爭測試（已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o lockbench.exe lockbench.c -lsynchronization
.\lockbench.exe
```

[`windows/lockbench.c`](windows/lockbench.c) 以六種鎖跑練習 E 的工作負載。它的 `WaitOnAddress` mutex 就是 futex mutex，改用 `WaitOnAddress(addr, &two, 4, INFINITE)` 與 `WakeByAddressSingle`。

看結果之前，先**預測**這六種鎖在 16 個執行緒時的排名。

兩次執行、每格 0.25 s 的觀察結果（[完整輸出](windows/example-output.txt)）：

| 鎖，16 個執行緒 | Mops/s，第 1 次 | Mops/s，第 2 次 | kernel ms／user ms，第 1 次 |
| --- | ---: | ---: | --- |
| `SRWLOCK` | 5.39 | 5.36 | 125／4,078 |
| `CRITICAL_SECTION` | 2.36 | 2.16 | 312／3,438 |
| critical section，spin count 4000 | 2.73 | 2.63 | 141／3,547 |
| `WaitOnAddress` mutex | 4.37 | 4.43 | 250／3,125 |
| `WaitOnAddress` mutex，先自旋 50 次 | 3.66 | 3.78 | 109／3,359 |
| 自旋鎖 | 3.05 | 2.76 | 125／3,172 |
| 一個執行緒，任何鎖 | 5.76–6.67 | 5.89–6.45 | |

- **拆鎖**：每個執行緒一把鎖，讓 `SRWLOCK` 從 5.40 提高到 77.64 Mops/s（第 2 次：5.49 到 73.08），`WaitOnAddress` mutex 從 4.35 提高到 78.34（第 2 次：4.39 到 78.88）。
- **公平性：** 32 個執行緒時，SRW 最不忙的執行緒在兩次執行中都完成 0 次迭代（min/max 0.00）；第 2 次執行中，自旋鎖與設了 spin count 的 critical section 也一樣。Learn 說 SRW lock「are neither fair nor FIFO」（[SRW lock](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)）。
- **核心時間在 Windows 上一直很低**，每一種鎖最多 625 ms；而練習 E 中 Linux 會睡眠的 mutex 大部分 CPU 時間花在核心裡。這裡引用的 Learn 頁面沒有說明 `WaitOnAddress` 或 SRW lock 如何等待，`GetProcessTimes` 也沒有說明它如何取樣。請把這一點當成問題，而不是結論。帶有取樣堆疊的 WPR 追蹤（實驗 06）可以顯示 CPU 時間花在哪裡。
- `WaitOnAddress` mutex 的 `waits/1k` 超過 1,000，代表 `WaitOnAddress` 呼叫比鎖操作還多。其中許多呼叫立即返回，因為那個字已經不是 2。Learn 也允許虛假返回（[WaitOnAddress 頁面](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress) Remarks 中的 Note），所以迴圈要重新檢查。

### I. Windows 上的遺失喚醒（已在此執行）

```powershell
gcc -O2 -Wall -o lostwake.exe lostwake.c -lsynchronization
.\lostwake.exe
.\lostwake.exe 20000 0
```

四種模式和練習 F 相同，改用 `SleepConditionVariableSRW` 與 `WaitOnAddress`。觀察結果：

| 模式 | 5 µs 視窗：輪數、遺失、時間 | 沒有視窗：輪數、遺失、時間 |
| --- | --- | --- |
| 1. 條件變數，在鎖外測試 | 30、20，1,303 ms 後停止 | 73、20，1,343 ms 後停止 |
| 2. 條件變數，在鎖內測試 | 20,000、0，446 ms | 20,000、0，24 ms |
| 3. `WaitOnAddress`，使用看到的值 | 20,000、0，107 ms | 20,000、0，11 ms |
| 4. `WaitOnAddress`，先喚醒再發布 | 24、20，1,238 ms 後停止 | 38、20，1,221 ms 後停止 |

有缺陷的模式即使沒有額外的視窗，也在幾十輪之內就遺失了喚醒。這種錯誤在正式環境中會表現為一個卡住的執行緒，直到某個不相干的事件把它叫醒。

### J. 跨行程的鎖為何不能用 `WaitOnAddress`（已在此執行）

```powershell
gcc -O2 -Wall -o crossproc.exe crossproc.c -lsynchronization
.\crossproc.exe
```

兩個行程映射同一個以分頁檔為後盾的 section（一個具名的 file mapping）。在那一頁上的原子指令可以跨行程運作，問題只在於喚醒。Learn 說，等待者會在「another thread in the same process」呼叫 `WakeByAddressSingle` 或 `WakeByAddressAll` 時、或逾時到期時返回（[WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)）。兩次執行的觀察結果：

| 第 1 部分：一個等待者、一個喚醒者 | 第 1 次 | 第 2 次 |
| --- | --- | --- |
| a. 同一行程，`WaitOnAddress`／`WakeByAddressAll` | 喚醒後 0.031 ms 察覺 | 0.014 ms |
| b. 兩個行程，`WaitOnAddress`／`WakeByAddressAll` | 喚醒後 1,787.8 ms 才察覺，也就是它 2 s 的逾時到期時 | 1,798.7 ms |
| c. 兩個行程，具名的自動重設 event | `SetEvent` 後 0.018 ms 被喚醒 | 0.026 ms |

| 第 2 部分：兩個行程、一個計數器、1 s | 操作次數，父＋子，第 1 次 | 第 2 次 | 等待次數，第 1／2 次 | 因 20 ms 逾時才結束的等待 |
| --- | --- | --- | --- | --- |
| 共享頁面中的 `WaitOnAddress` mutex | 2,849,646 + 2,385,180 | 2,488,573 + 2,584,878 | 163／124 | 33／33 |
| 具名 mutex（`CreateMutexW`） | 131,577 + 131,618 | 124,558 + 124,581 | 未計數 | |
| benaphore：interlocked 計數加上具名 semaphore | 399,078 + 400,104 | 289,199 + 289,666 | 234,416／235,051 | |

每個計數器都正確，所以三者都提供了互斥。差別在於等待：

- `WaitOnAddress` mutex 看起來很快，但兩次執行中，一秒內都有 33 次等待只因 20 ms 逾時到期才結束，等於鎖空著時睡了 0.66 s。另一個行程的 `WakeByAddressSingle` 從未送達它們。如果像練習 H 那樣用 `INFINITE`，另一個行程中的等待者會一直睡，直到它自己行程中有東西剛好叫醒它；在這個程式中，那永遠不會發生。
- 具名 mutex 在任何情況下都能用，因為它是核心的 dispatcher 物件，但每次取得、每次釋放都是一次系統呼叫：兩個行程合計每秒約 250,000 次迭代，每次約 4 µs。
- benaphore 把沒有競爭的路徑留在使用者模式（共享頁面中的一次 interlocked 遞增），只有必須等待時才使用具名 semaphore。它是 futex mutex 的跨行程對應版本，由一個核心物件代替 futex。

Linux 不需要這樣拆分。futex(2) 描述了透過共享記憶體在行程之間共用的 futex，並說明 `FUTEX_PRIVATE_FLAG` 是告訴核心某個 futex 不共用的方法（[futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)）。本實驗的 futex mutex 使用 private 操作；跨行程的版本沒有在此測試。

## Linux／NT 對照

- **使用者空間的快速路徑。** futex 讓沒有競爭的上鎖完全留在使用者空間，只有等待或喚醒時才進入核心（[futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)）。`WaitOnAddress` 是同樣的想法，限於單一行程。SRW lock 與 critical section 在沒有競爭時也留在使用者模式。練習 E 與 H 中一個執行緒的那幾列顯示：Linux 沒有系統呼叫，Windows 幾乎沒有核心時間。
- **範圍。** `FUTEX_PRIVATE_FLAG` 把 futex 限制在一個行程內；不加它，共享記憶體中的 futex 可以跨行程使用。`WaitOnAddress` 與 SRW lock 只能在單一行程內使用（[WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)、[SRW lock](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)）。跨行程時，Windows 使用具名或複製的核心物件：mutex、event、semaphore。
- **核心自旋鎖。** xv6 的自旋鎖會關閉所在 CPU 的中斷。NT 核心自旋鎖只能短暫持有，而且必須放在非分頁記憶體（[spin locks](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks)）；在 `DISPATCH_LEVEL` 以上不得等待 dispatcher 物件（[dispatcher objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-kernel-dispatcher-objects)）。和 xv6 的 `sched()` 在 `noff != 1` 時拒絕切換比較看看。
- **讀多寫少的資料。** Linux 為讀取遠多於寫入的資料提供 RCU（[RCU](https://docs.kernel.org/RCU/whatisRCU.html)）；xv6 與本實驗都沒有用到。

## 檢核

在 `NOTES.md` 中回答。

1. 用數字呈現改善前後的競爭：練習 A 與 B 中 `lockstat` 的 `kmem` 那一行，附上 `CPUS` 與輪數。再列出練習 E 或 H 中 16 個執行緒、單一鎖與每執行緒一把鎖的那幾列。說明這些數字沒有顯示什麼，例如 xv6 的 tick 數為何沒有改變。
2. 說明 `push_off()`：為何在 `mycpu()` 之前關閉中斷、為何用計數器、為何只有最外層的呼叫保存 `intena`，以及練習 C 表格中的每一個 panic 各是哪一個檢查產生的。
3. 針對練習 D 的每一組有缺陷配對，逐步畫出遺失喚醒的交錯順序，標出每一步中每個行程持有的鎖。再說明同樣的交錯順序，在真正的 `sleep_prepare()`／`sleep()`／`wakeup()`，以及較舊的 `sleep(chan, lk)` 之下，為何無害。
4. 描述一個 lockdep 會回報的死結：兩把鎖、兩種順序、各自的程式路徑，以及回報內容。使用練習 G 或 xv6 的例子。
5. 用練習 J 的證據，說明 Windows 上兩個行程共用的鎖為何不能用 `WaitOnAddress`，並舉出兩種可行的設計。
6. 練習 F 與 I 的模式 3 不可能遺失喚醒，模式 4 可能。說出區分兩者的那一條規則，並在 xv6 的 `sleep()` 中找出同一條規則。

## 常見失敗情況

- **`patch` 失敗，或帶著偏移量套用。** 修補檔是針對 commit `fa2ddcb7` 做的。在其他原始碼樹上，先檢查 `git log -1`，再讀被拒絕的 hunk；MIT 2025 年的 `lock` 分支有不同的 `spinlock.c`，用的是較舊的 `sleep(chan, lk)`。
- **把實驗 05 與實驗 06 的修補檔疊在一起。** 它們應該各用一份新的原始碼樹，而且兩者都使用系統呼叫號碼 23。
- **`lockstat` 沒有印出 pipe 的鎖。** pipe 的鎖位於 `kalloc()` 配置的記憶體中，刻意不登記。
- **`kmemstress` 沒有自旋。** 檢查 `CPUS`：一顆 CPU 不會有競爭。也確認你跑了三個行程。
- **你的每 CPU 配置器死結，或在 `acquire` panic。** 你持有一顆 CPU 的 `kmem` 鎖時去取另一把，或是在中斷開著時呼叫 `cpuid()`，中途換到別顆 CPU。
- **有缺陷的配對下 `lostwake` 從不卡住。** 使用 `CPUS=3`，並保留 `race_window()`；沒有它，視窗只有幾道指令寬，在 QEMU 中很少發生競態。
- **正確的核心上 `lostwake` 回報 STUCK。** 看門狗的上限是 100 個 tick（約 10 s）。模擬器非常慢時，把它調高：`lostwake 2000 300`。
- **expect 腳本在開機時逾時。** 核心在 shell 啟動前就 panic 了，像練習 C 那樣；紀錄中會有那個 panic。腳本在逾時時會結束 QEMU，所以不會有東西留在背景執行。
- **Linux：ThreadSanitizer 說 `unexpected memory mapping`。** 見練習 G：在你能控制的機器上把 `vm.mmap_rnd_bits` 降到 28，或在限制較少的容器之外以 `setarch -R` 執行。
- **Linux：在容器中 `strace` 失敗並出現 `Operation not permitted`。** 加上 `--cap-add SYS_PTRACE`。
- **Windows：`undefined reference to WaitOnAddress`。** 以 `-lsynchronization` 連結。
- **每次執行的時間差很多。** 其他程式、電源計畫，以及容器所在的主機，都會反映在這些數字上。像這裡的表格一樣，回報多次執行的範圍。

## 後續會用到的地方

- 實驗 06 量測被喚醒的執行緒要等多久才拿到 CPU，也就是本實驗所保護的喚醒之後的下一步。
- 階段 08 在行程之間傳遞檔案描述元與共享記憶體；那些行程之間的同步，靠的就是共享記憶體中的 futex，或 Windows 的具名物件。
- 階段 14 的 fence 為 GPU 回答了和 `sleep`／`wakeup` 相同的問題：消費者如何等待生產者，又不錯過訊號。

## 下一步

[實驗 06](../06-scheduling-latency/README.zh-TW.md) 換掉 xv6 的輪轉排程器，並量測在有負載時，60 Hz 的喚醒在 Linux 與 Windows 上會晚多少。
