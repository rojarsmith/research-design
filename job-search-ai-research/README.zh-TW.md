[English](README.md) | **繁體中文**

# 求職 AI 應用研究：GitHub 近三個月星數前三名的優劣，與「免安裝瀏覽器 + 登入 + 2FA」缺口

研究日期：2026-10-28。星數與專案狀態皆以當日 GitHub API 查詢結果為準。

## 1. 結論摘要

- **前三名都把「登入後的網站操作」排除在產品邊界之外。** career-ops 與 ai-job-search 的核心價值是「用你的 AI coding CLI 評分職缺、改履歷、寫求職信」，而不是幫你投遞。get_jobs 是唯一真正操作登入後網站的專案，但做法是微信掃碼而非帳密 2FA，且只支援中國大陸四大平台。
- **你的觀察是對的，但原因不是 Playwright 做不到。** career-ops 的 apply 模式刻意規定「每次用 `chromium.launch` 開一個全新瀏覽器，禁止 connectOverCDP、禁止 user-data-dir、禁止 attach 到你已登入的瀏覽器」，而且視窗被移到螢幕外座標。這是產品安全政策，不是技術限制。ai-job-search 則完全沒有瀏覽器自動化。
- **免安裝瀏覽器在技術上已經驗證可行。** 本目錄的 `prototype/` 用 Playwright 內建 Chromium（下載到使用者目錄，不需要安裝程式或管理員權限）配合獨立的持久化 profile，實測 cookie 與 localStorage 都能跨次重啟保留。2FA 只需在第一次 headed 登入時人工完成一次，之後重用 profile。
- **真正的障礙是反機器人偵測，不是 2FA。** 實測顯示內建 Chromium 預設會曝露 `navigator.webdriver = true`，headless 更會把 UA 改成 `HeadlessChrome`。LinkedIn、104（Cloudflare）這類站台會因此在登入頁或搜尋頁就擋下。解法是 headed、停用 AutomationControlled、優先使用真正的 Chrome 或 Patchright。

## 2. 如何選出前三名

GitHub 不提供「某段期間的星數增量」API，因此採兩條線交叉：

| 方法 | 結果 |
| --- | --- |
| GitHub Search API：`created:>2026-07-28` 且關鍵字 job apply / job search agent，依星數排序 | 第一名 pinloop-cli 566 星，其次 JobFinderOS 182 星、job-application-agent 169 星。新建專案規模都很小。 |
| 第三方星數追蹤（GitHub.Locker、GitStarClub、Warpchart、Breakwave）的 job-application / job-hunting 主題，看近三個月的增量與排名 | career-ops 近三個月每月增加 6K 到 7K 星，ai-job-search 從 9 月底的 44K 衝到 67K，get_jobs 以 9.3K 穩居第三。 |

本報告以第二條線為準，因為「近三個月最熱門」比「近三個月才建立」更符合實際決策需求。嚴格以建立日期篩選的名單列在第 6 節。

| 排名 | 專案 | 星數 | 建立 | 最後推送 | 語言 / 授權 |
| --- | --- | --- | --- | --- | --- |
| 1 | career-ops-hq/career-ops | 78,179 | 2026-04-04 | 2026-10-28 | JavaScript、MIT |
| 2 | ai-job-search-dev/ai-job-search（原 MadsLorentzen/ai-job-search） | 67,035 | 2026-03-18 | 2026-10-27 | Python、MIT |
| 3 | loks666/get_jobs | 9,341 | 2024-03-07 | 2026-10-28 | Java、PolyForm Noncommercial |

## 3. 三個專案的優劣

### 3.1 career-ops

**定位。** 在 Claude Code、Codex、OpenCode 等 11 種 AI coding CLI 內執行的求職代理人。零 token 掃描 100 多個 ATS 與職缺看板，對每個職缺打 1 到 5 分，產生 ATS 友善的履歷與求職信 PDF，並追蹤每筆申請。官方口號是「它幫你填，你按送出」。

**優點。**
- 社群與維護密度最高。717 位貢獻者，v1.38.0 於 10 月 28 日釋出，Windows、NixOS、Codex、Hermes 等都有獨立文件。
- 掃描層不需要登入。直接打 Greenhouse、Ashby、Lever、Workday 等 ATS 的公開 API，再用 Playwright 做職缺是否仍開放的驗證。
- apply 模式有相當成熟的表單邏輯。知道 Lever 點 checkbox 會觸發 hCaptcha 所以只填文字欄位、知道 Ashby 會依 email 合併候選人、會偵測 knock-out 問題並警告。
- 有 14 種模式，涵蓋面試準備、追蹤、follow-up、公司紅旗偵測。

