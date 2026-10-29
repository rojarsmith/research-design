// session-broker.mjs
// Minimal "login once, reuse forever" session broker for job sites.
// Uses Patchright (stealth Playwright fork) with a portable Chrome binary (no installer, no admin rights) OR a
// portable Chrome via --exe, plus a dedicated persistent profile per site.
//
//   node session-broker.mjs login  <site> <url>   # headed; you finish 2FA by hand
//   node session-broker.mjs check  <site> <url>   # reuse profile; report login state
//   node session-broker.mjs export <site> <url>   # dump storageState json
//
// Options: --exe <path to portable chrome.exe>   --headless   --timeout <ms>
import { chromium } from 'patchright';
import fs from 'node:fs';
import path from 'node:path';
import readline from 'node:readline';

const args = process.argv.slice(2);
const [cmd, site, url] = args;
const opt = (k, d) => { const i = args.indexOf(k); return i >= 0 ? args[i + 1] : d; };
const flag = (k) => args.includes(k);
if (!cmd || !site || !url) {
  console.error('usage: node session-broker.mjs <login|check|export> <site> <url> [--exe chrome.exe] [--headless] [--timeout ms]');
  process.exit(2);
}

const profileDir = path.resolve('profiles', site);           // never Chrome's own "User Data"
fs.mkdirSync(profileDir, { recursive: true });

const launch = {
  headless: flag('--headless'),
  viewport: null,
  args: ['--disable-blink-features=AutomationControlled'],
};
// Default: portable Chrome. Point at App\Chrome-bin\chrome.exe, NOT the PortableApps
// launcher GoogleChromePortable.exe (the launcher spawns Chrome and exits, so the
// CDP pipe closes and launch fails).
const DEFAULT_EXE = 'C:/my/build/toolchain/GoogleChromePortable64-auto/App/Chrome-bin/chrome.exe';
const exe = opt('--exe', fs.existsSync(DEFAULT_EXE) ? DEFAULT_EXE : undefined);
if (exe) launch.executablePath = exe;                         // portable Chrome, no install
// Without --exe this uses the Chromium that `npx playwright install chromium`
// dropped into %LOCALAPPDATA%\ms-playwright — also install-free.

const ctx = await chromium.launchPersistentContext(profileDir, launch);
const page = ctx.pages()[0] ?? await ctx.newPage();
await page.goto(url, { waitUntil: 'domcontentloaded' });

const cookieCount = async () => (await ctx.cookies()).length;

if (cmd === 'login') {
  console.log(`[login] profile=${profileDir}`);
  console.log('[login] Finish username / password / 2FA / CAPTCHA in the window, then press Enter here.');
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
  await new Promise((r) => rl.question('', r));
  rl.close();
  console.log(`[login] saved. cookies=${await cookieCount()} url=${page.url()}`);
} else if (cmd === 'check') {
  const t = Number(opt('--timeout', 5000));
  await page.waitForTimeout(t);
  console.log(`[check] url=${page.url()} title=${JSON.stringify(await page.title())} cookies=${await cookieCount()}`);
} else if (cmd === 'export') {
  const out = path.resolve('profiles', `${site}.storageState.json`);
  await ctx.storageState({ path: out });
  console.log(`[export] wrote ${out} (contains session cookies; keep it out of git)`);
}
await ctx.close();
