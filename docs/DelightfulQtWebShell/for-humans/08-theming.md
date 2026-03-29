# Theming

The app ships 1000+ color themes that apply to both sides — React (CSS variables) and Qt (QSS stylesheets). Change a theme in one place, both update.

## How It Works

Both theme systems are generated from the same source file (`themes.json`):

- **React** imports the JSON at build time and applies themes as CSS variables on `:root`
- **Qt** uses pre-generated QSS files (one per theme × dark/light) applied via `qApp->setStyleSheet()`
- **The bridge** keeps them in sync — change theme in the React settings panel, Qt matches. Change in the Qt toolbar, React matches.

## Using Themes

### From the Qt Toolbar

The toolbar has a searchable theme dropdown and a 🌙/☀️ dark/light toggle. Type to filter, select to apply. The toggle switches between `<theme>-dark` and `<theme>-light` variants.

### From the React Settings Panel

The Settings tab has a full theme picker with preview swatches, a dark mode toggle, and separate controls for the code editor theme.

### From the Live Theme Editor

1. Go to the **Editor** tab
2. Click **🎨 Edit Current Theme**
3. The current QSS file loads into Monaco with CSS syntax highlighting
4. Edit colors, borders, whatever you want
5. **Ctrl+S** to save — the app updates instantly

The `QFileSystemWatcher` detects the file change and re-applies the stylesheet in real time. You're editing the compiled QSS directly, so changes persist until you regenerate.

## Dark/Light Mode

Each theme has a `-dark` and `-light` variant. When you toggle:

1. The app tries the same theme in the other mode (e.g., `dracula-dark` → `dracula-light`)
2. If that variant doesn't exist, it falls back to the last theme you used in that mode
3. Ultimate fallback: `default-dark` or `default-light`

The toggle also calls `QStyleHints::setColorScheme()` to update the platform chrome (title bar, native dialogs on Windows/macOS).

## Theme Sources

The StyleManager checks three locations, in order:

1. **Dev path** — `desktop/styles/` in the repo (set via `STYLES_DEV_PATH` compile-time define, only outside CI)
2. **User override** — `AppData/Local/<app>/styles/` (create this folder to customize)
3. **QRC embedded** — compiled into the binary, always available

Sources 1 and 2 get a `QFileSystemWatcher` for live reload. Source 1 also supports `.scss` files compiled at runtime via libsass.

### Power User Theming

Create `%LOCALAPPDATA%/MyOrganization/Delightful Qt Web Shell/styles/` and drop QSS files in it. The app picks them up automatically and adds them to the theme dropdown. Edit them while the app is running — changes apply instantly.

## Creating Custom Themes

### Option 1: Add to themes.json (Both Sides)

1. Add a theme entry to `web/apps/main/src/data/themes.json` with `light` and `dark` color objects
2. Run `bun run tools/generate-qss-themes.ts` to regenerate QSS files
3. Rebuild — both React and Qt pick up the new theme

### Option 2: QSS Only (Qt Side)

Drop a file named `<slug>-dark.qss` in `desktop/styles/compiled/`. It appears in the Qt theme dropdown immediately (with live reload) or after a rebuild (for QRC).

### Option 3: Edit in the App

Use the live theme editor (Editor tab → 🎨 Edit Current Theme) to modify an existing theme's colors. Save with Ctrl+S, see the result instantly.

## The Widget Template

`desktop/styles/shared/widgets.qss.template` defines how theme colors map to Qt widgets. It uses `$variable` placeholders:

```css
QMenuBar {
  background-color: $background;
  color: $foreground;
  border-bottom: 1px solid $border;
}

QPushButton[class~="destructive"] {
  background-color: $destructive;
  color: $destructive-foreground;
}
```

The generator replaces these with actual color values for each theme. Edit the template to change how themes look across all 1000+ themes at once, then regenerate.

### Widget Classes

Qt widgets can have semantic CSS-like classes:

```cpp
auto* btn = new QPushButton("Delete");
btn->setProperty("class", QStringList{"destructive"});
```

The QSS template targets these with `[class~="destructive"]`. Built-in classes: `destructive`, `secondary`, `ghost`.

## Key Files

| File | Purpose |
|------|---------|
| `themes.json` | Source of truth — 1000+ themes with light/dark CSS variables |
| `widgets.qss.template` | How theme colors map to Qt widgets |
| `desktop/styles/compiled/` | Generated QSS files (committed, regenerate with the generator) |
| `theme-names.json` | Slug ↔ display name mapping for Qt/React sync |
| `tools/generate-qss-themes.ts` | The generator script |
| `desktop/src/style_manager.hpp` | C++ theme loader and live reload engine |
| `shared/lib/themes.ts` | React-side theme application |

## Acknowledgements

Themes sourced from [ui.jln.dev](https://github.com/jln13x/ui.jln.dev) by Julian (MIT License).
