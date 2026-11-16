[English](PROTOCOL.md) | **繁體中文** | [實驗 09](README.zh-TW.md)

# 實驗 09 協定，版本 1

這是你的伺服器要實作的協定。[`harness/`](harness/) 中的測試用戶端說這個協定，[`harness/check.py`](harness/check.py) 依照它檢查你的伺服器。[`harness/lab09-protocol.h`](harness/lab09-protocol.h) 為下面每個數值取了 C 名稱。README 說明了[為何協定是固定的](README.zh-TW.md#為何協定是固定的)而不是由你設計，練習 A 則要你設計版本 2。

「必須」標示測試框架會檢查或依賴的事項。「可以」標示由你決定的選擇；請寫進你的設計說明。

## 執行伺服器

```
server --socket PATH --out DIR
```

- **Socket。** 在 `PATH` 上以 Unix domain stream socket 監聽。先移除 `PATH` 上殘留的檔案。依序接受任意數量的用戶端；同時至少 16 個。
- **輸出。** 一個模擬的 320 × 240 像素、60 Hz 輸出。影格緩衝區是預乘 ARGB8888，每次合成前清為 `0xff202030`（不透明；R 0x20、G 0x20、B 0x30）。
- **影格。** 把合成的影像寫成 `DIR/frame-NNNNNN.ppm`，其中 `NNNNNN` 是該影格的 `seq`（見下文），十進位，前面補零到至少六位。格式是二進位 PPM：標頭 `P6\n320 240\n255\n`，接著 320 × 240 個 RGB 三元組，列由上到下，捨去 alpha 位元組。`DIR` 在伺服器啟動前就已存在。
  - 每一次合成影像有變化的 vblank，你都必須寫出一個影格。你可以寫得更多（例如每個影格都寫）；檢查程式永遠讀取 `seq` 不大於所需值的最新檔案。
  - 每個檔案先以暫時的名稱寫在 `DIR` 中，再用 `rename(2)` 放到定位，讓讀取者永遠不會看到半個檔案。
  - 影格 `n` 的檔案必須在伺服器送出任何 `seq` 為 `n` 的 `frame_done` 之前寫完。
- **關閉。** 收到 `SIGTERM` 或 `SIGINT` 時，關閉一切，並在一秒內以狀態 0 結束。
- **紀錄。** stdout 或 stderr 上寫什麼都可以，`check.py` 會存成 `server.log`。每次連線與每次斷線各一行並附上原因，除錯會容易得多。

## 線上格式

一條連線在兩個方向上各傳送一串訊息。請求從用戶端到伺服器，事件從伺服器到用戶端。每個訊息都是整數個以主機位元組順序表示的 32 位元字組：

| 字組 | 內容 |
| --- | --- |
| 0 | 訊息的目標物件 id（請求）或來源物件 id（事件） |
| 1 | `size << 16 \| opcode`：`size` 是以位元組計的訊息長度，含標頭；`opcode` 是該物件介面中請求或事件的編號 |
| 2… | 參數，每個一個字組 |

- `size` 是 4 的倍數，至少 8，在版本 1 中至多 64。伺服器必須把其他任何大小當作 `invalid_message`。
- 參數型別：`int`（有號 32 位元）、`uint`（無號 32 位元）、`new_id`（用戶端為新物件選的 `uint`）、`object`（指名既有物件的 `uint`，0 表示沒有）。
- **檔案描述元**不出現在位元組中。需要 `fd` 的請求，從 socket 的 `SCM_RIGHTS` 輔助資料取得它（[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)）。用戶端把每個 fd 與需要它的請求的第一個位元組放在同一次 `sendmsg` 中送出。伺服器把收到的每個 fd 依抵達順序加入該連線的佇列，每個需要 fd 的請求取走最舊的那個。佇列為空時收到需要 fd 的請求，就是 `no_fd`。連線結束時仍留在佇列中的 fd 必須關閉。
- 訊息可能被拆在多次讀取中抵達，一次讀取也可能包含多個訊息。在湊齊完整訊息之前，先把位元組排隊保存。

## 物件與 id

- 物件 1 是 **display**，每條連線一開始就存在。
- 用戶端為它建立的每個物件選擇 id：2 到 65535，且在這條連線上未被使用。否則就是 `invalid_id`。
- id 屬於連線。一個用戶端無法指名另一個用戶端的物件。
- `destroy` 立即釋放 id；用戶端可以在下一個請求中重複使用它。
- 對不存在的 id 發出請求，或使用該介面沒有的 opcode，分別是 `invalid_object` 或 `invalid_method`。必須指名緩衝區的參數卻指向別的東西，是 `invalid_object`。

## 版本

- 連線上的第一個請求必須是 `display.hello(version)`。先送其他請求，或送第二個 `hello`，都是 `invalid_message`。`version` 為 0 時是 `version`。
- 伺服器回覆 `display.welcome(v, 320, 240, 60000)`，其中 `v` 是用戶端的 `version` 與伺服器實作的最高版本兩者中較小的。`v` 適用於整條連線。
- 較新的版本保留較舊版本的每個訊息不變。新的請求與事件使用新的 opcode。伺服器絕不送出比 `v` 新的事件，並把比 `v` 新的請求當作 `invalid_method`。

## 介面

### display（物件 1）

| Opcode | 請求 | 參數 | 大小 | 作用 |
| --- | --- | --- | --- | --- |
| 0 | `hello` | `uint version` | 12 | 必須最先送出；以 `welcome` 回覆 |
| 1 | `sync` | `uint serial` | 12 | 所有較早的請求都處理完後，以 `done(serial)` 回覆 |
| 2 | `create_surface` | `new_id surface` | 12 | 一個沒有內容、位於 (0, 0)、z 為 0 的新表面 |
| 3 | `create_buffer` | `new_id buffer`、`fd`（資料之外）、`int offset`、`int width`、`int height`、`int stride`、`uint format` | 32 | 一個新緩衝區：像素 (x, y) 是 fd 所指檔案中位元組 `offset + y × stride + 4 × x` 處的 32 位元字組 |

| Opcode | 事件 | 參數 | 大小 | 時機 |
| --- | --- | --- | --- | --- |
| 0 | `welcome` | `uint version`、`uint width`、`uint height`、`uint refresh_mhz` | 24 | 回覆 `hello` |
| 1 | `done` | `uint serial` | 12 | 回覆 `sync` |
| 2 | `error` | `object id`、`uint code` | 16 | 致命錯誤；伺服器送出後關閉連線 |

### surface

| Opcode | 請求 | 參數 | 大小 | 作用 |
| --- | --- | --- | --- | --- |
| 0 | `destroy` | | 8 | 表面從下一個影格消失；不會再為它送出 `frame_done` |
| 1 | `attach` | `object buffer`（0：移除內容） | 12 | 待定：要顯示的緩衝區 |
| 2 | `set_position` | `int x`、`int y`、`int z` | 20 | 待定：在輸出上的左上角，以及堆疊順序（z 較大者在上） |
| 3 | `damage` | `int x`、`int y`、`int width`、`int height` | 24 | 待定：緩衝區中有變化的矩形，以緩衝區像素為單位 |
| 4 | `frame` | | 8 | 待定：要求一個 `frame_done` |
| 5 | `commit` | | 8 | 一次套用所有待定狀態（見下文） |

| Opcode | 事件 | 參數 | 大小 | 時機 |
| --- | --- | --- | --- | --- |
| 0 | `frame_done` | `uint seq`、`uint time_us_hi`、`uint time_us_lo` | 20 | 影格 `seq` 合成並寫好之後，針對包含 `frame` 的 commit |

### buffer

| Opcode | 請求 | 參數 | 大小 | 作用 |
| --- | --- | --- | --- | --- |
| 0 | `destroy` | | 8 | 伺服器忘掉這個緩衝區；表面保持原本顯示的內容；它的待定 `attach` 被捨棄 |

| Opcode | 事件 | 參數 | 大小 | 時機 |
| --- | --- | --- | --- | --- |
| 0 | `release` | | 8 | 在再次被 attach 之前，伺服器不會再讀取這個緩衝區 |

## 緩衝區

除非下列條件全部成立，`create_buffer` 必須以 `display.error(緩衝區 id, bad_buffer)` 拒絕該緩衝區，並關閉連線：

1. `fcntl(fd, F_GET_SEALS)` 成功，且包含 `F_SEAL_SHRINK`（[memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html)、[fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)）。這要在檢查大小*之前*確認：沒有封存，大小在你看過之後仍可能改變。
2. 檔案至少有 `offset + stride × height` 個位元組（`fstat`），計算時不得溢位。
3. `1 ≤ width ≤ 4096`、`1 ≤ height ≤ 4096`、`offset ≥ 0`、`stride ≥ 4 × width`，且 `stride` 是 4 的倍數。
4. `format` 是 0（ARGB8888）或 1（XRGB8888）。其他值是 `bad_format`，而不是 `bad_buffer`。

格式使用 `wl_shm` 的數值（[協定](https://wayland.freedesktop.org/docs/html/apa.html)）：每個像素是以主機位元組順序表示的 32 位元字組，第 31–24 位元是 A（或 x），接著是 R、G、B。ARGB8888 是預乘的：每個色彩通道都已乘上 alpha，所以沒有任何通道超過 alpha。XRGB8888 的最高位元組沒有意義：不論內容是什麼，都把像素當作不透明。

fd 由伺服器負責關閉。檢查成功之後，伺服器可以把檔案以唯讀（`MAP_SHARED`）映射，然後關閉 fd；映射仍然有效。

## Commit

commit 依下列順序套用表面的待定狀態，然後清空它：

1. **Attach。** 若 commit 帶有 `attach(0)`，表面失去內容，不再被繪製。若帶有 `attach(B)`，伺服器從 `B` 把像素複製到它自己的表面複本中：
   - 若 `B` 不是這個表面上一次顯示的緩衝區，或 `B` 的尺寸與表面不同，就複製整個 `B`；
   - 否則只複製受損的像素（見下文）。其他每個像素都保留表面先前顯示的內容。

   接著伺服器送出 `B.release`。伺服器在 attach 某個緩衝區的那次 commit 之後，絕不再讀取它，所以用戶端可以在伺服器顯示複本的同時，畫進已被釋放的緩衝區。
2. **位置。** `set_position` 生效。
3. **Frame。** 待定的 `frame` 請求，變成一個在下一次 vblank 送出一個 `frame_done` 的承諾。

同一次 commit 中沒有 `attach` 的 damage 會被捨棄。

**Damage。** 矩形以緩衝區像素為單位。`width ≤ 0` 或 `height ≤ 0` 的矩形是空的。其他每個矩形都以精確的算術裁切到 `[0, 緩衝區寬度) × [0, 緩衝區高度)`：`x + width` 可能超過 32 位元，所以要用 64 位元計算。一次 commit 的 damage 是各裁切後矩形的聯集。伺服器每次 commit 必須精確保存至少 16 個矩形；超過時，可以把這次 commit 當作整個緩衝區都受損。

## Vblank、合成與 frame_done

- **Vblank** `n`（n = 1, 2, …）發生在 `CLOCK_MONOTONIC` 上的 `t₁ + (n − 1) × 16 666 667 ns`，其中 `t₁` 在伺服器啟動時選定。使用第一次到期為絕對時間的週期性 `timerfd`（[timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html)），讓格點不會漂移。若伺服器醒得太晚，計時器回報 `k > 1` 次到期，`seq` 就前進 `k`：錯過的 vblank 對用戶端而言是可見的間隔。
- **影格** `n` 包含伺服器在處理 vblank `n` 之前處理過的每一次 commit。
- **合成。** 先清為背景，再依 z 由小到大（z 相同的表面順序由伺服器決定），把每個有內容的表面畫在它的位置上，裁切到輸出範圍內。位置可以是 `int` 範圍內的任何值。每個像素都以[圖學課程介面](../../SYLLABUS.zh-TW.md#與圖學課程的介面)的替代實作混合，也就是在編碼值上逐通道（A、R、G、B）做預乘的「over」：

  ```
  out = src + round(dst × (255 − src.alpha) / 255)，最大 255
  ```

  `check.py` 容許每個通道相差 1，所以任何四捨五入到最接近值的作法都可以。這是在編碼值上混合；線性光下的混合是圖學課程 T4 模組的工作（練習 F）。
- **frame_done。** 影格 `n` 合成完成且（若有變化）檔案寫好之後，伺服器為每個已提交狀態中要求了 frame 的表面，送出一個 `frame_done(n, t_hi, t_lo)`。`t` 是 vblank `n` 在 `CLOCK_MONOTONIC` 上以微秒為單位的名義時間，拆成高低各 32 位元。每個 `frame` 請求恰好產生一個 `frame_done`。

## 錯誤與斷線

| 代碼 | 名稱 | 時機 |
| --- | --- | --- |
| 1 | `invalid_object` | 未知的物件，或型別錯誤的物件 |
| 2 | `invalid_method` | 該物件介面中未知的 opcode |
| 3 | `invalid_message` | 大小錯誤；缺少或重複 `hello` |
| 4 | `invalid_id` | `new_id` 超出範圍或已被使用 |
| 5 | `no_fd` | 需要 fd 的請求，但佇列中沒有 fd |
| 6 | `bad_buffer` | 上述緩衝區檢查失敗 |
| 7 | `bad_format` | 未知的格式 |
| 8 | `no_resources` | 達到伺服器的某個上限（物件、緩衝區記憶體、途中的 fd） |
| 9 | `version` | 以版本 0 送出 `hello` |

- 送出 `error` 之後，伺服器關閉連線。錯誤是最後一個事件；如果 socket 收得下就送出它，但絕不為此等待。
- 對方掛斷，或送出失敗（`EPIPE`、`ECONNRESET`）時，伺服器也關閉連線。
- **背壓。** 伺服器絕不能因某個用戶端而阻塞：不能在 `recv` 中、不能在 `send` 中，也不能在訊息尚未完整時。socket 現在收不下的事件，放在每個用戶端一個、大小有上限的佇列中等待。當用戶端讓這個佇列滿了，伺服器要嘛關閉連線，要嘛在佇列消化之前停止讀取該用戶端的請求。兩者都允許；選一個，並在設計說明中說明理由。
- 不論用戶端送出什麼位元組、什麼 fd，或在任何時刻消失，伺服器都必須撐住。
- 你的各項上限（每個用戶端的物件數、每個用戶端的緩衝區記憶體、途中的 fd、用戶端數）由你決定；超過任何一項就是 `no_resources`。

## 版本 1 沒有的東西

與 Wayland 相比（[協定](https://wayland.freedesktop.org/docs/html/apa.html)）：沒有 registry 或全域物件，沒有角色或 shell（位置由用戶端設定），沒有輸入，沒有區域、縮放、變換或子表面，沒有回呼物件（`frame_done` 與 `done` 都是既有物件上的事件），沒有 `delete_id`，只有一個輸出，除了 `frame_done` 的時間之外也沒有呈現回饋。[Wayland 比較](README.zh-TW.md#wayland-比較)要你說明每一項的代價或好處。
