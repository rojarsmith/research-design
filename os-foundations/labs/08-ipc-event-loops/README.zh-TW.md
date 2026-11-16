[English](README.md) | **繁體中文** | [實作建置計畫，階段 08](../../SYLLABUS.zh-TW.md)

# 實驗 08：行程間通訊、共享記憶體與事件迴圈

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **Linux 部分（練習 A–D）：已建置並執行**，於 2026-11-16 在 Windows 11 上以 podman 執行的 Ubuntu 24.04 容器中完成（podman machine 跑在 WSL 2 上，為 rootful 模式；不是 WSL 2 Ubuntu 本身）：核心 6.6.87.2-microsoft-standard-WSL2、16 個 vCPU、gcc 13.3.0、strace 6.8，以 root 執行。兩次執行都記錄在 [`linux/example-output.txt`](linux/example-output.txt)。練習 C 中修好的伺服器來自一個不隨附的參考修正。
> - **io_uring（練習 D）：未執行。** 在容器中 `io_uring_setup` 回傳 `ENOSYS`，但核心有 `CONFIG_IO_URING=y`，`io_uring_disabled` 也是 0，所以是 podman 預設的 seccomp 設定檔拒絕了這個呼叫。這裡沒有在不套用該設定檔的情況下執行。
> - **Windows 部分（練習 E–H）：已建置並執行**，直接在本機、未提升權限，各跑兩次，使用 WinLibs 的 gcc 16.2。輸出：[`windows/example-output.txt`](windows/example-output.txt)，其中的帳號名稱、機器名稱與 SID 已替換。
> - **未執行：** 練習 F 的 Process Explorer 步驟（這台機器沒有安裝 Process Explorer；程式會印出相同的資訊），以及練習 I 的 libwayland 原始碼閱讀。這裡沒有 libwayland 原始碼可讀，所以該處的預期答案來自它的公開 API 與文件（本草稿之前已知的內容），不是來自原始碼。
> - **文件：** 下文引用的 man page 與 Learn 說法，本草稿沒有重新查閱；也沒有使用網路。表格與紀錄中的每一種行為，都是實際執行程式觀察到的。
> - **機器：** Windows 11 Pro 組建 26300、Intel Core i7-11700（16 個邏輯處理器）、有 hypervisor。

## 目標

合成器是一個伺服器：它映射不可信的用戶端交給它的記憶體，而且絕不能因為任何一個用戶端而停下來。本實驗建立階段 09 無螢幕顯示伺服器所依賴的三種機制，以及它們在 Windows 上的對應：

- 透過 Unix socket 傳遞描述子，並知道是誰送來的；
- 一個接收端可以信賴、不會在它底下縮小的共享緩衝區；
- 一條同時等待 socket、計時器、訊號與其他執行緒的執行緒。

接著區分核心回報 I/O 的兩種方式：就緒與完成，並帶你去讀 libwayland 的事件迴圈，指出它用的是哪一種。

```
用戶端                                       核心                            伺服器：一條執行緒在 epoll_wait 中
memfd_create("buf", MFD_ALLOW_SEALING)
寫入像素；F_ADD_SEALS SHRINK|GROW|SEAL
sendmsg(sock, "BUFFER", SCM_RIGHTS [fd 5])  --> 安裝一個新的 fd（11），       socket 可讀 -> recvmsg：fd 11
                                                它和用戶端的 5 指向同一個      SO_PEERCRED：connect() 時的 pid、uid
                                                開啟檔案描述                   F_GET_SEALS：有 SHRINK 嗎？大小對嗎？
                                                                               mmap(PROT_READ, MAP_SHARED)
```

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`linux/fdpass.h`](linux/fdpass.h)、[`linux/fdpass.c`](linux/fdpass.c) | 以 `SCM_RIGHTS` 傳送與接收 fd、讀取 `SO_PEERCRED`、計算開啟的 fd 數；可在階段 09 重用 | 已在此建置並執行 |
| [`linux/sealbuf.h`](linux/sealbuf.h)、[`linux/sealbuf.c`](linux/sealbuf.c) | 建立、封存與檢查 `memfd` 緩衝區；接收端的檢查；可在階段 09 重用 | 已在此建置並執行 |
| [`linux/evloop.h`](linux/evloop.h)、[`linux/evloop.c`](linux/evloop.c) | 一個小型的單執行緒 `epoll` 迴圈，有 fd、`timerfd`、`signalfd` 與 `eventfd` 四種來源，形狀仿照 libwayland 的 `wl_event_loop`；可在階段 09 重用 | 已在此建置並執行 |
| [`linux/fdserver.c`](linux/fdserver.c) | 練習 A：接收 fd 並檢查對方身分的 Unix socket 伺服器 | 已在此建置並執行 |
| [`linux/sealtest.c`](linux/sealtest.c) | 練習 B：接收端對十種提供物的檢查、傳送端縮小不了的封存緩衝區、會引發 `SIGBUS` 的未封存緩衝區，以及寫入封存 | 已在此建置並執行 |
| [`linux/evserver.c`](linux/evserver.c) | 練習 C：一個 `epoll` 迴圈服務用戶端、60 Hz 的 `timerfd`、一個 `signalfd` 與一個 `eventfd`，附有腳本化的正常與有問題的用戶端；裡面有一個要你修的洩漏 | 已在此建置並執行 |
| [`linux/readiness.c`](linux/readiness.c) | 練習 D：水平觸發與邊緣觸發、可寫性即背壓、一般檔案、`io_uring` 探測 | 已在此建置並執行；`io_uring` 部分被 seccomp 拒絕 |
| [`linux/run.sh`](linux/run.sh) | 在 `/tmp` 建置所有程式，不需輸入地執行練習 A–D；若 `SOLUTION_DIR` 指向你的修正，也一併執行 | 已在此執行 |
| 參考解答（不隨附） | 練習 C 洩漏的修正 | 輸出紀錄中「your fix for the leak」一段來自放在本儲存庫之外的參考修補檔 |
| [`windows/namedpipe.c`](windows/namedpipe.c) | 練習 E：訊息模式的具名管道伺服器，以及用戶端的身分 | 已在此建置並執行 |
| [`windows/dupsection.c`](windows/dupsection.c) | 練習 F：以 `DuplicateHandle` 推給用戶端的分頁檔後盾 section | 已在此建置並執行 |
| [`windows/msgwait.c`](windows/msgwait.c) | 練習 G：一條擁有僅訊息視窗的 GUI 執行緒，以 `MsgWaitForMultipleObjectsEx` 同時等待四個 handle、它的訊息佇列與 APC | 已在此建置並執行 |
| [`windows/iocp.c`](windows/iocp.c) | 練習 H（選做）：把管道伺服器改成一條執行緒服務一個 I/O completion port | 已在此建置並執行 |
| [`linux/example-output.txt`](linux/example-output.txt)、[`windows/example-output.txt`](windows/example-output.txt) | 輸出紀錄 | 2026-11-16 記錄 |

**參考解答（不隨附）：** Linux 輸出中「exercise C: your fix for the leak」一段，來自放在本儲存庫之外的參考修正。請自己寫：複製 `linux/evserver.c`，修好它，再把差異存成針對隨附檔案的修補檔，例如 `diff -u evserver.c my-evserver.c > ~/lab08-mine/evserver-fdleak.patch`。之後 `SOLUTION_DIR=~/lab08-mine sh linux/run.sh` 會把它套到建置目錄中的複本，再跑一次示範。沒有它時，該步驟印出 `SKIPPED`。修補檔標頭中的檔名不重要；`.gitignore` 會讓 `*.patch` 與 `linux/solution/` 不進入 git。