**缺點，以你的需求來看。**
- `modes/apply.md` 明文規定：用 `chromium.launch` 開全新瀏覽器、「不得呼叫 `chromium.connect` 或 `connectOverCDP`」、「不得傳 remote-debugging port 或 Chrome user-data directory」、「不得 attach 到候選人已開啟的瀏覽器」。`apply-scope.mjs` 甚至用正則過濾掉任何帶 attach 語意的啟動參數。
- 因此每次 apply 都是乾淨 profile，登入狀態不會保留。碰到需要登入的站台，你要在 agent 開的視窗裡重新登入並重做 2FA。
- 預設啟動參數把視窗放在 `--window-position=-3200,-3200`，也就是螢幕外。人工完成 2FA 時要先把視窗拉回來。
- 永不送出。驗證碼、checkbox、radio 全部留給人。
- 掃描層以歐美 ATS 與各國看板為主，台灣的 104、1111、CakeResume 不在內建清單。
- 依賴 AI coding CLI 的訂閱或 API 額度；批次模式需要 `claude -p` 這類 headless worker。

### 3.2 ai-job-search

**定位。** 一個 fork 即用的 Claude Code 工作流。`/setup` 由 Claude 訪談你建立檔案，`/scrape` 抓職缺，`/rank` 批次評分，`/apply` 用 drafter-reviewer 雙代理人產生 LaTeX 履歷與求職信並編譯、檢查 PDF 文字層，`/interview` 做面試準備，`/outcome` 記錄結果。

**優點。**
- 文件品質與產出品質口碑最好。drafter-reviewer 架構把求職文件當成對抗式文件處理，AgentConn 等評論將它與「灑網式機器人」明確區隔。
- 強制 PDF 編譯與視覺檢查、用 `pdftotext` 檢查 ATS 文字層，這是其他專案沒有的。
- 完全本機執行，無 SaaS、無帳號。
- 近三個月成長最快，從 7 月初登上 GitHub trending 第一名後持續加速。

**缺點，以你的需求來看。**
- **完全沒有瀏覽器自動化。** `/apply` 的輸出是 PDF 與文字，不會開任何表單。抓職缺用的是 GET 請求配合 robots.txt 檢查，403 時才加瀏覽器 header 重試。
- 需要登入才能看的職缺或表單一律不在範圍內，所以 2FA 問題根本沒有被處理。
- 抓取 skill 針對丹麥市場（Jobindex、Jobnet），其他國家要自己寫。
- 綁定 Claude Code，無法換 CLI。
- 需要 LaTeX 或 Typst 工具鏈。

### 3.3 get_jobs

**定位。** 針對 Boss 直聘、前程無憂、獵聘、智聯招聘的桌面自動投遞工具，Java 21 加 Gradle，附網頁管理介面與 SQLite 資料庫，AI 負責寫打招呼語與判斷匹配度。

**優點。**
- 三者中唯一真正登入並操作求職網站。
- 已改用 Patchright 1.62，這是去除自動化指紋的 Playwright 分支，正面處理反偵測。
- 登入狀態存進資料庫，README 稱多數平台每週只需掃碼一次。
- 有黑名單、不活躍 HR 過濾、獵頭過濾、企業微信通知。

**缺點，以你的需求來看。**
- 登入方式是**微信掃碼**，不是帳密加 2FA。這條路徑在 LinkedIn、104、Indeed 上完全不適用。
- 只支援中國大陸四大平台，且 README 要求關閉境外代理。
- 授權是 PolyForm Noncommercial，禁止商用。
- 自動下載 chromedriver 的敘述與 Patchright 並存，環境設定比另外兩者繁瑣。
- 維護者自己建議 Boss 直聘手動投遞，本工具只做兜底。

### 3.4 橫向比較

| 面向 | career-ops | ai-job-search | get_jobs |
| --- | --- | --- | --- |
| 開瀏覽器填表 | 有，新開乾淨 Playwright 視窗 | 無 | 有，Patchright |
| 保留登入狀態 | 刻意不保留 | 不適用 | 有，資料庫存 cookie |
| 2FA 處理 | 每次人工，視窗在螢幕外 | 不適用 | 微信掃碼，約每週一次 |
| 可接已登入的瀏覽器 | 明文禁止 | 無 | 無 |
| 可指定免安裝瀏覽器 | 部分（`channel` 限 chrome / chromium / msedge，不開放 executablePath） | 不適用 | 否，內建下載 |
| 反機器人處理 | 迴避 checkbox 與驗證碼 | 遵守 robots.txt | Patchright |
| 自動送出 | 否 | 否 | 是 |
| 台灣站台 | 無 | 無 | 無 |
| 授權 | MIT | MIT | PolyForm Noncommercial |

