# minilang：單檔樹走訪直譯器範例

[回主報告](../../README.zh-TW.md)｜[路線圖](../../research/03-design-your-own-dynamic-language.zh-TW.md)

這是研究路線圖「階段 2」的可執行範例：一門小型動態語言的完整管線，用 Python 寫成單一檔案，不依賴標準函式庫以外的套件。目的在示範元件如何銜接，不是生產品質的語言。已在 Python 3.13 執行並通過測試。

## 執行

```bash
cd dynamic-language-design-research/examples/minilang
python3 -m unittest test_minilang -v   # 13 個測試
python3 minilang.py programs/fib.ml
python3 minilang.py programs/closures.ml
python3 minilang.py programs/errors.ml  # 最後一行故意觸發未捕捉錯誤，結束碼 1
python3 minilang.py                     # REPL，陳述以 ; 結尾，Ctrl-D 離開
```

## 語言一覽

```text
# 註解以 # 開頭
let x = 1;                          # 宣告；之後用 x = 2; 指派
fn add(a, b) { return a + b; }      # 具名函式
let sq = fn(v) { return v * v; };   # 匿名函式，為閉包
if (x > 0) { print("pos"); } else if (x == 0) { print("zero"); } else { print("neg"); }
while (x < 10) { x = x + 1; if (x == 5) { break; } }
let xs = [1, 2, 3]; push(xs, 4); xs[0] = 9; print(xs[-1], len(xs));
try { throw "boom"; } catch (e) { print(e); }
```

| 項目 | 設計決定 | 對應路線圖的討論 |
| --- | --- | --- |
| 值 | 數字（整數與浮點）、字串、布林、nil、列表、函式 | 值模型：少數原生型別加一種容器 |
| 真假值 | 只有 `nil` 與 `false` 為假；`0` 與 `""` 為真 | 仿 Lua，避免隱含轉換 |
| 相等 | `==` 不跨型別；列表比較身分 | 避免 `1 == true` 這類陷阱 |
| 作用域 | 詞法作用域；`{}` 建立新環境；閉包捕捉變數而非值 | 迴圈內建立的閉包共享同一個變數，測試有示範 |
| 錯誤 | `throw` 任意值；`try/catch` 綁定該值；執行期錯誤的值是訊息字串，未捕捉時附行號 | 錯誤模型：例外 |
| 控制流程 | `return` 與 `break` 以宿主例外傳遞 | 樹走訪直譯器的常見做法 |
| 遞迴 | 語言層限制 400 層，超過回報 stack overflow | 宿主堆疊限制要轉成語言層錯誤 |
| 內建 | `print`、`len`、`push`、`pop`、`str`、`num`、`type`、`clock` | 輸出可注入，測試才能比對 |

## 檔案結構與對應階段

| 區段 | 內容 | 行數概估 |
| --- | --- | --- |
| errors | `SyntaxErr`、`RuntimeErr`，都帶行號 | 30 |
| lexer | `tokenize()`：手寫狀態機，處理數字、識別字、關鍵字、字串逸出、註解 | 90 |
| AST | `Node(kind, line, **fields)` | 15 |
| parser | 遞迴下降；運算式依優先順序分層（or、and、等號、比較、加減、乘除、一元、呼叫與索引、基本） | 220 |
| values | `Environment` 鏈、`Function`、`Builtin`、真假值與相等規則 | 110 |
| interpreter | `exec()` 處理陳述、`eval()` 處理運算式、`call()` 建立呼叫環境 | 220 |
| main | 檔案模式與 REPL | 50 |

## 若要往階段 4 推進

1. 把 `Parser` 的輸出改成位元組碼：為每個 `Node.kind` 設計指令，常數進常數池，區域變數改成槽位索引。
2. `Environment` 鏈改為 VM 堆疊加 upvalue，`Function` 改為程式碼物件加捕捉的 upvalue。
3. 保留本目錄的測試當作等價性測試：兩種實作必須產生相同輸出。
4. 用 `programs/fib.ml` 記錄加速倍數。

詳細決策見[路線圖](../../research/03-design-your-own-dynamic-language.zh-TW.md)。
