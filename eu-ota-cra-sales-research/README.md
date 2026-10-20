# European OTA Server Sales, Customer Trust, and CRA Research

Research date: 2026-10-20. This topic assesses a managed OTA service based on Eclipse hawkBit, using the operator-provided context of a London Linode/Akamai host, free HTTPS, a Gandi domain, and an introductory digital-signing option.

## Main findings

- Do not market “ETSI EN 303 645 CRA certification.” EN 303 645 is a consumer-IoT security baseline; ETSI TS 103 701 provides an assessment method. Neither is a generic ETSI CRA certificate for an OTA SaaS.
- CRA conformity is product-scoped. Default products can generally use internal production control; Important Class I products have conditional routes; Important Class II and Critical products generally need third-party assessment or an applicable EU cybersecurity certification scheme.
- OTA management may be close to CRA Important Class I “network management systems,” depending on its actual core functionality. An OEM’s remote-processing product boundary may also include a cloud solution when the legal criteria are met. Confirm the service/software boundary and economic-operator roles before making claims.
- There is no universal public CRA or ETSI assessment price. ETSI does not set a lab tariff. Budget engineering and evidence work separately from penetration testing, optional certificates, and any legally required notified-body assessment.
- Customer trust should be built with evidence for signing-key custody, device-side verification, staged rollout and recovery, tenant isolation, vulnerability response, SBOM, support period, DPA, hosting, backups, and independent testing.

## Research documents

- [Traditional Chinese main report](README.zh-TW.md)
- [EU sales and positioning](research/01-market-entry.zh-TW.md)
- [CRA scope, product classification, and self-assessment](research/02-cra-scope-and-conformity.zh-TW.md)
- [ETSI baseline, testing, and assurance tiers](research/03-etsi-testing-and-assurance.zh-TW.md)
- [Cost planning and customer-trust evidence](research/04-costs-and-trust-plan.zh-TW.md)
- [Sources, limits, and open questions](research/05-sources.zh-TW.md)

This is research and planning material, not a legal opinion or a claim that Bitdove OTA has passed any assessment. See the Chinese main report for assumptions, caveats, and the recommended customer-facing wording.
