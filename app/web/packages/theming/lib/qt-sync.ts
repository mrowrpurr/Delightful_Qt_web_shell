// Qt theme sync — keeps React in lockstep with Qt-side theme changes.
//
// Qt owns the chrome (toolbar dropdown, dark/light toggle in QSS). When the
// user changes the theme there, Qt emits `qtThemeChanged` via SystemBridge.
// This module subscribes to that signal, pulls the new theme, applies it on
// the React side, persists it to localStorage, and dispatches
// `editor-theme-changed` (so editor consumers can refresh) and `qt-theme-synced`
// (so panels mirroring the state can re-read).
//
// The `guard` flag prevents the obvious infinite loop:
//   React → setQtTheme → Qt emits qtThemeChanged → React re-applies → ...
// React's own theme change paths set the guard during their own setQtTheme()
// call (or the listener sets it before re-applying), so the listener bails
// when it sees its own echo.

import { applyTheme, loadTheme, setDarkMode as persistDarkMode } from './themes'
import { applyThemeEffects } from './theme-effects'
import { getSystemBridge } from '@app/bridge/lib/bridges/system-bridge'

let cleanup: (() => void) | null = null
let guard = false

export async function setupQtThemeListener(): Promise<() => void> {
  if (cleanup) return cleanup

  try {
    const system = await getSystemBridge()
    cleanup = system.qtThemeChanged(async () => {
      if (guard) return
      guard = true
      try {
        const state = await system.getQtTheme()
        const theme = await loadTheme(state.displayName)

        persistDarkMode(state.isDark)

        if (theme) {
          applyTheme(theme, state.isDark)
          applyThemeEffects(theme.name)
          localStorage.setItem('theme-name', theme.name)
          window.dispatchEvent(new CustomEvent('editor-theme-changed'))
        }

        window.dispatchEvent(new CustomEvent('qt-theme-synced'))
      } finally {
        guard = false
      }
    })
    return cleanup
  } catch {
    // WASM/browser mode — no bridge. Return a no-op so callers can always invoke it.
    cleanup = () => {}
    return cleanup
  }
}
