import { getBridge } from '../transport/bridge'

// Re-export generated DTOs so consumers can import from the bridge file
export type {
  SetQtThemeRequest,
  GetQtThemeResponse,
  GetQtThemeFilePathResponse,
  ThemeState,
} from '../dtos/theme.dtos'
export type { OkResponse } from '../dtos/framework.dtos'

import type {
  SetQtThemeRequest,
  GetQtThemeResponse,
  GetQtThemeFilePathResponse,
  ThemeState,
} from '../dtos/theme.dtos'
import type { OkResponse } from '../dtos/framework.dtos'

// TypeScript interface for the ThemeBridge C++ bridge.
// Coordinates React ↔ Qt theme state (display name, dark/light mode, QSS file path).
// DTOs are generated from C++ — run `xmake run generate-dtos` after changes.

export interface ThemeBridge {
  setQtTheme(req: SetQtThemeRequest): Promise<OkResponse>
  getQtTheme(): Promise<GetQtThemeResponse>
  getQtThemeFilePath(): Promise<GetQtThemeFilePathResponse>
  qtThemeChanged(callback: (data: ThemeState) => void): () => void
  qtThemeRequested(callback: (data: SetQtThemeRequest) => void): () => void
}

export async function getThemeBridge(): Promise<ThemeBridge> {
  return getBridge<ThemeBridge>('theme')
}
