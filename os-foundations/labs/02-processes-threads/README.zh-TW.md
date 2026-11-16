[English](README.md) | **繁體中文** | [實作建置計畫，階段 02](../../SYLLABUS.zh-TW.md)

# 實驗 02：行程、執行緒與情境切換

> **草稿，待審閱（2026-11-15），xv6 與 Linux 部分於 2026-11-16 執行。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A–D）：2026-11-16 已執行**，環境是 Windows 11 上 podman 中的 Ubuntu 24.04 容器，podman machine 跑在 WSL 2 上；這不是本課程要你使用的 WSL 2 Ubuntu 發行版。容器內有 QEMU 8.2.2、riscv64-linux-gnu-gcc 13.3.0 與 gdb-multiarch 15.1，[xv6-riscv](https://github.com/mit-pdos/xv6-riscv) master 的 commit 為 `fa2ddcb`（2026-11-05）。練習 B 由 expect 腳本同時在 xv6 主控台與 GDB 中輸入，練習 C 由批次 GDB 腳本驅動。練習 D 是用一份只為檢查而寫的 `uthread_win64.c` 移植版驗證，那份程式碼不在本資料夾中。輸出：[`xv6/example-output.txt`](xv6/example-output.txt)。
> - **Linux 部分（練習 E）：2026-11-16 已執行**，在同一個容器中：podman machine 的核心 6.6.87.2-microsoft-standard-WSL2，有 16 顆線上 CPU，glibc 2.39、gcc 13.3.0，以及不需額外容器權限就能運作的 strace 6.8。請把它的計時當成虛擬機器的數字。輸出：[`linux/example-output.txt`](linux/example-output.txt)。
> - **Windows 部分（練習 F、G）：已建置並執行**，於 2026-11-15 在撰寫本實驗的機器上以 WinLibs 的 gcc 16.2 完成。輸出在 [`windows/example-output.txt`](windows/example-output.txt)，下文的 Windows 數字都取自該檔。
> - **未執行：** MIT 的 `xv6-labs-2025` 儲存庫及其 `util` 作業，以及練習 C 最後關於鎖的問題；那些問題靠閱讀程式碼回答。

## 目標

要得到第二條控制流程有三種方法：新的行程、新的核心執行緒，以及由你自己的程式碼切換過去的使用者層級執行緒。三種都做出來、量出各自的成本，並能精確說出情境切換保存了哪些暫存器、存在哪裡、為何可以略過其餘的暫存器。xv6 核心的 `swtch`、你為 xv6 寫的使用者層級切換，以及本資料夾的 Windows x64 切換，分別在不同的 ABI 下回答這個問題。

```
行程 A 的核心執行緒                  這顆 CPU 的排程器執行緒                行程 B 的核心執行緒
yield()、sleep() 或 kexit()
  sched()
    swtch(&A->context, &c->context) ─► 在 scheduler() 內返回
                                       釋放 A 的鎖，選出 B
                                       swtch(&c->context, &B->context) ─► 在 sched() 內返回；
                                                                          第一次執行時則在 forkret()
```

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/pipeline.c`](xv6/pipeline.c) | 用 `fork`、`exec`、`pipe`、`dup`、`wait` 組成 `cat README \| grep the \| wc`，附 `-leak` 開關 | 已在 xv6 中執行（容器） |
| [`xv6/example-output.txt`](xv6/example-output.txt) | xv6 的輸出紀錄：管線、`fork()` 與 `swtch()` 的追蹤，以及使用者層級執行緒的檢查 | 2026-11-16 記錄 |
| [`linux/pipeline.c`](linux/pipeline.c) | Linux 上的同一條管線，給 `strace -f` 用 | 已在 Linux 上建置並執行（容器） |
| [`linux/procthread.c`](linux/procthread.c) | `nt_procthread.c` 的 Linux 對應版：`fork`、`fork` 加 `exec`、`posix_spawn`、`pthread_create`、號誌來回傳遞、`swapcontext` | 已在 Linux 上建置並執行（容器） |
| [`linux/example-output.txt`](linux/example-output.txt) | 兩個 Linux 程式的輸出紀錄，含 `strace` 的執行 | 2026-11-16 記錄 |
| [`windows/nt_procthread.c`](windows/nt_procthread.c) | 行程與執行緒的識別碼、`CreateProcess` 與 `CreateThread` 的成本、兩個執行緒之間的事件來回傳遞、`SwitchToFiber` | 已在此建置並執行 |
| [`windows/uthread_win64.c`](windows/uthread_win64.c)、[`windows/uswitch_win64.S`](windows/uswitch_win64.S) | 以手寫 x64 情境切換實作的使用者層級執行緒：一張執行緒表、組合語言的切換，以及 yield | 已在此建置並執行 |
| [`windows/example-output.txt`](windows/example-output.txt) | 輸出紀錄，包含 `-DSKIP_XMM` 版本的執行結果與 `worker` 的反組譯 | 2026-11-15 記錄 |

## 環境設定

- 實驗 00 的工作環境。練習 B 與 C 請用 `CPUS=1`，讓中斷點只在一個 hart 上觸發。
- 對應的 MIT 實驗是 `util`（在 `xv6-labs-2025` 中執行 `git checkout util`）。它 2025 年的作業是 `sleep`（建立在 `pause()` 系統呼叫上）、`sixfive`、`memdump`、`find`，以及使用 `fork`、`exec`、`wait` 的 `find -exec`（[util 實驗](https://pdos.csail.mit.edu/6.1810/2025/labs/util.html)）。先把它們當暖身；本實驗假設你會寫小型的 xv6 使用者程式。
- **Linux：** 和實驗 01 一樣，WSL 2 Ubuntu 加上 `strace`。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc。練習 G 會用到同一套工具鏈的 `objdump`。

## 先讀

- xv6 課本第 1 章（行程、檔案描述元、pipe）與排程那一章（第 7 章）。
- `user/sh.c`：`runcmd()`，特別是 `PIPE` 的情況，以及 `parsepipe()`。
- `kernel/proc.c`：`allocproc`、`kfork`、`scheduler`、`sched`、`yield`、`forkret`、`kexit`、`kwait`。
- `kernel/swtch.S` 與 `kernel/proc.h` 的 `struct context`。
- [RISC-V 呼叫慣例](https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc)的整數暫存器表：`s0`–`s11` 與 `sp` 跨呼叫保留，`ra`、`t0`–`t6`、`a0`–`a7` 則不保留。這就是 `c-compiler-foundations` 的呼叫慣例，現在由核心使用。

## 練習

執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 用四個系統呼叫組成管線（xv6）

把 [`xv6/pipeline.c`](xv6/pipeline.c) 複製到 `user/`，在 `UPROGS` 加上 `$U/_pipeline`，然後開機。

**預測：** `pipeline` 印出的數字，會和在 xv6 shell 輸入 `cat README | grep the | wc` 的結果相同嗎？三行 `pid N exited` 的順序為何？`pipeline -leak` 會怎樣？哪個行程會卡住、在等什麼？

兩者都執行，再從程式碼回答：

- 每個子行程在 `dup` 之後關閉全部四個 pipe 端點，父行程也一樣。找出 `sh.c` 中做同樣事情的那一行。
- `sh.c` 建立 `a | b | c` 的方式不同：`parsepipe()` 是右遞迴，所以 shell 為 `a` 分出一個子行程，再分出另一個本身就是 `b | c` 管線的子行程。畫出兩種做法各自產生的行程樹。
- 執行 `-leak` 時，按 Ctrl-p 查看每個行程與狀態。xv6 的 shell 沒有工作控制，請按 Ctrl-a 再按 x 離開 QEMU。

<details>
<summary>2026-11-16 的觀察結果。請在預測並執行之後再看。</summary>

- `pipeline` 印出 `3 33 197`，和在 shell 輸入 `cat README | grep the | wc` 相同。
- 五次執行中，不論一個或三個 hart，`pid N exited` 都依 cat、grep、wc 的順序出現，計數那一行則可能夾在任何位置。有一次在三個 hart 下，wc 的計數和一行 `pid N exited` 逐字元混在一起（`3 p3i3d  1129 ex7i t`），因為 xv6 的 `printf` 每個字元做一次 `write`。
- `pipeline -leak` 印出 `cat` 的 `pid 4 exited with status 0` 之後就沒有下文。Ctrl-p 除了 `init` 與 `sh`，還列出 `3 sleep  pipeline`、`5 sleep  grep` 與 `6 sleep  wc`；幾秒後再按一次，清單不變。

</details>

### B. 追蹤 `fork()`（xv6）

```
(gdb) b kfork
(gdb) c                      第一次停下是 init 啟動 sh，此時你還沒輸入任何東西
(gdb) c                      現在在 xv6 主控台輸入 "pipeline"：這次是 sh 為它 fork
(gdb) c                      這次是 pipeline 自己的第一次 fork
(gdb) p cpus[0].proc->name   在中斷點時 p 本身還沒設定
(gdb) p cpus[0].proc->pid
(gdb) next                   （重複；在 allocproc() 返回後停下）
(gdb) p np->pid
(gdb) p/x np->context        子行程核心執行緒的 ra 與 sp
(gdb) p/x np->kstack
(gdb) p p->sz / 4096
```

**繼續單步之前先預測：**

1. `allocproc()` 設定了子行程的 `context.ra` 與 `context.sp`。子行程的核心執行緒會先執行哪個函式？在哪個堆疊上？
2. `uvmcopy()` 會複製幾頁？master 的 `uvmcopy` 會複製 `p->sz` 以下所有已映射的分頁、略過未映射的分頁，所以 `p->sz / 4096` 是上限。階段 03 的寫入時複製 fork 會降低這個數字，現在先記下來以便比較。
3. 執行 `*(np->trapframe) = *(p->trapframe)` 與 `np->trapframe->a0 = 0` 之後，子行程會在使用者空間的哪裡繼續執行？`fork()` 在那裡會回傳什麼？

用 `disable 1`、`b forkret`、`c` 與 `p cpus[0].proc->pid` 檢查預測 1（只有一個 hart 時，`cpus[0]` 就是唯一的 CPU）。如果沒有 `disable`，下一次停下會是父行程的第二次 `fork`，因為父行程會一直執行到被阻塞為止。到 `user/pipeline.asm` 檢查預測 3：找出 `fork` stub 中的 `ecall` 與它的下一道指令。

<details>
<summary>觀察結果。請在預測並執行之後再看。</summary>

- 三次停下依序是 `init`（pid 1）、`sh`（pid 2）與 `pipeline`（pid 3）。在中斷點時，`p p->name` 印出 `value has been optimized out`。
- 三次 `next` 之後（第 271 行，呼叫 `uvmcopy` 處），`np->pid` 是 4；`np->context` 的 `ra = 0x80001912`，`info symbol` 說它是 `forkret`；`sp = 0x3fffff8000`，等於 `np->kstack`（`0x3fffff7000`）加 4096。所有 `s` 暫存器都是 0。
- `p->sz / 4096` 是 4。
- 中斷點 1 仍啟用時，`c` 又停在 `kfork`，這次是 pid 3。`disable 1` 之後，停在 `forkret`，`cpus[0].proc->pid` 是 4，名稱仍是 `pipeline`，因為 `exec` 還沒執行；`$sp` 是 `0x3fffff8000`。
- 在 `user/pipeline.asm` 中，`fork` stub 的 `ecall` 在 `0x49e`，其後的 `ret` 在 `0x4a2`。

</details>

### C. 追蹤 `swtch()`（xv6）

```
(gdb) b swtch
(gdb) c
(gdb) bt
(gdb) p/x $a0
(gdb) p/x $a1
(gdb) p &cpus[0].context
(gdb) p/x $sp
```

把 `$a0`、`$a1` 和 `&cpus[0].context` 比較，判斷這次切換的方向：從行程到排程器，或從排程器到行程。**預測** `swtch` 最後的 `ret` 會落在哪裡、在哪個堆疊上，再單步走到那裡：

```
(gdb) si                     （重複到 ret：從第一道指令起共 28 次）
(gdb) p/x $ra
(gdb) info symbol $ra
(gdb) si
(gdb) bt
(gdb) p/x $sp
```

兩個方向都要抓到，並各記錄一次 `info symbol $ra`。預期會看到 `scheduler`、`sched`，或從未執行過的行程的 `forkret`。接著回答：`scheduler()` 在哪個堆疊上執行？提示：`main()` 呼叫 `scheduler()` 後永不返回，而 `main()` 在 `entry.S` 設好的堆疊上執行。

也找出切換期間持有的是哪一把鎖。`yield()` 在 `sched()` 之前取得 `p->lock`，而另一邊有程式碼負責釋放它。找出 `scheduler()` 與 `forkret()` 中的 `release`。

<details>
<summary>開機後前四次停下的觀察結果。請在預測並執行之後再看。</summary>

- 第 1 次，排程器到行程：`$a0` 是 `0x80012350`，即 `&cpus[0].context`；`$a1` 是 `0x800127a8`，第一個行程的 context。`bt` 顯示 `swtch` 由 `scheduler` 呼叫，`scheduler` 由 `main` 呼叫。28 次 `si` 走到 `ret`。`$ra` 是 `forkret`，`$sp` 從 `0x8000b1a0` 變成 `0x3fffffe000`，也就是 pid 1 的核心堆疊。
- 第 2 次，行程到排程器：pid 1 在 `virtio_disk_rw()` 中睡眠，這是 `forkret()` 呼叫的 `fsinit()` 裡面。`$a0` 與 `$a1` 對調。`ret` 回到 `scheduler+130`，`$sp` 又變回 `0x8000b1a0`。
- 第 3 次，又是排程器到行程：這次 `ret` 回到 `sched+112`，也就是第 2 次離開的那個 `sleep()` 裡面。第 4 次和第 2 次相同。
- `scheduler()` 的 `$sp` 是 `0x8000b1a0`，位於 `stack0` 最前面 4096 位元組（`0x8000a220` 到 `0x8000b220`）之內：這是 `entry.S` 為 hart 0 設好的堆疊。
- 剛切換進 `forkret` 時，`bt` 列出一個虛構的呼叫者 `myproc () at kernel/proc.c:89`。沒有任何函式呼叫 `forkret`，是 `swtch` 的 `ret` 跳過去的。

</details>

### D. xv6 的使用者層級執行緒

把 [`windows/uthread_win64.c`](windows/uthread_win64.c) 移植到 xv6，成為 `user/uthread.c`，並為 RISC-V 撰寫切換程式 `user/uthread_switch.S`：

- **切換：** `thread_switch(struct context *old, struct context *new)` 保存並還原與 `kernel/swtch.S` 相同的 14 個暫存器：`ra`、`sp`、`s0`–`s11`。在 `uthread.c` 中定義自己的 `struct context`，不要引入核心的定義。
- **新執行緒的情境：** 把 `ra` 設為 `thread_entry`，`sp` 設為該執行緒堆疊的頂端。堆疊可以是以 `__attribute__((aligned(16)))` 宣告的靜態 `char` 陣列，讓 `sp` 符合 RISC-V psABI 要求的 16 位元組對齊。在 RISC-V 上，`ret` 跳到 `ra`，所以堆疊上不必預先放任何東西。在 x64 上，`uswitch_win64.S` 必須把 `thread_entry` 的位址放在新堆疊上，讓 `ret` 彈出。請對照 Windows 檔案中的 `thread_create()`。
- **移除 Windows 的部分：** Windows 與 C 函式庫的標頭檔（改用 `kernel/types.h`、`kernel/stat.h`、`user/user.h`）、`VirtualAlloc`、`VirtualProtect`、防護頁、`QueryPerformanceCounter` 與 `setvbuf`。把 `int64_t` 換成 `uint64` 或 `long`。xv6 有 `printf`，`main` 以 `exit(0)` 結束。
- **先只用整數總和。** xv6 的 `printf` 本來就沒有 `%f`。之後再做個實驗：把 `double` 總和加回來，轉成整數比較。如果 xv6 終止了程式，記下 `scause` 並解釋。提示：讀 RISC-V 特權規格中 `sstatus` 的 `FS` 欄位，再到 xv6 中 grep 它。
- **Makefile：** 使用者程式由 `_%` 規則從單一目的檔連結。新增一條 `$U/_uthread` 規則，複製那條規則的指令，並一起連結 `$U/uthread_switch.o`；再新增一條規則，用 `$(CC) $(CFLAGS) -c` 從 `.S` 建出 `$U/uthread_switch.o`。指令行必須以 tab 開頭。把 `$U/_uthread` 加進 `UPROGS`，再用 `make qemu` 或 `make fs.img` 重新建置；只執行 `make` 只會建置核心。

**先預測，再執行：** 第一部分的輸出應該像 Windows 上一樣交錯出現：`thread_a round 0`、`thread_b round 0`、`thread_c round 0`、`thread_a round 1`，依此類推（練習 F）。**刻意弄壞：** 讓 `thread_switch` 只保存 `ra` 與 `sp`。先預測會壞掉什麼、原因為何：xv6 以 `-O` 編譯使用者程式，`worker` 的迴圈變數一定放在某處。讀 `user/uthread.asm` 找出位置，再執行。

<details>
<summary>參考用移植版（不在本資料夾中）的觀察結果。請在預測並執行你自己的版本之後再看。</summary>

- 第一部分完全如預測般交錯，共 16 次切換，每個整數總和都正確。兩個執行緒各 1,000,000 回合，總和也正確。
- 在那份移植版的 `user/uthread.asm` 中，`worker` 把 `i` 放在 `s1`、`isum` 放在 `s3`、`w` 放在 `s2`，`current` 的位址放在 `s4`。
- 只保存 `ra` 與 `sp` 時，一個總和都沒印出來。`thread_c round 0` 之後，xv6 印出 `usertrap(): unexpected scause 0xf pid=3`，以及 `sepc=0x252 stval=0x800e022e40611b1`，並終止程式。`0x252` 在 `main` 自己的迴圈中，也就是內聯後的 `current->state = RUNNABLE`。`main` 以為 `s3` 存著 `&current`，但它恢復執行時拿到的是某個 worker 留在 `s3` 的值，於是寫到了垃圾位址。
- 把 `double` 總和加回來後，xv6 在第一行輸出之前就終止程式：`scause 0x2`（illegal instruction）、`sepc=0x396`、`stval=0xa422`。`0xa422` 是 `worker` prologue 中 `fsd fs0,8(sp)` 的編碼。xv6 從不設定 `sstatus` 的 `FS` 欄位，grep 也找不到任何使用它的地方。

</details>

### E. 在 Linux 上問同樣的問題（WSL 2）

```sh
cd linux
gcc -O2 -Wall -o pipeline pipeline.c
gcc -O2 -Wall -pthread -o procthread procthread.c
./pipeline
cat pipeline.c | grep pipe | wc -l
strace -f -e trace=clone,clone3,execve,pipe,pipe2,dup2,wait4 -o pipeline.trace ./pipeline
./procthread
strace -f -e trace=clone,clone3,execve,wait4 ./procthread identity
strace -c -f ./procthread switch > /dev/null
```

`pipeline` 會讀取目前目錄的 `pipeline.c`，所以要在 `linux/` 內執行。

**先預測，再確認：**

1. C 程式呼叫 `fork()` 的地方，追蹤紀錄中出現的是哪個系統呼叫？fork(2) 說明 glibc 的包裝函式自 glibc 2.3.3 起改為呼叫 clone(2)（[fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)）。
2. `pthread_create` 傳入了哪些 `fork` 沒有的 clone 旗標？找找 `CLONE_VM`、`CLONE_FILES`、`CLONE_SIGHAND` 與 `CLONE_THREAD`（[clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)）。較新的 glibc 可能呼叫 `clone3`，strace 會把它的參數印成一個結構。
3. 主執行緒的 `tid` 等於 `pid` 嗎？新建的執行緒呢？子行程呢？
4. `swapcontext` 會進入核心嗎？用 `strace -c` 計數。提示：`ucontext_t` 包含訊號遮罩（[getcontext(3)](https://man7.org/linux/man-pages/man3/getcontext.3.html)）。把它的成本和下面的 `SwitchToFiber`、`uswitch` 比較。
5. 把你的數字和練習 F 表格中的 Linux 欄比較；那一欄是在開頭說明的容器中記錄的。WSL 2 同樣在虛擬機器中執行，兩者都只做相對比較。

<details>
<summary>容器中的觀察結果。請在預測並執行之後再看。</summary>

- `./pipeline` 印出 12，和 `cat pipeline.c | grep pipe | wc -l` 相同。`./pipeline --leak` 在 `cat` 結束後卡住，最後由 `timeout` 終止。
1. `fork()` 在追蹤紀錄中是 `clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, ...)`。紀錄中也看得到 `execvp` 沿著 `PATH` 搜尋：`execve("/usr/local/sbin/cat", ...)` 以 `ENOENT` 失敗，依此類推，直到 `/usr/bin/cat`。
2. `pthread_create` 呼叫 `clone3`，旗標為 `CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID`，`exit_signal=0`，並帶有自己的堆疊。
3. 主執行緒的 tid 等於 pid（35）；新建的執行緒 pid 相同、tid 是新的（36）；子行程有新的 pid，且 tid 等於 pid（37）。
4. 會。`strace -c -f ./procthread switch` 算到 4,000,010 次 `rt_sigprocmask`，4,000,000 次 `swapcontext` 各一次；另有號誌來回傳遞產生的 861,958 次 `futex`。這次執行花了好幾分鐘，因為每個被追蹤的系統呼叫都很慢。

</details>

### F. Windows 上的行程、執行緒與切換（已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o nt_procthread.exe nt_procthread.c
.\nt_procthread.exe
gcc -O2 -Wall -o uthread_win64.exe uthread_win64.c uswitch_win64.S
.\uthread_win64.exe
```

