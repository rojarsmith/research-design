[English](README.md) | **繁體中文** | [實作建置計畫，階段 00](../../SYLLABUS.zh-TW.md)

# 實驗 00：工作環境，開機並除錯 xv6

> **草稿，待審閱（2026-11-15），2026-11-16 實際執行。哪些有執行、哪些沒有：**
> - **2026-11-16 已執行：環境設定步驟 3 與 5、練習 A–G，以及兩個「刻意弄壞」步驟。** 執行環境是 Windows 11 上 podman 中的 Ubuntu 24.04 容器，podman machine 跑在 WSL 2 上；這不是環境設定步驟 1 安裝的 WSL 2 Ubuntu 發行版。容器內有 QEMU 8.2.2、riscv64-linux-gnu-gcc 13.3.0、binutils 2.42 與 gdb-multiarch 15.1，xv6-riscv master 的 commit 為 `fa2ddcb`（2026-11-05）。GDB 以批次腳本驅動，指令與下文相同。輸出在 [`xv6/example-output.txt`](xv6/example-output.txt)，下文的觀察值都取自該檔。
> - **未執行：環境設定步驟 1、2、4**（安裝 WSL 2 與 Ubuntu、`apt-get`、`git clone`），因為容器已有這些套件與 xv6 複本。WSL 1、`/mnt/c`、CRLF 換行與缺少 `bc` 這幾種失敗情況都沒有重現。
> - 本實驗是在一台沒有 WSL、RISC-V 工具鏈與 QEMU 的 Windows 11 機器上撰寫的，依據是 MIT 6.1810 的[工具頁](https://pdos.csail.mit.edu/6.1810/2025/tools.html)、[xv6-riscv](https://github.com/mit-pdos/xv6-riscv) 原始碼與 QEMU 原始碼，皆於 2026-11-15 閱讀。你的工具版本與 commit 可能不同，請記下你實際看到的內容。

## 目標

建立能用的 xv6 工作環境，並用它精確回答一個問題：從 QEMU 重置到 xv6 的 `main()` 之間執行了什麼，各在哪個特權模式？完成後，你能開機、離開、重新建置 xv6，接上 GDB，並在重置向量到 shell 提示字元之間的任何一道指令停下。

```
QEMU 重置 ROM    ->  _entry        ->  start()      --mret-->  main()       ->  scheduler()  ->  /init、sh
0x1000，M 模式       0x80000000        start.c                 main.c           proc.c           U 模式
                     entry.S，M 模式   M 模式                  S 模式           S 模式
```

## 先備條件

會用指標與結構的 C、Linux shell、git 與 make，以及讀懂 RISC-V 組合語言，程度相當於 [`c-compiler-foundations` 學習路線](../../../c-compiler-foundations/research/04-integration-path.zh-TW.md)第 1 關：讀得懂 `-O0` 的 prologue、找得到 `ra`、說得出區域變數為何在 `fp` 的負偏移。xv6 是 RV64（`-march=rv64gc`），暫存器與指標都是 64 位元；你也會遇到那條路線沒有涵蓋的 CSR 與特權模式。

## 環境設定（本草稿未執行步驟 1、2、4）

以下依照 MIT 2025 年的工具頁。2026-11-15 當天它仍是最新版本：2026 秋季課程首頁連到的 `tools.html` 當天回傳 404。

1. **WSL 2 搭配 Ubuntu 24.04。** MIT 給 Windows 的做法是 WSL 2 加 Ubuntu 24.04，並說明 WSL 1 無法使用。在系統管理員 PowerShell 中先用 `wsl --list --online` 列出發行版，再用 `wsl --install -d <清單中的名稱>` 安裝 24.04，然後確認 `wsl -l -v` 顯示它的 `VERSION` 為 2（[Install WSL](https://learn.microsoft.com/en-us/windows/wsl/install)）。
2. **套件**：在 Ubuntu 內，完全照 MIT 的清單安裝：

   ```sh
   sudo apt-get update && sudo apt-get upgrade
   sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
   ```

3. **版本。** MIT 要求 QEMU 7.2 以上、GDB 8.3 以上。2026-11-15 時 packages.ubuntu.com 列出 Ubuntu 24.04（noble）的版本為：`qemu-system-misc` 8.2.2、`gdb-multiarch` 15.0.50、`gcc-riscv64-linux-gnu` 13.2.0、`binutils-riscv64-linux-gnu` 2.42。記下你實際裝到的版本：

   ```sh
   qemu-system-riscv64 --version
   riscv64-linux-gnu-gcc --version
   riscv64-linux-gnu-ld --version
   gdb-multiarch --version
   ```

   2026-11-16 在容器中觀察到：QEMU 8.2.2、gcc 13.3.0、GNU ld 2.42、GDB 15.1。編譯器與 GDB 比上面的清單新，因為它們來自 Ubuntu 的更新（`gdb-multiarch 15.1-1ubuntu1~24.04.1`）。`gcc-riscv64-linux-gnu` 套件本身仍是 13.2.0，它會一併安裝 `gcc-13-riscv64-linux-gnu` 13.3.0。

4. **原始碼**：複製到 Linux 檔案系統內（家目錄），不要放在 `/mnt/c` 底下：

   ```sh
   cd ~
   git clone https://github.com/mit-pdos/xv6-riscv.git
   cd xv6-riscv
   git log -1 --format='%H %cd'
   ```

   記下這個 commit。從階段 01 起還會用到 MIT 的實驗儲存庫 `git clone git://g.csail.mit.edu/xv6-labs-2025`，每個實驗一個分支。
5. **讓 GDB 讀取 xv6 的 `.gdbinit`。** 除非你允許，GDB 不會執行目前目錄下的 `.gdbinit`。MIT 的說明是把警告訊息建議的那一行加進 `~/.gdbinit`。以這份複本來說是：

   ```sh
   echo "add-auto-load-safe-path $HOME/xv6-riscv/.gdbinit" >> ~/.gdbinit
   ```

   GDB 15.1 的警告訊息指名要修改的是 `~/.config/gdb/gdbinit`。寫在 `~/.gdbinit` 也可以：本實驗的執行把這一行放在那裡，之後 GDB 都會讀取 `.gdbinit`。

現在就建立 `NOTES.md`，記下 Windows 組建編號、WSL 版本、發行版、上面所有版本，以及 xv6 的 commit。

## 先讀

這些檔案都很短。

- `Makefile`：`QEMUOPTS`、`CPUS := 3`、`qemu` 與 `qemu-gdb` 目標、由使用者 id 算出的 `GDBPORT`，以及從 `.gdbinit.tmpl-riscv` 產生 `.gdbinit` 的規則。
- `kernel/kernel.ld`：核心連結在 `0x80000000`，`kernel/entry.o(_entry)` 排第一，trampoline 獨占一頁。
- `kernel/entry.S`：每個 hart 計算 `sp = stack0 + (hartid + 1) * 4096`，再呼叫 `start`。
- `kernel/start.c`：把 `mstatus` 的前一特權設為監督者、`mepc` 設為 `main`，關閉分頁，把陷阱委派給監督者模式，開放實體記憶體保護（PMP），讓硬體設定分頁表的 A 與 D 位元（`menvcfg`），要求計時器中斷，把 hart id 存進 `tp`，最後執行 `mret`。
- `kernel/main.c`：hart 0 負責全部初始化，其他 hart 等待 `started`。
- [xv6 課本](https://pdos.csail.mit.edu/6.1810/2025/xv6/book-riscv-rev5.pdf)第 2 章中啟動 xv6 的一節。

## 練習

每個練習開始前，先把預測寫進 `NOTES.md`；執行後，把結果寫在預測旁邊。能解釋的錯誤預測，比猜對更有價值。

### A. 開機、觀察、離開

**預測：** 會出現幾行 `hart N starting`，順序如何？按 Ctrl-p 會列出哪些行程名稱？

```sh
make qemu
```

出現 `$` 提示字元後輸入 `ls`，再按 Ctrl-p。核心的 `procdump()` 會每個行程印一行，依序是 pid、狀態與名稱。按 Ctrl-a 再按 x 離開 QEMU。

再用 `make CPUS=1 qemu` 開機一次，檢查你對單一 hart 的預測。

<details>
<summary>2026-11-16 的觀察結果。請在預測並執行之後再看。</summary>

- 預設 `CPUS := 3`：先是 `xv6 kernel is booting`，接著 `hart 2 starting` 與 `hart 1 starting`，然後是 `init: starting sh` 與 `$` 提示字元。同一份建置再開機一次，先印出的是 `hart 1`。在 `kernel/main.c` 中，hart 0 印出開頭訊息並負責全部初始化，其他 hart 等 `started` 設定後才各印一行，誰先到誰先印。
- `ls` 之後按 Ctrl-p，印出 `1 sleep  init` 與 `2 sleep  sh`；`ls` 已經結束了。
- Ctrl-a x 印出 `QEMU: Terminated`。
- `CPUS=1` 時沒有任何 `hart N starting`。

</details>

### B. 每個 QEMU 參數

`make -n qemu` 只印出指令、不執行。觀察到的輸出：先是 `check-qemu-version` 規則的 shell 程式碼，其中 `bc` 的結果已經代入（`if [ "1" -eq 0 ]; then ...`），接著是寫在同一行的 QEMU 指令。以下拆成幾行方便閱讀：

```
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic
  -global virtio-mmio.force-legacy=false
  -drive file=fs.img,if=none,format=raw,id=x0
  -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
```

在 `NOTES.md` 中為每個參數寫一行說明它的作用，再指出 xv6 原始碼中依賴它的那一行。可以從這幾個開始：

- `-machine virt` 決定實體記憶體配置。把 QEMU `virt` 的配置和 `kernel/memlayout.h` 的 `UART0`、`VIRTIO0`、`PLIC`、`KERNBASE` 對照。
- `-bios none` 表示不執行任何韌體，所以 xv6 自己從機器模式開始。練習 C 會看到 QEMU 仍會先執行的那幾道指令。
- `-m 128M` 對應 `PHYSTOP = KERNBASE + 128 * 1024 * 1024`。
- `-smp 3` 不能超過 `kernel/param.h` 的 `NCPU`；在本實驗閱讀的版本中它是 8。`start.c` 的 `stack0` 有 `NCPU` 個堆疊。

**刻意弄壞：** 把 `QEMUOPTS` 中的 `-m 128M` 改成 `-m 64M`。先預測核心會在哪裡、以什麼方式失敗。提示：`kinit()` 把 `PHYSTOP` 以下的每一頁交給 `freerange()`，而 `kfree()` 會用垃圾值填滿每一頁。執行並記錄結果，最後改回來。

<details>
<summary>使用 <code>-m 64M</code> 的觀察結果。請在預測並執行之後再看。</summary>

主控台印出 `xv6 kernel is booting` 之後就沒有任何輸出，也沒有 panic 訊息。要找出原因，用 `make CPUS=1 qemu-gdb` 執行並設 `b *0x0`：這時 `stvec` 仍是重置值 0，因為 `trapinithart()` 要稍後才執行。停下時處於監督者模式，`scause` 為 7（store/AMO access fault），`sepc` 在 `memset` 中，`stval` 為 `0x84000000`，也就是 64 MiB RAM 之後的第一個位元組。GDB 也讀不到位址 0 的記憶體，所以陷阱處理位址指向空無一物之處，核心根本沒有機會印出訊息。

</details>

### C. 第一道指令不是 xv6 的

在一個終端機讓 QEMU 停在重置狀態，在第二個終端機、同一個目錄下接上 GDB。只用一個 hart，讓中斷點只觸發一次：

```sh
make CPUS=1 qemu-gdb      # 終端機 1：會印出 "*** Now run 'gdb' in another window."
gdb-multiarch             # 終端機 2，位於 ~/xv6-riscv，會讀取 .gdbinit
```

**預測：** GDB 接上時，`$pc` 是多少？

```
(gdb) p/x $pc
(gdb) x/6i $pc
(gdb) p $priv
```

用 `si` 單步執行，直到 `$pc` 變成 `0x80000000`，途中印出 `$a0`、`$a1` 與 `$a2`。`x/2gx 0x1018` 會顯示 ROM 載入的兩個字組。

QEMU 的 gdb stub 把目前的特權等級提供為虛擬暫存器 `priv`（`target/riscv/gdbstub.c`）：3 是機器模式、1 是監督者、0 是使用者。如果你的 GDB 看不到 `$priv`，改用 `p/x $mstatus` 與練習 E 的推理。

<details>
<summary>QEMU 原始碼怎麼說，以及實際執行的結果。請在預測並執行之後再看。</summary>

依 QEMU 原始碼：`hw/riscv/virt.c` 在 `0x1000` 配置一小塊 ROM，`hw/riscv/boot.c` 的 `riscv_setup_rom_reset_vec()` 在其中寫入六道指令。它們把 hart id 放進 `a0`、裝置樹位址放進 `a1`，再跳到 `0x1018` 存放的位址；使用 `-bios none` 時，那個位址就是 RAM 的起點。

觀察結果（QEMU 8.2.2、GDB 15.1）：`$pc` 是 `0x1000`，`x/6i $pc` 印出 `auipc t0,0x0`、`addi a2,t0,40`、`csrr a0,mhartid`、`ld a1,32(t0)`、`ld t0,24(t0)`、`jr t0`。`$priv` 是 3。`x/2gx 0x1018` 印出 `0x0000000080000000` 與 `0x0000000087e00000`。第六次 `si` 之後，`$pc` 是 `0x80000000`（`_entry () at kernel/entry.S:12`），`a0` 為 0、`a1` 為 `0x87e00000`、`a2` 為 `0x1028`。

</details>

### D. 從 `_entry` 到 `start()`

```
(gdb) x/8i 0x80000000
(gdb) si          （重複執行，觀察 $sp、$a0、$a1）
(gdb) p/x $sp
(gdb) p &stack0
```

**預測：** hart 0 在 `call start` 之前的 `sp` 值，並與 `&stack0` 對照。為什麼是 `hartid + 1` 而不是 `hartid`？堆疊往哪個方向長？

xv6 的第一道指令是 `_entry` 的 `la sp, stack0`。`la` 是虛擬指令，`x/i` 會顯示它實際變成的指令，請記下來。

改回預設的三個 hart 再跑一次：`make qemu-gdb`，然後 `b *0x80000000`，再多按幾次 `c`。**預測**中斷點會被觸發幾次，再用 `info threads` 與 `info breakpoints` 確認。QEMU 把每個 hart 當成一個執行緒呈現給 GDB。

<details>
<summary>觀察結果。請在預測並執行之後再看。</summary>

- `la sp, stack0` 變成 `auipc sp,0xa` 與 `ld sp,472(sp)`：從全域偏移表（GOT）載入 `stack0` 的位址，核心確實有這張表（練習 G 的 `.got`）。組譯器為位置無關程式碼展開 `la` 時就是這樣。Ubuntu 的 `riscv64-linux-gnu-gcc` 以 `--enable-default-pie` 建置，而 Makefile 處理 `.S` 檔的規則不像 `CFLAGS`，沒有傳入 `-fno-pie`。手動加上 `-fno-pie` 組譯 `entry.S`，得到的則是 `auipc` 與 `addi`。
- `li a0, 1024*4` 變成 `lui a0,0x1`，`call start` 變成 `jal 0x80000054 <start>`。
- 在 `jal` 之前，`sp` 是 `0x8000b220`，`p &stack0` 印出 `0x8000a220`：hart 0 的堆疊是最前面的 4096 位元組，`sp` 從它的頂端開始。`a0` 為 `0x1000`，`a1` 為 1。
- 三個 hart 時，中斷點被觸發三次，這次依序是執行緒 1、3、2，`info breakpoints` 顯示 `already hit 3 times`。再加上 `b scheduler` 後，下一次停在執行緒 1 的 `scheduler`。

</details>

### E. `start()` 與 `mret`

```
(gdb) b start
(gdb) c
(gdb) p/x $mstatus
(gdb) next        （重複執行，並印出 $mstatus、$mepc、$satp、$medeleg、$mideleg）
```

- `mstatus` 的 MPP 欄位是第 12:11 位元（`kernel/riscv.h` 的 `MSTATUS_MPP_MASK`）。**預測** `w_mstatus(x)` 之後它的值：0b01 代表監督者。
- `w_mepc((uint64)main)` 之後，比較 `p/x $mepc` 與 `p main`。
- `start()` 把 `0xffff` 寫入 `medeleg` 與 `mideleg`。**預測**讀回來是不是 `0xffff`。特權規格把許多 CSR 欄位定為 WARL（可寫入任意值，讀回合法值）。記下你看到的值，並找出解釋它的規則。
- 用 `si` 與 `x/i $pc` 走到 `mret` 本身，或在 `kernel/kernel.asm` 找到它的位址，`b *` 該位址再 `c`。印出 `$priv` 與 `$pc`。接著 `b *$mepc`，執行一次 `si`，再印一次。這一道指令就是 xv6 離開機器模式的位置。不要省略這個中斷點：在本實驗記錄的執行中（QEMU 8.2.2、GDB 15.1），對 `mret` 直接 `si` 並不會在一道指令後停下，xv6 一路執行到 shell 提示字元。把 GDB 輸出複製到 `NOTES.md`，檢核 3 會用到。

<details>
<summary><code>start()</code> 中的觀察結果。請在預測並執行之後再看。</summary>

- 進入時 `mstatus` 是 `0xa00000000`，`w_mstatus(x)` 之後是 `0xa00000800`，所以 MPP 為 0b01。
- `w_mepc((uint64)main)` 之後 `mepc` 是 `0x80000e2e`，`p main` 印出同一個位址。
- `medeleg` 讀回 `0xbfff`，`mideleg` 讀回 `0x3666`，都不是 `0xffff`。寫入之前，`mideleg` 就已經讀到 `0x1444`。替每個沒寫進去的位元，以及每個在 xv6 寫入前就已設定的位元，找出對應的規則。
- `satp` 一直是 0。`pmpaddr0` 讀到 `0x3fffffffffffff`，`pmpcfg0` 讀到 `0xf`。
- 在 `mret`（這次建置位於 `0x800000c6`）時 `$priv` 是 3。`b *$mepc` 再 `si` 一次之後，`$priv` 是 1，`$pc` 是 `main` 的第一道指令 `0x80000e2e`。這時 `mstatus` 是 `0xa00000080`：MPP 回到 0，MPIE 被設定。

</details>

**刻意弄壞：** 把兩行 PMP 設定（`w_pmpaddr0`、`w_pmpcfg0`）註解掉，重新建置，同時設好 `b *$mepc` 與 `b *0x0`，再單步越過 `mret`。先預測，再觀察 `$pc`、`$priv`、`$scause`、`$mcause` 與 `$mepc`。最後恢復這兩行。

<details>
<summary>拿掉 PMP 設定後的觀察結果。請在預測並執行之後再看。</summary>

在 `mret` 時，`pmpaddr0` 與 `pmpcfg0` 都讀到 0。`si` 停在位址 0，`$priv` 仍是 3，`$mcause` 為 1（instruction access fault），`$mepc` 等於 `mret` 本身的位址，`$scause` 為 0。這個例外在機器模式中發生，所以委派給監督者模式的設定不適用。它跳到 `mtvec`，而 `mtvec` 仍是重置值 0，因為 xv6 從不寫入它。注意，例外是記在 `mret` 上，而不是 `main` 的第一道指令；請在 QEMU 的 RISC-V 目標程式碼中找出決定這件事的地方。

</details>

### F. `main()` 與第一張分頁表

```
(gdb) b main
(gdb) c
(gdb) bt
(gdb) set backtrace past-main on
(gdb) bt
(gdb) p $priv
(gdb) b kvminithart
(gdb) c
(gdb) p/x $satp
(gdb) finish
(gdb) p/x $satp
```

**預測：** `bt` 在 `main` 會印出什麼？設定 `set backtrace past-main on` 前後有何不同？沒有任何函式呼叫 `main`：`mret` 從 `mepc` 載入 `pc`，`ra` 裡留著的是 `start()` 中最後一次 `call` 放進去的值。GDB 用那個 `ra` 建立回溯，所以請帶著懷疑讀它的輸出。`kvminithart()` 之後，`satp` 最高四個位元應為 8（`SATP_SV39`），其餘是核心分頁表的實體頁號。**預測：** 開啟分頁後，GDB 中的核心位址會改變嗎？提示：讀 `kernel/vm.c` 的 `kvmmake()`，看它怎麼映射核心程式碼與 RAM。

繼續 `c`，直到終端機 1 出現 shell 提示字元。

<details>
<summary>觀察結果。請在預測並執行之後再看。</summary>

- 第一次 `bt` 只印出一個 frame：`#0 main () at kernel/main.c:13`，因為 GDB 預設在 `main` 停止回溯。`set backtrace past-main on` 之後，它印出 `#1 0x00000000800000be in start () at kernel/start.c:44` 與 `#2 0x000000008000001a in _entry () at kernel/entry.S:19`。第 44 行是呼叫 `timerinit()` 的那一行。`$ra` 是 `0x800000be`，即 `start+106`，也就是那次呼叫的返回位址。
- `$priv` 是 1。
- 在 `kvminithart` 時 `satp` 是 0，`finish` 之後是 `0x8000000000087fff`：模式 8，PPN 為 `0x87fff`。`p/x kernel_pagetable` 印出 `0x87fff000`。
- 開啟分頁後，`x/2i $pc` 顯示的位址和之前同一類（`0x80000ea8 <main+122>`）。

</details>

### G. 從外面看連結器腳本

```sh
riscv64-linux-gnu-readelf -h kernel/kernel | grep Entry
riscv64-linux-gnu-objdump -h kernel/kernel
grep -E ' (_entry|start|main|etext|_trampoline|end)$' kernel/kernel.sym
```

執行前先**預測**進入點與 `.text` 的起始位址，並確認 `_trampoline` 對齊頁邊界。接著回答：QEMU 跳到 `0x80000000` 是因為那是 RAM 的起點，而不是因為它讀了 ELF 的進入點。如果 `kernel.ld` 把另一個目的檔排在第一個，會發生什麼事？

<details>
<summary>觀察結果。請在預測並執行之後再看。</summary>

進入點是 `0x80000000`，`.text` 從那裡開始，大小為 `0x7000`。`kernel.sym` 列出 `_trampoline` 在 `0x80006000`（對齊頁邊界）、`etext` 在 `0x80007000`、`main` 在 `0x80000e2e`、`start` 在 `0x80000054`、`end` 在 `0x80023528`。`objdump -h` 也列出 `.got` 與 `.got.plt`，練習 D 的 `la sp, stack0` 讀的就是這張表。

</details>

## 檢核

在 `NOTES.md` 中回答，每個答案都附上支持它的 GDB 輸出。

1. 說明練習 B 的每個 QEMU 參數，並分別指出依賴它的 xv6 常數或程式碼。
2. 列出 CPU 執行的第一道指令（QEMU 位於 `0x1000` 的 ROM）與 xv6 執行的第一道指令（`0x80000000` 的 `_entry`），以 GDB 印出的樣子為準。
3. 指出 xv6 離開機器模式的那道指令，附上它前後的 `$priv` 與 `$pc`。
4. 和微控制器從 reset 到 `main` 的過程比較（[`mcu-foundations`](../../../mcu-foundations/02-learning-path.md) 階段 1）：因為這台機器有特權等級，多出了哪些工作？逐項列出 `start()` 做的事：特權設定、陷阱委派、PMP、`menvcfg`、計時器存取、`tp`。再反過來看：MCU 的啟動檔會複製 `.data` 並清零 `.bss`，而 `entry.S` 兩件都沒做。是誰替 xv6 做了這些事？
5. 複製 `bt` 在 `main` 的輸出，設定 `set backtrace past-main on` 前後各一份。它把 `start` 列為呼叫者時，說明為何這是誤導：實際轉移控制權的是哪一道指令？GDB 用的那個 `ra` 值從哪裡來？

## 常見失敗情況

- **WSL 1。** MIT 說明這些實驗在 WSL 1 下無法運作。用 `wsl -l -v` 檢查，用 `wsl --set-version <發行版> 2` 轉換。
- **`Couldn't find a riscv64 version of GCC/binutils`。** Makefile 會逐一執行幾種前綴的 `objdump` 來偵測工具鏈。安裝步驟 2 的套件，或執行 `make TOOLPREFIX=riscv64-linux-gnu-`。
- **`ERROR: Need qemu version >= 7.2`。** 發行版的 QEMU 太舊。MIT 提醒大概需要 Ubuntu 24 以上。
- **`check-qemu-version` 出現 shell 錯誤。** 這條規則用 `bc` 比較版本；若缺少 `bc`，用 `sudo apt-get install bc` 安裝。（MIT 的 Arch 套件清單有 `bc`，Ubuntu 清單沒有。）
- **`auto-loading has been declined`。** GDB 忽略了 `.gdbinit`，因此沒有連線，也沒有符號；`p $pc` 會回答 `No registers.`。加入設定步驟 5 的 safe-path。GDB 15.1 建議寫在 `~/.config/gdb/gdbinit`，寫在 `~/.gdbinit` 也可以。
- **對 `mret` 或 `sret` 執行 `si` 後停不下來。** 在 QEMU 8.2.2 與 GDB 15.1 下，單步越過這兩道指令都不會停，機器會一直執行到下一個中斷點。先在目標設中斷點（`b *$mepc` 或 `b *$sepc`），再 `si`。
- **`ld: warning: kernel/kernel has a LOAD segment with RWX permissions`。** binutils 2.42 每次建置核心都會印出這行。它只是警告，建置會繼續。
- **`Connection refused`，或卡在 `target remote`。** 先啟動 `make qemu-gdb`，並在同一個目錄執行 GDB，讓 `.gdbinit` 帶有你的連接埠。`make print-gdbport` 會印出連接埠。
- **用了一般的 `gdb` 而不是 `gdb-multiarch`。** Ubuntu 預設的 GDB 可能不含 RISC-V，請用 `gdb-multiarch`。
- **QEMU 對 Ctrl-C 沒反應。** 按 Ctrl-a 再按 x 離開。在 `qemu-gdb` 下，QEMU 會等 GDB 下 `c` 才開始執行。
- **中斷點觸發三次。** 每個 hart 各觸發一次。學習時請用 `CPUS=1`。
- **在 `/mnt/c` 下建置很慢，或腳本出現奇怪錯誤。** 請複製到 WSL 自己的檔案系統。用 Windows git 且 `core.autocrlf=true` 複製的檔案是 CRLF 換行，可能讓 `usys.pl` 與 shell 腳本出錯。
- **切換分支後留下舊的目的檔。** MIT 的實驗步驟在 `git checkout` 之後會執行 `make clean`，請照做。

## 後續會用到的地方

- 階段 01 用同樣的 GDB 設定停在陷阱路徑上；每次系統呼叫都會在那裡切換特權模式。
- 階段 03 會讀這裡開啟的分頁表；QEMU 監視器（Ctrl-a c，再輸入 `info mem`）能印出它。
- 階段 04 會追蹤 `timerinit()` 在這裡要求的計時器中斷。

## 下一步

[實驗 01](../01-system-calls/README.zh-TW.md) 從 `ecall` 經 trampoline 進入核心再返回，追蹤一次系統呼叫，接著在 Linux 與 Windows 上做同樣的事。
