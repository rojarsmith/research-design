# AI 影音工具與選型

查核日期：2026 年 10 月 25 日。[回主報告](../README.zh-TW.md)。本案優先選能由 Claude 指揮、可保存時間軸資料、可精確修改雙語文字的工具。下列適合度是本案判斷，不是已完成的音質或效能評測。

## 六項工作如何用 AI

| 工作 | Claude 或 AI 做什麼 | 人員要檢查什麼 | 成果 |
| --- | --- | --- | --- |
| 企劃 | 從已驗證功能提煉主軸、寫雙語稿、安排鏡頭、整理版本差異 | 對象、產品事實、每個主張是否有畫面證據 | 創意 brief、腳本、shot list |
| 字幕 | 語音轉文字、翻譯初稿、依場景切句、生成時間碼 | EDT／HMI／LVGL 等名稱、繁體用字、語意與可讀時間 | EN 與 zh-TW 字幕、雙語燒錄版 |
| 語音 | 逐段生成英文旁白、控制語速／停頓、替換錯誤句 | 自然度、發音、專有詞、配音與字幕內容一致 | WAV 分句音檔、voice 設定紀錄 |
| 配樂 | 依氣氛、節奏與片長生成無人聲背景音樂 | 商用資格、段落轉折、循環疲勞、旁白遮蔽 | 90 秒音樂、30 秒剪輯、授權紀錄 |
| 音效 | 生成少量點擊、轉場、完成提示聲 | 音量、不誤認為產品內建聲音、不像現場警報 | 獨立 WAV、cue sheet |
| 剪輯 | 根據鏡頭清單調用模板、修剪、排字幕、混音及輸出 | UI 沒變造、狀態順序正確、字幕不遮操作、無黑幀 | 可修改模板／JSON、MP4、渲染記錄 |

本案已經有腳本時，字幕應以核准腳本為文字主體，再對齊最終旁白；不用先產生錯字再全靠辨識修正。ASR 特別適合既有解說錄影或臨場口述素材。

## 雲端剪輯主平台比較

| 工具 | 文件確認的能力 | 本案用途 | 必做驗證 |
| --- | --- | --- | --- |
| Creatomate | 官方 MCP 提供模板讀寫與 render；自訂字幕時間資料、自訂字型有 API 範例 | 主候選：固定版型、多版本雙語影片 | 繁體字型、同時雙語兩層、音量控制、3840×2160、素材存取、局部修改 |
| Shotstack | 官方 MCP 可組合及渲染影片，提供 OAuth／API key 串接 | 備候選：較偏時間軸資料驅動的製作流程 | 轉場／字幕配置、4K 方案、音效音軌、可取回專案資料 |
| Runway | 官方 MCP 可由 Claude 生成圖片和影片 | 選配：抽象背景、少量過場 | 是否保留品牌與外觀；生成結果不得變造功能展示 |
| Descript | 文件記載以文字編輯音訊／影片 | 訪談或長解說素材整理 | 本次未確認官方 MCP；不作本案雲端指揮主平台 |

