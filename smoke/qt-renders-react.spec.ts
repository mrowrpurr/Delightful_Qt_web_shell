import { test, expect, chromium, type Browser, type Page } from '@playwright/test'
import { spawn, type ChildProcess } from 'child_process'
import fs from 'fs'

// Desktop smoke tests: launches the REAL Qt desktop app, connects to its
// WebEngine via Playwright's connectOverCDP, and tests with the same
// page API used by the browser e2e tests.
//
// Requires: desktop target built (`xmake build desktop`)
// Requires: playwright-core patch (patches/playwright-core@*.patch)

// QtWebEngine reports targets as type "other" instead of "page".
// This tells Playwright to treat them as pages anyway.
process.env.PW_CHROMIUM_ATTACH_TO_OTHER = '1'

let qtProcess: ChildProcess | null = null
let browser: Browser | null = null

function getExePath(): string {
  return fs.readFileSync('build/.desktop-binary.txt', 'utf8').trim()
}

async function launchAndConnect(): Promise<Page> {
  const exePath = getExePath()

  qtProcess = spawn(exePath, [], {
    env: {
      ...process.env,
      QTWEBENGINE_REMOTE_DEBUGGING: '9222',
    },
    stdio: 'ignore',
  })

  // Wait for Qt to start and CDP to be available
  await new Promise<void>((resolve, reject) => {
    qtProcess!.on('error', (err) => reject(new Error(`Failed to launch Qt app: ${err.message}`)))

    const start = Date.now()
    const poll = async () => {
      try {
        const res = await fetch('http://localhost:9222/json')
        const pages = await res.json() as Array<{ webSocketDebuggerUrl: string }>
        if (pages.length > 0) return resolve()
      } catch {}
      if (Date.now() - start > 15_000) return reject(new Error('Qt app CDP timeout'))
      setTimeout(poll, 250)
    }
    poll()
  })

  browser = await chromium.connectOverCDP('http://localhost:9222')

  // Find the actual app page (not devtools:// or blank pages)
  const findAppPage = (): Page | undefined => {
    const allPages = browser!.contexts().flatMap(c => c.pages())
    return allPages.find(p => {
      const url = p.url()
      return url !== 'about:blank' && !url.startsWith('devtools://')
    })
  }

  // The app page may not be available immediately — poll for it
  const start = Date.now()
  while (Date.now() - start < 10_000) {
    const page = findAppPage()
    if (page) return page
    await new Promise(r => setTimeout(r, 500))
  }

  // Debug: log all pages we can see
  const allPages = browser.contexts().flatMap(c => c.pages())
  const urls = allPages.map(p => p.url())
  throw new Error(`No app page found. Available pages: ${JSON.stringify(urls)}`)
}

test.afterEach(async () => {
  if (browser) {
    await browser.close().catch(() => {})
    browser = null
  }
  if (qtProcess) {
    qtProcess.kill()
    qtProcess = null
  }
})

test('Qt app renders the React heading', async () => {
  const page = await launchAndConnect()
  await expect(page.getByTestId('heading')).toBeVisible({ timeout: 10_000 })
})

test('Qt app bridge responds to method calls', async () => {
  const page = await launchAndConnect()

  await page.getByTestId('new-list-input').fill('Smoke Test List')
  await page.getByTestId('create-list-button').click()

  await expect(page.getByTestId('todo-list').filter({ hasText: 'Smoke Test List' })).toBeVisible()
})
