import { getBridge } from '../transport/bridge'

// TypeScript interface for the ThemeBridge C++ bridge.
// Coordinates React ↔ Qt theme state (display name, dark/light mode, QSS file path).

export interface ThemeBridge {
  setQtTheme(req: { displayName: string; isDark: boolean }): Promise<{ ok: boolean }>
  getQtTheme(): Promise<{ displayName: string; isDark: boolean }>
  getQtThemeFilePath(): Promise<{ path: string; embedded: boolean }>
  qtThemeChanged(callback: (data?: any) => void): () => void
}

export async function getThemeBridge(): Promise<ThemeBridge> {
  return getBridge<ThemeBridge>('theme')
}
