[English](README.md) | **繁體中文** | [實作建置計畫，階段 07](../../SYLLABUS.zh-TW.md)

# 實驗 07：檔案系統

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **xv6 部分（練習 A–F）：已建置並執行**，於 2026-11-16 在 Windows 11 上以 podman 執行的 Ubuntu 24.04 容器中完成（podman machine 跑在 WSL 2 上，為 rootful 模式；不是 WSL 2 Ubuntu 本身），使用 QEMU 8.2.2、riscv64-linux-gnu-gcc 13.3.0、主機端工具用的 gcc 13.3.0 與 expect，xv6-riscv 的 commit 為 [`fa2ddcb7`](https://github.com/mit-pdos/xv6-riscv/commit/fa2ddcb7ca87a77af579380a6eafd5aff163b44f)（2026-11-05）。練習 B、C、D 的結果來自不隨附的參考實作。練習 A、E、F 只用到本資料夾中的檔案。在寫入途中以 SIGKILL 強制結束 QEMU 共 11 次：5 次停在 commit 中選定的位置，6 次在固定的延遲之後。輸出：[`xv6/example-output.txt`](xv6/example-output.txt)。在套用全部三個參考修補檔的核心上，以三個 hart 執行 `usertests -q` 通過，約花 8.5 分鐘；其中的 `writebig` 測試現在要寫一個 65,803 個區塊的檔案。
> - **Linux 部分（練習 G、H）：已建置並執行**，在同一個容器中跑了兩次，核心 6.6.87.2-microsoft-standard-WSL2、gcc 13.3.0，檔案放在一個 ext4 的 podman volume 上。`kcmp(2)` 沒有 `--cap-add SYS_PTRACE` 時被拒絕，加上後可以使用。輸出：[`linux/example-output.txt`](linux/example-output.txt)。
> - **Windows 部分（練習 I、J）：已建置並執行**，以 WinLibs 的 gcc 16.2 直接在本機、不提升權限執行，檔案放在 NTFS 上。輸出：[`windows/example-output.txt`](windows/example-output.txt)。
> - **沒有執行的部分：** 標為「*未在此執行*」的步驟。負責寫回的執行緒本身（Linux 的 flusher 執行緒、NT 的 lazy writer 與 mapped page writer）沒有做追蹤；練習 H 與 J 是從外部量測它們的效果。
> - **機器：** Windows 11 Pro 組建 26300、Intel Core i7-11700（16 個邏輯處理器）、有 hypervisor。每次執行期間機器上都同時有其他工作，包括同一台 podman machine 裡另外兩個實驗。以下所有時間都只屬於這台機器與這幾次執行。

## 目標

檔案系統要守住兩個承諾：檔案內容是最後一次寫入的內容；任何時刻當機，磁碟都停在下次開機能修好的狀態。在 xv6 中，你要擴充磁碟上的 inode，讓檔案能長到 64 MiB；在路徑查找中加入符號連結；並用 `mmap` 把檔案映射進記憶體。接著在寫入途中強制結束 QEMU，在下次開機前從主機端讀取磁碟映像檔，看日誌如何把它恢復原狀。在 Linux 與 NT 上，你會碰到本課程後面會用到的兩個問題：檔案描述子（或 handle）到底指向什麼，也就是階段 08 在行程間傳遞的東西；以及被映射的髒頁何時寫回磁碟，由誰決定。

```
行程 A             行程 A              行程 B（A 的 fork）       行程 C
fd 3 ──┐           fd 4（dup 自 3）─┐  fd 3 ──┐                  fd 5 ──┐
       ▼                            ▼         ▼                         ▼
  開啟檔案描述：位移、狀態旗標、存取模式                       另一個開啟檔案描述
  （xv6 與 Linux 的 struct file；NT 的檔案物件）               （有自己的位移）
                     │                                                 │
                     └──────────────────► inode ◄──────────────────────┘
                                            │ 緩衝快取（xv6）、page cache（Linux）、cache manager（NT）
                                            ▼
                                           磁碟

xv6 commit()：  write_log()       write_head(n)         install_trans()       write_head(0)
                區塊寫進日誌區    提交點：表頭寫上      區塊複製回原位        清除這筆交易
                                  這些區塊編號
在這裡當機：    如同這次呼叫      下次開機把 n 個       下次開機再複製        沒事可做
                從未發生          區塊全部複製回原位    一次回原位
                當機點 1          當機點 2（k）         當機點 3
```

## 固定的 xv6 commit

本實驗的所有內容都是針對 **xv6-riscv commit `fa2ddcb7ca87a77af579380a6eafd5aff163b44f`** 撰寫並執行的，也就是[實驗 03](../03-virtual-memory/README.zh-TW.md#固定的-xv6-commit) 固定的版本。這個版本中與本實驗有關的事實：區塊大小 1024 位元組，`FSSIZE` 為 2000 個區塊；日誌可放 `LOGBLOCKS` = 30 個區塊，每個系統呼叫預留其中 `MAXOPBLOCKS` = 10 個；inode 有 12 個直接區塊與 1 個單一間接區塊。`fsinit()` 在第一個行程中執行（由 `forkret()` 呼叫），因為復原要讀磁碟，必須能夠睡眠。復原之後，`ireclaim()` 會釋放沒有連結、但當機時仍被開啟的 inode。另外有一個 `sync()` 系統呼叫。MIT 的 `fs` 與 `mmap` 實驗分支起始程式碼不同，所以這裡的修補檔只適用於原版 xv6-riscv。

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`xv6/ofdtest.c`](xv6/ofdtest.c) | 在原版核心上以 `dup`、兩次 `open` 與 `fork` 透過兩個描述子寫入 | 已在此建置並執行 |
| [`xv6/fsinspect.c`](xv6/fsinspect.c) | 在主機端讀取 `fs.img` 的程式：磁碟配置、日誌表頭與每個記錄區塊的身分、一致性檢查（一個 fsck）、單一檔案使用的區塊 | 已在此建置並執行 |
| [`xv6/bigfile.c`](xv6/bigfile.c) | 寫一個檔案直到核心拒絕，讀回檢查，再刪除 | 已在此建置並執行，原版與參考核心皆有 |
| [`xv6/symlinktest.c`](xv6/symlinktest.c) | 符號連結：跟隨、`O_NOFOLLOW`、連結鏈、循環、懸空連結、`link`／`unlink` | 已在此建置並執行，參考核心 |
| [`xv6/mmaptest.c`](xv6/mmaptest.c) | `mmap`／`munmap`：延遲載入、`MAP_SHARED` 與 `MAP_PRIVATE`、權限、部分解除映射、`fork`、`exit` | 已在此建置並執行，參考核心 |
| [`xv6/lab07-crashpoint.patch`](xv6/lab07-crashpoint.patch) | 在 `commit()` 中加入當機點，以及啟用其中一個的 `crashpoint(p, k)` 系統呼叫 | 已在此建置並執行 |
| [`xv6/crashwrite.c`](xv6/crashwrite.c) | 給日誌一筆可以弄丟的交易：附加寫入、截短、無止盡的重寫迴圈，以及檢查每一個區塊 | 已在此建置並執行 |
| [`xv6/break-install-first.patch`](xv6/break-install-first.patch) | 刻意弄壞：`commit()` 先把區塊寫回原位，之後才寫表頭 | 已在此建置並執行 |
| [`xv6/run.sh`](xv6/run.sh)、[`xv6/xv6crash.exp`](xv6/xv6crash.exp) | 從乾淨的原始碼樹建出每個變體，用實驗 03 的 `xv6run.exp` 開機，在指定處以 SIGKILL 結束 QEMU，並在兩次開機之間執行 `fsinspect` | 已在此執行 |
| 參考解答（不隨附） | 練習 B、C、D | 這幾項的輸出紀錄來自放在本儲存庫之外的參考修補檔 |
| [`linux/ofd.c`](linux/ofd.c) | 描述子與開啟檔案描述：位移、旗標、`/proc/self/fdinfo`、`kcmp`、OFD 鎖與 POSIX 鎖 | 已在此建置並執行 |
| [`linux/pagecache.c`](linux/pagecache.c) | `mmap` 與 `read` 共用一份 page cache；`msync`、`munmap`、`fsync` 與 flusher 執行緒，透過 `/proc/self/smaps` 觀察 | 已在此建置並執行 |
| [`linux/run.sh`](linux/run.sh) | 建置並執行上面兩個程式 | 已在此執行 |
| [`windows/nt_handles.c`](windows/nt_handles.c) | `DuplicateHandle`、兩次 `CreateFile`、被繼承的 handle、複製進子行程的 handle，以及唯讀的複製品 | 已在此建置並執行 |
| [`windows/nt_flush.c`](windows/nt_flush.c) | 被映射的檔案：`FlushViewOfFile`、`FlushFileBuffers`、一直保持髒的 view、解除映射的 view，以及經由 cache manager 的 `WriteFile` | 已在此建置並執行 |
| `*/example-output.txt` | 輸出紀錄 | 2026-11-16 記錄 |

參考解答不隨附：練習 B、C、D 的輸出紀錄來自放在本儲存庫之外的參考修補檔。它們解的是本實驗版本的 MIT 6.1810 `fs`（大型檔案、符號連結）與 `mmap` 作業，請自己寫。要讓 `run.sh` 執行你的成果，把它存成針對固定 commit 的修補檔，每一個疊在前一個之上：練習 B 存成 `0001-bigfile.patch`，C 存成 `0002-symlink.patch`，D 存成 `0003-mmap.patch`。把它們放在 git 會忽略的 `xv6/solution/`，或用 `SOLUTION_DIR` 指向它們。缺少修補檔的部分會印出說明後略過。測試程式不必自己加進 `UPROGS`；你的修補檔沒有加的話，`run.sh` 會替你加。

## 環境設定

- **xv6。** 切到固定 commit 的實驗 00 工作環境，再加上 `expect`、`patch` 與主機端的 `gcc`（給 `fsinspect` 用）。`run.sh` 每個變體都會把你的 checkout 複製到暫存目錄，從不修改它：

  ```sh
  sh xv6/run.sh ~/xv6-riscv stock crash random break    # 不需要解答
  SOLUTION_DIR=~/lab07-mine sh xv6/run.sh ~/xv6-riscv   # 全部
  SOLUTION_DIR=~/lab07-mine sh xv6/run.sh ~/xv6-riscv usertests
  ```

  預設的整套執行在這裡約花 5 分鐘，其中約 3 分鐘用在練習 B 寫一個最大尺寸的檔案。手動操作時請用一個 hart（`make CPUS=1 qemu`），區塊編號才會和下文一致。
- **這裡使用的容器。** 一個裝有 MIT 工具套件、expect，並在 `/opt/xv6-riscv` 放有固定版本原始碼的映像檔，從 Git Bash 啟動，儲存庫以唯讀方式掛載：

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm -v "C:/path/to/research-design:/work:ro" \
    -v "C:/path/to/your/patches:/sol:ro" oslab:24.04 \
    sh -c 'SOLUTION_DIR=/sol sh /work/os-foundations/labs/07-file-systems/xv6/run.sh /opt/xv6-riscv'
  ```

- **Linux：** gcc，然後在 WSL 2 Ubuntu 中執行 `sh linux/run.sh ~`。給它一個位在磁碟檔案系統上的目錄：WSL 2 的家目錄是 ext4。不要用 tmpfs 上的 `/tmp`，也不要用 `/mnt/c`。這裡的執行使用一個 podman named volume（`podman volume create lab07-data`，掛在 `/data`），它是 ext4；容器自己的根目錄是以 `volatile` 選項掛載的 overlayfs，會略過 `fsync`。在容器中使用 `kcmp(2)` 要加上 `--cap-add SYS_PTRACE`。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc。`nt_flush.c` 要連結 `-lpdh`。都不需要系統管理員權限。
- 對應的 MIT 實驗是 [fs](https://pdos.csail.mit.edu/6.1810/2025/labs/fs.html) 與 [mmap](https://pdos.csail.mit.edu/6.1810/2025/labs/mmap.html)。它們的測試程式（`bigfile`、`symlinktest`、`mmaptest`）在 `xv6-labs-2025` 的分支裡；這裡的測試程式是本實驗自己的，在原版 xv6-riscv 上執行。

## 先讀

- xv6 課本第 10 章（檔案系統），也就是 MIT fs 實驗要求閱讀的章節。
- `kernel/log.c`：`begin_op()`、`end_op()`、`log_write()`、`commit()`、`recover_from_log()`。`kernel/bio.c`：`bread()`、`bwrite()`、`bpin()`。`kernel/fs.c`：`balloc()`、`bmap()`、`itrunc()`、`iput()`、`ireclaim()`、`namex()`。`kernel/file.c` 與 `kernel/file.h`：`struct file`。`kernel/sysfile.c`：`sys_open()`、`create()`、`sys_link()`、`sys_unlink()`。磁碟配置看 `mkfs/mkfs.c`。
- Linux：[open(2)](https://man7.org/linux/man-pages/man2/open.2.html)（開啟檔案描述）、[F_GETFL(2const)](https://man7.org/linux/man-pages/man2/F_GETFL.2const.html)、[F_OFD_SETLK(2const)](https://man7.org/linux/man-pages/man2/F_OFD_SETLK.2const.html)、[proc_pid_fdinfo(5)](https://man7.org/linux/man-pages/man5/proc_pid_fdinfo.5.html)、[msync(2)](https://man7.org/linux/man-pages/man2/msync.2.html)、[/proc/sys/vm 文件](https://docs.kernel.org/admin-guide/sysctl/vm.html)中的 `dirty_*` 設定，以及 [VFS 概觀](https://docs.kernel.org/filesystems/vfs.html)。
- Learn：[DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle)、[CreateFileMapping](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga)、[FlushViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-flushviewoffile)、[UnmapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile)、[FlushFileBuffers](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers)、[File Caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching)。

## 練習

和之前一樣，執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. xv6 的描述子、開啟檔案描述與 inode（已在此執行）

讀 `kernel/file.h` 的 `struct file`，以及 `kernel/file.c` 的 `filealloc()`、`filedup()`、`fileclose()`。`p->ofile[fd]` 是一個指標；`dup()` 與 `fork()` 複製這個指標並增加 `f->ref`，`open()` 則為同一個記憶體中的 inode 配置一個新的 `struct file`。位移 `f->off` 放在 `struct file` 裡。把 [`ofdtest.c`](xv6/ofdtest.c) 複製到 `user/`、加進 `UPROGS`，執行前先**預測**每一部分之後檔案的內容。

觀察到的結果（原版核心，一個 hart）：

```
1. dup(): two descriptors, one struct file
  fd 3 and fd 4, inode 26 and 26
  write AAAA via fd a, then BBBB via fd b:
    the file is now "AAAABBBB"
2. open() twice: two struct files, one inode
  fd 3 and fd 4, inode 26 and 26
  write cccc via fd c, then dd via fd d:
    the file is now "ddccBBBB"
3. fork(): the child's descriptor is the parent's struct file
  the child writes child. and exits, then the parent writes parent:
    the file is now "child.parent"
4. the access mode lives in the struct file, so a dup has it too
  write via the O_RDONLY fd: -1, via its dup: -1
```

`struct file` 就是 POSIX 所說的開啟檔案描述，也是階段 08 從一個行程傳給另一個行程的東西。xv6 沒有 `lseek`，也沒有可改的檔案狀態旗標，所以它只存位移與存取模式。

接著看磁碟本身。在主機端針對你的原始碼樹建置 [`fsinspect.c`](xv6/fsinspect.c)（`gcc -O2 -Wall -I ~/xv6-riscv -o fsinspect fsinspect.c`），對一份新的 `fs.img` 執行。先依 `mkfs.c` 與 `param.h` **預測**：日誌從哪裡開始、到哪裡結束，有幾個 inode 區塊，資料區從哪裡開始？

```
fs.img: 2000 blocks of 1024 bytes
  superblock: size 2000, nblocks 1953, ninodes 200, nlog 31
  layout: 0 boot | 1 super | 2-32 log (header + 30) | 33-45 inodes (16 per block) | 46-46 bitmap | 47-1999 data
  log header: n = 0
  data blocks in use (bitmap): 994 of 1953; inodes in use: 24 of 199
/README: inode 2, type 2 (file), nlink 1, size 2441 bytes (3 blocks)
  inode lives in block 33; struct dinode is 64 bytes, 13 addrs (NDIRECT 12)
  direct (3 of 12 used): 48-50
```

`fsinspect fs.img check` 是一個小型 fsck。它沿著每個 inode 的區塊指標走，檢查每個資料區塊只被用一次且在點陣圖中標為使用中、點陣圖中標為使用中的區塊都有人用，以及每個 inode 的 `nlink` 等於指向它的目錄項數（在 xv6 中 `.` 不算）。練習 B 與 E 都靠它。

### B. 更大的檔案：雙重間接區塊（xv6）

在原版核心上，[`bigfile.c`](xv6/bigfile.c) 顯示出上限。觀察到：`MAXFILE = 268 blocks`、`wrote 268 blocks (274432 bytes); the write of block 268 was refused`。執行 `bigfile 268 keep` 後，`fsinspect fs.img file /big.file` 顯示 12 個直接區塊（1041–1052）、間接區塊 1053 與它的 256 個項目（1054–1309）。

照 MIT [fs 實驗](https://pdos.csail.mit.edu/6.1810/2025/labs/fs.html)的說明修改 inode：

1. `NDIRECT` 改成 11。`addrs[NDIRECT]` 仍是單一間接區塊，`addrs[NDIRECT + 1]` 是新的雙重間接區塊：一個存放 256 個區塊編號的區塊，每個編號指向一個列出 256 個資料區塊的間接區塊。`struct dinode` 維持 64 位元組，所以 `addrs[]` 在 `struct dinode` 與 `struct inode` 中都維持 13 個項目。`MAXFILE` 變成 11 + 256 + 256 × 256 = 65,803 個區塊。
2. `bmap()`：雙重間接區塊與其下的間接區塊，第一次需要時才配置；每個改過的索引區塊都要 `log_write()`；每個 `bread()` 過的區塊都要 `brelse()`。
3. `itrunc()`：釋放每個資料區塊、雙重間接區塊下的每個間接區塊，以及雙重間接區塊本身。
4. `kernel/param.h` 的 `FSSIZE`：一個最大尺寸的檔案需要 65,803 個資料區塊與 258 個索引區塊。參考解答用 70,000 個區塊（70 MB 的映像檔）；MIT 的實驗用 200,000。`mkfs` 不必做其他修改，為什麼？

執行前先**檢查** `filewrite()` 的一個分段是否仍放得進一筆交易：`filewrite()` 每筆交易最多寫 3 個區塊，而 `begin_op()` 每個系統呼叫預留 `MAXOPBLOCKS` = 10 個日誌區塊。數一數雙重間接範圍內的最壞情況：資料區塊、點陣圖區塊、inode、雙重間接區塊與其下的間接區塊，假設寫入沒有對齊區塊且跨過一個間接區塊。並**預測** `bigfile 6000 keep` 會留下幾個索引區塊。

搭配參考解答（不隨附）、一個 hart 觀察到的結果：

| | 原版核心 | 雙重間接核心 |
| --- | ---: | ---: |
| `MAXFILE` | 268 個區塊（274,432 位元組） | 65,803 個區塊（67,382,272 位元組） |
| `bigfile` | 寫入 268 個，第 268 個被拒絕 | 寫入 65,803 個，第 65,803 個被拒絕 |
| 寫一個最大尺寸檔案的 tick 數（1 tick ≈ 0.1 s） | 7 | 1,865 |
| `bigfile 6000 keep`：索引區塊 | （不可能） | 25：1 個間接、1 個雙重間接、其下 23 個 |
| `bigfile` 刪除檔案後的 `fsinspect check` | 一致 | 一致：使用中 7,019 個區塊 = 994 + 6,000 + 25 |

```
/big.file: inode 26, type 2 (file), nlink 1, size 6144000 bytes (6000 blocks)
  inode lives in block 34; struct dinode is 64 bytes, 13 addrs (NDIRECT 11)
  direct (11 of 11 used): 1049-1059
  singly indirect, block 1060 (256 of 256 entries used): 1061-1316
  doubly indirect, block 1317 (23 of 256 indirect blocks, 5733 data blocks): 1318 1575 1832 2089 2346 2603 ...
```

雙重間接區塊下的間接區塊每隔 257 個區塊出現一次（1318、1575……）：每一個都正好在它所列的 256 個資料區塊之前配置。以每個區塊來算，最大尺寸的檔案與 268 個區塊的檔案成本差不多：0.028 與 0.026 tick。這個成本來自日誌：每 3 個區塊的分段是一筆交易，每個資料區塊寫兩次（先寫日誌，再寫原位），點陣圖、inode 與索引區塊也各寫兩次，再加兩次表頭寫入。`balloc()` 每次配置都從區塊 0 開始掃描點陣圖，這個成本會隨磁碟變滿而增加，但在這個大小還看不出來。

**刻意弄壞（未在此執行）：** 在 `itrunc()` 中漏掉雙重間接的部分。**預測** `bigfile 6000` 刪除檔案之後，`fsinspect check` 會回報什麼。

### C. 符號連結（xv6）

照 MIT fs 實驗的規格新增 `symlink(target, path)`：一種新的 inode 類型 `T_SYMLINK`（4，放在 `kernel/stat.h`），內容是目標路徑；以及一個旗標 `O_NOFOLLOW`（0x800，放在 `kernel/fcntl.h`，不與其他 `O_` 旗標重疊）。`open()` 會跟隨連結，以及連到連結的連結，直到碰到不是連結的東西；加上 `O_NOFOLLOW` 時則開啟連結本身。遇到循環必須失敗而不是無限迴圈；設一個深度上限（MIT 建議約 10）就夠了。`link()` 與 `unlink()` 作用在連結本身，不需要支援指向目錄的連結。把 [`symlinktest.c`](xv6/symlinktest.c) 複製到 `user/`。

寫 `open()` 的迴圈之前先**預測**：查找目標時，可以繼續持有連結 inode 的鎖嗎？如果持有，一個指向自己的連結會發生什麼事？

搭配參考解答（不隨附）觀察到的結果；參考解答最多跟隨 10 層，並且先對連結做 `iunlockput()` 才對目標呼叫 `namei()`：所有檢查都通過，連續兩次（第二次一開始先清掉殘留的檔案）。5 層的連結鏈可以開啟，12 層的被拒絕（`open("/sl.12") = -1`）。加上 `O_NOFOLLOW` 時，`/sl.b` 以 `T_SYMLINK` inode 的身分開啟，內容是 `"/sl.a"`。

想想這個簡單版本沒做的事：
- 路徑中間的連結（`/link/file`）不會被跟隨，只有 `open()` 的最後一個分量會。你會在 `namex()` 的哪裡加上它？
- 相對路徑的目標是從呼叫者的目前目錄查找的。Linux 則從放置連結的目錄解析（[path_resolution(7)](https://man7.org/linux/man-pages/man7/path_resolution.7.html)）。測試只檢查連結與呼叫者都在 `/` 的情況，此時兩者結果相同。

### D. 以檔案為後盾的 mmap（xv6）

照 MIT [mmap 實驗](https://pdos.csail.mit.edu/6.1810/2025/labs/mmap.html)的說明實作 `mmap()` 與 `munmap()`，`user/user.h` 中的宣告如下，常數放在 `kernel/fcntl.h`（`PROT_READ` 1、`PROT_WRITE` 2、`PROT_EXEC` 4、`MAP_SHARED` 1、`MAP_PRIVATE` 2）：

```c
void *mmap(void *addr, uint64 len, int prot, int flags, int fd, uint64 off);
int munmap(void *addr, uint64 len);
```

`addr` 一律是 0（由核心選擇），`off` 是分頁大小的倍數（測試中都是 0），失敗時 `mmap` 回傳 `(void *)-1`。組成部分：

1. **每個行程一張映射表：** 16 個項目，每項記錄位址、長度、位移、保護權限、旗標與 `struct file *`（用 `filedup()` 取得參考）。映射放在 `TRAPFRAME` 之下，每個新的映射放在目前最低的映射之下，並讓 heap 不會長進去。
2. **發生分頁錯誤前什麼都不做。** `mmap()` 不讀檔，也不配置記憶體。在這個版本中，`usertrap()` 把讀取與寫入的分頁錯誤交給 `vmfault()`，`copyin()`／`copyout()` 也會呼叫它，所以就在那裡處理映射的位址：配置一頁，用 `readi()` 把檔案讀進去（超過檔尾的部分維持 0），再依映射的權限建立映射。這樣一來，即使行程從未碰過某一頁，`write(fd, mapped_address, n)` 也能運作。
3. **在 `munmap()`、`exit()` 與 `exec()` 時寫回。** 對 `MAP_SHARED`，寫回硬體標為髒的分頁（`PTE_D`，第 7 位元；`start()` 設定了 `MENVCFG_ADUE`，所以 QEMU 在第一次寫入時就會設定它），每筆交易不大於 `filewrite()` 的分段，而且絕不寫過檔尾。`munmap()` 可以移除映射的開頭、結尾或全部。釋放分頁；映射消失時關閉檔案。
4. **`fork()`** 讓子行程擁有相同的映射（與檔案參考）。MIT 允許子行程在自己的分頁錯誤時重新讀檔。參考解答則改為複製父行程碰過的每一頁，讓私有的修改在 fork 之後仍然保留。

參考解答必須處理兩個陷阱：`pipewrite()` 與 `kwait()` 會在持有自旋鎖時呼叫 `copyin()`／`copyout()`，而 `readi()` 可能睡眠，所以在那裡不能讀入映射的分頁（參考解答拒絕這次分頁錯誤，系統呼叫因而失敗）；另外 `read(fd, 同一個檔案的映射, n)` 發生分頁錯誤時，`readi()` 已經持有該 inode 的睡眠鎖。

把 [`mmaptest.c`](xv6/mmaptest.c) 複製到 `user/`。標為 `ok`／`FAILED` 的行是要求；標為 `observed` 的行是要求沒有規定的行為。執行前先**預測**每一行 `observed`。搭配參考解答（不隨附）觀察到的結果：一個 hart 與三個 hart 都是 `mmaptest: 0 failure(s)`。各行 `observed`，對照 Linux 的行為：

| 問題 | xv6，參考解答 | Linux |
| --- | --- | --- |
| 在對檔案 `write()` 之前就碰過的分頁顯示 | 舊的位元組 | 新的位元組 |
| `MAP_SHARED` 寫入後、`munmap()` 之前以 `read()` 讀檔 | 舊的位元組 | 新的位元組 |
| 對第 0 頁 `munmap()` 之後，仍在映射中的第 2 頁 | 還不在檔案中 | 已經在檔案的分頁中 |
| 子行程看到父行程先前的私有寫入 | 有繼承 | 有繼承（寫入時複製） |
| 子行程寫入並結束後，父行程的 `MAP_SHARED` 分頁 | 看不到 | 看得到 |
| 行程 A 寫入自己的映射後，行程 B 的 `MAP_SHARED` 分頁 | 看不到 | 立刻看得到（[實驗 03](../03-virtual-memory/README.zh-TW.md) 練習 D） |

每一列的原因都一樣：xv6 沒有 page cache。它的緩衝快取存放供檔案系統程式碼使用的磁碟區塊，而每個映射對每一頁都有自己的私有複本，只有寫回時才會和檔案碰頭。Linux 直接映射 page cache 的分頁，也就是 `read()` 與 `write()` 使用的同一批分頁（練習 H）。輸出紀錄中映射的位址是 `0x3FFFFFB000`：緊接在 `TRAPFRAME`（`0x3FFFFFE000`）之下的三頁。

**刻意弄壞（未在此執行）：** 在你的 `vmafault()`（或你取的名字）中拿掉自旋鎖的檢查，再執行 `write(pipe_fd, 未碰過的映射分頁, 1)`。**預測**會出現哪一種 panic。

### E. 在寫入途中強制結束 QEMU（xv6，已在此執行）

這是本階段的檢核重點，而且不需要解答：[`lab07-crashpoint.patch`](xv6/lab07-crashpoint.patch) 在 `commit()` 中加入三個核心可以停下的位置，以及一個系統呼叫 `crashpoint(p, k)`（編號 30），為下一次 commit 啟用其中一個。停下時，核心印出這筆交易的區塊、磁碟上的表頭內容，以及已經寫回原位幾個區塊，接著關閉中斷並空轉。[`xv6crash.exp`](xv6/xv6crash.exp) 看到這則訊息，就以 SIGKILL 結束 QEMU，這最接近直接拔掉電源線。xv6 看到已完成的每一筆寫入都已經在 `fs.img` 中：`virtio_disk_rw()` 會睡到裝置回報完成為止，而那時 QEMU 已把資料交給主機。

`run.sh crash` 的每個情境都從同一份映像檔開始，由 `crashwrite prepare` 產生：一個 12 個區塊的 `/crashfile`（inode 27，區塊 1093–1104），正好是原版 inode 的全部直接區塊。接著：

1. 開機，執行 `crashwrite append P K`（或 `trunc`），在當機點強制結束 QEMU；
2. 在主機端執行 `fsinspect fs.img log` 與 `fsinspect fs.img check`：當機留下的磁碟狀態；
3. 再次開機：`recover_from_log()` 在 `fsinit()` 中、`init` 啟動之前執行，每複製一個區塊回原位就印出 `recovering tail i dst b`；接著 `crashwrite check` 讀取 `/crashfile` 的每一個區塊；
4. 在主機端再執行一次 `fsinspect fs.img check`。

附加寫入以一次 3,072 位元組的 `write()` 寫入區塊 12–14，`filewrite()` 把它做成一筆交易。看答案之前先**預測**它的區塊，依 `log_write()` 第一次看到它們的順序：點陣圖、一個新的間接區塊（區塊 12 是直接區塊之後的第一個）、三個資料區塊，以及 inode 所在的區塊。觀察到：`46 1105 1106 1107 1108 34`。

| 情境 | 強制結束時磁碟上的表頭 | 已寫回原位的區塊 | 下次開機前的 `fsinspect check` | 下次開機 | 之後的 `/crashfile` |
| --- | --- | --- | --- | --- | --- |
| 附加寫入，當機點 1 | n = 0 | 6 個中 0 個 | 一致 | 沒有東西要復原 | 12 個區塊，第 1 代：附加寫入從未發生 |
| 附加寫入，當機點 2，k = 3 | n = 6 | 6 個中 3 個 | 4 個問題：區塊 1105–1108 標為使用中，卻沒有 inode 使用 | `recovering tail 0`…`5` | 15 個區塊：0–11 為第 1 代，12–14 為第 2 代 |
| 附加寫入，當機點 3 | n = 6 | 6 個中 6 個 | 一致 | 再次 `recovering tail 0`…`5` | 15 個區塊，同上 |
| 截短，當機點 2，k = 1 | n = 2 | 2 個中 1 個（點陣圖） | 12 個問題：區塊 1093–1104 被 `/crashfile` 使用，卻標為空閒 | `recovering tail 0`、`1` | 0 個區塊；接著 `crashwrite other` 取用其中 4 個被釋放的區塊，檢查仍然一致 |

凡是表頭為 n > 0 的情況，`fsinspect` 也會在記憶體中把記錄的區塊複製回原位，檢查「復原後的樣子」，結果在開機真正這麼做之前就已經是一致的。

**逐步說明當機點 2**，使用上面的區塊編號，再在 `NOTES.md` 中針對你自己的執行做一次：

1. `write()` → `filewrite()` → `begin_op()`：沒有正在進行的 commit，日誌也有空間，所以 `log.outstanding` 變成 1。
2. `writei()` → 對區塊 12 呼叫 `bmap()`：`balloc()` 在緩衝快取中的點陣圖區塊 46 設定一個位元，並呼叫 `log_write(46)`；它只記下區塊編號並釘住這個緩衝區。用同樣方式把新的間接區塊 1105 清成零，再配置資料區塊 1106，把它的編號寫進 1105。區塊 13 與 14 加入 1107 與 1108，`iupdate()` 加入 inode 所在的區塊 34。到這裡還沒有任何東西寫到磁碟。
3. `end_op()`：`outstanding` 降到 0，所以由這次呼叫來 commit。`write_log()` 把六個快取中的區塊複製到日誌區塊 3–8：六次磁碟寫入。（當機點 1 停在這裡。資料已在日誌中，但表頭仍是 n = 0，所以下次開機會忽略它，檔案維持原樣。這就是「全有或全無」中的「全無」。）
4. `write_head()` 把 n = 6 與六個原位區塊編號寫進區塊 2。**這一次單一區塊的寫入就是提交點。** 區塊寫入被假設為不可分割，所以當機後表頭不是舊的 n = 0，就是新的 n = 6。
5. `install_trans(0)` 把區塊複製回原位：46、1105、1106……當機點讓它在 1107 之前停下。此時磁碟上：點陣圖說 1105–1108 使用中，區塊 1105 列出了它們，區塊 1106 存有資料，但區塊 34 裡的 inode 仍然說只有 12 個區塊、沒有間接區塊。`fsinspect check` 把 1105–1108 判為遺失（leaked）。這是一個不一致的檔案系統，只有日誌能讓它安全。
6. 下次開機：`fsinit()` → `initlog()` → `recover_from_log()` → `read_head()` 讀到 n = 6，`install_trans(1)` 把六個日誌區塊全部再複製回原位一次（`recovering tail 0 dst 46` … `recovering tail 5 dst 34`），包括已經在原位的三個，然後 `write_head()` 寫入 n = 0。之後 `ireclaim()` 尋找孤兒 inode。
7. 現在 `/crashfile` 有 15 個區塊，檢查結果一致。當機點 3 顯示在全部都已寫回之後再重播一次：把同一個區塊複製回原位兩次，得到的位元組相同，所以復原是冪等的，復原途中再當機也無妨。

截短顯示的是危險的方向。釋放在日誌中排在前面（`bfree()` 清除區塊 46 中的位元），之後才是 inode。寫回一半時，點陣圖說檔案的區塊是空閒的，inode 卻仍指向它們。若沒有復原，下一個被建立的檔案就會拿到這些區塊。練習 F 會讓這件事真的發生。

**隨機時間強制結束（已在此執行）。** `run.sh random` 用當機版核心開機，但不啟用任何當機點，執行 `crashwrite loop`（一輪接一輪重寫一個 30 個區塊的 `/crashfile`，並建立、刪除 `/crash.tmp`，不印任何東西），在固定延遲之後強制結束 QEMU。六次強制結束的觀察結果：

| 結束時間 | 磁碟上的表頭 | 下次開機前的 `fsinspect check` | 下次開機 | 之後的 `/crashfile` |
| --- | --- | --- | --- | --- |
| 2 s | n = 6：正在附加區塊 18–20，6 個中已有 4 個與原位相同 | 一致 | 複製 6 個區塊回原位 | 21 個區塊，全為第 23 代 |
| 3 s | n = 5：正在附加區塊 9–11，尚無任何區塊寫回 | 一致 | 複製 5 個區塊回原位 | 12 個區塊，全為第 35 代 |
| 4 s | n = 0 | 一致 | 沒有東西要復原 | 18 個區塊，全為第 45 代 |
| 5 s | n = 0 | 一致 | 沒有東西要復原 | 30 個區塊，全為第 55 代 |
| 6 s | n = 0 | 一致 | 沒有東西要復原 | 12 個區塊，全為第 65 代 |
| 7 s | n = 6：正在附加區塊 21–23，6 個中有 2 個與原位相同 | 3 個問題：區塊 1116–1118 標為使用中，卻沒有 inode 使用 | 複製 6 個區塊回原位 | 24 個區塊，全為第 76 代 |

下次開機之後，每份映像檔都一致，`/crashfile` 的每個區塊都是自己的那一行。檔案長度取決於最後一筆提交的交易：每一輪都先截短檔案，再每次 3 個區塊寫回去。半數的強制結束中，日誌裡都有一筆已提交的交易在等待。已提交的交易可能只寫回一半，看起來卻仍然一致：2 s 那次，依日誌順序前四個區塊已與原位相同，最後兩個（一個資料區塊與 inode 區塊）則不同，最可能是在 `install_trans()` 途中被結束；已經寫回的新區塊，從舊 inode 看來位在檔尾之後。`fsinspect log` 中的「same as home」只表示位元組相同，沒有改動的區塊看起來也是這樣，它不能證明該區塊已經寫回。

執行前先**預測**：多少比例的強制結束會看到表頭 n > 0？由什麼決定？

### F. 刻意弄壞：在提交之前寫回原位（已在此執行）

[`break-install-first.patch`](xv6/break-install-first.patch) 對調 `commit()` 中的兩行：先把區塊寫回原位，之後才寫表頭。其他都沒變，沒有當機時一切看起來也都正常。`run.sh break` 在這個核心上重做截短的情境。先**預測**下次開機會復原什麼，以及 `crashwrite other` 建立一個 4 個區塊的檔案之後，`/crashfile` 會發生什麼事。

觀察到的結果：

```
lab07: crash point 2, a transaction of 2 blocks: 46 34
lab07: all 2 written to the log; the header on the disk says n = 0; 1 of 2 installed at home
...
log header at block 2: n = 0. Recovery will install nothing.
  12 problem(s): blocks 1093-1104 in use (data blocks 0-11 of inode 27 /crashfile) but marked free in the bitmap
--- next boot (no "recovering" lines)
$ crashwrite check
check: /crashfile is 12288 bytes, 12 blocks
check:   blocks 0-11: generation 1
$ crashwrite other
$ crashwrite check
check: block 0 is torn, it starts "other: a new file"
...
check:   blocks 4-11: generation 1
check: 4 block(s) torn
--- host: fsinspect fs.img check
  PROBLEM: block 1093 is used twice: by inode 27 (/crashfile) and inode 28 (/other)
  ... (1094-1096 the same; 1097-1104 still in use but marked free)
```

（已節錄；完整內容在輸出紀錄中。）檔案系統毫無怨言地開機了，而兩個檔案現在共用四個區塊：寫入任何一個都會破壞另一個。這就是為什麼表頭的寫入必須在日誌寫入之後、任何原位寫入之前，也是為什麼日誌必須在提交點之前就存有整筆交易。

### G. Linux 的描述子與開啟檔案描述（已在此執行）

```sh
sh linux/run.sh ~        # 或：gcc -O2 -Wall -o ofd linux/ofd.c && ./ofd ~
```

[open(2)](https://man7.org/linux/man-pages/man2/open.2.html) 說，每一次 `open()` 都會建立一個新的開啟檔案描述，而 `dup()` 產生的描述子、或經由 `fork()` 繼承的描述子指向同一個，因此兩者共用檔案位移與檔案狀態旗標。[F_GETFL(2const)](https://man7.org/linux/man-pages/man2/F_GETFL.2const.html) 說在 Linux 上，`F_SETFL` 只能改變 `O_APPEND`、`O_ASYNC`、`O_DIRECT`、`O_NOATIME` 與 `O_NONBLOCK`。[`ofd.c`](linux/ofd.c) 重做一次 xv6 的 `ofdtest`，再加上 Linux 多出來的部分。先**預測**第 1–5 部分的每一行，特別是第 4、5 部分。

觀察到的結果（第 2 次，加上 `--cap-add SYS_PTRACE`；第 1 次除了 `kcmp` 顯示 `Operation not permitted` 之外都相同）：

```
1. dup(): two descriptors, one open file description
  write AAAA via fd a, then BBBB via its dup b:        file: "AAAABBBB"
    after F_SETFL O_APPEND and F_SETFD FD_CLOEXEC on fd 3 only:
    fd 3: O_APPEND on, FD_CLOEXEC on
    fd 4: O_APPEND on, FD_CLOEXEC off
    fdinfo/3:  pos:	8  flags:	02102002  ino:	64211
    fdinfo/4:  pos:	8  flags:	0102002  ino:	64211
    kcmp(KCMP_FILE, 3, 4) = 0 (the same open file description)
2. open() twice: two open file descriptions, one inode
  write cccc via fd c, then dd via fd d:               file: "ddccBBBB"
    kcmp(KCMP_FILE, 3, 4) = 1 (different ones)
3. fork(): the child's descriptor shares the parent's offset
  child writes child. and exits, then parent writes:   file: "child.parent"
4. open("/proc/self/fd/3"): a new description or the same?
    reopened as fd 4; its offset is 0, fd 3's is 12
5. locks: OFD locks belong to the description, POSIX locks to the process
    F_OFD_SETLK write lock via fd a:              granted
    F_OFD_SETLK write lock via its dup b:         granted
    F_OFD_SETLK write lock via a second open() c: refused (EAGAIN)
    after close(a), via c:                        refused (EAGAIN)
    after close(b) too, via c:                    granted
    F_SETLK (POSIX) write lock via fd a:          granted
    child, F_SETLK on its own open():             refused (EAGAIN)
    parent opened and closed an unrelated fd c for the same file
    child, again after the parent closed fd c:    granted
```

（已節錄；完整輸出有每一行 `fdinfo`。）各部分說明了什麼：

- **狀態旗標跟著描述走，描述子旗標不會。** 透過 fd 3 設定的 `O_APPEND` 出現在 fd 4 上，`FD_CLOEXEC` 則沒有。`fdinfo` 中的 `flags` 是八進位（[proc_pid_fdinfo(5)](https://man7.org/linux/man-pages/man5/proc_pid_fdinfo.5.html)）：`02` 是 `O_RDWR`、`02000` 是 `O_APPEND`、`0100000` 是 `O_LARGEFILE`，`02000000` 是 `O_CLOEXEC`，當該描述子設了 close-on-exec 時，核心會把它加進這一行。每個描述子的 `ino` 都是同一個 inode。
- **`kcmp(KCMP_FILE)`** 直接回答「是不是同一個開啟檔案描述」：0 表示相同，1 或 2 表示不同，並給出兩者的排序（[kcmp(2)](https://man7.org/linux/man-pages/man2/kcmp.2.html)）。它需要對被比較的行程有 ptrace 存取權。在容器中，沒有加 `--cap-add SYS_PTRACE` 時它以 `EPERM` 失敗；可能的原因是 podman 預設的 seccomp 設定，這點沒有進一步確認。
- **開啟 `/proc/self/fd/3` 會建立新的描述**（位移是 0，`kcmp` 也說不同）。open(2) 沒有描述這個情況；這是這次執行在這個核心上顯示的結果。
- **鎖。** OFD 鎖屬於開啟檔案描述：dup 出來的描述子可以再取得它，同一個行程中第二次 `open()` 則不行，而且要等該描述的最後一個描述子關閉才會釋放（[F_OFD_SETLK(2const)](https://man7.org/linux/man-pages/man2/F_OFD_SETLK.2const.html)）。傳統的 POSIX 鎖屬於行程，同一頁說，行程關閉該檔案的*任何*一個描述子時就會釋放：這裡父行程開啟又關閉一個不相干的描述子，鎖就不見了，子行程取得了先前被拒絕的鎖。這就是為什麼階段 08 的用戶端與伺服器不應依賴 POSIX 記錄鎖。

xv6 的 `struct file` 是同一種物件，只是更小：只有位移與存取模式，沒有狀態旗標，也沒有鎖。

### H. Linux 的 page cache 與由誰寫回（已在此執行）

帶著本實驗開頭那張圖去讀 [VFS 概觀](https://docs.kernel.org/filesystems/vfs.html)。檔案物件代表行程開啟的一個檔案（也就是開啟檔案描述）；VFS 開啟檔案時會建立一個新的 `struct file`，初始化時帶有指向 dentry 的指標，由 dentry 再找到 inode。inode 的 `address_space` 就是 page cache：它用來組織與管理 page cache 中的分頁，資料寫入分頁時會設定該頁的髒旗標。`read()`、`write()` 與該檔案的每一個 `MAP_SHARED` 映射，用的都是同一批分頁。

[`pagecache.c`](linux/pagecache.c) 以 `MAP_SHARED` 映射一個 16 MiB 的檔案，把每一頁弄髒，再從 `/proc/self/smaps` 計算自己這個映射的髒頁。請在磁碟檔案系統上執行（這裡是 ext4 volume）。先**預測**：另一個描述子上的 `pread()` 會不會立刻看到經由映射的寫入？`msync(MS_ASYNC)` 與 `msync(MS_SYNC)` 處理 16 MiB 各要多久？`munmap()` 與 `close()` 會寫任何東西嗎？如果什麼都不呼叫，依第二行印出的設定，分頁何時會變乾淨？

觀察到的結果，兩次執行（完整輸出也顯示 `/proc/meminfo` 的 `Dirty` 與 `Writeback`，它們計算的是整台虛擬機器）：

| 步驟（4,096 個髒頁，16 MiB） | 第 1 次 | 第 2 次 |
| --- | --- | --- |
| 經由映射寫入，再在另一個 fd 上 `pread()` 該位元組 | 看得到 | 看得到 |
| `msync(MS_ASYNC)` | 0.001 ms，仍有 16,384 kB 髒 | 0.001 ms，仍有 16,384 kB 髒 |
| `msync(MS_SYNC)` | 29.5 ms，之後 0 kB 髒 | 29.0 ms，之後 0 kB 髒 |
| `munmap()` 與 `close()`，重新映射並讀取每一頁 | 新映射中有 16,384 kB 髒 | 相同 |
| `fsync(fd)` | 6.0 ms，之後 0 kB 髒 | 6.8 ms，之後 0 kB 髒 |
| 什麼都不呼叫：分頁變乾淨的時間 | 31 s | 26 s |
| 設定 | `dirty_expire_centisecs` 3000、`dirty_writeback_centisecs` 500 | 相同 |

- **每一頁只有一份。** 經由映射的寫入不用 `msync()` 就能被 `pread()` 看到，`pwrite()` 也能從映射看到。沒有什麼需要保持一致，因為只有 page cache 這一份。
- **`MS_ASYNC` 什麼都不做。** [msync(2)](https://man7.org/linux/man-pages/man2/msync.2.html) 說，從 Linux 2.6.19 起 `MS_ASYNC` 實際上是 no-op，因為核心會正確追蹤髒頁，並在需要時寫回儲存裝置；`MS_SYNC` 則要求更新並等它完成。
- **解除映射與關閉都不會寫入。** `munmap()` 與 `close()` 之後，新的映射發現同樣的 16 MiB 仍是髒的：髒的狀態屬於 page cache，不屬於映射。msync(2) 說，不呼叫它的話，不保證變更會在 `munmap(2)` 之前寫回。在檔案的任何一個描述子上 `fsync()`，也會讓映射的分頁變乾淨。
- **由 flusher 執行緒決定。** 依[核心的 vm 設定說明](https://docs.kernel.org/admin-guide/sysctl/vm.html)，在記憶體中髒了超過 `dirty_expire_centisecs`（這裡是 30 s）的資料，會在下一次 flusher 執行緒醒來時寫出，而這些執行緒每隔 `dirty_writeback_centisecs`（5 s）醒來一次。髒記憶體超過可用記憶體的 `dirty_background_ratio` 時，寫回會提早開始；超過 `dirty_ratio` 時，正在寫入的行程會被要求自己寫回。第 1 次符合這條規則：31 s。第 2 次（以及之前一次試跑）在 26 s，早於分頁髒滿 30 s。原因沒有查明：其他容器同時在寫同一個 ext4 檔案系統，而任何對整個裝置啟動寫回的動作，例如別處的 `sync()`，都會順帶寫回這些分頁。請自己多跑幾次。

和練習 D 比較：在 xv6 中，除了弄髒它的行程的 `munmap()`、`exit()` 與 `exec()` 之外，沒有任何東西會寫回被映射的髒頁；在 Linux 上，弄髒它的行程完全不參與寫回。

### I. NT 的 handle 與檔案物件（已在此執行）

```powershell
cd windows
gcc -O2 -Wall -o build\nt_handles.exe nt_handles.c
build\nt_handles.exe .
```

在 NT 上，`CreateFile` 會建立一個檔案物件，目前的檔案位置存在檔案物件中。handle 是行程對某個物件的參考，帶有自己的存取權限。[DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle) 說，複製出的 handle 與原本的 handle 指向同一個物件，所以對物件的任何改變都會反映在兩個 handle 上；例如複製一個檔案 handle，兩者的目前檔案位置永遠相同。同一頁也說，要讓兩個 handle 有不同的位置，就再呼叫一次 `CreateFile`。先**預測** [`nt_handles.c`](windows/nt_handles.c) 的每一部分，特別是第 4 部分。

觀察到的結果，兩次執行相同：

```
1. DuplicateHandle: two handles, one file object
  write AAAA via handle a, then BBBB via its duplicate b:  file: "AAAABBBB"
    after SetFilePointerEx(a, 0, FILE_BEGIN): position via a 0, via b 0
    handles 00000000000000B0 and 00000000000000B4: CompareObjectHandles TRUE (one file object); file index 000c0000000eb088 and 000c0000000eb088
2. CreateFile twice: two file objects, one file
  write cccc via handle c, then dd via handle d:           file: "ddccBBBB"
    handles 00000000000000B4 and 00000000000000B0: CompareObjectHandles FALSE (two file objects); file index 000c0000000eb088 and 000c0000000eb088
3. a child process gets the parent's file object
  inherited: the child writes child., then the parent writes: file: "child.parent"
  DuplicateHandle into a child: it writes dup'd., then the parent: file: "child.parentdup'd.PARENT"
4. access rights belong to the handle, the position to the file object
    WriteFile via the read-only duplicate: FALSE, error 5 (ERROR_ACCESS_DENIED)
    ReadFile of 6 bytes via the duplicate; position via a is now 6
```

（handle 的值與檔案索引每次執行都不同。）

- 第 1–3 部分就是 NT 版的練習 G：複製或繼承的 handle 是同一個檔案物件的第二個參考，就像 `dup()` 與 `fork()` 為同一個開啟檔案描述產生第二個描述子；呼叫兩次 `CreateFile` 就是呼叫兩次 `open()`。`GetFileInformationByHandle` 取得的檔案索引扮演 inode 編號的角色。[CompareObjectHandles](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles)（Windows 10 以後）是 NT 版的 `kcmp(KCMP_FILE)`，而且不需要特殊權限。
- 第 3 部分的後半是階段 08 在 NT 上的那一步：用 `DuplicateHandle` 把 handle 複製進另一個行程，新 handle 的值透過 pipe 傳給子行程。在 Linux 上，同一步是 `SCM_RIGHTS`。
- 第 4 部分是兩個系統不同之處。唯讀的複製品無法寫入（錯誤 5），雖然原本的 handle 可以；而經由它讀取，會移動從原本 handle 看到的位置。在 Linux 上，存取模式屬於開啟檔案描述，所以 `dup()` 出來的描述子權限永遠相同，要較少的權限就得重新 `open()`（練習 G 第 4 部分）。DuplicateHandle 也提到，複製品不一定能拿到比原本*更多*的權限：以 `GENERIC_READ` 開啟的檔案 handle 無法被複製成具有寫入權限。

### J. NT 上被映射的檔案與寫回（已在此執行）

```powershell
gcc -O2 -Wall -o build\nt_flush.exe nt_flush.c -lpdh
build\nt_flush.exe . 256 30      # 目錄、MiB、觀察秒數；約 2 分鐘
```

[`nt_flush.c`](windows/nt_flush.c) 在 NTFS 上映射一個 256 MiB 的檔案，並把每一頁弄髒。兩個整機的計數器顯示髒資料在哪裡等待，都可以不提升權限透過 PDH 讀取：`\Memory\Modified Page List Bytes` 與 `\Cache\Dirty Pages`。其他程式也會讓它們變動。計時是比較乾淨的證據：發現 256 MiB 髒資料的寫回要花數十毫秒，什麼都沒發現的寫回約 2–3 ms 就返回。先**預測**每一部分：view 一直保持映射 30 s 之後，`FlushViewOfFile` 會發現分頁仍是髒的嗎？解除映射之後，多久才會寫出？

觀察到的結果，兩次執行：

| 步驟（65,536 個髒頁，256 MiB） | 第 1 次 | 第 2 次 |
| --- | --- | --- |
| 經由 view 寫入，再在另一個 handle 上 `ReadFile` 該位元組 | 看得到 | 看得到 |
| 對髒的 view 呼叫 `FlushViewOfFile` | 70.8 ms | 83.4 ms |
| 再呼叫一次 `FlushViewOfFile`，沒有髒頁 | 3.4 ms | 2.2 ms |
| 接著 `FlushFileBuffers` | 0.1 ms | 0.1 ms |
| 再弄髒，view 保持映射 30 s：modified page list | 沒有變化 | 沒有變化（有其他跳動） |
| 接著 `FlushViewOfFile` | 72.6 ms：仍然是髒的 | 65.6 ms：仍然是髒的 |
| 再弄髒，`UnmapViewOfFile` 與 `CloseHandle`：modified page list | +257 MiB，1 s 內消失 | +256 MiB，1 s 內消失 |
| 同樣 256 MiB 改用 `WriteFile`，立刻 `FlushFileBuffers` | 104.1 ms | 93.4 ms |
| 同上，但 30 s 後才 `FlushFileBuffers` | 24.5 ms；期間 cache dirty 由 256 降到 83 MiB | 24.6 ms；期間 cache dirty 由 258 降到 80 MiB |

- **view 還在映射時，沒有任何東西寫出它的髒頁。** 30 s 後，`FlushViewOfFile` 花的時間和剛弄髒時一樣。[UnmapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-unmapviewoffile) 解釋了原因：被修改的分頁要等到共用計數歸零，也就是所有共用這些分頁的行程都解除映射，或把它們從 working set 中修剪掉之後，才會寫到磁碟。這裡沒有看到修剪；在記憶體吃緊的機器上，修剪會把分頁移出 working set，它們就會被寫出。Linux 的 flusher 在約 30 s 後就寫出了被映射的分頁（練習 H）；在這裡，只要 view 還在，NT 就不去動它們。
- **解除映射就把它們交給記憶體管理員。** 256 MiB 立刻出現在 modified page list 上，並在一秒內離開。被修改的分頁必須先寫出，才能移到 standby list（這是 *Windows Internals* 描述的記憶體管理員設計，這裡引用的 Learn 頁面沒有談到），所以它們最可能就在那一秒內寫出。同一頁接著說，即使如此，被修改的分頁仍是「延遲」寫到磁碟的，並建議應用程式呼叫 `FlushViewOfFile`，以降低停電或系統當機時遺失資料的風險。一秒是這台機器的表現，不是保證。
- **`FlushViewOfFile` 不是全部。** [FlushViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-flushviewoffile) 不會寫回檔案的中繼資料，也不會等到變更從底層硬碟的快取寫到實體磁碟才返回；要做到這點，之後要再呼叫 `FlushFileBuffers`。這裡第二個呼叫只花 0.1 ms，這幾乎說明不了磁碟自己的快取做了什麼。
- **`WriteFile` 則經過 cache manager。** 依 [File Caching](https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching)，cache manager 每秒執行一次 lazy writer，把最近沒有寫回的分頁中的八分之一排入寫回佇列，並隨時調整數量。cache 的髒頁在 30 s 內減少約三分之二，之後 `FlushFileBuffers` 只剩約四分之一的工作（24.5 ms 對 104.1 ms）。
- **一致性。** 兩次執行中，經由 view 的寫入都能被 `ReadFile` 看到，`WriteFile` 也都能從 view 看到。[CreateFileMapping](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga) 只保證同一個檔案的各個 view 之間一致（遠端檔案除外），並明說被映射的檔案與用 `ReadFile`／`WriteFile` 存取的檔案不一定一致。所以這是這台機器上 NTFS 的表現，不是保證。

## Linux／NT 對照

| | xv6（你的 `mmap`） | Linux | NT |
| --- | --- | --- | --- |
| 描述子或 handle 指向什麼 | `struct file`：位移、存取模式 | 開啟檔案描述（`struct file`）：位移、狀態旗標、存取模式、OFD 鎖 | 檔案物件：目前位置；存取權限在每個 handle 上 |
| `dup()`／`DuplicateHandle`、`fork()`／繼承時是否共用 | 是 | 是 | 是，而且每個 handle 的權限可以較少 |
| 第二次 `open()`／`CreateFile` | 新的 `struct file` | 新的描述 | 新的檔案物件 |
| 被映射檔案的分頁 | 每個映射一份私有複本 | 檔案的 page cache 分頁 | 該檔案所有 view 共用的分頁 |
| 由誰寫回被映射的髒頁 | 該行程的 `munmap()`、`exit()`、`exec()`；沒有其他 | flusher 執行緒，在資料髒超過 `dirty_expire_centisecs` 之後，或超過髒資料門檻、遇到 sync 時提早；需要時用 `msync(MS_SYNC)` 或 `fsync()` | view 映射中且未被修剪時沒有；最後一個 view 解除映射之後由記憶體管理員「延遲」寫回；需要時用 `FlushViewOfFile` 與 `FlushFileBuffers` |
| 這裡的量測 | 在 `munmap()` 時寫回（mmaptest 第 6 部分） | 什麼都不呼叫時，26 s 與 31 s | 映射 30 s 後仍是髒的；解除映射後 1 s 內寫出 |
| 中繼資料的當機一致性 | 本實驗的日誌 | 檔案系統自己的日誌（ext4 用 jbd2） | NTFS 自己的日誌 |

最後一列不在本實驗的量測範圍內，只標出下一步可以去哪裡找。

## 檢核

在 `NOTES.md` 中回答以下問題。

1. **逐步說明當機後的復原過程。** 用你自己在當機點 2、k 介於 1 與 n − 1 之間的一次當機：依序列出這筆交易的每一次磁碟寫入（日誌區塊、表頭、原位區塊），下次開機前 `fsinspect log` 與 `fsinspect check` 的結果、`recovering` 各行，以及之後的檢查。明確指出哪一次寫入是提交點，以及為什麼在它之前或之後當機都安全。再解釋為什麼重播一筆交易兩次無害（當機點 3），以及 `break-install-first.patch` 弄壞了什麼（練習 F）。
2. **解釋 fd 與它背後的開啟檔案描述有何不同。** 用你的 `ofd` 輸出：哪些狀態在描述子上（`FD_CLOEXEC`），哪些在描述上（位移、狀態旗標、存取模式、OFD 鎖），哪些在 inode 上。對 xv6 的 `p->ofile[]` 與 `struct file`，以及 NT 的 handle 與它的檔案物件（`nt_handles` 第 4 部分）各做一次。階段 08 以 `SCM_RIGHTS` 把開啟檔案描述傳給另一個行程：之後傳送者與接收者共用哪些東西？哪些東西任一方改了，另一方不會察覺？
3. **分別找出每個系統中，由誰決定被映射的髒頁何時寫回磁碟**，並用你的數字佐證：你的 xv6 `mmap`（mmaptest 第 3、6 部分）、Linux（pagecache 第 4 部分）、NT（nt_flush 第 3、4 部分）。
4. 你的 `bigfile` 數字：為什麼 `NDIRECT` 降到 11，`MAXFILE` 如何算出 65,803，為什麼 `FSSIZE` 必須加大，以及一個 n 個區塊的檔案需要幾個索引區塊。
5. 在 mmaptest 第 2、3、7、8 部分，為什麼你的 xv6 中 `MAP_SHARED` 的行為和 Linux 不同？xv6 需要什麼才能表現得像 Linux？
6. 為什麼 `open()` 在查找目標之前必須先釋放符號連結的 inode？

## 常見失敗情況

- **`run.sh` 中 `patch` 回報套用失敗。** 你的修補檔是針對另一個 commit，或 MIT 的實驗分支做的。確認你產生 diff 的那個 clone 在哪個 commit。
- **`run.sh` 印出 `SKIPPED: needs ... in SOLUTION_DIR`。** 你的修補檔不在它找的地方，或檔名不是它預期的。
- **`fsinspect` 回報莫名其妙的區塊編號，或在新的映像檔上回報問題。** 它是用另一個原始碼樹的 `kernel/fs.h` 建置的，`NDIRECT` 不同。用 `-I` 指向你要讀的 `fs.img` 所屬的原始碼樹再建置。
- **`bigfile` 在你的核心上停在 268 個區塊，或出現 `panic: bmap: out of range`。** `MAXFILE` 或 `bmap()` 仍只涵蓋舊的配置；`writei()` 會拒絕超過 `MAXFILE * BSIZE` 的寫入。
- **`bigfile` 執行中出現 `balloc: out of blocks`。** `FSSIZE` 太小，放不下一個最大尺寸的檔案與它的索引區塊。
- **`panic: freeing free block`，或 `bigfile` 之後 `fsinspect check` 發現遺失的區塊。** `itrunc()` 重複釋放某個區塊，或漏掉雙重間接的部分。
- **`open()` 一個循環的符號連結時卡住或死結。** 沒有深度上限，或 `namei()` 查找目標時連結的 inode 仍被鎖住。
- **第一次碰觸映射分頁時出現 `usertrap(): unexpected scause 0xd`。** 分頁錯誤沒有送到你的映射程式碼：在這個版本中，`vmfault()` 對任何 `p->sz` 以上的位址都回傳 0。
- **有映射的行程結束時出現 `panic: freewalk: leaf`。** 在 `proc_freepagetable()` 之前沒有解除映射的分頁；它們位在 `p->sz` 之上，`uvmfree()` 管不到。
- **`pipewrite()` 中出現 `panic: sched locks`。** 你的分頁錯誤處理在 CPU 持有自旋鎖時呼叫了 `readi()`。
- **`munmap()` 之後檔案變大了。** 寫回時把整頁寫過了檔尾。
- **`expect` 等 `waiting to be killed` 時逾時。** 沒有走到當機點：k 不小於這筆交易的區塊數，核心印出 `not reached ... disarmed`。
- **強制結束後 QEMU 還在跑。** 訊號送給了 `make` 而不是 QEMU。這正是 `xv6crash.exp` 自己啟動 QEMU 的原因。
- **Linux：pagecache 第 4 部分中，映射一直沒有變乾淨。** 檔案在 tmpfs 上，沒有磁碟可寫。
- **Linux：`kcmp` 顯示 `Operation not permitted`。** 在容器中加上 `--cap-add SYS_PTRACE`。
- **Windows：`undefined reference to PdhOpenQueryA`。** 連結時加上 `-lpdh`。
- **Windows：計數器跳動數十 MiB。** 其他程式也在寫。執行兩次，並且比起計數器，更相信寫回的計時。

## 之後會在哪裡用到

- 階段 08 以 `SCM_RIGHTS` 在行程之間傳遞開啟檔案描述，並以 `DuplicateHandle` 把 NT handle 複製進另一個行程（練習 I 第 3 部分）。封存的 `memfd` 背後沒有磁碟，所以練習 H 的寫回問題不會發生在它身上。
- 階段 09 的伺服器會映射用戶端的緩衝區，階段 14 的 dma-buf 檔案描述子也是開啟檔案描述。

## 下一步

[實作建置計畫](../../SYLLABUS.zh-TW.md)的階段 08：把檔案描述子當作權能來傳遞、封存共享記憶體，並執行單一事件迴圈。
