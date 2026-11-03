[English overview](02-btc-gift-card.md) | **繁體中文** | [回研究入口](../README.zh-TW.md)

# 讓海外客戶刷卡購買 BTC 禮物卡

查核日期：2026-11-03（Asia/Taipei）。情境：使用者已有美國 LLC 與 Stripe 帳號（用於賣軟體），想讓海外線上客戶刷卡購買自己提供的 BTC 禮物卡。本文是研究整理，不是法律意見；上線前需由熟悉 MSB 與加密資產的律師審閱實際流程。

## 結論

**不要用現有 Stripe 帳號直接賣，也不要自己當 BTC 的賣方。** 較可行的做法是整合有牌照的法幣購買加密貨幣服務（onramp），由對方擔任賣方，負責 KYC、洗錢防制與刷卡退單。

## 為什麼不能直接賣

| 面向 | 問題 |
| --- | --- |
| Stripe 規定 | 受限行業清單把「加密貨幣交易所與錢包」及「預付卡、禮物卡、虛擬點數等儲值商品」列為受限，需事先申請核准，核准只對個別產品有效。未經核准上線可能導致帳號關閉、款項凍結，連帶影響軟體收入 |
| 美國法規 | 以法幣出售 BTC 給他人，在 FinCEN 定義下通常屬於資金服務業（MSB）中的貨幣傳送業者，需要聯邦登記，多數州另需貨幣傳送牌照（MTL），並負擔洗錢防制計畫、KYC、制裁篩查與可疑交易申報；州牌照有保證金與淨值門檻 |
| 禮物卡形式 | 付款人與收卡人不同、兌換時間不確定；等待兌換期間等於替他人保管價值，可能另外涉及預付儲值規定與加密資產保管 |
| 刷卡風險 | BTC 送出後無法追回，盜刷集團常以盜卡買幣；持卡人提出退單時款項被扣回，幣卻追不回 |

## 三種做法

| 做法 | 法律上的賣方 | 你要做的事 | 適合 |
| --- | --- | --- | --- |
| **A. 整合持牌 onramp（建議）** | Stripe Crypto Onramp、MoonPay、Transak、Ramp、Banxa 等 | 在網站嵌入購買元件或導到對方頁面；收推薦分潤或另收服務費 | 想快速上線、不想申請牌照 |
| B. 代銷持牌發行商的 BTC 兌換券 | 發行商 | 作為經銷商販售兌換碼 | 有可合作的發行商；收款服務仍可能拒絕 |
| C. 自己做 | 你 | FinCEN 登記、各州牌照、高風險商戶收單、KYC 服務、制裁篩查、錢包與保管安全 | 有資本與合規團隊的金融科技公司 |

### Stripe Crypto Onramp 查核重點

- Stripe 擔任這些交易的 MoR，承擔詐騙與退單的全部責任，並處理 KYC 與制裁篩查。
- 需先送申請，官方表示多數申請在 48 小時內完成審核；核准後才能使用測試環境。
- 三種整合方式：Stripe 託管頁（crypto.link.com）、嵌入網站、行動 SDK；目前為公開預覽。
- 查核的文件未列出支援的客戶國家與幣種，「海外客戶」能否購買、是否支援 BTC 需逐項確認。

## 「禮物卡」的變通做法

onramp 的標準流程是付款人自己通過 KYC，幣直接送到他自己的錢包地址，不支援「先買卡、之後送人兌換」。若需要禮物形式：

1. **代買後轉送的體驗**：送禮人在你的網站選金額、寫祝福卡片，再透過 onramp 以收禮人提供的錢包地址購買。你只負責包裝與通知，不經手資金也不經手幣。實際上能否以他人地址購買，取決於各 onramp 的 KYC 與地址政策，需確認。
2. **找支援禮物或兌換碼功能的持牌業者**，由你擔任經銷或推薦方。本次未查證有哪些業者支援、支援哪些國家。

## 你的情況要特別注意

- **與軟體業務分開**：另設公司或至少另開收款帳號，避免加密貨幣業務的風險牽連軟體收入。
- **台灣監管**：若實際營運在台灣，台灣也要求虛擬資產服務商（VASP）完成洗錢防制登記，未登記營業有刑事責任；即使以美國 LLC 營運，也需確認台灣的適用範圍。本次未查核台灣法規原文。
- **律師審閱**：做法 A 通常不需自行申請牌照，但產品一旦涉及收款、保管或兌換碼，判斷就會改變。

## 下一步

1. 決定採用做法 A，列出目標客戶國家。
2. 向 Stripe Crypto Onramp 與一至兩家其他 onramp 確認：支援國家、是否支援 BTC、能否送到他人地址、分潤與費用、推薦方的責任。
3. 另開帳號提交申請，在測試環境驗證流程。
4. 請律師審閱最終流程，同時確認美國與台灣兩邊的義務。

## 來源

- [Stripe 受限與禁止行業清單](https://stripe.com/legal/restricted-businesses)
- [Stripe fiat-to-crypto onramp 文件](https://docs.stripe.com/crypto/onramp)
- [FinCEN 2013 虛擬貨幣指引](https://www.fincen.gov/news/news-releases/fincen-issues-guidance-virtual-currencies-and-regulatory-responsibilities)
- [FinCEN 禮物卡與貨幣傳送業者的行政解釋](https://fincen.gov/resources/statutes-regulations/administrative-rulings/definition-money-transmitterstored-value-gift)
- [WSGR：預付與禮物卡業務何時算 MSB](https://wsgr.com/en/insights/msb-or-not-msb-that-is-the-question-for-determining-applicability-of-anti-money-laundering-rules.html)

未建立任何帳號、未送出 onramp 申請，也未執行交易測試。
