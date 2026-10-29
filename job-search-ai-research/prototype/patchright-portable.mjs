import { chromium } from 'patchright';
import path from 'node:path';
import fs from 'node:fs';
const EXE = process.argv[2] ?? 'C:/my/build/toolchain/GoogleChromePortable64-auto/App/Chrome-bin/chrome.exe';
const dir = path.resolve('profiles', '_patchright_test');
fs.mkdirSync(dir, { recursive: true });
const run = async (label) => {
  const ctx = await chromium.launchPersistentContext(dir, { executablePath: EXE, headless: false, viewport: null, args: ['--window-position=-3200,-3200'] });
  const p = ctx.pages()[0] ?? await ctx.newPage();
  await p.goto('https://example.com');
  if (label === 'first') { await ctx.addCookies([{ name: 'sess', value: 'ok', domain: 'example.com', path: '/' , expires: Math.floor(Date.now()/1000)+86400}]); }
  const r = await p.evaluate(() => ({ webdriver: navigator.webdriver, ua: navigator.userAgent.match(/(Headless)?Chrome\/[\d.]+/)[0], cdcKeys: Object.keys(window).filter(k => /^cdc_|__playwright|__pw_/.test(k)).length }));
  const cookie = (await ctx.cookies()).find(c => c.name === 'sess')?.value;
  console.log(label, JSON.stringify({ ...r, cookieSurvived: cookie === 'ok' }));
  await ctx.close();
};
await run('first'); await run('second');
