import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App'
import DialogView from './DialogView'
import './App.css'

// Page-load timing — flows through LoggingWebPage to qDebug on desktop.
const t0 = performance.now()
const lap = (label: string) => console.log(`[load-time] web: ${label} at ${(performance.now() - t0).toFixed(1)}ms`)
lap('main.tsx start')

// Monaco setup — share a single instance between @monaco-editor/react and monaco-vim.
// The web worker must be configured before any editor mounts.
import * as monaco from 'monaco-editor'
import { loader } from '@monaco-editor/react'
import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker'
self.MonacoEnvironment = { getWorker: () => new editorWorker() }
loader.config({ monaco })

// Load theme + font data (imported at build time — fetch doesn't work with app:// scheme)
import { setThemeData, initTheme } from '@shared/lib/themes'
import { setFontData, initFont } from '@shared/lib/fonts'
import themesJson from '@shared/data/themes.json'
import fontsJson from '@shared/data/google-fonts.json'
lap('json modules imported')
setThemeData(themesJson as any)
setFontData(fontsJson as any)
lap('setThemeData + setFontData done')

// Apply saved theme + font before first render to prevent flash
import { applyThemeEffects } from './theme-effects'
import { isDarkMode } from '@shared/lib/themes'
initTheme()
initFont()
applyThemeEffects(localStorage.getItem('theme-name') || 'Default')
lap('theme + font init done')

// Sync React's persisted theme state to Qt on startup.
// React owns the truth (localStorage persists across sessions, Qt doesn't).
import { getSystemBridge } from '@shared/api/system-bridge'
getSystemBridge().then(system => {
  const themeName = localStorage.getItem('theme-name') || 'Default'
  const dark = isDarkMode()
  system.setQtTheme({ displayName: themeName, isDark: dark })
}).catch(() => {}) // WASM/browser mode — no bridge

// Hash-based routing — same React app, different content.
// The main window loads app://main/ (no hash) → full app.
// A dialog loads app://main/#/dialog → lightweight dialog UI.
// No React Router needed — the hash is set once at load time.
const route = window.location.hash

const Root = route === '#/dialog' ? DialogView : App

lap('about to render')
createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Root />
  </StrictMode>,
)
lap('render call returned')
