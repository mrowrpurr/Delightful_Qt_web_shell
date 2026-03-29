# Session Handoff (2026-03-29)

## What Was Built This Session

### QSS Theming System
- **StyleManager** — 3-source theme loading (STYLES_DEV_PATH → AppData/Local → QRC embedded), QFileSystemWatcher live reload, libsass runtime SCSS compilation, dark/light smart switching with mode memory
- **QSS generator** (`tools/generate-qss-themes.ts`) — reads themes.json + widgets.qss.template → 2060 QSS files + theme-names.json slug↔displayName mapping
- **Widget template** (`desktop/styles/shared/widgets.qss.template`) — full Qt widget coverage: menus, tabs, toolbar, buttons (primary/secondary/ghost/destructive via class property), scrollbars, inputs, combos, checkboxes, radios, dialogs, tooltips, progress bars, splitters
- **Searchable theme dropdown** in toolbar — QComboBox with QCompleter, base theme names (no -dark/-light suffixes), applies on selection
- **🌙/☀️ dark/light toggle** in toolbar — calls setColorScheme() for platform chrome

### Qt ↔ React Theme Sync
- **Bidirectional sync** — change theme or dark/light in Qt toolbar → React updates. Change in React settings → Qt updates. Sync guard prevents infinite loops.
- **Bridge methods** — setQtTheme(displayName, isDark), getQtTheme(), qtThemeChanged signal, getQtThemeFilePath()
- **Slug↔displayName mapping** — theme-names.json loaded by StyleManager for translating between Qt slug names and React display names
- **Startup sync** — React sends persisted theme state to Qt on load (main.tsx)

### Live Theme Editor
- **Editor tab** loads current QSS file into Monaco with CSS syntax highlighting
- **Three save paths**: Ctrl+S (page-level capture), :w (vim), toolbar/menu Save (bridge saveRequested signal)
- **Context-aware Save** — toolbar Save button emits saveRequested signal via bridge. Editor tab subscribes when editing, unsubscribes on tab switch. Other tabs get normal Save behavior.
- **QFileSystemWatcher** picks up the save → theme updates instantly around you

### SystemBridge Additions
- `writeTextFile(path, text)` — write files from React
- `setQtTheme(displayName, isDark)` — React → Qt theme control
- `getQtTheme()` — query current Qt theme state
- `getQtThemeFilePath()` — get filesystem path of current theme file
- `qtThemeChanged` signal — Qt → React theme notification
- `saveRequested` signal — Qt Save action → React intercept

### CI/CD
- **ci.yml** — push/PR to main, Windows + macOS, Catch2 + Bun + Playwright browser e2e
- **release.yml** — tag push (v*), tests + windeployqt/macdeployqt + GitHub Release
- **nightly.yml** — daily 6AM UTC, skips if no commits in 24h, rolling pre-release
- GitHub Actions bumped to node24 where available (checkout v6, cache v5, upload-artifact v7, download-artifact v8, setup-bun v2.2)
- Playwright runs directly (`npx playwright test`) not through xmake (os.execv can't resolve .cmd on Windows)

### Tests
- **12 SystemBridge file I/O tests** — readTextFile, readFileBytes, listFolder, globFolder, file handle streaming, error cases, getter methods. All hit real C++ dev-server over WebSocket.

### Other Fixes
- Sticky tab bar (won't scroll out of view)
- Tron (Moving) double grid fix (removed SVG wallpaper, canvas only)
- writeTextFile binary mode (no doubled \r\n on Windows)
- Demo Widget changed from QDialog to QWidget (can preview themes while open)
- libsass integrated via xmake (forked to mrowrpurr/libsass, C++23 compatible)
- README: acknowledgement of ui.jln.dev as theme source (MIT)

### Docs
- **08-theming.md** (new) — full QSS theming architecture, StyleManager API, generator, sync, live editor
- **07-desktop-capabilities.md** — updated with writeTextFile, Qt theme control, saveRequested signal
- **README** — doc table updated with 08, acknowledgements section added

## Git State
- Branch: `qt-delightfulness`
- Clean working tree
- All compiled QSS themes committed (2060 files, ~17MB)

## Key Gotchas Discovered
- QSS doesn't support CSS border-triangle hack — use SVG images for dropdown arrows
- `QIODevice::Text` flag doubles `\r\n` on Windows when content already has `\r\n` — use binary mode for writeTextFile
- `os.execv("npx", ...)` fails on Windows CI — npx is a .cmd shim, not a real executable
- xmake `@default` version means the repo's default branch, not the version name in add_versions
- libsass repo's default branch must be set correctly on GitHub or xmake clones the wrong branch
- Dart Sass chokes on QSS pseudo-selectors (`:!selected`, `::pane`, etc.) — use direct variable substitution instead
- `QComboBox::currentTextChanged` fires on every keystroke — use `activated` for dropdown selections
- Qt theme listener must be in App.tsx (always mounted), not SettingsTab (unmounts on tab switch)
- TabsContent unmounts inactive tabs (`if (value !== context.value) return null`) — useEffect cleanup is reliable

## What's NOT Done
- WASM SystemBridge (no file I/O, clipboard, drag & drop — user uses OPFS extensively, viable future work)
- Pywinauto tests need attention (conftest close_dialogs fixture issues from prior session)
- Human docs 03-tutorial and 04-testing not updated for tabbed layout
- Qt toolbar Save button doesn't fall back to file dialog when not editing a theme (it just does nothing)
- Theme dropdown in Qt toolbar doesn't update when React changes theme (cosmetic — QSS still applies)
