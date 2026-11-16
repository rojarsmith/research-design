[English](README.md) | **繁體中文** | [實作建置計畫，階段 09](../../SYLLABUS.zh-TW.md) | [協定](PROTOCOL.zh-TW.md)

# 實驗 09：第一部分專題，無螢幕顯示伺服器

> **草稿，待審閱（2026-11-16）。哪些有執行、哪些沒有：**
> - **測試框架（六個用戶端、`check.py`、`run.sh`）：已建置並執行**，於 2026-11-16 在 Windows 11 上以 podman 執行的 Ubuntu 24.04 容器（映像 `oslab:24.04`）中完成。podman machine 是一台 WSL 2 虛擬機器，所以這不是 WSL 2 Ubuntu 本身，以下所有時間都是虛擬機器中的時間：核心 6.6.87.2-microsoft-standard-WSL2、16 個 vCPU、gcc 13.3.0、Python 3.12.3。受檢查的伺服器是一個不隨附的參考實作。三次執行全數通過，記錄在 [`harness/example-output.txt`](harness/example-output.txt)。三次開始時，虛擬機器的一分鐘平均負載為 0.6–0.7。
> - **以測試框架檢查有錯的伺服器：已執行。** 參考實作另外建置了 14 次，每次刻意加入一個錯誤（見[測試框架抓得到什麼](#測試框架抓得到什麼)）。每一個都失敗了，而且都失敗在針對該錯誤的那項檢查上。這些建置同樣不隨附；輸出在同一個檔案中。
> - **未執行：** 任何在 WSL 2 Ubuntu 本身上的步驟（步驟相同，但沒有試過）；機器在 CPU 負載下的執行（練習 E）；設計說明、Wayland 比較與延伸練習，這些由你撰寫。圖學課程的 T4 合成模組尚未存在，所以只使用了混合替代實作（stub）。
> - **機器：** Windows 11 Pro 組建 26300、Intel Core i7-11700（16 個邏輯處理器）。同一台機器上同時有其他容器在執行。所有數字都只屬於這台機器與這幾次執行。

## 目標

第一部分以一個需要用上前面所有內容的程式作結：一個沒有螢幕的顯示伺服器。用戶端透過 Unix socket 連線，以 `SCM_RIGHTS` 交出封存的共享記憶體緩衝區，再用 attach、damage 與 commit 說明哪裡變了。在每次模擬的 vblank，也就是每秒六十次，伺服器若有任何變化，就把各用戶端的表面合成到一個離屏影格，把該影格寫成圖檔，並告訴有要求的用戶端它的影格已完成。接著問題用戶端登場：一個在影格中途當掉、一個永遠不讀自己的 socket、一個送出超出緩衝區的 damage、一個在伺服器底下縮小自己的緩衝區。伺服器必須撐過全部，並在它們搗亂時維持 60 Hz 的影格節奏。

```
 good-anim (A) ──────┐  Unix stream socket：請求 ->，<- 事件
 good-alpha (B) ─────┤  封存的 memfd 以 SCM_RIGHTS 輔助資料一起送過來
 bad-* (C N R D U) ──┘
                     │
                     ▼  單一執行緒、單一 epoll 迴圈：監聽 socket、用戶端 socket、
                        60 Hz 的 timerfd、接收 SIGTERM 的 signalfd
  每個用戶端：輸入位元組、fd 佇列、物件、有上限的輸出佇列
                     │  commit：從用戶端的 memfd 複製受損像素，釋放緩衝區
                     ▼
  表面（伺服器的複本）── vblank n：若有變化就合成 ── frames/frame-n.ppm
                          └── 對每個有要求的表面送出 frame_done(n, vblank n 的時間)
```

這是實驗 06 的 60 Hz 喚醒、實驗 03 的 `SIGBUS`，以及階段 08 的 fd 傳遞、封存與事件迴圈，拿來對付不合作的用戶端。階段 08 的基本機制在 [`labs/08-ipc-event-loops/`](../08-ipc-event-loops/) 教；本專題假設你已經做完。

**時數：** 20–35。**執行環境：** WSL 2（任何有 gcc 與 python3 的 Linux 皆可；不需要螢幕）。

## 題目

用 C 寫一個伺服器，做到：

1. 實作[協定](PROTOCOL.zh-TW.md)：訊息切分、物件與 id、`hello` 與版本協商、`sync`、表面與緩衝區，以及錯誤；
2. 只接受封存為不可縮小的 memfd 作為緩衝區，而且大小必須足夠；
3. 為每個用戶端保存狀態，不論用戶端以什麼方式離開，都釋放它的全部狀態（物件、映射、佇列中的 fd）；
4. 以不可分割的方式套用 attach、damage 與 commit，複製受損像素，安全地裁切 damage，並釋放緩衝區；
5. 以預乘 ARGB8888 的混合替代實作，依堆疊順序把表面合成到 320 × 240 的離屏影格；
6. 用 `timerfd` 模擬 60 Hz 的 vblank，並送出帶有影格序號與 vblank 時間的 `frame_done`；
7. 把有變化的影格寫成 PPM 檔；
8. 絕不因某個用戶端而阻塞，限制為每個用戶端排隊的資料量，並撐過 `SIGPIPE`；
9. 收到 `SIGTERM` 時乾淨地結束。

然後執行測試框架，直到連續三次通過，再寫設計說明與一頁的 Wayland 比較。

## 檔案

| 檔案 | 內容 | 狀態 |
| --- | --- | --- |
| [`PROTOCOL.md`](PROTOCOL.md)、[`PROTOCOL.zh-TW.md`](PROTOCOL.zh-TW.md) | 你的伺服器要實作的協定，版本 1 | 規格 |
| [`harness/lab09-protocol.h`](harness/lab09-protocol.h) | 協定中各數值的 C 名稱；你的伺服器可以 include 它 | 已在此建置 |
| [`harness/clientlib.c`](harness/clientlib.c)、[`clientlib.h`](harness/clientlib.h) | 用戶端共用的部分：連線、連同 fd 送出、讀取事件、建立封存或未封存的 memfd | 已在此建置並執行 |
| [`harness/good-anim.c`](harness/good-anim.c) | 用戶端 A：一個不透明表面、兩個緩衝區、每個 `frame_done` 一次 commit；依指令換畫面；以它的時間戳記量測影格節奏 | 已在此建置並執行 |
| [`harness/good-alpha.c`](harness/good-alpha.c) | 用戶端 B：一個半透明表面，緩衝區從偏移量處開始；一個最高位元組放雜訊的 XRGB8888 表面；要求版本 7 | 已在此建置並執行 |
| [`harness/bad-crash.c`](harness/bad-crash.c) | 問題用戶端 C：送出半個 commit、一個沒用到的 fd 還在途中、一個沒 attach 的緩衝區，然後以 `SIGKILL` 結束 | 已在此建置並執行 |
| [`harness/bad-noread.c`](harness/bad-noread.c) | 問題用戶端 N 與 R，同一個程式的兩種模式：N 大量送出請求且從不讀取；R 關閉自己的讀取端，使每個回覆都以 `EPIPE` 失敗 | 已在此建置並執行 |
| [`harness/bad-damage.c`](harness/bad-damage.c) | 問題用戶端 D：超出緩衝區的 damage，包括 `x + width` 超過 32 位元的情況 | 已在此建置並執行 |
| [`harness/bad-unsealed.c`](harness/bad-unsealed.c) | 問題用戶端 U：被接受後縮小到 0 的未封存 memfd，以及一個已封存但對其尺寸而言太小的 memfd | 已在此建置並執行 |
| [`harness/check.py`](harness/check.py) | 啟動你的伺服器與各用戶端，檢查存活、影格節奏、資源，以及每個比對的影格；只用標準函式庫 | 已在此執行 |
| [`harness/run.sh`](harness/run.sh) | 建置各用戶端（以及你的伺服器），再執行 `check.py` | 已在此執行 |
| [`harness/example-output.txt`](harness/example-output.txt) | 三次通過的執行，以及 14 個有錯的建置 | 2026-11-16 記錄 |
| 參考解答（不隨附） | 一個能通過的伺服器，以及它的 14 個有錯版本 | 輸出紀錄來自它 |

參考解答（不隨附）：這裡記錄的通過結果來自一個放在本儲存庫之外的參考伺服器，那 14 個有錯的建置也是。請自己寫。要用測試框架檢查你的伺服器，把原始碼放在本 README 旁的 `solution/`（git 會忽略它），形式可以是一個 `Makefile`，其預設目標建出名為 `server` 的程式；也可以是能用 `gcc -O2 -Wall -Wextra` 一起編譯的 `.c` 檔。或者讓 `SOLUTION_DIR` 指向該目錄，或直接傳入你自己建好的程式：`sh harness/run.sh ./server`。沒有伺服器時，`run.sh` 會建置各用戶端，並說明它略過了檢查。

## 為何協定是固定的

建置計畫要求「你自己設計的小協定」。本實驗改為公布一份協定 [PROTOCOL.zh-TW.md](PROTOCOL.zh-TW.md)，由你的伺服器實作。理由如下：

- **測試框架只能檢查它能對話的伺服器。** 線上格式固定之後，問題用戶端就能做得很精確：一個在 8 位元組標頭的第 6 個位元組處被截斷的 commit、一個 `x + width` 超過 `int32` 的 damage 矩形。檢查程式也能預測每個影格的每個像素。
- **這個階段難的是伺服器，不是訊息的排列方式。** 每個用戶端的狀態、背壓、絕不阻塞與影格節奏，仍然全部由你設計。PROTOCOL.md 規定的是線上必須成立的事，而不是怎麼做到；它也點出留給你的選擇，例如不讀取的用戶端要被斷線還是被限流。
- **協定設計改由版本管理來練習。** 練習 A 要你設計版本 2，而測試框架的版本 1 用戶端必須繼續通過。用戶端 B 一開始就要求版本 7，所以你的伺服器從第一天起就得往下協商。
- **它刻意接近 Wayland，也刻意有所不同。** 它有相同的標頭排列、在資料之外傳遞的 fd，以及 attach、damage、commit、frame 與 release。不同之處有幾個：每個 memfd 一個緩衝區、必須封存、由 damage 決定要複製什麼、`frame_done` 是表面上的事件，以及整條連線只有一個版本。這讓比較有內容可寫。

如果你仍想用自己的協定：各用戶端的所有編碼都經過 `clientlib.c` 與 `lab09-protocol.h`。移植這兩個檔案，再加上 `bad-crash.c` 與 `bad-noread.c` 手工組出的訊息，測試框架的其餘部分仍然適用。這條路在這裡沒有試過。

## 環境設定

- 實驗 00 的 WSL 2 Ubuntu 24.04，或任何有 gcc 與 python3 的 Linux。不需要其他套件、螢幕或 root。
- 這裡記錄的執行使用前面實驗的容器，從 Git Bash 啟動，以唯讀方式掛載本儲存庫與參考實作：

  ```sh
  MSYS_NO_PATHCONV=1 podman run --rm \
    -v "C:/path/to/research-design:/work:ro" -v "C:/path/to/your/server:/solution:ro" oslab:24.04 \
    sh -c 'SOLUTION_DIR=/solution sh /work/os-foundations/labs/09-headless-display-server/harness/run.sh'
  ```

  `run.sh` 在 `/tmp` 底下的新目錄中建置，從不寫入這個資料夾。每次執行約 10 秒。

## 先讀

- [PROTOCOL.zh-TW.md](PROTOCOL.zh-TW.md)，在寫任何程式碼之前全部讀完。
- 階段 08，見 [`labs/08-ipc-event-loops/`](../08-ipc-event-loops/)：`SCM_RIGHTS`、封存的 memfd，以及搭配 `timerfd` 與 `signalfd` 的單一 `epoll` 迴圈。[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)：輔助資料在串流中形成的屏障、`MSG_CTRUNC` 與它關閉的 fd、`SO_PEERCRED`，以及 `EPIPE`、`SIGPIPE` 與 `MSG_NOSIGNAL`。[memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html) 與 [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html) 中的封存。[epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)、[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)。
- [實驗 03](../03-virtual-memory/README.zh-TW.md) 練習 E：映射底下的檔案被縮小時的 `SIGBUS`，以及無法縮小的封存 memfd。[實驗 06](../06-scheduling-latency/README.zh-TW.md) 練習 D：60 Hz 的 `timerfd` 喚醒在這台虛擬機器中晚多少。
- Wayland：[協定附錄](https://wayland.freedesktop.org/docs/html/apa.html)中的 `wl_surface`、`wl_buffer`、`wl_shm` 與 `wl_callback`，以及 [Wayland Protocol and Model of Operation](https://wayland.freedesktop.org/docs/book/Protocol.html) 中的線上格式。

## 檢查清單

| # | 要求 | 規格位置 | 測試框架如何檢查 |
| --- | --- | --- | --- |
| 1 | 監聽、`hello`/`welcome`、往下協商版本 | PROTOCOL，執行伺服器、版本 | setup：A 要求 1，必須得到 1；B 要求 7，必須得到 1 到 7；兩者都收到第一個 `frame_done` |
| 2 | 只接受已封存且夠大的緩衝區 | 緩衝區 | unsealed 階段：U 的兩個緩衝區都被以 `bad_buffer` 拒絕、連線關閉、伺服器存活 |
| 3 | 釋放用戶端留下的一切 | 線上格式（fd 佇列）、錯誤 | 每個問題階段之後：伺服器的 fd 數與 memfd 映射數（取自 `/proc/PID`）回到基準值 |
| 4 | attach、damage、commit、複製、release | Commit | A 的局部重繪；D 的三個狀態；最後 A 收到的 release 數等於它的 attach 數 |
| 5 | 不溢位地裁切 damage | Commit（damage） | D 狀態 1：恰好兩個小區域被更新（角落的 4 × 4、上緣的 4 × 5），其他都沒有 |
| 6 | 以混合替代實作、依 z 順序合成，stride、偏移量與 XRGB 都正確 | 緩衝區、合成 | 每個比對的影格都由 `check.py` 從各用戶端已知的畫面重新合成；每個通道容許差 1 |
| 7 | 60 Hz 的 `timerfd` vblank、`frame_done(seq, time)` | Vblank | 每個階段：A 的 `frame_done` 事件、它錯過的 vblank、每個事件晚到多少，以及時間是否落在 60 Hz 的格點上 |
| 8 | 有變化的影格寫成 PPM，在其 `frame_done` 之前寫好，並以改名方式就位 | 執行伺服器 | 每個比對的影格，是序號不大於檢查程式所需 `seq` 的最新檔案 |
| 9 | 絕不因某個用戶端而阻塞；佇列有上限 | 錯誤（背壓） | crash 階段：C 送出半個訊息後停 500 ms；noread 階段：N 大量送出 2 秒，然後檢查影格節奏與伺服器的峰值記憶體（`VmHWM`） |
| 10 | 撐過 `SIGPIPE` | 錯誤 | noread 階段：R 已關閉讀取端，所以伺服器的第一個回覆就以 `EPIPE` 失敗 |
| 11 | 收到 `SIGTERM` 時乾淨結束 | 執行伺服器 | end：3 秒內以結束狀態 0 結束 |

測試框架不檢查你的各項上限（物件、緩衝區記憶體、途中的 fd），也不檢查送出垃圾標頭或上千個 fd 的用戶端。練習 B 與階段 17 會檢查。

各表面畫在哪裡，每個字元代表 16 × 16 像素：

```
      x 0   80  160 240 320
y   0  ....................     A   good-anim         (16, 16)   160 × 120  z 0  不透明，stride 168 像素
   16  .AAAAAAAAAA...22222.     1   good-alpha B1     (96, 64)   128 × 96   z 1  半透明，偏移量 4096
   32  .AAAAAAAAAA...22222.     2   good-alpha B2     (232, 16)   64 × 48   z 2  XRGB8888，最高位元組是雜訊
   48  .AAAAAAAAAA...22222.     C   bad-crash         (200, 150)  96 × 72   z 3
   64  .AAAAA11111111......     N   bad-noread flood  (120, 176)  48 × 48   z 3
   80  .AAAAA11111111......     D   bad-damage        (16, 160)   64 × 64   z 3
   96  .AAAAA11111111.uuuu.     u   bad-unsealed      (250, 100)  48 × 40   z 3  絕不能出現
  112  .AAAAA11111111.uuuu.
  128  .AAAAA11111111.uuuu.
  144  ......111111CCCCCCC.
  160  .DDDD.......CCCCCCC.
  176  .DDDD..NNNN.CCCCCCC.
  192  .DDDD..NNNN.CCCCCCC.
  208  .DDDD..NNNN.CCCCCCC.
  224  ....................
```

## 你的專題，分成里程碑

建議的順序。每一步之後都執行測試框架；good 階段通過後，改用 `CHECK_ARGS="--only crash"`（或其他階段）。

1. **骨架。** 監聽 socket、`epoll` 迴圈、絕對 60 Hz 格點上的 `timerfd`、`signalfd`，以及在第一次 vblank 寫出只有背景的影格檔。像實驗 06 一樣，記錄每次 tick 晚了多少。
2. **訊息切分。** 每個用戶端的輸入緩衝區、只處理完整的訊息、`hello`/`welcome`、`sync`/`done`、物件表、`error` 之後關閉連線。
3. **緩衝區。** fd 佇列、封存與大小檢查、唯讀映射、關閉 fd。
4. **表面。** 待定與目前狀態、`commit`、依裁切後的 damage 複製、`release`、合成、PPM 檔、`frame_done`。此時 good 階段應該要通過。
5. **問題用戶端**，一次一個：`--only crash`、`--only noread`、`--only damage`、`--only unsealed`。
6. **三次完整通過的執行**，然後寫設計說明。

## 執行測試框架

```sh
sh harness/run.sh                         # 你放在 ./solution 的伺服器
SOLUTION_DIR=~/lab09 sh harness/run.sh    # 或其他位置
sh harness/run.sh ./server                # 或你建好的程式
CHECK_ARGS="--only damage" sh harness/run.sh
python3 harness/check.py --help           # 所有選項與預設值
```

`check.py` 對同一個伺服器行程跑五個階段。A 與 B 全程保持連線，而 A 每收到一個 `frame_done` 就 commit 一次，所以每次 vblank 都應該傳到它那裡。每個階段之後，檢查程式把 A 當作屏障：A 顯示新畫面，檢查程式再等三次 vblank，然後記下要比對哪個影格檔。這樣一來，剛死掉的用戶端已經有時間消失。影格在執行結束後才比對，這樣合成預期影像就不會影響影格節奏。

### 容許範圍

每個階段，檢查程式都讀取 A 的紀錄。若 `frame_done` 抵達 A 的時間比它標示的 vblank 時間晚超過 `--late-ms`（8 ms），就算遲到。錯過的 vblank 數由 `seq` 的間隔算出。一個階段在遲到事件不超過 `--max-late`（3）個、錯過的 vblank 不超過 `--max-skipped`（2）個時通過。遲到時間包含伺服器的喚醒、合成、送出，以及 A 自己的喚醒。這些預設值為虛擬機器中單次的排程異常留了餘地。實驗 06 在這個容器中量到，有負載時 60 Hz 的 `timerfd` 喚醒長尾達 4–15 ms。撰寫本實驗時的一次探索性執行（不在紀錄檔中）有一個 `frame_done` 晚了 19.5 ms，並錯過一次 vblank。會停住的伺服器則差得很遠，如下一張表所示。在實體硬體上應該收緊這些值。noread 階段另外允許伺服器的峰值常駐記憶體增加 `--max-growth-mib`（16 MiB）。

### 記錄的執行顯示了什麼

參考實作連續執行三次（設定 `SOLUTION_DIR` 後執行 `sh harness/run.sh`），[已記錄](harness/example-output.txt)：

| 階段 | 長度 | A 收到的 frame_done | 錯過的 vblank | 遲到（> 8 ms） | 遲到時間，中位數 | 最大值 |
| --- | --- | --- | --- | --- | --- | --- |
| good | 1.17 s | 70 / 70 / 70 | 0 / 0 / 0 | 0 / 0 / 0 | 0.29 / 0.24 / 0.26 ms | 1.14 / 1.03 / 1.03 ms |
| crash | 0.62 s | 37 / 37 / 37 | 0 / 0 / 0 | 0 / 0 / 0 | 0.27 / 0.27 / 0.30 ms | 0.94 / 0.93 / 1.27 ms |
| noread | 2.58 s | 155 / 155 / 155 | 0 / 0 / 0 | 0 / 0 / 0 | 0.23 / 0.21 / 0.21 ms | 0.81 / 1.09 / 0.72 ms |
| damage | 0.13 s | 8 / 8 / 8 | 0 / 0 / 0 | 0 / 0 / 0 | 0.60 / 0.57 / 0.62 ms | 1.01 / 0.78 / 0.93 ms |
| unsealed | 0.08 s | 5 / 5 / 5 | 0 / 0 / 0 | 0 / 0 / 0 | 0.19 / 0.14 / 0.20 ms | 0.67 / 0.79 / 0.71 ms |

- **三次執行的 43 項檢查全部通過。** 比對的 10 個影格全部吻合，伺服器每次執行寫出 15 個影格檔，每次變化一個。它自己的紀錄顯示錯過 0 次 vblank，最晚的一次 tick 比預定時間晚 0.32–0.67 ms。
- **真正考驗影格節奏的是 noread 階段：** 2.6 秒裡有一個用戶端全速寫入。參考實作在 N 寫了 369–422 KB 後把它斷線。三次執行中 N 看到的都是 `ECONNRESET`，因為伺服器關閉 socket 時 N 的請求還沒被讀完；撰寫本實驗時的一些較早執行則看到 `EPIPE`。伺服器的峰值記憶體維持在 2.4–2.5 MiB 的基準值。R 每次都被斷線，伺服器沒有受到傷害。
- **damage 與 unsealed 階段很短**（5–8 次 vblank），它們的影格節奏數字只涵蓋那段時間。damage 階段的中位數最高（0.57–0.62 ms），原因沒有調查。
- **crash 階段包含 C 送出半個 commit 後的 500 ms。** 伺服器保留那 6 個位元組，回到 `epoll_wait`，所以 A 沒有看到間隔。
- **這些是虛擬機器中的數字**，來自負載很輕的機器，而且每次記錄都不同：撰寫本實驗時的其他幾組三次執行，中位數介於 0.10 與 0.8 ms 之間，最大值到 1.6 ms。A 的遲到時間包含合成、一個影格檔、一次送出，以及另一個行程的喚醒，所以高於實驗 06 閒置時 `timerfd` 的中位數（41–110 µs）。

### 測試框架抓得到什麼

同一個參考實作，每次刻意加入一個錯誤（`-DBUG_...`，不隨附），各執行一次：

| 錯誤 | 哪裡失敗 |
| --- | --- |
| 阻塞式 socket 與 `send` | noread 的影格節奏：錯過 119 次 vblank，間隔 2.0 秒（伺服器卡在對 N 的 `send`，直到檢查程式殺掉 N） |
| 以阻塞式讀取補完半個訊息 | crash 的影格節奏：錯過 29 次 vblank，間隔 501 ms（C 停的 500 ms） |
| 沒有上限的輸出佇列 | noread 的記憶體：`VmHWM` 在 2 秒內增加 1,076 MiB |
| 沒有忽略 `SIGPIPE`，也沒有 `MSG_NOSIGNAL` | R 連線時伺服器被 `SIGPIPE` 殺掉 |
| 沒有檢查封存 | U 的未封存緩衝區被接受；U 把它截斷後，伺服器在它的 commit 時被 `SIGBUS` 殺掉 |
| 沒有檢查大小 | U 的未封存緩衝區被拒絕，太小的那個被接受；伺服器在它的 commit 時被 `SIGBUS` 殺掉 |
| 沒有裁切 damage | 伺服器在 D 的狀態 1 被 `SIGSEGV` 殺掉 |
| 以 32 位元加法裁切 damage | D 狀態 1：16 個像素錯誤，也就是 `x + width` 溢位的那個 4 × 4 角落 |
| 忽略 damage（每次都整個複製） | D 狀態 1：4,060 個像素錯誤 |
| 把 `stride` 當成 `4 × width` | 10 個影格錯誤：A 露出洋紅色的填充，疊在上面的 B1 也跟著不同 |
| 忽略緩衝區的 `offset` | 10 個影格錯誤：B1 所有看得到的部分 |
| 把 XRGB8888 的最高位元組當成 alpha | 10 個影格錯誤：B2 的 3,072 個像素中有 3,017 個 |
| 用戶端死掉時沒有關閉佇列中的 fd | crash 階段之後 fd 數為 10 而非 9，之後每個階段都是 |
| 用戶端死掉時沒有解除緩衝區映射 | memfd 映射數為 6–7 而非 4 |

在你需要之前，先完整讀過一次失敗的輸出。紀錄檔裡有每一種的樣子。

## 交付項目

1. **伺服器的原始碼**，能用 `make` 或 `run.sh` 使用的那一行 `gcc` 建置，除了 libc 之外不用其他函式庫。
2. **連續三次通過的** `sh harness/run.sh` 執行，附上包含影格節奏表的完整輸出，以及其中一次的 `server.log`。
3. **1–2 頁的設計說明**，依照下列大綱撰寫。
4. **一頁的 Wayland 比較**，依照其後的大綱撰寫。

### 設計說明大綱

1. **架構。** 以圖說明迴圈、其中的每個 fd、每個事件做什麼，以及每個用戶端的狀態。
2. **訊息處理。** 訊息切分、fd 佇列、每個請求如何驗證，以及你的錯誤處理原則。
3. **絕不阻塞，以及背壓。** 每次喚醒處理多少輸入、輸出佇列的上限、不讀取的用戶端會怎樣，以及原因。使用你 noread 階段的數字。
4. **影格節奏。** 計時器如何設定、計時器回報超過一次到期時你怎麼處理，以及你的 tick 遲到時間分布（中位數、p99、最大值），並與實驗 06 並列。
5. **緩衝區所有權與安全。** 封存、大小、何時讀取用戶端記憶體、何時釋放，以及為何 `SIGBUS` 碰不到你的伺服器。說明編碼值轉成線性值的工作，應該由合成器還是 T4 模組來做（[圖學課程介面](../../SYLLABUS.zh-TW.md#與圖學課程的介面)）。
6. **一個你診斷過的錯誤。** 症狀、最初的假設、證實或推翻假設的量測或紀錄，以及修正方式。
7. **限制與下一步。** 階段 17 的哪些攻擊用戶端，今天就能打倒你的伺服器。

## Wayland 比較

一頁篇幅，比較你的協定與 Wayland `wl_surface` 的 attach、damage、commit 與影格回呼。每一項關於 Wayland 的敘述，都要引用[協定附錄](https://wayland.freedesktop.org/docs/html/apa.html)與[線上格式](https://wayland.freedesktop.org/docs/book/Protocol.html)。要涵蓋的重點如下，每一點都已在 2026-11-16 對照這些頁面查核：

- **線上格式。** 兩個協定都先放一個傳送者或目標的 id 字組，接著一個字組，上 16 位元是大小（含標頭，至少 8），下 16 位元是 opcode。fd 走輔助資料，順序與攜帶它們的訊息相同。Wayland 的用戶端在 [2, 0xfeffffff] 配置 id，伺服器在 [0xff000000, 0xffffffff]。本協定沒有伺服器配置的 id。什麼情況下會需要它們？
- **緩衝區。** `wl_shm.create_pool` 交出一個 fd，由合成器映射。一個 pool 可以容納多個緩衝區，而 `wl_shm_pool.resize` 只能讓它變大。`wl_shm` 中沒有任何要求封存的規定。這裡每個緩衝區都有自己的 memfd，而且一定要封存。查出 libwayland-server 的 `wl_shm` 程式碼在用戶端把 pool 縮小時會怎麼做，再比較這兩種作法。
- **Attach 與 release。** 在 Wayland 中，合成器可以在 commit 之後的任何時間讀取緩衝區，直到它送出 `wl_buffer.release`。這裡的伺服器在 commit 時複製，並立刻釋放。兩者在複製次數、記憶體與延遲上各要付出什麼？兩邊 attach 空緩衝區都會移除內容。`wl_surface.attach` 另有 x 與 y 參數，自 `wl_surface` 版本 5 起必須為零，改由 `wl_surface.offset` 接手；兩者都是讓新緩衝區相對於舊緩衝區移動。這裡由用戶端以 `set_position` 決定表面在輸出上的位置。找出 Wayland 中有沒有哪個請求能讓用戶端決定自己的視窗出現在輸出的哪裡，並說明原因。
- **Damage。** `wl_surface.damage` 使用表面座標，附錄告訴新的用戶端改用以緩衝區座標表示的 `wl_surface.damage_buffer`。待定的 damage 是聯集，表面之外的部分會被忽略。這裡的 damage 以緩衝區座標表示，而且是精確的：它決定要複製哪些像素。在 Wayland 中，它是關於哪裡變了的資訊。哪一種設計讓合成器上傳得更少？哪一種讓它需要信任得更少？
- **Commit。** 兩者的表面狀態都是雙重緩衝的，而 Wayland 會先套用緩衝區，再套用其他狀態。本協定 commit 的步驟順序也一樣嗎？
- **影格回呼。** `wl_surface.frame` 建立一個 `wl_callback`，只觸發一次，在適合畫下一個影格的時候。它的資料是以毫秒為單位、起點未定義的時間，而且合成器不應該為看不見的表面觸發它。這裡的 `frame_done` 是表面上的事件，帶有 vblank 的序號與它在 `CLOCK_MONOTONIC` 上以微秒為單位的時間，即使表面被遮住也會觸發。有了序號，用戶端能做哪些用 Wayland 的時間做不到的事？階段 18 的 `presentation-time` 回答了一部分。
- **錯誤、sync 與 id。** 兩邊的 `wl_display.error` 都是致命的。`wl_display.sync` 回傳一個 `wl_callback`，而本協定回傳 `display.done`。Wayland 以 `wl_display.delete_id` 確認每個被釋放的用戶端 id。這防止了什麼競爭？為何本協定可以立刻釋放 id？
- **版本。** Wayland 為每個介面分別管理版本：全域物件公告它的最高版本，`wl_registry.bind` 選定新物件的版本。這裡整條連線只有一個版本。對一個有數十個擴充的合成器來說，按介面管理版本換到了什麼？

最後附上階段 11 的對照表：本協定的每個訊息，旁邊是做同一件事的 Wayland 請求或事件，或「無」。

## 延伸練習

- **A — 版本 2。** 設計並實作版本 2。例如：`surface.set_opacity`、一個同時回報影格晚了多少的 `frame_done`，或住在 pool 裡的 `buffer`。協商它，寫一個版本 2 的用戶端，並證明測試框架的版本 1 用戶端仍然通過。
- **B — 更多攻擊用戶端（階段 17 的預習）。** 大小為 0 或 65532 的標頭、一千個 `create_surface` 請求、送出卻沒有請求來取用的 fd、單次 commit 中 10,000 個 damage 矩形、不斷建立 4096 × 4096 的緩衝區直到某處撐不住。把每一個加入你自己那份 `check.py`，作為一個階段。各是哪個上限擋下了它？
- **C — 零複製。** 直接從用戶端的映射合成，保留緩衝區直到使用它的影格寫好，再釋放。量測你省下的複製時間，並說明封存現在保護了什麼、沒保護什麼。
- **D — 輸出的 damage。** 只重新合成影格中受損的區域，並回報每個影格合成了多少像素。靜止的桌面應該幾乎不花成本。
- **E — 有負載時。** 在每個核心上放一個吃 CPU 的程式（實驗 06 的負載執行緒），伺服器先用 `SCHED_OTHER`，再用 `SCHED_FIFO`，執行測試框架。各有多少個遲到的 `frame_done`？這裡沒有執行。
- **F — 真正的合成模組。** 當圖學課程的 T4 模組存在時，用它取代替代實作，讓混合在線性光下進行。修改 `check.py` 的 `compose()` 使其一致，並找出一個 B1 疊在 A 上、兩種結果相差超過 1 的像素。
- **G — 期限。** 不在 vblank 時合成，而在 vblank 減去一段餘裕時合成，回報錯過期限的影格，並量測在這台虛擬機器中餘裕最小可以到多少。

## 自我檢視

交出前確認：

- `run.sh` 連續三次通過，並且每個階段再以 `--only` 各通過一次。
- 設計說明中的每項原則（上限、不讀取的用戶端會怎樣）都存在於程式碼中，程式碼中也沒有設計說明沒提到的原則。
- 在 noread 階段對你的伺服器執行 `strace -f -e trace=sendmsg,recvmsg,sendto,recvfrom`，每個呼叫都回傳資料或 `EAGAIN`，從不睡眠。在容器中這需要 `--cap-add SYS_PTRACE`。
- 伺服器以 `-Wall -Wextra` 建置，沒有任何警告。
- `server.log` 對每一次斷線都說明是哪個用戶端、為什麼。

## 檢核

在 `NOTES.md` 中回答。

1. 證明你的伺服器撐過每個問題用戶端，並維持影格節奏：三次通過的執行，附上它們影格節奏數字的範圍。
2. 對每一種問題行為（送出半個訊息後當掉、大量送出、關閉讀取端、超出緩衝區的 damage、未封存的緩衝區、太小的緩衝區），指出會傷到天真伺服器的系統呼叫或信號，並指出你的伺服器中防止它的那一行。拿掉其中一個修正，用 `--only` 執行那個階段，展示失敗的樣子。
3. 你的伺服器還可能在哪裡阻塞？逐一檢視迴圈中的每個系統呼叫。寫影格檔就是其中之一：磁碟很慢或滿了會怎樣？你會怎麼把它移出迴圈？
4. 把你的 tick 遲到時間（中位數、p99、最大值）與實驗 06 練習 D 比較，並解釋差異。
5. 交出一頁的 Wayland 比較。

## 常見失敗情況

- **noread 階段出現 `server died: killed by SIGPIPE`。** 伺服器寫入了一個對方無法讀取的 socket。忽略 `SIGPIPE`，或以 `MSG_NOSIGNAL` 送出，並把 `EPIPE` 當作斷線。
- **unsealed 階段出現 `server died: killed by SIGBUS`。** 你接受了沒有 `F_SEAL_SHRINK` 的緩衝區，或小於 `offset + stride × height` 的緩衝區，然後讀取了它。
- **noread 的影格節奏失敗，間隔約 2 秒。** 伺服器卡在對 N 的 `send`，直到檢查程式殺掉 N。把每個用戶端 socket 都設為非阻塞。
- **crash 的影格節奏失敗，間隔約 500 ms。** 伺服器在等 C 送了一半的 commit 的其餘部分。把不完整的訊息留在該用戶端的緩衝區，回到 `epoll_wait`。
- **noread 的記憶體檢查失敗。** 你的輸出佇列沒有上限。
- **crash 階段之後 `fds back to the baseline` 失敗。** 你沒有關閉死掉的用戶端佇列中剩下的 fd，或映射緩衝區之後沒有關閉它的 fd。
- **A 的影格右側出現洋紅色條紋。** 你把 `4 × width` 當成 stride。A 的填充是洋紅色。
- **B1 整個錯了，其他都對。** 你忽略了 `offset`。
- **B2 出現破洞或色偏。** 你把 XRGB8888 的最高位元組當成 alpha 混合。
- **D 狀態 1 失敗，16 個像素錯誤。** `x + width` 在 32 位元下溢位了。若錯了大約 4,060 個，你是整個緩衝區複製而忽略了 damage。
- **`no frame file at or before seq`。** 影格檔不存在、名稱錯誤，或在它的 `frame_done` 之後才寫。
- **`the server did not create ... display.sock`。** 檢查參數：`--socket PATH --out DIR`。
- **`frame_done times on the 60 Hz grid` 失敗，或遲到時間為負。** 時間不是 `CLOCK_MONOTONIC` 上以微秒為單位的名義 vblank 時間，或高低兩半對調了。

## 後續會用到的地方

- **階段 11** 把本協定的每個訊息對應到做同一件事的 Wayland 請求。
- **階段 13** 以真正的翻頁事件取代 `timerfd`；`frame_done` 的路徑保留。
- **階段 14** 以 dma-buf 取代 memfd，以 fence 取代複製。
- **階段 17** 攻擊合成器，它的測試腳本就從這些問題用戶端開始。
- **階段 18** 的合成器有同樣的每用戶端狀態、同樣的絕不阻塞規則與同樣的影格迴圈，只是建在 DRM/KMS 上。

## 下一步

第二部分從[階段 10](../../SYLLABUS.zh-TW.md) 開始：在 Windows 上以 PresentMon 量測 DWM 的呈現路徑。
