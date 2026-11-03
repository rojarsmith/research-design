[English overview](README.md) | **繁體中文**

# 新創桌面軟體的收費、授權與 IAM 架構

研究日期：2026-11-03（Asia/Taipei）。主題是新創軟體公司賣桌面軟體時，如何收「序號（永久授權）」或「訂閱」，以及使用者帳號、登入與企業 SSO（IAM）怎麼處理。延伸自[支付整合研究](../payment-integration-research/README.zh-TW.md)與 [IAM 研究](../iam-research/README.zh-TW.md)。

## 結論

1. **收費系統拆成三層，不要混在一起**：收款（金流、稅務、發票）→ 授權／權益（誰可以用哪些功能、幾台機器、到哪天）→ 身分（使用者是誰）。三層都可以買現成服務，新創真正要自己寫的只有中間的「把 Webhook 轉成權益」那段膠水程式。
2. **台灣主體的小團隊，第一選擇是 Merchant of Record（MoR）**，例如 Paddle、Polar、Lemon Squeezy。MoR 是名義上的賣方，替你處理全球 VAT／銷售稅、退款、拒付與收據；代價是約 4–5% + 每筆固定費。Stripe 本身與 Stripe Managed Payments 都**不接受台灣公司**當商家，要用 Stripe 通常得先設美國公司（見[美國公司研究](../us-company-from-taiwan/)）。
3. **授權層**：只有一個產品、規則簡單（幾台機器、訂閱到期失效）時，MoR 內建的 License Key 就夠（Polar、Lemon Squeezy 都有）；需要離線授權、浮動席位、功能旗標、試用轉正、企業大量授權時，再上 Keygen、Cryptlex、LicenseSpring 等專門服務，或自架 Keygen CE。
4. **IAM**：初期可以「只有序號、沒有帳號」；一旦有訂閱、多裝置、雲端同步或 B2B 客戶，就要有帳號。桌面登入的標準作法是**開系統瀏覽器走 OAuth 2.0 Authorization Code + PKCE，再以 loopback 或自訂 URI scheme 回到 App**（RFC 8252），不要在 App 內嵌網頁或自己收密碼。代管服務可選 WorkOS AuthKit、Clerk、Auth0、Logto；已在用的 Keycloak 也可以直接沿用。
5. **防盜版要務實**：用伺服器簽章的授權檔＋定期線上驗證＋寬限期，讓「正常付費的人不會被誤殺」比「破解者完全破不了」重要。桌面程式一定破得了，目標是提高門檻、讓正版更方便。

## 整體架構

```text
 ┌─────────────┐  1. 購買/訂閱   ┌────────────────────┐
 │ 官網 / App   │ ──────────────▶ │ 收款層 (MoR 或 Stripe) │
 │ 「購買」按鈕  │                 │ 結帳頁、稅、收據、退款   │
 └─────────────┘                 └─────────┬──────────┘
                                           │ 2. Webhook（付款成功、續訂、取消、退款、拒付）
                                           ▼
                                 ┌────────────────────┐
                                 │ 你的後端（膠水）      │──▶ 授權層：License / Entitlement
                                 │ 建立或更新權益        │      （Keygen、Cryptlex 或 MoR 內建 key）
                                 └─────────┬──────────┘
                                           │ 4. 驗證 / 下發簽章授權檔
 ┌─────────────┐  3. 登入 (OAuth+PKCE) ┌───┴──────────┐
 │ 桌面 App     │ ◀───────────────────▶ │ 身分層 (IdP)   │
 │ 授權快取+寬限期 │                       │ WorkOS/Clerk/  │
 └─────────────┘                       │ Auth0/Keycloak │
                                       └──────────────┘
```

關鍵原則：

- **權益（entitlement）以你的後端或授權服務為準**，不要讓 App 直接相信「結帳成功頁」或 MoR 的前端回呼。只有收到並驗證過簽章的 Webhook 才變更權益。
- Webhook 要**冪等**（同一事件重送不會發兩把 key）、要能補發（排程比對 MoR 訂閱狀態），因為 Webhook 會遲到、重送、亂序。
- 授權與帳號用**客戶 ID／Email 綁定**，換機、轉移、退款、停權都在後台一處處理。

## 一、收費模式怎麼選

