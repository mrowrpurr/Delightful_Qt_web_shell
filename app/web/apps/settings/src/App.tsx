import { useEffect } from 'react'
import { signalReady } from '@app/bridge/lib/transport/bridge'
import { AppearancePanel } from '@app/theming/components/appearance-panel'
import { Toaster } from '@app/ui/components/sonner'

// Thin settings app. Composes the editor-free <AppearancePanel /> from
// @app/theming. Plausibly embeddable inside a real product's settings window.
//
// If you want the editor knobs too (use-app-theme/font toggles, editor
// transparency), import <EditorAppearancePanel /> from the same package
// and render it below — see web/apps/demo/src/tabs/SettingsTab.tsx for the
// composed pair.
export default function App() {
  useEffect(() => {
    signalReady()
  }, [])

  return (
    <div className="min-h-screen text-foreground bg-page">
      <AppearancePanel />
      <Toaster />
    </div>
  )
}
