[返回主報告](README.zh-TW.md)

# 求職迴圈工程設計：一貫性 SOP、三專案優點整合、本地 Patchright 驅動免安裝 Chrome

撰寫日期：2026-10-29。本文件是在 [主報告](README.zh-TW.md) 的研究結論之上，規劃一套可以自己實作、自己維運的求職迴圈系統。目標是把 career-ops 的掃描與追蹤、ai-job-search 的文件品質、get_jobs 的登入態保留三者合併，並補上三者都沒有的「本地 Patchright 操作免安裝瀏覽器、保留登入與 2FA」能力。

## 1. 設計目標與不做的事

**要達成。**

- 一個固定節奏的迴圈：發現、評分、客製、準備、投遞、追蹤、跟進、結果、復盤。每一步有明確的輸入、輸出、閘門與記錄位置。
- 瀏覽器層只用本機 Patchright 驅動 `C:\my\build\toolchain\GoogleChromePortable64-auto\App\Chrome-bin\chrome.exe`，不需安裝 Chrome，不需管理員權限。
- 每個求職站台登入一次，之後重用。2FA 由人在 headed 視窗完成一次。
- 所有決策與動作可追溯：每個職缺有報告檔，每筆投遞有狀態列與日期。
- AI 只做建議，不做授權。送出、法律題、薪資題、簽證題一律由人確認。

**明確不做。**

- 不自動解 CAPTCHA、不自動收簡訊 OTP、不繞過網站的機器人挑戰頁。遇到就停下通知人。
- 不把登入 session 放進 git、不同步到雲端。
- 不無人值守批次送出。批次只到「填好待送出」為止。
- 不把可攜版 Chrome 自己的 `Data\profile` 當 user-data-dir。Chrome 136 起這會導致頁面不載入或瀏覽器退出。

## 2. 從三個專案各取什麼

| 來源 | 取用的設計 | 在本系統的落點 |
| --- | --- | --- |
| career-ops | 零 token 的 ATS API 掃描、1 到 5 分評分、knock-out 問題偵測、liveness 檢查、applications 追蹤表、follow-up 排程、agency 與直投雙渠道去重 | 第 4 節 S1、S2、S5、S6、S7 |
| ai-job-search | drafter-reviewer 雙代理人、PDF 編譯後視覺檢查、`pdftotext` 檢查 ATS 文字層、「關鍵字不是履歷支持的就當 gap 而不是硬塞」 | 第 4 節 S3 |
| get_jobs | Patchright 取代 Playwright、登入態存本機並定期刷新、黑名單與不活躍 HR 過濾、每日投遞上限 | 第 5 節瀏覽器層、第 4 節 S5 |
| 小型參考專案 | job-radar-2.0 的六階段狀態機與 email OTP 走 IMAP、dabloo26 的 NEEDS_HUMAN 停止原則、agentic-browser-mcp 的 `browser_wait_human` 工具、GetJob 的 104 storageState 流程 | 第 5 節與第 6 節 |

## 3. 系統分層

```
┌─────────────────────────────────────────────────────────────┐
│ L5 SOP 迴圈控制   每日 / 每週排程、閘門、狀態機、復盤           │
├─────────────────────────────────────────────────────────────┤
│ L4 AI 工作流      評分、客製履歷、求職信、表單答案草稿、面試準備 │
│                   在 Claude Code / Codex 內以 skill 或 mode 執行 │
├─────────────────────────────────────────────────────────────┤
│ L3 資料與追蹤     profile.yml、cv.md、reports/、tracker.md、    │
│                   blacklist.yml、followups.md（全部純文字進 git）│
├─────────────────────────────────────────────────────────────┤
│ L2 站台轉接器     每站台一個 adapter：search()、detail()、     │
│                   login_state()、fill()、submit_gate()         │
├─────────────────────────────────────────────────────────────┤
│ L1 瀏覽器層       Patchright + 可攜版 chrome.exe +             │
│                   每站台獨立持久化 profile + wait_human()       │
└─────────────────────────────────────────────────────────────┘
```

