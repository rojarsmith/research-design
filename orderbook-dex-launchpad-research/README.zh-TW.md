[English overview](README.md) | **繁體中文**

# 限價訂單簿 DEX 與 ICO 發幣平台

研究日期：2026-11-04（Asia/Taipei）。問題：用 2026 年主流技術做一個簡單的去中心化交易所，需求如下，做不做得到、該怎麼做？

1. 只有限價買賣單，不要自動造市（AMM）。
2. 用一個錢包（以 MetaMask 為例）連線，就能存入、交易多條鏈上的資產。
3. 不審查、完全自助。
4. 交易所擁有者可以設定每筆成交的抽傭，或用業界其他方式賺錢。

目的：讓 ICO 發起方在多條熱門鏈上自由發幣，並自己掛單注入或撤單收回資金；Uniswap 那種 AMM 曲線反而干擾發起方操盤。

## 結論

- **做得到，而且技術上不難。** 鏈上限價訂單簿（CLOB）在 2017 年的以太坊 L1 跑不動，但在 2026 年的 L2 與高效能鏈上（Base、Arbitrum、BNB Chain、Monad 等），每次掛單、成交的 gas 只要幾分錢以下，已有 Clober、Kuru、DeepBook、OpenBook 等先例。
- **建議架構：撮合完全寫在智能合約裡，每條鏈各部署一份相同合約，前端統一整合。** 不設鏈下撮合伺服器、不做跨鏈橋。這是唯一能真正做到「無審查」的做法，也最簡單、最不需要信任營運方。
- **「一個錢包多條鏈」用 EVM 就自然達成**：同一個 MetaMask 地址在所有 EVM 鏈上都通用，前端切換網路即可。Solana 等非 EVM 鏈要另寫一套合約，放第二階段。
- **ICO 發起方的需求，用訂單簿剛好滿足**：發起方就是一般掛單者（maker），想注入資金就掛買單、想出貨就掛賣單、想收回就撤單，沒有 AMM 曲線和 LP 池。
- **收費寫在合約裡**：吃單費（taker fee）＋可選的掛單費／返佣，加上發幣費、開市場費。費率由擁有者設定，但合約內要寫死上限並加時間鎖，否則使用者不會信任。
- **真正的風險不在技術，而在法律與詐騙**：「誰都能發幣、發起方隨時抽走資金、平台抽成」幾乎就是 rug pull 的完美工具，也是證券監管最在意的 ICO 情境。合約可以做到無法審查，但收費的擁有者和網站前端都是可被追究的實體，上線前必須找律師。

## 一、為什麼 2026 年做得到

| 時代 | 問題 | 現況 |
| --- | --- | --- |
| 2017 EtherDelta | 以太坊 L1 每筆操作數美元到數十美元 gas，12 秒出塊 | 鏈上訂單簿只能做到「鏈下掛單、鏈上成交」 |
| 2020–2023 | DEX 為了省 gas 改用 AMM（Uniswap） | 訂單簿退到中心化或專用鏈（dYdX、Hyperliquid） |
| 2024–2026 | L2 手續費降到分以下（EIP-4844 blob），高效能 EVM 鏈出現 | 完整鏈上 CLOB 在一般 EVM 鏈上可行 |

可參考的鏈上訂單簿專案（細節需上線前再查證）：

| 專案 | 鏈 | 特點 |
| --- | --- | --- |
| Clober | EVM（Base 等） | 全鏈上 CLOB，用分段樹處理價格層級，壓低 gas |
| Kuru | Monad | 全鏈上 CLOB＋可選的 AMM 金庫 |
| DeepBook | Sui | 鏈上共用訂單簿，任何人可開交易對 |
| OpenBook（Serum 後繼） | Solana | 鏈上 CLOB，任何人可開市場 |
| Hyperliquid 現貨 | 自家 L1 | 透過競標部署新代幣（HIP-1），但需要自建整條鏈，不適合本案 |

## 二、三種架構比較

