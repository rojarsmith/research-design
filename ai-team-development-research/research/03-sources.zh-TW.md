# 來源與查證範圍

[回主報告](../README.zh-TW.md)

查閱日期：2026-10-18。以下為已開啟查閱的官方文件。Git 手冊用於命令語意；平台文件用於產品能力。時限、角色分工、四週導入與流程選擇是本研究提出的制度，不是官方標準或已測得成效。

| 編號 | 官方來源 | 支持的內容 |
| --- | --- | --- |
| 1 | [GitHub flow](https://docs.github.com/en/get-started/using-github/github-flow) | 分支、PR、審查、合併及完成後刪除 |
| 2 | [git branch](https://git-scm.com/docs/git-branch) | 本機分支、可達性判斷、`-d` 與 `-D` |
| 3 | [git worktree](https://git-scm.com/docs/git-worktree) | 多工作目錄、移除與管理紀錄 |
| 4 | [git fetch](https://git-scm.com/docs/git-fetch) | 遠端追蹤參照與 prune |
| 5 | [git push](https://git-scm.com/docs/git-push) | 遠端刪除與 force-with-lease |
| 6 | [git revert](https://git-scm.com/docs/git-revert) | 用新提交撤回既有變更 |
| 7 | [GitHub 合併方式](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/about-merge-methods-on-github) | merge、squash、rebase 與歷史差異 |
| 8 | [GitHub 分支刪除與還原](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-branches-in-your-repository/deleting-and-restoring-branches-in-a-pull-request) | PR 分支的 UI 操作條件 |
| 9 | [GitHub 受保護分支](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches) | 核准、檢查與分支規則 |
| 10 | [GitHub CODEOWNERS](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners) | pattern 順序、owner 權限與任一 owner 核准語意 |
| 11 | [GitHub merge queue](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/managing-a-merge-queue) | 組合驗證、`merge_group` 及可用方案 |
| 12 | [GitHub Actions 安全參考](https://docs.github.com/en/actions/reference/security/secure-use) | 不可信輸入、權限、action 固定 SHA 與執行邊界 |
| 13 | [GitHub Copilot on GitHub](https://docs.github.com/en/copilot/concepts/copilot-surfaces/copilot-on-github) | GitHub 內探索、修改與審查能力 |
| 14 | [Codex GitHub review](https://learn.chatgpt.com/docs/third-party/github) | PR review、啟用條件與 repository review 規則 |
| 15 | [GitLab merge requests](https://docs.gitlab.com/user/project/merge_requests/) | MR、討論與 pipeline 整合 |
| 16 | [Azure Repos branch policies](https://learn.microsoft.com/en-us/azure/devops/repos/git/branch-policies?view=azure-devops) | reviewer 要求與 build validation |

## 證據如何解讀

官方文件適合確定自己的產品行為，不足以證明跨廠商效能優劣。本研究沒有進行 AI 品質排名，也沒有採用供應商宣稱的生產力數字。Git 命令歷史與 GitHub 合併機制交叉解釋 squash 後的清理陷阱；跨平台則比較同類審查與整合機制，而不預設某平台最好。

研究未連入任何公司的 GitHub 組織、實際方案或正式部署系統。命令與範本是操作參考，未在目前 repository 執行分支刪除、改保護設定或發布。

## 導入前待確認

| 問題 | 影響的決策 |
| --- | --- |
| 人數、角色與目前 reviewer 瓶頸 | 任務大小、審查服務目標與並行量 |
| Web、桌面、設備或其他產品；發布頻率 | 是否需要長期 release 分支 |
| 同時支援的版本與停止支援政策 | backport 矩陣與維護線保留 |
| 現有 repo 結構與程式讀取隔離要求 | monorepo 或多 repo |
| 平台方案、Cloud／Server 與版本 | 可用保護、佇列、核准與部署功能 |
| 現有 CI、產物庫、部署及監控 | 真正可強制的驗收閘門 |
| 哪些 AI 工具及帳號方案 | 指令檔、repo 存取與可用整合 |
| 原始碼、提示與測試資料的資料政策 | 雲端／本機執行、紀錄及資料保留設定 |
| 目前缺陷、交付時間與總成本基準 | 是否擴大 AI 使用的判準 |

資料保留、模型訓練使用、資料落地區域與合約條款須按實際 AI 產品及方案另行查核，不能因工具在本機 IDE 顯示，就假設資料不會傳出裝置。