原則：L1 與 L2 是純程式，不含 LLM；L4 的 LLM 輸出只會寫進 L3 的文字檔，再由 L2 讀取執行。這樣 LLM 的幻覺不會直接變成瀏覽器動作，也方便離線重跑。

## 4. SOP 迴圈：九個階段

每個階段表格列出輸入、輸出、閘門、記錄位置。閘門分兩種：**自動閘門**由程式判定，**人工閘門**需要你在 CLI 回覆。

### S1 發現 Discover（每日）

| 項目 | 內容 |
| --- | --- |
| 輸入 | `config/portals.yml` 站台與關鍵字清單、`config/profile.yml` 的地點與職稱過濾 |
| 做法 | 免登入優先：ATS 公開 API（Greenhouse、Ashby、Lever、Workday）、104 與 CakeResume 的 JSON API 或 TLS 模擬、LinkedIn guest 搜尋 API。只有需要登入才看得到的職缺才動用 L1 |
| 輸出 | `data/pipeline.md` 新增 `- [ ] URL | 公司 | 職稱 | 來源 | 發現日` |
| 自動閘門 | 去重（URL 正規化加公司與職稱相似度）、黑名單公司剔除、職缺 liveness 檢查 |
| 記錄 | `data/scan-history.tsv` 每次掃描的來源、數量、耗時 |

### S2 評分 Score（每日，跟在 S1 後）

| 項目 | 內容 |
| --- | --- |
| 輸入 | pipeline 中未評分項目、`cv.md`、`config/profile.yml` 的 deal-breakers |
| 做法 | 抓職缺全文存成 `reports/NNN-公司-職稱.md`，LLM 依五個維度打 1 到 5 分：技能吻合、資歷吻合、地點與型態、薪資與條件、公司紅旗。deal-breaker 命中直接 1 分並標註原因 |
| 輸出 | report 檔頭寫入分數、理由、建議動作（apply / maybe / skip） |
| 自動閘門 | 分數低於 `config/thresholds.yml` 的 `apply_min` 者不進 S3 |
| 記錄 | `data/pipeline.md` 該列加上分數 |

### S3 客製 Tailor（每日，針對 apply 與 maybe）

| 項目 | 內容 |
| --- | --- |
| 輸入 | report、`cv.md`、模板 |
| 做法 | drafter 產生客製履歷與求職信，reviewer 對照職缺與 `cv.md` 逐條檢查：每個關鍵字都要能在 `cv.md` 找到證據，找不到就寫進 report 的 gaps 區而不是塞進履歷。編譯 PDF 後檢查頁數、孤行、字型，再用 `pdftotext` 檢查聯絡資訊與閱讀順序 |
| 輸出 | `applications/NNN/cv.pdf`、`cover.pdf`、`answers.md` |
| 自動閘門 | 履歷不超過兩頁、求職信一頁、文字層可抽取聯絡資訊 |
| 人工閘門 | 你讀過 reviewer 的 diff 摘要後回覆 ok，才能進 S4 |
| 記錄 | report 的 `## Materials` 區記錄版本與檢查結果 |

### S4 準備 Prepare（投遞前）

| 項目 | 內容 |
| --- | --- |
| 輸入 | 目標站台名稱 |
| 做法 | L1 以該站台 profile 開啟職缺頁，偵測登入狀態。未登入或遇到 2FA 挑戰頁，切到 `wait_human`：視窗拉回螢幕內、CLI 顯示「請在視窗完成登入或驗證後按 Enter」。完成後存 storageState 快照 |
| 輸出 | `profiles/<site>/`（git 忽略）與 `profiles/<site>.storageState.json`（git 忽略） |
| 自動閘門 | 登入狀態檢查通過（找得到使用者選單或 API 回 200） |
| 記錄 | `data/sessions.tsv`：站台、最後登入日、最後驗證日、2FA 方式 |

### S5 投遞 Apply（人在旁邊）

