export interface ThemeEntry {
  name: string
  source: string
  light: Record<string, string>
  dark: Record<string, string>
}

// Lazy-loaded at runtime. The app that uses themes must call setThemeData()
// with the imported JSON before any theme operations work.
let themesData: ThemeEntry[] = []

export function setThemeData(data: ThemeEntry[]) {
  themesData = data
}

export function loadThemes(): Promise<ThemeEntry[]> {
  return Promise.resolve(themesData)
}

export function getThemesSync(): ThemeEntry[] {
  return themesData
}

// Every theme in themes.json defines these 31 vars for both light and dark.
// applyTheme() writes them to :root as `--foo` AND `--color-foo` so both
// direct `var(--foo)` lookups and Tailwind v4 utilities (`bg-foo` → `var(--color-foo)`)
// resolve to the active theme's values.
const ALL_VARS = [
  // Surface (6)
  'background', 'foreground',
  'card', 'card-foreground',
  'popover', 'popover-foreground',
  // Action (8)
  'primary', 'primary-foreground',
  'secondary', 'secondary-foreground',
  'accent', 'accent-foreground',
  'destructive', 'destructive-foreground',
  // Support (5)
  'muted', 'muted-foreground',
  'border', 'input', 'ring',
  // Chart (5)
  'chart-1', 'chart-2', 'chart-3', 'chart-4', 'chart-5',
  // Sidebar (8)
  'sidebar', 'sidebar-foreground',
  'sidebar-primary', 'sidebar-primary-foreground',
  'sidebar-accent', 'sidebar-accent-foreground',
  'sidebar-border', 'sidebar-ring',
]

export function isDarkMode(): boolean {
  return localStorage.getItem('theme-mode') !== 'light'
}

export function setDarkMode(dark: boolean) {
  localStorage.setItem('theme-mode', dark ? 'dark' : 'light')
}

// We inject a <style> element to override Tailwind's @layer theme vars.
// Inline styles on :root SHOULD override @layer, but QWebEngine's Chromium
// can be inconsistent. A <style> with :root {} is bulletproof.
let themeStyleEl: HTMLStyleElement | null = null

export function applyTheme(theme: ThemeEntry, dark: boolean) {
  const vars = dark ? theme.dark : theme.light

  const lines: string[] = []
  for (const name of ALL_VARS) {
    const value = vars[`--${name}`]
    if (value) {
      lines.push(`--${name}: ${value};`)
      lines.push(`--color-${name}: ${value};`)
    }
  }

  // Inject or update the theme <style> element
  if (!themeStyleEl) {
    themeStyleEl = document.createElement('style')
    themeStyleEl.id = 'theme-overrides'
    document.head.appendChild(themeStyleEl)
  }
  themeStyleEl.textContent = `:root { ${lines.join(' ')} }`

  localStorage.setItem('theme-name', theme.name)
}

export function initTheme() {
  const dark = isDarkMode()
  const savedName = localStorage.getItem('theme-name')
  if (!savedName) return
  loadThemes().then(themes => {
    const theme = themes.find(t => t.name === savedName)
    if (theme) applyTheme(theme, dark)
  })
}

export function extractPreviewColor(theme: ThemeEntry, isDark: boolean): string {
  const vars = isDark ? theme.dark : theme.light
  return vars['--primary'] || '#888'
}

export function extractBgColor(theme: ThemeEntry, isDark: boolean): string {
  const vars = isDark ? theme.dark : theme.light
  return vars['--background'] || (isDark ? '#1e1e1e' : '#fff')
}