來源：[Creatomate MCP](https://creatomate.com/docs/fundamentals/getting-started/mcp-integration)、[自訂字幕](https://creatomate.com/docs/api/quick-start/provide-your-own-subtitles)、[自訂字型](https://creatomate.com/docs/api/quick-start/use-a-custom-font)、[Shotstack MCP](https://shotstack.io/docs/guide/agents/mcp-server/)、[Runway MCP](https://help.runwayml.com/hc/en-us/articles/51931843164691-Connecting-to-Runway-MCP)、[Descript 編輯說明](https://help.descript.com/getting-started/edit-like-a-doc)。

「有 MCP」只證明存在操作介面；不代表 Claude 會自動做出好影片，也不保證每個 API 功能都已包裝成 MCP 工具。登入後要以實際工具清單為準。

## 語音與音效

**ElevenLabs 為優先試聽候選**，因其官方提供可串入 Claude 的 MCP 創作入口。官方一般計費文件說明付費生成內容可有商用權，但仍受服務別條款限制；聲音來源、Beta 功能及音樂另查。先測 3 個聲音，每個只生成相同 15–20 秒稿，保存成本與音檔，以聽感決定，不用「最自然」的廠商評語排序。[MCP](https://elevenlabs.io/mcp)、[計費說明](https://elevenlabs.io/docs/overview/administration/billing)。

AI 音效也可用 Firefly 文字生成。它適合為轉場補一個聲音；本次未確認 Firefly 音效官方 MCP，因此以網頁生成後匯入為備案。[Firefly 音效操作](https://helpx.adobe.com/firefly/web/work-with-audio-and-video/work-with-audio/text-to-sound-effects.html)。

聲音測試句包含：EDT HMI Studio、HMI、LVGL、controller、register、繁體中文「暫存器／預覽／人機介面」。英文先用 E D T、H M I 分開念的版本試聽；LVGL 的念法由產品負責人確認。不要在每句切換說話者，除非有明確敘事理由。

若雲端語音口音不合適，採人工錄音再清理噪音；AI 是否參與每個步驟不應凌駕影片可理解性。

## 配樂選項及授權差異

| 方案 | 本案判斷 | 取得與限制 |
| --- | --- | --- |
| Adobe Firefly Generate Music | 適合先試無人聲科技背景音樂 | 官方宣稱可商用，需記錄實際模型／方案及採購時條款；先採網頁生成下載，MCP 未驗證 |
| ElevenLabs Music | 適合追求 Claude 直接指揮的候選，但公司資格是門檻 | 2026-10-09 音樂條款另設個人／組織及方案限制，須核對使用公司人數與用途 |
| Suno | 可作風格探索及候選，暫不列主線 | 2026 年更新下載與商用政策，不能只沿用早期「付費就全部可用」說法；亦未確認本案官方 MCP |
| 有清楚企業授權的曲庫／委託作曲 | AI 音樂不適合時的交付備援 | 依合約確認展示、網路影片、剪輯與後續重用範圍 |

來源：[Firefly 音樂](https://www.adobe.com/products/firefly/features/ai-music-generator.html)、[ElevenLabs Music Terms](https://elevenlabs.io/music-terms)、[Suno 政策更新](https://suno.com/blog/suno-updates-tos)、[Suno 條款](https://suno.com/terms)。

ElevenLabs 音樂權利表查得：Free／Starter／Creator／Pro 標示僅個人使用；Scale 為個人或少於 10 人的組織；Business 為個人或少於 50 人的組織；Enterprise 音樂方案沒有該人數限制。不能以員工個人帳號直接推定公司符合資格。付費語音的商用說明不能替代這張表；透過第三方平台轉售的音樂也要查該平台適用合約。

本案不把「AI 生成」「royalty-free」解讀為不必檢查權利。須保存生成日期、帳號法人／方案、模型版本、下載及使用條款；現場播放與網路上架都列為實際用途。若用途有爭議，向供應商取得書面適用說明或換素材，不以行銷頁保證作結論。

## 桌面備援

DaVinci Resolve 官方目前展示 21 版；適合收尾剪輯、聲音與色彩修整。Premiere 提供字幕翻譯，官方文件說明翻譯會用雲端服務。已有合法授權及熟手時，兩者都能作備援；不為這個案子同時購買兩套。[Resolve](https://www.blackmagicdesign.com/products/davinciresolve)、[Premiere 翻譯字幕](https://helpx.adobe.com/premiere/desktop/add-text-images/insert-captions/translate-captions.html)。

Subtitle Edit 是字幕修訂與轉錄的備選；官方搜尋結果有語音辨識功能，但本次頁面正文擷取不完整，安裝與引擎配置另驗證。[官方網站](https://nikse.dk/SubtitleEdit/)。

## 選型試作與淘汰標準

先讓 Creatomate 做一段 20 秒真實錄屏。若有阻礙，再用同素材試 Shotstack，避免同時學習多套平台。

| 評分面向 | 權重 | 實測方法 |
| --- | --- | --- |
| 真實素材及畫面可控 | 25% | 指定一個操作區域，裁切／放大後沒有遮擋或變造 |
| 雙語字幕 | 25% | 測「體、臺、畫、暫存器」、英文大小寫、換行及時間碼 |
| Claude 指揮與可修訂性 | 20% | 改一行字幕、替換一個鏡頭，其餘鏡頭保持原設定 |
| 4K 與播放穩定 | 15% | 取回可離線播放檔，50 吋試看，循環測試 |
| 授權及實際成本 | 15% | 保存方案、生成與重出費用、使用權紀錄 |

各項用 1–5 分，記錄理由再加權；本次未填入臆測分數。繁體缺字、無合法使用途徑、不能下載無浮水印影片、不能保存可修改資料，任一項無法解決即不採用。試作成功後鎖定平台及版本，避免正式剪輯中途換工具。
