[English overview](README.md) | **繁體中文**

# MSIX 檔案與 Windows 應用程式封裝研究

研究日期：2026 年 10 月 21 日，Asia/Taipei。

MSIX 是 Windows 應用程式的封裝與部署格式。它把應用程式檔案、套件身分及部署宣告交給 Windows 管理，適合需要一致安裝、版本更新及套件身分的桌面軟體。它不會把原生程式改寫成 UWP，也不會自動修好既有程式對路徑、權限或驅動程式的依賴。[Microsoft MSIX 概述](https://learn.microsoft.com/en-us/windows/msix/overview)、[桌面程式執行模型](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)。

本研究從使用者提供的 Python／Kivy 封裝文章出發，說明格式、工具、簽章、部署、更新及選型。對一般桌面應用程式，值得先用乾淨測試環境驗證 MSIX；若產品深度依賴驅動、全機修改或既有安裝器行為，應先比較保留 MSI／EXE 的成本。這是依技術限制提出的工程建議，不代表任何特定廠商的產品立場。

## 核心結論

- **Store 並非只收 MSIX。** Microsoft Store 也接受符合條件的 MSI／EXE，包含簽章、HTTPS 版本化下載位置及靜默安裝等要求。[Store 安裝檔要求](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msi/app-package-requirements)。
- **封裝成功與可交付是兩個階段。** 接著仍須驗證啟動、資源與資料路徑、信任鏈、更新及解除安裝；後文提供驗收表。
- **正式側載需要處理簽章與裝置信任。** 自簽可用於受控測試，但不能假設陌生使用者的電腦會信任它。[簽章概述](https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview)。
- **自動更新需要通道。** Store 或已配置的 App Installer 更新關聯可以承擔更新；單獨交付一個檔案並未配置這些行為。[App Installer 概述](https://learn.microsoft.com/en-us/windows/msix/app-installer/app-installer-file-overview)。

## MSIX 裡面有什麼

可把 MSIX 理解為附帶 Windows 部署規則的應用程式套件。執行檔仍然是原本的 EXE；套件另外宣告 Windows 要如何識別及啟動它。

| 元件 | 用途 | 檢查重點 |
| --- | --- | --- |
| 應用程式檔案 | EXE、DLL、字型及其他資源 | 必須納入程式實際需要的內容 |
| `AppxManifest.xml` | 套件身分、應用程式、視覺資產及部署宣告 | EXE 路徑與檔案、身分、架構一致 |
| `AppxBlockMap.xml` | 檔案區塊雜湊資訊 | 簽章使用的摘要演算法須與封裝一致 |
| `AppxSignature.p7x` | 已簽套件的簽章資料 | 驗證來源與內容完整性 |

MakeAppx 可以由檔案目錄建立套件或將套件解開；套件根目錄須提供有效的 manifest。[MakeAppx](https://learn.microsoft.com/en-us/windows/win32/appxpkg/make-appx-package--makeappx-exe-)；區塊雜湊與簽章的關聯見 [SignTool 套件簽章](https://learn.microsoft.com/en-us/windows/msix/package/sign-app-package-using-signtool) 與 [套件完整性](https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview)。

### 容易混淆的檔案

| 副檔名 | 角色 | 使用方式 |
| --- | --- | --- |
| `.msix` | 個別應用程式或資源套件 | 安裝、部署的實體內容 |
| `.msixbundle` | 組合多個套件，例如不同架構版本 | 讓 Windows 選擇適用內容 |
| `.appinstaller` | XML 部署描述檔 | 指向套件位置並設定更新 |
| `.msixupload` | Store 提交格式之一 | 交給 Partner Center，勿當作一般安裝檔散發 |
| `.appx`／`.appxbundle` | 同一部署生態中的相關格式 | 舊專案與工具仍可能使用這些名稱 |

Bundle 的建立與拆解參考 [MakeAppx](https://learn.microsoft.com/en-us/windows/win32/appxpkg/make-appx-package--makeappx-exe-)；Store 接受格式參考 [Store 安裝檔要求](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msi/app-package-requirements)；App Installer 的角色參考 [官方概述](https://learn.microsoft.com/en-us/windows/msix/app-installer/app-installer-file-overview)。

## 套件身分與執行權限

Manifest 中的 `Identity` 包含 Name、Publisher、Version 與 ProcessorArchitecture 等資訊；顯示名稱另有欄位。Visual Studio 專案常見的 `Package.appxmanifest` 是開發輸入，建置後產生的 `AppxManifest.xml` 才是套件使用的清單。[Manifest 產生方式](https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/generate-package-manifest)。

例如下列片段只示範身分，並非可直接封裝的完整清單：

```xml
<Identity Name="Example.ResearchApp"
          Publisher="CN=Example Publisher"
          Version="1.0.0.0"
          ProcessorArchitecture="x64" />
```

這些欄位應視為發行契約：開發測試可使用測試身分；Store 發行要對應 Partner Center 指派的資料；側載簽章的 Publisher 要符合簽章憑證 Subject。名稱或 Publisher 改動可能改變套件家族，不能只靠改檔名維持更新關係。[Manifest 身分](https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/generate-package-manifest)、[Publisher 與憑證](https://learn.microsoft.com/en-us/windows/win32/appxpkg/make-appx-package--makeappx-exe-)、[更新套件家族要求](https://learn.microsoft.com/en-us/powershell/module/appx/add-appxpackage?view=windowsserver2025-ps)。

**MSIX 不等於 AppContainer 安全沙箱。** 桌面應用程式可採 full trust 執行模型；`RuntimeBehavior` 與 `TrustLevel` 等宣告影響行為。一般套件部署會保護安裝檔案，特定檔案與登錄存取則依模型及 Windows 版本虛擬化。不可推論封裝後的程式就無法存取使用者檔案，或所有寫入都會被隔離。[桌面程式執行模型](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)。

## 指定文章彙整與校正

艾肯比於 2025 年 7 月 28 日發表的文章，以 Windows 11 上已經由 PyInstaller 產生的 Kivy 程式為例，示範透過 MSIX Packaging Tool 手動放入檔案、設定啟動點並輸出套件。它適合當圖解入門，不足以單獨作為正式發行規格。[原文](https://vocus.cc/article/6860e967fd89780001a6382d)。

| 原文涉及的操作或說法 | 適用範圍與補充 |
| --- | --- |
| Store 上架必須用 MSIX | 應改為 MSIX 是可選且受推薦的途徑，另有 MSI／EXE 提交途徑 |
| 在目前電腦上建立套件 | 適合乾淨環境；日常工作機的背景活動可能污染擷取結果 |
| Windows Search 勾選狀態 | 官方描述為可選擇停用以減少擷取干擾，不是要求啟用搜尋才能打包 |
| 不選安裝器並手動放入檔案 | 屬於官方支援的 manual installation 路徑 |
| 使用空資料夾 | 是這個手動複製流程的做法，不是所有 MSIX 專案一律如此 |
| 輸出 MSIX 後完成 | 正式交付還須完成簽章、安裝、功能、更新與 Store 驗證 |

Store 校正依 [官方格式與安裝要求](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msi/app-package-requirements)；環境、Search 與手動安裝依 [Packaging Tool 官方流程](https://learn.microsoft.com/en-us/windows/msix/packaging-tool/create-app-package)。

## 封裝工具如何選

以下是依現有資產與維護方式提出的選擇建議。

| 現有資產 | 優先嘗試 | 適用原因 |
| --- | --- | --- |
| 現成 MSI／EXE 安裝器 | MSIX Packaging Tool | 擷取原安裝流程產生的變更 |
| 可直接執行的完整程式資料夾 | Packaging Tool 手動安裝，或 MakeAppx | 可先人工驗證，再建立可重現封裝 |
| Visual Studio 可建置專案 | 專案支援的封裝流程 | 身分、資產與建置較容易一起維護 |
| 已整理的套件目錄及 manifest | MakeAppx 加簽章步驟 | 適合納入腳本與 CI |
| 無原始碼且有路徑相容問題 | 評估 Package Support Framework | 可對特定執行期問題套用修正 |

工具能力依 [Packaging Tool](https://learn.microsoft.com/en-us/windows/msix/packaging-tool/create-app-package)、[MakeAppx](https://learn.microsoft.com/en-us/windows/win32/appxpkg/make-appx-package--makeappx-exe-) 與 [PSF](https://learn.microsoft.com/en-us/windows/msix/psf/package-support-framework-overview)。PSF 是相容性修正工具，不能據此保證任何傳統程式皆可轉換。

## Packaging Tool 實作流程

以下是建議工作順序。各版本的畫面文字可能不同，應以工具目前顯示的檢查結果為準。

1. **盤點原程式。** 記錄架構、依賴、安裝步驟、啟動參數、資料位置與系統整合。
2. **準備可還原的乾淨環境。** 安裝工具，建立 VM 快照；確認擷取驅動可用，處理待重新啟動狀態及背景活動。
3. **選擇封裝環境與輸入方式。** 指定安裝器，或在擷取期間執行手動安裝／複製。
4. **填入身分資料。** 先確定預定發行通道、Publisher、版本及架構。
5. **執行安裝並設定啟動點。** 包含需要的靜態資源；不要把測試者帳號、權杖或機器專屬設定包進去。
6. **檢查輸出內容。** 核對 manifest、檔案、啟動點及服務報告，排除無關擷取內容。
7. **完成簽章及驗收。** 在另一個乾淨環境安裝，測試完整功能，再發布下一版驗證更新。

擷取及手動安裝機制見 [Packaging Tool 流程](https://learn.microsoft.com/en-us/windows/msix/packaging-tool/create-app-package)。乾淨機器、快照、敏感資料排除與跨機驗證是此研究建議的操作控制。

### Python 與 Kivy 特別注意

使用者文章提供了一個既有程式資料夾的封裝案例，不能推論任意 Python 程式均可照搬。建議把驗證分成兩段：先在沒有開發環境的機器驗證 PyInstaller 產物，再驗證 MSIX 版本。這樣可以區分原始打包缺件與 MSIX 部署行為造成的問題。

- 資源與動態載入：檢查字型、圖像、KV 檔、DLL、外部工具及其他實際需要的內容。
- 路徑：從開始功能表啟動測試；不要依賴開發時的目前工作目錄。
- 寫入：將設定、日誌與資料庫放在可寫的使用者資料位置；避免寫回套件安裝目錄。
- 機器差異：測試中文使用者名稱、空格路徑、標準使用者及目標 CPU 架構。
- 升級：確認新版仍讀得到舊版設定，並明確決定解除安裝時哪些資料應保留。

這些是待實測項目；路徑與寫入風險依 [封裝前準備](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-prepare)，必要時可用 [PSF](https://learn.microsoft.com/en-us/windows/msix/psf/package-support-framework-overview) 處理特定問題。

## 簽章與信任

| 發行情境 | 簽章與信任做法 |
| --- | --- |
| 開發測試 | 可自簽，並只在受控測試裝置建立相應信任 |
| 企業內部分發 | 由企業配置憑證信任、部署權限與套件政策 |
| 官網直接下載 | 使用目標裝置可驗證的正式簽章，另測裝置政策及安裝體驗 |
| Microsoft Store | 依 Store 提交流程處理，由 Store 簽署交付套件 |

一般部署要同時滿足有效簽章及裝置信任；正式簽章建議加上時間戳記，讓有效簽署時間可供後續驗證。簽章不是程式無害或通過 Store 審核的證明。[簽章概述](https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview)。

Windows 11 另有專供測試的未簽章安裝機制，需要特殊 Publisher OID 與 `-AllowUnsigned`；它會使用不同於正式簽章套件的身分，不能當作一般使用者散發方式。[未簽章測試套件](https://learn.microsoft.com/en-us/windows/msix/package/unsigned-package)。因此「任何未簽 MSIX 都絕不可能安裝」過度絕對，但正式交付仍應簽章。

## 部署與更新

| 通道 | 需要準備 | 主要取捨 |
| --- | --- | --- |
| Store MSIX | Store 身分與提交程序 | 使用 Store 分發及更新機制 |
| 官網 `.msix`／bundle | 簽章、下載位置、安裝指引 | 單獨檔案未配置後續更新關聯 |
| `.appinstaller` | 套件 URL、身分、版本與更新設定 | 可建立自管更新通道，須維護伺服器與部署描述檔 |
| 企業管理 | 管理平台、群組、信任及政策 | 適合集中分發及分階段部署 |

通道能力參考 [MSIX 概述](https://learn.microsoft.com/en-us/windows/msix/overview) 與 [App Installer](https://learn.microsoft.com/en-us/windows/msix/app-installer/app-installer-file-overview)。官網連結宜直接提供可下載的 `.appinstaller`；`ms-appinstaller:` URI 協定自 2023 年 12 月起預設停用，不能把舊的一鍵網頁啟動教學當作普遍可用流程。[官方說明](https://learn.microsoft.com/en-us/windows/msix/app-installer/app-installer-file-overview)。

App Installer 可設定啟動時或背景檢查、是否顯示提示，以及是否要求更新後才能啟動。`UpdateBlocksActivation` 要搭配 `ShowPrompt`；各設定有 Windows 最低版本要求。正常更新朝較高版本前進，降版須另外配置與測試。[更新設定](https://learn.microsoft.com/en-us/windows/msix/app-installer/update-settings)。

建議發版順序是先上傳新套件及依賴，確認可下載及驗證，再更新描述檔指向新版本。保留上一版與資料備份；套件回退不代表資料庫結構會自動回退。此為發版設計建議。

## 相容性與選型限制

| 需求 | 判斷與處理方向 |
| --- | --- |
| 安裝 Windows 驅動 | MSIX 不支援封裝驅動；須另外設計部署依賴或保留其他安裝方式 |
| 每位使用者的 Windows service | 不支援；不能與一般 session 0 服務混為一談 |
| 機器層級 Windows service | 有條件支援，需符合帳號、系統版本與權限要求 |
| 寫入安裝目錄或依賴原捷徑工作目錄 | 調整程式路徑，或評估 PSF |
| 全機登錄與跨程式狀態共享 | 先盤點實際用途，確認虛擬化後的可見性與權限 |
| COM、Shell 或外部程式載入模組 | 依使用的擴充模型、manifest 及 Windows 版本逐項驗證 |

驅動、per-user service 及傳統程式相容性依 [封裝前準備](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-prepare)。機器層級服務支援從 Windows 10 2004 開始，安裝需要管理員權限；官方亦列出套件外服務依賴等限制。[服務轉換](https://learn.microsoft.com/en-us/windows/msix/packaging-tool/convert-an-installer-with-services)。

部分舊文件用 Desktop Bridge 的限制描述整體環境，不宜把所有擴充情境一概判為可行或不可行。決策時應固定目標 Windows 版本、應用程式模型及實際 API，再做原型驗證；檔案及登錄行為可對照 [執行模型文件](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)。

工程上可依需求採以下方向：單純桌面工具優先試作 MSIX；已有成熟 MSI 且需要高度系統整合的產品，先計算轉換成本；驅動及系統元件較多的產品，先劃分部署責任再選格式。不要只因格式較新就替換整個安裝架構。

## 命令列操作範例

以下供具備 Windows SDK 工具的測試環境使用，不會建立可執行的示範 App。須先準備完整 payload、有效 manifest、視覺資產及依賴；輸出目錄放在 payload 外。命令分段執行，前一步成功才繼續。

```powershell
# 檢查 PATH 是否可找到 Windows SDK 工具
Get-Command MakeAppx.exe, SignTool.exe

# payload 內須包含 AppxManifest.xml 與全部程式檔案
New-Item -ItemType Directory -Path .\out -Force
MakeAppx.exe pack /d .\payload /p .\out\ResearchApp.msix

# 解開以檢查內容，使用未存在的新目錄
MakeAppx.exe unpack /p .\out\ResearchApp.msix /d .\inspect
```

封裝與解包語法依 [MakeAppx](https://learn.microsoft.com/en-us/windows/win32/appxpkg/make-appx-package--makeappx-exe-)。

以下簽章範例假設憑證及可用私鑰已在目前使用者憑證存放區。替換指紋與憑證供應者的 RFC 3161 時間戳記 URL；token／HSM／雲端簽章仍需相應供應者設定。`/sha1` 是憑證指紋的選擇方式，內容摘要以 `/fd SHA256` 指定。

```powershell
SignTool.exe sign /sha1 REPLACE_WITH_CERT_THUMBPRINT /fd SHA256 /tr https://REPLACE_WITH_TIMESTAMP_HOST /td SHA256 .\out\ResearchApp.msix
SignTool.exe verify /pa /v .\out\ResearchApp.msix
```

套件簽署及摘要要求依 [套件簽章文件](https://learn.microsoft.com/en-us/windows/msix/package/sign-app-package-using-signtool)；時間戳記、憑證選擇及驗證參數依 [SignTool 參考](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool)。

在已信任簽章且政策允許的測試裝置，使用下列命令安裝至目前使用者。若有 framework 套件依賴，應另準備相應版本並使用 `-DependencyPath`；不要用任意網路下載的 DLL 湊齊。

```powershell
Add-AppxPackage -Path .\out\ResearchApp.msix
```

`Add-AppxPackage` 的用途及依賴參數見 [官方命令參考](https://learn.microsoft.com/en-us/powershell/module/appx/add-appxpackage?view=windowsserver2025-ps)。

## 發行前驗收表

此表是建議的測試計畫，尚未針對特定應用程式執行。

| 測試 | 通過條件 |
| --- | --- |
| 乾淨環境首次安裝 | 沒有開發工具或既有 runtime 偶然補齊依賴 |
| 簽章與政策 | 發行者符合預期，目標裝置接受信任鏈與部署政策 |
| 標準使用者啟動 | 從開始功能表可啟動，核心操作正常 |
| 字型與資源 | 中文顯示、圖像、KV 檔及動態載入內容正確 |
| 設定與寫入 | 首次執行、重新啟動及中文帳號下可讀寫資料 |
| 版本升級 | v1 到 v2 可更新，使用者設定及文件符合預期 |
| 程式使用中更新 | 使用者提示、延後或重啟行為符合設計 |
| 斷網與下載失敗 | 現有版本行為可接受，恢復網路後可重新更新 |
| 解除安裝與重裝 | 程式移除，使用者文件保留規則與產品說明一致 |
| 架構與系統版本 | 每個宣稱支援的組合都有測試證據 |
| Store 發行 | 身分、能力與提交驗證符合目前要求 |

遇到安裝失敗時先保存完整錯誤與 ActivityId，區分信任、依賴、身分、架構、政策及檔案占用問題；不要先關閉系統安全機制。遇到封裝後才出現的啟動問題，先對照相同程式在未封裝環境的結果，再檢查工作目錄、DLL 及寫入路徑。

## 來源與後續實作

[來源索引](sources.md) 列出指定文章與官方文件的用途。此次完成的是公開資料研究與操作範例；尚未選定應用程式、產生實際 MSIX、購買憑證或提交 Store。下一步若要實作，需先確定程式產物、CPU 架構、最低 Windows 版本，以及 Store、官網或企業部署通道。
