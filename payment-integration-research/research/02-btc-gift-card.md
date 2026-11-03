**English** | [完整繁體中文](02-btc-gift-card.zh-TW.md) | [Research home](../README.md)

# Selling BTC Gift Cards to Overseas Card Customers: Overview

Checked on 2026-11-03 (Asia/Taipei). This is research, not legal advice.

The scenario is a US LLC with an existing Stripe account for software sales that wants to let overseas customers buy its own BTC gift cards by card. The recommendation is not to sell through the existing Stripe account and not to act as the BTC seller. Instead, integrate a licensed fiat-to-crypto onramp that is merchant of record and handles KYC, AML and chargebacks.

- Stripe lists crypto exchanges and wallets, as well as gift cards and other stored value, as restricted businesses that need approval for each product.
- Selling BTC for fiat usually makes a business a money transmitter under FinCEN rules, with federal registration and most state money transmitter licenses.
- A gift card adds stored value and custody questions, and card-funded crypto purchases carry high fraud and chargeback risk.
- Stripe Crypto Onramp (public preview, application required) makes Stripe the merchant of record and liable for fraud and disputes. Customer countries and BTC support were not listed in the checked page.
- Keep crypto separate from the software business, check Taiwan VASP registration if operations are in Taiwan, and have counsel review the final flow.

The [complete Traditional Chinese note](02-btc-gift-card.zh-TW.md) covers the three options, gift-style workarounds, next steps and sources.
