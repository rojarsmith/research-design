# AI 協作規則與平台設定

[回主報告](../README.zh-TW.md)

## AI 任務如何進入團隊流程

建議把 AI 視為有明確工作範圍的貢獻者。人類 owner 提供需求、允許修改的路徑、驗收方式與必要背景；AI 交付 diff、驗證結果及未知事項，owner 確認後才進入正式審查。

任務內容至少包含：Issue、基準 commit、目標分支、可改範圍、介面契約、禁止觸及的資料與系統、測試命令、完成條件、相依工作與停止條件。不要只說「把整個系統做好」，也不要用大量無關背景取代明確驗收。

| 工作 | AI 合理用途 | 人類必須確認 |
| --- | --- | --- |
| 需求分析 | 列邊界案例、找矛盾 | 業務規則與優先順序 |
| 實作 | 修改範圍清楚的任務、重構草案 | 架構、相容性、可維護性 |
| 測試 | 提出案例、建立 fixture、執行測試 | 測試是否檢查真實需求，是否被弱化 |
| 審查 | 探索缺陷、追蹤呼叫鏈 | 有效缺陷、誤報與是否可合併 |
| 文件 | 更新 API、遷移說明及 PR 摘要 | 與實際程式行為一致 |
| 發布 | 準備變更摘要與檢查結果 | 正式環境授權、風險及回復決策 |

第二個 AI reviewer 可以補充角度，但仍可能與作者共享同類盲點。人類應先從需求推導關鍵負面測試，避免「AI 寫程式，再用同一假設寫測試」只證明實作符合自己。

## 工作環境與權限建議

| 階段 | 預設權限 | 需要升級的理由 |
| --- | --- | --- |
| 探索 | 指定 repo 唯讀、無正式秘密 | 需要額外 repo 時由 owner 指定用途 |
| 開發 | 任務分支可寫、隔離測試環境 | 依賴或網路存取依任務設定 |
| 開 PR | 只授權所需 repo 的提交及 PR 操作 | 不附帶管理員、刪 repo 或繞過保護能力 |
| 部署 | 與開發身分分離的部署流程 | 正式發布由既有核准機制控制 |

Repository 文件、Issue、PR 討論與外部網頁是輸入資料，可能帶有錯誤或惡意指令。提示詞規則不能取代作業系統、token、網路與環境權限限制。AI 任務不應讀取正式客戶資料或長效管理員憑證；測試優先使用合成資料。

