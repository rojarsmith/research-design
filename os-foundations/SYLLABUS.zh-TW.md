[English](SYLLABUS.md) | **繁體中文** | [課程總覽](README.zh-TW.md)

# 實作建置計畫

> **草稿，待審閱（2026-11-15）。** 目前只有階段 00–10 有實驗草稿，放在 [`labs/`](labs/)；每份實驗 README 都註明哪些步驟有執行。其餘內容都尚未實作。階段、時數與工具皆屬提案。技術敘述附上查核時對照的官方頁面，完整清單見[來源](#來源)。

這是一條建置路線，而非主題清單。每個階段先列出**先備**、**時數**（專注時間，含閱讀）與**執行環境**，接著是**製作**、**需要時再學**，有助理解時加上 **Linux／NT 對照**，最後是**檢核**。與圖學課程相同：能解釋程式的行為才算完成，程式能跑還不夠。

文中「MIT 實驗」指公開的 [6.1810 2025 秋季實驗](https://pdos.csail.mit.edu/6.1810/2025/schedule.html)，可當作現成練習；下方的製作項目補上本課程額外需要的部分。

## 第一部分：作業系統基礎

### 00 — 工作環境：開機並除錯 xv6

**先備：** 會用指標與結構的 C、Linux shell、git 與 make，以及讀懂 RISC-V 組合語言，程度相當於 [`c-compiler-foundations` 學習路線](../c-compiler-foundations/research/04-integration-path.zh-TW.md)第 1 關。
**時數：** 4–8。**執行環境：** WSL 2，搭配 Ubuntu 24.04。**實驗草稿：** [`labs/00-workbench/`](labs/00-workbench/README.zh-TW.md)（已於 2026-11-16 在 Ubuntu 24.04 容器中執行；WSL 2 安裝步驟未執行）。

**製作：** 安裝 WSL 2，以及 MIT [工具頁](https://pdos.csail.mit.edu/6.1810/2025/tools.html)列出的套件：`git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu`。該頁要求 QEMU 7.2 以上、GDB 8.3 以上。複製 [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) 後執行 `make qemu`，再執行 `make qemu-gdb` 並接上 `gdb-multiarch`。從 `_entry` 經 `start()` 逐步追到 `main()`。所有工具版本都記入 `NOTES.md`。

**需要時再學：**
- QEMU 的 `virt` 機器，以及 Makefile 中的 `-machine virt -bios none -kernel … -m 128M -smp $(CPUS)`；`CPUS` 預設為 3。
- 機器、監督者與使用者三種模式。
- `start()` 如何用 `mret` 降到監督者模式。
- 核心的連結器腳本。

**檢核：** 說明每個 QEMU 參數的作用。指出 xv6 執行的第一道指令，以及離開機器模式的位置。再和微控制器從 reset 到 `main` 的過程比較（[`mcu-foundations`](../mcu-foundations/02-learning-path.md) 階段 1）：因為這台機器有特權等級，多出了哪些工作？

### 01 — 系統呼叫：跨越邊界

**先備：** 00。**時數：** 8–14。**執行環境：** WSL 2；NT 部分在 Windows 上做。**MIT 實驗：** `syscall`。**實驗草稿：** [`labs/01-system-calls/`](labs/01-system-calls/README.zh-TW.md)（Windows 部分已執行；xv6 與 Linux 部分已於 2026-11-16 在 Ubuntu 24.04 容器中執行）。

**製作：** 在 xv6 新增一個系統呼叫，從使用者端 stub（`usys.S` 的 `ecall`）開始追蹤，經 `trampoline.S` 的 `uservec`、`usertrap()`、`syscall()`，再從 `userret` 返回。接著從使用者空間傳入錯誤指標，證明 `copyin`／`copyout` 會拒絕它。在 Linux 上，先用 `strace` 執行同一個小程式，再改成直接呼叫 `syscall(2)` 執行一次。在 Windows 上，用 Win32 寫同樣的程式，再用除錯器找出底下的 `Nt*` 常式。

**需要時再學：**
- 陷阱相關的 CSR：`stvec`、`sepc`、`scause`、`sstatus`。
- trampoline 頁與 trap frame。
- 系統呼叫參數如何取得。
- 核心為何絕不能信任使用者指標。

**Linux／NT 對照：** NT 的系統服務有 Nt 與 Zw 兩種入口。從使用者模式呼叫時，參數一律視為不可信並經過探測；在核心模式中，常式依執行緒的 previous mode 判斷是否信任參數（[Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)）。有正式文件的介面是 Win32；Learn 把 `NtQueryInformationProcess` 這類原生呼叫描述為作業系統內部介面，各版本之間可能變動（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess)）。

**檢核：** 畫出 `write(1, buf, n)` 進入核心再返回的過程，標出每個保存的暫存器、每次切換的堆疊，以及每張用到的分頁表。說明 `buf` 指向核心記憶體時會發生什麼，並分別用 xv6 與 NT previous mode 的術語解釋。

### 02 — 行程、執行緒與情境切換

**先備：** 01。**時數：** 8–14。**執行環境：** WSL 2、Windows。**MIT 實驗：** `util`。**實驗草稿：** [`labs/02-processes-threads/`](labs/02-processes-threads/README.zh-TW.md)（Windows 部分已執行；xv6 與 Linux 部分已於 2026-11-16 在 Ubuntu 24.04 容器中執行）。

**製作：** 在 xv6 中用 `fork`、`exec`、`pipe`、`wait` 寫一個小管線，並追蹤 `fork()` 與 `swtch()`。接著為 xv6 寫使用者層級執行緒：一張執行緒表、一段自己用組合語言寫的情境切換，以及 `yield`。在 Linux 上用 `pthread_create` 建立執行緒，再用 `strace -f` 找出底下的 `clone` 旗標。

**需要時再學：**
- 行程狀態、行程表與核心堆疊。
- `swtch` 保存哪些暫存器，以及為何不必保存呼叫者保存（caller-saved）暫存器，也就是 [`c-compiler-foundations`](../c-compiler-foundations/README.zh-TW.md) 的呼叫慣例。
- xv6 每顆 CPU 各有的排程器執行緒。

**Linux／NT 對照：** Linux 的執行緒是以 `clone` 建立的 task，使用 `CLONE_VM`、`CLONE_THREAD` 等旗標（[clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)）。NT 把兩者分開：行程提供資源（位址空間、handle、安全性內容），執行緒才是被排程的單位（[Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)）。NT 沒有 `fork`，請比較 `CreateProcess` 與 `fork` 加 `exec`。

**檢核：** 列出 `swtch` 保存了什麼、存在哪裡。解釋 xv6 從一個行程切到另一個行程時，為何要先經過排程器執行緒。說明你的使用者層級切換有哪些東西不必保存。

### 03 — 虛擬記憶體

**先備：** 02。**時數：** 14–24。**執行環境：** WSL 2、Windows。**MIT 實驗：** `pgtbl`、`cow`。**實驗草稿：** [`labs/03-virtual-memory/`](labs/03-virtual-memory/README.zh-TW.md)（xv6、Linux 與 Windows 部分已執行；xv6 與 Linux 在 Ubuntu 24.04 容器中）。

**製作：** 印出並走訪 xv6 的分頁表，再實作寫入時複製（copy-on-write）的 `fork`，統計前後複製的分頁數。在 Linux 上，讓兩個行程映射同一個檔案，分別用 `MAP_SHARED` 與 `MAP_PRIVATE`，比較兩邊看到的內容；讀 `/proc/self/maps`；在映射存在時截短檔案，並攔下 `SIGBUS`。在 Windows 上，用 `VirtualAlloc` 保留與提交記憶體，並讓兩個行程共用一個以分頁檔為後盾的檔案映射。

**需要時再學：**
- Sv39 三層分頁表、PTE 位元、TLB 與 `sfence.vma`。
- xv6 的核心分頁表與使用者分頁表。
- 分頁錯誤處理與需求分頁。
- COW 分頁的參考計數。
- `mmap` 語意（[mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html)）。

**Linux／NT 對照：** Learn 說明檔案映射物件又稱為 section object（[Learn](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)）；在 NT 上，保留與提交是兩個步驟（[Learn](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory)）。在 Linux 上，存取超出檔案結尾的映射分頁會收到 `SIGBUS`。

**檢核：** 手算一個虛擬位址的轉換，再到 GDB 中確認。展示 COW 省下的分頁。解釋任何伺服器只要映射了用戶端交給它的記憶體，`SIGBUS` 這種情況就會構成安全問題；階段 17 會回到這一點。

### 04 — 中斷、計時器與裝置驅動程式

**先備：** 03。**時數：** 12–20。**執行環境：** xv6 在 WSL 2，核心模組在 Linux 虛擬機器，Windows 只做觀察。**MIT 實驗：** `traps`、`net`。**實驗草稿：** [`labs/04-interrupts-drivers/`](labs/04-interrupts-drivers/README.zh-TW.md)（xv6、Linux 使用者空間與 Windows 部分已執行；核心模組尚未執行）。

**製作：** 在 xv6 中追蹤一次 UART 中斷，從 PLIC 到 `devintr()`，再進入 UART 驅動；也追蹤迫使行程讓出 CPU 的計時器中斷。在 Linux 虛擬機器中寫一個小型核心模組，提供字元裝置，以及一個定期喚醒等待讀取者的核心計時器：每次 `read()` 都會阻塞到下一次 tick，再回傳序號與時間戳記。這是階段 12 要讀的 DRM 事件串流的縮小版。

**需要時再學：**
- 開關中斷與 PLIC。
- 上半部與延後處理。
- Linux 的執行緒化中斷處理（`request_threaded_irq`，[genericirq](https://docs.kernel.org/core-api/genericirq.html)）。
- Linux 以 sysfs 匯出的匯流排、裝置與驅動程式模型（[driver model](https://docs.kernel.org/driver-api/driver-model/overview.html)）。
- udev。

**Linux／NT 對照：**
- NT 的程式碼在某個 IRQL 下執行。ISR 要快速結束，並排入 DPC，在較低的 IRQL 完成其餘工作（[DPC](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-dpc-objects)）。
- 在 `DISPATCH_LEVEL` 以上，驅動程式不能等待；高於 `APC_LEVEL` 時發生分頁錯誤是致命錯誤（[IRQL](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities)）。
- 裝置位於裝置堆疊中：底層是 PDO，恰有一個功能驅動程式，以及可選的篩選驅動程式（[Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/device-nodes-and-device-stacks)）。
- WDM 已不再是建議的模型，新驅動程式改用 KMDF 或 UMDF（[WDM](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-wdm)、[WDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/)）。
- 載入測試簽章的 NT 驅動需要開啟 `TESTSIGNING`，可能還得關閉安全開機（[Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)）。因此所有 NT 驅動工作都只在虛擬機器中進行，不在主力桌機上做。

**檢核：** 分別就 xv6、Linux 與 NT，說明中斷情境中什麼可以睡眠、什麼不行，以及原因。展示你的模組在每次 tick 時喚醒阻塞的讀取者。

### 05 — 同步

**先備：** 04。**時數：** 12–20。**執行環境：** WSL 2、Windows。**MIT 實驗：** `lock`。**實驗草稿：** [`labs/05-synchronization/`](labs/05-synchronization/README.zh-TW.md)（xv6、Linux 與 Windows 部分已執行；ThreadSanitizer 與 GDB 步驟尚未執行）。

**製作：**
- 量測並降低 xv6 的鎖競爭。
- 讀 `acquire()`，說明 `push_off()` 的作用。
- 寫一組有缺陷、會遺失喚醒的 `sleep`／`wakeup`，再修好它。
- 在 Linux 上以 `futex(2)` 實作 mutex，並在競爭下與 `pthread_mutex` 比較。
- 在 Windows 上，分別用 SRW lock 與 `WaitOnAddress` 做同樣的測試。

**需要時再學：**
- C11 原子操作與記憶體順序，以及 RISC-V 的 `amoswap` 與 `fence` 指令。
- 自旋鎖與睡眠鎖，以及 xv6 的自旋鎖為何要關閉所在 CPU 的中斷。
- 鎖的取得順序與死結。
- Linux 的 lockdep（[lockdep](https://docs.kernel.org/locking/lockdep-design.html)），以及適合讀多寫少資料的 RCU（[RCU](https://docs.kernel.org/RCU/whatisRCU.html)）。

**Linux／NT 對照：**
- futex 讓沒有競爭的上鎖完全留在使用者空間，只有需要等待時才進入核心（[futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)）。
- NT 核心自旋鎖只能短暫持有，而且必須放在非分頁記憶體（[spin locks](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks)）；在 `DISPATCH_LEVEL` 以上不得等待 dispatcher 物件（[dispatcher objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-kernel-dispatcher-objects)）。
- `WaitOnAddress` 類似 futex，但喚醒者必須在同一個行程內（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)）；SRW lock 同樣只能在單一行程內使用（[Learn](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)）。

**檢核：** 用數字呈現改善前後的競爭程度。描述一個 lockdep 會回報的死結。說明 Windows 上的跨行程鎖為何不能用 `WaitOnAddress`。

### 06 — 排程與延遲

**先備：** 05。**時數：** 10–16。**執行環境：** WSL 2、Windows；硬體驗證會在 Linux 實體機上重新量測。**實驗草稿：** [`labs/06-scheduling-latency/`](labs/06-scheduling-latency/README.zh-TW.md)（xv6、Linux 與 Windows 部分已執行；WPR／WPA 錄製尚未執行）。

**製作：** 把 xv6 的輪轉迴圈換成優先權或彩票排程器，用一個吃 CPU 的工作與一個互動式工作測試。在 Linux 上，寫一個透過 `timerfd` 每秒喚醒 60 次的執行緒，在機器有負載時分別以預設排程策略與 `SCHED_FIFO` 執行，畫出每次喚醒的延遲。在 Windows 上以不同的執行緒優先權重複量測，並用 WPR 錄製一次執行、在 WPA 中檢視（[WPT](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/)）。

**需要時再學：**
- 計時器驅動的搶占、執行佇列，以及公平與延遲的取捨。
- Linux 的公平排程器：Linux 從 6.6 開始由 CFS 轉向 EEVDF（[EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)）。
- 即時排程類別，以及結合 EDF 與 CBS 的 `SCHED_DEADLINE`（[文件](https://docs.kernel.org/scheduler/sched-deadline.html)）。
- `sched_ext`：由 BPF 程式定義排程器，ABI 不保證穩定（[文件](https://docs.kernel.org/scheduler/sched-ext.html)）。

**Linux／NT 對照：** NT 依 0 到 31 的優先權排程執行緒，同一等級輪流執行，較高等級會搶占較低等級（[Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)）。基本優先權在 0 到 15 的執行緒，會因前景行程、視窗輸入或等待完成而獲得提升（[Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)）。

**檢核：** 畫出延遲分布圖，並解釋它的長尾。把它和顯示連起來：合成器醒得太晚就會錯過 vblank、掉一個影格，所以排程也是顯示品質的一部分。WSL 2 在虛擬機器中執行，數字只能相對比較。

### 07 — 檔案系統

**先備：** 03、05。**時數：** 12–20。**執行環境：** WSL 2、Windows。**MIT 實驗：** `fs`、`mmap`。**實驗草稿：** [`labs/07-file-systems/`](labs/07-file-systems/README.zh-TW.md)（xv6、Linux 與 Windows 部分已執行；xv6 與 Linux 在 Ubuntu 24.04 容器中；負責寫回的執行緒沒有追蹤）。

**製作：** 擴充 xv6 檔案系統，並實作以檔案為後盾的 `mmap`。在寫入途中強制結束 QEMU，證明下次開機時日誌能把檔案系統恢復到一致的狀態。

**需要時再學：**
- 緩衝快取與預寫式日誌。
- inode、目錄與路徑查找。
- 檔案描述子（fd）與開啟檔案描述（open file description）的差別。
- Linux 的 VFS 與 page cache。

**Linux／NT 對照：** NT 透過階段 03 的檔案映射物件提供以檔案為後盾的共享。分別找出兩個系統中，由誰決定被映射的髒頁何時寫回磁碟。

**檢核：** 逐步說明當機後的復原過程。解釋 fd 與它背後的開啟檔案描述有何不同；階段 08 在行程間傳遞的是後者。

### 08 — 行程間通訊、共享記憶體與事件迴圈

**先備：** 03、07。**時數：** 12–20。**執行環境：** WSL 2、Windows。**實驗草稿：** [`labs/08-ipc-event-loops/`](labs/08-ipc-event-loops/README.zh-TW.md)（Linux 與 Windows 部分已執行，Linux 在 Ubuntu 24.04 容器中；io_uring、Process Explorer 步驟與 libwayland 原始碼閱讀尚未執行）。

**在 Linux 上製作：**
1. 一個 Unix domain socket 伺服器，以 `SCM_RIGHTS` 接收 fd，並用 `SO_PEERCRED` 檢查對方身分（[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)）。
2. 以 `memfd_create` 加 `MFD_ALLOW_SEALING` 建立的共享緩衝區；只要緩衝區仍可能被縮小，接收端就拒收（[memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html)）。
3. 一個單執行緒的 `epoll` 迴圈，同時服務 socket、`timerfd`、`signalfd` 與 `eventfd`（[epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)、[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)、[signalfd(2)](https://man7.org/linux/man-pages/man2/signalfd.2.html)、[eventfd(2)](https://man7.org/linux/man-pages/man2/eventfd.2.html)）。

**在 Windows 上製作：**
1. 一個具名管道伺服器。
2. 一個以分頁檔為後盾的檔案映射，用 `DuplicateHandle` 交給用戶端（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle)、[named shared memory](https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory)）；再用 [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer) 在兩個行程中找到這個映射的 handle。
3. 一個 GUI 執行緒，用 `MsgWaitForMultipleObjectsEx` 同時等待 handle 與視窗訊息（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex)）。
4. 可選：I/O completion port 版本（[Learn](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)）。

**需要時再學：**
- pipe、socket 與共享記憶體的差別。
- 傳遞 fd 就是傳遞權能（capability）。
- 為何面對不可信的對方需要封存（sealing）：沒有封存時，對方可以隨時改寫記憶體，或把它縮小造成 `SIGBUS`。
- 就緒通知模型（`epoll`）與完成通知模型（IOCP、[io_uring](https://man7.org/linux/man-pages/man7/io_uring.7.html)）。
- Windows 每個執行緒各自的訊息佇列（[Learn](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)）。

**檢核：** 證明封存後的緩衝區無法被傳送端縮小。解釋就緒與完成兩種模型的差異。閱讀 libwayland 事件迴圈的原始碼，指出它採用哪一種模型。

### 09 — 第一部分專題：無螢幕顯示伺服器

**先備：** 06 與 08；若圖學課程的 T4 合成模組已完成就使用它，否則先用替代實作（stub）。**時數：** 20–35。**執行環境：** WSL 2。**實驗草稿：** [`labs/09-headless-display-server/`](labs/09-headless-display-server/README.zh-TW.md)（測試框架已於 2026-11-16 在 Ubuntu 24.04 容器中對不隨附的參考伺服器執行；設計說明與 Wayland 比較尚未執行）。

**製作：** 一個伺服器與至少兩個用戶端，透過 Unix socket 使用你自己設計的小協定溝通。用戶端建立封存的 memfd 緩衝區，以 `SCM_RIGHTS` 送出，再送出 attach、damage 與 commit 訊息。伺服器要做到：
- 把各用戶端的緩衝區合成到離屏影格緩衝區，使用圖學課程的合成模組，或一個混合預乘 ARGB8888 的替代實作；
- 用 `timerfd` 模擬 60 Hz 的 vblank；
- 對每個用戶端送出「影格完成」事件；
- 把影格寫成圖檔。

接著餵它有問題的用戶端：一個在影格中途當掉、一個永遠不讀自己的 socket、一個送出超出緩衝區範圍的 damage、一個送出未封存的緩衝區後再把它縮小。

**需要時再學：**
- 協定設計：物件、請求與事件、版本管理。
- 每個用戶端的狀態，以及背壓。
- 伺服器為何絕不能因為某個用戶端而阻塞。

**檢核：** 伺服器撐過所有問題用戶端，並維持影格節奏。寫一頁比較你的協定與 Wayland `wl_surface` 的 attach、damage、commit 與影格回呼（[協定](https://wayland.freedesktop.org/docs/html/apa.html)）。

## 第二部分：視窗與顯示系統

進入第二部分前，先完成圖學課程第一部到實驗 05。到階段 18 時，圖學課程[第二部草稿](../computer-graphics-practice/SYLLABUS-PART2.zh-TW.md)的「2D、文字與合成渲染路線」應已完成 T4（合成）；在那之前，由階段 09 的替代實作頂替。

**第二部分的執行環境。** 階段 10 與 11 在 Windows 上進行。階段 12–18 在 Linux 虛擬機器中進行：以 QEMU 的 virtio-gpu 作為顯示裝置，需要 overlay 平面或 writeback 時改用 VKMS。客體只能以軟體繪製。每個階段把虛擬機器看不到的部分標為**硬體檢核**，並集中到階段 18 之後的[硬體驗證](#硬體驗證)。各虛擬機器顯示驅動的比較見 README。

### 10 — 視窗系統架構：在 Windows 上量測

**先備：** 第一部分。**時數：** 8–12。**執行環境：** RTX 5070 Ti 桌機上的 Windows 11。**實驗草稿：** [`labs/10-windows-composition/`](labs/10-windows-composition/README.zh-TW.md)（視窗與被遮住兩種情況已於 2026-11-16 在另一台機器的遠端桌面工作階段中執行；無邊框執行、PresentMon 與 Win+Ctrl+Shift+B 尚未執行）。

**製作：** 一個使用 flip model swapchain 的小型 D3D11 或 D3D12 程式，或圖學課程的原生程式。用 [PresentMon](https://github.com/GameTechDev/PresentMon) 擷取三種情況：視窗模式、無邊框全螢幕，以及有其他視窗疊在上面時。記錄每次執行的 PresentMode，例如 `Composed: Flip`、`Hardware: Independent Flip`、`Hardware Composed: Independent Flip`。並排畫出 X11、Wayland 與 Windows 的呈現路徑。

**需要時再學：**
- **X11 與 Wayland。** 在 X 中，X 伺服器夾在用戶端與另一個獨立的合成器之間。在 Wayland 中，合成器本身就是顯示伺服器，掌管 KMS 與 evdev，用戶端則繪製到自己的緩衝區（[architecture](https://wayland.freedesktop.org/architecture.html)）。
- **Windows 的分層。** WDDM 分成三部分：由 Direct3D runtime 載入的廠商使用者模式驅動、廠商核心模式 miniport 驅動，以及 DirectX 圖形核心 `Dxgkrnl`（[WDDM 架構](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)）。
- **Flip model。** swapchain 的緩衝區與 DWM 共用，DWM 合成時不必複製（[flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)）。DWM 可以讓符合條件的視窗直接翻頁（Direct Flip、Independent Flip），或配給它硬體 overlay 平面（[DirectX blog](https://devblogs.microsoft.com/directx/dxgi-flip-model/)）。
- **Overlay 與重置。** 多平面 overlay 從 Windows 8.1 的 WDDM 1.3 開始提供（[MPO](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support)）。GPU 超過預設的兩秒逾時仍無回應時，由 TDR 重置（[TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery)）。
- **為何 DWM 無法取代：**
  - 從 Windows 8 起，DWM 已無法再以程式關閉（[DwmEnableComposition](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenablecomposition)）。
  - 自訂合成器唯一有官方文件的途徑是 `Windows.Devices.Display.Core`，而且只能處理 DWM 忽略的顯示器，也就是頭戴式顯示器與被標記為特殊用途的螢幕。這些顯示器會離開桌面，沒有 shell，也收不到滑鼠或觸控輸入（[namespace](https://learn.microsoft.com/en-us/uwp/api/windows.devices.display.core)、[specialized displays](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/specialized-monitors)）。
  - 間接顯示驅動（IddCx）用來加入遠端、虛擬或 USB 連接的螢幕，並從作業系統接收桌面影格（[IddCx](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview)）。它把畫面交給桌面合成器處理，而不是取代合成器。
  - 結論：在 Windows 上可以為專用顯示器寫合成器，但永遠無法替桌面寫。這就是本課程把合成器總結專題放在 Linux 的原因。

較舊的 [DWM 概觀](https://learn.microsoft.com/en-us/windows/win32/dwm/dwm-overview)頁面仍把 DWM 服務描述成可以開關；Windows 8 以後的說法以 API 頁面為準。

**檢核：** 根據 PresentMon 紀錄，說明哪些影格由 DWM 合成、哪些未經合成就到達螢幕，以及每次切換的原因。列出 Windows 開發者在這條路徑上能改與不能改的部分。再連到 [`display-wake-research`](../display-wake-research/README.zh-TW.md)：Win+Ctrl+Shift+B 重置圖形驅動時，重新啟動的是什麼？

### 11 — 從用戶端看 Wayland

**先備：** 08、10。**時數：** 10–16。**執行環境：** WSL 2 搭配 WSLg，不需要 Linux 實體機。

**製作：** 一個不用工具套件、直接建立在 libwayland 上的用戶端。它要：
- 從 registry 綁定全域物件；
- 用 memfd 建立 `wl_shm` pool；
- 以預乘 alpha 繪製 ARGB8888；
- 顯示 `xdg_toplevel`；
- 只在影格回呼時重繪；
- 透過 `wl_seat` 讀取鍵盤與指標輸入，用 xkbcommon 處理鍵盤配置。

以 `WAYLAND_DEBUG=1` 執行，閱讀通訊紀錄。

**需要時再學：**
- 物件、介面、請求與事件。
- registry，以及在 commit 時才生效的雙重緩衝 surface 狀態。
- `wl_shm` 格式：除了 ARGB8888 與 XRGB8888 之外，其餘沿用 DRM fourcc 代碼；這兩種是所有繪製器都應支援的格式（[協定](https://wayland.freedesktop.org/docs/html/apa.html)）。
- 預乘 alpha：`wl_buffer` 的說明把它定義在編碼後（electrical）的數值上。
- 以影格回呼作為重繪節流。
- wayland-protocols 的 stable、staging 與 unstable 分級。
- [The Wayland Protocol](https://wayland-book.com/)（草稿，CC BY-SA 4.0）可作為輔助教材。

**Linux／NT 對照：** WSLg 本身就是 Wayland 合成器：它執行搭配 XWayland 的 Weston，經 RDP 轉送到 Windows，並為你的發行版預設好 `WAYLAND_DISPLAY`（[WSLg](https://github.com/microsoft/wslg)、[Learn](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps)）。它的影格要經過系統記憶體，因此不要在這裡下任何時序結論。

**檢核：** 逐行解釋 `WAYLAND_DEBUG` 紀錄中一個影格的內容。證明影格回呼停止時，用戶端也會停止繪製。把階段 09 協定的每則訊息對應到相應的 Wayland 請求。

### 12 — DRM/KMS 與 atomic modesetting

**先備：** 04、11。**時數：** 14–24。**執行環境：** Linux 虛擬機器。使用 QEMU 的 [virtio-gpu](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html)（客體核心需啟用 `CONFIG_DRM_VIRTIO_GPU`）或 VKMS。virtio-gpu 是具備 render node 的 atomic 驅動，每個輸出有一個主平面與一個游標平面（[virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c)、[virtgpu_display.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_display.c)）。

**製作：** 先用 `drm_info` 與 libdrm 測試工具中的 `modetest` 檢視裝置。接著寫一個 C 程式：
- 以 DRM master 身分開啟 primary node；
- 列出 connector、CRTC、平面及其屬性；
- 配置 dumb buffer；
- 以一次 atomic commit 點亮螢幕。

用 test-only commit 探測驅動接受哪些設定，並以 page-flip 事件做雙重緩衝。閱讀 `kmscube`，把它當作 KMS 搭配 GBM 與 EGL 的參考程式；客體的 Mesa 能繪製時再實際執行。

**需要時再學：**
- KMS 管線：framebuffer → 平面 → CRTC → encoder → connector（[KMS overview](https://docs.kernel.org/gpu/drm-kms.html#overview)）。
- 屬性與 blob。
- atomic commit 與 test-only commit（[atomic](https://docs.kernel.org/gpu/drm-kms.html#atomic-mode-setting)）。
- 平面類型：對 atomic 用戶端而言，`type` 只是提示；每個 CRTC 各有自己的主平面（[plane abstraction](https://docs.kernel.org/gpu/drm-kms.html#plane-abstraction)）。
- 格式 modifier 與 `IN_FORMATS` 屬性（[format handling](https://docs.kernel.org/gpu/drm-kms.html#format-modifiers)）。
- DRM master 與 render node：render node 提供不需特權的 GPU 存取，但不能 modeset（[drm-uapi](https://docs.kernel.org/gpu/drm-uapi.html#render-nodes)）。

VKMS 是純軟體的 KMS 驅動模型，可透過 configfs 建立具有指定平面、CRTC 與 connector 的實例，並以計時器模擬 vblank（[VKMS](https://docs.kernel.org/gpu/vkms.html#configuring-with-configfs)）。適合練習 API，但量不出有意義的時序。

**檢核：** 根據 `drm_info` 的輸出，畫出這台機器的 KMS 圖。解釋程式為何必須在文字主控台執行，且不能有其他合成器持有 DRM master（[DRM master](https://docs.kernel.org/gpu/drm-uapi.html#primary-nodes-drm-master-and-authentication)）。展示一次被 test-only commit 拒絕的設定，並解釋原因。

### 13 — 平面、翻頁、vblank、VRR 與熱插拔

**先備：** 06、12。**時數：** 12–20。**執行環境：** Linux 虛擬機器；翻頁與熱插拔用 virtio-gpu，overlay 平面用 VKMS。

**製作：**
1. **翻頁事件。** 記錄每個翻頁完成事件的序號與時間戳記，和送出的時間比對，再刻意錯過一次 vblank。DRM 從 Linux 4.15 起在這些事件中使用單調時鐘時間戳記（[drm_vblank.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_vblank.c)）。虛擬機器中的數字跟著主機計時器走，因此這裡只學 API，真實時序留到硬體驗證再看。
2. **平面。** 透過 configfs 建立一個含 overlay 平面的 VKMS 實例，以 test-only commit 把 surface 放到 overlay 平面上，再透過 writeback connector 擷取合成結果（[VKMS](https://docs.kernel.org/gpu/vkms.html#configuring-with-configfs)）。記錄 VKMS 拒絕哪些設定。
3. **熱插拔。** 監聽 DRM uevent：事件帶有 `HOTPLUG=1`，針對單一 connector 的事件另帶 `CONNECTOR=<id>`（[drm_sysfs.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_sysfs.c)）。virtio-gpu 會把主機端的顯示變更（例如調整 QEMU 輸出大小或啟用輸出）轉成熱插拔事件（[virtgpu_kms.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_kms.c)）。收到後重新探測 connector 並選擇新模式。
4. **VRR 與連結狀態。** 讀懂相關屬性並寫好程式路徑：只有 connector 回報 `vrr_capable` 時，才在 CRTC 上設定 `VRR_ENABLED`；`link-status` 變成 `BAD` 時，選一個有效模式重新 commit。虛擬機器沒有 VRR 螢幕，也沒有會出問題的纜線，所以這些路徑要在硬體驗證時才會實際跑到。

**需要時再學：**
- 掃描輸出與遮沒區間、vblank 計數器，以及 page-flip 事件。
- VRR 如何延長垂直前廊（front porch）直到下一次翻頁，以及為何大幅變動可能造成頓挫，或在部分面板上造成亮度閃爍（[VRR properties](https://docs.kernel.org/gpu/drm-kms.html#variable-refresh-properties)）。
- 連結狀態（[connector properties](https://docs.kernel.org/gpu/drm-kms.html#standard-connector-properties)）。
- 游標大小限制。

**硬體檢核：**
- 真實的翻頁間隔。
- 真實 GPU 接受哪些平面。NVIDIA 只註冊主平面與游標平面（[README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kms.html)），所以 overlay 平面需要 Intel 或 AMD GPU。
- 在支援 VRR 的螢幕上測 VRR。ASUS VG289Q 規格列出 40–60 Hz（[規格](https://www.asus.com/displays-desktops/monitors/tuf-gaming/tuf-gaming-vg289q/techspec/)）。
- 實際拔掉纜線，並從 `link-status` 變成 `BAD` 的狀態恢復。

**檢核：** 程式執行中停用再重新啟用 QEMU 輸出，並在不重啟的情況下恢復。展示一張從 VKMS writeback 擷取的 overlay 平面合成結果。與 `display-wake-research` 比較：Windows 可能把休眠中的 DisplayPort 螢幕當成已中斷連線，同樣的情況在 Linux 上會產生哪些 KMS 事件？

### 14 — 緩衝區共享與同步

**先備：** 08 與 13，以及圖學課程的 GPU 階段。**時數：** 16–26。**執行環境：** Linux 虛擬機器；D3D 對照在 Windows 上做。

**製作：** 用 GBM（或 dumb buffer）配置緩衝區並繪製。在虛擬機器中只能以軟體繪製，但 virtio-gpu 支援含 timeline 的 DRM syncobj（[virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c)），所以下列三種方式都能在虛擬機器中執行。接著匯出成 dma-buf，經 `SCM_RIGHTS` 送給第二個行程。在第二個行程中連同 modifier 匯入為 KMS framebuffer，送去掃描輸出。依序以三種方式執行：
1. 隱式同步。
2. 顯式同步：透過平面的 `IN_FENCE_FD` 附上 sync_file，並從 `OUT_FENCE_PTR` 取回一個。
3. DRM syncobj timeline。

刻意拿掉一個 fence，擷取出錯的影格。在 Windows 上，用 `CreateSharedHandle` 與 `DuplicateHandle` 讓兩個行程共用 D3D12 資源與 fence（[D3D12](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle)、[DXGI](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiresource1-createsharedhandle)）。

**需要時再學：**
- GEM 與 PRIME（[PRIME](https://docs.kernel.org/gpu/drm-mm.html#prime-buffer-sharing)）。
- dma-buf、dma-fence 與 dma-resv（[dma-buf](https://docs.kernel.org/driver-api/dma-buf.html)）。
- sync_file：在驅動程式與行程之間傳遞 fence 的載體（[sync_file](https://docs.kernel.org/driver-api/sync_file.html)）。
- 銜接顯式與隱式同步的 `DMA_BUF_IOCTL_EXPORT_SYNC_FILE` 與 `DMA_BUF_IOCTL_IMPORT_SYNC_FILE`（[ioctls](https://docs.kernel.org/driver-api/dma-buf.html#dma-buffer-ioctls)）。
- KMS 的顯式 fence 屬性（[KMS](https://docs.kernel.org/gpu/drm-kms.html#explicit-fencing-properties)）。
- DRM sync object 的 binary 與 timeline 兩種形式（[syncobj](https://docs.kernel.org/gpu/drm-mm.html#drm-sync-objects)）。
- Wayland 的 `linux-dmabuf-v1`（stable）與 `linux-drm-syncobj-v1`（staging）。
**硬體檢核：** 改用 GPU 繪製與真實的 modifier 再做一次。NVIDIA 的顯式同步使用 syncobj timeline 點，需要 6.8 以上的核心（[README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/wayland-issues.html)）。

**檢核：** 畫出一個緩衝區的生命週期：用戶端繪製、fence、合成器匯入、掃描輸出、釋放、重用。為 Windows 的共享 handle 與 fence 畫出同樣的圖。說明若合成器在掃描輸出結束前就釋放緩衝區，會壞掉什麼。

### 15 — 輸入堆疊

**先備：** 08、12。**時數：** 10–16。**執行環境：** Linux 虛擬機器。虛擬機器的虛擬鍵盤與滑鼠會以 evdev 裝置出現，其餘情境用 uinput 補足。

**製作：** 直接讀取 `/dev/input/eventX`，印出以 `SYN_REPORT` 分組的 `EV_KEY`、`EV_REL`、`EV_ABS` 事件。改用 libinput，再加入 xkbcommon 處理鍵盤配置與修飾鍵。用 uinput 建立虛擬裝置，寫可重複執行的測試腳本。為階段 09 的伺服器加入指標與鍵盤焦點，讓事件只送到取得焦點的用戶端。

**需要時再學：**
- evdev 事件模型（[input](https://docs.kernel.org/input/input.html)、[event codes](https://docs.kernel.org/input/event-codes.html)）。
- 以 `EVIOCGRAB` 獨占裝置（[input.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h)）。
- uinput（[文件](https://docs.kernel.org/input/uinput.html)）。
- libinput：在合成器行程內執行、讀取事件裝置、處理觸控板、繪圖板等裝置的特性修正，用戶端完全看不到它（[libinput](https://wayland.freedesktop.org/libinput/doc/latest/what-is-libinput.html)）。
- xkbcommon 負責鍵盤配置與鍵盤狀態（[xkbcommon](https://xkbcommon.org/)）。
- Wayland 如何透過 `wl_keyboard` 傳送鍵盤配置。

**Linux／NT 對照：** Windows 的 Raw Input 讓應用程式註冊裝置、接收 `WM_INPUT`，並能分辨兩支滑鼠（[Learn](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input)）；一般輸入則以視窗訊息的形式送進執行緒的佇列。

**檢核：** 從紀錄量測輸入到掃描輸出的延遲，也就是從事件時間戳記到顯示其效果的那次翻頁。虛擬機器中的數字只能相對比較，硬體驗證會再量一次。說明為何輸入裝置必須由合成器，而不是由各個用戶端持有。

### 16 — 座位與工作階段管理

**先備：** 12、15。**時數：** 6–10。**執行環境：** Linux 虛擬機器。若主機攔截了 Ctrl+Alt+F 組合鍵，就改用 `chvt` 切換 VT。

**製作：** 讓階段 12 的程式透過 libseat，以一般使用者身分執行，後端用 logind 或 seatd。處理 VT 切換：暫停時停止 commit 並釋放 DRM master；恢復時取得新的 fd 並還原狀態。確認工作階段非作用中時收不到任何輸入。

**需要時再學：**
- logind 的 session API：`TakeControl`、`TakeDevice`、`PauseDevice`、`PauseDeviceComplete`、`ResumeDevice`（[logind](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html)）。
- Weston 所使用的 libseat 及其後端（[Weston](https://wayland.pages.freedesktop.org/weston/toc/running-weston.html)），以及 [seatd](https://git.sr.ht/~kennylevinsen/seatd)。
- 以 `SET_MASTER` 與 `DROP_MASTER` 交接 DRM master（[drm-uapi](https://docs.kernel.org/gpu/drm-uapi.html#primary-nodes-drm-master-and-authentication)）。
- 用 `EVIOCREVOKE` 撤銷輸入 fd 的存取權（[input.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h)）。

**Linux／NT 對照：** Windows 的服務全部在 session 0 執行，不能直接與使用者互動（[Learn](https://learn.microsoft.com/en-us/windows/win32/services/interactive-services)）。window station 與桌面都是可設定安全性的物件，界定剪貼簿、視窗、選單與 hook 的範圍（[Learn](https://learn.microsoft.com/en-us/windows/win32/winstation/about-window-stations-and-desktops)）。

**檢核：** 來回切換 VT 十次。之後不應有遺留的 DRM master、卡住的輸入，也不應掉影格。說明在每種設計中，裝置存取權由誰授予。

### 17 — 用戶端緩衝區的隔離與安全

**先備：** 09、14、16。**時數：** 8–14。**執行環境：** Linux 虛擬機器；共享記憶體的部分可在 WSL 2 進行；Windows 用於對照。

**製作：** 攻擊自己的伺服器，再修好它。寫出以下用戶端：
- 縮小自己的 shm pool；
- 送出未封存的 memfd；
- 大量灌入請求；
- 送出大小或 modifier 錯誤的 dma-buf；
- 要求取得其他用戶端的緩衝區；
- 送出永遠不會結束的 GPU 工作。現在先在 Windows 上觀察 TDR，Linux 的 GPU 重置留到硬體驗證。

接著用 xdg-desktop-portal ScreenCast 正確地加入畫面擷取：其中 `Start` 通常會詢問使用者，並回傳 PipeWire 串流（[portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.ScreenCast.html)）。也了解特權擷取協定 `ext-image-copy-capture-v1`（staging）的用途。

**需要時再學：**
- 用戶端送來的每個請求與緩衝區都是不可信的輸入。
- 封存與 `SIGBUS` 處理。
- 驗證 dma-buf 匯入。
- GPU 強健性與重置。
- 供沙箱用戶端使用的 `security-context-v1`（staging）。
- 在 X11 與 Wayland 規格中，找出哪些請求能讓一個用戶端讀取另一個用戶端的像素，或注入輸入。

**Linux／NT 對照：** UIPI 會擋下來自較低完整性等級傳送者的視窗訊息（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex)）。`WDA_EXCLUDEFROMCAPTURE`（Windows 10 2004）可讓視窗不出現在擷取畫面中，但只在 DWM 合成時有效，Learn 也說明它不是安全功能（[Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity)）。

**檢核：** 為你的合成器寫一頁威脅模型：要保護的資產、不可信的輸入、緩解措施與剩餘風險。製作階段的每個攻擊用戶端都在測試腳本中執行，且伺服器不會當掉。

### 18 — 總結專題：DRM/KMS 上的最小 Wayland 合成器

**先備：** 10–17。**時數：** 60–120。**執行環境：** Linux 虛擬機器（QEMU 搭配 virtio-gpu），以軟體繪製；之後在硬體驗證中於真實螢幕上執行。

**路線（審閱時決定）：** 把階段 12–16 的程式（DRM/KMS、libinput、libseat、GBM）組成自己的後端，建立在 libwayland-server 之上。面向核心的程式用 C；合成器本身可用 C++，以配合圖學課程的核心。先讀 wlroots 的最小範例合成器 [tinywl](https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/tinywl)，把它當地圖，而不是當基礎。後端不使用 [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)。

**製作，分里程碑：**
1. **單一用戶端。** `wl_compositor`、`wl_shm`、`xdg_wm_base`。由圖學模組把一個全螢幕 toplevel 合成到 KMS framebuffer，並由 page-flip 事件驅動影格回呼。
2. **多個視窗。** 堆疊順序、焦點，以及用快捷鍵移動與置頂。透過 `wl_seat` 處理鍵盤與指標，游標放在游標平面上。
3. **dma-buf 用戶端。** 匯入 `linux-dmabuf-v1`、damage 追蹤；驅動支援時，以 `linux-drm-syncobj-v1` 做顯式同步。在虛擬機器中，用階段 14 的緩衝區共享用戶端測試；GPU 繪製的用戶端與 GPU 合成留到硬體驗證。
4. **時序。** `presentation-time` 回饋；格式與 modifier 允許時，直接掃描輸出全螢幕用戶端的緩衝區。
5. **強健性。** 用戶端當掉、熱插拔、VT 切換、GPU 重置，以及階段 17 的攻擊用戶端。

要依序跑起來的真實應用程式：
1. [Weston](https://wayland.pages.freedesktop.org/weston/) 的示範用戶端。
2. [`foot`](https://packages.debian.org/sid/foot)，一個精簡的 Wayland 終端機。
3. Vulkan 示範程式，例如 `vkcube`。在虛擬機器中需要客體 Mesa 提供軟體 Vulkan 驅動，否則留到硬體驗證。
4. GTK 4 或 Qt 6 應用程式。工具套件會用到更多 xdg-shell 功能（popup、configure 流程）與更多協定，預期要一路補上全域物件，直到程式能啟動。

Xwayland 屬選修。

**檢核：** 錄下一段操作：
- 開機進入文字主控台，以一般使用者身分啟動合成器；
- 開啟終端機、一個工具套件應用程式與一個 Vulkan 示範程式；
- 移動視窗、切換 VT 再切回、停用再重新啟用一個輸出、強制結束一個用戶端。

寫出設計說明：附實測時間戳記，追蹤一個用戶端影格從 `wl_surface.commit` 到掃描輸出、再回到影格回呼的過程，並附上階段 17 的威脅模型。在虛擬機器中，這些時間戳記只能相對比較。

## 硬體驗證

**先備：** 階段 18 已在虛擬機器中完成。**時數：** 15–30。**執行環境：** 能直接控制真實顯示器的 Linux 實體機，也就是 RTX 5070 Ti 桌機上的雙系統開機，或另一台電腦（README 待決問題 1）。

在真實硬體上執行合成器與各階段程式，補齊虛擬機器看不到的部分：

1. **翻頁時序：** 影格間隔與 vblank 的關係、錯過的期限，以及階段 06 的排程選擇如何影響它們。
2. **平面：** 真實 GPU 接受哪些平面。NVIDIA 只註冊主平面與游標平面（[README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kms.html)），因此 overlay 平面需要 Intel 或 AMD GPU。
3. **VRR：** 在支援 VRR 的螢幕上（ASUS VG289Q 規格列出 40–60 Hz），比較開與不開 `VRR_ENABLED` 的影格間隔。
4. **熱插拔：** 實際拔掉纜線並恢復，處理 `link-status` 變成 `BAD` 的情況。
5. **GPU 緩衝區：** 以 GPU 繪製、GPU 合成與真實的 modifier，重做階段 14 與總結專題里程碑 3。在 NVIDIA 上需要開放核心模組，顯式同步還需要 6.8 以上的核心（[README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/wayland-issues.html)）。
6. **延遲與重置：** 階段 15 的輸入到畫面延遲，以及階段 17 的 GPU 重置。
7. **再做一次總結專題：** 重做階段 18 的檢核，這次的時間戳記才有意義。

**檢核：** 設計說明加入一張表，對照同一組執行在虛擬機器與實體機上的數字，並解釋每一項差異。

## 與圖學課程的介面

合成器每輸出一個影格，就呼叫一次圖學課程的合成模組，也就是圖學課程[第二部草稿](../computer-graphics-practice/SYLLABUS-PART2.zh-TW.md)「2D、文字與合成渲染路線」的 T4（合成）；文字來自 T3，色彩處理來自 C5 與 T5。兩份文件都還是草稿，因此以下是待雙方確認的提案。T4 已接收含位置、大小、不透明度、變換與圓角裁切的 surface，本介面再補上系統合成器需要的部分。

**輸入：** 一串 surface，每個 surface 包含：
- 緩衝區：`wl_shm` 是 CPU 記憶體，dma-buf 是 GPU 影像；
- DRM fourcc 格式與 modifier；
- 尺寸、位置、縮放與變換；
- 不透明度與裁切；
- damage 區域與堆疊順序；
- GPU 緩衝區的 acquire fence。

**輸出：** 寫入目標緩衝區（KMS framebuffer 或 GBM 緩衝區）的影格、有變動的區域，以及 release fence。

**雙方必須約定的規則：**
- Wayland 緩衝區的 alpha 預乘在編碼後的數值上，而 T4 是在線性光中混合預乘後的顏色。因此每個 Wayland surface 都要先解除預乘、解碼，再於線性光中重新預乘後才能混色，最後把結果編碼成輸出格式。決定這項轉換由合成器還是 T4 負責，並寫進設計說明。T4 的檢核已要求展示編碼空間與線性光混色的差異。
- 合成模組需要能在 Linux 上執行的後端。圖學課程第二部讓你在 Vulkan 與 Direct3D 12 之間選擇，但只有 Vulkan 或 CPU 路徑能在這裡執行。在虛擬機器中，除非客體有 3D 加速，否則 Vulkan 只能靠軟體驅動；比照 T1 的 CPU 光柵器保留一條 CPU 合成路徑，虛擬機器路線才能順利使用。
- ARGB8888 與 XRGB8888 必須能用，其他格式與 modifier 組合都屬選用，需要協商。
- KMS 平面的「pixel blend mode」屬性區分預乘與覆蓋（coverage）兩種混色。合成器把 surface 直接交給硬體平面時，平面的結果必須與合成模組的計算一致。
- 文字（視窗標題、除錯覆層）由 T3 提供。
- 合成模組不得阻塞：它會收到一個期限（下一次 vblank 減去安全餘裕），並回報實際花費的時間。

在 T4 完成前，階段 09 使用介面相同的替代實作。

## 選修支線

- **rv32sim 特權模式。** 為 [`c-compiler-foundations`](../c-compiler-foundations/README.zh-TW.md) 的模擬器加入機器碼解碼、CSR、陷阱進出與 Sv32 分頁，再在上面開機一個自己寫的小核心。這條支線把兩個目錄接起來，約需 30–60 小時。
- **在虛擬機器中寫 NT 驅動。** 在啟用測試簽章的 Windows 虛擬機器中寫一個 KMDF 驅動。
- **HDR 與色彩。** 目前的核心文件已記載 KMS 逐平面色彩管線（[colorop](https://docs.kernel.org/gpu/drm-kms.html#colorop-abstraction)），NVIDIA 列出自 Linux 6.19 起支援。可搭配 Wayland 的 `color-management-v1`（staging）與圖學課程的 C5、T5 一起研究。
- **Xwayland**，用來執行 X11 應用程式。
- **NVK。** 在同一張卡上比較 Mesa 的 [NVK](https://docs.mesa3d.org/drivers/nvk.html)（列出支援消費級 Blackwell）與 NVIDIA 驅動。

## 建議的學習節奏

每個階段可拆成短循環：

1. **預測：** 修改程式前，先寫下預期的追蹤結果、數值或時序。
2. **實作：** 每次加一項能力，並保留可比較的已知正常版本。
3. **觀察：** 用 GDB、`strace`、`WAYLAND_DEBUG`、`drm_info`、PresentMon 或時間戳記紀錄看清實際發生的事。
4. **解釋：** 寫下發生什麼、原因為何，並附上定義該行為的文件頁面。
5. **弄壞：** 注入一個可恢復的錯誤，記錄它如何顯現。

## 來源

查核日期為 2026-11-15。版本、狀態與頁面內容都可能變動，實作前請重新確認。標示 *未重新閱讀* 的頁面只附上連結，因為查核當天這些網站拒絕自動化請求。

**教學核心與模擬**
- MIT 6.1810：[課程首頁](https://pdos.csail.mit.edu/6.1810/)（2026 秋季建置中）、[2025 課表](https://pdos.csail.mit.edu/6.1810/2025/schedule.html)（實驗順序）、[2025 工具頁](https://pdos.csail.mit.edu/6.1810/2025/tools.html)（WSL 2 搭配 Ubuntu 24.04、套件、QEMU 7.2+、GDB 8.3+）、[xv6 課本 RISC-V rev5](https://pdos.csail.mit.edu/6.1810/2025/xv6/book-riscv-rev5.pdf)。
- [xv6-riscv 原始碼庫](https://github.com/mit-pdos/xv6-riscv)：README，以及 Makefile 中的 `QEMUOPTS`、`CPUS := 3`、`-march=rv64gc`。
- QEMU：[Windows 下載](https://www.qemu.org/download/#windows)、[加速器（含 WHPX）](https://www.qemu.org/docs/master/system/introduction.html#virtualisation-accelerators)、[virtio-gpu](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html)。
- WSL：[WSL 1 與 WSL 2 比較](https://learn.microsoft.com/en-us/windows/wsl/compare-versions)、[.wslconfig 設定](https://learn.microsoft.com/en-us/windows/wsl/wsl-config)（`nestedVirtualization`、GPU 半虛擬化）、[GUI 應用程式](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps)、[WSLg 架構](https://github.com/microsoft/wslg)。

**Linux 核心文件與原始碼**
- 排程：[EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)、[SCHED_DEADLINE](https://docs.kernel.org/scheduler/sched-deadline.html)、[sched_ext](https://docs.kernel.org/scheduler/sched-ext.html)。
- 核心機制：[generic IRQ 與執行緒化處理](https://docs.kernel.org/core-api/genericirq.html)、[driver model](https://docs.kernel.org/driver-api/driver-model/overview.html)、[lockdep](https://docs.kernel.org/locking/lockdep-design.html)、[RCU](https://docs.kernel.org/RCU/whatisRCU.html)。
- 輸入：[輸入子系統與 evdev](https://docs.kernel.org/input/input.html)、[event codes](https://docs.kernel.org/input/event-codes.html)、[uinput](https://docs.kernel.org/input/uinput.html)、[uapi input.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h)（`EVIOCGRAB`、`EVIOCREVOKE`）。
- KMS：[drm-kms](https://docs.kernel.org/gpu/drm-kms.html)。使用的章節：overview、atomic mode setting、plane abstraction、standard plane properties、explicit fencing、variable refresh、standard connector properties、format modifiers、colorop abstraction。
- DRM uAPI：[drm-uapi](https://docs.kernel.org/gpu/drm-uapi.html)（DRM master、render node）、[drm-mm](https://docs.kernel.org/gpu/drm-mm.html)（PRIME、sync object）。
- 文件過長無法擷取時改讀的原始碼：[drm_sysfs.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_sysfs.c)（熱插拔 uevent）、[drm_vblank.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_vblank.c)（vblank 處理、單調時鐘時間戳記）。
- 緩衝區與 fence：[dma-buf、dma-fence、dma-resv 與 sync-file ioctl](https://docs.kernel.org/driver-api/dma-buf.html)、[sync_file](https://docs.kernel.org/driver-api/sync_file.html)、[VKMS](https://docs.kernel.org/gpu/vkms.html)。

**Linux man page（man7.org）**
- [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html)、[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)、[mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html)、[clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)、[futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)。
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)、[eventfd(2)](https://man7.org/linux/man-pages/man2/eventfd.2.html)、[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)、[signalfd(2)](https://man7.org/linux/man-pages/man2/signalfd.2.html)、[io_uring(7)](https://man7.org/linux/man-pages/man7/io_uring.7.html)。

**Wayland 與 freedesktop.org**
- Wayland：[架構](https://wayland.freedesktop.org/architecture.html)、[協定規格](https://wayland.freedesktop.org/docs/html/apa.html)（`wl_shm.format`、`wl_shm` 與 `wl_buffer` 的預乘 alpha、`wl_surface.frame`）、[The Wayland Protocol](https://wayland-book.com/)（草稿，CC BY-SA 4.0）。
- [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols)（*未重新閱讀*）。狀態取自由同一批 XML 產生的 [wayland.app 索引](https://wayland.app/protocols/)：
  - stable：xdg-shell、presentation-time、viewporter、linux-dmabuf-v1；
  - staging：linux-drm-syncobj-v1、security-context-v1、ext-image-copy-capture-v1、color-management-v1、fifo-v1、commit-timing-v1、tearing-control-v1；
  - unstable：xdg-decoration、linux-explicit-synchronization。
- 輸入與座位：[libinput](https://wayland.freedesktop.org/libinput/doc/latest/what-is-libinput.html)、[xkbcommon](https://xkbcommon.org/)、[logind D-Bus API](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html)、[seatd](https://git.sr.ht/~kennylevinsen/seatd)（*未重新閱讀*）。
- 合成器：[Weston 的執行方式與後端](https://wayland.pages.freedesktop.org/weston/toc/running-weston.html)（libseat 搭配 seatd 或 logind）、[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) 與 [tinywl](https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/tinywl)（*未重新閱讀*）。
- [xdg-desktop-portal ScreenCast](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.ScreenCast.html)。
- 工具，經 Debian 套件頁確認：[kmscube](https://packages.debian.org/sid/kmscube)、[drm-info](https://packages.debian.org/sid/drm-info)、[libdrm-tests 檔案清單](https://packages.debian.org/sid/amd64/libdrm-tests/filelist)（內含 `modetest`）、[foot](https://packages.debian.org/sid/foot)、[weston](https://packages.debian.org/sid/weston)。

**Microsoft Learn 與 Microsoft**
- DWM 與呈現：[DwmEnableComposition](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenablecomposition)、[DWM 概觀](https://learn.microsoft.com/en-us/windows/win32/dwm/dwm-overview)（較舊的文字）、[WDDM 架構](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)、[多平面 overlay](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support)、[DXGI flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)、[DirectX blog 談 flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/)、[PresentMon](https://github.com/GameTechDev/PresentMon)。
- 桌面以外的顯示器：[Windows.Devices.Display.Core](https://learn.microsoft.com/en-us/uwp/api/windows.devices.display.core)、[specialized displays](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/specialized-monitors)、[IddCx](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview)、[TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery)。
- 共享、擷取與輸入：[DXGI CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiresource1-createsharedhandle)、[D3D12 CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle)、[SetWindowDisplayAffinity](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity)、[ChangeWindowMessageFilterEx 中的 UIPI](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex)、[window station 與桌面](https://learn.microsoft.com/en-us/windows/win32/winstation/about-window-stations-and-desktops)、[Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input)。
- 核心與驅動：[IRQL](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities)、[DPC 物件](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-dpc-objects)、[Nt 與 Zw 常式](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)、[NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess)、[WDM](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-wdm)、[WDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/)、[裝置堆疊](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/device-nodes-and-device-stacks)、[TESTSIGNING](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)。
- 行程與記憶體：[行程與執行緒](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)、[排程優先權](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)、[優先權提升](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)、[檔案映射](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)、[具名共享記憶體](https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory)、[保留與提交](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory)。
- 行程間通訊、訊息與服務：[DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle)、[I/O completion port](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)、[訊息與訊息佇列](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)、[MsgWaitForMultipleObjectsEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex)、[互動式服務與 session 0](https://learn.microsoft.com/en-us/windows/win32/services/interactive-services)。
- 同步：[自旋鎖](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks)、[dispatcher 物件](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-kernel-dispatcher-objects)、[SRW lock](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)、[WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)。
- 工具：[Windows Performance Toolkit](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/)、[Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer)。

**虛擬機器**
- Hyper-V：[Discrete Device Assignment](https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/plan/plan-for-deploying-devices-using-discrete-device-assignment)（以 Windows Server 為前提、需伺服器等級硬體、列出的客體只有 Windows）、[啟用 Hyper-V](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/quick-start/enable-hyper-v)（Windows 11 專業版或企業版；與其他 hypervisor 並用的警告）。
- virtio-gpu 客體驅動：[virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c)（atomic、render node、含 timeline 的 syncobj）、[virtgpu_display.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_display.c)（主平面與游標平面）、[virtgpu_kms.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_kms.c)（主機顯示事件轉為熱插拔）。
- vmwgfx：[vmwgfx_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vmwgfx/vmwgfx_drv.c)、[vmwgfx_stdu.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vmwgfx/vmwgfx_stdu.c)（atomic、render node、主平面與游標平面、沒有 syncobj）。驅動功能那一行與平面清單，都來自工具對檔案後段所做的摘要。
- hyperv_drm：[hyperv_drm_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/hyperv/hyperv_drm_drv.c)、[hyperv_drm_modeset.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/hyperv/hyperv_drm_modeset.c)（只有一個主平面，沒有 render node）。
- [VMware Fusion 與 Workstation 對所有使用者免費](https://blogs.vmware.com/cloud-foundation/2024/11/11/vmware-fusion-and-workstation-are-now-free-for-all-users/)（2024-11-11）。

**NVIDIA、Mesa 與螢幕**
- NVIDIA Linux 驅動 620.70.06 README：[KMS](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kms.html)（預設開啟；只有主平面與游標平面；已知問題）、[開放核心模組](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kernel_open.html)（Blackwell 只支援開放模組）、[GBM](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/gbm.html)、[Wayland 已知問題](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/wayland-issues.html)（顯式同步需 6.8+ 核心、COLOR_PIPELINE 需 6.19+、simpledrm 與 `fbdev=1` 的說明）。KMS 章節另說 GBM 緩衝區送交 KMS 尚未支援，與 GBM 章節矛盾，需在實機上確認。
- [NVIDIA 開放 GPU 核心模組](https://github.com/NVIDIA/open-gpu-kernel-modules)（Turing 以後）。
- [Mesa NVK](https://docs.mesa3d.org/drivers/nvk.html)（Kepler 到 Ada，以及消費級 Blackwell）。
- [ASUS VG289Q 規格](https://www.asus.com/displays-desktops/monitors/tuf-gaming/tuf-gaming-vg289q/techspec/)（Adaptive-Sync，40–60 Hz）。
