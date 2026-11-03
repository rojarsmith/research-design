**English** | [繁體中文](README.zh-TW.md)

# Limit-order-book DEX and token launch platform

Research date: 2026-11-04. The full report is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

## Summary

- Goal: a simple DEX with limit orders only (no AMM), one wallet such as MetaMask across several chains, no censorship, and a configurable fee for the owner. It serves token issuers who want to launch on popular chains and add or pull their own liquidity by placing and cancelling orders.
- This is feasible. On L2s and fast chains (Base, Arbitrum, BNB Chain, Monad) a fully on-chain central limit order book costs cents or less per action. Clober, Kuru, DeepBook and OpenBook already do it.
- Recommended architecture: matching entirely in smart contracts, the same contracts deployed on each EVM chain at the same address, and one frontend over all of them. No off-chain matcher and no bridge. This is the only option that is actually censorship-resistant, and it is also the simplest.
- One MetaMask address already works on every EVM chain. Solana would need a separate Rust program and is a second phase.
- Contracts: market factory, order book (tick bitmap plus FIFO queues, funds locked at placement, pull-based claims for makers, capped fills per transaction), token factory, fee controller.
- Tooling: Truffle and Ganache were sunset in 2023. Use Foundry or Hardhat 3. Both now run Foundry-style Solidity fuzz and invariant tests. Hardhat 3 is stronger for TypeScript integration tests with viem and for multichain simulation. Foundry has the more mature tooling and is what most reference order-book projects and audit PoCs use. Either way, anvil (the local chain that replaces Ganache, including mainnet forks) and cast work standalone. Add Slither and Aderyn, Safe for owner actions, and Sepolia-based testnets. The full table and the local dev loop are in the Chinese report.
- Revenue: taker fee, optional maker rebate, token launch fee, market creation fee, featured listings, referrals. Hard-code a fee cap and route changes through a multisig and timelock.
- Stack: Solidity with Foundry or Hardhat 3 and OpenZeppelin, Next.js or Vite with wagmi and viem, Ponder or Envio for indexing, TradingView Lightweight Charts, an external audit.
- The main risks are legal and fraud, not technical. Permissionless token launches where issuers can pull liquidity at will are the standard rug-pull pattern and the ICO case securities regulators watch most. The contracts can be uncensorable, but the fee-collecting owner and the website can be held liable. Get legal advice before launch.

Written from general knowledge, not checked against official docs, statutes or on-chain data. Project status, fees and regulations change quickly. Not investment or legal advice.
