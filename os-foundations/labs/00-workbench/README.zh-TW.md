[English](README.md) | **繁體中文** | [實作建置計畫，階段 00](../../SYLLABUS.zh-TW.md)

# 實驗 00：工作環境，開機並除錯 xv6

> **草稿，待審閱（2026-11-15）。本實驗的所有步驟都沒有在撰寫它的機器上執行過。** 那台 Windows 11 機器沒有 Ubuntu WSL 發行版、沒有 RISC-V 工具鏈，也沒有 QEMU，撰寫時也沒有安裝。指令取自 MIT 6.1810 的[工具頁](https://pdos.csail.mit.edu/6.1810/2025/tools.html)、[xv6-riscv](https://github.com/mit-pdos/xv6-riscv) 原始碼與 QEMU 原始碼，皆於 2026-11-15 閱讀。標為「*預期*」的輸出是依原始碼推得的結果，都不是實際觀察到的輸出。請記下你實際看到的內容。

## 目標

建立能用的 xv6 工作環境，並用它精確回答一個問題：從 QEMU 重置到 xv6 的 `main()` 之間執行了什麼，各在哪個特權模式？完成後，你能開機、離開、重新建置 xv6，接上 GDB，並在重置向量到 shell 提示字元之間的任何一道指令停下。

```
QEMU 重置 ROM    ->  _entry        ->  start()      --mret-->  main()       ->  scheduler()  ->  /init、sh
0x1000，M 模式       0x80000000        start.c                 main.c           proc.c           U 模式
                     entry.S，M 模式   M 模式                  S 模式           S 模式
```

## 先備條件

會用指標與結構的 C、Linux shell、git 與 make，以及讀懂 RISC-V 組合語言，程度相當於 [`c-compiler-foundations` 學習路線](../../../c-compiler-foundations/research/04-integration-path.zh-TW.md)第 1 關：讀得懂 `-O0` 的 prologue、找得到 `ra`、說得出區域變數為何在 `fp` 的負偏移。xv6 是 RV64（`-march=rv64gc`），暫存器與指標都是 64 位元；你也會遇到那條路線沒有涵蓋的 CSR 與特權模式。

## 環境設定（未在此執行）

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

現在就建立 `NOTES.md`，記下 Windows 組建編號、WSL 版本、發行版、上面所有版本，以及 xv6 的 commit。

## 先讀

這些檔案都很短。

- `Makefile`：`QEMUOPTS`、`CPUS := 3`、`qemu` 與 `qemu-gdb` 目標、由使用者 id 算出的 `GDBPORT`，以及從 `.gdbinit.tmpl-riscv` 產生 `.gdbinit` 的規則。
- `kernel/kernel.ld`：核心連結在 `0x80000000`，`kernel/entry.o(_entry)` 排第一，trampoline 獨占一頁。
- `kernel/entry.S`：每個 hart 計算 `sp = stack0 + (hartid + 1) * 4096`，再呼叫 `start`。
- `kernel/start.c`：把 `mstatus` 的前一特權設為監督者、`mepc` 設為 `main`，關閉分頁，把陷阱委派給監督者模式，開放實體記憶體保護（PMP），要求計時器中斷，把 hart id 存進 `tp`，最後執行 `mret`。
- `kernel/main.c`：hart 0 負責全部初始化，其他 hart 等待 `started`。
- [xv6 課本](https://pdos.csail.mit.edu/6.1810/2025/xv6/book-riscv-rev5.pdf)第 2 章中啟動 xv6 的一節。

## 練習

每個練習開始前，先把預測寫進 `NOTES.md`；執行後，把結果寫在預測旁邊。能解釋的錯誤預測，比猜對更有價值。

### A. 開機、觀察、離開

**預測：** 會出現幾行 `hart N starting`，順序如何？按 Ctrl-p 會列出哪些行程名稱？

```sh
make qemu
```

*預期（依 `kernel/main.c` 與 MIT util 實驗頁）：* 先是 `xv6 kernel is booting`，接著 hart 0 以外的每個 hart 各一行 `hart N starting`，然後是 `init: starting sh` 與 `$` 提示字元。預設 `CPUS := 3`，所以會有 hart 1 與 hart 2 兩行。它們互相競爭，每次開機的順序可能不同。輸入 `ls`，再按 Ctrl-p。`procdump()` 每個行程印一行，依序是 pid、狀態與名稱，所以預期會看到 `init` 與 `sh` 兩行。按 Ctrl-a 再按 x 離開 QEMU。

再用 `make CPUS=1 qemu` 開機一次，檢查你對單一 hart 的預測。

### B. 每個 QEMU 參數

`make -n qemu` 只印出指令、不執行。*預期（依 Makefile）：*

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

*預期（依 QEMU 原始碼：`hw/riscv/virt.c` 在 `0x1000` 配置一小塊 ROM，`hw/riscv/boot.c` 的 `riscv_setup_rom_reset_vec()` 在其中寫入六道指令）：* `$pc` 是 `0x1000`，六道指令依序為 `auipc t0`、`addi a2`、`csrr a0, mhartid`、`ld a1, 32(t0)`、`ld t0, 24(t0)`、`jr t0`。它們把 hart id 放進 `a0`、裝置樹位址放進 `a1`，再跳到 `0x1018` 存放的位址；使用 `-bios none` 時，那個位址就是 RAM 的起點 `0x80000000`。QEMU 的 gdb stub 也把目前的特權等級提供為虛擬暫存器 `priv`（`target/riscv/gdbstub.c`）：3 是機器模式、1 是監督者、0 是使用者。如果你的 GDB 看不到 `$priv`，改用 `p/x $mstatus` 與練習 E 的推理。

用 `si` 單步執行，直到 `$pc` 變成 `0x80000000`。

### D. 從 `_entry` 到 `start()`

```
(gdb) x/8i 0x80000000
(gdb) si          （重複執行，觀察 $sp、$a0、$a1）
(gdb) p/x $sp
(gdb) p &stack0
```

**預測：** hart 0 在 `call start` 之前的 `sp` 值，並與 `&stack0` 對照。為什麼是 `hartid + 1` 而不是 `hartid`？堆疊往哪個方向長？

xv6 的第一道指令是 `_entry` 的 `la sp, stack0`。`la` 是虛擬指令，`x/i` 會顯示它實際變成的指令，請記下來。

改回預設的三個 hart 再跑一次：`make qemu-gdb`，然後 `b *0x80000000`，再多按幾次 `c`。**預測**中斷點會被觸發幾次，再用 `info threads` 確認。QEMU 把每個 hart 當成一個執行緒呈現給 GDB。

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
- 用 `si` 與 `x/i $pc` 走到 `mret` 本身。印出 `$priv` 與 `$pc`，執行一次 `si`，再印一次。這一道指令就是 xv6 離開機器模式的位置。把 GDB 輸出複製到 `NOTES.md`，檢核 3 會用到。

**刻意弄壞：** 把兩行 PMP 設定（`w_pmpaddr0`、`w_pmpcfg0`）註解掉，重新建置，再單步越過 `mret`。先預測，再觀察 `$pc`、`$priv`、`$scause` 與 `$mcause`。最後恢復這兩行。

### F. `main()` 與第一張分頁表

```
(gdb) b main
(gdb) c
(gdb) bt
(gdb) p $priv
(gdb) b kvminithart
(gdb) c
(gdb) p/x $satp
(gdb) finish
(gdb) p/x $satp
```

**預測：** `bt` 在 `main` 會印出什麼？沒有任何函式呼叫 `main`：`mret` 從 `mepc` 載入 `pc`，`ra` 裡留著的是 `start()` 中最後一次 `call` 放進去的值。GDB 用那個 `ra` 建立回溯，所以請帶著懷疑讀它的輸出。`kvminithart()` 之後，`satp` 最高四個位元應為 8（`SATP_SV39`），其餘是核心分頁表的實體頁號。**預測：** 開啟分頁後，GDB 中的核心位址會改變嗎？提示：讀 `kernel/vm.c` 的 `kvmmake()`，看它怎麼映射核心程式碼與 RAM。

繼續 `c`，直到終端機 1 出現 shell 提示字元。

### G. 從外面看連結器腳本

```sh
riscv64-linux-gnu-readelf -h kernel/kernel | grep Entry
riscv64-linux-gnu-objdump -h kernel/kernel
grep -E ' (_entry|start|main|etext|_trampoline|end)$' kernel/kernel.sym
```

執行前先**預測**進入點與 `.text` 的起始位址，並確認 `_trampoline` 對齊頁邊界。接著回答：QEMU 跳到 `0x80000000` 是因為那是 RAM 的起點，而不是因為它讀了 ELF 的進入點。如果 `kernel.ld` 把另一個目的檔排在第一個，會發生什麼事？

## 檢核

在 `NOTES.md` 中回答，每個答案都附上支持它的 GDB 輸出。

1. 說明練習 B 的每個 QEMU 參數，並分別指出依賴它的 xv6 常數或程式碼。
2. 列出 CPU 執行的第一道指令（QEMU 位於 `0x1000` 的 ROM）與 xv6 執行的第一道指令（`0x80000000` 的 `_entry`），以 GDB 印出的樣子為準。
3. 指出 xv6 離開機器模式的那道指令，附上它前後的 `$priv` 與 `$pc`。
4. 和微控制器從 reset 到 `main` 的過程比較（[`mcu-foundations`](../../../mcu-foundations/02-learning-path.md) 階段 1）：因為這台機器有特權等級，多出了哪些工作？逐項列出 `start()` 做的事：特權設定、陷阱委派、PMP、計時器存取、`tp`。再反過來看：MCU 的啟動檔會複製 `.data` 並清零 `.bss`，而 `entry.S` 兩件都沒做。是誰替 xv6 做了這些事？
5. 複製 `bt` 在 `main` 的輸出。如果它把 `start` 列為呼叫者，說明為何這是誤導：實際轉移控制權的是哪一道指令？GDB 用的那個 `ra` 值從哪裡來？

## 常見失敗情況

- **WSL 1。** MIT 說明這些實驗在 WSL 1 下無法運作。用 `wsl -l -v` 檢查，用 `wsl --set-version <發行版> 2` 轉換。
- **`Couldn't find a riscv64 version of GCC/binutils`。** Makefile 會逐一執行幾種前綴的 `objdump` 來偵測工具鏈。安裝步驟 2 的套件，或執行 `make TOOLPREFIX=riscv64-linux-gnu-`。
- **`ERROR: Need qemu version >= 7.2`。** 發行版的 QEMU 太舊。MIT 提醒大概需要 Ubuntu 24 以上。
- **`check-qemu-version` 出現 shell 錯誤。** 這條規則用 `bc` 比較版本；若缺少 `bc`，用 `sudo apt-get install bc` 安裝。（MIT 的 Arch 套件清單有 `bc`，Ubuntu 清單沒有。）
- **`auto-loading has been declined`。** GDB 忽略了 `.gdbinit`，因此沒有連線，也沒有符號。加入設定步驟 5 的 safe-path。
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
