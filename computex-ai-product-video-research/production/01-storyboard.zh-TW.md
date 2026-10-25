# 創意企劃與雙語分鏡

[回主報告](../README.zh-TW.md)。本稿為拍攝／剪輯提案，產品功能須通過對應證據驗證後才可成為公開文案。

## 觀眾與訊息

主觀眾暫定設備製造商、嵌入式工程團隊及產品設計人員；次觀眾為採購與通路。第一層訊息是「這是用來設計設備介面的工具」，第二層是「設計能連到可見的操作與狀態回應」，第三層才是 AI 輔助、資料綁定與輸出細節。

畫面以產品與成果為中心，避免先放長篇公司歷史或沒有功能資訊的科技光效。背景以深色或淺色其中一套穩定使用，亮度要配合實際 UI；只用一種強調色標示操作位置。Logo 使用正式授權素材，不交給影像模型重新畫字。

90 秒主片每個場景都帶小型產品識別。重要訊息使用大字短句，雙語字幕為補充；不要求觀眾看清整個軟體桌面的細字。約每 15–20 秒出現可獨立理解的成果，方便中途加入。

## 90 秒主片分鏡草案

各時段是目標畫面長度，旁白不必填滿；旁白錄出後重新對齊字幕，不能把表格時間直接當成最終 SRT。

| ID／時間 | 畫面與拍攝方式 | 英文畫面短句／繁體中文 | 英文旁白草稿 | 證據及音效 |
| --- | --- | --- | --- | --- |
| S01／0–8 秒 | 實機觸控，立即看到畫面回應；產品名稱進場 | Design interfaces for your devices.／為設備打造操作介面。 | Meet EDT HMI Studio, a workspace for creating embedded user interfaces. | 實機素材 A02；輕微進場音效，音效為後製 |
| S02／8–19 秒 | 編輯器局部：選取元件、修改文字／外觀、看到改變 | Edit the interface visually.／以視覺化方式編輯介面。 | Arrange your screens and refine the details in a visual editor. | C01；不把空專案到完成的步驟假裝瞬間完成 |
| S03／19–31 秒 | 真實 AI 請求、工具操作、結果與一次人工調整 | Describe a change. Review the result.／描述修改，檢查結果。 | Describe a change to the AI assistant, review the result, and keep control of the design. | C02；只保留可重現功能，縮時需標示 |
| S04／31–43 秒 | Tags／資料綁定局部；選一個 WaterLevel 值，畫面對應 | Connect values to the interface.／讓數值連動畫面。 | Connect interface elements to the values used by your application. | C03；確認用的是模擬資料還是實際通訊 |
| S05／43–56 秒 | 洗衣機範例，WaterLevel 20→45→65，數值與圓環同步 | See the state change.／看見狀態變化。 | In this washer demo, the display follows changing values as the cycle progresses. | C03；若使用模擬器，畫面註明 Demo data／示範資料 |
| S06／56–67 秒 | Pause 請求→Phase 確認→畫面暫停；用少量標示說明 | Show the confirmed state.／顯示已確認的狀態。 | A pause request and a confirmed pause are shown as separate steps in this demo. | C04；保留事件順序，不剪成按下即確認 |
| S07／67–79 秒 | 桌面成果與實機觸控呼應；實機全景→特寫 | See it on the target display.／在目標顯示器上查看成果。 | See the interface on the target display, and explore the workflow with our team. | C05；硬體部署流程未驗證則改成單純實機展示 |
| S08／79–90 秒 | 乾淨片尾：產品名、雙語 CTA、核准網址與 QR | See the demo. Talk to our team.／現場體驗，洽詢團隊。 | Visit the EDT HMI Studio demo to discuss your device and interface requirements. | C06；11 秒停留；未定展位不填虛構資訊 |

上述英文旁白刻意保留展示及停頓時間，不必高速念滿 90 秒。先試錄確認實際時長；不足以理解的鏡頭優先延長，刪除重複說法，而不是一律加速。

## 繁體中文字幕語意底稿

| 場景 | 字幕內容，定稿時再按音訊分句 |
| --- | --- |
| S01 | 認識 EDT HMI Studio，用於建立嵌入式使用者介面的工作空間。 |
| S02 | 在視覺化編輯器中安排畫面，調整設計細節。 |
| S03 | 向 AI 助理描述修改，檢查結果，掌握設計。 |
| S04 | 將介面元件連結至應用程式使用的數值。 |
| S05 | 在這個洗衣機示範中，畫面隨流程中的數值變化更新。 |
| S06 | 這個示範將暫停請求與確認暫停，呈現為兩個步驟。 |
| S07 | 在目標顯示器上查看介面，與我們一起了解操作流程。 |
| S08 | 歡迎體驗 EDT HMI Studio，討論您的設備與介面需求。 |

英文字幕採對應英文旁白，不把較短的英文畫面標題當逐字字幕。若繁體翻譯太長，拆成兩個連續字幕單位，維持與旁白相同意思；不要為縮字加入沒有說到的能力。

## 產品證據不足時的替換

| 原場景 | 若未驗證 | 可用替代 |
| --- | --- | --- |
| AI 編輯 | 指定版本無法穩定完成操作 | 改為人工視覺化修改，不在影片保留 AI 能力主張 |
| 資料連動 | 只有手動改值，未接實機控制器 | 明示「示範資料」，不說已與客戶控制板通訊 |
| 目標硬體 | 只有桌面預覽 | 改說 Preview the interface／預覽介面；移除實機部署暗示 |
| 多語 | 只有既有日文畫面，未驗證切換 | 不把影片雙語字幕說成產品多語功能 |
| 性能數字 | 無可重現基準 | 刪除數字，保留可觀察操作 |

## 30 秒循環版

| 時間 | 主畫面 | 畫面短句 |
| --- | --- | --- |
| 0–4 秒 | 實機回應＋產品名 | EDT HMI Studio |
| 4–11 秒 | 編輯器修改前後 | Design the interface／設計介面 |
| 11–19 秒 | 數值與示範狀態連動 | See the state change／看見狀態變化 |
| 19–24 秒 | 實機觸控特寫 | Explore the demo／體驗示範 |
| 24–30 秒 | CTA／QR／產品名 | Talk to our team／洽詢團隊 |

30 秒版重新編排，不把 90 秒片直接倍速播放。以無旁白也成立為主，保留雙語畫面文字；有聲版可僅加音樂。頭尾使用相近亮度與構圖，避免循環時突然全黑或音量跳變。

## 拍攝與剪輯語言

錄屏每個動作前後保留 2–3 秒，游標先停穩再點擊。拍攝一組乾淨 UI、一組帶游標及操作、一組硬體；剪輯時才能放大重點而不遮擋。硬體全景交代產品存在，特寫呈現觸控與結果；模擬器和實機外觀不可用無標示剪接混成單一真實連線。

不要生成看似 EDT 真實硬體的新接口或按鍵。需要背景時可用無字、低對比的抽象幾何；字幕、標誌、產品 UI 全部使用可編輯圖層或真實拍攝素材。
