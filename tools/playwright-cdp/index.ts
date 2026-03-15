// Importable API for driving the app via Playwright.
//
// Three connection modes:
//   1. Qt desktop (default): connects to CDP on :9222
//   2. Browser headless: PLAYWRIGHT_URL → launches Chromium per-invocation (agent solo)
//   3. Browser persistent: open() → spawns detached Chromium on :9333, all commands reuse it
//
// For pairing (human watches agent work), use open() then run commands — browser stays open.
// For solo agent work, just set PLAYWRIGHT_URL and each command gets its own headless browser.
//
// NOTE: Must run under Node, not Bun — Bun's ws polyfill breaks CDP.

import { chromium, type Page, type Browser } from "playwright"
import { existsSync, readFileSync, writeFileSync, unlinkSync } from "fs"
import { execSync, spawn } from "child_process"
import { join } from "path"
import { tmpdir } from "os"

const QT_CDP_URL = "http://localhost:9222"
const BROWSER_CDP_PORT = 9333
const BROWSER_CDP_URL = `http://localhost:${BROWSER_CDP_PORT}`
const LAUNCH_URL = process.env.PLAYWRIGHT_URL
const HEADLESS = process.env.PLAYWRIGHT_HEADLESS !== "false"

// Persistent browser state file
const STATE_FILE = join(tmpdir(), "playwright-cdp-browser.json")

let _page: Page | null = null
let _cdpBrowser: Browser | null = null  // CDP connection (reused or Qt)
let _ownedBrowser: Browser | null = null  // per-invocation headless browser (we close this)

// Check if a persistent browser is already running on the CDP port
async function tryConnectPersistent(): Promise<Browser | null> {
  try {
    return await chromium.connectOverCDP(BROWSER_CDP_URL)
  } catch {
    return null
  }
}

// Launch a persistent Chromium as a detached subprocess (survives Node exit)
export async function open(url: string, opts: { headless?: boolean } = {}): Promise<void> {
  // Already running?
  const existing = await tryConnectPersistent()
  if (existing) {
    // Navigate to the URL if different
    const pages = existing.contexts().flatMap(c => c.pages())
    const p = pages.find(pg => !pg.url().startsWith("about:blank"))
    if (p && p.url() !== url) await p.goto(url, { waitUntil: "networkidle" })
    existing.close()
    console.log(`Browser already open on :${BROWSER_CDP_PORT}`)
    return
  }

  const headless = opts.headless ?? false  // open() defaults to headed (pairing mode)
  const execPath = chromium.executablePath()
  const args = [
    `--remote-debugging-port=${BROWSER_CDP_PORT}`,
    "--no-first-run",
    "--no-default-browser-check",
    "--disable-search-engine-choice-screen",
    ...(headless ? ["--headless=new"] : []),
    url,
  ]

  const child = spawn(execPath, args, {
    detached: true,
    stdio: "ignore",
  })
  child.unref()

  // Save state
  writeFileSync(STATE_FILE, JSON.stringify({ url, pid: child.pid }))

  // Wait for CDP to be ready
  for (let i = 0; i < 30; i++) {
    const browser = await tryConnectPersistent()
    if (browser) {
      browser.close()
      console.log(`Browser opened on :${BROWSER_CDP_PORT} (PID ${child.pid})`)
      return
    }
    await new Promise(r => setTimeout(r, 200))
  }
  throw new Error("Browser launched but CDP didn't become ready")
}

// Close the persistent browser
export async function close(): Promise<void> {
  if (existsSync(STATE_FILE)) {
    try {
      const { pid } = JSON.parse(readFileSync(STATE_FILE, "utf-8"))
      process.kill(pid)
    } catch {}
    try { unlinkSync(STATE_FILE) } catch {}
    console.log("Browser closed")
  } else {
    console.log("No persistent browser running")
  }
}

async function page(): Promise<Page> {
  if (_page && !_page.isClosed()) { touch(); return _page }

  let found: Page | undefined

  if (LAUNCH_URL) {
    // First: try connecting to a persistent browser on :9333
    const persistent = await tryConnectPersistent()
    if (persistent) {
      _cdpBrowser = persistent
      const pages = persistent.contexts().flatMap(c => c.pages())
      found = pages.find(p => !p.url().startsWith("about:blank"))
    }

    if (!found) {
      // No persistent browser — launch a headless one for this invocation
      _ownedBrowser = await chromium.launch({ headless: HEADLESS })
      const context = await _ownedBrowser.newContext()
      found = await context.newPage()
      await found.goto(LAUNCH_URL, { waitUntil: "networkidle" })
    }
  } else {
    // Qt desktop mode: connect via CDP on :9222
    _cdpBrowser = await chromium.connectOverCDP(QT_CDP_URL)
    const pages = _cdpBrowser.contexts().flatMap(c => c.pages())
    found = pages.find(p => {
      const url = p.url()
      return url !== "about:blank" && !url.startsWith("devtools://")
    })
  }

  if (!found) throw new Error("No app page found. Is dev-desktop or open running?")

  // Inject console interceptor (idempotent)
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

export async function reload(opts: { waitUntil?: "load" | "networkidle" | "domcontentloaded" } = {}): Promise<void> {
  const p = await page()
  await p.reload({ waitUntil: opts.waitUntil ?? "networkidle" })
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
    if (_ownedBrowser) {
      // Per-invocation headless browser — close it
      await _ownedBrowser.close()
      _ownedBrowser = null
    } else if (_cdpBrowser) {
      // CDP connection (persistent browser or Qt) — just disconnect, don't kill
      _cdpBrowser.close()
      _cdpBrowser = null
    }
    _page = null
  }
}

// Auto-disconnect: fires after the last API call to clean up CDP connections
// that would otherwise keep the event loop alive.
let _autoExit: ReturnType<typeof setTimeout> | null = null

function scheduleAutoExit() {
  if (_autoExit) clearTimeout(_autoExit)
  _autoExit = setTimeout(() => disconnect().then(() => process.exit(0)), 200)
  _autoExit.unref()
}

// In PLAYWRIGHT_URL mode with a persistent browser, we connect via CDP and
// need auto-exit. Only skip for per-invocation browsers (run.ts handles those).
function touch() {
  if (_page && !_ownedBrowser) scheduleAutoExit()
}