| 架構 | 做法 | 無審查程度 | 複雜度 | 適合本案 |
| --- | --- | --- | --- | --- |
| A. 全鏈上 CLOB，每鏈一份 | 掛單、撮合、結算都在合約內；每條鏈獨立部署 | 高：沒有營運方能擋單 | 低 | **推薦** |
| B. 鏈下撮合＋鏈上結算 | 使用者簽 EIP-712 訂單，伺服器撮合後送鏈上結算（0x、早期 dYdX 模式） | 低：伺服器可以拒單、插隊、停機 | 中 | 不符合需求 3 |
| C. 中心結算鏈＋跨鏈存款 | 各鏈存款進金庫，在一條主鏈上統一記帳撮合（Hyperliquid、Orderly 模式） | 低：提款要靠營運方或驗證者簽名 | 高：跨鏈橋是最常被駭的元件 | 規模大後才考慮 |

選 A 的理由：

- ICO 代幣本來就發在某一條鏈上，該鏈的交易對自然用同一條鏈的 USDC、USDT 或原生幣報價，不需要跨鏈。
- 沒有伺服器掌握撮合，就沒有人能審查；前端掛了，任何人都能直接呼叫合約或另做前端。
- 不碰跨鏈橋，少掉最大的資安與託管責任。

代價：各鏈流動性分開，同一個代幣若在兩條鏈發行，會是兩個獨立市場。對 ICO 場景影響不大。

## 三、合約設計

### 1. 合約組成

| 合約 | 職責 |
| --- | --- |
| `MarketFactory` | 任何人付開市費即可建立交易對（base／quote、tick size、最小下單量） |
| `OrderBook`（每個市場一個，或單例合約管理所有市場） | 掛單、撮合、撤單、領取成交款 |
| `TokenFactory` | 發幣：一鍵部署標準 ERC-20，可選同時開市場 |
| `FeeController` | 費率、費率上限、收款地址、時間鎖 |

### 2. 訂單簿資料結構

- **價格用 tick 表示**（例如每 tick 0.01% 價差），用 bitmap 快速找最佳買價／賣價。
- **每個價格層級是一條 FIFO 佇列**，同價先到先成交。
- **掛單時就把資金鎖進合約**：買單鎖 quote，賣單鎖 base。成交保證不會因為使用者把錢轉走而失敗。
- **成交款用「領取」模式**：吃單者當下拿到款；被吃到的 maker 成交款記在合約內帳，由 maker 自己 `claim`。這樣一筆大吃單不必逐一轉帳給每個 maker，gas 可控。
- **每筆交易最多撮合 N 張單**，超過就停，避免超出區塊 gas 上限。
- **最小下單量與最小 tick**，防止用灰塵單塞爆訂單簿。

### 3. 主要函式

```solidity
// 掛限價單：先和對手盤撮合，剩餘部分掛進訂單簿
function placeLimit(MarketId m, Side side, int24 tick, uint128 amount,
                    bool postOnly, uint16 maxFills) returns (OrderId);

function cancel(OrderId[] calldata ids);          // 撤單，退回未成交資金
function claim(OrderId[] calldata ids);           // 領取已成交款
function placeMarket(MarketId m, Side side, uint128 amount,
                     uint256 minOut);              // 可選：市價單＝不掛單的限價單
```

`postOnly` 讓 ICO 發起方保證自己只當 maker、不會意外吃到別人的單。

### 4. 代幣相容性

| 問題 | 處理 |
| --- | --- |
| 轉帳抽稅代幣（fee-on-transfer） | 用轉帳前後餘額差計帳，或直接拒絕 |
| 會變動餘額的代幣（rebasing） | 拒絕上市 |
| 原生 ETH／BNB | 內部包成 WETH／WBNB |
| 惡意代幣（轉帳時回呼攻擊） | `ReentrancyGuard`、checks-effects-interactions、每個市場資金隔離記帳 |

`TokenFactory` 發出的標準代幣可以在前端標示「標準合約、無增發、無黑名單」，和外部帶進來的代幣區分。

### 5. 錢包體驗

- **EIP-7702**（2025 年 5 月 Pectra 升級）：讓 MetaMask 一般帳戶一次簽名完成「授權＋掛單」，也能設 session key 減少彈窗。
- **Permit2 或 EIP-2612 permit**：用簽名取代每個代幣一次的 `approve` 交易。
- 第一版可以先用傳統 `approve`＋`placeLimit` 兩步，7702 當作體驗優化。

