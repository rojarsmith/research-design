# 分支生命週期與操作手冊

[回主報告](../README.zh-TW.md)

## 分支不是備份也不是發布紀錄

刪除分支主要是移除名稱指向，不會把已整合進主線的變更撤回。GitHub 上已合併 PR 的討論與歷史仍保留。但未合併、沒有其他參照或備份的 commit，不能假設永久可恢復；本機 reflog 不是團隊備份制度。[GitHub flow](https://docs.github.com/en/get-started/using-github/github-flow)、[git branch](https://git-scm.com/docs/git-branch)

發布用 tag、release 紀錄及受控保存的產物追蹤。Git tag 本身也能被修改或刪除，因此重要版本應限制變更權限，並記錄 SHA 與產物 digest。保留 branch 不能取代資料、產物或 repository 備份。

## 保留與刪除決策表

以下期限是建議的起始制度，可按產品驗證週期調整。最終判斷依工作狀態，不依分支年齡自動決定。

| 分支類型或狀態 | 何時保留 | 何時刪除 | 決策者 |
| --- | --- | --- | --- |
| `main` | 產品維護期間 | 不納入日常清理 | repository owner |
| `feature/*`、`fix/*` | 任務進行中、有未整合工作 | PR 已合併、沒有新提交或依賴 | 任務 owner；符合條件可自動刪遠端分支 |
| Draft PR | 仍有明確下一步 | 明確取消或完成整合後 | 任務 owner |
| 已關閉但未合併 PR | 仍要復用或待決策 | owner 決定放棄，或需保存內容已另行歸檔 | 任務 owner |
| 被其他 PR 當 base | 下游仍依賴其歷史 | 下游改接正確 base 並驗證後 | 整合負責人 |
| `release/2.4` | 2.4 系列仍收修正、仍需發布 | 停止支援，完成最終版本與必要歷史封存 | 發布 owner |
| `hotfix/*` | 修正尚未合併至需要的維護線 | 所有必要 backport／forward-port 已追蹤完成 | 發布 owner |
| `experiment/*` | 有實驗問題、owner 與期限 | 結果已紀錄，決定採用或放棄 | 實驗 owner |
| 臨時整合分支 | 有明確驗證目的且尚未完成 | 結果已回到正式主線／維護線 | 整合負責人 |
| 遠端已刪、本機仍在 | 本機有未推送工作 | 確認沒有需要保留的工作後 | 本機使用者 |

建議每週盤點；14 天沒有更新先詢問 owner，30 天沒有下一步交由整合負責人處理，不直接刪除。長週期驗證、外部審查、設備認證可列例外，記錄下次檢視日期。不要為了每個人、部門或 AI 建立永不結案的工作分支。

## 合併方式會影響清理判斷

| 方式 | 歷史效果 | 適用與注意事項 |
| --- | --- | --- |
| Merge commit | 保留原分支提交及合併關係 | 需保留細節或長期維護線同步；歷史較繁複 |
| Squash and merge | 將 PR 變更整合為新的一個 commit | 短期任務容易對應一個變更；原提交未必成為主線祖先 |
| Rebase and merge | 將提交重放到目標線，GitHub 會產生新 SHA | 線性歷史；不要假設舊 SHA 與主線提交相同 |

GitHub 官方列出上述方法與 squash 後繼續使用原分支的衝突風險。[合併方式](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/about-merge-methods-on-github)

建議普通任務統一使用 squash，合併後從最新 `main` 開新任務分支。長期 release 同步則事先選定 merge 或逐項 cherry-pick 策略，避免混用造成重複修正。PR 描述應保留設計原因，因為 squash 後主線不呈現所有中間提交。

`git branch --merged origin/main` 判斷的是 commit 可達性，不是程式內容是否等價。因此 squash／rebase 後，已完成 PR 的原分支仍可能顯示未合併；反過來，分支顯示已合併也不能證明目前產品仍保有該功能，因為後來可能已 revert。[git branch](https://git-scm.com/docs/git-branch)

## 三種刪除各自影響哪裡

| 操作 | 影響 | 不會做的事 |
| --- | --- | --- |
| `git branch -d feature/421-export` | 刪除本機分支 | 不刪 GitHub 分支 |
| `git push origin --delete feature/421-export` | 刪除 origin 上的分支 | 不清除所有同事的本機分支 |
| `git fetch --prune origin` | 更新遠端資訊並清掉過期遠端追蹤參照 | 不刪本機工作分支，也不刪遠端分支 |

以上採一般分支 fetch refspec；若另設定 tag 映射或 `pruneTags`，prune 的影響範圍須另外檢視。[git fetch](https://git-scm.com/docs/git-fetch)、[git push](https://git-scm.com/docs/git-push)

`-d` 的合併檢查依 configured upstream；沒有 upstream 時才依 HEAD。若 upstream 是遠端同名任務分支，`-d` 成功也不能單獨證明已整合到 `main`。`-D` 跳過合併保護，不能當日常自動清理預設。[git branch](https://git-scm.com/docs/git-branch)

## 一般任務操作範例

以下命令供演練，沒有在目前 repository 執行。假設 repo 的預設分支是 `main`、遠端是 `origin`，且使用者已保存現有工作；請換成實際名稱。每段確認成功後再往下做。

```powershell
git status --short
git fetch origin
git switch main
git pull --ff-only origin main
git switch -c feature/421-export
```

完成一個可解釋的修改後先檢查 diff，以實際檔案路徑暫存，避免無差別加入秘密或產物。

```powershell
git diff
git add src/export.ts tests/export.test.ts
git diff --cached
git commit -m "feat: add authorized order export"
git push -u origin feature/421-export
```

建立 PR，完成 CI、審查及合併。清理前確認：PR 狀態是 merged 且目標正確、最後 head SHA 與待刪分支相符、沒有合併後新增提交、沒有下游 PR 或他人尚在使用。若待刪分支與 PR head 不一致，先查明差異。

```powershell
git fetch --prune origin
git status --short
git worktree list
git log --oneline origin/main..feature/421-export
git branch --merged origin/main
```

最後兩個命令是調查資訊；squash 後出現舊提交並不直接代表遺失。應回頭核對 PR 的合併紀錄、最後 head、變更內容及後續提交。

若遠端尚未刪除且清理條件成立，可單獨執行：

```powershell
git push origin --delete feature/421-export
git fetch --prune origin
git switch main
git pull --ff-only origin main
git branch -d feature/421-export
```

若 `-d` 因 squash 歷史拒絕，不要立即批次強制刪除。完成上述核對，必要時先將完整分支存成 Git bundle 或建立受管理的封存參照，再針對確定可刪的單一分支執行 `git branch -D feature/421-export`。未提交與未追蹤檔案不會因保存 commit 而自動被保存，必須另行處理。

## 多人與 AI 使用 worktree

每位工程師應有自己的 clone。單一工程師讓多個任務並行時，可用不同 worktree 與不同分支；Git worktree 共用部分 repository 資料，並不是權限或安全沙箱，也不會隔離共用資料庫、連接埠或環境憑證。[git worktree](https://git-scm.com/docs/git-worktree)

```powershell
git fetch origin
git worktree add -b feature/422-export-api ../export-api origin/main
git worktree add -b feature/423-export-ui ../export-ui origin/main
git worktree list
```

每個工作環境分配不同測試資料庫、暫存目錄與服務 port。整合時透過 PR，不把兩個工作目錄的檔案直接相互覆蓋。

任務結束先確認 agent 已停止寫入，檢查該 worktree 的未提交、未追蹤及忽略檔案；後者可能含需要保留的測試資料。確認 PR 與分支處理完成、目錄可移除後，使用 Git 正式移除 worktree。不要預設加 `--force`。

```powershell
git -C ../export-api status --short
git -C ../export-api status --short --ignored
git worktree remove ../export-api
```

移除 worktree 後再依分支規則清理分支。`git worktree prune` 清理失效管理紀錄，不能取代正常移除工作目錄的流程。[git worktree](https://git-scm.com/docs/git-worktree)

## 衝突與相依 PR

一般共享分支避免改寫歷史；可把最新主線 merge 進任務分支，解衝突後重新測試。只有作者獨自使用且團隊已約定時才 rebase；需要推送改寫歷史時用 `--force-with-lease` 並確認預期遠端 SHA。它仍是歷史改寫，不能保護不知情同事的工作流程。

若 PR B 建在 PR A 之上，先標記依賴。A squash 合併後，B 可能仍帶著 A 的舊歷史；由作者把 B 的獨有變更重新接到最新主線，重新檢查 diff、base 與 CI。不能只因 A 已完成就自動刪除所有相關分支，也不能假設變更 base 就一定消除重複提交。

衝突處理要理解兩邊行為；禁止把「全部選 ours／theirs」作為 AI 的一般策略。若 API 或 schema 改變，相關 owner 共同確認，再跑整合測試。

## Hotfix 與多版本維護

假設正式版本為 `v2.4.3`，`main` 已在開發 2.5：從相應維護線或可追溯發布點建立 `hotfix/431-export-auth`，補重現測試、審查後合併至 `release/2.4`，產生 `v2.4.4`。再建立獨立 PR 將修正帶回 `main`，並檢視其他受支援版本是否受影響。

不要把包含 2.5 未發布功能的整條主線合回 2.4。Cherry-pick 適用時仍需每條線重新測試，因為相同 patch 在不同版本不一定有相同行為。建立修正追蹤表：原 PR、受影響版本、各 backport PR、測試及發布狀態，全部完成後才結案。

緊急事故若需 break-glass 繞過一般流程，須由指定負責人限時使用、保留紀錄、立即回復保護並補審查；不給 AI 永久繞過權限。

## 誤刪與撤回

誤刪分支時先找 PR head SHA、其他 clone 或本機 reflog；確認 commit 仍存在後，可用 `git branch recovered/421 <verified-sha>` 重新建立指向。GitHub 部分已關閉 PR 可提供 Restore branch 操作，但不要把 UI 還原能力當永久備份承諾。

參考：[GitHub 刪除與還原](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-branches-in-your-repository/deleting-and-restoring-branches-in-a-pull-request)、[git revert](https://git-scm.com/docs/git-revert)。

需要撤回已合併程式時建立 revert PR，再測試與發布；刪除舊任務分支不會撤回程式。若是 merge commit，需確認正確 parent；若是資料變更，另行處理資料。若問題是秘密已入庫，先撤銷與輪替秘密，再處理歷史及其他副本，單純刪分支不能消除外洩。
