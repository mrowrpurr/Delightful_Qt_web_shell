import { useState, useCallback } from 'react'
import { applyFont } from '../lib/fonts'
import { isDarkMode } from '../lib/themes'
import { Switch } from '@app/ui/components/switch'
import { Label } from '@app/ui/components/label'
import { Card, CardHeader, CardTitle, CardDescription, CardContent } from '@app/ui/components/card'
import { ThemePicker } from './theme-picker'
import { FontPicker } from './font-picker'
import { TransparencySlider } from './transparency-slider'

// EditorAppearancePanel — editor-specific theme/font/transparency.
// Opt-in for apps that embed an editor (e.g., demo). The companion
// <AppearancePanel> stays editor-free for portability.
//
// Behavior contract: when "Use in Code Editor" is ON, the editor consumer
// (e.g., EditorTab) reads `theme-name` / `app-font-family` directly. When OFF,
// the editor reads `editor-theme-name` / `editor-font-family`. This panel
// only writes the editor-specific keys; the consumer picks which to honor.
export function EditorAppearancePanel() {
  const [dark] = useState(isDarkMode)
  const [editorUseAppTheme, setEditorUseAppTheme] = useState(
    localStorage.getItem('editor-use-app-theme') !== 'false'
  )
  const [editorTheme, setEditorTheme] = useState(
    localStorage.getItem('editor-theme-name') || 'Default'
  )
  const [editorUseAppFont, setEditorUseAppFont] = useState(
    localStorage.getItem('editor-use-app-font') !== 'false'
  )
  const [editorFont, setEditorFont] = useState<string | null>(
    localStorage.getItem('editor-font-family')
  )
  const [editorTransparency, setEditorTransparency] = useState(
    parseInt(localStorage.getItem('editor-transparency') ?? '0', 10)
  )

  const onEditorUseAppTheme = useCallback((v: boolean) => {
    setEditorUseAppTheme(v)
    localStorage.setItem('editor-use-app-theme', String(v))
    window.dispatchEvent(new CustomEvent('editor-theme-changed'))
  }, [])

  const onEditorTheme = useCallback((name: string) => {
    setEditorTheme(name)
    localStorage.setItem('editor-theme-name', name)
    window.dispatchEvent(new CustomEvent('editor-theme-changed'))
  }, [])

  const onEditorUseAppFont = useCallback((v: boolean) => {
    setEditorUseAppFont(v)
    localStorage.setItem('editor-use-app-font', String(v))
    window.dispatchEvent(new CustomEvent('editor-font-changed'))
  }, [])

  const onEditorFont = useCallback((family: string | null) => {
    setEditorFont(family)
    applyFont(family, 'editor')
    window.dispatchEvent(new CustomEvent('editor-font-changed'))
  }, [])

  const onEditorTransparency = useCallback((value: number) => {
    setEditorTransparency(value)
    localStorage.setItem('editor-transparency', String(value))
    window.dispatchEvent(new CustomEvent('editor-theme-changed'))
  }, [])

  return (
    <div className="max-w-lg mx-auto p-6">
      <Card>
        <CardHeader>
          <CardTitle>Code Editor</CardTitle>
          <CardDescription>Per-editor theme, font, and transparency overrides.</CardDescription>
        </CardHeader>
        <CardContent className="space-y-6">
          <div>
            <Label htmlFor="editor-use-app-theme" className="font-normal text-muted-foreground">
              <Switch
                id="editor-use-app-theme"
                checked={editorUseAppTheme}
                onCheckedChange={onEditorUseAppTheme}
                size="sm"
              />
              Use the app theme in the code editor
            </Label>
            {!editorUseAppTheme && (
              <div className="mt-3 ml-1 pl-3 border-l-2 border-border">
                <p className="text-sm font-medium mb-1">Editor Theme</p>
                <ThemePicker value={editorTheme} isDark={dark} onChange={onEditorTheme} />
              </div>
            )}
          </div>

          <div>
            <Label htmlFor="editor-use-app-font" className="font-normal text-muted-foreground">
              <Switch
                id="editor-use-app-font"
                checked={editorUseAppFont}
                onCheckedChange={onEditorUseAppFont}
                size="sm"
              />
              Use the app font in the code editor
            </Label>
            {!editorUseAppFont && (
              <div className="mt-3 ml-1 pl-3 border-l-2 border-border">
                <p className="text-sm font-medium mb-1">Editor Font</p>
                <FontPicker value={editorFont} onChange={onEditorFont} />
              </div>
            )}
          </div>

          <TransparencySlider
            label="Editor Transparency"
            description="Make the editor background see-through"
            value={editorTransparency}
            onChange={onEditorTransparency}
          />
        </CardContent>
      </Card>
    </div>
  )
}