## 4. 為什麼「免安裝瀏覽器 + 登入 + 2FA」在這些專案裡是缺口

三個專案各有不同原因，但沒有一個是 Playwright 的限制：

1. **產品邊界。** career-ops 與 ai-job-search 的作者都把「代替使用者送出」視為會污染招聘管道、也會讓使用者帳號被封的行為，因此主動不做登入態重用。career-ops 甚至把禁止 attach 寫進程式碼防呆。
2. **責任與帳號風險。** 一旦工具持有使用者的登入 session，就要處理 session 外洩、帳號被鎖、違反網站條款等問題。Playwright 官方文件也警告 storageState 檔案等同可冒用你身分的憑證。
3. **反機器人比 2FA 難。** 2FA 只要人在旁邊一次就能過；但 LinkedIn、Cloudflare、hCaptcha 會持續對自動化指紋做判定。auto-apply-bot 等專案的 README 直接承認在搜尋頁就被擋。
4. **Chrome 政策變更。** Chrome 136 起不允許用 remote debugging 自動化預設 profile，所以「直接接上使用者平常的 Chrome」這條最省事的路被堵住，必須改用獨立 profile 或瀏覽器擴充套件。

## 5. 技術驗證：免安裝瀏覽器可以保留登入態

`prototype/` 內有三支腳本，全部用 Playwright 1.64.1 與內建 Chromium 156 在 Windows 11 上實測。

| 腳本 | 用途 | 實測結果 |
| --- | --- | --- |
| `smoke.mjs` | 用 `launchPersistentContext` 寫入 cookie 與 localStorage，關閉後重開驗證 | cookie 與 localStorage 都保留 |
| `fingerprint.mjs` | 比較三種啟動方式的自動化指紋 | 見下表 |
| `session-broker.mjs` | 「登入一次，之後重用」的最小工作流 | 可指定 `--exe` 使用可攜版 Chrome |

指紋比較：

| 啟動方式 | navigator.webdriver | User-Agent |
| --- | --- | --- |
| headless 預設 | true | HeadlessChrome/156 |
| headed 預設 | true | Chrome/156 |
| headed 加 `--disable-blink-features=AutomationControlled` | false | Chrome/156 |

幾個關鍵事實：

- Playwright 的 `npx playwright install chromium` 把瀏覽器放在 `%LOCALAPPDATA%\ms-playwright`，約 437 MB，不寫登錄檔、不需管理員權限。這本身就是免安裝瀏覽器。
- 若要用可攜版 Chrome 或 Edge，傳 `executablePath` 即可。career-ops 的 `apply-scope.mjs` 只開放 `channel`，不開放 `executablePath`，這是它無法指到可攜版瀏覽器的直接原因。
- 持久化 profile 必須是**獨立目錄**，不能指向 Chrome 的 User Data。Playwright 文件明確標注這會導致頁面載入失敗或瀏覽器退出。
- 2FA 流程：第一次用 `session-broker.mjs login` 以 headed 模式開啟，人工完成帳密、簡訊或 TOTP、驗證碼，按 Enter 關閉。之後 `check` 或你的 agent 直接重用同一個 profile。TOTP 類 2FA 可用 pyotp 或 otplib 自動產生，email OTP 可用 IMAP 撈，簡訊 OTP 只能人工。
- 另一條路是 Playwright MCP 的 `--extension` 模式或 `playwright-cli attach --cdp=chrome`，直接重用你平常瀏覽器已登入的分頁。前者需要安裝擴充套件，後者需要在 `chrome://inspect` 允許遠端除錯。這兩條路不需要你把密碼交給任何工具，但對「免安裝」要求來說仍然依賴已安裝的 Chrome 或 Edge。

## 6. 嚴格以「近三個月才建立」篩選的名單

