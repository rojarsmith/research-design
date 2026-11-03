**English** | [繁體中文](README.zh-TW.md)

# Hyperliquid and modern decentralized derivatives exchanges

Research date: 2026-11-03. The full report is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

## Summary

- EtherDelta (2016–2017) was slow because every deposit, trade and withdrawal was an Ethereum L1 transaction. Each one cost ETH gas, waited for a block and could be front-run.
- Hyperliquid runs its own L1. The order book, matching, margin and liquidation are native chain logic (HyperCore), not smart contracts. HyperBFT consensus finalizes in under a second.
- Trading needs no gas token. Orders are EIP-712 signatures, spam is limited by volume-based rate limits, and trade-only agent wallets remove per-order wallet prompts.
- Deposits come in through a validator-signed Arbitrum bridge, native USDC via CCTP, and bridges such as Unit for BTC, ETH and SOL.
- Perpetuals use a validator-reported oracle price, hourly funding, the community HLP vault for market making and liquidations, and auto-deleveraging as the backstop.
- Trust is concentrated in a small validator set and a validator multisig bridge. Validators intervened during the March 2025 JELLY incident.
- Ledger growth is linear but steep. State stays bounded through short-lived orders, per-account open-order caps, minimum order size and rate limits. History is kept out of consensus: nodes sync from state snapshots and prune old data, and full history lives in a public S3 archive and third-party indexers.
- The rest of the industry is moving the same way. Ethereum (EIP-4444) and Solana keep current state on nodes and leave full history to archive services.

Written from conversation answers and general knowledge, not checked against official documentation or on-chain data. Numbers such as latency, order caps and fees change; recheck them before relying on them. Not investment advice.
