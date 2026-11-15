[English](README.md) | **繁體中文** | [實作建置計畫，階段 01](../../SYLLABUS.zh-TW.md)

# 實驗 01：系統呼叫，跨越邊界

> **草稿，待審閱（2026-11-15）。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A–D）：未執行。** 撰寫本實驗的機器沒有 RISC-V 工具鏈與 QEMU。標為「*預期*」的內容，都來自 2026-11-15 閱讀的 [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) master。
> - **Linux 部分（練習 E）：未執行。** 那台機器沒有 Linux 發行版，也沒有 Linux 工具鏈，所以 `linux/boundary.c` 也沒有編譯過。
> - **Windows 部分（練習 F、G）：已建置並執行**，於 2026-11-15 在那台機器上以 WinLibs 的 gcc 16.2 與 gdb 18.1 完成。完整輸出在 [`windows/example-output.txt`](windows/example-output.txt)，下文引用的數字都取自該檔。

## 目標

在 xv6 中逐道指令追蹤一次系統呼叫：從使用者程式碼進入核心，再回到使用者程式碼。接著在 Linux 上用 `strace`、在 Windows 上從 ntdll 的 stub 與除錯器找出同一條邊界。最後交給每個核心一個它必須拒絕的指標，並用該核心自己的術語解釋每一次拒絕。

```
使用者  usys.S 的 write()：li a7, SYS_write ; ecall
  硬體  sepc <- pc，scause <- 8，sstatus.SPP <- 0（來自使用者），模式 <- S，pc <- stvec
核心    uservec   (trampoline.S)  把 31 個使用者暫存器存進 TRAPFRAME，載入核心的 sp、tp、satp
        usertrap() (trap.c)       stvec <- kernelvec，epc += 4，intr_on()，syscall()
        syscall()  (syscall.c)    num = a7 ; a0 = syscalls[num]()
        sys_write -> filewrite -> consolewrite -> either_copyin -> copyin
        prepare_return() (trap.c) stvec <- uservec，填入 trapframe->kernel_*，SPP <- 0，sepc <- epc
        userret   (trampoline.S)  satp <- 使用者分頁表，還原暫存器，sret
使用者  ecall 後面的 ret，結果在 a0
```

