**English** | [繁體中文](README.zh-TW.md)

# Job-search AI tools on GitHub: top three by stars in the last three months, and the install-free browser login gap

Research date: 2026-10-28. The full report is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

## Summary

- The three most-starred job-search AI projects of the last three months are career-ops (78K stars), ai-job-search (67K) and get_jobs (9.3K). All three keep "operate a logged-in job site" outside their product boundary.
- career-ops opens a fresh Playwright browser for every application and its code explicitly forbids `connectOverCDP`, user-data directories and attaching to an existing browser. ai-job-search has no browser automation at all. get_jobs logs in by WeChat QR code and only supports four mainland-China platforms.
- The observation that none of them support Playwright with an install-free browser plus 2FA login is correct. The cause is product policy and anti-bot risk, not a Playwright limitation.
- `prototype/` verifies on Windows 11 that Playwright's bundled Chromium (no installer, no admin rights) with a dedicated persistent profile keeps cookies and localStorage across restarts. 2FA is completed once by hand in a headed window and reused afterwards.
- The real blocker is bot detection. Default launches expose `navigator.webdriver = true`; headless also reports `HeadlessChrome`. Headed mode with `--disable-blink-features=AutomationControlled`, real Chrome, or Patchright is required for LinkedIn and Cloudflare-fronted sites such as 104.

## Comparison

| Aspect | career-ops | ai-job-search | get_jobs |
| --- | --- | --- | --- |
| Browser form filling | Yes, fresh Playwright window | No | Yes, Patchright |
| Persists login state | Deliberately not | N/A | Yes, in SQLite |
| 2FA handling | Manual each run, window placed off-screen | N/A | WeChat QR about weekly |
| Attach to logged-in browser | Forbidden in code | No | No |
| Portable browser path | `channel` only, no `executablePath` | N/A | No |
| Auto submit | No | No | Yes |
| Taiwan job boards | No | No | No |
| License | MIT | MIT | PolyForm Noncommercial |

## Prototype

| Script | Purpose |
| --- | --- |
| `prototype/smoke.mjs` | Cookie and localStorage persistence test with the bundled Chromium |
| `prototype/fingerprint.mjs` | Compares automation fingerprints of headless, headed and flag-suppressed launches |
| `prototype/session-broker.mjs` | Login once by hand, reuse the profile; supports `--exe` for a portable Chrome |

```bash
cd prototype && npm install && npx playwright install chromium
node session-broker.mjs login linkedin https://www.linkedin.com/login
node session-broker.mjs check linkedin https://www.linkedin.com/jobs/
```
