# 團隊工作範本

[回主報告](../README.zh-TW.md)

本檔為可複製的建議範本，未啟用任何 repository 設定。請替換 team、路徑及命令；勿把示例名稱當成已存在的帳號或程式。

## 任務單

```markdown
## 問題與範圍
使用者情境：
本次包含：
本次不包含：

## 責任與相依性
負責人：
整合負責人：
必要 reviewer：
相依 Issue／PR／契約版本：

## 驗收條件
- 正常行為：
- 錯誤與邊界行為：
- 權限與資料範圍：
- 相容性：

## 交付
測試方式：
部署或資料遷移：
回復方式：
文件與監控：
```

## PR 描述

```markdown
## 問題與結果
對應任務：
修改前後的行為：
影響的模組與契約：

## 驗證
測試命令、結果與對應 commit：
人工驗收或預覽連結：
尚未驗證部分及原因：

## 風險與發布
相依 PR：
資料／API 相容性：
部署次序、旗標與回復方法：

## AI 協助
AI 參與的範圍：
作者已核對的邏輯及證據：

## 合併後
下游仍使用此分支：是／否，相關 PR：
backport／forward-port：
文件、旗標與環境清理：
```

## AI 任務交接

```text
目標：完成 Issue ___ 的 ___ 驗收條件。
責任人：___。
基準 commit：___；工作分支：___；工作目錄：___。
先閱讀：___ 架構文件、___ 契約、適用的 repository 指令。
允許修改：___。
不可修改或存取：___。
先確認現有未提交工作，不覆寫別人的修改。
不得自行改變驗收條件、削弱測試或繞過 CI。
相依任務：___，版本或 PR：___。
驗證命令：___；另需檢查的負面情境：___。
停止並回報條件：需要擴大範圍、接觸正式環境、介面矛盾或無法驗證。
交付：變更摘要、檔案差異、測試命令及結果、未解決風險。
推送、開 PR、部署的授權範圍：___。
```

## CODEOWNERS 示意

複製到目標專案前，確認團隊存在並具必要寫入權限，啟用要求 code owner review 的保護設定。

```text
# 後面的匹配會覆蓋前面的匹配。
*                         @example/product-maintainers
/apps/web/                @example/frontend
/services/api/            @example/backend
/contracts/               @example/api-owners
/infra/                   @example/platform
/.github/workflows/       @example/platform
/.github/CODEOWNERS        @example/repository-admins
```

此範例提供審查路由，沒有實作「前後端兩部門必須各批准」。若新增 `/contracts/ @example/frontend @example/backend`，也只代表其中任一 owner 即可滿足這項 owner 要求，需另設跨團隊強制檢查。[官方語意](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)

## 分支清理紀錄

| 欄位 | 填寫內容 |
| --- | --- |
| repository 與分支完整名稱 |  |
| 本機、遠端或 worktree |  |
| owner 與用途 |  |
| 對應 PR、目標分支及 merge SHA |  |
| 最後 head SHA 是否相符 |  |
| 有無合併後新提交、未推送或未提交工作 |  |
| 有無下游 PR、其他使用者或執行中的 agent |  |
| 是否為仍受支援維護線 |  |
| 必須保留的證據或封存位置 |  |
| 決定、執行人與日期 |  |

建議清理自動化先產生候選清單，排除保護分支、維護線及例外，再依確認狀態執行。不要從 `--merged` 或最後更新日直接產生批次強制刪除命令。

## 發布紀錄

```markdown
產品與版本：
來源 commit／tag：
產物名稱與 digest：
服務版本組合與設定版本：
已通過的測試與驗收：
相容性與資料遷移步驟：
發布負責人與必要核准：
發布時間與環境：
監控指標、觀察期間及停止條件：
程式回復方法：
資料修復方法：
需保留的維護線與停止支援日期：
backport／forward-port 狀態：
```

## 每週流程檢視

1. 哪些 PR 等待太久，是否缺 reviewer 或任務太大？
2. 哪些分支已完成卻還有人持續使用？
3. 哪些 release 分支沒有明確支援期限？
4. 哪些測試經常不穩定，導致團隊習慣忽略失敗？
5. 哪些 AI 任務產生最多重工，是否需改善上下文或縮小權限？
6. 上週上線問題能否追溯到需求、PR、產物及部署？
