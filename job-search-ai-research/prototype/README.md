# Session broker prototype

Minimal proof that an install-free browser can hold a logged-in job-site session for an AI agent.

```bash
npm install
npx playwright install chromium          # ~440 MB into %LOCALAPPDATA%\ms-playwright, no installer

node smoke.mjs                           # persistence check
node fingerprint.mjs                     # automation fingerprint comparison
node session-broker.mjs login  104 https://login.104.com.tw/
node session-broker.mjs check  104 https://www.104.com.tw/jobs/main/
node session-broker.mjs export 104 https://www.104.com.tw/jobs/main/
node session-broker.mjs login  linkedin https://www.linkedin.com/login --exe "D:\PortableApps\GoogleChromePortable\App\Chrome-bin\chrome.exe"
```

Rules baked in:

- One persistent profile per site under `profiles/`, never Chrome's own User Data directory.
- `login` is always headed and waits for you to finish password, 2FA and CAPTCHA before it saves.
- `--disable-blink-features=AutomationControlled` is on so `navigator.webdriver` reads false.
- `profiles/` and `*.storageState.json` are git-ignored because they are equivalent to your credentials.
