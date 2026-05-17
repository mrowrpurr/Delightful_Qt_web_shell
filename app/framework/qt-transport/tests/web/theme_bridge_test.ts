import { test, expect, beforeAll, afterAll } from 'bun:test'
import { createWsConnection, type BridgeConnection } from '@app/bridge/lib/transport/bridge-transport'
import { spawn, type Subprocess } from 'bun'
import fs from 'fs'

// These tests launch the REAL dev-server and verify that ThemeBridge's
// theme state methods and signal round-trip work through the bridge protocol.
//
// The dev-server self-wires qtThemeRequested → updateQtThemeState so the full
// signal loop is testable without a StyleManager.

const PORT = 19879
let server: Subprocess
let conn: BridgeConnection

interface ThemeBridge {
  setQtTheme(req: { displayName: string; isDark: boolean }): Promise<{ ok: boolean }>
  getQtTheme(): Promise<{ displayName: string; isDark: boolean }>
  getQtThemeFilePath(): Promise<{ path: string; embedded: boolean }>
  qtThemeChanged: (cb: (data: any) => void) => () => void
}

beforeAll(async () => {
  const binaryPath = fs.readFileSync('build/.dev-server-binary.txt', 'utf8').trim()
  if (!fs.existsSync(binaryPath))
    throw new Error(`dev-server binary not found at ${binaryPath} — run xmake build dev-server`)

  server = spawn([binaryPath, '--port', String(PORT)], {
    stdout: 'pipe',
    stderr: 'pipe',
  })

  const start = Date.now()
  while (Date.now() - start < 10000) {
    try {
      conn = await createWsConnection(`ws://localhost:${PORT}`)
      return
    } catch {
      await new Promise(r => setTimeout(r, 200))
    }
  }
  throw new Error('dev-server did not start within 10s')
})

afterAll(() => {
  server?.kill()
})

function bridge() {
  return conn.bridge<ThemeBridge>('theme')
}

// ── setQtTheme ──────────────────────────────────────────────────────

test('setQtTheme returns ok', async () => {
  const result = await bridge().setQtTheme({ displayName: 'Zinc', isDark: true })
  expect(result.ok).toBe(true)
})

// ── getQtTheme ──────────────────────────────────────────────────────

test('getQtTheme returns state set via setQtTheme (self-wired round-trip)', async () => {
  await bridge().setQtTheme({ displayName: 'Catppuccin', isDark: false })
  const result = await bridge().getQtTheme()
  expect(result.displayName).toBe('Catppuccin')
  expect(result.isDark).toBe(false)
})

test('getQtTheme reflects latest state after multiple sets', async () => {
  await bridge().setQtTheme({ displayName: 'Rose Pine', isDark: true })
  await bridge().setQtTheme({ displayName: 'Dracula', isDark: false })
  const result = await bridge().getQtTheme()
  expect(result.displayName).toBe('Dracula')
  expect(result.isDark).toBe(false)
})

// ── getQtThemeFilePath ──────────────────────────────────────────────

test('getQtThemeFilePath returns default empty state', async () => {
  const result = await bridge().getQtThemeFilePath()
  expect(result).toHaveProperty('path')
  expect(result).toHaveProperty('embedded')
})

// ── qtThemeChanged signal ───────────────────────────────────────────

test('qtThemeChanged signal fires when setQtTheme is called', async () => {
  const received = new Promise<any>((resolve) => {
    bridge().qtThemeChanged(resolve)
  })

  await bridge().setQtTheme({ displayName: 'Nord', isDark: true })

  const data = await received
  expect(data.displayName).toBe('Nord')
  expect(data.isDark).toBe(true)
})
