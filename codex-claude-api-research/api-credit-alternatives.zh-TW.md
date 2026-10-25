# Claude 與國網 API 免費額度的替代工具

查核日期：2026-10-25。目標是使用已取得的 API 額度，找到可自行填入金鑰的聊天與開發工具。本研究以公開官方文件確認支援方式，未實測使用者的 API key。

**建議先用 Cherry Studio 整合日常聊天，再用 Cline 處理程式開發。** 兩者都有 Anthropic 與自訂 OpenAI 相容供應商路徑。喜歡 IDE 內輔助可選 Continue，喜歡終端機與指定檔案修改可選 Aider。這是依操作需求提出的選擇順序，不是模型品質排名。

國網部分依使用者補充，以「已有可用額度、沒有額外用途限制、需要第三方介面」為前提；公開技術文件採 Taiwan AI RAP／Medusa 多模型 API。若金鑰所屬服務不同，沿用選型原則，但應改用該服務的 API 位址與模型 ID。

## Claude 額度的真正限制

若使用的是 Max／Team 每月 API 福利，官方目前列出 Max 5x 每月 US$100、Max 20x 每月 US$200。這是金額額度，能換得多少 token 取決於模型及輸入輸出。

官方排除的是互動式 Claude Code 與訂閱額外用量，並非所有開發工具。Messages API、批次 API、Playground、Managed Agents、Agent SDK 均列為可用；自行以連結組織的 API key 執行 `claude -p` 也有列明的適用條件。由 Claude Code GitHub Action、IDE extension 或 Claude desktop 啟動的執行則另有排除，不能只看 `-p` 參數判定。