| 模式 | 做法 | 適合 | 實作重點 |
| --- | --- | --- | --- |
| 永久授權（買斷序號） | 一次付費得到一把 key，可用指定版本 | 工具型軟體、不想綁雲端的客群 | 綁機器數量；版本或日期上限 |
| 永久授權＋一年更新（perpetual fallback） | 買斷後一年內有更新，到期可續費；不續費仍可用最後一版 | 開發工具、創作軟體（JetBrains、Sketch 類型的作法） | 授權檔記錄「可用版本的發布日上限」，App 比對自身 build 日期 |
| 訂閱（月／年） | 持續付費，停止付款就失效 | 有雲端服務、持續更新、B2B | 到期日＋寬限期；離線時用最後一次簽章授權撐 N 天 |
| 席位／浮動授權 | 公司買 N 個座位，同時上線 N 人 | B2B、實驗室、教室 | 租約（lease）＋心跳；租約過期自動釋放 |
| 用量計費／點數 | 依處理量、AI token、匯出次數計費 | 有後端成本的功能 | 用量回報、對帳；通常仍需帳號 |
| 免費試用 | 14／30 天全功能或功能限制 | 幾乎所有產品 | 試用要綁機器或帳號，否則重灌就重置 |

新創常見起手式：**年訂閱為主＋提供買斷（perpetual fallback）**，或**買斷＋付費升級大版本**。第一版不必全做，先選一種，權益模型設計成之後能加。

## 二、收款層：直接金流 vs Merchant of Record

| 方式 | 誰是法律上的賣方 | 稅務 | 費用（查核時） | 台灣主體能否用 |
| --- | --- | --- | --- | --- |
| Stripe（Billing＋Tax） | 你 | 你自己登記、申報；Stripe Tax 只計算 | 美國卡約 2.9% + 30¢，另計 Billing、Tax | 不支援台灣公司；需美國等支援國公司 |
| Stripe Managed Payments | Stripe | Stripe 處理 80+ 國 | 官方產品頁：一般 Stripe 手續費再加 3.5%；Billing 與在地支付方式另計 | **否**。官方支援商家地區只有美加、歐洲多國、澳、港、日、新加坡；也不支援 Connect／Express 帳戶 |
| Paddle | Paddle | Paddle 處理 | 5% + 50¢（多方來源一致） | 可用公司文件申請；個人資格與撥款方式需向 Paddle 確認 |
| Lemon Squeezy | Lemon Squeezy | 代處理 | 5% + 50¢，國際卡另加 | 個人可申請；2024 年被 Stripe 收購，產品路線往 Managed Payments 靠攏，長期性要評估 |
| Polar | Polar | 代處理 | Starter 5% + 50¢；Pro $20/月 3.8% + 40¢；國際卡 +1.5%；拒付每件 $15 | **可**。撥款走 Stripe Connect Express，官方列表含台灣 |
| FastSpring、Dodo Payments、Creem 等 | 平台 | 代處理 | 依合約 | 需個別查 |

判斷方式：

- **月營收低、團隊小、賣到全球** → MoR。5% 換掉全球 VAT／銷售稅登記與申報，通常划算。
- **營收大、以美國或特定市場為主、已有美國公司** → Stripe Billing＋Stripe Tax，自己處理稅務登記，費率較低但責任在你。
- **台灣本地 B2B 客戶要統一發票** → MoR 開的是國外收據，台灣企業客戶可能需要另走綠界／藍新或直接匯款開發票。這類大客戶通常走「報價單＋匯款＋手動發大量授權」，不必走線上結帳。

## 三、授權層：買服務、用 MoR 內建，還是自己做

### 方案比較