CI 同樣需要邊界：token 最小權限、第三方 action 固定經審查的完整 commit SHA、外部輸入不要直接拼成 shell 程式。特別避免在有秘密或寫入權限的 `pull_request_target` 工作中執行不可信 PR 程式。這些風險與是否使用 AI 無關，AI 自動化會增加需要管理的執行入口。[GitHub Actions 安全參考](https://docs.github.com/en/actions/reference/security/secure-use)

把組織規則寫成各工具實際支援的 repository 指令，例如 Codex 的 `AGENTS.md`；不要假設每個 AI 工具都自動讀同一檔名。規則中說明專案架構、正式驗證命令、修改範圍與禁止繞過的閘門。工具設定必須與文字規則一致。

## GitHub 建議設定

以下為落地檢查表；實際可用性取決於方案、公開或私人 repo，以及 Cloud／Server 版本。

| 設定 | 建議起點 | 要驗證的例外 |
| --- | --- | --- |
| 主線與維護線保護 | 必須 PR；禁止一般直接推送、force push 與刪除 | 管理員、App 及 bypass 名單是否仍能繞過 |
| 審查 | 一般變更至少一位獨立 reviewer；高風險依 owner 要求 | 作者不可自行滿足核准；最後修改是否需要重審 |
| Required checks | 建置、必要測試、契約與風險相關檢查 | check 名稱唯一、正確來源；跳過必要工作不能回報假成功 |
| 程式碼 owner | 啟用 code owner review，保護 owner 規則本身 | reviewer 有正確權限，重要路徑真的匹配 |
| 合併方式 | 普通任務統一 squash；維護線另外定義 | 若要求線性歷史，不再允許 merge commit |
| 分支清理 | 符合條件的短期 head branch 合併後刪除 | 依賴 PR、長期維護分支與保留例外 |
| 正式部署 | 受控環境、獨立部署權限、版本記錄 | 所選方案支援的環境核准與防自我核准設定 |

GitHub branch protection 可要求核准、狀態檢查及其他合併條件；套用後必須用一般開發者和機器人身分測試，不能只看管理員畫面認定已生效。[受保護分支](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)

### CODEOWNERS 不等於兩部門必須同意

同一路徑列兩個 owner，任一 owner 的核准即可滿足該項 owner 要求；最後匹配的 pattern 生效。若需求是「後端與資安各一人都要核准」，不能僅在同一行放兩個團隊，也不能只要求兩個 approval 後假定它們來自不同部門。[CODEOWNERS 官方規則](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)

需要不同部門都通過時，採平台可用的專門審查政策，或建立受信任的 required check／GitHub App，根據變更路徑及團隊身分驗證兩方核准。該機制須處理新 commit、撤銷核准、團隊異動、作者身分與重跑事件，且 PR 作者不能改自己的驗證邏輯來取得成功。暫時人工檢查必須標示為人工制度，不能宣稱已被平台強制。

### Merge queue 的條件

多人同時合併時，單一 PR 對舊主線通過 CI，不保證與前一個剛合併 PR 相容。Merge queue 會以更新主線及排隊變更組合進行檢查。使用 GitHub Actions 時，required checks 必須處理 `merge_group` 事件。[Merge queue](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/managing-a-merge-queue)

文件所列可用範圍為組織擁有的公開 repo，或 GitHub Enterprise Cloud 組織的私人 repo。導入前確認目前方案與限制。沒有佇列時，可要求最新主線驗證並由整合負責人序列合併；這能降低風險，但不應宣稱等同佇列機制。

不要只為了展示而加入空的 `merge_group` workflow。需要讓與 PR 相同的實質驗證在整合候選上執行，也要避免 CI concurrency 設定把排隊的必要工作互相取消。

## 平台與 AI 工具如何選

以下比較協作能力與導入條件，不是價格表或效能排名。既有帳號、身分系統與團隊習慣通常比單一 AI 展示功能更影響遷移成本。

| 選項 | 已查核能力 | 選擇判準與限制 |
| --- | --- | --- |
| GitHub | PR、分支保護、CODEOWNERS、符合方案條件的 merge queue | 已用 GitHub 時可漸進導入；跨部門強制簽核需另外設計 |
| GitLab | MR 集中變更、討論及 pipeline 資訊；有 Cloud 與 Self-Managed 等型態 | 需要整體 DevSecOps 或自管時納入評估；各 approval／安全功能須另查 tier |
| Azure DevOps／Azure Repos | 分支政策可要求 reviewer 與 build validation | 適合評估既有 Azure DevOps 工單及企業流程整合；仍需確認政策例外 |
| GitHub Copilot | GitHub 上可探索、規劃、執行變更與 PR review | 適合評估 GitHub 內的任務交接；不能據此推論程式品質或一定比較快 |
| Codex | GitHub 整合可請求 PR review，並使用 repository review 規則 | 適合既有 Codex 使用者評估；需連接 repo 並啟用 code review，規則不取代 CI 與核准 |
| 其他 IDE／CLI AI 助手 | 本研究不對未查核產品宣稱功能 | 用同一組任務評估上下文、隔離、權限、資料政策與成本 |

來源：[GitLab MR](https://docs.gitlab.com/user/project/merge_requests/)、[Azure Repos policies](https://learn.microsoft.com/en-us/azure/devops/repos/git/branch-policies?view=azure-devops)、[Copilot on GitHub](https://docs.github.com/en/copilot/concepts/copilot-surfaces/copilot-on-github)、[Codex GitHub review](https://learn.chatgpt.com/docs/third-party/github)。

各廠商文件只證明其產品機制，不證明相較其他產品更好。本研究不把廠商行銷中的速度或品質主張當成獨立結論，也未對以上平台進行實機比較。

## 導入驗收與成本

先挑三類試點：低風險缺陷修正、既有介面下的新功能、跨模組相容性修改。記錄需求相近案件的人工耗時、審查時間、CI 耗時、AI 使用成本、上線後缺陷及重工。敏感資料、帳號方案與產品版次須固定或記錄，避免把環境差異誤認為模型差異。

最低驗收應包含：AI 無法直接改主線；失敗測試會阻擋合併；新修改不會沿用不適當舊核准；受保護檔案觸發正確審查；未授權資料不能讀取；發布能追溯到被驗證產物；分支清理不會遺失未推送工作。先在測試 repo 演練，再套用正式 repo。

總成本以「席位或用量費＋CI／執行環境費＋設定維護時間＋審查與重工時間」計算。若產碼速度提高但 review backlog、缺陷或總成本惡化，就應縮小 AI 任務範圍或降低並行量，而不是只增加 agent 數量。