| 專案 | 星數 | 建立 | 特色 | 登入與瀏覽器 |
| --- | --- | --- | --- | --- |
| pinloop-ai/pinloop-cli | 566 | 2026-08-23 | 給 coding agent 用的職缺看板 CLI，搜尋與判斷數百萬筆職缺 | 純搜尋，不操作網站 |
| matthewprice/JobFinderOS | 182 | 2026-09-06 | Claude Code 上的招募員、爬蟲、市場分析三角色 | 爬蟲層，無登入 |
| vaibhavarora14/job-application-agent | 169 | 2026-08-12 | Agent Skill 加 CLI，廣搜後精準匹配 | 以文件產出為主 |
| leooo-sketch/ai-job-copilot-edge | 120 | 2026-08-25 | Edge 側邊欄代理人，直接在你已登入的瀏覽器內篩選與投遞 | **瀏覽器擴充套件，天然繼承登入態與 2FA** |
| Stupidoodle/swissdevjobs-cli | 103 | 2026-08-24 | 七國薪資透明職缺的搜尋與投遞 | 單一站台 |

值得注意的是 ai-job-copilot-edge：它用瀏覽器擴充套件而非 Playwright，完全繞過登入與 2FA 問題，代價是只能在 Edge 內互動、無法批次無人值守。FormFilla（3.7K 星，7 月建立）走同一條路。

## 7. 其他有處理登入或 2FA 的小型專案

這些專案星數不高，但設計上直接面對你提出的問題，可作為實作參考：

| 專案 | 做法 |
| --- | --- |
| yvandanasrisai/job-radar-2.0 | ANALYZE 到 REVIEW 六階段狀態機，先登入、沒有帳號才註冊，email 驗證碼走 Gmail IMAP 自動完成 |
| dabloo26/job-application-agent | CAPTCHA、MFA、未知必填題一律標記 NEEDS_HUMAN 停下，不猜 |
| PARK0301/job-agent | 混合投遞：Playwright ATS、Chrome CDP、螢幕 OCR、人工，逐層降級 |
| Mehmoodqureshi/linkedin-mcp | 用 `channel: 'chrome'` 驅動已安裝 Chrome，登入人工完成，session 存在 `~/.linkedin-mcp` 並另存 storageState 快照 |
| q35888/agentic-browser-mcp | `real` 模式 connectOverCDP 到 9222 埠的已登入 Chrome，`isolated` 模式用獨立持久化 profile，附 `browser_wait_human` 工具讓人處理 2FA |
| bless25min/GetJob | 台灣 104 的本機 CLI，`python main.py login` 人工登入後存 `auth/auth_104.json`，之後用 storageState |
| YuJunWang/104-job-hunter-mcp | 攔截 104 後端 JSON API 繞過前端反爬，投遞採 human-in-the-loop |
| wmh/job-source-mcp | 104、CakeResume、LinkedIn 用 curl_cffi 模擬 Chrome TLS 指紋免登入搜尋，Yourator 用 Playwright 持久化 profile |

## 8. 如果要自己補上這個缺口

建議架構是在既有專案外面加一層「session broker」，而不是改動 career-ops 或 ai-job-search 的核心：

1. **每個站台一個獨立持久化 profile**，放在專案目錄下並加進 `.gitignore`。
2. **登入指令永遠 headed**，視窗放在螢幕內，等待人工完成 2FA 後才關閉。
3. **agent 執行時重用 profile**，優先 headed 加 `--disable-blink-features=AutomationControlled`；需要更強反偵測時換 Patchright 並指到真正的 Chrome。
4. **偵測登出或 2FA 挑戰頁時暫停並通知人**，不要嘗試自動解 CAPTCHA 或簡訊 OTP。
5. **送出前保留 dry-run 閘門**，沿用 career-ops 的 knock-out 偵測與 ai-job-search 的文件品質檢查。
6. **台灣站台**優先走公開 API 或 TLS 模擬搜尋，只有投遞階段才動用登入態，降低被封風險。

`prototype/session-broker.mjs` 是第 1 到第 3 點的最小實作。

## 9. 來源

- career-ops：<https://github.com/career-ops-hq/career-ops>、`modes/apply.md`、`docs/APPLY_AUTOFILL.md`、`apply-scope.mjs`、<https://career-ops.org/llms.txt>
- ai-job-search：<https://github.com/ai-job-search-dev/ai-job-search>、AgentConn 評論 <https://agentconn.com/blog/ai-job-search-agent-drafter-reviewer-claude-code-2026/>
- get_jobs：<https://github.com/loks666/get_jobs>
- 星數追蹤：GitHub.Locker job-application 與 job-hunting 主題、GitStarClub、Warpchart、Breakwave、Enterprise DNA 2026-09-19
- Playwright 文件：Authentication、BrowserType.launchPersistentContext、MCP browser-extension、playwright-cli attach
- Patchright：<https://github.com/Kaliiiiiiiiii-Vinyzu/patchright>
- 其他專案連結見第 6、7 節表格中的 owner/repo 名稱
