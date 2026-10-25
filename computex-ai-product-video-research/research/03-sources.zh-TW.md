# 來源與查核範圍

[回主報告](../README.zh-TW.md)。查核日期：2026 年 10 月 25 日，時區 Asia/Taipei。官方資料用於確認功能、接口與方案；不把廠商音質、效率或商業成效主張視為獨立驗證。所有規劃數字及選型順序另以本案假設呈現。

## 官方來源

| ID | 來源 | 支持內容及限制 |
| --- | --- | --- |
| R01 | [COMPUTEX 官方](https://www.computextaipei.com.tw/en/index.html) | 2027-06-01 至 06-04；不代表本案已確認屆次、攤位與設備 |
| R02 | [Claude custom connectors](https://support.claude.com/en/articles/11175166-get-started-with-custom-connectors-using-remote-mcp) | 遠端 MCP 與雲端連線；實際帳號權限需登入核對 |
| R03 | [Creatomate MCP](https://creatomate.com/docs/fundamentals/getting-started/mcp-integration) | endpoint、模板／render 工具、dry run 與預覽；本案未連線 |
| R04 | [Creatomate 自訂字幕](https://creatomate.com/docs/api/quick-start/provide-your-own-subtitles) | 可提供字幕時間資料；雙語版面是本案設計，尚待實測 |
| R05 | [Creatomate 自訂字型](https://creatomate.com/docs/api/quick-start/use-a-custom-font) | 可用自訂字型；不證明任意繁體字型都可授權／成功渲染 |
| R06 | [Creatomate credits](https://creatomate.com/docs/account/how-are-credits-calculated) | 計價公式與額外計費；不等於訂閱報價 |
| R07 | [Shotstack MCP](https://shotstack.io/docs/guide/agents/mcp-server/) | 官方 endpoint、認證與組合／render |
| R08 | [Shotstack pricing](https://shotstack.io/pricing/) | 查核當日價目、4K 方案邊界與生成另計 |
| R09 | [Runway MCP](https://help.runwayml.com/hc/en-us/articles/51931843164691-Connecting-to-Runway-MCP) | Claude 連接及生成圖片／影片；不等於完整剪輯工具 |
| R10 | [ElevenLabs MCP](https://elevenlabs.io/mcp) | 官方創作入口及 endpoint；可用工具應以登入後清單為準 |
| R11 | [ElevenLabs Hosted MCP](https://elevenlabs.io/docs/eleven-agents/operate/hosted-mcp) | Agents 管理及文字轉語音、短效下載連結；完整音樂／音效工具另驗證 |
| R12 | [ElevenLabs 開源 MCP](https://github.com/elevenlabs/elevenlabs-mcp) | 官方 repository 候選；本次搜尋確認，未安裝／程式審查 |
| R13 | [ElevenLabs billing](https://elevenlabs.io/docs/overview/administration/billing) | 付費內容一般商用說明；仍受服務別條款約束 |
| R14 | [ElevenLabs Music Terms](https://elevenlabs.io/music-terms) | 2026-10-09 條款與 Exhibit A 組織資格；不以一般語音授權取代 |
| R15 | [Firefly Generate Music](https://www.adobe.com/products/firefly/features/ai-music-generator.html) | 音樂生成與廠商商用說明；MCP 未確認，實際條款於採購時保存 |
| R16 | [Firefly 音效](https://helpx.adobe.com/firefly/web/work-with-audio-and-video/work-with-audio/text-to-sound-effects.html) | 文字產生音效的官方操作說明 |
| R17 | [Suno policy update](https://suno.com/blog/suno-updates-tos) | 2026-08-10 公告的下載政策變更 |
| R18 | [Suno Terms](https://suno.com/terms) | 2026-09-03 生效條款；商用及特定 remix 另有限制 |
| R19 | [DaVinci Resolve](https://www.blackmagicdesign.com/products/davinciresolve) | 目前官方展示 21 版；未在本機試用 |
| R20 | [Premiere 字幕翻譯](https://helpx.adobe.com/premiere/desktop/add-text-images/insert-captions/translate-captions.html) | 雲端翻譯與字幕軌；繁體品質需人工檢查 |
| R21 | [Descript 文字剪輯](https://help.descript.com/getting-started/edit-like-a-doc) | 以文字操作媒體；本次未確認官方 MCP |
| R22 | [Subtitle Edit](https://nikse.dk/SubtitleEdit/) | 官方搜尋結果可見功能，正文擷取不完整；僅列候選 |

除 R12／R22 註明的限制外，主要推薦已打開相應官方網頁確認。不引用搜尋結果中的 Reddit 經驗或第三方轉述作授權結論。

## 本機來源

新增的[功能費用評估](04-feature-cost-budget.zh-TW.md)另外核對以下來源：

| 來源 | 查核用途 |
| --- | --- |
| [Claude 價目](https://claude.com/pricing) | 月繳、年繳及 API 計價 |
| [Claude Team 說明](https://support.claude.com/en/articles/9266767-what-is-the-team-plan) | 席次單價與最低人數；與舊首頁資訊有差異，採購時再核對 |
| [Creatomate 方案說明](https://creatomate.com/docs/account/how-does-the-pricing-work) | Essential US$54、Growth US$129、公司方案建議與不跨月額度 |
| [ElevenLabs 價目](https://elevenlabs.io/pricing) | 正常價格、首月優惠、TTS／轉錄／音樂／音效計價單位 |
| [ElevenLabs 音效文件](https://elevenlabs.io/docs/overview/capabilities/sound-effects) | 指定長度的 API 計費，與按次報價分開 |
| [Adobe 台灣 Firefly](https://www.adobe.com/tw/products/firefly/plans.html) | 以瀏覽器核對個人及商業方案動態卡：含稅／未稅、月繳／年約 |
| [Adobe credits FAQ](https://helpx.adobe.com/creative-cloud/apps/generative-ai/generative-credits-faq.html) | 音樂、語音及音效耗點；影片耗點與部分行銷卡有差異 |
| [Runway 價目](https://runway.com/pricing) | 月繳與年約折算、生成額度；MCP 計費另連官方說明 |

金額換算 US$1＝NT$33 為預算假設，不是外匯報價。尚未有報價的公司客製音樂合約以 Q 表示，不以個人方案取代。

- `D:\SyncthingStorageEdt1\edt\EDT HMI Studio\evk043027b-smart-washer-5min-cht.md`：已讀取，示範流程依據。
- 同目錄 MP4、AVI、PPTX、PDF：檔案層級盤點，未逐一播放、提取簡報或驗證權利；詳見[素材表](../production/02-assets-and-claims.zh-TW.md)。
- [既有產品研究](../../claude-startups-research/research/project-assessment.zh-TW.md)：已讀，2026-10-12 的歷史查核，不能替代拍攝版本驗證。
- [HMI Studio 公開網站](https://www.hmistudio.io/)：本次網頁工具無法擷取，未宣稱已確認最新版本。

## 尚未完成的驗證

1. 影音平台帳號、方案、實際 MCP 工具與憑證權限。
2. 生成音訊的口音、字詞讀法、音質與企業適用條款。
3. 雙語字型、4K、混音及素材上傳端到端試作。
4. 原始影片畫質、實機內容與現行產品能力。
5. 50 吋螢幕、播放主機、展場音量限制及攤位配置。

這些是製作啟動及交付的驗收工作，不阻礙本次規劃完成。價格、額度、服務條款與 endpoint 在正式採購、首次連線及展前定稿時重新核對；記錄變動日期，避免默默沿用舊方案。