## 環境設定

- **Linux：** gcc，選做步驟另需 `strace` 與 `patch`。實驗 00 的 WSL 2 Ubuntu 也可以；它的數字同樣是虛擬機器中的數字。

  ```sh
  sh linux/run.sh                                  # 全部
  SOLUTION_DIR=~/lab08-mine sh linux/run.sh        # 再加上你對練習 C 的修正
  ```

  以一般使用者執行時，`fdserver` 會略過兩個切換到 uid 65534 的用戶端，`evserver` 的「foreign」用戶端也會說明它被略過：切換 uid 需要 root。這裡記錄的執行使用實驗 05 的容器，從 Git Bash 啟動，儲存庫以唯讀方式掛載：

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm --cap-add SYS_PTRACE -e SOLUTION_DIR=/sol \
    -v "C:/path/to/research-design:/work:ro" -v "C:/path/to/your-fix:/sol:ro" oslab:24.04 \
    sh /work/os-foundations/labs/08-ipc-event-loops/linux/run.sh
  ```

  第三次執行沒有加 `--cap-add SYS_PTRACE`，一樣能追蹤伺服器，因為 `strace` 追蹤的是它自己的子行程。
- **Windows：** 任何 x86-64 的 MinGW-w64 gcc，在一般（未提升權限的）命令提示字元中：

  ```powershell
  cd windows
  gcc -O2 -Wall -o namedpipe.exe namedpipe.c -ladvapi32
  gcc -O2 -Wall -o dupsection.exe dupsection.c -ladvapi32
  gcc -O2 -Wall -o msgwait.exe msgwait.c -luser32
  gcc -O2 -Wall -o iocp.exe iocp.c
  ```

  每個程式都會把自己的複本當作用戶端啟動，所以都能無人值守地執行。Process Explorer 步驟需要 Sysinternals 的 [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer)。
- **給階段 09：** `fdpass`、`sealbuf` 與 `evloop` 是寫來讓你複製到階段 09 伺服器中的。每個標頭檔都說明了這些呼叫做什麼、不做什麼。

## 先讀

- [unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)：`SCM_RIGHTS`、`SO_PEERCRED`、抽象命名空間，以及 `SOCK_STREAM` socket 傳送輔助資料時至少要帶一個位元組資料的規則。建立控制訊息請看 [cmsg(3)](https://man7.org/linux/man-pages/man3/cmsg.3.html)。
- [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html)，以及 [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html) 的 "File Sealing"。
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)、[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)、[signalfd(2)](https://man7.org/linux/man-pages/man2/signalfd.2.html)、[eventfd(2)](https://man7.org/linux/man-pages/man2/eventfd.2.html)、[io_uring(7)](https://man7.org/linux/man-pages/man7/io_uring.7.html)。
- Learn：[CreateNamedPipe](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createnamedpipea)（包括它的預設安全描述元允許什麼）、[ImpersonateNamedPipeClient](https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-impersonatenamedpipeclient)、[DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle)、[Creating Named Shared Memory](https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory)、[MsgWaitForMultipleObjectsEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex)、[About Messages and Message Queues](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)、[I/O Completion Ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)。
- 實驗 03 練習 E（映射存在時的 `SIGBUS`），以及實驗 06 練習 D（`timerfd`）。

## 練習

執行任何東西之前，先把預測寫進 `NOTES.md`。

### A. 傳遞一個 fd，並檢查是誰送的（Linux）

[`linux/fdserver.c`](linux/fdserver.c) 在一個 `SOCK_SEQPACKET` socket 上監聽，並 fork 出自己的用戶端。每一部分是一個問題：

1. 用戶端開啟一個檔案，寫入 `0123456789`，移到偏移量 4，然後送出 fd。**預測：** 伺服器拿到的 fd 偏移量是多少？它讀到什麼？之後用戶端看到的偏移量是多少？
2. 用戶端以唯讀開啟一個檔案、刪除它的名稱（unlink），再送出 fd。**預測：** 伺服器能讀它嗎？能透過它寫入嗎？能透過 `/proc/self/fd` 以讀寫模式重新開啟嗎？
3. 三個用戶端連到一個抽象 socket：A 用伺服器的 uid；B 先切換到 uid 65534；C 以 uid 0 連線，在 `connect()` 之後、送出之前切換到 65534。**預測** `SO_PEERCRED` 對每一個回報什麼。
4. 對只容得下一個 fd 的接收端送出三個 fd；以及在 `SOCK_STREAM` socket 上隨位元組一起送 fd。

觀察結果，取自第一次執行，路徑與一則訊息經過縮短；第二次執行結果相同（[完整輸出](linux/example-output.txt)）：

```
  client: opened /tmp/lab08-fdserver-.../shared.txt as fd 4, inode 63392, offset now 4; sending it
  server: message "file" with fd 5 -> /tmp/lab08-fdserver-.../shared.txt, inode 63392
  server: offset of the received fd: 4
  server: read 3 bytes through it: "456"
  server: my own open() of the same path is fd 6, offset 0: a new open file description
  client: after the server read 3 bytes, my fd 4's offset is 7

  server: fd 5 -> /tmp/lab08-fdserver-.../nameless.txt (deleted)
  server: pread -> 23 bytes: "only the fd leads here"
  server: write through it -> -1, errno 9 (Bad file descriptor): the access mode travelled with the fd
  server: open("/proc/self/fd/5", O_RDWR) -> 6  (a reopen is checked against the file's permissions, not the fd's mode)

  server: peer pid 48 (fork returned 48), uid 0, gid 0; message "A (same uid), sent with uid 0"
  server: SO_PEERPIDFD gave pidfd 5, whose fdinfo says Pid 48
  server: peer pid 49 (fork returned 49), uid 65534, gid 65534; message "B (uid 65534 before connect), ..."
  server: uid 65534 is not mine (0): rejected, connection closed
  server: peer pid 50 (fork returned 50), uid 0, gid 0; message "C (uid 65534 after connect), sent with uid 65534"

  a) 3 fds sent, room for 1: received 1, MSG_CTRUNC set, open fds 8 -> 9
  b) SOCK_STREAM: sent "AB"+fd, "CD", "EF"+fd, "GH"; each recvmsg asks for 100 bytes and up to 4 fds
     recvmsg -> "AB" with 1 fd(s)
     recvmsg -> "CDEF" with 1 fd(s)
     recvmsg -> "GH" with 0 fd(s)