| 項目 | 內容 |
| --- | --- |
| 輸入 | report、`answers.md`、站台 adapter |
| 做法 | adapter 偵測表單類型，填文字、textarea、select、上傳檔案。不碰 checkbox、radio、CAPTCHA（沿用 career-ops 對 Lever hCaptcha 的經驗）。每個欄位填完後重讀 DOM 確認。knock-out 題與法律、薪資、簽證、身分題全部標記 `needs_confirmation` 並列出建議答案 |
| 輸出 | `applications/NNN/filled.json` 每欄位的值與狀態 |
| 自動閘門 | 必填欄位全部非空；同公司同職稱在 tracker 已有別的渠道則停下詢問 |
| 人工閘門 | 你在視窗檢視後自己按送出，再回 CLI 回覆 submitted |
| 記錄 | `data/tracker.md` 新增一列 Applied 與日期；`data/applications-log.tsv` 當日投遞計數，超過 `daily_max` 拒絕繼續 |

### S6 追蹤 Track（每日）

| 項目 | 內容 |
| --- | --- |
| 做法 | 讀 tracker，列出各狀態數量；掃描信箱或站台訊息（可選，需登入態）更新狀態為 Screening、Interview、Rejected、Offer |
| 記錄 | tracker 狀態列與 `data/status-log.tsv` 的每次變更 |

### S7 跟進 Follow-up（每週）

| 項目 | 內容 |
| --- | --- |
| 做法 | 投遞後第 7 天與第 14 天沒有回應者產生跟進信草稿，你決定是否寄出 |
| 記錄 | `data/followups.md` |

### S8 結果 Outcome（事件觸發）

| 項目 | 內容 |
| --- | --- |
| 做法 | 面試前以 report 與 `cv.md` 產生準備題庫；面試後寫 debrief；拒絕或 offer 時關閉該列 |
| 記錄 | `applications/NNN/interview.md`、tracker 最終狀態 |

### S9 復盤 Retro（每週）

| 項目 | 內容 |
| --- | --- |
| 做法 | 統計各來源的投遞到面試轉換率、各分數段的回應率、被拒原因分類。調整 `thresholds.yml`、關鍵字、黑名單。檢查 `sessions.tsv` 是否有站台需要重新登入 |
| 記錄 | `retro/YYYY-WW.md` |

### 狀態機

```
discovered → scored → (skip | tailored) → prepared → filled → submitted
                                                     ↓
                                               needs_human（登入、2FA、CAPTCHA、確認題）
submitted → screening → interview → (offer | rejected | ghosted)
```

任何階段遇到 `needs_human` 都只做兩件事：記錄原因到 report、停止並等待。不重試、不猜。

## 5. 瀏覽器層規格：Patchright 加可攜版 Chrome

### 5.1 實測結果（2026-10-29，Windows 11，Patchright 1.64.2，Chrome 153.0.8010.37 可攜版）

| 測試 | 結果 |
| --- | --- |
| `executablePath` 指向 `App\Chrome-bin\chrome.exe` 加獨立 `launchPersistentContext` | 啟動成功，`navigator.webdriver` 為 false，UA 為 Chrome/153，無 `cdc_` 或 `__playwright` 殘留鍵，cookie 跨次保留 |
| `executablePath` 指向 `GoogleChromePortable.exe` 啟動器 | 失敗，錯誤為 `Target page, context or browser has been closed`，子程序以 exitCode 0 退出。原因是啟動器只負責設定環境後派生真正的 chrome.exe 然後自行結束，Patchright 持有的管線隨之關閉 |
| 同設定但指向 Playwright 內建 Chromium 並用原版 Playwright | 持久化正常，但 `navigator.webdriver` 為 true，需另外加 `--disable-blink-features=AutomationControlled` |

### 5.2 固定啟動設定

```js
import { chromium } from 'patchright';

const EXE = 'C:/my/build/toolchain/GoogleChromePortable64-auto/App/Chrome-bin/chrome.exe';
const ctx = await chromium.launchPersistentContext(`profiles/${site}`, {
  executablePath: EXE,
  headless: false,          // Patchright 建議 headed；headless 會回到可偵測狀態
  viewport: null,           // 不覆寫視窗大小，避免固定 viewport 指紋
  // 不設 userAgent、不設 locale 覆寫，讓 Chrome 自己回報
});
```

Patchright 官方建議：真 Chrome 優於 Chromium、`launchPersistentContext` 優於 `launch`、headed、`no_viewport`，不要自訂 UA 或 header。以上設定全部符合。