額度按帳期到期、不累積；耗盡後若組織有購買餘額或自動儲值，會繼續扣款。來源：[Claude 每月 API 額度說明](https://support.claude.com/en/articles/17154008-monthly-api-credits-for-max-and-team-plans)。

對第三方工具的實務判讀是：**選直接呼叫 Anthropic 的 API provider，使用收到額度之 Console 組織的 key。** 工具支援 API key 與某次請求確實扣到贈送額度是兩件事；首次短測後，應查看 Console 帳務確認。不要把工具自己的託管模型餘額、OpenRouter 餘額或 Claude 訂閱登入當成同一筆額度。

## 四個優先候選

| 工具 | 適合用途 | Claude 接法 | 國網接法 | 本研究判斷 |
| --- | --- | --- | --- | --- |
| Cherry Studio | 桌面聊天、摘要、翻譯、整理資料 | Anthropic provider 直連 | 自訂 OpenAI-compatible provider | 最適合作為兩組額度的日常入口 |
| Cline | 代理式開發、讀寫檔案與多步任務 | 選 Anthropic API | OpenAI Compatible，填 URL、key、model ID | 最接近原本希望 Codex 代辦開發工作的方向 |
| Continue | IDE 內聊天、程式編輯與代理工作 | `provider: anthropic` | `provider: openai` 加 `apiBase` | 適合主要留在編輯器內操作的人 |
| Aider | 終端機協作、指定檔案修改 | `ANTHROPIC_API_KEY` | 自訂 `OPENAI_API_BASE` 與模型名稱 | 適合偏好 CLI、工作範圍明確的任務 |

上表中的國網接法是根據雙方公開協定的相容性判斷，尚未完成各工具與國網模型的端到端測試。國網提供 OpenAI／LiteLLM 相容 API 與 Bearer key 驗證；不需要因為選項叫「OpenAI Compatible」就購買 OpenAI 額度。[國網 API 文件](https://rap-docs.genai.nchc.org.tw/docs/api/taiwan-ai-rap-api/)

## Cherry Studio 作為日常桌面入口

官方文件記載 Anthropic provider 可以填入 Console API key，預設 API 位址是 `https://api.anthropic.com`。另外可以新增自訂 OpenAI 相容供應商，因此可以將 Claude 與國網分成兩個設定，依任務切換。[Anthropic 設定](https://cherryai.com/docs/en/pre-basic/providers/anthropic/)、[自訂供應商](https://cherryai.com/docs/en/pre-basic/providers/zi-ding-yi-fu-wu-shang/)

建議先完成兩組普通聊天：各送一段短文，要求整理三個重點，檢查模型回覆與兩端用量。第一階段不必設定知識庫或代理功能。

要區分普通聊天與 Cherry Agent：官方目前列明 Agent 需要 Anthropic 協定，不能因國網普通聊天成功，就認定國網可直接使用該 Agent 功能。[供應商協定說明](https://cherryai.com/docs/en/pre-basic/providers/)

## Cline 作為開發代理

Cline 文件明確分成 Anthropic API key 與 Claude Code 訂閱兩條路。本需求應選 **Anthropic**，填入 API key；不要選透過 Claude CLI 的 Claude Code provider。這個設定差異比工具頁面是否出現「Claude Code」字樣更重要。[Cline Anthropic 設定](https://docs.cline.bot/provider-config/anthropic)

國網另外設定為 **OpenAI Compatible**，使用國網提供的 Base URL、API key 與 Model ID。Cline 官方支援這三個自訂欄位。[Cline OpenAI Compatible 設定](https://docs.cline.bot/provider-config/openai-compatible)

建議先讓 Claude 完成一個小型開發任務，再以同一任務測國網模型。例如：讀取一個文字檔、修改指定句子、回報差異。這可分辨「連得上 API」與「能穩定完成代理任務」。

## Continue 與 Aider 的選擇理由

Continue 可在 YAML 中配置 Anthropic 模型，也可用 `provider: openai` 配合 `apiBase` 連其他相容服務。若某個模型名稱觸發 Responses 路徑，而服務只提供 Chat Completions，官方提供 `useResponsesApi: false` 設定。模型角色與能力仍須按實際模型設定。[Continue Anthropic](https://docs.continue.dev/customize/model-providers/top-level/anthropic)、[Continue OpenAI 相容設定](https://docs.continue.dev/customize/model-providers/top-level/openai)

Aider 支援 Anthropic API key，亦支援自訂 OpenAI 相容端點。國網模型使用 Aider 時，模型參數通常採 `openai/<實際模型 ID>`，並將 `OPENAI_API_BASE` 指向國網；這裡的前綴指定介面，不代表模型由 OpenAI 提供。官方頁面中的舊 Claude 模型範例不能當作目前推薦型號，應依當前帳號可用清單選擇。[Aider Anthropic](https://aider.chat/docs/llms/anthropic.html)、[Aider 相容端點](https://aider.chat/docs/llms/openai-compat.html)

## 國網需要填哪些資料

| 欄位 | 從哪裡確認 | 常見問題 |
| --- | --- | --- |
| API Base URL | 金鑰附帶說明或平台 API 範例 | 不要把 Portal 登入網址填成推論 API 位址 |
| API key | 已取得的推論金鑰 | 應與同一個國網服務位址配對 |
| Model ID | Portal 模型列表或 `GET /models` | 使用完整 ID，不以顯示名稱猜測 |
| 模型能力 | 該模型說明及小型測試 | 文字成功不代表圖片、工具或長上下文都成功 |

國網文件提供 `GET /models`，用於取得模型 ID 與可用性。[模型清單介面](https://rap-docs.genai.nchc.org.tw/docs/api/list-models/)

Portal 可查看計畫餘額、模型及金鑰用量，並設定預算與有效期間。這些是帳務與操作欄位，不代表另有本研究推定的用途限制。[國網金鑰管理](https://rap-docs.genai.nchc.org.tw/docs/sw_intro/core_features/api_entry/API%20%E9%87%91%E9%91%B0%E7%94%B3%E8%AB%8B%E6%9C%8D%E5%8B%99/)

國網另有 `/chat/completions` 工具呼叫文件，可作為評估 Cline／Continue 的基礎；但不能直接推論每一個可選模型都有相同工具能力。[國網 Tool Calling](https://rap-docs.genai.nchc.org.tw/docs/api_features/tool_calling/chat-completions-tool-calling/)

## 建議的使用安排

1. **日常資料工作**：在 Cherry Studio 建立 Claude 與國網兩組供應商，測試摘要、翻譯、資料改寫。
2. **開發工作**：在 Cline 先啟用 Anthropic API，確認免費額度扣抵，再測國網模型的同一組小任務。
3. **選定分工**：以實際任務品質、等待時間與用量決定分配，不能僅因一組免費就假設較適合所有批次工作。
4. **確認額度**：每次較長的代理任務前查看餘額；首次使用先確認付費後備與自動儲值狀態。免費額度不代表工具會自動在免費部分耗盡時停止。

若只是要用掉已有的兩組 API 額度，上述原生 BYOK 工具比先替 Codex 建置協定轉接層更直接。原本的 [Codex 閘道研究](README.zh-TW.md)仍適用於「必須保留 Codex 桌面介面」的需求；目前優先目標則是選一個能直接連兩個供應商的客戶端。
