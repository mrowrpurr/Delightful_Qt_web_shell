# 🎨 Theme Authoring & Consumption Audit

Goal of this doc: explain **how themes are authored**, **what variables each theme provides**, and **which of those we actually consume** (and how). No recommendations yet — just the map.

---

## 1. How themes are authored

Single source of truth: **`web/shared/data/themes.json`** (3.0 MB, ~1030 theme entries).

Each theme is one JSON object with this shape:

```json
{
  "name": "Mrowr Purr - Synthwave '84",
  "source": "custom",
  "light": { "--background": "hsl(268 30% 98%)", ... },
  "dark":  { "--background": "hsl(268 85% 4%)",  ... }
}
```

- **`name`** — the display name used by React, the bridge, and the theme picker UI. Gets slugified (`mrowr-purr-synthwave-84`) for QSS filenames.
- **`source`** — a label shown as a tag in the theme picker. Seen so far: `"default"`, `"custom"`, `"shadcn"` (jln13x import).
- **`light` / `dark`** — two independent CSS-variable palettes. Tokens are written using the **shadcn flat naming** (`--background`, not `--color-background`) and the **Tailwind v4 HSL format** (`hsl(310 100% 64%)` — space-separated, no commas).

### The vocabulary each theme is expected to provide (31 variables)

Grouped by purpose:

| Group | Variables | Count |
|-------|-----------|-------|
| Surface | `--background`, `--foreground`, `--card`, `--card-foreground`, `--popover`, `--popover-foreground` | 6 |
| Action | `--primary`, `--primary-foreground`, `--secondary`, `--secondary-foreground`, `--accent`, `--accent-foreground`, `--destructive`, `--destructive-foreground` | 8 |
| Support | `--muted`, `--muted-foreground`, `--border`, `--input`, `--ring` | 5 |
| Charts | `--chart-1`, `--chart-2`, `--chart-3`, `--chart-4`, `--chart-5` | 5 |
| Sidebar | `--sidebar`, `--sidebar-foreground`, `--sidebar-primary`, `--sidebar-primary-foreground`, `--sidebar-accent`, `--sidebar-accent-foreground`, `--sidebar-border`, `--sidebar-ring` | 8 |
| **Total** | | **32** (31 unique — `background` appears in surface, `sidebar` repeats the pattern) |

### The Default theme is intentionally empty

```json
{ "name": "Default", "source": "default", "light": {}, "dark": {} }
```

Both `applyTheme()` (runtime) and `generate-qss-themes.ts` (build time) detect the empty-palette case and substitute a **hard-coded 19-var fallback** defined inside each consumer. So the Default theme's actual palette lives in code, not data.

### Adding a custom theme

Per the agent docs: add an entry to `themes.json`, re-run `bun run tools/generate-qss-themes.ts`, done. React picks it up from the imported JSON on next build; Qt gets a new `<slug>-dark.qss` / `<slug>-light.qss` pair under `desktop/styles/compiled/`.

---

## 2. How the variables flow to each surface

Three independent consumers read from `themes.json`. Each uses a different subset.

```
themes.json  (31 vars × light+dark × 1030 themes)
     │
     ├── React runtime      — applyTheme() in shared/lib/themes.ts
     │      ↓
     │    <style id="theme-overrides">  ← injected into <head>
     │    :root { --background: ...; --color-background: ...; ... }
     │      ↓
     │    Tailwind v4 utilities (bg-background, text-primary, border-border, ...)
     │    + direct var() consumers in App.css (.markdown-body, .theme-glow)
     │
     ├── QSS build pipeline — tools/generate-qss-themes.ts
     │      ↓
     │    desktop/styles/shared/widgets.qss.template   ← $var tokens
     │      ↓
     │    desktop/styles/compiled/<slug>-<mode>.qss    ← literal colors
     │      ↓
     │    Qt StyleManager → qApp->setStyleSheet()
     │
     └── Monaco runtime     — shared/lib/monaco-theme.ts
            ↓
          buildMonacoThemeFromVars() reads from the applied :root vars
            ↓
          monaco.editor.defineTheme("delightful-custom", ...)
```

