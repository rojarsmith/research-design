# 桌機實驗

還沒有開發板時，先用這些小程式看清機制：誰搬資料、誰決定時間、誰擁有記憶體、失敗時看起來像什麼。每個實驗只需要一個 C11 編譯器，一行命令建置，並附自我檢查。

模型能證明邏輯與策略是否正確，例如解析器能否從雜訊恢復、超載時遺失是否被計數。它不能證明電氣訊號、真實的中斷延遲、時脈精度與功耗；這些要到上板實作時量測。

| 實驗 | 對應課程 | 建置與執行 |
| --- | --- | --- |
| [01-reset-to-main](01-reset-to-main/) | [第二課](../lessons/01-reset-to-main.md) | `gcc -std=c11 -Wall -Wextra -Werror -O2 -o boot_sim boot_sim.c`，再執行 `./boot_sim --test` |
| [02-gpio-registers](02-gpio-registers/) | [第三課](../lessons/02-gpio-and-clocks.md) | `gcc -std=c11 -Wall -Wextra -Werror -O2 -o gpio_model gpio_model.c`，再執行 `./gpio_model --test` |
| [03-timer-scheduler](03-timer-scheduler/) | [第四課](../lessons/03-timer-and-interrupts.md) | `gcc -std=c11 -Wall -Wextra -Werror -O2 -o sched_sim sched_sim.c`，再執行 `./sched_sim --test` |
| [04-uart-pipeline](04-uart-pipeline/) | [第五課](../lessons/04-uart-ring-buffer.md) | 見第五課；共有三個程式，各一行建置 |

各課還有不帶自我檢查的觀察型實驗，例如 map file 與延遲迴圈，命令寫在課文裡。

## 環境

- 已在 Windows 11 上以 GCC 16.2（WinLibs MinGW-w64 UCRT）建置並執行全部實驗，同時用 -O0 與 -O2 檢查過。Linux 與 macOS 尚未實測，原始碼只用標準 C11，另外在 Windows 上呼叫一次 `SetConsoleOutputCP` 讓主控台正確顯示中文。
- 命令在 PowerShell 與 Git Bash 都能用。Windows 產生的執行檔帶 `.exe`，執行時可省略。
- 建置產物與存下來的 `.bin` 量測檔已列在 [.gitignore](.gitignore)，不會進入版本控制。

## 怎麼用這些程式

先讀原始碼開頭的說明，對每個情境寫下預測，再執行比對。預測錯的地方記進 [學習紀錄](../learning-log.md)，那是最值得追的部分。

自我檢查全部通過之後，試著改壞一處，例如把環形緩衝區的 release 改成 relaxed、把去彈跳門檻改成 2 ms、把逾期策略改成重新起算，看看哪一項檢查會失敗、為什麼。有些錯誤在單執行緒的桌機模擬裡看不出來，這也是一個要記下來的發現。
