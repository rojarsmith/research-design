[English](threads-gpos-vs-rtos.md) | **繁體中文** | [課程首頁](../README.zh-TW.md)

# 執行緒的原理，以及 RTOS 的執行緒和一般作業系統有何不同

> **研究日期：2026-11-20。** 技術敘述已在當天對照文末[來源](#來源)列出的官方頁面查核。本文沒有新執行任何程式；文中的量測數字都取自本課程的[實驗 02](../labs/02-processes-threads/README.zh-TW.md)與[實驗 06](../labs/06-scheduling-latency/README.zh-TW.md)，以及 `mcu-foundations` [第七課](../../mcu-foundations/lessons/06-rtos-tasks-and-queues.md)的模擬 kernel。量測條件與限制以各實驗的說明為準：Linux 數字來自虛擬機器，模擬 kernel 的成本是假設值。標為「推論」的段落是依來源做的判斷，不是查到的事實。

本文回答兩個問題：執行緒在機器層面是怎麼做出來的？RTOS（FreeRTOS、Zephyr）的執行緒和一般作業系統（Linux、Windows）的執行緒差在哪裡？這是一篇讀物，用來把實驗 02（情境切換）、05（同步）、06（排程）與 MCU 課程第七課（RTOS）串起來。要親眼看到每個機制，請做那些實驗，對照表在[第八節](#八去哪裡親手驗證)。

## 結論

1. **執行緒是一組暫存器、一個堆疊，加上核心裡的一筆紀錄。** 程式計數器（PC）記著執行到哪裡；堆疊指標（SP）指向這個執行緒自己的堆疊，上面是它的呼叫鏈與區域變數；核心用一個控制區塊記錄它的狀態與優先權，例如 Linux 的 task、Windows 的執行緒物件、FreeRTOS 的 TCB、Zephyr 的 thread control block。程式碼、全域變數與 heap 則和其他執行緒共用。
2. **切換執行緒就是存暫存器、換堆疊、返回。** 先把目前的暫存器存到舊執行緒那裡，再把 SP 換成新執行緒的堆疊，載回新執行緒上次存下的暫存器，然後返回。CPU 就從新執行緒上次停下的地方繼續。每種作業系統都這樣做，差別只在要多存哪些東西。
3. **RTOS 與一般作業系統的執行緒機制幾乎相同，設計目標不同。** 一般作業系統追求整體吞吐、公平與隔離，RTOS 追求可以預測的最壞情況。下列差異都由此而來：
   - **排程：** RTOS 用固定優先權搶占，較高優先權的執行緒一就緒就搶占較低的（Zephyr 的協作式執行緒除外），優先權只會被你自己或優先權繼承改變。一般作業系統的一般排程類別會動態決定誰先跑。
   - **隔離：** 一般作業系統的執行緒屬於某個行程，行程之間由 MMU 隔離。RTOS 通常整個韌體只有一個位址空間，執行緒預設可以存取全部記憶體。
   - **資源：** 一般作業系統在執行時動態建立數千個執行緒，堆疊用到才配置實體記憶體。RTOS 的執行緒少，常在開機時靜態配置，每個堆疊都是一塊固定大小的 RAM。
   - **邊界：** 一般作業系統的使用者執行緒要經過陷阱（trap）才能進入核心。RTOS 的執行緒通常在特權模式下以普通函式呼叫使用 kernel；真正嚴格的邊界在中斷服務常式（ISR）與執行緒之間。
4. **RTOS 不會讓程式變快。** 它讓「最高優先權的工作最晚多久開始執行」有個可以算出來的上限。一般作業系統的平均表現常常更好，但預設不保證最壞情況，實驗 06 的量測就是例子。

## 一、執行緒由哪些東西組成

```
一般作業系統：一個行程                       RTOS：整個韌體
  共用：程式碼、全域變數、heap、               共用：程式碼、全域變數、heap、
        檔案描述元、訊號處置                         周邊暫存器、kernel 物件
  執行緒 A：暫存器、使用者堆疊、核心堆疊        task A：暫存器、堆疊、TCB
  執行緒 B：暫存器、使用者堆疊、核心堆疊        task B：暫存器、堆疊、TCB
  其他行程在另一個位址空間，看不到這裡          沒有其他行程；ISR 共用一個中斷堆疊
```

POSIX 執行緒把屬性分成每個執行緒各自擁有的，以及整個行程共用的（[pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html)）：

| 每個執行緒各自擁有 | 同一行程的執行緒共用 |
| --- | --- |
| 暫存器，包括 PC 與 SP | 程式碼、全域變數、heap |
| 堆疊：區域變數與返回位址 | 行程 ID、父行程 ID、行程群組與工作階段 |
| 執行緒 ID | 開啟的檔案描述元、record lock |
| 訊號遮罩、`errno`、備用訊號堆疊 | 訊號處置（signal disposition） |
| 即時排程策略與優先權；Linux 上另有 CPU 親和性與 capabilities | 使用者與群組 ID、目前目錄、umask、計時器、資源限制 |
| 執行緒區域儲存（TLS） | |

pthreads(7) 把 nice 值列在行程共用的那一邊，但同一頁的 NPTL 一節與 [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html) 都說 Linux 上的 nice 值是每個執行緒各自的。`errno` 能做到每個執行緒一份，靠的就是 TLS。Windows 的說法是 TLS 讓每個執行緒透過同一個全域索引存取到各自的資料（[Thread Local Storage](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-local-storage)）。

**為什麼每個執行緒一定要有自己的堆疊？** 堆疊記錄的是「目前在呼叫鏈的哪裡」：每一層函式的返回位址、存下的暫存器與區域變數。兩個執行緒如果共用堆疊，一個呼叫函式就會蓋掉另一個的返回位址。反過來說，只要每個執行緒有自己的堆疊，同一個函式就能同時被好幾個執行緒執行，區域變數互不干擾。會互相干擾的只有共用的全域變數與 heap，這也是需要鎖的原因（實驗 05、MCU 課程第七課的 TX 競態）。

**行程與執行緒。** Windows 的說法是：行程提供執行程式所需的資源，執行緒是行程中被排程的實體（[About Processes and Threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)）。Linux 在核心裡不區分兩者，都是 task。用 `clone` 建立 task 時，如果帶了 `CLONE_VM`、`CLONE_FILES`、`CLONE_SIGHAND`、`CLONE_THREAD` 等旗標，就會共用位址空間與資源，成為同一行程的執行緒（[clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)）；實驗 02 練習 E 的 `strace` 可以看到這些旗標。Linux 的 NPTL 是 1:1 實作，每個執行緒對應一個核心排程實體（pthreads(7)）。

**核心看不到的執行緒。** Windows 的 fiber、glibc 的 `ucontext`，以及實驗 02 自己寫的 `uswitch`，用的都是同一種「存暫存器、換堆疊」的切換，只是由使用者程式碼決定何時切換。核心只看到一個執行緒，所以它們不會被搶占；其中一個卡在系統呼叫上，同一個核心執行緒上的全部都會停住。

## 二、切換是怎麼做的

### 自願切換：一次函式呼叫

情境切換的本體只是一小段組合語言。用 C 寫成虛擬碼：

```c
struct tcb {
    void *sp;        /* 切換出去時的堆疊頂端；FreeRTOS 把它放在 TCB 的第一個欄位 */
    int   priority;
    int   state;     /* RUNNING、READY、BLOCKED…… */
};

/* 被當成一般函式呼叫，所以呼叫者保存暫存器（caller-saved）已由編譯器在呼叫點處理 */
void switch_to(struct tcb *old, struct tcb *new)
{
    /* 1. 把被呼叫者保存暫存器（callee-saved）推到目前的堆疊         */
    /* 2. old->sp = SP;     記下舊執行緒停在哪裡                         */
    /* 3. SP = new->sp;     換成新執行緒的堆疊                           */
    /* 4. 從新堆疊彈出新執行緒當初推入的被呼叫者保存暫存器               */
    /* 5. return;  返回位址在新堆疊上，所以回到新執行緒上次呼叫 switch_to 之處 */
}
```

這段程式的三個重點：

- **只存被呼叫者保存暫存器就夠了。** 對呼叫者來說，`switch_to` 是一般函式，呼叫慣例本來就允許它破壞呼叫者保存暫存器，編譯器在呼叫之前已經處理好。所以 xv6 的 `swtch` 在 RISC-V 上只存 `ra`、`sp`、`s0`–`s11` 共 14 個暫存器；實驗 02 的 Windows x64 版本存 `rsp`、`rbx`、`rbp`、`rdi`、`rsi`、`r12`–`r15` 與 `xmm6`–`xmm15`。練習 G 拿掉 XMM 那一半之後，整數總和全對、double 總和全錯：被呼叫者保存的暫存器只要少存一種，就會出錯。
- **新執行緒的「第一次返回」是偽造的。** 新執行緒從沒呼叫過 `switch_to`，所以建立它時要偽造一份「看起來像剛切換出去」的狀態：返回位址設為入口函式，SP 設為新堆疊的頂端。第一次切換到它時，`return` 就跳進入口函式。xv6 的入口是 `forkret`，實驗 02 的是 `thread_entry`。練習 B 在 GDB 中看到子行程的 `context.ra` 正是 `forkret`。
- **入口函式結束後無處可回。** pthread 在你的函式外面包了一層，函式返回就等於呼叫 `pthread_exit`。FreeRTOS 則規定 task 不得以任何方式從實作它的函式返回，要結束就呼叫 `vTaskDelete(NULL)` 刪除自己（[FreeRTOS Kernel Book 第 4 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md)）。

### 被搶占：中斷先把其餘暫存器存起來

執行緒沒有自願讓出 CPU 時，例如正在跑一個長迴圈，只有中斷能把控制權搶回來。被打斷的位置不是函式呼叫點，每個暫存器都可能存著有用的值，所以保存分成兩段：進入中斷時先保存呼叫者保存暫存器（或全部暫存器）；排程器決定換人之後，再由上面那段切換程式保存其餘的暫存器。

**一般作業系統**，以 xv6 為例。Linux 與 NT 的架構相同，細節多得多：

```
使用者模式的執行緒 A
  │  計時器中斷或系統呼叫
  ▼  陷阱：全部使用者暫存器存進 A 的 trap frame，切到核心模式，改用 A 的核心堆疊
核心中的 A：排程器決定換人
  │  swtch：只存核心程式碼的被呼叫者保存暫存器
  │  （xv6 會先切到每顆 CPU 專用的排程器執行緒，再由它切到 B）
  │  如果 B 屬於另一個行程，回到使用者模式前還要換分頁表
  ▼  在 B 的核心堆疊上返回
核心中的 B：從 B 的 trap frame 還原全部使用者暫存器
  ▼  回到使用者模式的執行緒 B
```

**RTOS**，以 FreeRTOS 在 Cortex-M4F 上為例：

```
task A 執行中：執行緒模式，SP 是 PSP，指向 A 的堆疊
  │  SysTick 或某個 ISR 判定要換人，把 PendSV 設為 pending
  ▼  硬體：把 r0–r3、r12、lr、pc、xPSR 共 8 個 word 推到 A 的堆疊，
     進入 handler 模式，改用 MSP
PendSV（最低優先權：其他中斷都處理完才輪到它）
  │  r0 = PSP
  │  如果 A 用過 FPU：再推 s16–s31
  │  推 r4–r11 與 r14（例外返回值）
  │  把 r0 存進 A 的 TCB 第一個欄位
  │  呼叫 vTaskSwitchContext()：選出 B，pxCurrentTCB 改指向 B
  │  從 B 的 TCB 第一個欄位取出 B 的堆疊頂端
  │  彈出 r4–r11 與 r14；如果 B 用過 FPU，彈出 s16–s31
  │  PSP = B 的堆疊頂端
  ▼  例外返回：硬體從 B 的堆疊彈出那 8 個 word，pc 回到 B 被打斷的地方
task B 執行中
```

軟體保存的部分一步步對應 FreeRTOS [`portable/GCC/ARM_CM4F/port.c`](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/GCC/ARM_CM4F/port.c) 中 `xPortPendSVHandler` 的指令。硬體自動推入 8 個 word，沿用 MCU 課程第七課引用的 [Cortex-M4 Generic User Guide](https://support.arm.com/documentation/dui0553/latest/)。Zephyr 在 Cortex-M 上也這樣做（[Arm Cortex-M Developer Guide](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html)）：PendSV 設在最低的中斷優先權，好利用 Cortex-M 的 tail chaining，也不影響硬體中斷的延遲；軟體把 r4–r11 存進 kernel 記憶體中的執行緒結構，執行緒有作用中的浮點情境時，再存 s16–s31。

兩邊切換時各自要處理的東西：

| 項目 | 一般作業系統 | RTOS（Cortex-M） |
| --- | --- | --- |
| 整數暫存器 | 進核心時全部存進 trap frame，切換程式再存被呼叫者保存的那些 | 硬體推 8 個 word，PendSV 再推 r4–r11 |
| 浮點與向量暫存器 | 要保存；向量暫存器越寬，狀態越大 | 只有用過 FPU 的 task 才存 s16–s31。Zephyr 文件說，啟用 lazy stacking 時，一般中斷不需付出保存浮點情境的成本 |
| 特權模式 | 使用者模式與核心模式之間來回 | 通常都在特權模式；Zephyr 的 user 執行緒是例外 |
| 位址空間 | 切到另一個行程時要換分頁表；同一行程的執行緒之間不必換 | 沒有分頁表可換；使用 MPU 時，要換上該 task 的區域設定（推論） |
| 堆疊 | 每個執行緒有使用者堆疊與核心堆疊 | 每個 task 一個堆疊（PSP），ISR 共用一個中斷堆疊（MSP） |
| 選出下一個 | 一般排程類別的資料結構（EEVDF）、多顆 CPU、負載平衡 | 依優先權排列的就緒清單 |

### 量到的成本

本課程實驗 02 量到的數字（Windows 在本機；Linux 在虛擬機器中，只能比較同一欄內的比例）：

| 動作 | Windows | Linux（虛擬機器） |
| --- | ---: | ---: |
| 建立一個執行緒並等它結束 | 85.2 µs | 73.9 µs |
| 建立一個行程並等它結束 | 20,717–43,594 µs（`CreateProcess`） | 554 µs（`fork` + `exec` + `wait`） |
| 兩個核心執行緒交接，同一顆邏輯 CPU | 1,277 ns | 1,189 ns |
| 兩個核心執行緒交接，不固定 CPU | 3,264 ns | 18,803 ns |
| 使用者層級切換 | 27.1 ns（`SwitchToFiber`）、12.5 ns（`uswitch`） | 199.7 ns（`swapcontext`，每次都進核心改訊號遮罩） |

一次核心交接約 1.2–1.3 µs，暫存器切換本身只佔一小部分：同樣的切換在使用者層級只要 12.5 ns，其餘是兩次系統呼叫、排程器與喚醒。不固定 CPU 時更貴，因為要喚醒另一顆 CPU。

RTOS 這一端的切換成本，本專案還沒有在板子上量過。MCU 課程的模擬 kernel 假設每次切換 4 µs，只用來比較設計。真實數字要在板子上以 GPIO 搭配邏輯分析儀量，見第七課上板實作的第 3 步。

## 三、排程：下一個由誰跑

```
            被選中
   就緒 ──────────────► 執行中
    ▲   ◄──────────────   │
    │   被搶占、時間片用完  │  等鎖、等 I/O、sleep、等 queue
    │                      ▼
    └───── 事件發生 ────── 阻塞
```

三種狀態在每個系統都找得到。FreeRTOS 把狀態分成 Running 與 Not Running，再把 Not Running 細分為 Ready、Blocked 與 Suspended（[FreeRTOS Kernel Book 第 4 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md)）。**阻塞是執行緒最重要的能力。** 等待中的執行緒不在就緒清單上，不耗 CPU；超級迴圈只能一圈一圈地檢查旗標。這正是 MCU 課程第七課引入 RTOS 的理由。

| 項目 | Linux | Windows | FreeRTOS | Zephyr |
| --- | --- | --- | --- | --- |
| 優先權範圍與方向 | 一般策略的靜態優先權都是 0，以 nice 調整；`SCHED_FIFO`、`SCHED_RR` 為 1（低）到 99（高） | 0 到 31，越大越優先；即時優先權類別的基本優先權落在 16–31 | 數量由 `configMAX_PRIORITIES` 決定，越大越優先，idle 為 0 | 越小越優先；負數是協作式（cooperative），非負數是可搶占（preemptible） |
| 基本規則 | 一般策略由 EEVDF 追求公平（6.6 起取代 CFS，6.12 完成轉換）；即時執行緒一定搶占一般執行緒 | 在最高等級的就緒執行緒之間輪流分配時間片 | 一定執行能執行的最高優先權 task | 在每個重新排程點選出最高優先權的就緒執行緒；可搶占執行緒會被更高優先權者搶占，協作式執行緒要等它自己讓出 |
| 同優先權之間 | `SCHED_RR` 有時間片；`SCHED_FIFO` 一直跑到阻塞、被更高優先權搶占或呼叫 `sched_yield` | 輪流 | 時間片輪流，一個時間片等於一個 tick 週期 | 可選時間片；啟用 `CONFIG_SCHED_DEADLINE` 時，同優先權者以截止時間較早者優先 |
| 系統會不會改你的優先權 | 一般策略中由排程器決定先後；即時策略不會 | 會：基本優先權 0–15 的執行緒在前景、收到輸入或等待完成時被提升，每用完一個時間片降一級；16–31 不提升 | 不會；只有 mutex 的優先權繼承會暫時提升 | 不會；mutex 有優先權繼承 |
| 不被搶占的方式 | — | — | 關閉搶占，或進入臨界區 | 協作式執行緒（負優先權）；只有 meta-IRQ 執行緒能搶占它們 |
| 防止失控 | 即時節流：預設每 1 s 最多 0.95 s 給即時執行緒；6.12 加入 deadline server 基礎設施，目標是取代並改善節流 | Learn 警告幾乎不要使用 `REALTIME_PRIORITY_CLASS`，因為它會打斷處理滑鼠、鍵盤輸入與背景磁碟寫回的系統執行緒 | 沒有：高優先權 task 不阻塞，低優先權 task 永遠輪不到（推論） | 同左（推論） |

來源：[sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)、[sched-rt-group](https://docs.kernel.org/scheduler/sched-rt-group.html)、[EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)、kernelnewbies 的 [Linux 6.6](https://kernelnewbies.org/Linux_6.6) 與 [Linux 6.12](https://kernelnewbies.org/Linux_6.12)、Learn 的 [Scheduling Priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities) 與 [Priority Boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)、[FreeRTOS Kernel Book 第 4 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md)、Zephyr 的 [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html) 與 [Scheduling](https://docs.zephyrproject.org/latest/kernel/services/scheduling/index.html)。

最大的差別在第四列。一般作業系統會替你調整誰先跑：Linux 的一般策略看公平，Windows 會提升剛等到事件的執行緒。RTOS 不替你決定，你排錯優先權，系統就照錯的跑。

實驗 06 量到的兩個例子：

- **Windows：** 16 個優先權 8 的自旋執行緒佔滿 CPU 時，同樣是優先權 8 的 60 Hz 計時器執行緒，兩次執行各錯過 77 與 18 個週期；只高一級（9）時，每次喚醒都在 0.9 ms 內。
- **Linux（虛擬機器）：** 有負載時，`SCHED_FIFO` 讓喚醒延遲的中位數維持在約 22 µs，但每次執行仍有 4–8 ms 的最大值。實驗推測原因在虛擬機器的主機端，但沒有追蹤確認。

也就是說，一般作業系統靠提高優先權可以改善中位數，最壞情況卻不容易掌握。

Zephyr 還讓你選就緒佇列的實作：簡單鏈結串列適合可執行的執行緒很少（文件說大約 3 個）的系統；紅黑樹在大約 20 個以上時較適合；每個優先權一條串列的 multi-queue 需要較多 RAM，而且不能搭配截止時間排程。一般作業系統通常不會把這種選擇交給應用程式。

## 四、逐項比較

| 面向 | 一般作業系統（Linux、Windows） | RTOS（FreeRTOS、Zephyr） |
| --- | --- | --- |
| 稱呼 | thread，屬於某個 process | FreeRTOS 叫 task，Zephyr 叫 thread；沒有 process |
| 位址空間與隔離 | 每個行程一個位址空間，由 MMU 隔離；同一行程的執行緒共用 | 通常整個系統一個位址空間。Zephyr 的執行緒預設在 supervisor 模式執行，可以存取特權指令、整個位址空間與周邊。要隔離時用 MPU：FreeRTOS-MPU 的非特權 task 只能存取自己的堆疊與最多三個自訂區域；Zephyr 的 user 執行緒在 MPU 系統上只能存取自己的堆疊，預設也不能存取其他 kernel 物件 |
| 呼叫 kernel 的方式 | 系統呼叫：陷阱進入核心模式 | 特權 task 直接以函式呼叫 kernel；Zephyr 的 user 執行緒以 SVC 例外發出系統呼叫 |
| 建立時機 | 執行時動態建立，實驗 02 量到 74–85 µs | 也能動態建立，但常在開機時建立並靜態配置（`xTaskCreateStatic`、`K_THREAD_DEFINE`），之後很少增減 |
| 堆疊 | 使用者堆疊位在虛擬記憶體，先保留範圍，用到才配置實體分頁（實驗 03 的保留與提交）；另有核心堆疊 | 每個 task 一塊固定大小的 RAM，建立時就決定。FreeRTOS 以 word 為單位，32 位元核心上的 128 代表 512 bytes |
| 堆疊溢位 | 堆疊尾端通常有防護頁，溢位會變成分頁錯誤，程式被終止；實驗 02 的 `uthread_win64.c` 也自己設了防護頁 | 沒有 MPU 時，溢位會直接蓋掉相鄰的記憶體，例如另一個 task 的 TCB 或堆疊，症狀可能出現在很遠的地方。要靠 RTOS 的溢位檢查、剩餘量量測或 MPU 防護區（MCU 課程第七課） |
| 數量 | 數百到數千 | 個位數到數十 |
| 排程目標 | 吞吐、公平、互動回應；會動態調整 | 固定優先權搶占；最壞情況可以分析 |
| 中斷與執行緒的分工 | 中斷處理程式只做最少的事，其餘交給延後處理，例如 Linux 的 softirq 與 workqueue、NT 的 DPC（實驗 04） | ISR 只把事件放進 queue，由 task 處理。ISR 只能用不會阻塞的 API（FreeRTOS 的 `FromISR`、Zephyr 的 `K_NO_WAIT`），喚醒較高優先權的 task 後要求切換（`portYIELD_FROM_ISR`），否則要等到下一個 tick |
| 不受 kernel 延遲的中斷 | 一般不提供給應用程式 | FreeRTOS 中優先權高於 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 的中斷、Zephyr 的零延遲中斷，都不會被 kernel 遮罩延遲，代價是不能呼叫任何 kernel API |
| 同步物件 | futex、mutex、條件變數（實驗 05） | queue、semaphore、mutex。FreeRTOS 與 Zephyr 的 mutex 都有優先權繼承，FreeRTOS 的 binary semaphore 沒有 |
| 時間基準 | 計時器 tick 或 tickless。Windows 一般的可等待計時器在閒置時仍分散在 0 到約 15.3 ms（實驗 06） | tick 決定時間片與逾時的解析度；FreeRTOS 的一個時間片就是一個 tick 週期 |

來源除上一節所列，另有 Zephyr 的 [User Mode Overview](https://docs.zephyrproject.org/latest/kernel/usermode/overview.html)、FreeRTOS 的 [MPU Support](https://www.freertos.org/Security/04-FreeRTOS-MPU-memory-protection-unit) 與 [xTaskCreateRestricted](https://freertos.org/Documentation/02-Kernel/04-API-references/13-FreeRTOS-MPU-specific/01-xTaskCreateRestricted)、Zephyr 的 [Arm Cortex-M Developer Guide](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html)。ISR、mutex 與堆疊單位的出處見 MCU 課程第七課。

## 五、一般作業系統能不能當 RTOS 用？（推論）

- Linux 6.12 起，主線核心包含即時搶占（PREEMPT_RT）；sched(7) 也說自 6.12 起不需補丁就能啟用。搭配 `SCHED_FIFO` 或 `SCHED_DEADLINE`，Linux 可以做到很低的延遲。
- 但 Linux 的最壞情況比 RTOS 難分析：分頁錯誤、記憶體回收、驅動程式、多核快取與虛擬化，都可能出現在延遲路徑上。實驗 06 在虛擬機器中使用 `SCHED_FIFO`，仍量到 4–8 ms 的長尾。
- 所以常見的分工是：有硬性期限的控制交給 MCU 上的 RTOS，或 SoC 裡的 Cortex-M 協同處理器；Linux 負責網路、檔案與 UI。產品層面的取捨見 HMI 研究的 [19 號：RTOS 與 Linux 的比較](../../hmi-studio-iot-ide-spinoff-research/research/19-rtos-vs-linux.zh-TW.md)。

## 六、常見誤解

- **「RTOS 比較快。」** 不對。25 ms 的運算放進 RTOS 還是 25 ms，RTOS 只決定誰先拿到 CPU。MCU 課程第七課的模型中，RTOS 版本每 10 秒切換 4,830 次，超級迴圈只有 1 次；真正讓延遲降一個數量級的，是縮短臨界區。
- **「把優先權設最高就是即時。」** 在 Windows 上，`REALTIME_PRIORITY_CLASS` 會餓死處理輸入的系統執行緒；在 Linux 上，預設的即時節流會留 5% 給一般執行緒；在 RTOS 上，優先權反轉可能讓最高優先權的 task 等上整整一個中優先權 task 的運算時間（MCU 課程第七課，案例 D）。
- **「RTOS 的 task 就是小一號的行程。」** 不是。沒有 MPU 時，task 之間沒有任何隔離，一個野指標就能寫壞另一個 task 的堆疊或 kernel 的資料。
- **「執行緒越多越快。」** 只有在有多顆 CPU 可以平行執行，或有等待可以重疊時才成立。每個執行緒都要一個堆疊、一個控制區塊，每次交接都是一次切換。MCU 課程第七課建議以「等待的事件」與「期限」決定 task 的數量，而不是每個函式都包成一個 task。
- **「使用者層級執行緒不算執行緒。」** 它們的切換原理完全相同，只是排程者不同：fiber、`ucontext`、Go 的 goroutine 由使用者空間的程式碼決定何時切換。差別在於核心看不到它們，不會個別搶占它們或把它們分到不同的 CPU 上。要做到這些，得由執行時期系統自己來，例如 Go 的執行時期自己管理多個核心執行緒。

## 七、先自行回答

1. 兩個執行緒同時執行同一個函式，區域變數會互相干擾嗎？全域變數呢？
2. 情境切換為什麼只需要保存被呼叫者保存暫存器？那被搶占時呢？
3. 在 Linux 上，同一行程的兩個執行緒之間切換，比切到另一個行程的執行緒少做了什麼？
4. FreeRTOS 上一個優先權 3 的 task 進入不會阻塞的無窮迴圈，優先權 1、2 的 task 與 idle task 會怎樣？在 Linux 上，同樣的執行緒用 `SCHED_OTHER` 會怎樣？用 `SCHED_FIFO` 呢？
5. 執行緒的堆疊溢位，在 Linux 與在沒有 MPU 的 RTOS 上，結果各是什麼？
6. Cortex-M 上的 RTOS 為什麼把 PendSV 設成最低優先權？

### 解析

1. 區域變數在各自的堆疊上，不會互相干擾；全域變數與 heap 是共用的，會互相干擾，要用鎖保護，或只讓一個執行緒擁有它。
2. 自願切換時，切換程式是被當成函式呼叫的，呼叫者保存暫存器已由編譯器處理。被搶占時，被打斷的位置不是呼叫點，所以中斷進入時要先保存其餘的暫存器：一般作業系統由陷阱存進 trap frame，Cortex-M 由硬體推入 8 個 word。
3. 不必換分頁表，也就不會因為換位址空間而影響 TLB 的內容；兩者共用同一組檔案描述元等資源。核心堆疊、暫存器與排程器的工作仍然一樣要做。
4. FreeRTOS：排程器一定執行能執行的最高優先權 task，所以優先權 1、2 的 task 與 idle task 永遠輪不到。依賴 idle task 的工作也不會進行，例如回收已刪除 task 的記憶體；如果有 watchdog，最後會因為沒人餵狗而重啟。Linux `SCHED_OTHER`：EEVDF 讓其他執行緒仍分到 CPU 時間。Linux `SCHED_FIFO`：同一顆 CPU 上較低優先權的即時執行緒會被餓死，一般執行緒則因預設的即時節流，每秒仍有約 5% 的時間。
5. Linux：堆疊尾端通常有防護頁，溢位變成分頁錯誤，程式被終止，錯誤至少有跡可循。沒有 MPU 的 RTOS：溢位會靜悄悄地覆寫相鄰的記憶體，症狀可能在很久以後、在別的 task 中才出現。
6. 讓切換只在其他中斷都處理完之後才發生，利用 tail chaining 減少進出例外的成本，也不拉長硬體中斷的延遲。Zephyr 的 Cortex-M 文件列出的正是這兩個理由。

## 八、去哪裡親手驗證

| 想看到的機制 | 實驗 |
| --- | --- |
| `swtch` 存了什麼、新執行緒如何從 `forkret` 開始 | [實驗 02](../labs/02-processes-threads/README.zh-TW.md) 練習 B、C |
| 自己寫一個使用者層級執行緒，並刻意少存暫存器 | 實驗 02 練習 D、G |
| 執行緒與行程的 clone 旗標、建立與切換的成本 | 實驗 02 練習 E、F |
| 鎖、futex 與遺失喚醒 | [實驗 05](../labs/05-synchronization/README.zh-TW.md) |
| 優先權、`SCHED_FIFO` 與喚醒延遲 | [實驗 06](../labs/06-scheduling-latency/README.zh-TW.md) |
| RTOS 的 task、queue、優先權反轉與 ISR 邊界 | `mcu-foundations` [第七課](../../mcu-foundations/lessons/06-rtos-tasks-and-queues.md)與[模擬 kernel](../../mcu-foundations/labs/06-rtos-model/rtos_sim.c) |
| 產品該選 RTOS 還是 Linux | [HMI 研究 19 號](../../hmi-studio-iot-ide-spinoff-research/research/19-rtos-vs-linux.zh-TW.md) |

## 來源

2026-11-20 查核。

**Linux 與 POSIX**
- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html)：每個執行緒與整個行程共用的屬性；NPTL 是 1:1 實作。
- [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html)（man-pages 6.19）：排程策略、即時優先權 1–99、`SCHED_FIFO` 何時讓出 CPU、節流的 `/proc` 檔案與預設值、6.12 起不需補丁即可啟用即時搶占。注意：這一頁仍寫著「預設排程器是 CFS」，沒有提到 EEVDF。
- [sched-rt-group](https://docs.kernel.org/scheduler/sched-rt-group.html)：節流預設值 1,000,000 µs 與 950,000 µs，以及為 fair dl_server 保留頻寬的限制。
- [EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)；kernelnewbies 的 [Linux 6.6](https://kernelnewbies.org/Linux_6.6)（以 EEVDF 取代 CFS）與 [Linux 6.12](https://kernelnewbies.org/Linux_6.12)（完成 EEVDF 轉換、併入 PREEMPT_RT、SCHED_DEADLINE server 基礎設施、sched_ext）。
- [clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)。

**Windows**
- Learn：[Scheduling Priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)、[Priority Boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)、[Context Switches](https://learn.microsoft.com/en-us/windows/win32/procthread/context-switches)、[Thread Local Storage](https://learn.microsoft.com/en-us/windows/win32/procthread/thread-local-storage)、[About Processes and Threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)。Context Switches 頁面沒有說明時間片的長度。

**FreeRTOS**
- [FreeRTOS Kernel Book 第 4 章](https://github.com/FreeRTOS/FreeRTOS-Kernel-Book/blob/main/ch04.md)：task 狀態、TCB 與堆疊、以 word 計的堆疊深度、task 不得返回、`configMAX_PRIORITIES`、tick 與時間片。頁面很長，本次逐字核對了前段；Ready 與 Suspended 這兩個子狀態及 idle task 的優先權，取自同頁後段的摘要，與 MCU 課程第七課引用的內容一致。
- [`portable/GCC/ARM_CM4F/port.c`](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/GCC/ARM_CM4F/port.c)：`xPortPendSVHandler`、`vPortSVCHandler`、`prvPortStartFirstTask`。
- [MPU Support](https://www.freertos.org/Security/04-FreeRTOS-MPU-memory-protection-unit)、[xTaskCreateRestricted](https://freertos.org/Documentation/02-Kernel/04-API-references/13-FreeRTOS-MPU-specific/01-xTaskCreateRestricted)：非特權 task 只能存取自己的堆疊與最多三個自訂區域，只能用 `xTaskCreateRestricted()` 建立。這兩頁是由搜尋結果取得，沒有逐字讀完。

**Zephyr**
- [Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html)、[Scheduling](https://docs.zephyrproject.org/latest/kernel/services/scheduling/index.html)、[User Mode Overview](https://docs.zephyrproject.org/latest/kernel/usermode/overview.html)、[Arm Cortex-M Developer Guide](https://docs.zephyrproject.org/latest/hardware/arch/arm_cortex_m.html)。

**Arm**
- [Cortex-M4 Devices Generic User Guide](https://support.arm.com/documentation/dui0553/latest/)：這次無法以自動化方式讀取（頁面只回傳標題）。硬體在例外進入時推入 8 個 word 的說法，沿用 MCU 課程第七課引用同一份文件的內容；Zephyr 的 Cortex-M 文件也說，被切換出去的執行緒的呼叫者保存情境與返回位址，放在例外堆疊框中。