---

## 3. What each consumer actually uses

### React runtime — `shared/lib/themes.ts`

`applyTheme(theme, dark)` writes a `<style>` tag that defines exactly **19 variables**:

```
background, foreground,
card, card-foreground,
popover, popover-foreground,
primary, primary-foreground,
secondary, secondary-foreground,
muted, muted-foreground,
accent, accent-foreground,
destructive, destructive-foreground,
border, input, ring
```

Each gets written **twice**: `--background: X;` **and** `--color-background: X;`. The `--color-*` form is what Tailwind v4's `@theme` block resolves; the bare form is for hand-written `var(--background)` lookups.

**What it drops:** every `--chart-*` and every `--sidebar-*`. Whatever's in the theme JSON for those 13 variables is thrown away during `applyTheme()` — they never land on `:root` via this code path.

> ⚠️ Chart vars only reach Monaco via `getComputedStyle(document.documentElement)`. Since `applyTheme()` never sets them, Monaco only sees them when they exist somewhere else — which is **never** at runtime via this pipeline. Monaco falls back to the hard-coded defaults in `buildMonacoThemeFromVars`.

### Tailwind v4 `@theme` blocks — `shared/styles/theme.css` + `apps/main/src/App.css`

Both files define a hard-coded `@theme { --color-*: ...; }` block. This gives Tailwind utility classes (`bg-background`, `text-primary`, etc.) a **compile-time palette** — same dark fallback palette that `themes.ts` has inline. Two files, same values, both maintained by hand.

They also define `--radius: 0.5rem` — which is **not** a theme variable in `themes.json`. Every theme inherits the same border radius.

### QSS build pipeline — `widgets.qss.template` + `generate-qss-themes.ts`

The template consumes **exactly the same 19 vars** as React. No chart or sidebar references.

Selectors covered: `QWidget, QMainWindow, QMenuBar, QMenu, QTabWidget, QTabBar, QToolBar, QToolButton, QStatusBar, QPushButton, QScrollBar, QLineEdit, QTextEdit, QPlainTextEdit, QComboBox, QCheckBox, QRadioButton, QDialog, QMessageBox, QGroupBox, QToolTip, QProgressBar, QSplitter`.

Special token handling:
- `toQssColor()` rewrites `hsl(H S% L%)` → `hsl(H, S%, L%)` because Qt's QSS parser doesn't support the space-separated HSL syntax. Hex passes through untouched.
- Longest-first token replacement so `$card-foreground` is substituted before `$card`.
- SCSS `//` comments are stripped because QSS doesn't support them.

The generator also writes `theme-names.json` — a slug→display-name map used by `StyleManager` for bridge round-trips.

### Monaco editor — `shared/lib/monaco-theme.ts`

Uses **14 variables**:

```
--background, --foreground, --primary, --accent,
--muted, --muted-foreground, --card, --border,
--popover, --ring, --input,
--chart-1, --chart-2, --chart-4, --chart-5
```

Notable omissions: `--chart-3`, `--secondary(-foreground)`, `--destructive(-foreground)`, `--card-foreground`, `--popover-foreground`, `--accent-foreground`, `--primary-foreground`.

Each value goes through `cssColorToHex()` — a canvas-based converter (paint the pixel, read the RGB out of `getImageData`) — because Monaco's theme API expects hex strings, not HSL.

Chart colors map to syntax tokens: `chart-1` → `type`, `chart-2` → `string`, `chart-4` → `number`, `chart-5` → `constant`.

### App.css custom CSS

The `.markdown-body` and `.theme-glow` rules read a narrow subset via `var(--color-*)`:

- `--color-primary`, `--color-foreground`, `--color-muted-foreground`, `--color-border` — theme-aware ✅
- **Hardcoded `#1a1a1a`** — used for `code`, `pre`, `blockquote`, and `th` backgrounds. Escapes the theme system. Will look wrong on light themes.

### Components (Tailwind class sampling)

Across the component audit, these are the theme tokens actually referenced:

