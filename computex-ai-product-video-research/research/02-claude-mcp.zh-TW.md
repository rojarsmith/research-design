# Claude 經 MCP 指揮影音製作

[回主報告](../README.zh-TW.md)。使用者的意思是讓 Claude 實際呼叫影音工具完成製作與修改；MCP 是操作介面，Claude 是指揮者，平台執行生成與渲染。

## 工作分工

```text
使用者目標與回饋
        ↓
Claude：證據整理、腳本、分鏡、工具調用、工作狀態與版本紀錄
        ├─ 聲音 MCP：旁白／音效 → 保存已選音檔
        ├─ 生成影片 MCP（選配）：背景／過場 → 保存已選片段
        └─ 剪輯 MCP：真實錄屏＋音檔＋字幕＋時間軸 → 雲端渲染
                                      ↓
                         MP4、模板、字幕與授權紀錄
                                      ↓
                       人工試看／試聽 → Claude 局部修訂
```

真人仍提供操作錄影和實機攝影；Claude 不憑文字捏造成功操作。遠端 MCP 也不會自動讀取 D 槽資料：素材必須透過平台上傳、可用的素材工具或具期限的授權 URL 交付。

## 已查到的官方連接點

| 服務 | 官方 endpoint | 文件能力及本案用途 |
| --- | --- | --- |
| Creatomate | `https://api.creatomate.com/mcp` | 專案模板、修改、渲染及狀態查詢；本案主候選 |
| Shotstack | `https://mcp.shotstack.io/` | 組合與渲染；備選 |
| Runway | `https://mcp.runwayml.com/mcp` | 生成圖片／影片；選配 |
| ElevenLabs | `https://api.elevenlabs.io/v1/mcp` | 官方創作頁列語音等創作入口；帳號授權後確認實際音訊工具 |

