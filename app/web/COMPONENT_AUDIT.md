# 🏴‍☠️ Component Audit — `.tsx` Files

Covers every `.tsx` in `app/web/` except `*.stories.tsx` (Storybook demos, not components).

## Styling conventions across the board

- **Tailwind v4** — all utility classes. No CSS modules, no styled-components.
- **shadcn-style design tokens** — `bg-background`, `text-foreground`, `text-primary`, `border-border`, `bg-card`, `text-muted-foreground`, `hover:bg-accent`, etc. These map to CSS variables driven by the theme system.
- **`cn()` helper** from `@shared/lib/utils` for conditional class merging (standard shadcn pattern, `clsx` + `tailwind-merge`).
- **Inline `style={{...}}`** is used in two places only: dynamic theme colors pulled from CSS vars (`var(--color-primary)`, `oklch(...)`) and Storybook manager (which can't share the Tailwind build).
- **Emoji in UI** — 📖 ✏️ ✅ 📂 ⚙️ 🎨 💾 🗂️ 📋 📦 🚀 🌙 ☀️ 🎉 🔤 🤖 👤 🐕 show up as tab labels, button prefixes, and section headers. Consistent, deliberate, on-brand.
- **`data-testid`** on most interactive elements for Playwright. Naming is kebab-case (`new-list-input`, `create-list-button`, `delete-list-button`).
- **Radix primitives** for complex components (Select). In-house primitives for simpler ones (Tabs).

---

## Entry points

### `main.tsx` — App bootstrap
- Hash-routes between `App` (full app) and `DialogView` (lightweight dialog) based on `window.location.hash === '#/dialog'`.
- Monaco setup: registers the editor worker, shares one monaco instance between `@monaco-editor/react` and `monaco-vim`.
- Pre-renders theme + font from localStorage to prevent flash.
- Fires a one-shot `setQtTheme({displayName, isDark})` so Qt matches React's persisted state on startup. Silently ignored in WASM mode.
- No JSX of its own — just wiring.

### `App.tsx` — Main shell
- Tabs: Docs, Editor, Todos, Files, System, Settings. Controlled via URL hash (`app://main/#settings`) because custom URL schemes can't `history.pushState` cross-origin.
- Sets `document.title` from the active tab — this drives the dock widget label via Qt's `titleChanged` signal.
- Module-level `qtThemeCleanup` + `qtSyncGuard` globals handle the Qt→React theme listener. The guard prevents a React→Qt→React feedback loop.
- Background transparency is dynamic: `oklch(from var(--color-background) l c h / alpha)` when the user has set page transparency > 0.
- Fires `signalReady()` once on mount — **never remove this** or the Qt loading overlay hangs 15s and errors.

### `DialogView.tsx` — Quick-Add dialog
- Same React bundle as `App.tsx`, different view. Loaded when the window URL has `#/dialog`.
- Proves the "one build, multiple routes" pattern — same bridges, signals propagate to the main window instantly.
- Subscribes to all 6 todo signals to stay in sync; pure controlled component otherwise.
- Styling: centered max-w-md flex column, `bg-background text-foreground`, `Select` from shadcn.

---

## Tabs (`apps/main/src/tabs/*`)

### `DocsTab.tsx`
- **What it does:** Markdown reader for all 8 for-humans + 8 for-agents docs, plus the root README.
- **How it's written:** All docs imported statically with Vite's `?raw` loader — `readme from '../../../../../README.md?raw'` (yes, 5 levels of `../` — tolerable but ripe for a path alias).
- **Routing:** `useState` for current doc + agent/human mode. Toggle flips the visible doc list and resets selection if the chosen doc doesn't exist in the new set.
- **Rendering:** `react-markdown` + `remark-gfm` for GFM (tables, task lists). Styled via a global `.markdown-body` class.
- **Styling:** `max-w-3xl` centered, header row with a custom switch and a shadcn `Select`. The toggle is hand-rolled inline (not using the `Toggle` from `SettingsTab.tsx`).

### `EditorTab.tsx` — The most complex tab
- **Embeds Monaco** via `@monaco-editor/react` with `monaco-vim` enabled by default.
- **Dual-mode:**
  1. Sample code (TypeScript) — default.
  2. **Theme editing** — "🎨 Edit Current Theme" loads the active QSS file via `systemBridge.getQtThemeFilePath()` → `readTextFile()`. Sets model language to `css` for QSS highlighting.
- **Save plumbing (four paths into one outcome):**
  1. Monaco `Ctrl+S` action → fires `editor-save` custom event.
  2. Vim `:w` ex command (`VimMode.Vim.defineEx('write', 'w', ...)`) → same event.
  3. Page-level `keydown` capture-phase listener → stops Ctrl+S from bubbling to Qt.
  4. `systemBridge.saveRequested` (Qt toolbar/menu Save) → same event.
- **Theme syncing:** Listens for `editor-theme-changed`, `editor-font-changed`, `qt-theme-synced` window events and rebuilds Monaco's theme via `buildMonacoThemeFromVars()`.
- **Toast:** 2s `setToast()` state-based toast next to the filename — no library.
- **Layout:** `h-[calc(100vh-44px)]` header + editor + vim status bar (bottom).

### `FileBrowserTab.tsx`
- **Demos all three file-access tiers** — the header literally lists them.
- **Three state shapes:** browse entries, file preview (text), image preview (base64 dataURL).
- **Smart dispatch on click:** images < 10 MB → `readFileBytes` (base64 → data URL), text < 100 KB → `readTextFile`, otherwise → `openFileHandle` → `readFileChunk(0, 4096)` → `closeFileHandle` (streamed).
- **Glob search:** input + "🔍 Glob" button calls `globFolder({path, pattern, recursive: true})`.
- **Preview cards** show the method name in a pill so you can see which tier was used (nice agent-friendly touch).
- **Uses `getBridge<SystemBridge>('system')` directly** — not the `getSystemBridge()` helper. See "Flags" below.

### `SettingsTab.tsx` — The biggest file
- **Three in-file components:** `Toggle`, `ThemePicker`, `FontPicker`. Exported default is the page.
- **`ThemePicker`** — searchable dropdown for 1000+ themes. Custom-built (not the shadcn `Select`) because it needs:
  - Color-dot preview per theme (derived from each theme's own light/dark CSS vars via `extractPreviewColor` / `extractBgColor`).
  - Arrow key navigation + highlight scrolling.
  - Click-outside to close (mousedown listener on document).
- **`FontPicker`** — same pattern as `ThemePicker` for 1900+ Google Fonts. First entry is "System default" (null).
- **State (all localStorage-backed):** `dark`, `appTheme`, `appFont`, `editorUseAppTheme`, `editorTheme`, `editorUseAppFont`, `editorFont`, `pageTransparency`, `editorTransparency`.
- **Separate editor settings** — if "Use in Code Editor" toggles off, a nested `ThemePicker` / `FontPicker` appears for editor-only theme/font.
- **Transparency sliders (0–100)** — page transparency emits `page-transparency-changed`; editor transparency emits `editor-theme-changed`.
- **Qt sync** — every theme/dark change calls `systemBridge.setQtTheme({displayName, isDark})`. Listens for `qt-theme-synced` to refresh its own state when Qt drove the change.

### `SystemTab.tsx`
- **Three cards:** Clipboard, Drag & Drop, CLI Args & URL Protocol.
- **Clipboard:** copy-timestamp button + read-clipboard button. 2s feedback state.
- **Drag & Drop:** subscribes to `system.filesDropped`, re-fetches `getDroppedFiles()` on each signal.
- **CLI args:** initial fetch via `getAppLaunchArgs()` + subscription to `appLaunchArgsReceived` for second-instance forwarding.
- **Uses `import.meta.env.VITE_APP_SLUG`** to display the URL protocol in the description (e.g. `your-app://`).
- Clean, demo-style. All `shadcn/ui` `Card` primitives.

### `TodosTab.tsx` — CRUD showcase
- **Lists + detail-view pattern.** Selecting a list calls `getList({list_id})` for items.
- **Fully reactive** via 6 signal subscriptions (`listAdded`, `listRenamed`, `listDeleted`, `itemAdded`, `itemToggled`, `itemDeleted`) — each triggers a refresh.
- **Request objects everywhere**: `addList({name})`, `addItem({list_id, text})`, `toggleItem({item_id})`, `deleteList({list_id})`, `deleteItem({item_id})`. Matches the framework's one-request/one-response rule.
- **Top bar:** Clipboard demo button + "Quick Add" button that calls `system.openDialog()` (which triggers Qt to open the `#/dialog` window via `DialogView`).
- **Item row:** `role="checkbox"` + `aria-checked` for accessibility. `data-done` attribute for test assertions.

---

## Shared UI primitives (`shared/components/ui/`)

### `button.tsx`
- **`cva` (class-variance-authority)** defines 6 variants (`default`, `destructive`, `outline`, `secondary`, `ghost`, `link`) × 4 sizes (`default`, `sm`, `lg`, `icon`).
- `forwardRef`, `VariantProps<typeof buttonVariants>`, `cn()` — textbook shadcn.
- Has `cursor-pointer` baked in (non-standard shadcn — shadcn default is `cursor-default`).

### `card.tsx`
- Five components: `Card`, `CardHeader`, `CardTitle`, `CardDescription`, `CardContent`. All `forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>`.
- No variants. Pure layout scaffolding.
- `rounded-xl border bg-card text-card-foreground shadow`.

### `select.tsx`
- **Radix `@radix-ui/react-select`** wrapped with Tailwind classes.
- Five exports: `Select` (root), `SelectTrigger`, `SelectContent`, `SelectItem`, `SelectValue`.
- `lucide-react` icons for chevron and check.
- `SelectContent` uses `Portal` — which is critical in QtWebEngine because the web engine's native-ish `<select>` bugs out (the "expanding white rectangle" gotcha). This custom Radix-based version avoids that.
- Full animation data-attrs: `data-[state=open]:animate-in data-[state=closed]:animate-out ...`.

### `tabs.tsx`
- **In-house, not Radix.** Context-based — `TabsContext` stores `{value, onValueChange}`.
- Supports both controlled (`value` + `onValueChange`) and uncontrolled (`defaultValue`) modes.
- `TabsContent` returns `null` when inactive — so inactive tabs are **fully unmounted**, not just hidden. That means state in each tab is reset every time it's re-selected (unless the tab lifts state out or uses refs). Worth knowing when debugging "why did my scroll position reset?".

---

## Storybook (`web/.storybook/manager.tsx`)

- **Custom Storybook panel** titled "🎨 Theme" with two sub-tabs: Themes (1000+) and Fonts (1900+).
- **Cannot import `@shared`** because the manager runs in a separate iframe from the preview. Communicates via Storybook's `addons.getChannel()` with events `theme-addon:request-data`, `theme-addon:data`, `theme-addon:set-theme`, `theme-addon:set-font`.
- **Inline styles only** — no Tailwind in the manager iframe. Uses Storybook CSS variables (`var(--appBorderColor)`, `var(--textColor)`).
- Reusable `SearchableList<T>` generic component for both panels.
- `addons.register` + `addons.add({type: types.PANEL})` — standard Storybook 7+ addon API.

---

## 🚨 Flags worth your attention

### 1. Delete-list button is permanently invisible (real bug)
In `TodosTab.tsx:131-134` the `delete-list-button` uses `opacity-0 group-hover:opacity-100` for hover reveal — but the **parent row has no `group` class** (line 121-125). So:
- `opacity-0` is always active.
- `group-hover:opacity-100` never fires because there's no `group` ancestor.

The button exists and is clickable (Playwright tests pass because `getByTestId().click()` hits transparent elements fine), but a **human can't see it**. Either add `group` to the parent row, or drop the opacity classes and use a different reveal pattern.

### 2. Two ways to reach `SystemBridge` — pick one
- `TodosTab`, `SystemTab`, `FileBrowserTab` use `getBridge<SystemBridge>('system')` at module scope.
- `App`, `EditorTab`, `SettingsTab` use the helper `getSystemBridge()` from `@shared/api/system-bridge`.

Both work, but it's inconsistent. If `getSystemBridge()` is the blessed helper (it looks purpose-built), the others should migrate. Not urgent, just drift.

### 3. Duplicated `Toggle` component
There's a `Toggle` defined inline in `SettingsTab.tsx` (lines 14–35) and a near-identical hand-rolled switch in `DocsTab.tsx` (lines 74–88). ~20 lines of duplication. A shared `Toggle` in `shared/components/ui/` would collapse both.

### 4. Module-scope `await getBridge(...)` has no fallback
Files like `TodosTab.tsx` and `SystemTab.tsx` do `const todos = await getBridge<TodoBridge>('todos')` at the very top. If the bridge isn't reachable (e.g. future WASM flow where `todos` isn't registered), the **module fails to import** and the whole app crashes on tab mount. The `getSystemBridge().then(...).catch(() => {})` pattern in `EditorTab`/`SettingsTab` is more resilient. Worth standardising if WASM-only deployments are on the roadmap.

### 5. `DocsTab` relative paths (cosmetic)
`import readme from '../../../../../README.md?raw'` — five `../` segments. A `@docs` or `@root` Vite alias would be cleaner, but this is purely aesthetic.

### 6. Inactive `TabsContent` unmounts completely
The in-house `tabs.tsx` returns `null` for inactive tabs. Each tab remounts on re-select → local `useState` resets. This is fine for the current tabs (they all reload from bridges), but if someone adds a tab with expensive setup or non-serialisable state, it'll surprise them.
