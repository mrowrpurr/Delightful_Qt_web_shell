import { useState, useEffect, useCallback } from 'react'
import { applyTheme, loadTheme, isDarkMode, setDarkMode } from '../lib/themes'
import { applyFont } from '../lib/fonts'
import { applyThemeEffects } from '../lib/theme-effects'
import { getThemeBridge } from '@app/bridge/lib/bridges/theme-bridge'
import { Card, CardHeader, CardTitle, CardDescription, CardContent } from '@app/ui/components/card'
import { ThemePicker } from './theme-picker'
import { FontPicker } from './font-picker'
import { DarkModeToggle } from './dark-mode-toggle'
import { TransparencySlider } from './transparency-slider'

let themeBridge: Awaited<ReturnType<typeof getThemeBridge>> | null = null
getThemeBridge().then(b => { themeBridge = b }).catch(() => {})

// Notify editor consumers — if an editor exists in this app, it listens for these
// and re-applies its theme/font. Apps without an editor simply ignore them.
function notifyEditorTheme() {
  window.dispatchEvent(new CustomEvent('editor-theme-changed'))
}

// AppearancePanel — app-wide theme, font, dark mode, and surface transparency.
// Portable to editor-less consumers (e.g., the standalone settings app).
// Editor-specific knobs live in <EditorAppearancePanel> next to this one.
export function AppearancePanel() {
  const [dark, setDark] = useState(isDarkMode)
  const [appTheme, setAppTheme] = useState(localStorage.getItem('theme-name') || 'Default')
  const [appFont, setAppFont] = useState<string | null>(localStorage.getItem('app-font-family'))
  const [pageTransparency, setPageTransparency] = useState(
    parseInt(localStorage.getItem('page-transparency') ?? '0', 10)
  )
  const [surfaceTransparency, setSurfaceTransparency] = useState(
    parseInt(localStorage.getItem('surface-transparency') ?? '0', 10)
  )

  const onDarkToggle = useCallback(async (newDark: boolean) => {
    setDark(newDark)
    setDarkMode(newDark)
    const theme = await loadTheme(appTheme)
    if (theme) applyTheme(theme, newDark)
    applyThemeEffects(appTheme)
    notifyEditorTheme()
    if (themeBridge) {
      themeBridge.setQtTheme({ displayName: appTheme, isDark: newDark }).catch(() => {})
    }
  }, [appTheme])

  const onAppTheme = useCallback(async (name: string) => {
    setAppTheme(name)
    const theme = await loadTheme(name)
    if (theme) applyTheme(theme, dark)
    applyThemeEffects(name)
    notifyEditorTheme()
    if (themeBridge) {
      themeBridge.setQtTheme({ displayName: name, isDark: dark }).catch(() => {})
    }
  }, [dark])

  useEffect(() => {
    const handler = () => {
      setDark(isDarkMode())
      setAppTheme(localStorage.getItem('theme-name') || 'Default')
    }
    window.addEventListener('qt-theme-synced', handler)
    return () => window.removeEventListener('qt-theme-synced', handler)
  }, [])

  const onAppFont = useCallback((family: string | null) => {
    setAppFont(family)
    applyFont(family, 'app')
  }, [])

  const onPageTransparency = useCallback((value: number) => {
    setPageTransparency(value)
    localStorage.setItem('page-transparency', String(value))
    document.documentElement.style.setProperty('--page-opacity', String((100 - value) / 100))
  }, [])

  const onSurfaceTransparency = useCallback((value: number) => {
    setSurfaceTransparency(value)
    localStorage.setItem('surface-transparency', String(value))
    document.documentElement.style.setProperty('--surface-opacity', String((100 - value) / 100))
  }, [])

  return (
    <div className="max-w-lg mx-auto p-6">
      <Card>
        <CardHeader>
          <CardTitle>Appearance</CardTitle>
          <CardDescription>Theme, font, and transparency. Saved to localStorage.</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          <DarkModeToggle checked={dark} onChange={onDarkToggle} />

          <div>
            <p className="text-sm font-medium mb-1">Theme</p>
            <p className="text-sm text-muted-foreground mb-3">Choose from 1000+ color themes</p>
            <ThemePicker value={appTheme} isDark={dark} onChange={onAppTheme} />
          </div>

          <div>
            <p className="text-sm font-medium mb-1">Font</p>
            <p className="text-sm text-muted-foreground mb-3">Choose from 1900+ Google Fonts</p>
            <FontPicker value={appFont} onChange={onAppFont} />
            {appFont && (
              <p className="mt-2 text-sm text-muted-foreground" style={{ fontFamily: `"${appFont}", sans-serif` }}>
                The quick brown fox jumps over the lazy dog
              </p>
            )}
          </div>

          <TransparencySlider
            label="Page Transparency"
            description="Fade the page background so wallpaper themes show through"
            value={pageTransparency}
            onChange={onPageTransparency}
          />

          <TransparencySlider
            label="Surface Transparency"
            description="Fade cards and the sidebar so the page underneath shows through"
            value={surfaceTransparency}
            onChange={onSurfaceTransparency}
          />
        </CardContent>
      </Card>
    </div>
  )
}