| Token | Who uses it | Usage shape |
|-------|-------------|-------------|
| `bg-background`, `text-foreground` | App root, DialogView, all pickers | page/container surface |
| `bg-card`, `text-card-foreground` | Card primitives, pickers, editor status bar | elevated surfaces |
| `bg-muted`, `text-muted-foreground` | Helper text, method-name pills, pre/code | de-emphasized content |
| `text-primary`, `bg-primary/10`, `bg-primary/90` | Section headers, selected list, toggle on-state, buttons | brand accents |
| `bg-accent`, `hover:bg-accent`, `hover:text-accent-foreground` | Hover states (tabs, menu, ghost buttons, list items) | interactive hover |
| `bg-secondary`, `text-secondary-foreground` | Button variant, progress bar | secondary surface |
| `text-destructive`, `hover:text-destructive` | Delete buttons, destructive button variant | danger state |
| `border-border`, `border-input` | Borders, separators, input outlines | structure lines |
| `focus:ring-ring`, `focus-visible:ring-ring` | Focus outlines across inputs/buttons | focus affordance |
| `bg-popover` | Not used via Tailwind class — only via Radix Select's internal class names | dropdown surface |
| `--chart-*`, `--sidebar-*` | **Not used anywhere in React components** | (unused) |

`SettingsTab.tsx` also uses the raw `var(--color-primary)`, `var(--color-input)`, `var(--color-background)` inline for its Toggle — same variables, different syntax than the Tailwind class version nearby. Inconsistent but functionally identical.

---

## 4. Gaps between "provided" and "used"

| Variable(s) | Provided by themes.json | Consumed by | Unused? |
|-------------|------------------------|-------------|---------|
| 19 core (background → ring) | ✅ all 1030 themes (except Default) | React + QSS + Monaco | no |
| `--chart-1,2,4,5` | ✅ | Monaco only (syntax tokens) | partly — chart-3 is dead |
| `--chart-3` | ✅ | **nobody** | ✅ dead var |
| `--sidebar*` (8 vars) | ✅ | **nobody** | ✅ all dead |
| `--radius` | ❌ (hard-coded in `theme.css` / `App.css`) | Tailwind utility classes | N/A |
| Hardcoded `#1a1a1a` | N/A | `App.css` markdown styles | theme-leak |

**Summary:** themes.json ships ~31 variables per mode but the entire stack only consumes ~20. Nine variables per mode (chart-3, sidebar-*) are carried through the build and never read. Across 1030 themes × 2 modes × 9 vars ≈ **~18,500 lines of JSON that nothing reads**.

The inverse gap: `--radius` is a theme-like concept that *every* theme inherits identically because it lives in CSS not in `themes.json`. Custom themes can't override it.

---

## 5. Pipeline-by-pipeline consistency check

| Aspect | React `applyTheme` | QSS generator | Monaco |
|--------|-------------------|---------------|--------|
| Reads from | `themes.json` (bundled) | `themes.json` (build time) | `:root` computed vars at runtime |
| Var form in source | `--background` | `--background` | `--background` |
| Var form emitted | `--background` + `--color-background` | literal color | (hex string in theme object) |
| HSL format | Pass-through (space-separated OK in CSS) | Rewrites to comma-separated | Canvas-converts to hex |
| Fallback palette | `DEFAULT_DARK`/`DEFAULT_LIGHT` in `themes.ts` | `FALLBACK_DARK`/`FALLBACK_LIGHT` in `generate-qss-themes.ts` | Inline literals per var in `monaco-theme.ts` |
| Variables seen | 19 | 19 | 14 |

The two fallback palettes are **copies of each other**, defined in two different files. If you tweak one, the Default theme drifts between React and Qt. Not a bug today because both copies are identical, but they're duplicated source of truth.

---

## 6. Authored summary, in one sentence

> Themes are authored as flat `{--var: hsl(...)}` objects in a single 3 MB JSON file with 1030 entries, each providing a 31-variable palette for light + dark. Three consumers read it — React (19 vars, runtime), QSS generator (19 vars, build time), Monaco (14 vars, runtime) — and each maintains its own fallback for the empty `Default` theme. Sidebar vars and `--chart-3` are authored but unused. `--radius` is used but not authored per-theme.
