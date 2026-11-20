**English** | [繁體中文](case-template.zh-TW.md)

# Case: <symptom and date>

- Status: investigating / hypothesis / resolved
- Symptom and reproducible trigger:
- Incident time, explicit UTC offset and time-zone ID:
- Windows build, hardware, driver/firmware and app versions:
- Recent changes and baseline incident frequency:
- Observed provider/ID pairs and full-message summary:
- Private evidence location (do not commit raw logs or dumps):
- Hypotheses, supporting evidence and conflicting evidence:

| Change/test | Previous state and rollback | Result under same trigger | Evidence | Date |
| --- | --- | --- | --- | --- |
| | | | | |

- Confirmed cause (or still unknown):
- Resolution and verification workload/observation period:
- Residual uncertainty and next action:
- Official references and date checked:

## Filling rules

- Change one variable at a time; record the previous value and how to roll back.
- Write "not tested" for items you did not test, never "normal".
- A short quiet period only means "not reproduced yet". Judge a fix only after observing longer than the original failure interval, repeated under the same trigger.
- When a case needs the same hardware settings recorded repeatedly, keep an observation log in the case folder, for example [display wake](2026-10-14-display-wake/observation-log.md) and [LOXJIE A40](2026-10-14-loxjie-a40-usb/observation-log.zh-TW.md) (Traditional Chinese only).