| 方案 | 類型 | 授權模型 | 離線 | 價格（查核時） | 適合 |
| --- | --- | --- | --- | --- | --- |
| Polar License Keys | MoR 內建 | 啟用次數上限、訂閱有效即有效、用量計數 | 文件未描述離線 | 含在交易費內 | 單一產品、規則簡單、已用 Polar |
| Lemon Squeezy License API | MoR 內建 | activate／validate／deactivate；訂閱到期即 `expired` | 無 | 含在交易費內；API 60 次／分 | 同上，已用 Lemon Squeezy |
| Paddle | MoR | **無內建 license key**，需接 Keygen 等或自建 | — | — | 搭配授權服務 |
| [Keygen](https://keygen.sh/) | 專門授權 API | 機器綁定、浮動、功能（entitlement）、元件指紋、發布版本與自動更新、簽章授權檔 | 支援（簽章／加密授權檔） | Dev 免費（100 活躍授權使用者、硬上限）；Std／Ent 付費；**Keygen CE 可免費自架** | 想要彈性、願意寫一點整合；可從雲端版換自架 |
| [Cryptlex](https://cryptlex.com/) | 專門授權＋SDK | 節點鎖定、浮動（雲端或地端 LexFloatServer）、試用、離線啟用 | 支援，含氣隙環境 | 分級，需看官網或詢價 | 需要現成 C/C++/.NET/Python SDK、企業地端浮動 |
| [LicenseSpring](https://licensespring.com/) | 專門授權＋SDK | 節點鎖定、浮動、訂閱、用量、離線 | 支援 | 第三方列約 $199/月起，需詢價 | 外掛（Revit、AutoCAD 等）、B2B |
| Zentitle、10Duke、Thales Sentinel、Wibu CodeMeter | 企業級 | 完整、含硬體鎖 | 支援 | 企業報價 | 高單價工業／醫療／CAD 軟體 |
| 自建 | 自己寫 | 任意 | 自己做簽章 | 開發與維運人力 | 規則很特殊或規模大後 |

### 自建或理解原理時的最小設計

即使用服務，也建議理解這套機制，才能判斷服務夠不夠用：

1. **授權檔由伺服器以私鑰簽章**（Ed25519 或 RSA），內容含：客戶 ID、產品、方案、功能清單、機器指紋、到期日、可用版本上限、簽發時間、下次需聯網時間。App 只內建公鑰驗章，不放任何秘密。
2. **機器指紋**用多個來源組合（如 Windows MachineGuid、主機板序號、主要磁碟 ID）並容忍部分變動，避免換顯卡就要重新啟用。指紋要雜湊後再傳，不送原始硬體資訊。
3. **線上驗證＋寬限期**：每次啟動或每日驗證一次，失敗時用快取的簽章授權檔撐 7–30 天，到期前提示，不要直接鎖死。
4. **時鐘回撥偵測**：記錄最後看過的伺服器時間，本機時間早於它就要求聯網。
5. **自助管理**：讓使用者在網頁上自己停用舊機器，客服量會少很多。
6. **退款／拒付**：Webhook 進來就撤銷授權，App 下次驗證時失效。

不建議自己發明加密演算法、不要用「序號本身含校驗碼」作為唯一保護（keygen 很容易被做出來），也不要把 MoR 的 API key 放進 App。

## 四、IAM：要不要帳號、怎麼登入

### 兩種路線

| 路線 | 做法 | 優點 | 缺點 |
| --- | --- | --- | --- |
| 只有序號 | 購買後 Email 收到 key，App 貼上啟用 | 最簡單，沒有登入系統 | 換機、找回 key、多裝置管理都靠客服；難做雲端功能與企業 SSO |
| 帳號＋權益 | App 登入後向後端取得該帳號的權益與簽章授權檔 | 多裝置、訂閱、團隊管理、SSO、雲端同步都自然 | 要接 IdP，要處理離線 |

常見折衷：**兩者都支援**。個人用戶登入帳號自動取得授權；企業或離線環境用 key 或授權檔啟用。

### 桌面 App 登入的標準流程（RFC 8252）

1. App 產生 PKCE `code_verifier`／`code_challenge` 與 `state`。
2. App 在本機開 `http://127.0.0.1:<隨機埠>/callback`，或註冊自訂 URI scheme（如 `myapp://callback`）。
3. 用**系統預設瀏覽器**開 IdP 的授權頁（不要用內嵌 WebView，否則密碼管理器、Passkey、企業 SSO 會出問題，且部分 IdP 會擋）。
4. 使用者在瀏覽器登入（可含 MFA、Google、企業 SAML/OIDC）。
5. 瀏覽器帶 code 回到 App，App 用 code + verifier 換 token。
6. Refresh token 存在作業系統安全儲存：Windows Credential Manager／DPAPI、macOS Keychain、Linux Secret Service。
7. App 用 access token 呼叫你的後端取得權益與授權檔。

命令列或無瀏覽器的機器可用 **Device Authorization Grant（RFC 8628）**：App 顯示一組短碼，使用者在手機或另一台電腦輸入。

付款也用同樣模式：App 不嵌結帳頁，而是請後端建立結帳連結、用系統瀏覽器開啟，完成後 App 輪詢或透過 deep link 更新權益。

### IdP 方案

| 方案 | 類型 | 免費額度（第三方報導，查核時） | 適合 |
| --- | --- | --- | --- |
| WorkOS AuthKit | 代管 | 用戶管理 100 萬 MAU 內免費；企業 SSO 每連線約 $125/月 | 預期會賣 B2B、需要 SAML SSO 與 SCIM |
| Clerk | 代管 | 約 5 萬「月留存使用者」 | 前端體驗好、Web＋桌面混合，內建組織 |
| Auth0 | 代管 | 2.5 萬 MAU；B2B 方案較貴 | 需要成熟生態、規則與擴充 |
| Logto | 代管或自架（開源） | 雲端約 5 萬 MAU；自架不限 | 想保留自架選項 |
| Keycloak、Zitadel、Ory、Authentik | 自架（開源） | 不限，但要自己維運 | 已有維運能力；本 repo 已有 [Keycloak 研究](../iam-research/README.zh-TW.md) |
| Supabase Auth、Firebase Auth | BaaS 附帶 | 量大且便宜 | 後端本來就用 Supabase／Firebase |
| Kinde | 代管，內建計費 | 有免費層 | 想把登入與方案權益放同一服務 |

選型重點：

- **B2B 是否在一年內出現**？企業客戶會要求 SAML／OIDC SSO、SCIM 自動停權、稽核紀錄。若會，選 WorkOS、Auth0、Clerk 或 Keycloak 這類有 B2B 組織模型的方案，避免日後遷移。
- **帳號系統是「誰」，授權系統是「能用什麼」**。不要把方案或功能塞進 IdP 的角色裡當唯一來源；權益應由授權服務或你的資料庫決定，IdP 只負責身分。
- 價格與免費額度差異大且常變動，上表是第三方彙整，正式採用前以官方價目表確認。

## 五、建議組合

| 階段 | 組合 | 說明 |
| --- | --- | --- |
| A. 最快上線（個人／2–3 人） | Polar（MoR＋License Key）＋簡單 Email 登入或不做帳號 | 一週內可收錢；授權規則簡單；台灣可撥款 |
| B. 有訂閱與多裝置 | Paddle 或 Polar（收款）＋ Keygen（授權，雲端或自架 CE）＋ WorkOS AuthKit 或 Clerk（登入） | 各層分工清楚，換掉任一層影響小 |
| C. 有美國公司、以美國市場為主 | Stripe Billing＋Stripe Tax＋Stripe Entitlements 或 Keygen＋任一 IdP | 費率最低，稅務登記與申報自負 |
| D. B2B／地端／離線客戶 | 上述任一收款＋Cryptlex 或 LicenseSpring（地端浮動伺服器）＋ Keycloak 或 WorkOS（SSO、SCIM） | 大客戶多走報價與匯款，後台手動發授權 |

## 六、上線前驗收情境

| 情境 | 預期結果 |
| --- | --- |
| 付款成功，Webhook 延遲 10 分鐘 | App 顯示處理中，Webhook 到達後自動啟用 |
| 同一 Webhook 重送三次 | 只發一把授權 |
| 訂閱續費失敗 | 進入寬限期並提示更新付款方式，到期後降級 |
| 退款或拒付 | 授權撤銷，App 下次驗證時失效 |
| 斷網 7 天 | 使用快取授權正常使用，到寬限期前提示 |
| 本機時間調回一年前 | 要求聯網，不延長授權 |
| 換主機板或重灌系統 | 使用者可自行在網頁停用舊機器，再啟用新機器 |
| 超過啟用機器上限 | 明確錯誤訊息與管理頁連結 |
| 企業員工離職（SCIM 停權） | 下次刷新 token 失敗，席位釋放 |
| 登入使用系統瀏覽器、Passkey、企業 SSO | 都能回到 App 並取得權益 |

## 七、案例：Typora 怎麼做

Typora（Markdown 編輯器）屬於「MoR 收款＋自建簽章授權、沒有帳號系統」的組合。以下依 2026-11-03 對本機安裝目錄（1.14.10，Electron）的唯讀檢視與官方商店頁整理；未反組譯，也未分析或繞過驗證邏輯，標為「推測」的部分只根據檔案結構與字串判斷。

### 觀察

| 觀察 | 判讀 |
| --- | --- |
| 安裝檔中搜尋不到 Keygen、Cryptlex、LicenseSpring、Gumroad、Lemon Squeezy 等字串或網域；唯一的 `paddle` 是拼字字典單字 | 沒有使用現成授權服務 SDK |
| 程式內的商店與啟用網域是 `store.typora.io`，中國另有 `typoraio.cn/store` | 啟用伺服器自建，中國市場另一套商店 |
| 使用 `node-machine-id`，讀取 Windows `MachineGuid` | 以機器 ID 作指紋，配合裝置數上限 |
| 程式含 `publicDecrypt`，原生模組 `main.node` 含 RSA 字串 | 推測：伺服器以私鑰簽發啟用資料，App 內建公鑰驗證 |
| 主程式為 `atom.compiled.dist.jsc`（ByteNode 編譯的 V8 bytecode）＋原生模組 | 以編譯與原生模組提高逆向與竄改門檻 |
| 官方商店：FastSpring 與 2Checkout（Verifone）寄送授權碼與發票 | 收款、稅務、退款交給 MoR |
| 官方條款：$14.99 買斷、無到期日、同一使用者最多同時啟用 3 台、需同一 Email、可離線啟用、試用 15 天、30 天內可退款、不提供大量授權 | 授權規則刻意簡單 |
| 授權畫面只顯示部分遮罩的 Email 與授權碼，並提供「Deactivate」 | 降低截圖外流風險；使用者可自行釋放裝置名額 |

### 推測流程

1. 使用者在 FastSpring 或 2Checkout 結帳，MoR 處理稅務、收據與退款。
2. 授權碼由 MoR 付款時呼叫 Typora 伺服器即時產生，或由 Typora 預先產生一批交給 MoR 發送；兩家平台都支援，無法從客戶端判斷是哪一種。
3. App 送出 Email、授權碼與機器指紋到 `store.typora.io`，伺服器檢查啟用台數，未超過 3 台就回傳簽章的啟用資料。
4. App 以內建公鑰驗章後存於本機，之後啟動不一定需要連網；「Deactivate」通知伺服器釋放該台名額。

### 對新創的意義

- **適合**：單一產品、買斷制、規則只限裝置數、團隊能自己維護一個小型啟用伺服器、不想付授權服務月費或被廠商綁住。
- **代價**：換機與找回授權碼依賴客服或自助頁；Email 只是綁定欄位而非帳號，難延伸到訂閱、團隊席位、雲端同步或企業 SSO。
- 若要複製這個模式，對應本文第五節的組合 A，但把 MoR 內建 License Key 換成自建簽章服務，或用自架 Keygen CE 取代自己寫。

## 八、已有美國 LLC＋Stripe 帳號的入門方案

適用：公司登記在美國、已有 Stripe 帳號、產品沒有持續的雲端成本。建議：**Payment Link 或 Checkout 開啟 Managed Payments，賣買斷授權碼，先不做帳號與訂閱**。這是第五節組合 C 的簡化版。

### 為什麼是 Managed Payments

- 美國在 Managed Payments 的支援商家所在地名單內（台灣不在），可以沿用現有 Stripe 帳號，讓 Stripe 成為 MoR。
- 不開的話，稅務要自己處理：非歐盟公司賣電子服務給歐盟消費者，從第一筆起就要收 VAT，沒有門檻（通常透過 non-Union OSS 申報）；英國對非英國賣家同樣沒有門檻；美國各州銷售稅則依經濟關聯門檻判斷，不少州對下載軟體課稅。
- 費用：一般手續費再加 3.5%。以美國卡 2.9% + 30¢ 計，合計約 6.4% + 30¢，略高於 Paddle 的 5% + 50¢，換來不用換平台。
- 限制：需通過資格審核；只能用 Checkout 或 Payment Links；訂閱只能透過 Checkout 建立；每個產品要設軟體類 tax code，例如 `txcd_10202000`（個人用下載軟體）或 `txcd_10202003`（商用）；不支援 Connect。官方產品頁沒有標示目前是全面開放還是邀請制。

### 入門設計

| 項目 | 做法 |
| --- | --- |
| 收費 | 買斷，含 3 台裝置與當前大版本更新；大版本另收升級費 |
| 結帳 | Payment Link 或 Checkout＋Managed Payments；官網與 App 的購買按鈕都開這個連結 |
| 發授權碼 | `checkout.session.completed` Webhook → 後端產生簽章授權碼並寄 Email；或交給 Keygen（免費層 100 活躍授權使用者） |
| 退款、拒付 | `charge.refunded`、`charge.dispute.created` → 撤銷授權 |
| App | Email＋授權碼啟用，檢查裝置數，本機存簽章資料，提供寬限期與「停用此裝置」 |
| 帳號 | 先不做，客戶以 Stripe 收據與 Email 找回授權碼 |

### 之後的升級路徑

1. 加年訂閱：同樣走 Checkout，用 Customer Portal 讓客戶自行換卡與取消；授權改為「訂閱有效＋寬限期」。
2. 有雲端功能或團隊客戶時：加登入（第四節），用 Stripe Entitlements 或 Keygen 管功能權益。

### 例外

- 產品有 AI、同步等持續雲端成本：一開始就用訂閱或點數，不賣買斷。
- Managed Payments 審核未通過：改用 Stripe Tax 自行登記稅籍，或改用 Paddle、Polar。
- 客群以美國企業為主：可不開 Managed Payments，用 Stripe Tax 自行處理，費率較低。

## 來源與查核

- [Stripe Managed Payments 產品頁](https://stripe.com/managed-payments)：一般手續費再加 3.5%，無設定費與月費；未標示開放階段。
- [Typora 官方商店](https://store.typora.io)：付款平台、價格、裝置數、離線啟用、試用與退款條款；本機安裝目錄的觀察見第七節。
- [Stripe Managed Payments 文件](https://docs.stripe.com/payments/managed-payments)、[資格與支援商家地區](https://docs.stripe.com/payments/managed-payments/eligibility)：支援國家列表不含台灣；不支援 Connect／Express；軟體類 tax code 可用。
- [Polar 價格](https://polar.sh/resources/pricing)、[Polar 支援國家](https://polar.sh/docs/documentation/polar-as-merchant-of-record/supported-countries)、[Polar License Keys](https://polar.sh/docs/features/benefits/license-keys)。
- [Lemon Squeezy License API](https://docs.lemonsqueezy.com/api/license-api)。
- [Keygen 價格](https://keygen.sh/pricing/)：Std／Ent 價格頁以動態載入，未能讀到具體金額。
- [Cryptlex 文件](https://cryptlex.com/docs)、[LicenseSpring 桌面授權](https://licensespring.com/solutions/desktop-licensing)。
- 標準：[RFC 8252 OAuth 2.0 for Native Apps](https://www.rfc-editor.org/rfc/rfc8252)、[RFC 7636 PKCE](https://www.rfc-editor.org/rfc/rfc7636)、[RFC 8628 Device Authorization Grant](https://www.rfc-editor.org/rfc/rfc8628)。
- 第三方彙整（有商業立場，僅作參考）：[台灣獨立開發者收款指南 2026](https://www.shareuhack.com/en/posts/taiwan-indie-dev-global-payments-guide-2026)、[Paddle vs Lemon Squeezy](https://resources.rework.com/tools/billing-revenue/paddle-vs-lemon-squeezy)、[CIAM 價格比較](https://guptadeepak.com/ciam-compass/guides/ciam-pricing-at-scale-2026/)、[WorkOS vs Auth0 價格](https://workos.com/blog/auth0-vs-workos-pricing-comparison)。

限制：Paddle、Lemon Squeezy 費率與 IdP 免費額度來自多個第三方來源，彼此大致一致但未逐一核對官方價目頁；Paddle 是否接受台灣個人賣家、Cryptlex 與 LicenseSpring 實際價格需向廠商確認。本研究未建立任何帳號或執行付款測試。
