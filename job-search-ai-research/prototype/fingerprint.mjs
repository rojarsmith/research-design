import { chromium } from 'playwright';
import path from 'node:path';
for (const [label, o] of [
  ['headless default', { headless: true }],
  ['headed default', { headless: false }],
  ['headed + AutomationControlled off', { headless: false, args: ['--disable-blink-features=AutomationControlled'] }],
]) {
  const ctx = await chromium.launchPersistentContext(path.resolve('profiles', '_fp_' + label.replace(/\W+/g, '_')), { ...o, args: [...(o.args ?? []), '--window-position=-3200,-3200'] });
  const p = await ctx.newPage(); await p.goto('about:blank');
  const r = await p.evaluate(() => ({ webdriver: navigator.webdriver, ua: navigator.userAgent.match(/(Headless)?Chrome\/[\d.]+/)[0] }));
  console.log(label.padEnd(36), JSON.stringify(r));
  await ctx.close();
}
