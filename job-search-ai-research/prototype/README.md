# Session broker prototype

Minimal proof that an install-free browser can hold a logged-in job-site session for an AI agent.

```bash
npm install
npx playwright install chromium          # only for smoke.mjs / fingerprint.mjs; session-broker uses portable Chrome

# Portable Chrome: point at the real binary, never the PortableApps launcher
#   OK   C:\my\build\toolchain\GoogleChromePortable64-auto\App\Chrome-bin\chrome.exe
#   FAIL C:\my\build\toolchain\GoogleChromePortable64-auto\GoogleChromePortable.exe
node patchright-portable.mjs             # Patchright + portable Chrome check

node smoke.mjs                           # persistence check
node fingerprint.mjs                     # automation fingerprint comparison
node session-broker.mjs login  104 https://login.104.com.tw/
node session-broker.mjs check  104 https://www.104.com.tw/jobs/main/
node session-broker.mjs export 104 https://www.104.com.tw/jobs/main/
node session-broker.mjs login  linkedin https://www.linkedin.com/login --exe "D:\Other\Chrome-bin\chrome.exe"   # override the default binary
```

Rules baked in:

- One persistent profile per site under `profiles/`, never Chrome's own User Data directory.
- `login` is always headed and waits for you to finish password, 2FA and CAPTCHA before it saves.
- `session-broker.mjs` imports `patchright`, so `navigator.webdriver` reads false without extra flags; the flag is kept for the plain-Playwright scripts.
- `profiles/` and `*.storageState.json` are git-ignored because they are equivalent to your credentials.
