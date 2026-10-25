# ChatGPT 桌面版 Codex 使用 Claude API key 可行性研究

後續研究：[Claude 與國網 API 免費額度的替代工具](api-credit-alternatives.zh-TW.md)，比較 Cherry Studio、Cline、Continue 與 Aider，並釐清每月 Claude API 福利的適用範圍。

查核日期：2026-10-25。範圍為 ChatGPT 桌面版內的本機 Codex 工作流程，特別是 Windows；不將 CLI 的能力自動視為所有桌面功能皆可使用。

**結論：不能把 Claude API key 直接當成 OpenAI 登入金鑰；若透過符合 Codex 要求的模型閘道轉接，則有條件可行。** 官方已記載 Windows 與 macOS 桌面版的閘道設定方法，但這不構成特定 Claude 模型或轉接器通過驗證的證明。本研究完成文件查核，尚未執行 Claude 付費 API 呼叫或桌面端整合測試。

## 直接使用與間接轉接

| 使用方式 | 判定 | 依據與限制 |
| --- | --- | --- |
| 在桌面登入頁貼入 Claude key | 不屬於官方記載的登入方式 | API key 登入指向 OpenAI Platform 核發的 key。[1] |
| 只把模型名稱改成 Claude | 不足以完成整合 | 還需要供應商連線、路由與相符模型資料。[2][3] |
| 用自訂閘道連接 Claude | 有條件可行，待實測 | 桌面端支援閘道設定；轉接層須提供 Codex 要求的 Responses 行為。[2][3] |
| 讓 Codex 執行另外呼叫 Claude 的程式 | 是外部工具整合的設計選項 | 不表示 Codex 的主對話模型已更換為 Claude；本研究未部署此方案。 |

官方的 API key 登入說明涵蓋桌面版、CLI 與 IDE extension，並明確連到 OpenAI dashboard。據此，不能把該登入欄位理解成接受所有公司的模型金鑰。這個判斷不代表 Codex 完全不能連外部模型服務。[1]

## 閘道需要解決的問題

建議驗證的架構為：

```text
ChatGPT 桌面版 Codex
    → Responses 相容閘道
    → Claude API
```

Claude key 放在閘道的上游憑證設定，桌面端使用閘道憑證。這是依官方閘道架構提出的 Claude 整合推論，不是已驗證的 Claude 原生支援。[3]

最關鍵的限制是協定。官方明說：能處理 Chat Completions 或 Anthropic Messages 的端點，不能據此認定符合 Responses 相容要求。驗收還應涵蓋串流完成、多輪續接，以及工具呼叫和結果回傳的完整循環。[3]

因此，評估任何宣稱「OpenAI compatible」的代理服務時，應要求它展示實際 Codex 桌面端的工具循環，不能只看聊天文字成功。模型目錄也必須反映實際上游能力；換個模型別名不會增加模型能力。[3]

## Windows 設定方向

官方文件指定使用者層級的 `%USERPROFILE%\.codex\config.toml`，變更後重啟桌面程式。閘道所需資料包括 URL、模型名稱、供應商 ID、憑證來源，以及需要時的模型目錄。桌面程式必須能讀到憑證環境變數；只在某個終端機設定，不保證從桌面啟動的程式能繼承。[2]

以下僅列設定項目的角色，不提供可直接啟用的假路由：

| 設定 | 用途 |
| --- | --- |
| `model` | 閘道實際存在的模型別名 |
| `model_provider` | 自訂供應商 ID |
| `model_providers.<id>.base_url` | 閘道 API 位址 |
| `model_providers.<id>.wire_api` | 此官方閘道路徑使用 `responses` |
| `model_providers.<id>.env_key` | 桌面程式讀取閘道憑證的環境變數名稱 |

供應商設定應放在使用者層級。官方指出，專案內 `.codex/config.toml` 的 `model_provider` 與 `model_providers` 等設定會被忽略，不能以新增研究目錄內的設定檔來切換供應商。[4]

## 後續實測與決策

若目標是保留 Codex 桌面介面並由 Claude 擔任主模型，下一步是選定轉接器、Claude 模型與桌面版本，建立小型驗證。建議先用不含敏感資料的測試專案：

1. 確認文字請求成功完成，並從閘道紀錄核對實際上游路由。
2. 測試第二輪能保留第一輪內容。
3. 要求讀取測試檔、修改指定文字並回報結果，檢查工具循環。
4. 確認無效憑證、模型不存在及上游失敗會清楚回報。
5. 通過後才逐項驗證日常所需功能。

這份測試順序是本研究的實作建議。現階段最適合的決策是：**把 Claude 主模型方案視為需驗收的整合專案，而非貼入 key 即可完成的設定。** 若需求僅是偶爾取得 Claude 的第二意見，外部呼叫工具可作為另一種設計，無須預設更換整個 Codex 主模型。

## 參考來源

以下為 2026-10-25 實際開啟的官方文件；OpenAI 文件用於確認 Codex 自身介面與要求，不用來推定 Claude 品質或效能優劣。

1. [Authentication](https://learn.chatgpt.com/docs/auth) — 桌面版登入及 OpenAI API key 定義。
2. [Use API/provider credentials through a gateway](https://learn.chatgpt.com/docs/enterprise/connect-to-a-gateway) — Windows、macOS 桌面設定與憑證傳遞。
3. [Gateway compatibility requirements](https://learn.chatgpt.com/docs/enterprise/gateway-compatibility) — Responses、串流、工具、多輪及模型路由要求。
4. [Advanced Configuration](https://learn.chatgpt.com/docs/config-file/config-advanced) — 自訂供應商與使用者／專案設定邊界。
