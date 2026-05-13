import { AppearancePanel } from '@app/theming/components/appearance-panel'
import { EditorAppearancePanel } from '@app/theming/components/editor-appearance-panel'

// Demo composes both panels — it has an editor (see EditorTab), so editor knobs are useful.
// An editor-less consumer would render only <AppearancePanel />.
export default function SettingsTab() {
  return (
    <div className="flex flex-col gap-4">
      <AppearancePanel />
      <EditorAppearancePanel />
    </div>
  )
}