### 5.3 Profile 與 session 政策

- `profiles/<site>/` 每站台一個，完全獨立於可攜版 Chrome 的 `Data\profile`。
- 每次成功登入後另存 `profiles/<site>.storageState.json` 作為快速驗證與災難恢復用；profile 目錄損毀時用它重建。
- 兩者都在 `.gitignore`，並建議用 Windows EFS 或 BitLocker 保護磁碟。
- `data/sessions.tsv` 記錄每站台最後登入與最後驗證日。S4 會在每次執行時用一個輕量請求驗證，失敗才要求重新登入。
- 可攜版 Chrome 升級時，`App\Chrome-bin` 整個換掉，profile 不受影響。升級後第一次啟動跑一次 S4 驗證。

### 5.4 2FA 與挑戰頁處理

| 類型 | 處理 |
| --- | --- |
| 密碼加簡訊 OTP | `wait_human`，人工輸入。不嘗試讀簡訊 |
| 密碼加 TOTP（Authenticator） | 預設 `wait_human`。若你願意，TOTP secret 放在 Windows Credential Manager，用 otplib 自動填，僅限於你自己的帳號 |
| Email OTP | 預設 `wait_human`。可選用 IMAP 撈信，需另設應用程式密碼 |
| Passkey / WebAuthn | `wait_human`，Chrome 會彈 Windows Hello |
| CAPTCHA、Cloudflare Turnstile、hCaptcha | `wait_human`，絕不接付費解題服務 |
| 「偵測到異常活動」鎖定頁 | 停止整個站台的所有動作，寫入 `sessions.tsv` 的 `locked` 欄位，隔 24 小時再人工處理 |

`wait_human` 的實作：把視窗移回 `--window-position=100,100`、在 CLI 印出原因與 URL、等 Enter、再驗證登入狀態、再把視窗移回原位。原型 `prototype/session-broker.mjs` 的 `login` 子命令就是這個流程。

### 5.5 速率與行為

- 同站台兩次動作間隔隨機 2 到 6 秒，頁面間隔 8 到 20 秒。
- 每站台每日投遞上限由 `thresholds.yml` 的 `daily_max` 控制，建議 LinkedIn 10、104 15、ATS 直投不限但總量 25。
- 只在你的清醒時段執行 S5，排程不在深夜跑登入態操作。
- 每站台同時只開一個 profile 實例，Chrome 不允許同一 user-data-dir 開兩個。

## 6. 站台轉接器清單（台灣優先）

| 站台 | 搜尋 | 投遞 | 登入與 2FA | 備註 |
| --- | --- | --- | --- | --- |
| 104 | JSON API `jobs/search/api/jobs` 免登入，需 Chrome TLS 指紋 | 需登入，表單為站內應徵 | 帳密，可能有 Email OTP | 參考 bless25min/GetJob 與 YuJunWang/104-job-hunter-mcp |
| CakeResume | 頁面抓取免登入 | 需登入 | 帳密或 Google 登入 | Google 登入在自動化瀏覽器會被擋，建議改設站內密碼 |
| 1111 | 頁面抓取 | 需登入 | 帳密 | 待驗證 |
| Yourator | 攔截 `/api/v4/jobs` | 需登入 | 帳密 | 參考 wmh/job-source-mcp |
| LinkedIn | guest 搜尋 API 免登入 | Easy Apply 需登入 | 帳密加 2FA，偵測最嚴 | 每日上限最低，Patchright 必要 |
| Greenhouse / Ashby / Lever / Workable | 公開 API | 免登入直投 | 無 | career-ops 經驗可直接沿用：Lever 不碰 checkbox，Ashby 注意 email 合併 |
| Workday | 公司各自子網域 | 需註冊帳號 | 帳密，常有 Email 驗證 | 多頁精靈，參考 job-radar-2.0 |

轉接器介面統一為五個函式：`search(query)`、`detail(url)`、`login_state(ctx)`、`fill(ctx, answers)`、`submit_gate(ctx)`。`submit_gate` 永遠回傳「等待人工」，沒有自動送出路徑。

## 7. 目錄結構