```

這說明了：

- **fd 指的是開啟檔案描述，不是檔案。** 伺服器的 fd 5 和用戶端的 fd 4 共用同一個偏移量：伺服器的讀取把用戶端的偏移量從 4 移到 7。伺服器自己對同一路徑的 `open()` 則建立了第二個描述，有自己的偏移量。這就是實驗 07 檢核要你說明的差別，也是兩個共用 fd 的行程必須約定由誰移動偏移量的原因。
- **fd 是權能。** 被 unlink 的檔案已經沒有名稱，伺服器仍然讀得到；唯讀模式隨 fd 一起過來，所以 `write()` 以 `EBADF` 失敗。不過在 Linux 上這個權能並不嚴密：再次開啟 `/proc/self/fd/5` 是對同一個 inode 的一般開啟，依 inode 的權限檢查（這裡以 root 執行，但 `0600` 檔案的一般擁有者也會通過），結果回傳了一個可讀寫的 fd。練習 B 會說明封存如何為 `memfd` 堵住這個漏洞。
- **`SO_PEERCRED` 回報的是 `connect()` 當時的身分，不是傳送者的身分。** 用戶端 C 以 uid 65534 送出，伺服器看到的仍是 uid 0。一個行程可以在有權限時連線，之後再放棄權限；也可以用 `SCM_RIGHTS` 把連好的 socket 本身交給另一個行程，伺服器看到的始終是當初連線的那一方。請在 accept 時檢查對方，並把結果理解為「是誰建立了這條連線」。
- **pid 是脆弱的名稱。** 對方結束後，它的 pid 可能被重用。`SO_PEERPIDFD`（Linux 6.5）回傳的 pidfd 指向的正是當初連線的那個行程。
- **抽象 socket 沒有權限。** 同一個網路命名空間中的任何行程都能連到 `@lab08-fdserver-45`，所以 uid 檢查是唯一的檢查。練習 C 使用放在 `0700` 目錄中的 socket 檔案，外來的用戶端連 `connect()` 都過不了。Wayland 把它的 socket 放在 `$XDG_RUNTIME_DIR`，也是同樣的理由。
- **收到的 fd 是接收端的責任。** 只留一個 fd 的空間時，核心安裝了一個，設定 `MSG_CTRUNC`，沒有安裝另外兩個（開啟的 fd 數只增加一）。空間更大時，對方送來的每個 fd 都會進入你的 fd 表，不管你要不要：見練習 C。
- **在串流 socket 上，fd 跟著位元組走。** 在這個核心上，一次 `recvmsg` 從來沒有回傳兩次都帶 fd 的傳送的位元組，但它確實把沒帶 fd 的 `"CD"` 和後面帶了 fd 的 `"EF"` 接在一起。所以在 `SOCK_STREAM` 上的協定必須從自己的訊息格式知道每則訊息要用幾個 fd，並把收到的 fd 放在佇列中，直到有訊息來取用。練習 I 請你在 libwayland 中找出這件事。

### B. 一個不會縮小的緩衝區（Linux）

[`linux/sealtest.c`](linux/sealtest.c) 與 [`linux/sealbuf.c`](linux/sealbuf.c)。如果你想自己寫這個檢查，請在讀隨附的版本之前，依 memfd_create(2) 與 fcntl(2) 寫出你自己的 `sealbuf_check()`：只有在 fd 永遠不會縮小、而且大小不小於傳送端宣稱的值時才接受。

**預測**下列每一種提供物的判定，再執行 `./sealtest`（或 `sh linux/run.sh`）。觀察結果，兩次執行相同：

| 提供物 | 接收端的判定 |
| --- | --- |
| 一個 pipe | 拒絕：不是一般檔案 |
| `/tmp` 中的一般檔案 | 拒絕：無法封存（`F_GET_SEALS`：`EINVAL`） |
| POSIX `shm_open()` 物件（tmpfs） | 拒絕：封存為 `SEAL`，需要 `SHRINK` |
| 沒有 `MFD_ALLOW_SEALING` 的 `memfd` | 拒絕：封存為 `SEAL`，需要 `SHRINK` |
| 允許封存但沒有封存的 `memfd` | 拒絕：沒有封存，需要 `SHRINK` |
| 只封存 `GROW` 的 `memfd` | 拒絕：封存為 `GROW`，需要 `SHRINK` |
| 已封存，但 4 頁卻宣稱 16 頁的 `memfd` | 拒絕：宣稱 65536 位元組，實際只有 16384 |
| 封存 `SHRINK` 的 `memfd` | **接受** |
| 封存 `SHRINK\|GROW\|SEAL` 的 `memfd` | **接受** |
| 封存 `SHRINK\|GROW\|WRITE\|SEAL` 的 `memfd` | **接受** |

memfd_create(2) 說，沒有 `MFD_ALLOW_SEALING` 而建立的 `memfd` 一開始就帶有 `F_SEAL_SEAL`，所以永遠無法再加上任何封存；`shm_open()` 背後的 tmpfs 檔案也回報了同樣的結果。這就是為什麼想提供封存緩衝區的 Wayland 用戶端必須使用 `memfd_create`。

**檢核：封存後的緩衝區無法被傳送端縮小。** 在第 2 部分，接收端接受並映射一個 4 頁、封存為 `SHRINK|GROW|SEAL` 的 `memfd`，接著傳送端用盡一切手段：

```
  sender:   ftruncate to 1 page                 -> EPERM (Operation not permitted)
  sender:   ftruncate to 0                      -> EPERM (Operation not permitted)
  sender:   ftruncate to 8 pages                -> EPERM (Operation not permitted)
  sender:   open(/proc/self/fd/4, O_TRUNC)      -> EPERM (Operation not permitted)
  sender:   F_ADD_SEALS WRITE (after SEAL)      -> EPERM (Operation not permitted)
  sender:   F_GET_SEALS                         -> SEAL|SHRINK|GROW; there is no call that removes a seal
  sender:   writes 0xAA into page 0 through its mapping -> ok (contents may change)
  sender:   punches a hole in page 2            -> ok
  receiver: first byte of each page: 0xaa 0x11 0x00 0x13  (no SIGBUS)
```

封存屬於 inode，不屬於某一個 fd，所以透過 `/proc` 重新開啟也沒用；封存也只能增加。不過 `SHRINK` 只保護大小：傳送端改寫了第 0 頁，又在第 2 頁打了一個洞，接收端之後讀到的是零。

**未封存的緩衝區會怎樣**（第 3 部分）：一個只檢查 `fstat()` 大小的接收端接受了未封存的 `memfd`，映射 4 頁，然後傳送端把它縮小到一頁：

```
  a) receiver without a SIGBUS handler
  receiver: fstat says 16384 bytes >= 16384 claimed: looks fine, mapping it
  sender:   ftruncate to 1 page -> ok
  receiver was killed by signal 7 (Bus error)
  b) receiver with a SIGBUS handler
  receiver: SIGBUS (si_code 2, BUS_ADRERR) at offset 0x1000; dropping this client's buffer and carrying on
```

大小檢查在執行時為真，在接收端使用時已為假：這是檢查時間與使用時間不一致（TOCTOU）的競爭，而用戶端想贏就能贏。在合成器中，(a) 代表所有其他用戶端的工作階段跟著結束。實驗 03 練習 E 用一般檔案展示過同樣的事。

**凍結內容**（第 4 部分；每個 `EPERM` 後面的 "(Operation not permitted)" 已省略）：

```
  F_SEAL_WRITE while the writer still has a shared writable mapping -> EBUSY (Device or resource busy)
  F_SEAL_WRITE after munmap                                    -> ok
  pwrite                                                       -> EPERM
  mmap PROT_WRITE, MAP_SHARED                                  -> EPERM
  mmap PROT_WRITE, MAP_PRIVATE                                 -> ok  (writes go to private copies)
    wrote 'X' through it; the file still starts with 'f'
  reopen /proc/self/fd/3 O_RDWR (ok), then write             -> EPERM
  F_SEAL_FUTURE_WRITE with the writer's mapping still in place -> ok
  the writer writes "frame 2" through its old mapping; a reader's mapping shows "frame 2"
  pwrite                                                       -> EPERM
  a new mmap PROT_WRITE, MAP_SHARED                            -> EPERM