## 四、多鏈部署

- 用 CREATE2（例如 CreateX 部署器）讓合約在每條 EVM 鏈上地址相同，前端與使用者都好辨識。
- 第一批建議：**Base、Arbitrum、BNB Chain**（使用者多、gas 低、MetaMask 預設支援），之後看需求加 Monad、Polygon 等。以太坊主網 gas 仍偏貴，不適合鏈上 CLOB。
- 前端在使用者選市場時自動要求切換網路（wagmi 的 `switchChain`）。
- 首頁可以跨鏈列出所有市場，但每個市場只在自己那條鏈上成交。
- **Solana 第二階段**：MetaMask 已支援 Solana 帳戶，但合約要用 Rust（Anchor）重寫，或直接把 Solana 市場導到 OpenBook。

## 五、2026 年技術選型

| 層 | 選擇 | 說明 |
| --- | --- | --- |
| 合約 | Solidity 0.8.x＋Foundry＋OpenZeppelin 5 | 業界主流；Foundry 的 fuzz 與 invariant 測試對撮合邏輯特別重要 |
| 前端 | Next.js＋TypeScript＋Tailwind＋shadcn/ui | 也可用 Vite＋React，純靜態部署更容易多處鏡像 |
| 錢包 | wagmi＋viem，連線 UI 用 RainbowKit 或 Reown AppKit | MetaMask、Rabby、Coinbase Wallet 都能接 |
| 圖表 | TradingView Lightweight Charts | 開源、輕量 |
| 索引 | Ponder 或 Envio（TypeScript），存 PostgreSQL | 從合約事件產生 K 線、成交紀錄、訂單簿快照、使用者掛單列表 |
| 即時資料 | 索引器提供 WebSocket；訂單簿最佳價也可直接讀合約 | 索引器只是加速，不是必要依賴 |
| RPC | Alchemy、QuickNode 或自架節點 | 每條鏈至少兩家備援 |
| 資安 | Slither、Foundry invariant、外部稽核、Immunefi 漏洞賞金 | 稽核費用約數萬到十幾萬美元，視程式碼量 |

前端維持「沒有後端也能用」：索引器只做查詢加速，掛單、撤單、領款都直接從瀏覽器呼叫合約。前端可放 IPFS，降低被下架的影響。

### 開發工具鏈

Consensys 在 2023 年 9 月宣布停止維護 Truffle 與 Ganache，之後業界主要改用 Foundry 和 Hardhat。舊工具與替代品對照：

| 舊工具 | 現況 | 替代 |
| --- | --- | --- |
| Ganache（本地鏈） | 已停止維護 | **Anvil**（Foundry 內建），或 Hardhat 內建的本地節點 |
| Truffle（編譯、測試、部署） | 已停止維護 | **Foundry**（`forge`）或 **Hardhat 3** |
| Truffle migrations | 隨 Truffle 停止 | `forge script`，或 Hardhat Ignition |
| web3.js | 已封存 | **viem**（新專案首選）或 ethers v6 |
| Brownie（Python） | 已停止維護 | Ape Framework，或直接改用 Foundry |
| Goerli、Holesky 測試網 | 已關閉 | **Sepolia**、Hoodi，以及各 L2 的 Sepolia 測試網 |

建議的開發流程與工具：

