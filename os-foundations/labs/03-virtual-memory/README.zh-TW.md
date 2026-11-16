[English](README.md) | **繁體中文** | [實作建置計畫，階段 03](../../SYLLABUS.zh-TW.md)

# 實驗 03：虛擬記憶體

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A、B、C、F）：已建置並執行**，環境是 Windows 11 上 podman 中的 Ubuntu 24.04 容器。podman machine 是一台 WSL 2 虛擬機器，所以這不是 WSL 2 Ubuntu 本身，從中量到的時間都屬於虛擬機器的數字。容器內的工具：QEMU 8.2.2、riscv64-linux-gnu-gcc 13.3.0、gdb-multiarch 15.1。原始碼是 xv6-riscv 的 `fa2ddcb7` commit（見下方固定版本）。[`xv6/run-xv6.sh`](xv6/run-xv6.sh) 把參考解答（沒有放進專案）套用到原始碼的複本上，並用 expect 操作 QEMU 與 GDB。完整輸出在 [`xv6/example-output.txt`](xv6/example-output.txt)。寫入時複製版本的核心通過了 `usertests -q`。
> - **Linux 部分（練習 D、E）：已建置並執行**，在同一個容器中，核心是 WSL 的 6.6.87.2。輸出：[`linux/example-output.txt`](linux/example-output.txt)。
> - **Windows 部分（練習 G）：已建置並執行**，在 Windows 11 主機上以 WinLibs 的 gcc 16.2 完成。輸出：[`windows/example-output.txt`](windows/example-output.txt)。
> - **沒有執行的部分：** 手動輸入 GDB 指令（同樣的指令改由腳本以批次模式執行），以及標為「*未在此執行*」的「刻意弄壞」步驟。

## 目標

讀懂一張分頁表、修改一張分頁表，並看著它運作。在 xv6 中，先印出一個行程的分頁表，再把 `fork` 的完整複製換成寫入時複製（copy-on-write，以下簡稱 COW），並統計兩種版本各複製了幾頁。在 Linux 上，讓兩個行程以共享與私有兩種方式映射同一個檔案，再在映射仍存在時截短檔案，直到收到 `SIGBUS`。在 Windows 上，讓記憶體走過三種狀態（free、reserved、committed），並讓兩個行程共用一個 section。最後手算一個虛擬位址的轉換，並在 GDB 中確認每一步。

```
Sv39 虛擬位址（63..39 位元必須和第 38 位元相同；xv6 只使用 MAXVA = 2^38 以下的位址）
  | L2 索引：38..30 位元 | L1 索引：29..21 位元 | L0 索引：20..12 位元 | 位移：11..0 位元 |

satp.PPN ─► 根分頁 ─[L2]─► PTE ─► L1 分頁 ─[L1]─► PTE ─► L0 分頁 ─[L0]─► PTE ─► 資料分頁 + 位移
            每頁 512 個 8 位元組的 PTE；PTE 的 53..10 位元放下一層的實體分頁號碼，
            9..8 位元（RSW）留給軟體使用，7..0 位元是 D A G U X W R V
```

## 固定的 xv6 commit

本實驗的所有內容都是針對 **xv6-riscv commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`**（2026-11-05，"usertests: bound the zero-length pipe read regression test"）撰寫並執行的。在實驗 00 的 clone 中切到這個 commit，你的原始碼才會和產生輸出紀錄的版本一致：

```sh
cd ~/xv6-riscv
git fetch
git checkout fa2ddcb7ca87a77af579380a6eafd5aff163b44f
make clean
```

在這個版本中，`fork()` 叫 `kfork()`，核心用 `printk` 輸出，`sleep` 不帶參數：呼叫者先 `sleep_prepare(chan)`，釋放自己的鎖，再呼叫 `sleep()`。`copyin`、`copyout`、`vmfault` 的第二個參數是行程大小。`sbrk()` 立即配置，`sbrklazy()` 只移動行程的結尾，每一頁在第一次被存取時才由 `vmfault()` 映射。MIT 的實驗分支（`xv6-labs-2025` 中的 `git checkout pgtbl`、`cow`）起始程式碼不同，所以這裡的內容只適用於原版 xv6-riscv。

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/vmprinttest.c`](xv6/vmprinttest.c) | 印出自己的 text、data、堆疊與 heap 位置，再印出自己的分頁表；加上 `fork` 參數則印出 fork 出的子行程的分頁表 | 已在此建置並執行 |
| [`xv6/cowtest.c`](xv6/cowtest.c) | 統計一次 `fork` 複製與共用的分頁數、fork 一個占用 2/3 可用記憶體的行程，並讓核心寫入一個 COW 分頁 | 已在此建置並執行 |
| [`xv6/translate.gdb`](xv6/translate.gdb) | 在 GDB 中走訪分頁表，確認手算的轉換 | 已在此以批次模式執行 |
| [`xv6/run-xv6.sh`](xv6/run-xv6.sh)、[`xv6run.exp`](xv6/xv6run.exp)、[`xv6gdb.exp`](xv6/xv6gdb.exp) | 把你的修補檔套用到原始碼的複本、建置，並用 expect 操作 QEMU 與 GDB | 已在此搭配參考解答執行 |
| [`linux/mapshare.c`](linux/mapshare.c) | 兩個行程以 `MAP_SHARED` 與 `MAP_PRIVATE` 映射同一個檔案，並讀 `/proc/self/maps` | 已在此建置並執行 |
| [`linux/sigbus.c`](linux/sigbus.c) | 在映射存在時截短檔案、伺服器被用戶端弄死，以及封存的 memfd | 已在此建置並執行 |
| [`linux/forkcow.c`](linux/forkcow.c) | 從次要分頁錯誤次數觀察 Linux 的寫入時複製 `fork`，以及不同大小下 `fork` 的成本 | 已在此建置並執行 |
| [`linux/run-linux.sh`](linux/run-linux.sh) | 建置並執行三個 Linux 程式 | 已在此執行 |
| [`windows/nt_reserve_commit.c`](windows/nt_reserve_commit.c) | 保留、提交、存取、取消提交、釋放；按需提交；依配置單位列出位址空間 | 已在此建置並執行 |
| [`windows/nt_section.c`](windows/nt_section.c) | 兩個行程共用以分頁檔為後盾的 section、寫入時複製的 view、`SEC_COMMIT` 與 `SEC_RESERVE`、截短已映射的檔案 | 已在此建置並執行 |
| `*/example-output.txt` | 以上所有程式的輸出紀錄 | 2026-11-16 記錄 |