```

`F_SEAL_WRITE` 讓位元組對所有人都不可變，包括透過 `/proc` 重新開啟的人，但寫入者必須先放棄它可寫的映射。`F_SEAL_FUTURE_WRITE` 讓已經持有映射的寫入者繼續寫，同時沒有人能再建立新的寫入途徑：這是一個發出唯讀視圖的生產者的形狀。**決定**合成器對用戶端的像素緩衝區（用戶端每個影格都會重畫）以及合成器送給用戶端的 keymap（任何用戶端都不應能替其他人改掉它）各應要求哪些封存，並說明理由。

### C. 一個 `epoll` 迴圈（Linux）

[`linux/evserver.c`](linux/evserver.c) 是建立在 [`evloop.c`](linux/evloop.c) 上的伺服器。它唯一的迴圈執行緒在 `epoll_wait` 中等待：監聽 socket 與每個用戶端 socket、一個 60 Hz 的 `timerfd`（代替 vblank）、`SIGINT`、`SIGTERM` 與 `SIGUSR1` 的 `signalfd`，以及一個 `eventfd`，由工作執行緒在算完某個用戶端緩衝區的檢查碼後發出通知（每個工作另外睡 20 ms，代表真正的工作）。`./evserver demo` 讓一系列腳本化的用戶端對它執行：

| 用戶端 | 做什麼 | 伺服器的紀錄（兩次執行） |
| --- | --- | --- |
| `good` | 一個封存的 64 KiB 緩衝區、一個 PING | 接受，約 20 ms 後回傳檢查碼，PONG |
| `unsealed` | 一個未封存的緩衝區，再一個宣稱兩倍大小的封存緩衝區 | 兩次都拒絕，並附上理由 |
| `foreign` | 切換到 uid 65534 後再連線 | `connect: Permission denied`：socket 所在目錄是 `0700`，伺服器根本沒看到它 |
| `stall` | 三個緩衝區，然後要求伺服器阻塞 100 ms | `timerfd read returned 6 expirations: 5 vblank tick(s) went by unseen`；`eventfd read 3 ... 3 job(s) done`；PONG 比三個 OK 先到 |
| `crash` | 送出一個緩衝區後立刻結束 | `hung up`，20 ms 後 `client 4 left before its job finished; result dropped` |
| `fdflood` | 200 個 PING，每個附上 2 個 fd | 200 個 PONG；之後在 `SIGUSR1` 時：`open fds 410 (at start 10)` |

接著 `SIGUSR1` 透過 `signalfd` 印出統計，`SIGTERM` 讓伺服器正常關閉。

執行之前先**預測**：在 `stall` 的情況中，下一次 `timerfd` 讀取會回傳幾次到期？`eventfd` 讀取會回傳多少？用戶端會以什麼順序收到四個回覆？

輸出中的 `strace` 一段，展示了 `good` 用戶端在就緒模型下的運作。第一次執行，經過縮短，`#` 後面是註解：

```
73    epoll_wait(3, [{events=EPOLLIN, data={...}}], 32, -1) = 1          # fd 10 is readable
73    recvmsg(10, {... msg_iov=[{iov_base="\2\0\0\0\0\0\1\0..." ...       # the BUFFER message
73    write(1, "[  147.7 ms] server: client 1 bu"..., 107) = 107
73    recvmsg(10, {msg_namelen=0}, MSG_CMSG_CLOEXEC) = -1 EAGAIN          # nothing more from it
73    epoll_wait(3, [{events=EPOLLIN, data={...}}], 32, -1) = 1          # the timerfd
73    read(8, "\1\0\0\0\0\0\0\0", 8)    = 8                                # one expiration
73    epoll_wait(3,  <unfinished ...>
74    write(9, "\1\0\0\0\0\0\0\0", 8 <unfinished ...>                      # the worker posts the eventfd
73    <... epoll_wait resumed>[{events=EPOLLIN, data={...}}], 32, -1) = 1
74    <... write resumed>)              = 8
73    read(9, "\1\0\0\0\0\0\0\0", 8)    = 8                                # the loop reads it
73    write(1, "[  168.8 ms] server: eventfd rea"..., 86) = 86
73    sendto(10, "e\0\0\0\0\200 ochecksum 6f208000\0\0\0\0\0\0\0"..., 64, MSG_DONTWAIT|MSG_NOSIGNAL, NULL, 0) = 64
-- the control data of the BUFFER message, which those lines cut off:
   msg_control=[{cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, cmsg_data=[11]}]
```

`epoll_wait` 只說了「fd 10 可讀」；資料是在迴圈呼叫 `recvmsg` 時才搬過來的，用戶端的 `memfd` 也在那時被安裝成伺服器的 fd 11。執行緒 74 從沒碰過 socket，它只是寫了 8 個位元組到 `eventfd`，迴圈執行緒的 `epoll_wait` 就因此返回。

這說明了：

- **一個阻塞的回呼會阻塞一切。** 在 100 ms 的阻塞期間，其他東西都沒有執行：沒有讀計時器、沒有服務其他用戶端、沒有收工作執行緒的結果。之後的 `timerfd` 讀取數到 6 次到期，所以伺服器確切知道錯過了幾次 vblank。Windows 的可等待計時器沒有這樣的計數（練習 G）。
- **`eventfd` 是一個計數器。** 迴圈阻塞期間的三次通知，變成一次喚醒、一次讀到 3，所以迴圈每次醒來都必須把佇列處理完，不能期待每個項目各有一次喚醒。Windows 的自動重設事件則完全失去這個計數（練習 G）。
- **工作執行緒的結果可能比用戶端活得久。** `crash` 用戶端的工作在用戶端離開後才完成。伺服器依 id 查找用戶端，丟棄結果，而不是寫入一個已釋放的結構。
- **訊號變成資料。** `SIGTERM` 以帶有傳送者 pid 的 `signalfd_siginfo` 抵達，在迴圈中、夾在另外兩個事件之間，而不是在任意一條指令處打斷迴圈的處理常式中。訊號在工作執行緒啟動前就被封鎖，所以它繼承了這個遮罩（signalfd(2) 要求這些訊號被封鎖）。
- 第一次執行中，`fdflood` 用戶端期間也有一次 `timerfd` 讀取回傳 2；第二次執行沒有，修好的伺服器在兩次執行中都沒有。這一點沒有深入調查。一個可以驗證的假設：洩漏中的伺服器在 `fdflood` 期間，fd 表從 64 個項目長到 512 個，而擴大多執行緒行程的 fd 表可能要等待其他 CPU。可以用 `strace -T` 檢查，或把工作執行緒改成另一個行程再試。

**修好這個洩漏（請自己寫）。** `fdflood` 之後，伺服器持有的 fd 比開始時多了 400 個：每則訊息附帶的 fd 都會被安裝進伺服器的 fd 表，而沒有人關閉 PING 附帶的那些。用戶端可以用這種方式耗盡伺服器的 fd 上限，讓它再也無法接受任何人。修改 `evserver.c`，讓整個示範結束後，關閉時的開啟 fd 數等於開始時的數目；並決定一個送出不該送的 fd 的用戶端應得什麼對待（關掉那些 fd，或斷開用戶端；Wayland 把格式錯誤的請求視為協定錯誤，結束該用戶端）。存成 `evserver-fdleak.patch`，執行 `SOLUTION_DIR=... sh linux/run.sh`。不隨附的參考修正會關閉每一個訊息類型用不到的 fd，並計數：

