// Auto-generated from C++ def_type structs — do not edit.
// Regenerate: xmake run generate-dtos

export interface ThemeState {
  displayName: string
  isDark: boolean
}

export interface SetQtThemeRequest {
  displayName: string
  isDark: boolean
}

export interface GetQtThemeResponse {
  displayName: string
  isDark: boolean
}

export interface GetQtThemeFilePathResponse {
  path: string
  embedded: boolean
}