函式名稱以 2026-11-15 閱讀的 xv6-riscv master 為準，其中有 `kfork`、`kexit`、`kexec`、`prepare_return`、`printk` 與 `pause` 系統呼叫。MIT 2025 年的 syscall 實驗已經使用 `kfork()`。較舊的版本與較舊的課本使用 `fork`、`exit`、`exec`、`usertrapret`、`printf` 與 `sleep`。開始前先執行 `grep -n "prepare_return\|usertrapret" kernel/*.c`，看看你的版本用哪一組名稱。

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/badptr.c`](xv6/badptr.c) | xv6 使用者程式：把四個錯誤位址交給 `write`、`pipe` 與 `fstat`，再自己從核心位址讀取 | 未在此編譯 |
| [`xv6/trapinfotest.c`](xv6/trapinfotest.c) | 練習 C 新增的 `trapinfo()` 系統呼叫的測試 | 未在此編譯 |
| [`linux/boundary.c`](linux/boundary.c) | 用三種方式做同一次 `write`、錯誤指標，以及留在使用者模式與進入核心的呼叫成本 | 未在此編譯 |
| [`windows/nt_boundary.c`](windows/nt_boundary.c) | 對 Win32 與 ntdll 問同樣的問題 | 已在此建置並執行 |
| [`windows/win32_write.c`](windows/win32_write.c)、[`find_nt_routine.gdb`](windows/find_nt_routine.gdb)、[`user_mode_calls.gdb`](windows/user_mode_calls.gdb) | 只有兩次呼叫的 Win32 程式，以及兩個 GDB 腳本，顯示它進入了哪個 ntdll 常式 | 已在此建置並執行 |
| [`windows/example-output.txt`](windows/example-output.txt) | Windows 程式的輸出紀錄 | 2026-11-15 記錄 |

## 環境設定

- 實驗 00 的工作環境；學習期間只用一個 hart：`make CPUS=1 qemu-gdb`。
- 要做 MIT 的現成練習，就照 MIT 實驗頁的做法，使用實驗儲存庫及其 `syscall` 分支：

  ```sh
  git clone git://g.csail.mit.edu/xv6-labs-2025
  cd xv6-labs-2025
  git checkout syscall
  make clean
  ```

  本實驗的練習 A–D 在那個版本或一般的 xv6-riscv 複本中都能做。MIT 的 [syscall 實驗](https://pdos.csail.mit.edu/6.1810/2025/labs/syscall.html)另有自己的練習：「Using gdb」一節的問題和練習 B 重疊，還有一個沙箱系統呼叫，以及一個攻擊實驗版本未清零記憶體的練習。
- **Linux：** 實驗 00 的 WSL 2 Ubuntu，再加上 `sudo apt-get install strace`。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc。WinLibs 版本也附有 `gdb`。

## 先讀

- xv6 課本第 2 章，以及第 4 章的 4.3 與 4.4 節。這是 MIT 為這個實驗列的閱讀範圍。
- `user/usys.pl`、`user/user.h`、`kernel/syscall.h`、`kernel/syscall.c`。
- `kernel/trampoline.S`（`uservec`、`userret`）、`kernel/trap.c`（`usertrap`、`prepare_return`）、`kernel/proc.h` 的 `struct trapframe`。
- `kernel/vm.c`：`walkaddr`、`copyin`、`copyout`、`vmfault`；`kernel/memlayout.h`：`TRAMPOLINE`、`TRAPFRAME`、`KERNBASE`。

## 練習

和實驗 00 一樣，執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 讀 stub（xv6）

先執行一次 `make`，再打開產生出來的 `user/usys.S`，或在任何 `user/*.asm` 中找 `write:`。

**預測：** 載入 `a7` 的值；`fd`、`buf`、`n` 分別在哪個暫存器；執行 `ret` 時回傳值在哪裡。

*預期（依原始碼）：* 先是 `li a7, SYS_write`，master 的 `kernel/syscall.h` 中 `SYS_write` 為 16，接著是 `ecall`，然後是 `ret`。參數已經在 `a0`、`a1`、`a2`，因為 C 呼叫慣例把它們放在那裡。結果回到 `a0`，因為 `syscall()` 把它存進 `p->trapframe->a0`，而 `userret` 最後才還原 `a0`。RISC-V 上的 Linux 用同樣的慣例：`ecall`、編號放在 `a7`、參數放在 `a0`–`a5`（[syscall(2)](https://man7.org/linux/man-pages/man2/syscall.2.html)）。

### B. 在 GDB 中追蹤一次 `write`（xv6）

所有來自使用者模式的陷阱都會執行 `uservec`，計時器中斷也一樣，所以只在 `ecall`（`scause` 為 8）且 `a7` 為 16 時停下。`uservec` 在 `TRAMPOLINE` 位址執行，也就是 `MAXVA - PGSIZE` = `0x3ffffff000`，而不是 GDB 的 `uservec` 符號所記錄的核心連結位址。

```
(gdb) b *0x3ffffff000 if $scause == 8 && $a7 == 16
(gdb) c
```

如果沒有自然發生的 write，就在 xv6 主控台輸入 `echo hi`。停下後，**先預測，再印出：** `$priv`、`$satp`、`$sepc`、`$sp`、`$a0`、`$a1`、`$a2`、`$stvec`。接著：

```
(gdb) x/12i $pc           uservec；這個位址沒有符號，請對照 trampoline.S
(gdb) si                  （重複到 "csrw satp, t1" 之後；每次都印出 $satp 與 $sp）
```

到發出呼叫的程式的 `.asm` 檔中查 `$sepc`（shell 是 `user/sh.asm`）。`x/2i $sepc` 也可能可行，但 GDB 是透過 QEMU、以 CPU 目前的特權讀取記憶體，而監督者模式通常不能讀 `PTE_U` 分頁。如果 GDB 回答 `Cannot access memory`，原因就在這裡。

執行到 `jalr t0` 就進入 `usertrap()`，核心符號又能用了：

```
(gdb) bt
(gdb) p/x $sstatus        第 8 位元（SPP）：陷阱來自哪個模式
(gdb) next                （重複到 "p->trapframe->epc = r_sepc();" 之後）
(gdb) p/x p->trapframe->epc
(gdb) next                （重複到 "p->trapframe->epc += 4;" 之後）
(gdb) p/x p->trapframe->epc
(gdb) b syscall
(gdb) c
(gdb) next
(gdb) p num
```

回程要先算出 `userret` 實際執行的位址。它的符號是核心連結位址，但它在 trampoline 頁中執行：

```
(gdb) p/x 0x3ffffff000 + ((long)&userret - (long)&trampoline)
(gdb) b *<算出的值>
(gdb) c
(gdb) p/x $a0             usertrap() 回傳的使用者 satp
(gdb) si                  （重複到 sret；在它之前印出 $sepc 與 $sstatus）
(gdb) si
(gdb) p $priv
(gdb) p/x $pc
```

*預期（依原始碼）：* 在 `uservec` 時，`$priv` 是 1（監督者），`$satp` 仍是使用者分頁表，`$sepc` 指向 `ecall`，`$sp` 是使用者堆疊，`$stvec` 等於 `0x3ffffff000`。`csrw satp, t1` 切換到核心分頁表，程式能繼續執行，是因為 trampoline 頁在兩張分頁表中映射在同一個虛擬位址。`sret` 之後，`$priv` 是 0，`$pc` 是 `ecall` 的下一道指令。

### C. 新增系統呼叫：`trapinfo`（xv6）

新增 `int trapinfo(struct trapinfo *ti)`，把陷阱路徑為這一次呼叫保存的內容複製到使用者記憶體。建立 `kernel/trapinfo.h`，內容是 [`xv6/trapinfotest.c`](xv6/trapinfotest.c) 開頭列出的結構（`epc`、`ra`、`sp`、`a0`、`a7`，皆為 `uint64`）。接著修改每個 xv6 系統呼叫都需要的五個地方：

1. `user/user.h`：前置宣告 `struct trapinfo;` 與函式原型。
2. `user/usys.pl`：`entry("trapinfo");`。
3. `kernel/syscall.h`：`SYS_trapinfo`，取你的版本中下一個未使用的編號。
4. `kernel/syscall.c`：`extern` 宣告，以及 `syscalls[]` 中的 `[SYS_trapinfo]` 項目。
5. `kernel/sysproc.c`：`sys_trapinfo()`。用 `argaddr` 取得目的位址，從 `myproc()->trapframe` 填好一個 `struct trapinfo`，再用 `copyout` 複製出去。先確認你版本中 `copyout` 的參數；master 的第二個參數是行程大小。`copyout` 失敗時回傳 -1。

把 `trapinfotest.c` 複製到 `user/`，在 `UPROGS` 加上 `$U/_trapinfotest`，然後執行。

**執行前先預測：** `epc` 是 `ecall` 的位址，還是下一道指令的位址？在 `user/trapinfotest.asm` 中找出兩者。`a0` 是否等於 `&ti`？（trap frame 中的 `a0` 是 `uservec` 保存的值，那時 `syscall()` 還沒寫入回傳值。）`ra` 是什麼？最後三個錯誤的目的位址是否都回傳 -1？

**接著決定哪些不該複製。** `struct trapframe` 也有 `kernel_satp`、`kernel_sp` 與 `kernel_trap`。使用者程式碼自己讀不到 trap frame 那一頁，因為它映射時沒有 `PTE_U`。寫下：如果 `trapinfo` 把這三個欄位複製出去，行程會得知什麼？為什麼會隨機化自身配置的核心，把這種洩漏當成安全漏洞？

### D. 錯誤指標（xv6）

把 [`xv6/badptr.c`](xv6/badptr.c) 複製到 `user/`，在 `UPROGS` 加上 `$U/_badptr`，並**在執行前填好這張表：**

| 位址 | `write(1, addr, 8)`，核心複製進來 | `pipe(addr)`，核心複製出去 | `fstat(0, addr)`，核心複製出去 |
| --- | --- | --- | --- |
| `0`，使用者程式碼的起點 | | | |
| `KERNBASE` `0x80000000` | | | |
| `TRAPFRAME` `0x3fffffe000` | | | |
| `MAXVA` `0x4000000000` | | | |

也預測第 4 部分：使用者程式碼自己從 `KERNBASE` 讀取時會發生什麼、核心會印出什麼、子行程的結束狀態是多少。

<details>
<summary>原始碼推得的結果。請在預測並執行之後再看。</summary>

依 2026-11-15 的 xv6-riscv master：

- `walkaddr()` 拒絕 `MAXVA` 以上的位址、沒有 `PTE_V` 的 PTE，以及沒有 `PTE_U` 的 PTE。失敗時，`copyin`／`copyout` 會呼叫 `vmfault()`，而它只會為 `p->sz` 以下、延遲配置的堆積分頁配置記憶體。
- `copyout()` 還會拒絕沒有 `PTE_W` 的分頁，所以位址 0 的唯讀程式碼頁可以被 `write` 讀取，卻不能被 `pipe` 或 `fstat` 寫入。沒有這項檢查的版本會讓 `pipe(0)` 覆寫程式自己的程式碼。如果你的版本是這樣，預期程式稍後會當掉，請解釋原因。
- `consolewrite()` 在第一次複製失敗時停止，回傳目前已複製的位元組數，所以主控台的 `write` 案例回傳 0，而不是 -1。
- `KERNBASE` 失敗，是因為使用者分頁表在那裡根本沒有映射：`copyin` 走訪的是行程自己的分頁表，不是核心的。`TRAPFRAME` 在那張表中有映射，但沒有 `PTE_U`。
- 第 4 部分：讀取造成分頁錯誤（`scause` 13）。`usertrap()` 印出 `usertrap(): unexpected scause 0xd ...`，其中 `stval=0x80000000`，接著把行程標記為已終止，父行程看到的狀態是 -1。

</details>

**刻意弄壞：** 刪掉 `walkaddr()` 中的 `PTE_U` 檢查。先預測 `write(1, TRAPFRAME, 8)` 現在會印出什麼，再執行。核心現在替行程複製了一個行程自己不能讀的分頁，這類錯誤稱為 confused deputy。最後把檢查加回去。

### E. Linux 上的同一條邊界（WSL 2）

```sh
gcc -O2 -Wall -o boundary linux/boundary.c
./boundary
strace -o boundary.trace ./boundary > /dev/null
strace -c ./boundary > /dev/null
```

**先預測，再到 `boundary.trace` 與 `-c` 摘要中確認：**

1. 程式用三種方式寫出同一行：libc 包裝函式、`syscall(SYS_write, ...)`、直接的 `syscall` 指令。`strace` 分得出來嗎？
2. 兩次錯誤指標的 write 回傳什麼？`errno` 是什麼？
3. `strace -c` 算到幾次 `getpid`？`clock_gettime` 有沒有出現？[vdso(7)](https://man7.org/linux/man-pages/man7/vdso.7.html) 列出 `__vdso_clock_gettime` 是 x86-64 vDSO 符號之一，並說明 vDSO 呼叫不會出現在 `strace` 中。glibc 2.25 起，`getpid()` 每次都會發出真正的系統呼叫（[getpid(2)](https://man7.org/linux/man-pages/man2/getpid.2.html)）。如果 `clock_gettime` 以系統呼叫的形式出現，記錄下來，並找出你的系統為什麼會這樣。
4. 在 `strace` 下，`syscall(SYS_getpid)` 比沒有 `strace` 時慢多少？

和 xv6 對照暫存器。x86-64 上，編號放在 `rax`，參數依序放在 `rdi`、`rsi`、`rdx`、`r10`、`r8`、`r9`（[syscall(2)](https://man7.org/linux/man-pages/man2/syscall.2.html)）。第四個參數用 `r10` 而不是 `rcx`，因為 `SYSCALL` 指令把返回位址存進 `rcx`、把旗標存進 `r11`。Linux [`arch/x86/entry/entry_64.S`](https://github.com/torvalds/linux/blob/master/arch/x86/entry/entry_64.S) 中 64 位元進入程式碼上方的註解說明了這點，並補充 `SYSCALL` 不會在堆疊上保存任何東西。RISC-V 的 `ecall` 也不在堆疊上保存任何東西：硬體把陷阱記錄在 `sepc`、`scause`、`stval` 與 `sstatus`，每個通用暫存器都由 `uservec` 自己保存。

### F. Windows 上的同一條邊界（已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o nt_boundary.exe nt_boundary.c
.\nt_boundary.exe
```

**執行前先預測：** 第 1 部分的每一列，呼叫會不會進入核心？進入核心一次，大約等於幾次一般函式呼叫？然後執行。

2026-11-15 的觀察結果（Windows 組建 26300、Intel Core i7-11700、有 hypervisor；[完整輸出](windows/example-output.txt)）：

| 呼叫 | 每次 ns | 執行緒核心時間 | 執行緒使用者時間 |
| --- | ---: | ---: | ---: |
| 一般 C 函式呼叫 | 2.0 | 16 ms | 484 ms |
| `GetCurrentProcessId()` | 2.4 | 0 ms | 500 ms |
| `GetTickCount64()` | 2.7 | 0 ms | 500 ms |
| `GetProcessHandleCount()` | 373.1 | 266 ms | 266 ms |
| `NtQueryInformationProcess()` | 438.0 | 234 ms | 297 ms |
| `WaitForSingleObject(已設定的事件, 0)` | 429.7 | 250 ms | 266 ms |

這次執行顯示了什麼、沒有顯示什麼：

- 有三種呼叫的成本和一般呼叫差不多，另外三種要 370–440 ns，約為一般呼叫的 190 到 220 倍。下面的反組譯說明了原因。
- 只在使用者模式的那幾列，幾乎所有時間都算在使用者模式。進入核心的那幾列，當天每次執行都大約各算一半給兩種模式，儘管每次呼叫在使用者模式的部分只有少數幾道指令。Learn 的 [GetThreadTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes) 頁面沒有說明這些時間如何收集，所以不要把這個比例當成時間實際花在哪裡的量測。**練習：** 提出一個解釋，並設計測試來驗證。階段 06 的 WPR/WPA 追蹤是一種檢查方法。

第 2 部分印出四個 ntdll stub 的開頭。在這個組建上四個的形狀都一樣。以 GDB 看 `NtWriteFile` 是這樣（它的匯出名稱也叫 `ZwWriteFile`；Learn 說明兩者[從使用者模式呼叫時行為完全相同](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)）：

```
mov    r10,rcx
mov    eax,0x8
test   BYTE PTR ds:0x7ffe0308,0x1
jne    <+21>
syscall
ret
int    0x2e
ret
```

- `eax` 放的是系統服務編號：在這個組建上，`NtWriteFile` 是 0x8、`NtQueryInformationProcess` 是 0x19、`NtWaitForSingleObject` 是 0x4、`NtClose` 是 0xF。這些編號屬於內部細節，任何 Windows 組建都可能改變。
- `mov r10, rcx` 的理由和 Linux 用 `r10` 一樣：`syscall` 會覆寫 `rcx`。
- `0x7ffe0308` 的測試決定要用 `syscall` 還是 `int 2Eh`。這個位址位於 `KUSER_SHARED_DATA` 中。Learn 的[結構頁面](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-kuser_shared_data)描述了一個 `SystemCall` 成員，系統使用不同的系統呼叫機制時會設定它。偏移量請到 WDK 的 `ntddk.h` 確認。

接著照建置計畫的要求，用除錯器找出 Win32 呼叫底下的常式：

```powershell
gcc -O0 -g -Wall -o win32_write.exe win32_write.c
gdb -q -batch -x find_nt_routine.gdb win32_write.exe
gdb -q -batch -x user_mode_calls.gdb win32_write.exe
```

在此觀察到的結果（標準輸出被導向管線）：

- 兩次 `WriteFile`（寫到標準輸出與寫到磁碟檔案）都停在 `ntdll!ZwWriteFile`，呼叫者是 `KernelBase.dll!WriteFile`，再上一層是 `main`。`NtDeviceIoControlFile` 的中斷點沒有觸發。
- `GetCurrentProcessId` 是 `mov rax, QWORD PTR gs:0x40` 加上 `ret`。在 x64 Windows 上，`gs` 指向目前執行緒的環境區塊（TEB），所以行程 id 是從使用者記憶體讀出來的。
- `GetTickCount64` 讀取 `0x7ffe0004` 與 `0x7ffe0320`，也就是 `KUSER_SHARED_DATA` 的 `TickCountMultiplier` 與 `TickCount` 成員。核心負責更新這些資料，使用者程式碼只讀取。
- `kernel32.dll` 中的 `GetProcessHandleCount` 只有一道跳到另一個 DLL 的間接跳躍。請自己在跳躍目標用 `x/12i` 追下去。

**換你做：** 在真正的主控台視窗（Windows Terminal）中執行 `find_nt_routine.gdb`，再把輸出導向檔案執行一次。主控台的寫入還會到達 `NtWriteFile` 嗎？記錄你的發現。這一點在此沒有測試。

### G. Previous mode：NT 上的錯誤指標（已在此執行）

`nt_boundary.exe` 的第 3 部分把核心半部的位址與 `NULL` 當作緩衝區傳入。觀察結果：

| 呼叫 | 緩衝區 | 結果 |
| --- | --- | --- |
| `NtQueryInformationProcess` | 核心位址 | `0xC0000005`（`STATUS_ACCESS_VIOLATION`） |
| `NtQueryInformationProcess` | `NULL` | `0xC0000005` |
| `NtWriteFile` | 核心位址 | `0xC0000005`，`RtlNtStatusToDosError` 轉成 998 |
| `NtWriteFile` | `NULL` | `0xC00000E8`，轉成 1784 |
| `WriteFile` | 核心位址 | `FALSE`，錯誤 998，"Invalid access to memory location." |
| `WriteFile` | `NULL` | `FALSE`，錯誤 1784，"The supplied user buffer is not valid for the requested operation." |

用 Learn 對原生服務的說明解釋每一列：來自使用者模式的呼叫，參數一律視為不可信，常式會探測呼叫者提供的緩衝區，確認它們位於使用者記憶體中（[Nt 與 Zw 常式](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)、[PreviousMode](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/previousmode)）。範圍不在使用者模式內時，[ProbeForRead](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-probeforread) 會引發 `STATUS_ACCESS_VIOLATION`。它也提醒，另一個執行緒隨時可能解除映射或改變緩衝區，探測之後也一樣，所以之後的存取同樣需要例外處理。兩列 `NULL` 的結果不同，表示這兩個服務對 `NULL` 緩衝區的檢查方式不同；Learn 沒有說明原因。Win32 再把每個 `NTSTATUS` 轉成 Win32 錯誤碼。

## Linux／NT 對照

- **誰檢查指標。** xv6 透過行程自己的分頁表複製，核心位址在那裡根本沒有映射。Linux 與 NT 的核心所在的位址空間同時映射了使用者記憶體與核心記憶體，使用者提供的指標可能指向核心記憶體，所以核心必須檢查範圍。Linux 通用的 `__access_ok` 把位址和 `TASK_SIZE_MAX` 比較，其註解指出使用者位址空間獨立的架構可以略過這項檢查（[access_ok.h](https://github.com/torvalds/linux/blob/master/include/asm-generic/access_ok.h)）。NT 在 previous mode 為使用者模式時探測緩衝區。
- **Nt 與 Zw。** NT 的系統服務有 Nt 與 Zw 兩種入口。從使用者模式呼叫時兩者相同。從核心模式呼叫時，Zw 表示參數可信，Nt 則依執行緒的 previous mode 判斷（[Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)）。
- **有正式文件的介面。** 有正式文件的介面是 Win32。Learn 把 `NtQueryInformationProcess` 這類原生呼叫描述為內部介面，各版本之間可能變動（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess)）。Linux 的 ABI 文件把核心系統呼叫介面本身列為穩定介面（[ABI](https://docs.kernel.org/admin-guide/abi.html)），所以程式可以像 `syscall(2)` 那樣直接以編號呼叫。Windows 程式若寫死第 2 部分的服務編號，換一個組建就會壞掉。
- **跳過核心的呼叫。** Linux 的 vDSO 在使用者模式中提供 `clock_gettime`。Windows 從 `KUSER_SHARED_DATA` 提供 `GetTickCount64`，從 TEB 提供 `GetCurrentProcessId`。xv6 沒有這種路徑：每個系統呼叫都會進入陷阱。

## 檢核

在 `NOTES.md` 中回答。

1. 畫出 `write(1, buf, n)` 進入 xv6 核心再返回的過程。標出每個被保存的暫存器及保存位置、每次堆疊切換、每張用到的分頁表，以及每次特權切換。標明哪些步驟由硬體在 `ecall` 與 `sret` 時完成，哪些由軟體完成。
2. 為什麼 trampoline 頁必須在使用者與核心分頁表中映射到同一個虛擬位址？為什麼 `TRAPFRAME` 映射時沒有 `PTE_U`？
3. 為什麼 `usertrap()` 要把 `epc` 加 4？不加會怎樣？為什麼它要等讀完 `sepc`、`scause`、`sstatus` 之後才開啟中斷？
4. 說明 `buf` 指向核心記憶體時會發生什麼：用 xv6 的術語（`walkaddr`、使用者分頁表、回傳值），再用 NT previous mode 的術語（探測、`STATUS_ACCESS_VIOLATION`、Win32 錯誤 998）。最後補一行 Linux 的說法（`access_ok`、`EFAULT`）。
5. 把練習 E 與 F 中你認定會進入核心或留在使用者模式的每個呼叫填成一張表。每一項都附上證據（stub 位元組、反組譯、`strace`、計時），而不是預期。
6. 你的 `trapinfo` 複製了 trap frame 的哪些欄位、沒有複製哪些，原因是什麼？

## 常見失敗情況

- **用名稱設在 `uservec` 或 `userret` 的中斷點永遠不觸發。** 這些符號是核心連結位址，但程式碼在 `TRAMPOLINE` 執行。改設在 `*0x3ffffff000` 與算出來的 `userret` 位址。
- **`0x3ffffff000` 的中斷點一直觸發。** 來自使用者模式的計時器中斷也會進入 `uservec`。加上 `$scause == 8` 條件。
- **`No symbol "p" in current context`。** 你在錯誤的 frame。先 `bt`，再用 `frame N` 或 `up`。
- **主控台出現 `unknown sys call 23`。** `syscall()` 找不到處理函式：`syscall.h` 的編號和 `syscalls[]` 的項目對不上，或你沒有重新建置。
- **`implicit declaration of function 'trapinfo'`。** `user/user.h` 缺少原型。Makefile 用 `-Werror` 建置，任何警告都會讓建置停止。
- **`undefined reference to 'trapinfo'`。** `usys.pl` 缺少 `entry("trapinfo")` 這一行。
- **`exec trapinfotest failed`。** 程式不在 `UPROGS` 中，所以不在 `fs.img` 裡。
- **你的版本名稱不同：** `fork`／`kfork`、`usertrapret`／`prepare_return`、`printf`／`printk`。設中斷點前先 grep。
- **`badptr` 在位址 0 的案例之後當掉。** 你的 `copyout` 沒有 `PTE_W` 檢查，所以 `pipe(0)` 覆寫了程式的程式碼。請解釋這次當機，而不是把它藏起來。
- **Windows：GDB 印出 "Could not find platform independent libraries"。** 在撰寫本實驗的機器上，WinLibs 的 gdb 18.1 會印出這個 Python 警告，但腳本照常運作。
- **Windows：`Function "NtWriteFile" not defined`。** 程式啟動前 ntdll 還沒載入，所以兩個腳本都先停在 `main`。
- **Windows：每次執行的時間差很多。** 電源計畫、turbo、背景負載與 hypervisor 都會反映在這些數字上。多跑幾次，回報範圍，而不是只報一次。

## 後續會用到的地方

- 階段 02 從這個 trap frame 出發：`fork` 會複製它，而 `swtch` 保存的暫存器少得多。
- 階段 03 會改變 `walkaddr` 與 `vmfault` 接受的範圍：分頁表、延遲配置與寫入時複製。
- 階段 08 與 17 會讓伺服器處理由用戶端控制的記憶體。本實驗的規則「來自另一邊的指標不可信」，會成為共享緩衝區的規則。

## 下一步

[建置計畫](../../SYLLABUS.zh-TW.md)的階段 02 會用 `fork`、`exec`、`pipe`、`wait` 建立管線，追蹤 `swtch`，並撰寫使用者層級執行緒。