```
[  379.2 ms] server: client 5 sent 2 fd(s) that message type 3 does not take; closing them (and any later ones)
[  394.1 ms] server: client 5 had sent 400 fd(s) without a use; all were closed
[  444.7 ms] server: statistics: 0.44 s, 26 vblank ticks (27 expected), 1 late timer reads, at most 6 expirations in one read; 0 client(s); open fds 10 (at start 10)
```

**把它弄壞（此處未執行）：** 刪掉 `evloop.c` 計時器分支中的 `read()`，預測 `epoll_wait` 從此會怎樣，先在水平觸發下、再在 `EPOLLET` 下。接著從 `evloop_add_signal()` 拿掉 `pthread_sigmask`，再送出 `SIGUSR1`。

### D. 就緒與完成（Linux）

[`linux/readiness.c`](linux/readiness.c)，觀察結果兩次執行相同：

```
  level-triggered (EPOLLIN):
    wrote 10 bytes                 -> epoll_wait reports 1 event(s)
    read 4, 6 left unread          -> epoll_wait reports 1 event(s)
    read until EAGAIN (6 bytes)    -> epoll_wait reports 0 event(s)
  edge-triggered (EPOLLIN | EPOLLET):
    wrote 10 bytes                 -> epoll_wait reports 1 event(s)
    read 4, 6 left unread          -> epoll_wait reports 0 event(s)
    wrote 1 more byte              -> epoll_wait reports 1 event(s)
    read until EAGAIN (7 bytes)    -> epoll_wait reports 0 event(s)

  SO_SNDBUF 212992 bytes; 278 messages sent before send() failed with EAGAIN
  the sender's EPOLLOUT now: 0 event(s)
  the peer had to read 209 of the 278 messages before EPOLLOUT came back

  poll() on a regular file: 1 ready, revents 0x1 (POLLIN is 0x1)
  epoll_ctl(EPOLL_CTL_ADD) on it: -1, EPERM (Operation not permitted)

  io_uring_setup(4 entries) -> -1, ENOSYS (Function not implemented)
  /proc/sys/kernel/io_uring_disabled: 0
  this process's Seccomp mode: 2 (a seccomp filter is installed)
```

- **就緒**（`epoll`、`poll`、`select`）：核心告訴你某個操作現在不會阻塞；接著由你執行它，資料在那一刻才被複製，複製到你當時選擇的緩衝區。水平觸發回報的是狀態（還有未讀的位元組），所以回呼可以只讀一點、之後再回來。邊緣觸發回報的是變化（有新位元組到達），所以沒有一直讀到 `EAGAIN` 的回呼，可能會等待永遠不會再來的資料。
- **可寫性就是背壓。** 278 則 64 位元組的訊息就塞滿了 208 KiB 的傳送緩衝區（每則訊息被計入的量都超過它的 64 位元組），而對方要讀掉其中 209 則之後，`EPOLLOUT` 才再出現。想寫給一個不讀資料的用戶端的伺服器，必須排入佇列、等待 `EPOLLOUT`，並決定何時放棄：這就是階段 09 中「永遠不讀自己 socket」的用戶端。
- **就緒無法描述磁碟。** `poll()` 認為一般檔案永遠可讀，`epoll` 則以 `EPERM` 拒絕它；epoll_ctl(2) 把這個錯誤列給不支援 `epoll` 的 fd，一般檔案就是其中之一。必須等待磁碟的讀取仍然會阻塞。
- **完成**（IOCP、重疊 I/O、`io_uring`）：你事先把整個操作連同緩衝區交給核心；完成時核心通知你，資料已經在你的緩衝區裡。練習 G 與 H 在 Windows 上展示這一點。代價是操作進行期間緩衝區都被占用，取消也變成一套自己的協定；好處是檔案、socket 與計時器都納入同一個模型。
- **這裡沒有執行 `io_uring`。** 核心以 `CONFIG_IO_URING=y` 建置，`io_uring_disabled` 為 0，但在容器中 `io_uring_setup` 回傳 `ENOSYS`，這符合 podman 預設 seccomp 設定檔拒絕該呼叫的情況。在你能掌控的 Linux 機器上，`./readiness` 會回報 ring 的大小；提交一個讀取操作請參考 io_uring(7)。

### E. 具名管道伺服器（Windows）

```powershell
.\namedpipe.exe
```

[`windows/namedpipe.c`](windows/namedpipe.c) 以訊息模式建立 `\\.\pipe\lab08-namedpipe-<pid>`，帶有 `FILE_FLAG_FIRST_PIPE_INSTANCE` 與 `PIPE_REJECT_REMOTE_CLIENTS`，並啟動自己的複本作為用戶端。用戶端以 `SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION` 開啟管道，所以伺服器可以讀它的 token，但不能以它的身分行事。觀察結果，取自第一次執行（第二次只有 PID 不同），經過縮短：

```
  server: GetNamedPipeClientProcessId 23136 (matches CreateProcess), session 1 (mine 1)
  server: first message: "hello from the client" (21 bytes)
  server: impersonated the client at level SecurityIdentification and took its token
  client's token: user <machine>\<user> (S-1-5-21-<domain>-1001), integrity medium (0x2000)
  server's token: user <machine>\<user> (S-1-5-21-<domain>-1001), integrity medium (0x2000)
  server: same user: yes -> accept

  server: next message is 100 bytes (3100 bytes in the pipe in total)
    ReadFile into 64 bytes: 1 x (64 bytes, FALSE, ERROR_MORE_DATA), then 36 bytes, TRUE: end of message
  server: next message is 3000 bytes (3000 bytes in the pipe in total)
    ReadFile into 64 bytes: 46 x (64 bytes, FALSE, ERROR_MORE_DATA), then 56 bytes, TRUE: end of message

  a second instance with FILE_FLAG_FIRST_PIPE_INSTANCE: failed, error 5
  a second instance without the flag:           created (the flag guards only the first instance)
  client: pid 23136; GetNamedPipeServerProcessId says the server is pid 26824
```

- **對方是誰？** `GetNamedPipeClientProcessId` 對應 `SO_PEERCRED` 的 pid，用戶端的 token 則提供 Linux 上 uid 所提供的資訊，外加完整性層級。伺服器在模擬期間取得 token handle，然後立刻還原身分。這個程式的第一版在仍以識別層級模擬時查詢帳號名稱，查詢失敗了（印出 `?\?`）。
- **訊息保留邊界。** 在訊息模式下，緩衝區太小時會得到 `ERROR_MORE_DATA`，下一次讀取拿到同一則訊息的其餘部分，絕不會是下一則訊息的開頭。`PeekNamedPipe` 可以先告訴你大小。
- **名稱搶占。** 先建立管道名稱的行程擁有它。`FILE_FLAG_FIRST_PIPE_INSTANCE` 讓伺服器直接失敗（錯誤 5），而不是悄悄成為別人管道的第二個實例；用戶端則以 `GetNamedPipeServerProcessId` 檢查另一個方向。到 Learn 的 CreateNamedPipe 頁面讀讀預設安全描述元允許其他帳號做什麼，並決定你的伺服器應改傳哪一個 DACL。

### F. 以 `DuplicateHandle` 把 section 交給用戶端（Windows）

```powershell
.\dupsection.exe          # 無人值守
.\dupsection.exe hold     # 在第 3 部分後停下，供 Process Explorer 檢視；按 Enter 繼續
```

