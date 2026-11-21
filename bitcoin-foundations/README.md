**English** | [繁體中文](README.zh-TW.md)

# Bitcoin and Elliptic Curve Cryptography: How They Work

Research date: 2026-11-21. The full plain-language explainer is in Traditional Chinese; this page is an overview. The lab script uses only the Python standard library and prints in Traditional Chinese.

## Question

In plain language, how does Bitcoin work without a bank, and how does the elliptic curve cryptography behind it work?

## Findings in brief

- **Bitcoin is a public ledger everyone keeps a copy of.** The problem it solves is double spending without a trusted intermediary. The ledger holds unspent transaction outputs (UTXOs), like banknotes of arbitrary value; each can be spent once, and a "balance" is just the wallet's sum of notes you can spend.
- **Ownership is a private key.** Each output names a public key hash; spending it requires a signature over the whole transaction from the matching private key. Change the amount and the signature fails.
- **Proof of work decides who writes the next page and makes old pages expensive to rewrite.** Blocks chain by including the previous block's hash; a valid block's double SHA-256 hash must fall below a target. Finding one takes many tries, checking takes one. Difficulty retargets every 2016 blocks to keep about 10 minutes per block. The chain with the most accumulated work wins, and an attacker with under half the hash power falls behind exponentially, hence the custom of six confirmations.
- **Incentives:** the block subsidy started at 50 BTC and halves every 210,000 blocks (3.125 BTC after April 2024), capping supply just under 21 million; fees take over afterwards.
- **Elliptic curves give a one-way door.** On secp256k1 (y² = x³ + 7 over a 256-bit prime field), adding points is defined by chord, intersection, reflection. Computing k·G takes a few hundred point operations by double-and-add (about 380 and 5 ms in the lab); recovering k from k·G needs about 2¹²⁸ steps with the best known generic algorithm.
- **Signatures prove knowledge of the key without revealing it.** The Schnorr form (BIP340, active since Taproot in November 2021) is s = k + e·d, checked as s·G = R + e·Q. Reusing the nonce k across two signatures reveals d by simple algebra, as the PS3 incident in 2010 showed; the lab reproduces this.
- **Division of labour:** elliptic curves decide who may spend; proof of work decides the order. Neither needs a trusted party. Main limits: 51% attacks can reorder but not forge signatures, transactions are pseudonymous not anonymous, throughput is low, and a large quantum computer running Shor's algorithm would break the curve.

## Lab

`labs/toy_bitcoin.py` has five demos: `curve` draws y² = x³ + 7 mod 17 and walks a generator around its 18-element group; `keys` derives a secp256k1 public key and times a brute-force search to show the asymmetry; `sign` signs, verifies, rejects a tampered message and recovers the key from a reused nonce; `pow` finds hashes with 1 to 5 leading hex zeros; `chain` shows how editing an old block breaks every later link. Saved output is in `labs/example-output/all.txt`. It is a teaching tool, not a wallet.

## Documents

| Document | Content |
| --- | --- |
| [Explainer (Traditional Chinese)](README.zh-TW.md) | Part 1 Bitcoin: double spending, UTXOs, signatures, blocks and hash chain, proof of work and difficulty, rewards and supply, longest chain and confirmations, limits. Part 2 ECC: one-way functions, curve shape, point addition, finite fields, scalar multiplication versus discrete log, keys and addresses, Schnorr signatures, nonce reuse, ECC versus RSA, weaknesses. Sources and verification limits |
| [Lab script](labs/toy_bitcoin.py) | Standard-library Python demos described above |
