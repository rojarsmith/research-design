# 05 來源與查證邊界

查閱日期：2026-10-19。以下來源用於支撐 01 至 04 的敘述。歷史年份以第一手回顧（Ritchie、Thompson）與標準組織頁面為準；若二手資料與之衝突，以第一手為準。網址可能變動，請以標題搜尋替代。

## 第一手歷史文獻

| 來源 | 支撐的內容 | 備註 |
| --- | --- | --- |
| Dennis M. Ritchie, "The Development of the C Language" (1993), https://www.bell-labs.com/usr/dmr/www/chist.html | BCPL → B → C 的譜系、PDP-7 與 PDP-11 的影響、型別加入的動機、陣列與指標關係的由來、1973 年核心重寫 | 本主題歷史敘述的主要依據 |
| D. M. Ritchie, K. Thompson, "The UNIX Time-Sharing System", CACM 17(7), 1974 | Unix 首次對外完整描述、以 C 實作的事實 | |
| Ken Thompson, "Reflections on Trusting Trust", CACM 27(8), 1984, https://dl.acm.org/doi/10.1145/358198.358210 | 自舉造成的信任問題 | 圖靈獎演講 |
| B. W. Kernighan, D. M. Ritchie, *The C Programming Language*, 1978；第 2 版 1988 | K&R C 與 ANSI C 的差異 | |
| S. C. Johnson, D. M. Ritchie, "Portability of C Programs and the UNIX System", Bell System Technical Journal, 1978 | PCC 與 Interdata 8/32 移植、可攜性的工程驗證 | |
| Martin Richards, BCPL 文獻與網站，https://www.cl.cam.ac.uk/~mr10/ | BCPL 的年代與設計 | |

## 語言標準

| 來源 | 支撐的內容 | 備註 |
| --- | --- | --- |
| ISO/IEC JTC1/SC22/WG14 官方頁面，https://www.open-std.org/jtc1/sc22/wg14/ | 標準版本沿革、公開草稿編號 | 正式標準需購買；公開草稿（如 C23 的 N3220 系列）內容接近最終版 |
| ISO/IEC 9899:2024（C23） | 二補數為唯一整數表示、新語法 | 2024 年出版 |
| C 標準 5.1.2.3「Program execution」與 5.2.4.2.1「Sizes of integer types」 | 抽象機器、可觀察行為、`CHAR_BIT >= 8` 與整數最小範圍 | 各版本章節編號略有不同 |

## 編譯器與工具鏈

| 來源 | 支撐的內容 | 備註 |
| --- | --- | --- |
| GCC 發布歷史，https://gcc.gnu.org/releases.html | 1987 年 1.0 與後續版本 | |
| GCC Internals，https://gcc.gnu.org/onlinedocs/gccint/ | GENERIC／GIMPLE／RTL、機器描述檔、新增後端所需元件 | |
| GCC Named Address Spaces，https://gcc.gnu.org/onlinedocs/gcc/Named-Address-Spaces.html | AVR 的 `__flash` 等哈佛架構處理 | |
| GCC C Tests（torture 測試），https://gcc.gnu.org/onlinedocs/gccint/C-Tests.html | 後端正確性測試方法 | |
| Chris Lattner, "LLVM" in *The Architecture of Open Source Applications*, https://aosabook.org/en/v1/llvm.html | LLVM 的設計理念、以 IR 為中心、2000 年起源 | |
| LLVM 官方網站與文件，https://llvm.org/ | 版本與後端清單；`clang -print-targets` 可列出本機支援目標 | 本主題實測使用 Ubuntu clang 18.1.3 |
| Rui Ueyama, chibicc，https://github.com/rui314/chibicc | 逐 commit 建立 C 編譯器的教學實作 | tinyc 的結構受其影響 |
| Fabrice Bellard, TCC，https://bellard.org/tcc/ | 單遍 C 編譯器 | |
| SDCC，https://sdcc.sourceforge.net/ | 8051、Z80、PIC、STM8 等架構的 C 支援與限制（靜態區域變數、遞迴限制） | |
| Microchip XC8 編譯器使用手冊 | PIC 的編譯堆疊與重疊分析 | 廠商文件，版本需對應 |
| newlib，https://sourceware.org/newlib/ ；picolibc，https://github.com/picolibc/picolibc ；musl，https://musl.libc.org/ | 嵌入式與 hosted 的 C 函式庫選項、系統呼叫樁 | |
| Csmith，https://github.com/csmith-project/csmith | 隨機差分測試編譯器 | |
| Reproducible Builds，https://reproducible-builds.org/ | 與 Trusting Trust 相關的實務對策 | |

