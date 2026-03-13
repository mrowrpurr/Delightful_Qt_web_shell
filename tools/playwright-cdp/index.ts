// Importable API for driving the Qt app via Chrome DevTools Protocol.
//
// npx tsx -e "import { snapshot } from './tools/playwright-cdp'; console.log(await snapshot())"
//
// Requires: app running with CDP on :9222 (xmake run start-desktop)
// NOTE: Must run under Node, not Bun — Bun's ws polyfill mishandles the HTTP 101 upgrade.

import { chromium, type Page } from "playwright"

const CDP_URL = "http://localhost:9222"

let _page: Page | null = null

async function page(): Promise<Page> {
  if (_page && !_page.isClosed()) { touch(); return _page }
  const browser = await chromium.connectOverCDP(CDP_URL)
  const pages = browser.contexts().flatMap(c => c.pages())
  const found = pages.find(p => {
    const url = p.url()
    return url !== "about:blank" && !url.startsWith("devtools://")
  })
  if (!found) throw new Error("No app page found. Is dev-desktop running?")

  // Inject console interceptor (idempotent) — buffers messages in the page's
  // JS context so they persist across stateless CLI invocations.
  await found.evaluate(`
    if (!window.__cdp_console) {
      window.__cdp_console = [];
      for (const level of ['log', 'warn', 'error', 'info', 'debug']) {
        const orig = console[level];
        console[level] = (...args) => {
          window.__cdp_console.push({
            timestamp: new Date().toISOString(), level,
            text: args.map(String).join(' ')
          });
          if (window.__cdp_console.length > 200) window.__cdp_console.shift();
          orig.apply(console, args);
        };
      }
    }
  `)

  _page = found
  touch()
  return found
}

export async function snapshot(): Promise<string> {
  const p = await page()
  const tree = await p.locator("body").ariaSnapshot()
  const title = await p.title()
  const url = p.url()
  return `Page: ${title} (${url})\n\n${tree}`
}

export async function screenshot(path = "screenshot.png"): Promise<string> {
  const p = await page()
  await p.screenshot({ path })
  return path
}

export async function click(testId?: string, { selector }: { selector?: string } = {}): Promise<void> {
  const p = await page()
  if (testId) { await p.getByTestId(testId).click(); return }
  if (selector) { await p.click(selector); return }
  throw new Error("Provide testId or { selector }")
}

export async function fill(testId: string, value: string): Promise<void>
export async function fill(testId: undefined, value: string, opts: { selector: string }): Promise<void>
export async function fill(testId: string | undefined, value: string, opts?: { selector?: string }): Promise<void> {
  const p = await page()
  if (testId) { await p.getByTestId(testId).fill(value); return }
  if (opts?.selector) { await p.fill(opts.selector, value); return }
  throw new Error("Provide testId or { selector }")
}

export async function press(key: string, testId?: string, { selector }: { selector?: string } = {}): Promise<void> {
  const p = await page()
  if (testId) { await p.getByTestId(testId).press(key); return }
  if (selector) { await p.press(selector, key); return }
  await p.keyboard.press(key)
}

export async function eval_js(expression: string): Promise<any> {
  const p = await page()
  return p.evaluate(expression)
}

export async function text(testId?: string, { selector }: { selector?: string } = {}): Promise<string> {
  const p = await page()
  if (testId) return (await p.getByTestId(testId).textContent()) || "(empty)"
  if (selector) return (await p.textContent(selector)) || "(empty)"
  throw new Error("Provide testId or { selector }")
}

export async function wait(testId?: string, timeout = 5000, { selector }: { selector?: string } = {}): Promise<void> {
  const p = await page()
  if (testId) { await p.getByTestId(testId).waitFor({ timeout }); return }
  if (selector) { await p.waitForSelector(selector, { timeout }); return }
  throw new Error("Provide testId or { selector }")
}

export async function console_messages(opts: { level?: string; count?: number; clear?: boolean } = {}): Promise<{ timestamp: string; level: string; text: string }[]> {
  const p = await page()
  const { level, count, clear } = opts
  let msgs: { timestamp: string; level: string; text: string }[] = clear
    ? await p.evaluate("window.__cdp_console ? window.__cdp_console.splice(0) : []")
    : await p.evaluate("window.__cdp_console || []")
  if (level) msgs = msgs.filter(m => m.level === level)
  if (count) msgs = msgs.slice(-count)
  return msgs
}

export async function disconnect(): Promise<void> {
  if (_page) {
    const browser = _page.context().browser()
    if (browser) await browser.close()
    _page = null
  }
}

// Auto-disconnect: when all user code finishes but the CDP WebSocket keeps
// the event loop alive, this timer fires and cleans up. The unref() ensures
// it doesn't ITSELF keep the process alive if cleanup already happened.
let _autoExit: ReturnType<typeof setTimeout> | null = null

function scheduleAutoExit() {
  if (_autoExit) clearTimeout(_autoExit)
  // Short delay so chained awaits don't trigger premature disconnect
  _autoExit = setTimeout(() => disconnect().then(() => process.exit(0)), 200)
  _autoExit.unref()
}

// Re-schedule after every API call so multi-step scripts work
function touch() { if (_page) scheduleAutoExit() }
