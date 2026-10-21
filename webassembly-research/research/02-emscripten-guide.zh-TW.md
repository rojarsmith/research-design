# 02 Emscripten 與指定文章彙整

[回主報告](../README.zh-TW.md) · 查核日期：2026-10-21

## 指定文章的可用內容

使用者提供的 [archive 網址](https://claire-chang.com/archive/emscripten-webassembly-compile-guide/)直接擷取失敗；搜尋取得同站、相同 slug 的 [post 版本〈Emscripten 編譯 WebAssembly 實作筆記：從 MSYS2 到 emmake〉](https://claire-chang.com/post/emscripten-webassembly-compile-guide/)。以下摘要以搜尋工具回傳的 post 版本內文為依據，未確認兩網址是否重新導向或逐字相同。

作者以 Windows/MSYS2 移植經驗，說明 `emcc/em++`、`emconfigure`、`emcmake` 與 `emmake` 的角色；案例包括 libde265.js 與 AV1/AOM。最值得保留的是檢查實際建置紀錄、子目錄寫死的編譯器及舊 CMake cache。文中的路徑、套件組合和 CPU 設定是特定案例，不能視為所有專案的通用安裝清單。

## 與官方資料交叉檢查

| 文章重點 | 查核與補充 |
| --- | --- |
| 建置鏈要切到 Emscripten | 成立；但 host 端產生程式碼的工具可能仍須原生編譯，應區分 host 工具與 target 函式庫 |
| 要使用建置 wrapper | 應在 configure/CMake 階段配置工具鏈；wrapper 無法修復所有硬編碼設定 |
| 直接 `make` 可能回到原生編譯器 | 有可能，但不是必然。已正確設定的 Makefile 可以直接 `make`；官方目前也提供這種流程 |
| 要檢查產物是否正確 | 成立；副檔名不足以判斷目標與 ABI，必須配合 log、工具鏈設定與實際載入 |

本表依據：[Emscripten Building Projects](https://emscripten.org/docs/compiling/Building-Projects.html)。特別注意 configure 的試執行及 build 過程需要執行的產生器，是交叉編譯常見問題。

對安裝方式的補充：官方以 emsdk 為主要取得方式；Windows 可以使用 `emsdk.bat`，MSYS2 不是全部工作流程的先決條件。工程建議是先選定一套環境，避免套件來源與 PATH 相互遮蔽，並記錄實際 SDK 版本。[官方安裝文件](https://emscripten.org/docs/getting_started/downloads.html)

## 原創最小實驗：匯出一個 C 函式

以下為待執行配方，未在本機編譯。先依官方文件安裝並啟用 emsdk，確認 `emcc --version`。正式重現時記錄實際版本，避免持續使用浮動的 `latest`。

在同一個實驗目錄建立 `calc.c`：

```c
// Small inputs only: signed overflow is outside this example's contract.
int sum_squares(int x, int y) {
    return x * x + y * y;
}
```

以下單行指令適用 Windows PowerShell，假設 `emcc` 已在目前 shell 可用：

```powershell
emcc calc.c -O2 --no-entry -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORTED_FUNCTIONS=_sum_squares -o calc.mjs
```

建立 `index.html`，與產物放在同一目錄：

```html
<!doctype html>
<html lang="zh-Hant">
<meta charset="utf-8">
<title>Wasm 最小實驗</title>
<pre id="result">載入中</pre>
<script type="module">
import createCalc from './calc.mjs';
const output = document.querySelector('#result');
try {
  const calc = await createCalc();
  output.textContent = String(calc._sum_squares(3, 4));
} catch (error) {
  output.textContent = `載入失敗：${error.message}`;
  console.error(error);
}
</script>
</html>
```

啟動本地靜態服務：

```powershell
python -m http.server 8000 --bind 127.0.0.1
```

以瀏覽器開啟 `http://127.0.0.1:8000/`，預期顯示 `25`。確認 `.mjs` 與 `.wasm` 都成功下載，無 Console 錯誤。這只驗證編譯、匯出與載入，不能證明效能優於 JavaScript。

配方依據：[模組化輸出](https://emscripten.org/docs/compiling/Modularized-Output.html)、[函式匯出與直接呼叫](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html)。`await createCalc()` 等待初始化；C++ 版本應處理名稱修飾，例如為匯出介面使用 `extern "C"`。

## 進一步移植

採用獨立 build 目錄，避免污染原生建置。已啟用 emsdk 且安裝 CMake/Ninja 時，可從以下概念流程開始；個別函式庫仍可能需要額外選項：

```powershell
emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --verbose
```

先看 log 再修改設定。原生組合語言、執行緒、檔案系統、網路、同步等待與 CPU 特定最佳化，都需要逐項盤點。一般 Autotools 專案則從 `emconfigure ./configure` 開始，於支援的 shell 執行。這些是候選流程，不代表指定的兩套影音函式庫已成功重建。
