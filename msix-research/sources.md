# MSIX 研究來源索引

查閱日期：2026 年 10 月 21 日，Asia/Taipei。Microsoft 文件用於查核其格式與平台規則；平台宣傳或概述中的成效數字，不視為獨立效能實測。本研究未採用安裝成功率等宣傳數值作為選型依據。

| 來源 | 用途 |
| --- | --- |
| [艾肯比 使用 MSIX Packaging Tool 封裝工具打包成 MSIX 檔案](https://vocus.cc/article/6860e967fd89780001a6382d) | 使用者指定文章，2025 年 7 月 28 日發布及更新；Python／Kivy 手動封裝案例 |
| [What is MSIX](https://learn.microsoft.com/en-us/windows/msix/overview) | 格式定位、套件身分及部署能力 |
| [Create an MSIX package from any desktop installer](https://learn.microsoft.com/en-us/windows/msix/packaging-tool/create-app-package) | 擷取流程、環境準備、Search 選項、手動安裝 |
| [App package requirements for MSI/EXE app](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msi/app-package-requirements) | Store 同時接受 MSI／EXE 的條件及提交格式 |
| [Prepare to package a desktop application](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-prepare) | 驅動、服務、路徑及傳統應用程式相容性盤點 |
| [Understanding how packaged desktop apps run on Windows](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes) | RuntimeBehavior、TrustLevel、安裝位置及虛擬化條件 |
| [How Visual Studio generates an app package manifest](https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/generate-package-manifest) | manifest、Identity、建置與 Store 身分 |
| [App packager MakeAppx.exe](https://learn.microsoft.com/en-us/windows/win32/appxpkg/make-appx-package--makeappx-exe-) | 封裝、解包及 bundle 語法 |
| [Sign an MSIX package](https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview) | 裝置信任、簽章、時間戳記及套件完整性 |
| [Sign an app package using SignTool](https://learn.microsoft.com/en-us/windows/msix/package/sign-app-package-using-signtool) | 憑證選擇、SHA256 及區塊雜湊要求 |
| [SignTool 命令參考](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool) | 憑證存放區、RFC 3161 時間戳記及簽章驗證參數 |
| [App Installer file overview](https://learn.microsoft.com/en-us/windows/msix/app-installer/app-installer-file-overview) | 部署描述檔及 URI 協定限制 |
| [App Installer file update settings](https://learn.microsoft.com/en-us/windows/msix/app-installer/update-settings) | 啟動／背景更新、提示、版本與最低系統要求 |
| [Convert an installer that includes services](https://learn.microsoft.com/en-us/windows/msix/packaging-tool/convert-an-installer-with-services) | Windows 10 2004 服務支援、管理員權限與服務依賴限制 |
| [Package Support Framework overview](https://learn.microsoft.com/en-us/windows/msix/psf/package-support-framework-overview) | 工作目錄、安裝目錄寫入等執行期修正 |
| [Add-AppxPackage](https://learn.microsoft.com/en-us/powershell/module/appx/add-appxpackage?view=windowsserver2025-ps) | 使用者安裝、DependencyPath 及更新套件家族 |
| [Create an unsigned MSIX package](https://learn.microsoft.com/en-us/windows/msix/package/unsigned-package) | Windows 11 特殊未簽章測試途徑及發行限制 |

指定文章為個人操作經驗，本文只摘要流程並校正通用性，未重製其截圖。官方文件若因 Windows 版本或執行模型描述出現範圍差異，應回到具體功能文件及目標系統實測，不能只用概述承諾全版本相容。