| 用途 | 工具 | 說明 |
| --- | --- | --- |
| 合約框架 | **Foundry**（`forge`、`cast`、`anvil`、`chisel`） | 用 Solidity 寫測試，速度快；訂單簿這種重邏輯合約特別適合。也想用 TypeScript 寫測試的話，可以搭配 Hardhat 3 |
| 本地鏈 | `anvil` | 取代 Ganache：預設 10 個有錢的測試帳戶，chain id 31337；`--block-time` 模擬出塊時間 |
| 分叉主網測試 | `anvil --fork-url <RPC>` | 在本地複製 Base 或 Arbitrum 的真實狀態，拿真的 USDC 合約測試 |
| 多鏈本地測試 | 開多個 `anvil` 實例，各用不同 port 和 chain id | 測前端切換網路；OP 系鏈也可用 Supersim |
| 鏈上指令 | `cast` | 查餘額、呼叫合約、送交易、解碼 calldata、`cast run` 重播交易 |
| 單元與模糊測試 | `forge test`（fuzz、invariant） | 不變量範例：合約持有的代幣總額 ≥ 所有掛單鎖定額＋未領取成交款 |
| 進階模糊與形式驗證 | Echidna、Medusa、Halmos | 稽核前補強撮合與記帳邏輯 |
| gas 量測 | `forge snapshot`、`forge test --gas-report` | 每次改撮合邏輯都要比較 gas |
| 靜態分析 | Slither、Aderyn、Solhint | 放進 CI，每次 PR 都跑 |
| 除錯 | `forge test -vvvv` 呼叫追蹤、`forge debug`、Tenderly | Tenderly 可以視覺化追蹤測試網或主網的失敗交易 |
| 依賴管理 | `forge install`（git submodule）或 Soldeer | 取得 OpenZeppelin 等函式庫 |
| 部署 | `forge script --broadcast`＋CreateX | 各鏈部署到相同地址 |
| 原始碼驗證 | `forge verify-contract`＋Etherscan API V2 | V2 一把 API key 可以驗證多條鏈 |
| 擁有者權限 | Safe 多簽錢包 | 費率調整、緊急開關都由多簽執行 |
| 前端型別 | wagmi CLI 或 `@wagmi/cli` Foundry 外掛 | 從 Foundry 編譯產物自動產生 TypeScript ABI 與 hooks |
| 前端測試 | Vitest＋viem test client 接 anvil，端對端用 Playwright | Playwright 可搭配 Synpress 自動操作 MetaMask |
| 索引器開發 | `ponder dev` 接本地 anvil | 合約事件改了，索引器馬上更新 |
| CI | GitHub Actions＋`foundry-rs/foundry-toolchain` | 跑測試、Slither、gas 比較 |
| 測試幣 | 各鏈 Sepolia 水龍頭（Alchemy、QuickNode、Coinbase CDP 等） | 公開測試前取得 ETH；USDC 有官方測試網水龍頭 |

MetaMask 接本地鏈：新增自訂網路 `http://127.0.0.1:8545`、chain id 31337，再匯入 anvil 印出的測試私鑰。**開發用的 MetaMask 最好用獨立的瀏覽器設定檔**，別和放真錢的錢包混用：anvil 的預設私鑰是公開的，任何人轉進這些地址的主網資產都會立刻被盜。

日常開發迴圈：

1. 開一個終端機跑 `anvil`。
2. `forge script script/Deploy.s.sol --rpc-url http://127.0.0.1:8545 --broadcast` 部署合約與測試代幣。
3. `ponder dev` 和前端 `pnpm dev` 都接本地鏈。
4. 用 MetaMask 下單，用 `cast` 查狀態；改合約就重新部署，或用 `anvil --load-state`／`--dump-state` 保留測試資料。

## 六、收費方式

| 收入 | 做法 | 業界常見水準 |
| --- | --- | --- |
| 吃單費（taker fee） | 合約在成交時從吃單方收取 | 0.05%–0.3% |
| 掛單費／返佣 | maker 免費或負費率（返佣）吸引掛單 | 0% 或 −0.01% |
| 發幣費 | `TokenFactory` 部署時收固定費用 | 等值數美元到數百美元 |
| 開市場費 | 建立新交易對收費，順便擋垃圾市場 | 固定費用 |
| 首頁推薦位 | 付費置頂或曝光 | 鏈下收費即可 |
| 推薦分潤 | 引薦人分走部分手續費 | 10%–30% 手續費 |
| 前端加價 | 合約收基本費，官方前端另加一小筆 | 第三方前端可不加 |

讓使用者信任費率設定的做法：

- 合約寫死**費率上限**（例如 taker 最多 1%），擁有者只能在上限內調整。
- 調費走**多簽錢包＋時間鎖**（例如 48 小時後生效），使用者看到公告有時間撤單。
- 可按市場設定不同費率，例如自家 `TokenFactory` 發的幣給優惠。
- 手續費以成交的 quote 資產收取（USDC 等），收入比較穩定。

## 七、「無審查」能做到哪裡