```
job-loop/
├── config/
│   ├── profile.yml          姓名、聯絡、地點、簽證、薪資、deal-breakers
│   ├── portals.yml          站台、關鍵字、地區
│   ├── thresholds.yml       apply_min、daily_max、follow-up 天數
│   └── blacklist.yml
├── cv.md                    唯一事實來源，所有客製都要能回溯到這裡
├── templates/               履歷與求職信模板
├── adapters/                每站台一支，純程式
├── browser/
│   ├── launch.mjs           5.2 的固定設定
│   └── wait-human.mjs
├── modes/                   給 Claude Code / Codex 的 skill：score、tailor、answers、interview、retro
├── data/
│   ├── pipeline.md  tracker.md  sessions.tsv  scan-history.tsv  status-log.tsv  followups.md
├── reports/                 每職缺一檔
├── applications/            每筆投遞一個資料夾
├── retro/
├── profiles/                git 忽略
└── scripts/                 daily.ps1、weekly.ps1
```

## 8. 排程

| 時間 | 動作 | 是否需要人 |
| --- | --- | --- |
| 每日 08:00 | S1 發現、S2 評分、S3 客製（到人工閘門為止） | 否，Task Scheduler 執行 |
| 每日 09:00 到 11:00 | 你審 S3 輸出、跑 S4 與 S5 | 是 |
| 每日 18:00 | S6 追蹤 | 否 |
| 每週一 | S7 跟進草稿、S9 復盤 | 是 |
| 每週 | 檢查 `sessions.tsv`，過期站台重新登入 | 是 |

## 9. 實作順序與驗收

| 階段 | 範圍 | 驗收條件 |
| --- | --- | --- |
| P0 已完成 | `prototype/session-broker.mjs` 以 Patchright 驅動可攜版 Chrome，登入一次重用 | 2026-10-29 實測通過，見 5.1 |
| P1 | L3 資料檔與 L5 狀態機、S1 對 Greenhouse 與 104 API、S2 評分 skill | 連續三天 08:00 自動產生 pipeline 與評分，無人工介入 |
| P2 | S3 drafter-reviewer 與 PDF 檢查 | 十份履歷全部通過兩頁與文字層檢查，reviewer 抓到至少一次無證據關鍵字 |
| P3 | 104 與 Greenhouse 轉接器的 `fill`、`wait_human`、knock-out 偵測 | 五筆真實投遞，全部由你按送出，filled.json 與實際表單一致 |
| P4 | LinkedIn 轉接器、每日上限、鎖定偵測 | 一週內無帳號警告 |
| P5 | S6 到 S9、排程、復盤報表 | 第一份週復盤產出並據此調整一個 threshold |

每個階段結束都在 `retro/` 寫一頁紀錄，包含遇到的挑戰頁、被擋的站台、調整的參數。

## 10. 風險

- **帳號被封。** LinkedIn 對自動化最敏感。對策：Patchright、headed、低速率、每日上限、異常即停 24 小時。仍有風險，請接受可能需要申訴。
- **網站條款。** 多數求職站台條款禁止自動化。本系統把送出留給人、不做爬取囤積，但你應自行判斷。
- **Chrome 政策變動。** Chrome 已封鎖預設 profile 的遠端除錯，未來可能進一步限制。對策：獨立 profile 與 Patchright 的隔離 execution context，並在 retro 記錄版本。
- **Session 外洩。** profile 等同登入憑證。對策：git 忽略、磁碟加密、不放雲端同步資料夾。
- **LLM 捏造。** 對策：L4 只能寫文字檔、reviewer 逐條回溯 `cv.md`、法律與薪資題強制人工。

## 11. 參考

- 主報告第 3 節至第 7 節對三個專案與小型專案的分析
- Patchright：<https://github.com/Kaliiiiiiiiii-Vinyzu/patchright>
- Playwright 文件 BrowserType.launchPersistentContext 對 Chrome 預設 profile 的警告
- career-ops `modes/apply.md` 與 `docs/APPLY_AUTOFILL.md` 的 ATS 行為紀錄
- ai-job-search README「How /apply works」
- get_jobs README 的持久登入與每日上限做法