**執行前先預測：** 把下列項目由便宜到昂貴排序，並把每一項猜到十倍誤差以內：建立一個行程並等它結束；建立一個執行緒並等它結束；透過事件把控制權交給另一個核心執行緒；切換 fiber；用 `uswitch` 切換。

2026-11-15 的觀察結果（Windows 組建 26300、16 個邏輯處理器的 Intel Core i7-11700、有 hypervisor；[完整輸出](windows/example-output.txt)）。當天跑了兩次，先列記錄下來的那一次，方括號內是較早的一次。

Linux 欄於 2026-11-16 在本實驗開頭說明的容器中記錄，容器看到的是同一款 i7-11700、16 個邏輯 CPU（[完整輸出](linux/example-output.txt)）。先列第一次執行 `./procthread` 的結果，方括號內是第二次。那是虛擬機器，請比較同一欄內的比例，不要跨欄比較數值。

| 項目 | Windows，觀察值 | Linux，在容器中的觀察值 |
| --- | ---: | ---: |
| 建立一個行程並等它結束（以 `--child` 對本程式呼叫 `CreateProcess`） | 43,594 µs [20,717 µs] | `fork` + `exec` + `wait`：554 µs [613 µs] |
| 建立一個執行緒並等它結束 | 85.2 µs [82.8 µs] | `pthread_create` + `join`：73.9 µs [76.2 µs] |
| 兩個核心執行緒之間交接，同一個邏輯 CPU | 1,277 ns [1,301 ns] | 1,189 ns [1,322 ns] |
| 兩個核心執行緒之間交接，不固定 CPU | 3,264 ns [3,280 ns] | 18,803 ns [19,316 ns] |
| `SwitchToFiber` | 27.1 ns [28.8 ns] | `swapcontext`：199.7 ns [218.3 ns] |
| `uswitch` 輪流執行，含排程迴圈 | 12.5 ns [12.1 ns] | 未量測 |