## 指令集與 ABI

| 來源 | 支撐的內容 | 備註 |
| --- | --- | --- |
| RISC-V 已批准規格，https://riscv.org/specifications/ratified/ | RV32I 基本指令、M 擴充、除以零的定義行為 | rv32sim 的語意依此實作 |
| RISC-V ELF psABI，https://github.com/riscv-non-isa/riscv-elf-psabi-doc | ILP32 資料模型、`a0` 至 `a7` 傳參、`sp` 16 位元組對齊、呼叫者／被呼叫者保存暫存器 | tinyc 遵守其子集 |
| RISC-V Assembly Programmer's Manual，https://github.com/riscv-non-isa/riscv-asm-manual | 偽指令（`li`、`la`、`call`、`ret`、`seqz` 等）的定義 | |
| RARS（RISC-V Assembler and Runtime Simulator），https://github.com/TheThirdOne/rars | `ecall` 編號慣例（1 印整數、11 印字元、10／93 結束） | rv32sim 採相同慣例以便互相驗證 |
| Arm AAPCS32，https://github.com/ARM-software/abi-aa | 另一份公開 ABI，用於比較 | |
| System V AMD64 ABI，https://gitlab.com/x86-psABIs/x86-64-ABI | 參數暫存器 `rdi rsi rdx rcx r8 r9`、呼叫時 16 位元組對齊、LP64 | tinyc 的 `X86_64` 後端遵守其子集 |
| QEMU RISC-V 系統模擬文件，https://www.qemu.org/docs/master/system/target-riscv.html | 真實工具鏈輸出的執行環境 | 本環境未安裝 QEMU 與 RISC-V GCC，因此改以 clang 整合組譯器驗證 |

## 教材

| 來源 | 用途 |
| --- | --- |
| A. Aho, M. Lam, R. Sethi, J. Ullman, *Compilers: Principles, Techniques, and Tools*, 2nd ed., 2006 | 編譯器理論的標準參考 |
| A. Appel, *Modern Compiler Implementation in C*, 1998 | 以 C 實作完整編譯器 |
| R. Nystrom, *Crafting Interpreters*, https://craftinginterpreters.com/ | 遞迴下降解析與單遍生碼的清楚說明 |
| D. Patterson, J. Hennessy, *Computer Organization and Design, RISC-V Edition* | ISA、ABI 與處理器實作之間的關係 |
| Nand2Tetris，https://www.nand2tetris.org/ | 從邏輯閘到編譯器的一貫課程，對應本主題的「融會貫通」 |

## 本環境實測紀錄

- 主機：Ubuntu 24.04，gcc 13.3.0，clang 18.1.3（含 riscv32 後端與整合組譯器），python3。
- `labs/tinyc/run_tests.py`：五個範例在四條路徑（tinyc RV32 → rv32sim 並啟用 `--check-align`、主機 gcc 原生、tinyc x86-64 → 主機 `cc` 原生、clang -O2 riscv32 → rv32sim）輸出與結束碼一致。
- `labs/probe/probe.c` 於主機的輸出：`CHAR_BIT 8`、LP64、小端序、`-1 >> 1` 為算術位移、`-7 / 2 == -3`。
- 未能實測：真實 RISC-V 硬體或 QEMU 執行、`gcc -m32`（缺 32 位元標頭）、8051／AVR 工具鏈。相關敘述來自文件，標示於各篇。

## 查證邊界

- 年份精確到年；月份只在來源明確時給出。
- 編譯器效能比值（例如 tinyc 與 clang -O2 的指令數差距）來自本環境的模擬器計數，只能代表相對趨勢。
- 各處理器類別的工具鏈限制可能隨版本改變，使用前請對照當時版本的手冊。