[`windows/dupsection.c`](windows/dupsection.c) 建立一個沒有名稱、以分頁檔為後盾的 64 KiB section。沒有名稱，其他行程就打不開它：唯一的途徑是伺服器發出的 handle。伺服器從管道得知用戶端的 PID，只以 `PROCESS_DUP_HANDLE` 開啟用戶端行程，並推送兩個 handle 給它，一個是 `FILE_MAP_READ`，一個是 `FILE_MAP_READ | FILE_MAP_WRITE`。handle 的數值經由管道傳過去；在其他行程中它們沒有任何意義。`NtQueryObject` 印出 Process Explorer 的 handle 檢視會顯示的資訊。觀察結果，取自第二次執行，經過縮短：

```
  server: section handle               handle 0x00f8  type Section  access 0x000f0007  handle count 1
  server: DuplicateHandle into pid 19692 -> 0x00fc (FILE_MAP_READ), 0x0100 (FILE_MAP_READ | FILE_MAP_WRITE); valid only in that process
  server: section handle               handle 0x00f8  type Section  access 0x000f0007  handle count 3
  client: read-only handle             handle 0x00fc  type Section  access 0x00000004  handle count 3
  client: read-write handle            handle 0x0100  type Section  access 0x00000006  handle count 3
  client: read-only view at 000001E6C86E0000, 65536 bytes, MEM_MAPPED: "page 0, written by the server"
  client: MapViewOfFile(read-only handle, FILE_MAP_WRITE) -> NULL, error 5
  client: DuplicateHandle(read-only handle, FILE_MAP_ALL_ACCESS) -> TRUE, error 0
  client: upgraded copy                handle 0x0108  type Section  access 0x000f001f  handle count 4
  client: MapViewOfFile(upgraded copy, FILE_MAP_WRITE) -> a writable view
  client: locked-down section handle   handle 0x0104  type Section  access 0x00000004  handle count 2
  client: DuplicateHandle(locked-down handle, FILE_MAP_ALL_ACCESS) -> FALSE, error 5
  server: page 1 of my view now reads "page 1, written by the client"
  server: pulled the client's 0x00fc back as 0x0150; CompareObjectHandles with my section: TRUE; with another section: FALSE
  client: closed my handles too; my view still reads "page 0, written by the server" and "page 1, written by the client"
  client: server's pseudo-handle, copied handle 0x0110  type Process  access 0x001fffff  handle count 9
```

完整輸出在[輸出紀錄](windows/example-output.txt)中；兩次執行結果相同。

- **一個物件、三個 handle、兩個行程。** 伺服器推送兩個 handle 後，handle 計數從 1 增加到 3；對一個從用戶端拉回來的 handle 呼叫 `CompareObjectHandles`，確認它是同一個 section。用戶端的視圖和伺服器的視圖是同一組頁面，只是位址不同，和實驗 03 一樣。
- **handle 的存取遮罩限制的是那個 handle，不是物件。** 唯讀 handle 無法映射成可寫（錯誤 5）。但用戶端向 `DuplicateHandle` 要求一個帶 `FILE_MAP_ALL_ACCESS` 的複本，卻拿到了，還映射成可寫。這符合以下解釋：要求的存取權超過來源 handle 所有時，系統會依 section 的安全描述元做存取檢查；建立時沒有指定安全描述元的物件，其預設描述元允許建立者的使用者做任何事，而用戶端正是以同一個使用者執行。第二個 section 以只授予此使用者 `SECTION_QUERY | SECTION_MAP_READ`（0x5）的 DACL 建立，就拒絕了同樣的升級。這是練習 A 中 `/proc/self/fd` 重新開啟在 NT 上的雙胞胎。真正的限制需要一個會拒絕的描述元，或讓用戶端以另一個使用者、或以較低的完整性層級執行。
- **生命週期。** 伺服器解除自己的視圖並關閉 handle、用戶端也關閉自己的 handle 之後，用戶端的視圖仍然可用：映射中的視圖讓 section 繼續存在，就像 `mmap` 讓 `memfd` 的頁面繼續存在。
- **推送，不要拉取。** 用戶端以 `PROCESS_DUP_HANDLE` 開啟伺服器（同一個使用者，所以被允許），並把伺服器自己的虛擬 handle `(HANDLE)-1` 複製到自己這邊。它得到一個指向伺服器行程、存取權為 `0x1fffff`（全部）的 handle。讓別的行程對你持有 `PROCESS_DUP_HANDLE`，就等於把你的一切交給它，所以應由伺服器開啟用戶端並推送，絕不反過來。
- **NT 沒有封存。** Win32 沒有縮小 section 的呼叫，所以練習 B 的 `SIGBUS` 攻擊在這裡沒有直接的對應，實驗 03 也展示過對已映射檔案的 `SetEndOfFile` 會被拒絕。但建立者保有寫入權，而且沒有類似 `F_SEAL_WRITE` 的東西：不能接受位元組改變的伺服器必須先複製它們。

**Process Explorer（此處未執行）：** 執行 `dupsection.exe hold`。在 Process Explorer 中選 View > Lower Pane View > Handles，先選伺服器、再選用戶端，找出 `Section` 類型的 handle（它們沒有名稱）。把它們的 handle 值與存取遮罩和程式印出的比對，並確認計數一致。

### G. 一條 GUI 執行緒同時等待 handle 與訊息（Windows）

```powershell
.\msgwait.exe
```

[`windows/msgwait.c`](windows/msgwait.c) 在新的行程群組中啟動 `msgwait.exe loop`，連到它的管道並寫入三則訊息，然後以 `GenerateConsoleCtrlEvent` 送給它 `CTRL_BREAK_EVENT`。迴圈行程擁有一個僅訊息視窗（`HWND_MESSAGE`，從不顯示），它唯一的執行緒呼叫 `MsgWaitForMultipleObjectsEx`，等待的對象有：重疊管道操作的事件、每 16 ms 一次的高解析度可等待計時器、主控台控制處理常式設定的事件、工作執行緒設定的事件、它的訊息佇列（`QS_ALLINPUT`），以及 APC（`MWMO_ALERTABLE`）。觀察結果：

| 第 1 部分，單一執行緒 | 第一次執行 | 第二次執行 |
| --- | --- | --- |
| 對自動重設事件 `SetEvent` ×3，再等待直到逾時 | 喚醒 1 次 | 1 |
| `ReleaseSemaphore(3)`，再等待直到逾時 | 喚醒 3 次 | 3 |
| `PostMessage` ×3，再以 `PeekMessage` 取到空 | 3 則訊息 | 3 |
| 佇列中有 2 則訊息且已用 `PeekMessage(PM_NOREMOVE)` 看過；`MsgWaitForMultipleObjectsEx(..., 200 ms, QS_ALLINPUT, 0)` | 201 ms 後 `WAIT_TIMEOUT` | 210 ms 後 |
| 同上，但加上 `MWMO_INPUTAVAILABLE` | 立刻返回 | 立刻 |