在 Linux 上，只有 `fork` + `_exit` + `wait` 是 254.5 µs [201.9 µs]，`posix_spawn` + `wait` 是 437.0 µs [487.3 µs]。

這些執行顯示了什麼、沒有顯示什麼：

- **在這台機器上，建立行程既昂貴又不穩定。** 同一天的兩次執行相差一倍。Learn 說明 `CreateProcess` 會建立新行程及其主執行緒，並且在新行程完成初始化之前就返回（[CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)）。本程式還會等子行程結束。這幾十毫秒花在哪裡沒有量測。請提出候選原因（載入映像檔、DLL 初始化、安全軟體、主控台），並測試其中一個，例如用 WPR 追蹤（階段 06）或 Sysinternals 的 Process Monitor。
- **一次核心交接約等於 50 次 fiber 切換、100 次 `uswitch`**（1,277 ns 對 27.1 ns 與 12.5 ns）。核心路徑是等待一個事件、設定另一個事件，各是一次系統呼叫，再加上排程器。使用者層級的路徑只是一次函式呼叫。
- **不固定 CPU 的交接比固定時貴 2.5 倍。** 說明為何喚醒另一個邏輯 CPU 上的執行緒，可能比在同一個 CPU 上切換更久，再設計一個測試。
- **`SwitchToFiber` 的成本是 `uswitch` 的兩倍。** Learn 說明系統為 fiber 保存的狀態只有它的堆疊、函式呼叫之間通常會保留的那些暫存器，以及 fiber 資料，而 fiber local storage 會隨 fiber 一起切換（[Fibers](https://learn.microsoft.com/en-us/windows/win32/procthread/fibers)）。讀 `uthread_win64.c` 開頭的警告：為了讓例外在 fiber 的堆疊上仍能運作，Windows 還必須更新什麼？
- **在 Linux 虛擬機器中，不固定 CPU 的交接約為固定時的 16 倍**（18,803 ns 對 1,189 ns），遠高於 Windows 上的 2.5 倍。`swapcontext` 約為 `SwitchToFiber` 的 7 倍，練習 E 第 4 題找到的系統呼叫可以解釋其中一部分。和你自己的數字比較之前，先提出為何不固定 CPU 的情況在虛擬機器中這麼慢。

識別碼那一部分呈現了建置計畫描述的劃分：主執行緒與新建執行緒回報相同的 pid、不同的 tid，子行程則有新的 pid 與新的 tid。用 Learn 的說法，行程提供執行程式所需的資源，執行緒是行程中被排程的實體（[processes and threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)）。

### G. 切換必須保存什麼，不保存會壞掉什麼（已在此執行）

`uswitch_win64.S` 保存 `rsp`、`rbx`、`rbp`、`rdi`、`rsi`、`r12`–`r15` 與 `xmm6`–`xmm15`，也就是 Learn 的 [x64 呼叫慣例](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention)列為非揮發性（nonvolatile）的暫存器。拿掉 XMM 暫存器再建置一次：

```powershell
gcc -O2 -Wall -DSKIP_XMM -o uthread_noxmm.exe uthread_win64.c uswitch_win64.S
.\uthread_noxmm.exe
objdump -d -M intel --no-show-raw-insn uthread_win64.exe
```

**先預測**兩種總和中哪一種會出錯，再執行。在此觀察到：所有整數總和都正確，所有 double 總和都錯誤。例如執行緒 0 的 double 總和是 10.25，預期是 0.75；執行緒 1 是 0.00，預期是 3.75。程式以狀態 1 結束。`worker` 的反組譯（gcc 16.2、`-O2`，記錄在輸出檔中）說明了原因：

```
movaps XMMWORD PTR [rsp+0x20],xmm6     worker 的 prologue 保存 xmm6，因為它要用
pxor   xmm6,xmm6                       dsum = 0.0 放在 xmm6
xor    edi,edi                         isum = 0 放在 rdi
call   1400017b0 <thread_schedule>     內聯後的 yield，只剩這個呼叫
addsd  xmm6,xmm0                       dsum += ...
add    rdi,rax                         isum += ...
```

在切換執行緒的那次呼叫前後，編譯器把 `dsum` 放在 `xmm6`、常數 0.25 放在 `xmm7`、`isum` 放在 `rdi`、`i` 放在 `rbx`、`w` 放在 `rsi`，全都是非揮發性暫存器。切換時不處理 XMM，每個執行緒恢復時拿到的就是前一個執行緒留下的 `xmm6`。

接著比較這幾種切換：

| | xv6 `swtch`（RISC-V） | `uswitch`（Windows x64） |
| --- | --- | --- |
| 保存的整數暫存器 | `ra`、`sp`、`s0`–`s11` | `rsp`、`rbx`、`rbp`、`rdi`、`rsi`、`r12`–`r15` |
| 保存的浮點與向量暫存器 | 無 | `xmm6`–`xmm15` |
| 恢復執行的位址在哪裡 | `ra`，存在 `struct context` | 在舊堆疊上，透過保存的 `rsp` 找到 |
| 狀態存到哪裡 | `p->context` 或 `cpu->context` | 執行緒表中的 `struct uctx` |
| 分頁表 | 不動：所有核心執行緒共用核心分頁表，使用者分頁表在 `uservec`／`userret` 切換 | 不動：同一個行程 |

## Linux／NT 對照

- **Linux：** 執行緒是以 `clone` 建立、帶有 `CLONE_VM`、`CLONE_THREAD` 等旗標的 task；行程則是不帶這些旗標建立的 task（[clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)）。練習 E 會在追蹤紀錄中看到這些旗標。
- **NT：** 行程提供資源（位址空間、handle、安全性內容），執行緒才是被排程的單位（[Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)）。練習 F 顯示了識別碼。
- **NT 沒有 `fork`。** `CreateProcess` 從執行檔啟動新程式。`fork` 複製呼叫者行程，再由 `exec` 取代映像檔，所以這一對呼叫有一步便宜、一步昂貴。把你量到的 `fork` 加 `exec`，和只有 `fork` 及 `CreateProcess` 的數字比較。
- **使用者層級切換：** NT 有 fiber，glibc 有 `ucontext`。兩者都不會搶占。練習 E 第 4 題問的是 `swapcontext` 是否留在使用者模式。

## 檢核

在 `NOTES.md` 中回答。

1. 列出 `swtch` 保存了什麼、存在哪裡。psABI 說 `ra` 不跨呼叫保留，`swtch` 卻保存它，為什麼？為什麼 `swtch` 可以略過 `t0`–`t6`、`a0`–`a7` 與所有浮點暫存器？
2. 解釋 xv6 從一個行程切到另一個行程時，為何要經過排程器執行緒。用練習 C 的發現回答：排程器在哪個堆疊上執行、切換期間持有哪把鎖，以及 `kexit()` 為何需要一個不屬於自己的堆疊。
3. 說明你的使用者層級切換不必保存哪些東西及原因：呼叫者保存暫存器、分頁表、特權模式、`sepc` 與 `sstatus`、trap frame。再用練習 G 的證據說明切換必須保存什麼。
4. 比較 `fork` 加 `exec` 與 `CreateProcess`：各自複製或建立了什麼、練習 B 的 `uvmcopy` 分頁數，以及練習 E 與 F 的成本。
5. 從你的 `strace` 輸出，列出執行緒與行程各自的 clone 旗標，並對應到 NT 行程與執行緒的劃分。
6. 解釋 `pipeline -leak` 為何卡住，指出讓每個讀取者一直等待的檔案描述元。

## 常見失敗情況

- **管線卡住。** 某處還開著 pipe 的寫入端：可能在父行程，也可能在沒有關閉不用端點的子行程。只有所有寫入端都關閉，讀取端才會看到檔案結尾。
- **`exec cat failed`。** xv6 的程式放在 `/`，而 `exec("cat", ...)` 是相對於目前目錄，所以請從 `/` 執行 `pipeline`，或把路徑改成 `/cat`。
- **輸出順序很怪。** 父行程與子行程都寫到主控台。`pid N exited` 的順序取決於排程器。
- **`b swtch` 每個計時器 tick 都觸發。** xv6 就是這麼頻繁地切換。用 `CPUS=1`，並一直 `c`，直到抓到你要的方向。
- **在 `kfork` 中 `p p->name` 印出 `value has been optimized out`。** 中斷點時 `p` 還沒被設定，而且核心以 `-O` 建置。只有一個 hart 時改用 `cpus[0].proc`，或先 `next`。
- **`b forkret` 沒有觸發，又停在 `kfork`。** 父行程會一直執行到被阻塞，所以它的下一次 `fork` 先到。先 `disable` `kfork` 的中斷點。
- **使用者執行緒以 `scause 0x2` 被終止。** 它執行了浮點指令，而 xv6 讓 `sstatus` 的 `FS` 欄位維持關閉。
- **使用者執行緒一啟動就當掉。** 確認 `sp` 指向堆疊頂端而不是底端（堆疊往下長）、有 16 位元組對齊，而且 `ra` 是 `thread_entry`。
- **執行緒函式返回後程式當掉。** 它沒有可以返回的地方。一律經由 `thread_entry` 進入；它會把執行緒標成 `FREE` 並切換出去。
- **`make: *** missing separator`。** Makefile 的指令行是用空白而不是 tab 開頭。
- **`strace -f` 印出太多東西。** 用 `-e trace=...` 過濾，並用 `-o` 寫到檔案。
- **`strace -c -f ./procthread switch` 要跑好幾分鐘。** 它會發出將近五百萬次系統呼叫，而每次被追蹤的呼叫都很慢。
- **Windows：`uthread_win64.exe` 沒有訊息就結束。** 在 uthread 堆疊上引發的例外無法被分派（見原始碼中的警告）。執行緒內容只寫單純的 C。
- **Windows：你的 `SKIP_XMM` 版本沒有壞掉。** 你的編譯器把總和放在別的地方。下結論前先看 `worker` 的反組譯。
- **Windows：每次執行的時間差很多。** 回報多次執行的範圍，而不是單一數字。

## 後續會用到的地方

- 階段 03 會把 `uvmcopy` 的完整複製改成寫入時複製，並重新計算分頁數。
- 階段 05 會回到切換期間持有的那把鎖，以及 `sleep`／`wakeup`。
- 階段 06 會換掉 `scheduler()` 的輪流迴圈，並量測喚醒延遲，也就是你在事件來回傳遞中看到的成本。
- 階段 08 的事件迴圈是使用大量執行緒之外的另一種選擇；階段 09 的伺服器絕不能因用戶端而阻塞。

## 下一步

[建置計畫](../../SYLLABUS.zh-TW.md)的階段 03 處理虛擬記憶體：分頁表、寫入時複製的 `fork`，以及共享與私有映射。
