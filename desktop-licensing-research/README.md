**English** | [繁體中文](README.zh-TW.md)

# Desktop software billing, licensing and IAM for startups

Research date: 2026-11-03. The full report is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

## Summary

- Split the system into three layers: payments (checkout, tax, receipts), licensing/entitlements (what a customer may use, on how many machines, until when), and identity (who the user is). Each layer can be bought; the startup mainly writes the glue that turns verified webhooks into entitlements.
- For a small Taiwan-registered team, a Merchant of Record is the default: Polar (5% + 50¢ on Starter, payouts via Stripe Connect Express, Taiwan supported), Paddle (5% + 50¢), or Lemon Squeezy (5% + 50¢, Stripe-owned since 2024). Neither Stripe direct nor Stripe Managed Payments accepts Taiwan-based businesses.
- Licensing: MoR built-in license keys (Polar, Lemon Squeezy) cover simple rules. Offline licenses, floating seats, feature entitlements and enterprise volume need Keygen (free self-hosted CE), Cryptlex or LicenseSpring. Paddle has no built-in license keys.
- IAM: start key-only if needed, but subscriptions, multiple devices and B2B require accounts. Desktop sign-in should open the system browser with OAuth 2.0 Authorization Code + PKCE and return via loopback or a custom URI scheme (RFC 8252); use the device grant (RFC 8628) for CLIs. Hosted options include WorkOS AuthKit, Clerk, Auth0 and Logto; the existing Keycloak in [iam-research](../iam-research/README.md) also fits.
- Anti-piracy: server-signed license files, periodic online checks and a grace period. Do not lock out paying users to stop crackers.

- Case study, Typora: payments and tax through FastSpring and 2Checkout as Merchants of Record; activation is in-house at `store.typora.io`, with no third-party licensing SDK in the client. The client uses `node-machine-id`, RSA verification strings and ByteNode-compiled code. Terms are a $14.99 one-time license, up to three devices per user, offline activation and no accounts. This is based on a read-only look at the install directory; the signing details are inferred.

- With a US LLC and an existing Stripe account, the simplest start is a one-time license sold through Payment Links or Checkout with Stripe Managed Payments turned on. It adds 3.5% on top of standard processing fees, and Stripe handles global VAT and sales tax. A webhook-driven backend, or Keygen, issues the license keys. Accounts and subscriptions can come later.

## Recommended stacks

| Stage | Stack |
| --- | --- |
| Fastest launch | Polar (MoR + license keys), optional email login |
| Subscriptions and multiple devices | Paddle or Polar + Keygen + WorkOS AuthKit or Clerk |
| US company, US market | Stripe Billing + Stripe Tax + Keygen or Stripe Entitlements + any IdP |
| B2B, on-premises, offline | Any payments + Cryptlex or LicenseSpring floating server + Keycloak or WorkOS for SSO/SCIM |