來源：[Creatomate](https://creatomate.com/docs/fundamentals/getting-started/mcp-integration)、[Shotstack](https://shotstack.io/docs/guide/agents/mcp-server/)、[Runway](https://help.runwayml.com/hc/en-us/articles/51931843164691-Connecting-to-Runway-MCP)、[ElevenLabs](https://elevenlabs.io/mcp)。本次查到文件，尚未連線驗證；不要將表格誤認為帳號已可使用全部功能。

ElevenLabs 的 [Hosted MCP 文件](https://elevenlabs.io/docs/eleven-agents/operate/hosted-mcp)聚焦 Agents 管理，但也明列文字轉語音及短效下載連結；創作產品頁另列語音／音樂等能力。因此語音已有官方文件支持，音樂與音效仍須實際列出工具及核對權限。若必要工具未提供，改評估其[官方開源 MCP](https://github.com/elevenlabs/elevenlabs-mcp)或 API 包裝，不把對話 agent 管理當成完整音訊流程已串通。

## Claude 用戶端與網路

先用 Claude 的 Connectors 設定加入剪輯服務，再依官方登入流程授權指定專案。官方目前說明遠端連接由 Anthropic 雲端發起，所以本機路徑、localhost 或只在內網可達的 URL 不能直接當素材服務。實際 Claude 方案與組織管理權限要在連接時核對。[Claude 遠端 MCP 說明](https://support.claude.com/en/articles/11175166-get-started-with-custom-connectors-using-remote-mcp)。

如果使用 Claude Code，可以沿官方提供的 HTTP MCP 設定方式。此處不建立帳號設定、不把 API key 寫進儲存庫；OAuth 適合先試連線，需 API key 的自動化再由環境變數或秘密管理提供。Claude 訂閱、影音平台訂閱及平台生成額度分別計費，不因使用 MCP 而合併。

## 具體指揮流程

1. **讀取素材與證據。** Claude 讀本目錄 brief、分鏡及 manifest；回報缺少的可公開素材、未證實主張與費用上限。檔案盤點不等於允許整個資料夾上傳。
2. **發現工具。** 查詢連線實際提供的工具、輸入 schema、方案限制及素材上傳方法；優先讀平台 guide，不憑空發明工具名。
3. **建立製作專案。** 建立獨立影片專案／模板，保存 project ID、template ID 及版本；只匯入指定公開素材。
4. **文字定稿。** 先確認雙語語意，再產生英語配音。將旁白分成 6–10 段，避免每次改一句就重生整條音軌。
5. **產生及保存聲音。** 每種候選最多 3 次試作，選定後重用現有 asset；不得每次 render 又生成不同音樂與聲音。
6. **編排時間軸。** 以 scene ID 對應素材、裁切、起迄、字幕與音訊；雙語文字使用獨立圖層，並保存可讀的旁白及字幕檔。
7. **驗證後渲染。** 平台支援 dry run 時先驗證，再以低成本預覽輸出。保存 render ID，等待完成後下載；取得 ID 不等於成片成功。
8. **檢查與修訂。** Claude 如能取得幀圖／音訊則協助檢查；無法看影片的用戶端不得自稱已逐幀驗收。人員試看後提供 scene ID＋修改指令。
9. **輸出版本。** 模板固定後產生 4K、1080p、有聲／靜音版及 30 秒版；確認所有必要檔案下載成功。
10. **交付與保存。** 保留模板／時間軸、素材清單、授權、字幕、成片、checksum，避免依賴過期雲端下載連結。

Creatomate 文件提供 `get_guide`、`list_templates`、`get_template`、`create_template`、`update_template`、`create_render`、`get_render` 等工具。其 dry run 與 half-scale 預覽也有文件，但具體參數必須從當次 schema 讀取。[官方操作範圍](https://creatomate.com/docs/fundamentals/getting-started/mcp-integration)。本案選用其中必要功能，無需刪除其他專案。

## 20 秒驗證片

| 時間 | 內容 | 要驗證的能力 |
| --- | --- | --- |
| 0–4 秒 | 產品名稱＋一句雙語用途 | 英文字重與繁體字型 |
| 4–12 秒 | 真實錄屏，一次操作及結果 | 素材取用、trim、裁切、時間軸不變造 |
| 12–16 秒 | 第二個 UI 特寫＋一句英語旁白 | 音訊同步、分軌及音量 |
| 16–20 秒 | 雙語 CTA 測試文字 | 片尾、靜音理解、循環接頭 |

先出 1080p，通過後出同一內容 4K。再要求 Claude「只將第二句繁體字幕改成指定文字，保留所有素材與起迄」，比較兩版差異。未提供正式 CTA 時畫面標示測試版本，不放不存在的展位或 QR code。

通過條件：可取得可播 MP4；無浮水印或缺字；兩語同時可讀；音畫不錯位；修改沒有無關變動；費用及專案資料能追溯。失敗時先修一個問題，連續兩次無法修正則試另一剪輯平台。

## 執行紀錄與失敗處理

| 情況 | 處理 |
| --- | --- |
| 素材 URL 失效／403 | 更新素材授權 URL 或重新上傳指定檔，不把整個磁碟開放 |
| 生成／渲染 timeout | 先查既有 job ID，再決定重送；避免重複付費 |
| 供應商無 idempotency | 自行保存任務指紋、輸入 hash 與 job ID，人工判斷未知結果 |
| 繁體字缺字 | 換成有授權且具繁體字形的字型，重出測試片 |
| 4K 被方案拒絕 | 先記錄方案差價；可交 1080p，不能宣稱已交原生 4K |
| MCP 沒有必要工具 | 改用官方 API 小型 adapter 或平台介面完成該步，不假裝全自動 |
| 背景音樂不合授權 | 替換整條 music asset，不需要重錄產品畫面 |

每筆任務保存：scene ID、素材／腳本版本、供應商、模型、job ID、狀態、生成成本、下載位置與審核結果。媒體與授權敏感資料放私有製作儲存區，Git 只放允許保存的企劃、非機密範本及去識別紀錄。

## 仍需人工介入的邊界

MCP 連線不會自動完成畫質、口音、產品正確性或展場測試。Claude 可根據回饋修改，但實際 50 吋閱讀效果與現場音量要由人員判斷。音樂若採 Firefly 網頁備案，只有生成與下載需人員操作，後續組裝仍由 Claude 指揮。只有在試作跑通後，才把對應步驟標為「已自動化」。