參考解答沒有放進專案：這裡記錄的 xv6 輸出，來自練習 A、B、C 的參考修補檔，它們不在儲存庫中。這些練習對應 MIT 6.1810 的 vmprint 與 cow 作業，請自己寫。要讓 `run-xv6.sh` 執行你的成果，把它存成針對固定 commit 的修補檔，例如在你的 clone 中用 `git diff` 產生：練習 A、B 存成 `0001-vmprint-vmstat.patch`（兩個程式都要加進 `UPROGS`），練習 C 存成疊在 0001 之上的 `0002-cow.patch`。把它們放在 git 會忽略的 `xv6/solution/`，或用 `SOLUTION_DIR` 指向它們。缺少修補檔的步驟會印出說明後略過。

## 環境設定

- 切到固定 commit 的實驗 00 工作環境。學習時只用一個 hart（`make CPUS=1 qemu`），計數與 GDB 過程才能重現。
- 對應的 MIT 實驗是 [pgtbl](https://pdos.csail.mit.edu/6.1810/2025/labs/pgtbl.html) 與 [cow](https://pdos.csail.mit.edu/6.1810/2025/labs/cow.html)。pgtbl 有自己的 `vmprint` 作業（透過 `kpgtbl()` 系統呼叫）、一個與核心共用、供 `ugetpid()` 使用的分頁，以及超級分頁（superpage）。可以當額外練習；本實驗不需要它們。
- 要像這裡一樣不必手動輸入就跑完所有 xv6 部分：`sudo apt-get install expect patch`，然後 `SOLUTION_DIR=<你的修補檔目錄> sh xv6/run-xv6.sh ~/xv6-riscv`（預設是 `xv6/solution/`）。腳本不會碰你的 checkout，它會把原始碼複製到 `/tmp/lab03-xv6/` 再建置。`BREAKIT=1` 會多跑參考計數的弄壞版本，`USERTESTS=1` 會多跑 `usertests -q`。整個腳本（含 `usertests`）在這裡約花三分鐘。
- **Linux：** WSL 2 Ubuntu 加上 gcc，然後 `sh linux/run-linux.sh`。它在 `/tmp/lab03-linux/` 建置。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc。

## 先讀

- xv6 課本第 3 章（分頁表）與第 4 章的分頁錯誤一節。MIT 為 pgtbl 指定第 3 章，為 cow 指定第 4 章。
- `kernel/riscv.h`：`PTE_*` 位元、`PX`、`PTE2PA`、`PA2PTE`、`MAKE_SATP`。
- `kernel/vm.c`：`walk`、`mappages`、`uvmcopy`、`freewalk`、`copyout`、`vmfault`。
- `kernel/kalloc.c`、`kernel/memlayout.h`，以及 `kernel/exec.c` 中建立堆疊與防護頁的部分。
- `kernel/trap.c`：`usertrap()`，它在 `scause` 為 13（載入）與 15（儲存）的分頁錯誤時呼叫 `vmfault()`。

## 練習

和前面一樣，執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 印出分頁表（xv6）

在 `kernel/vm.c` 中寫 `void vmprint(pagetable_t)`。仿照 `freewalk()` 走訪三層：有效的 PTE 如果 `R`、`W`、`X` 都沒設，就指向下一層。每個有效的 PTE 印一行：它對應的起始虛擬位址、PTE 與實體位址，依層級縮排，再加上旗標字母。接著新增系統呼叫 `vmprint()`，印出呼叫者自己的分頁表，要改的五個地方和實驗 01 的 `trapinfo` 相同。把 [`vmprinttest.c`](xv6/vmprinttest.c) 複製到 `user/`，並在 `UPROGS` 加上 `$U/_vmprinttest`。下面的輸出來自參考解答（沒有放進專案）。

**執行 `vmprinttest` 前先預測：** 會有幾個葉節點 PTE？哪些分頁沒有 `U`？為什麼？`sz` 以下哪些虛擬分頁根本沒有 PTE？硬體會設定哪些旗標？為什麼 trampoline 的實體位址落在核心的 text 中？

在此觀察到的結果（原版核心，一個 hart）：

```
text   main    0x0000000000000000
data   marker  0x0000000000001000
stack  local   0x0000000000004F9C
heap   eager   0x0000000000005000  (1 page from sbrk)
heap   lazy    0x0000000000006000  (3 pages from sbrklazy, only the 2nd touched)
size   sbrk(0) 0x0000000000009000
page table 0x0000000087f25000
 ..0x0000000000000000: pte 0x0000000021fc8401 pa 0x0000000087f21000 V--------
 .. ..0x0000000000000000: pte 0x0000000021fc8001 pa 0x0000000087f20000 V--------
 .. .. ..0x0000000000000000: pte 0x0000000021fc885b pa 0x0000000087f22000 VR-XU-A--
 .. .. ..0x0000000000001000: pte 0x0000000021fc7c17 pa 0x0000000087f1f000 VRW-U----
 .. .. ..0x0000000000002000: pte 0x0000000021fc7817 pa 0x0000000087f1e000 VRW-U----
 .. .. ..0x0000000000003000: pte 0x0000000021fc7407 pa 0x0000000087f1d000 VRW------
 .. .. ..0x0000000000004000: pte 0x0000000021fc70d7 pa 0x0000000087f1c000 VRW-U-AD-
 .. .. ..0x0000000000005000: pte 0x0000000021fc98d7 pa 0x0000000087f26000 VRW-U-AD-
 .. .. ..0x0000000000007000: pte 0x0000000021fd50d7 pa 0x0000000087f54000 VRW-U-AD-
 ..0x0000003fc0000000: pte 0x0000000021fc9001 pa 0x0000000087f24000 V--------
 .. ..0x0000003fffe00000: pte 0x0000000021fc8c01 pa 0x0000000087f23000 V--------
 .. .. ..0x0000003fffffe000: pte 0x0000000021fd08c7 pa 0x0000000087f42000 VRW---AD-
 .. .. ..0x0000003ffffff000: pte 0x000000002000184b pa 0x0000000080006000 VR-X--A--
```

字母依序是 `V R W X U G A D`，最後的 `C` 是 RSW 第 8 位元，練習 C 會用到。逐行解釋：

- **0x0，text：`VR-XU`。** `exec` 依 ELF 旗標給每個區段 `X` 或 `W`（`flags2perm`），所以 text 永遠不可寫。
- **0x1000 與 0x2000，data 與 bss：`VRW-U`，沒有 `A`。** `marker` 是單獨一頁、對齊的資料。程式還沒存取過這兩頁。`exec` 是透過實體位址填入內容，不經過這張分頁表，所以沒有任何硬體走訪設定過 `A`。
- **0x3000，防護頁：沒有 `U`。** `exec` 配置 `USERSTACK + 1` 頁，並對最低的一頁呼叫 `uvmclear()`。堆疊溢位會在那裡觸發錯誤，而不會蓋到資料。
- **0x4000，堆疊：設了 `A` 與 `D`。** `kernel/start.c` 的 `start()` 設定了 `MENVCFG_ADUE`，所以在這個 QEMU 上，硬體會在存取時自己設定 `A`、在寫入時設定 `D`。沒有這個位元時，RISC-V 特權規格允許實作改為引發分頁錯誤，把這兩個位元交給軟體處理。
- **0x5000 與 0x7000，heap。** `sbrk(4096)` 立即映射了 0x5000。`sbrklazy(3 * 4096)` 只把 `sz` 提高到 0x9000；寫入 `lazy[4096]` 引發分頁錯誤，`vmfault()` 因此映射了 0x7000 這一頁。0x6000 與 0x8000 在 `sz` 以下，但沒有映射任何分頁。
- **0x3fffffe000 的 trap frame 與 0x3ffffff000 的 trampoline：沒有 `U`。** 只有監督者模式的程式碼使用它們。trampoline 的實體分頁 `0x80006000` 在核心映像檔中，因為 `trampoline.S` 連結在核心裡；同一個實體分頁映射到每個行程，也映射到核心自己的分頁表。
- **五個分頁表分頁：** 根分頁、兩個 L1 分頁與兩個 L0 分頁，位址空間的底部與頂部各一組。練習 B 會用到這個數字。

`vmprinttest fork` 改為印出 fork 出的子行程的分頁表。在原版核心中，子行程的分頁表形狀相同、實體位址不同：每一頁都是複本。

**刻意弄壞（未在此執行）：** 讓 `vmprint` 對每個有效 PTE 都遞迴，連葉節點也不例外。預測它對 text 分頁會印出什麼，以及核心能不能撐過去。

### B. 統計 `fork` 複製了什麼（xv6）

接著加上計數器。建立 `kernel/vmstat.h`，內容是 [`cowtest.c`](xv6/cowtest.c) 開頭列出的結構。統計 `uvmcopy()` 複製的每一頁（`fork_copied`），維護 `kalloc()` 可用清單上的分頁數，並新增系統呼叫 `vmstat(struct vmstat *)`，填好結構後用 `copyout` 複製出去。可能有好幾個 hart 同時執行 `uvmcopy()`，所以請用 `__atomic_fetch_add` 或在鎖的保護下計數。其他三個計數器在練習 C 之前都維持 0。把 `cowtest.c` 複製到 `user/`，並在 `UPROGS` 加上 `$U/_cowtest`。**預測** `cowtest` 第 1 部分：行程在 `sz` 以下有 71 頁。`fork` 會複製幾頁？一次 `fork` 會用掉幾個可用分頁？除了複本之外，`kfork()` 與 `allocproc()` 中還有哪些核心配置會用掉分頁？核心堆疊不算：`proc_mapstacks()` 在開機時就為每個行程槽配好了。

在此觀察到的結果（原版核心，搭配沒有放進專案的參考解答）：

```
  process size 71 pages
  the fork itself     : fork copied 71, fork shared 0, cow copied 0, cow reused 0
  free pages: 32465 before the fork, 32388 in the child just after it
  parent, after wait  : fork copied 71, fork shared 0, cow copied 0, cow reused 0
  free pages: 32465 before the fork, 32465 after the child exited: ok
2. fork a process that holds 2/3 of the free memory (21686 of 32529 pages)
  fork failed: a full copy needs more pages than are free
```

這次 fork 用掉 77 頁：71 個複本、子行程的五個分頁表分頁，以及它的 trap frame 分頁。第 2 部分失敗，因為 `uvmcopy()` 複製到一半就沒有分頁了，它釋放已複製的部分，`fork` 回傳 -1。把這個數字和實驗 02 的 `uvmcopy` 分頁數比較。

### C. 寫入時複製的 `fork`（xv6）

MIT 的 [cow 實驗](https://pdos.csail.mit.edu/6.1810/2025/labs/cow.html)說明了任務：`fork` 共用父行程的分頁而不複製，只有某一方寫入某一頁時才複製那一頁。依這個順序實作：

1. **標記分頁。** 使用特權規格留給監督者軟體的 RSW 位元：在 `kernel/riscv.h` 加上 `#define PTE_COW (1L << 8)`。
2. **計算擁有者數量。** 在 `kalloc.c` 中為每個實體分頁維護一個計數，以 `(pa - KERNBASE) / PGSIZE` 為索引，由 `kmem.lock` 保護。`kalloc()` 把它設為 1，新的 `krefinc()` 加一，`kfree()` 只在計數降到零時才真正釋放分頁。`kinit()` 會在所有計數仍為 0 時把每一頁釋放一次，所以 0 必須代表「可用」。
3. **共用而不複製。** 在 `uvmcopy()` 中，把父行程的實體分頁映射到子行程。如果這一頁原本可寫，就在兩個 PTE 中清掉 `PTE_W`、設定 `PTE_COW`。唯讀的分頁（例如 text）維持唯讀並共用。登記新的擁有者，然後用 `sfence_vma()` 清除 TLB，因為父行程的 PTE 改變了。
4. **攔下寫入錯誤。** 在 `usertrap()` 中，在 `vmfault()` 的分支之前，處理落在 `PTE_COW` 分頁上的 `scause` 15。`vmfault()` 對任何已映射的分頁都回傳 0，所以如果你的判斷放在它後面，行程會被終止。
5. **複製或收回。** 在 `cowfault(pagetable, va)` 中：如果計數是 1，其他共用者都已離開，只要設定 `PTE_W`、清掉 `PTE_COW`。否則配置一頁、複製內容、以可寫方式映射，並把舊分頁的參考減一。清除 TLB。
6. **核心自己的寫入。** `copyout()` 透過實體位址寫入使用者記憶體，所以硬體在這裡不會對 COW 分頁觸發錯誤。讓 `copyout()` 遇到 `PTE_COW` 時呼叫 `cowfault()`，但對單純唯讀的分頁照樣拒絕。

在你的寫入時複製程式碼中，也要在 `uvmcopy()` 統計 `fork_shared`，在 `cowfault()` 統計 `cow_copied` 與 `cow_reused`。下表的數字來自參考解答（沒有放進專案）。再次執行 `cowtest` 之前，先**預測**下表的每一列，最後再跑 `usertests -q`。

| `cowtest`，在此觀察 | 原版 `fork` | 寫入時複製的 `fork` |
| --- | ---: | ---: |
| `fork` 複製的分頁 | 71 | 0 |
| `fork` 共用的分頁 | 0 | 71 |
| 一次 `fork` 用掉的可用分頁（fork 前對比子行程剛建立時） | 77 | 7 |
| 子行程讀取 64 個 heap 分頁時的複製 | 0 | 0 |
| 子行程寫入 4 個 heap 分頁時的複製 | 0 | 4 |
| 第 1 部分全程的複製與收回 | 0 | 複製 5、收回 1 |
| 子行程結束後的可用分頁，與 fork 前比較 | 相同 | 相同 |
| fork 一個占用 2/3 可用記憶體的行程 | 失敗 | 成功 |
| 子行程對 COW 分頁呼叫 `pipe()`；父行程的內容不變 | ok | ok |
| 3 個 hart 下的 `usertests -q` | 未執行 | `ALL TESTS PASSED` |

解釋那些不是 0 或 71 的數字：

- **fork 用掉 7 頁：** 五個分頁表分頁、trap frame，再加一個複本。「fork itself」那一列已經包含這個複本。這些數字符合「子行程的第一次儲存」：`snap()` 的函式前言寫入它的堆疊分頁，而那一頁當時是共用的。請自己確認：在 `cowfault` 設中斷點並印出 `va`。
- **複製 5、收回 1：** 子行程的堆疊分頁與 4 個 heap 分頁被複製。子行程結束後，父行程的堆疊分頁只剩一個擁有者，所以父行程下一次寫入它（`wait()` 對結束狀態的 `copyout`）時，只是拿回 `PTE_W`。

在寫入時複製版本的核心上，`vmprinttest fork` 印出子行程在 fork 之後的分頁表：

```
 .. .. ..0x0000000000000000: pte 0x000000002174a85b pa 0x0000000085d2a000 VR-XU-A--
 .. .. ..0x0000000000001000: pte 0x00000000218cb513 pa 0x000000008632d000 VR--U---C
 .. .. ..0x0000000000002000: pte 0x000000002194b913 pa 0x000000008652e000 VR--U---C
 .. .. ..0x0000000000003000: pte 0x00000000219cbd03 pa 0x000000008672f000 VR------C
 .. .. ..0x0000000000004000: pte 0x00000000213c8cd7 pa 0x0000000084f23000 VRW-U-AD-
 .. .. ..0x0000000000005000: pte 0x00000000215499d3 pa 0x0000000085526000 VR--U-ADC
 .. .. ..0x0000000000007000: pte 0x00000000214c95d3 pa 0x0000000085325000 VR--U-ADC
```

data、防護頁與 heap 分頁都是唯讀並設了 `C`。text 沒有 `C`，因為它從來不可寫。堆疊分頁已經是子行程自己的可寫複本：子行程在進入 `vmprint()` 的途中寫過自己的堆疊。

**刻意弄壞（已在此執行）：** 刪掉 `uvmcopy()` 中增加參考計數的那一行。如果你的修補檔裡有參考解答的那一行 `    krefinc((void *)pa);`，`BREAKIT=1` 會替你刪掉；否則它會說明原因並略過。先**預測**。觀察到的結果：計數顯示的是「cow reused」而不是「cow copied」，所以子行程直接寫進父行程仍在映射的分頁。子行程結束時，這些分頁回到可用清單並被填滿垃圾。接著共用它們的行程一個個死去：`cowtest` 本身（pid 3）在 `sepc=0x1000` 以 `scause 0x2`（非法指令）結束，shell（pid 2）以 `scause 0x2` 結束，init（pid 1）以 `scause 0xc`（指令分頁錯誤）結束，核心停在 `panic: init exiting`。

**刻意弄壞（未在此執行）：**
- 拿掉 `copyout()` 的修改。預測 `cowtest` 第 3 部分會印出什麼。
- 拿掉兩個 `sfence_vma()` 呼叫。預測 xv6 上會不會壞掉。提示：讀 `trampoline.S` 中的 `userret`，每次返回使用者空間前都會執行它。再說明：如果核心不在那裡清除 TLB，為什麼就需要這兩個呼叫。

### D. Linux 上的共享與私有映射（已在此執行）

```sh
cd linux
gcc -O2 -Wall -o mapshare mapshare.c
./mapshare
```

父行程（A）與子行程（B）在 fork 之後各自開啟並 `mmap` 這個檔案，就像兩個不相關的行程那樣。執行前先**預測**每一個「map」與「file」欄位。第 2 回合中，B 以 `MAP_PRIVATE` 映射檔案。A 寫入第 0 頁後，B 會看到什麼？B 寫入第 0 頁後呢？A 寫入 B 從未寫過的第 1 頁後呢？

在此觀察到的結果（欄位依序是：這個行程透過映射看到的第 0 頁與第 1 頁，以及 `pread` 讀到的檔案內容）：

```
Round 1: A and B both map mapshare.dat with MAP_SHARED
  A /proc/self/maps: 7b04543c7000-7b04543c9000 rw-s 00000000 08:30 63677   /tmp/lab03-linux/mapshare.dat
  B after B writes page 1   map: [A wrote page 0  ] [B wrote page 1  ]   file: [A wrote page 0  ] [B wrote page 1  ]
  A after B writes page 1   map: [A wrote page 0  ] [B wrote page 1  ]   file: [A wrote page 0  ] [B wrote page 1  ]

Round 2: A maps with MAP_SHARED, B with MAP_PRIVATE
  B /proc/self/maps: 7b04543c7000-7b04543c9000 rw-p 00000000 08:30 63677   /tmp/lab03-linux/mapshare.dat
  B after A writes page 0   map: [A wrote page 0  ] [page 1: original]   file: [A wrote page 0  ] [page 1: original]
  B after B writes page 0   map: [B private write ] [page 1: original]   file: [A wrote page 0  ] [page 1: original]
  A after B's private write map: [A wrote page 0  ] [page 1: original]   file: [A wrote page 0  ] [page 1: original]
  B after A writes both     map: [B private write ] [A wrote page 1  ]   file: [A wrote 0 again ] [A wrote page 1  ]
```

（已縮短；完整的行在輸出紀錄中。）`MAP_SHARED` 的分頁就是檔案在 page cache 中的分頁，所以兩個映射與 `read(2)` 都會立刻看到每一次寫入，不需要 `msync`。私有映射一開始也映射同樣的分頁。B 第一次寫入第 0 頁時得到自己的複本，所以 B 再也看不到 A 之後對第 0 頁的寫入；而 B 從未寫過的第 1 頁，仍然看得到 A 的寫入。[mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) 說明，`MAP_PRIVATE` 映射能否看到 `mmap()` 之後對檔案的修改並未規定，所以這是 Linux 在這個核心上的行為，不是保證。在 `/proc/self/maps` 中，權限欄位的第四個字母 `s` 代表共享，`p` 代表私有（[proc_pid_maps(5)](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html)）。

接著執行 `./forkcow`。在此觀察到的結果：

```
1. fork with 16384 written pages (64 MiB) in the parent
  child: minor faults 10 at start, +0 after reading 16384 pages, +1000 after writing 1000 pages
2. fork + _exit + wait, by size of the parent's written memory
     1 MiB: best of 20 runs    167.7 us
    64 MiB: best of 20 runs   1643.6 us
   256 MiB: best of 20 runs   5827.9 us
```

讀取共用的分頁不會產生錯誤。每次第一次寫入產生一次次要分頁錯誤，也就是那次複製，和你的 xv6 版本完全一樣。父行程越大，`fork` 仍然越慢，因為 [fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html) 說明它的成本是複製父行程分頁表的時間與記憶體，以及為子行程建立 task 結構。這些是虛擬機器中的數字，只能彼此比較。

### E. Linux 上在映射存在時截短檔案（已在此執行）

```sh
gcc -O2 -Wall -o sigbus sigbus.c
./sigbus
```

檔案從 3 頁縮成 1 頁加 100 位元組之後，**預測**第 1 部分的每一次讀取。在此觀察到的結果：

```
  read page 0, offset 0             -> 0x61
  read page 1, offset 50            -> 0x62
  read page 1, offset 200 (past EOF) -> 0x00  (zero)
  read page 2, offset 0             -> SIGBUS, si_code 2 (BUS_ADRERR), si_addr = map + 0x2000
```

[mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html) 列出：存取緩衝區中位於映射檔案結尾之後的分頁會收到 `SIGBUS`。它也說明最後一個不完整分頁的其餘部分會被填零。所以新檔案結尾之後、但仍在最後一頁內的位元組讀出 0，而結尾之後第一個完整的分頁收到訊號。

第 2 部分是檢核要問的情況。伺服器映射一個從用戶端拿到的檔案並讀取它，讀到一半時，用戶端把檔案截短：

```
  a) server without a SIGBUS handler
  client: truncated the shared file to one page
  server was killed by signal 7 (Bus error)
  b) server with a SIGBUS handler
  client: truncated the shared file to one page
  server: SIGBUS at buffer + 0x1000; rejecting this client's buffer and carrying on
```

第 3 部分展示階段 08 會建立的修正：以 `F_SEAL_SHRINK` 封存的 `memfd`，會以 `EPERM` 拒絕 `ftruncate`（[memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html)，以及 [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html) 的 "File Sealing"）。

### F. 手算位址轉換，再到 GDB 中確認（已在此執行）

拿 `vmprinttest` 印出的堆疊變數 `local`（位於 `0x4F9C`），只用上面的 `vmprint` 輸出在紙上轉換：

1. 拆開位址。L2 索引 = `(va >> 30) & 0x1ff` = 0，L1 索引 = `(va >> 21) & 0x1ff` = 0，L0 索引 = `(va >> 12) & 0x1ff` = 4，位移 = `va & 0xfff` = `0xF9C`。
2. 根分頁（位於 `0x87f25000`）的第 0 項是 `0x21fc8401`。`R`、`W`、`X` 都沒設，所以它指向下一層分頁表：實體分頁號碼 `0x21fc8401 >> 10` = `0x87f21`，L1 分頁位於 `0x87f21000`。
3. 那裡的第 0 項是 `0x21fc8001`，所以 L0 分頁位於 `0x87f20000`。
4. 那裡的第 4 項是 `0x21fc70d7`。旗標是 `0xd7`（`V R W U A D`），所以它是使用者可寫的葉節點。實體分頁 `0x87f1c`，實體位址 `0x87f1cf9c`。

現在對你自己選的位址做同樣的事，其中一個要在上半部，再到 GDB 中確認。[`translate.gdb`](xv6/translate.gdb) 停在 `sys_vmprint`，從 `cpus[0].proc->pagetable` 讀出行程的根分頁，並以實體位址走訪分頁表。它對 `marker` 做一次，最後讀到的位元組必須是 marker 字串；也對 `TRAPFRAME` 做一次，結果必須等於核心自己的指標 `p->trapframe`。在此觀察到的結果：

```
process vmprinttest, pid 3
va of marker       0x0000000000001000
root page table    0x0000000087f25000  (its satp value: 0x8000000000087f25)
satp right now     0x8000000000087fff  (the kernel's page table)
indexes            L2 0, L1 0, L0 1, offset 0x0
L2 PTE at 0x87f25000  0x0000000021fc8401  flags 0x001
L1 PTE at 0x87f21000  0x0000000021fc8001  flags 0x001
L0 PTE at 0x87f20008  0x0000000021fc7c17  flags 0x017
physical address   0x0000000087f1f000
string at that physical address:
0x87f1f000:	"lab03 marker: translate my address by hand"

va of TRAPFRAME    0x0000003fffffe000  indexes L2 255, L1 511, L0 510
PTEs               0x0000000021fc9001  0x0000000021fc8c01  0x0000000021fd08c7
leaf flags 0x0c7: U is clear
physical address   0x0000000087f42000
p->trapframe       0x0000000087f42000
```

GDB 讀得到這些實體位址，是因為中斷點在核心中，而 `kvmmake()` 在核心的分頁表中把整個 RAM 映射到與實體位址相同的虛擬位址。若像硬體那樣透過使用者的分頁表存取，GDB 連第一個分頁表分頁都讀不到。

### G. Windows 上的保留、提交與 section（已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o build\nt_reserve_commit.exe nt_reserve_commit.c -lpsapi
build\nt_reserve_commit.exe
gcc -O2 -Wall -municode -o build\nt_section.exe nt_section.c -lpsapi
build\nt_section.exe
```

先**預測**第 1 部分每一步的每一欄，再執行。2026-11-16 的觀察結果（Windows 組建 26300、Intel Core i7-11700、64 GiB RAM、有 hypervisor；[完整輸出](windows/example-output.txt)）：

| 步驟 | 行程的提交量 | 系統提交總量 | 工作集 | 分頁錯誤 | `VirtualQuery` |
| --- | ---: | ---: | ---: | ---: | --- |
| 保留 1 GiB | +0.0 MiB | +0.0 MiB | +0.1 MiB | +14 | `MEM_RESERVE` |
| 提交前 256 MiB | +256.5 MiB | +256.6 MiB | +0.1 MiB | +147 | `MEM_COMMIT` |
| 每頁寫入一個位元組 | +256.5 MiB | +258.1 MiB | +256.1 MiB | +65,683 | `MEM_COMMIT` |
| 取消提交這 256 MiB | +0.0 MiB | +1.6 MiB | +0.1 MiB | +65,684 | `MEM_RESERVE` |
| 釋放整個區域 | +0.0 MiB | +1.6 MiB | +0.1 MiB | +65,684 | `MEM_FREE` |

- **保留只花位址空間。** [VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc) 說明 `MEM_RESERVE` 保留一段範圍，"without allocating any actual physical storage in memory or in the paging file on disk"。
- **提交會計費，存取才配置。** `MEM_COMMIT` 從記憶體與分頁檔的總量中扣除費用（charge），Learn 說明實體分頁要等位址真正被存取時才配置。在這裡，提交的那一刻提交量就跳上去，工作集與分頁錯誤次數則要等每一頁被寫入時才增加：65,536 頁產生 65,536 次錯誤。`PrivateUsage` 就是行程的提交量（[PROCESS_MEMORY_COUNTERS_EX](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex)）。系統總量也會隨其他行程變動。
- **按需提交。** 第 2 部分保留 64 MiB，寫入 16 頁，並在向量式例外處理常式中逐頁提交，做法和 Learn 的 [Reserving and Committing Memory](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory) 範例以例外篩選器提交相同。這和 xv6 的 `sbrklazy` 與 `vmfault()` 是同一個概念。觀察到的結果：每次都提交了 16 頁，但 16 頁彼此相隔 4 MiB 時，提交量增加 31 或 33 頁；緊鄰時增加 16 或 18 頁。Learn 沒有說明還計入了什麼。**練習：** 提出一個解釋（分頁表分頁是候選之一，因為每隔 4 MiB 就需要一個新的），並設計測試。
- **第 3 部分**依配置單位列出位址空間，是 `/proc/self/maps` 在 NT 上的對應：`MEM_IMAGE` 是 exe 與 DLL，`MEM_MAPPED` 是 section（語系檔與沒有名稱的 section），`MEM_PRIVATE` 是 heap 與堆疊。

`nt_section.exe` 的觀察結果：

```
  A after A writes page 0          view 00000213F02B0000: [A wrote page 0      ] [                    ]
  B after opening by name          view 000001BB35820000: [A wrote page 0      ] [                    ]
  B after B writes page 1          view 000001BB35820000: [A wrote page 0      ] [B wrote page 1      ]
  B copy view, after writing 0     view 000001BB355E0000: [B private write     ] [B wrote page 1      ]
  B shared view, same moment       view 000001BB35820000: [A wrote page 0      ] [B wrote page 1      ]
  B commit charge: +0.012 MiB for mapping a 3-page copy view
  B copy view page 0: PAGE_READWRITE  resident, private
  B copy view page 1: PAGE_WRITECOPY  resident, shared
  A after B exited                 view 00000213F02B0000: [A wrote page 0      ] [B wrote page 1      ]
  SEC_COMMIT  create  +256.6 MiB, map a view    +0.5 MiB, (nothing more)           +0.0 MiB
  SEC_RESERVE create    +0.0 MiB, map a view    +0.5 MiB, VirtualAlloc 16 MiB     +16.0 MiB
  with a view mapped (same handle):            SetEndOfFile -> FALSE, error 1224 (The requested operation cannot be performed on a file with a user-mapped section open.)
  with a view mapped (second handle):          SetEndOfFile -> FALSE, error 1224
  view unmapped, section handle still open:    SetEndOfFile -> FALSE, error 1224
  view unmapped and section closed:            SetEndOfFile -> TRUE, file size now 4096
```

- **一個 section、兩個行程、兩個位址。** 以 `INVALID_HANDLE_VALUE` 呼叫 [CreateFileMapping](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga)，會建立一個 "backed by the system paging file" 的物件；第二個行程可以透過名稱、繼承的 handle 或 `DuplicateHandle`（階段 08）開啟它。兩個行程把它映射在不同的位址，所以共享緩衝區裡必須存位移，而不是指標。
- **`FILE_MAP_COPY` 是 NT 的私有映射。** [MapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile) 說明，寫入寫入時複製的分頁時，系統會把它複製到一個行程私有的新分頁，保護屬性由寫入時複製變為讀寫。`VirtualQuery` 與 `QueryWorkingSetEx` 對第 0 頁顯示的正是如此。同一頁也說明提交量是以整個 view 計算，因為每一頁都可能變成私有。那就是在任何寫入之前就計入的 +0.012 MiB，也就是三頁。
- **`SEC_COMMIT` 在建立時計費，`SEC_RESERVE` 在 `VirtualAlloc` 時計費。** 兩者都在 `CreateFileMapping` 頁面中說明，預設是 `SEC_COMMIT`。
- **NT 拒絕截短已映射的檔案。** [SetEndOfFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setendoffile) 說明必須先解除所有 view 的映射並關閉映射物件。在這裡，透過第二個 handle 呼叫也以錯誤 1224 失敗；即使 view 已解除映射，只要 section 的 handle 還開著，也一樣失敗。把它和練習 E 的 Linux 結果比較。

## Linux／NT 對照

- **保留與提交。** NT 把兩者分開（[Learn](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory)），提交量在提交時計算，而不是在存取時。Linux 沒有獨立的提交呼叫：匿名記憶體的 `mmap` 保留位址空間，分頁在第一次存取時才出現，而 `mmap` 能否承諾超過實際存在的記憶體，是系統政策（[proc_sys_vm(5)](https://man7.org/linux/man-pages/man5/proc_sys_vm.5.html) 的 `overcommit_memory`）。xv6 有 `sbrk()`（立即）與 `sbrklazy()`（存取時），完全沒有計費。
- **section 與映射。** Learn 說明檔案映射物件又稱為 section object（[Learn](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)）。以分頁檔為後盾的 section 就是 NT 的匿名共享記憶體。在 Linux 上最接近的是 `memfd`，或跨 `fork` 的 `MAP_SHARED | MAP_ANONYMOUS`。
- **三個核心中的寫入時複製。** 你的 xv6 `fork`、Linux 的 `fork` 與 `MAP_PRIVATE`，以及 NT 的 `FILE_MAP_COPY`，都是以唯讀方式共用分頁，在第一次寫入錯誤時才複製。Win32 沒有 `fork`，所以在 NT 上你會在這類 view 中遇到寫入時複製。
- **被截短的已映射檔案。** Linux 允許檔案縮小，並對存取消失分頁的程式送出 `SIGBUS`。NT 在該檔案還有任何 section 時拒絕 `SetEndOfFile`（錯誤 1224）。不過 NT 對已映射的資料檔也有自己的失敗情況：[Reading and Writing From a File View](https://learn.microsoft.com/en-us/windows/win32/memory/reading-and-writing-from-a-file-view) 說明存取可能引發 `EXCEPTION_IN_PAGE_ERROR`，例如遠端檔案的伺服器斷線或磁碟已滿，並要求以結構化例外處理包住每一次存取。

## 檢核

在 `NOTES.md` 中回答。

1. **手算一個虛擬位址的轉換，再到 GDB 中確認。** 從你自己的 `vmprint` 輸出選一個不在第一頁的位址，最好是上半部的位址。寫出三個索引與位移、每個 PTE 及其旗標、哪些 PTE 指向分頁表、哪一個是葉節點，以及實體位址。再用 `translate.gdb`，或手動輸入同樣的指令確認。說明 GDB 為什麼能在那個中斷點以實體位址讀取分頁表分頁。
2. **展示 COW 省下的分頁。** 列出兩種核心下的 `cowtest` 數字，並解釋可用分頁的算式（這裡是 77 與 7）。展示占用 2/3 記憶體的 fork 先失敗、後成功。貼上你的 `usertests -q` 結果。
3. **解釋為什麼 `SIGBUS` 的情況是安全問題**：任何伺服器只要映射了用戶端交給它的記憶體就會遇到。用戶端能控制什麼？沒有處理常式時，伺服器和它的其他用戶端會怎樣？提出三種防禦：禁止縮小的封存、能復原的處理常式，以及複製而不映射，並說明各自的代價。NT 的錯誤 1224 讓情況有什麼不同？`EXCEPTION_IN_PAGE_ERROR` 又留下了什麼缺口？階段 17 會回到這一點。
4. 使用者的寫入會被分頁錯誤攔下，為什麼 `copyout()` 還必須自己處理寫入時複製的分頁？
5. Linux 的 `fork` 根本不複製資料，為什麼父行程越大就越慢？
6. 在 NT 上，提交量量的是什麼？為什麼寫入分頁改變了工作集，卻沒有改變提交量？

## 常見失敗情況

- **`run-xv6.sh` 中的 `patch` 回報 hunk 失敗。** 你的修補檔是針對別的 commit 或 MIT 的實驗分支做的。在你產生 diff 的 clone 中用 `git log -1` 確認。
- **`run-xv6.sh` 印出 `SKIPPED: ... needs ... in SOLUTION_DIR`。** 你的修補檔不在腳本找的位置，或檔名不是它預期的名稱。
- **`fork` 之後的第一次寫入出現 `usertrap(): unexpected scause 0xf`。** `usertrap()` 缺少 COW 的分支，或把它放在 `vmfault()` 的分支之後；`vmfault()` 對已映射的分頁回傳 0。
- **`panic: kfree` 或 `panic: freewalk: leaf`。** 參考計數對不上：`kalloc()` 沒有設定計數、共用分頁被釋放了兩次，或 `uvmunmap` 釋放了另一張分頁表仍在映射的分頁。
- **子行程結束後，其他行程隨機當掉。** 共用分頁在還有人映射時就被釋放了。這就是上面刻意弄壞的那次執行。
- **在 COW 下，子行程中的 `pipe()` 或 `read()` 失敗。** `copyout()` 仍然拒絕所有沒有 `PTE_W` 的分頁。
- **`fork` 成功，但父行程看到了子行程的寫入。** 父行程的 PTE 保留了 `PTE_W`。兩個 PTE 都必須失去寫入權限。
- **在核心中斷點讀使用者位址時，GDB 印出 `Cannot access memory`。** 核心的分頁表沒有映射使用者位址。改讀實體位址，核心的分頁表有映射它。
- **GDB 腳本把核心弄壞了。** `set $a0 = ...` 或 `set $t1 = ...` 會寫入 CPU 暫存器。改用不是暫存器名稱的變數名。
- **`expect` 逾時。** 某個指令印出了腳本沒有等待的內容，或核心 panic 了。讀輸出的最後幾行。
- **Linux：沒有收到 `SIGBUS`。** 讀取仍在最後一個不完整的分頁內，那裡讀出 0。只有新檔案結尾之後的完整分頁才會出錯。
- **Windows：`undefined reference to WinMain`。** `nt_section.c` 使用 `wmain`，需要 `-municode`。
- **Windows：系統提交量那一欄亂跳。** 其他行程也在配置記憶體。比較行程那一欄，並執行兩次。

## 後續會用到的地方

- 階段 04 看另一種把 CPU 從使用者程式碼拿走的東西：中斷。`usertrap()` 是同一個入口。
- 階段 07 在 xv6 中實作檔案映射 `mmap`，並追問在每個系統上，由誰把被寫過的映射分頁寫回磁碟。
- 階段 08 封存 `memfd`，讓對方無法縮小它，並用 `DuplicateHandle` 把 NT 的 section 交給另一個行程。
- 階段 09 與 17 會讓伺服器面對一個縮小自己緩衝區的用戶端：也就是練習 E 的第 2 部分。

## 下一步

[實驗 04](../04-interrupts-drivers/README.zh-TW.md) 會在 xv6 中追蹤一次裝置中斷與計時器中斷、撰寫一個小型 Linux 字元裝置模組，並在 Windows 上觀察中斷與 DPC。
