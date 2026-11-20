# 工具鏈：WinLibs GCC 16.2.0 的版本怎麼選

紀錄日期：2026-11-15。環境：Windows 11。版本資料來自 WinLibs 下載清單、`winget search WinLibs`，以及本機安裝後執行 `g++ -v` 的輸出。

## 1. 結論：首選哪一個

**建議首選：Win64、POSIX threads、UCRT、不含 LLVM、Zip 格式。**

```
winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3.zip
```

更省事的做法是用 Windows 11 內建的 winget 安裝**同一個建置**。不用自己解壓縮，也不用手動設定 PATH：

```bash
winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact
```

裝完要重開 VS Code 或終端機，新的 PATH 才會生效。

選這個的理由：

| 選擇 | 理由 |
| --- | --- |
| Win64（x86_64） | LeetCode 和 HackerRank 的評測機都是 64 位元 Linux 上的 g++，64 位元最接近。32 位元程式的指標只有 4 bytes，位址空間也只有 2 GB 左右。 |
| POSIX threads | 相容性最好，是 WinLibs 和 winget 的主流版本。刷題幾乎不會用到多執行緒，不需要 MCF 的效能優勢。 |
| UCRT | Windows 10 以上內建新版的 C 執行時期函式庫，Windows 11 不必另外安裝任何 runtime |
| 不含 LLVM | 這個專案只用 g++ 和 gdb。含 LLVM 版本體積大得多，而且多出的工具用不到。 |
| Zip | 內容和 7-Zip 版相同，但 Windows 內建就能解壓縮，不用多裝 7-Zip |

## 2. 為什麼同樣是 16.2.0 會有這麼多版本

清單上的每一行 GCC 版本都一樣，差別在**建置時的幾個獨立選項**。這些選項的組合，就成了很多個下載連結。

| 選項 | 可選值 | 意思 | 練習時要不要在意 |
| --- | --- | --- | --- |
| 目標架構 | Win32（i686）、Win64（x86_64） | 產生 32 位元或 64 位元程式 | 選 64 位元 |
| 例外處理機制 | dwarf（i686）、seh（x86_64） | C++ 拋出例外時如何回溯堆疊。32 位元用 DWARF-2 unwinding，64 位元用 Windows 原生的 SEH。**這一項由架構決定**，所以檔名裡的 `dwarf`、`seh` 不是另外的選項。 | 不用選 |
| 執行緒模型 | POSIX（winpthreads）、MCF（mcfgthread） | `std::thread`、`std::mutex` 底層的實作。POSIX 版用 mingw-w64 的 winpthreads 模擬 pthread，相容性最好。MCF 版用 mcfgthread 直接呼叫 Windows 原生的同步機制，效能較好、也較新，但編出的程式要附帶 mcfgthread 的 DLL，或者靜態連結。 | 選 POSIX |
| C 執行時期函式庫 | UCRT、MSVCRT | 這份清單裡都是 UCRT。WinLibs 另外有給舊版 Windows 用的 MSVCRT 版本。 | Windows 11 選 UCRT |
| 是否附 LLVM | 不含、含 LLVM/Clang/LLD/LLDB 23.1.3 | 含的版本多一套 clang 編譯器、lld 連結器、lldb 除錯器 | 只用 g++ 就選不含 |
| 壓縮格式 | 7-Zip、Zip | 內容相同，7-Zip 檔較小 | 選 Zip，少裝一個工具 |
| `release 3`（`r3`） | 打包修訂號 | 同一組版本的第 3 次建置，通常是修正打包或更新相依套件。GCC 本身的版本並沒有不同。 | 選最新的 r |
| `MinGW-w64 14.0.0` | 固定 | Windows 標頭檔和匯入函式庫的版本 | 不用選 |

**算一下連結數。**

- POSIX：2 種架構 × 是否含 LLVM 2 種 × 2 種壓縮格式，共 8 個連結。
- MCF：2 種架構 × 2 種壓縮格式，共 4 個連結。MCF 沒有提供含 LLVM 的版本。
- 一共 12 個下載連結，但實際上只有 6 種不同的建置。壓縮格式不影響內容。

**從檔名就能讀出所有選項。** 以首選版本為例：

```
winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3.zip
        架構    執行緒 例外  GCC 版本  CRT 與 MinGW-w64 版本  修訂  格式
```

含 LLVM 的版本會在 GCC 版本後面多一段 `llvm-23.1.3`。

## 3. winget 上的對應套件

查詢日期 2026-11-15：

| winget Id | 版本 | 內容 |
| --- | --- | --- |
| `BrechtSanders.WinLibs.POSIX.UCRT` | 16.2.0-14.0.0-r3 | **首選**，和建議的 Zip 是同一個建置 |
| `BrechtSanders.WinLibs.POSIX.UCRT.LLVM` | 14.2.0-19.1.1-12.0.0-r2 | 含 LLVM，但版本落後很多 |
| `BrechtSanders.WinLibs.MCF.UCRT` | 16.1.0-14.0.0-r1 | MCF 執行緒，版本比清單上的舊 |
| `BrechtSanders.WinLibs.POSIX.MSVCRT` | 16.2.0-14.0.0-r3 | 舊版 C 執行時期函式庫，給舊 Windows |
| `BrechtSanders.WinLibs.POSIX.MSVCRT.LLVM` | 14.2.0-19.1.7-12.0.0-r3 | MSVCRT 加 LLVM，版本落後 |

如果一定要 clang 或 lldb，winget 上含 LLVM 的套件版本落後，建議改從 WinLibs 下載含 LLVM 的 Zip 手動安裝。

## 4. 本機實際安裝的結果

- `winget list`：`BrechtSanders.WinLibs.POSIX.UCRT 16.2.0-14.0.0-r3`
- `g++ --version`：`g++.exe (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders, r3) 16.2.0`
- `g++ -v`：`Thread model: posix`、`--disable-sjlj-exceptions`
- `gdb --version`：GNU gdb 18.1，在 PowerShell 裡執行時，內建的 Python 正常
- 安裝位置：`%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin`。winget 會把這個目錄加到使用者的 PATH。
- 磁碟用量：解壓後約 949 MB。裡面除了 g++、gdb，還附了 gfortran、yasm 等工具。

## 5. 和評測機的差異：在 Windows 練習要注意

| 差異 | Windows g++（MinGW） | LeetCode / HackerRank（Linux g++） | 怎麼做 |
| --- | --- | --- | --- |
| `long` 的大小 | 4 bytes（LLP64） | 8 bytes（LP64） | 需要 64 位元時一律用 `long long` 或 `int64_t`，不要用 `long` |
| 堆疊大小 | 預設約 1 MB，深度遞迴會崩潰 | 通常大很多 | 本專案在連結時加上 `-Wl,--stack,268435456`（256 MB）。了解原因就好，不要依賴它。 |
| AddressSanitizer | MinGW 不支援 | 不適用 | 改用 `-D_GLIBCXX_ASSERTIONS`：測試版預設開啟，會檢查 `vector[]` 越界等錯誤。除錯版用 `-D_GLIBCXX_DEBUG`，檢查更完整。 |
| `bits/stdc++.h` | 可以用 | 可以用 | 本專案的 `leetcode.hpp` 改用明確的標準標頭，MSVC 也能編譯 |