```
[  221.0 ms] pipe: a client connected (pid 26636)
[  329.9 ms] event: the worker's event is set
[  330.3 ms] message: WM_APP+1 from the worker, job 1
[  330.5 ms] APC: job 1's callback ran on thread 28340 inside the alertable wait
...
[  734.7 ms] pipe: overlapped ReadFile completed, 13 bytes already in my buffer: "first message"
...
[  923.2 ms] pipe: the client went away (error 109); waiting for the next one
[ 1126.5 ms] console control: CTRL_BREAK_EVENT, delivered by the handler on thread 7328; this is thread 28340
[ 1127.6 ms] message: WM_QUIT; leaving the loop

  in 0.92 s: 135 waits; waitable-timer wakeups 56 (58 expected at 16 ms); WM_TIMER 59 (92 asked for at 10 ms); worker: 3 event wakeup(s), 3 message(s), 3 APC(s)
```

- **一次等待、四種機制。** 核心物件（事件、計時器）、執行緒的訊息佇列與 APC，都結束了同一次等待。管道的事件是在讀取「完成」時觸發，位元組已經在緩衝區裡：在一個看起來像 `epoll` 的迴圈中，Windows 的重疊 I/O 是完成模型。
- **喚醒如何計數。** 自動重設事件是一個旗標：等待之前的三次 `SetEvent` 只帶來一次喚醒，比 `eventfd` 還少，後者至少會回傳總和。號誌會計數，投遞的訊息則逐一排隊。
- **「新輸入」規則。** 執行緒看過自己的佇列之後，沒有 `MWMO_INPUTAVAILABLE` 的 `MsgWaitForMultipleObjectsEx` 睡滿了 200 ms，即使有兩則訊息在等：它等待的是上次查看之後的新輸入。`GetQueueStatus` 顯示了這一點：`new 0x0000, present 0x0008`。沒有把佇列取空、或沒有傳入 `MWMO_INPUTAVAILABLE` 的迴圈，就會這樣卡住。
- **訊號在另一條執行緒上抵達。** 主控台控制處理常式在系統建立的執行緒 7328 上執行，不是在 GUI 執行緒上；它只設定一個事件。這就是 Windows 版的 `signalfd`：把非同步通知變成迴圈可以等待的東西。
- **計時器。** 可等待計時器醒了 56 次，而這段時間可容納 58 個 16 ms 週期。和 `timerfd` 不同，它不會告訴你自上次等待以來過了幾個週期；需要時請以 `QueryPerformanceCounter` 比對。以 10 ms 呼叫的 `SetTimer` 在 0.92 s 內送來 59 則 `WM_TIMER`，約每 15.6 ms 一則。這符合實驗 06 的一般計時器與 15.6 ms 的 tick；tick 本身這裡沒有量測。

### H. 選做：I/O completion port（Windows）

```powershell
.\iocp.exe
```

[`windows/iocp.c`](windows/iocp.c) 開啟同一個管道的三個重疊實例，各自關聯到同一個 completion port（實例編號就是完成鍵），對每一個開始一次連線，之後就只呼叫 `GetQueuedCompletionStatusEx`。執行緒集區的計時器每 16 ms 以 `PostQueuedCompletionStatus` 投遞一個「tick」封包，最後一個用戶端離開時，伺服器投遞「quit」給自己。觀察結果：

```
[   29.4 ms] instance 0: connect completed, client pid 15704
[   29.8 ms] instance 0: read completed, 19 bytes already in the buffer: "client 0, message 1"
...
[  150.0 ms] instance 0: read completed with error 109: client pid 15704 is gone after 4 messages
...
  in 0.29 s: 36 GetQueuedCompletionStatusEx calls returned 49 entries (at most 3 at once); 18 timer packets (18 expected at 16 ms); clients that got every reply: 3 of 3
```

第二次執行：32 次呼叫、48 個項目、一次最多 3 個、17 個計時器封包；3 個用戶端中 3 個收齊回覆。每個項目都是一個已完成、附有位元組數的操作。從來沒有人問「這個管道可讀嗎？」，而計時器只是同一個佇列上的另一個封包。**解釋**如果有四條執行緒對這個 port 呼叫 `GetQueuedCompletionStatusEx` 會有什麼改變（每個實例的狀態機都假設一次只有一個操作），以及 port 的並行值 1 起了什麼作用。

### I. 閱讀 libwayland 的事件迴圈（此處未執行）

