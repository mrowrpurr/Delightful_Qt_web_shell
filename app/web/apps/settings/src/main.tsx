import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { HashRouter, Routes, Route } from 'react-router'
import App from './App'
import './App.css'

// Theme: same fast-path / cold-path dance as demo. Settings renders themed
// components (AppearancePanel sits inside a Card on a themed surface), so
// theme variables must be set on :root before React mounts.
import { tryFastTheme, isDarkMode, loadTheme, applyTheme, slugifyThemeName } from '@app/theming/lib/themes'
import { setFontData, initFont } from '@app/theming/lib/fonts'
import fontsJson from '@app/theming/data/google-fonts.json'

setFontData(fontsJson as any)

function applyTransparency(key: string, cssVar: string) {
  const pct = parseInt(localStorage.getItem(key) ?? '0', 10) || 0
  document.documentElement.style.setProperty(cssVar, String((100 - pct) / 100))
}
applyTransparency('page-transparency', '--page-opacity')
applyTransparency('surface-transparency', '--surface-opacity')

const usedFastPath = tryFastTheme()

import { applyThemeEffects } from '@app/theming/lib/theme-effects'
const savedThemeName = localStorage.getItem('theme-name') || 'Default'
initFont()
applyThemeEffects(savedThemeName)

// Cold path: no cached CSS yet — BLOCK render until the saved theme's module
// is loaded and applied. Top-level await is supported (target: esnext).
if (!usedFastPath) {
  const theme = await loadTheme(savedThemeName)
  if (theme) {
    applyTheme(theme, isDarkMode())
  } else {
    console.warn('[theme] no module for', savedThemeName, '(slug:', slugifyThemeName(savedThemeName), ')')
  }
}

// Tell Qt our persisted theme (settings is one place a user can change it),
// then subscribe so any other window changing theme keeps us in sync.
import { getSystemBridge } from '@app/bridge/lib/bridges/system-bridge'
import { setupQtThemeListener } from '@app/theming/lib/qt-sync'
getSystemBridge().then(system => {
  system.setQtTheme({ displayName: savedThemeName, isDark: isDarkMode() })
}).catch(() => {}) // WASM/browser mode — no bridge
setupQtThemeListener()

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <HashRouter>
      <Routes>
        <Route path="*" element={<App />} />
      </Routes>
    </HashRouter>
  </StrictMode>,
)