| 層 | 能否做到無審查 | 說明 |
| --- | --- | --- |
| 智能合約 | 可以 | 不設暫停、黑名單、可升級代理，部署後誰都無法改 |
| 撮合 | 可以 | 全鏈上撮合，沒有伺服器可擋單 |
| 前端網站 | 有限 | 網域、主機、雲端服務都會收到檢舉與法律要求；Uniswap Labs 的官方前端也會隱藏特定代幣 |
| 擁有者本人 | 不可以 | 收手續費的地址與公司是可被追查、可被起訴的實體 |

取捨建議：

- **合約不可升級，或只有限縮權限的升級**：完全不可升級最無審查，但發現漏洞也修不了。常見折衷是保留「只能停止新掛單、不能動到使用者資金」的緊急開關，讓使用者永遠能撤單、提款。
- **前端與協定分開**：官方前端可依法屏蔽制裁地址或特定國家，協定本身保持中立，社群可以自行架設其他前端。

## 八、ICO 場景的特別風險

### 1. 詐騙與 rug pull

「任何人發幣＋發起方隨時撤出資金」就是 rug pull 的標準流程：發起方掛買單撐價、吸引散戶買入，再撤掉買單、倒貨。平台不審查，但可以做**透明度工具**讓買方自行判斷：

- 標示發起方錢包、其持幣比例與掛單佔比。
- 顯示代幣是否由 `TokenFactory` 標準合約發出（無增發、無黑名單、無轉帳稅）。
- 可選的**承諾機制**：發起方自願把部分買單或代幣鎖定一段時間，前端顯示「鎖倉至某日」。這是自願的信任訊號，不是強制審查。

### 2. 法律

不是法律意見，上線前務必找熟悉加密資產的律師。主要風險：

- **證券法**：多數 ICO 代幣在美國等地可能被認定為證券；為未註冊證券提供交易場所並收費，可能被認定為未註冊的交易所或經紀商。
- **台灣**：金管會對虛擬資產服務提供者（VASP）有登記與洗錢防制要求，相關專法正在推動，具體適用範圍要查最新法規。
- **歐盟**：MiCA 對加密資產服務提供者有執照要求；「完全去中心化」可能豁免，但有收費擁有者的平台很難主張完全去中心化。
- **制裁與洗錢**：Tornado Cash 開發者的刑事案件顯示，即使合約本身不可審查，開發者仍可能被追究。
- **詐騙協助責任**：平台明知大量詐騙代幣仍收費，可能被主張協助詐欺。

降低風險的常見做法：公司設在法規明確的地區、官方前端做制裁名單與地理封鎖、服務條款、不對代幣背書、不碰使用者私鑰與資金託管。

## 九、開發路線與工時估計

| 階段 | 內容 | 粗估 |
| --- | --- | --- |
| 1. 原型 | 單一鏈（Base 測試網）、單一交易對的 `OrderBook`＋簡單前端 | 3–4 週 |
| 2. MVP | `MarketFactory`、`TokenFactory`、費率控制、索引器、K 線、我的訂單 | 再 6–8 週 |
| 3. 資安 | invariant 測試、外部稽核、測試網公開測試、漏洞賞金 | 4–8 週（稽核排期常是瓶頸） |
| 4. 多鏈上線 | Arbitrum、BNB Chain 部署；透明度工具 | 2–4 週 |
| 5. 後續 | EIP-7702 體驗、Solana、推薦分潤、API 給程式交易者 | 視需求 |

估計以一位熟悉 Solidity 與 React 的全端工程師為準，不含法律與營運工作。

## 十、下一步

1. 先諮詢律師，確認公司所在地、能否收費、前端要做哪些限制，這決定其他設計。
2. 用 Foundry 寫單一市場的 `OrderBook` 原型，重點測撮合正確性與每筆 gas。
3. 讀 Clober、Kuru 的合約設計與稽核報告，借用成熟的價格層級資料結構。
4. 找幾個真正的 ICO 發起方試用，確認「自己掛單操盤」的流程是否真的比 AMM 好用。

---

本研究根據對話回答與一般知識整理，未逐一核對官方文件、法規原文或鏈上資料。專案狀態、手續費、法規變化很快，依賴前請再查證。不構成投資或法律建議。