本草稿沒有 libwayland 原始碼可讀。請在 [wayland 儲存庫](https://gitlab.freedesktop.org/wayland/wayland)中、以一個你記錄在 `NOTES.md` 的發行標籤閱讀：

| 檔案 | 要找什麼 |
| --- | --- |
| `src/wayland-server-core.h` | `wl_event_loop` API：`wl_event_loop_add_fd`、`_add_timer`、`_add_signal`、`_add_idle`、`wl_event_loop_dispatch`、`wl_event_loop_get_fd`，以及 `WL_EVENT_READABLE`／`WL_EVENT_WRITABLE` 遮罩 |
| `src/event-loop.c` | `wl_event_loop_dispatch` 在什麼裡面睡眠；每種來源背後是哪一種核心物件；水平還是邊緣觸發；某個來源的事件仍在本批次中時被移除會怎樣；計時器如何保存 |
| `src/wayland-server.c` | `wl_display_run`，以及用戶端 socket 的回呼（`wl_client_connection_data`） |
| `src/connection.c` | `wl_connection_read` 與 `wl_connection_flush`：在 `SOCK_STREAM` socket 上，位元組與 `SCM_RIGHTS` 的 fd 如何保持對應 |
| `src/wayland-os.c` | 包裝 `socket`、`recvmsg` 與 `epoll_create` 的 close-on-exec 函式 |
| `src/wayland-shm.c` | 用戶端縮小它的 `wl_shm` pool 時合成器怎麼做，以及它是否會看封存 |
| `src/wayland-client.c` | 用戶端為何要透過 `wl_display_prepare_read` 與 `wl_display_read_events` 讀取 |

以檔案與函式名稱回答：

1. `wl_event_loop` 使用哪一種模型，就緒還是完成？引用那個系統呼叫，以及呼叫來源回呼的那一行，並說明回呼收到的是「fd 已就緒」，還是「這是位元組」。
2. 把每一種來源和 [`evloop.c`](linux/evloop.c) 比較。計時器來源與訊號來源背後是哪一種核心物件？十個計時器會建立幾個 timer fd？
3. 它如何讓在回呼中移除來源變得安全？和 `evloop.c` 的 dead 串列比較。
4. `connection.c` 如何把收到的 fd 配給需要它們的訊息？和練習 A 的 `SOCK_STREAM` 結果比較。
5. `wayland-shm.c` 如何處理 `SIGBUS`？它會不會對封存的 pool 另眼看待？和練習 B 的兩個接收端比較。

本草稿對第 1 題的預期答案，是根據 libwayland 的公開 API 與文件寫成，**沒有在這裡對照原始碼確認**：就緒。`wl_event_loop_add_fd` 接受可讀、可寫條件的遮罩，並以已就緒的遮罩呼叫回呼，然後由回呼自己讀寫該 fd；`wl_event_loop_get_fd` 回傳一個在任何來源就緒時變成可讀的 fd，這正是 epoll 的模式。請自行確認：在 `event-loop.c` 中找到 `epoll_wait` 呼叫，再從 `wl_client_connection_data` 追蹤一個用戶端請求到 `connection.c` 中的 `recvmsg`。

## Linux／NT 對照

- **交出權能。** Linux：傳送端把 fd 放進 `SCM_RIGHTS` 訊息，接收端呼叫 `recvmsg` 時，核心在接收端安裝新的 fd。NT：對另一個行程持有 `PROCESS_DUP_HANDLE` 的行程，以 `DuplicateHandle` 把 handle 推進去，之後還要告訴對方它的數值。兩者都帶著權利：fd 的存取模式、handle 的存取遮罩。兩者也都能在物件允許時被擴大：透過 `/proc/self/fd` 重新開啟、再一次 `DuplicateHandle`（練習 A 與 F）。
- **對方是誰。** `SO_PEERCRED` 提供 `connect()` 當時的 pid、uid 與 gid，`SO_PEERPIDFD` 提供一個 pidfd。`GetNamedPipeClientProcessId` 提供 pid，模擬則提供用戶端的 token：使用者、群組、完整性層級（練習 A 與 E）。
- **大小與內容。** `memfd` 可以封存，禁止縮小、變大與寫入（練習 B）。section 的大小固定，Win32 沒有縮小它的呼叫，但沒有寫入封存：建立者隨時可以改它。
- **等待。** `epoll` 等待 fd 的就緒，`signalfd`、`timerfd` 與 `eventfd` 把訊號、時間與喚醒變成 fd。`MsgWaitForMultipleObjectsEx` 最多等待 63 個 handle，再加上執行緒的訊息佇列與 APC。視窗訊息屬於建立該視窗的執行緒（[About Messages and Message Queues](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)），所以必須由 GUI 執行緒自己等待。I/O completion port 只等待完成，其他一切都以封包的形式投遞給它（練習 C、G 與 H）。
- **計時器。** `timerfd` 的讀取會回傳到期次數；可等待計時器只有「已觸發」或「未觸發」。

## 檢核

在 `NOTES.md` 中回答。

1. **證明封存後的緩衝區無法被傳送端縮小**，用你自己執行的練習 B 第 2 部分。接著展示接受了未封存緩衝區的接收端會怎樣（第 3 部分），並說明合成器對用戶端的像素緩衝區、以及對它發出的 keymap，各應要求哪些封存。
2. **以證據解釋就緒與完成的差異：** 練習 D 的水平與邊緣觸發結果和一般檔案、練習 C 的 `strace`（先 `epoll_wait` 再 `recvmsg`），以及練習 G 或 H 中位元組已在緩衝區裡的重疊讀取。指出每種模型的代價：操作進行期間由誰擁有緩衝區，取消時會發生什麼事。
3. **閱讀 libwayland 的事件迴圈，指出它採用哪一種模型。** 以檔案與函式名稱回答練習 I 的五個問題，並註明你讀的標籤。
4. 傳遞 fd 或 handle 就是傳遞權能。說明什麼會跟著過去（開啟檔案描述、存取模式或遮罩）、什麼不會（名稱）、Linux 與 NT 在哪裡讓接收端擴大它，以及封存或 DACL 如何堵住這一點。
5. 為什麼 `SO_PEERCRED` 回報的是 `connect()` 當時的身分？這對檢查對方身分的伺服器意味著什麼？
6. 修好練習 C 的洩漏，證明關閉時的開啟 fd 數等於開始時的數目。解釋為何伺服器無法拒絕用戶端送來的 fd，只能關掉它們。
7. 根據練習 G：為什麼三次 `SetEvent` 只帶來一次喚醒，而三次 `PostMessage` 帶來三則訊息？`MsgWaitForMultipleObjectsEx` 在什麼情況下會在有訊息等待時仍然睡眠？

## 常見失敗情況

- **只送 fd、不帶資料。** unix(7) 要求在 `SOCK_STREAM` socket 上傳送輔助資料時至少帶一個位元組的實際資料。`fdpass_send()` 會以 `EINVAL` 拒絕空的內容。
- **`F_ADD_SEALS` 以 `EPERM` 失敗。** 該 `memfd` 建立時沒有 `MFD_ALLOW_SEALING`（一開始就帶有 `F_SEAL_SEAL`），或已經設定了 `F_SEAL_SEAL`。
- **以 `F_SEAL_WRITE` 呼叫 `F_ADD_SEALS` 時以 `EBUSY` 失敗。** 仍然存在可寫的共享映射；先解除它，或改用 `F_SEAL_FUTURE_WRITE`。
- **接收端因 `SIGBUS` 而死。** 它檢查了大小，沒有檢查封存（練習 B 第 3 部分）。
- **開啟的 fd 數持續增加。** 某種訊息類型附帶的 fd 沒有人關閉。在前後各數一次 `/proc/self/fd` 中的項目（`fdpass_count_open()`）。
- **`SIGTERM` 直接殺死伺服器，而不是經由 `signalfd` 抵達。** 這個訊號沒有在每一條執行緒中被封鎖。請在建立執行緒之前封鎖它，就像 `evloop_add_signal()` 在主執行緒中所做的。
- **迴圈以 100 % CPU 空轉。** 某個水平觸發的來源沒有被取空：`timerfd` 或 `eventfd` 沒有讀取，或用戶端 socket 中還有未讀的訊息。
- **邊緣觸發的迴圈再也聽不到某個用戶端。** 回呼沒有一直讀到 `EAGAIN`。
- **tick 不見了。** 有回呼阻塞了。`timerfd` 的到期次數會告訴你少了幾個。
- **對 `strace` 下 `kill` 沒有作用。** `strace -o FILE PROG` 預設會封鎖致命訊號；本草稿最早的 `run.sh` 就是卡在這裡。改為送訊號給被追蹤的程式。
- **在容器中 `io_uring_setup` 回傳 `ENOSYS`**，即使核心支援它。容器的 seccomp 設定檔拒絕了它。
- **Windows：模擬期間查詢帳號失敗。** 在識別層級下，執行緒無法以用戶端的身分行事；先取得 token，`RevertToSelf()`，再查詢。
- **Windows：佇列中有訊息，`MsgWaitForMultipleObjectsEx` 卻在睡。** 傳入 `MWMO_INPUTAVAILABLE`，或每次都以 `PeekMessage` 把佇列取空。
- **Windows：直接使用另一個行程的 handle 數值。** handle 數值以行程為單位：同一個數字在這個行程中是無效的，更糟的是指向某個不相干的物件。只有被複製「進」這個行程的 handle 在這裡才有效。
- **Windows：`GenerateConsoleCtrlEvent` 送到了錯的行程。** 本草稿把 `CTRL_BREAK_EVENT` 送給以 `CREATE_NEW_PROCESS_GROUP` 啟動的行程，讓只有那個群組收到；嘗試 Ctrl+C 之前，請先讀 Learn 的 GenerateConsoleCtrlEvent 頁面（這裡沒有試）。

## 後續會用到的地方

- 階段 09 以 `fdpass`、`sealbuf` 與 `evloop` 建立它的無螢幕顯示伺服器，以 `timerfd` 模擬 vblank，並且必須撐過這裡勾勒出的用戶端：一個會當掉、一個永遠不讀（練習 D 的背壓）、一個送出未封存的緩衝區後再把它縮小。
- 階段 11 的 `wl_shm` pool 是共享記憶體的 fd，通常是 `memfd`，以 `SCM_RIGHTS` 傳遞；`wl_keyboard` 的 keymap 也以 fd 的形式抵達。
- 階段 14 以同樣的方式傳遞 dma-buf 與 `sync_file` 的 fd，並等待 fence。
- 階段 17 把練習 B 與 C 變成攻擊：未封存的緩衝區、fd 洪水、請求洪水。

## 下一步

[建置計畫](../../SYLLABUS.zh-TW.md)的階段 09 是第一部分的專題：一個有自己協定的無螢幕顯示伺服器。
