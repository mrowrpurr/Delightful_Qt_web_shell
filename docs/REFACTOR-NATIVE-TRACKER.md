# Native Refactor — Progress Tracker 🏴‍☠️

Companion to `REFACTOR-NATIVE.md` (the **what** + **why**) and `REFACTOR-NATIVE-PHASES.md` (the **how**, 23 atomic commits across 7 phases). This file is the live checklist.

Tick a sub-box when its commit lands green (`xmake build desktop` + `xmake build wasm-app`) and the demo runs. Tick the phase's outer box only when **every** sub-box under it is green.

---

## Phase 1 — Foundation

- [x] **Phase 1 complete**
  - [x] **1. Introduce `app_shell::App` as a real class** — `Application` becomes a thin transitional subclass; commit `173a9ec`
  - [x] **2. Kill `qobject_cast<Application*>(qApp)`** — 11 fishing sites across 5 files migrated to typed `app_shell::App&` references; commit `bc2a18e`
  - [x] **3. `app.iconPath()` accessor** — adds `iconPath()` and `brandingImagePath()`; all hardcoded `:/icon.ico` and `:/icon.png` literals collapsed into the two App accessors; commit `6157c07`

---

## Phase 2 — App-scoped subsystems opt-in

- [x] **Phase 2 complete**
  - [x] **4. Extract `Tray`** — `QObject` subclass parented to `App`, owns `QSystemTrayIcon` + `QMenu`; demo's tray content (Alpha/Beta/Gamma submenus) moved into `app/desktop/src/main.cpp`; commit `c0335a1`
  - [x] **5. Extract `UrlProtocol`** — static methods on `App` became instance methods on the subsystem; `menu_bar.cpp` retrieves via `app.findChild<UrlProtocol*>()`; commit `a9db15f`
  - [x] **6. Extract `SingleInstance`** — accepted the transitional secondary-process ordering regression per PHASES.md note (shrinks as Phases 3–4 lighten App's ctor); commit `95f290e`
  - [x] **7. Extract `WindowLifecycle`** — `DockManager::restoreWindows()` and the `topLevelWidgets()`-iteration in `MainWindow::closeEvent` moved here; class originally named `WindowRegistry` but renamed because it holds no state and registers nothing (commit `55d7310`); original extraction commit `37d9d0e`

### Phase 2 cleanups landed on top

- `9dc1808` — 🪦 Deleted the now-vestigial `Application` shim (`application.hpp`/`application.cpp`)
- `55d7310` — 🪪 Renamed `WindowRegistry` → `WindowLifecycle` (file pair `git mv`'d to preserve history)
- `5921294` — 🧹 Renamed leftover `registry` local in `MainWindow::closeEvent` to `lifecycle` after the class rename
- `9413ee9` — 🧹 Stripped roadmap framing from `app.hpp`'s class header
- `b73eb94` — 🔒 Reworked the `raise` lambda in `main.cpp` to read from `QApplication::topLevelWidgets()` instead of capturing the startup `windows` list by reference (which held raw `MainWindow*` that go dangling on close)

### Out-of-scope work I attempted and rolled back

- `865a081` (`🍎 UrlProtocol: implement macOS path…`) was an unscoped scope-grab to fix macOS URL protocol registration. Pulled — I'm not trusted with macOS work from this Windows box. The commit remains in history but is no longer my responsibility; treat the macOS branch as needing rework before any Mac build.

---

## Phase 3 — Theming

- [x] **Phase 3 complete**
  - [x] **8. Carve `ThemeBridge` from `SystemBridge`** — theme methods/signals moved to `app/bridges/theme/`; JS-side uses `getBridge<ThemeBridge>('theme')`; `SystemBridge` is pure stateless OS I/O; Catch2 + Bun tests added; commit `5b5c18a`
  - [x] **9. Extract `Theming`** — `QObject` subsystem parented to `App`; registers ThemeBridge, creates StyleManager, wires them; skip the construction → no StyleManager, no libsass, no watcher, no ThemeBridge; commits `ca4659a`, `e7c5a0d`, `cbb9b07`

---

## Phase 4 — Typed bridge access

- [x] **Phase 4 complete**
  - [x] **10. Add `app.addBridge<T>(name)` and `app.bridge<T>()`** — both APIs available; old fishing pattern still compiles; commit `31f60cd`
  - [x] **11. Migrate every fishing-cast site to typed `app.bridge<T>()`** — 4 sites across `main.cpp`, `main_window.cpp`, `menu_bar.cpp`, `web_shell_widget.cpp`; commit `31f60cd`
  - [x] **12. ~~Extract `register_bridges(registry)`~~** — shared header was created then deleted (`1a049bc` — "inline 2 lines, don't abstract them"); both entry points register bridges independently via `app.addBridge<T>()` (demo) and `registry.add()` (test_server); consistent but not DRY; commit `31f60cd`

---

## Phase 5 — `MainWindow` untangle

- [x] **Phase 5 complete**
  - [x] **13. Bare `MainWindowBase` + `MainWindow` preset** — `MainWindowBase` holds `App&` + `app()` accessor; `MainWindow` inherits and composes the standard set; commit `9203bd5`
  - [x] **14. MainWindow no longer knows WebShellWidget** — finds `QWebEngineView` via `findChild`, invokes `toggleDevTools` via `QMetaObject`; commit `3646202`
  - [x] **15. `DockManager` accepts any `QWidget`** — takes a `WidgetFactory` (std::function), content URL stored as dock property; commit `07ae473`
  - [x] **16. Docks carry their host as a property** — all `topLevelWidgets()` iterations in DockManager replaced by `"hostWindow"` property; commit `07ae473`
  - [x] **17. `undockTab` uses `tabData()` quintptr** — `windowTitle()` string-match and dead `dragTabTitle_` field removed; commit `69f7b99`
  - [x] **18. LoadingOverlay generalizes the message** — "open developer tools" instead of hardcoded "F12"; commit `69f7b99`

---

## Phase 6 — Apps split + xmake consolidation

- [x] **Phase 6 complete**
  - [x] **19. Consolidate framework into one `app-shell` static-lib xmake target** — 6 targets merged into one `app-shell` static lib (desktop) + one `app-shell-wasm` headeronly (WASM); files reorganized into `bridge/`, `core/`, `transport/`, `docks/`, `widgets/`, `capabilities/`; `AppLifecycle` renamed to `ReadySignal`; bridges keep their own targets; commit `5a61166`
  - [x] **20. Create `app/apps/demo/`** — demo binary with its own xmake target, `main.cpp` (224 lines), demo dialogs (`about_dialog`, `demo_widget_dialog`), tray items (Alpha/Beta/Gamma); all demo content lives under `app/apps/demo/`, zero in framework; commit `1ea962d`
  - [x] **21. Create `app/apps/main/` slate** — 19-line `main.cpp` (App + Theming + one MainWindow), its own xmake target `desktop`; commit `1ea962d`

---

## Phase 7 — Sweep

- [ ] **Phase 7 complete**
  - [x] **22. Unify `kBackground`** — three duplicates collapsed into `app_shell::kDefaultBackground` in `framework/core/colors.hpp` (`#242424`, matching `index.html`); `loading_overlay.cpp` fixed from wrong `#09090b` to correct value
  - [ ] **23. Update agent + human docs** — stale references found across 8+ doc files:
    - `Application::` old class name → `app_shell::App` (1 hit: `for-agents/02-architecture.md`)
    - `application.cpp` deleted file (8 hits across 5 files: `03-adding-features`, `06-gotchas`, `07-desktop-capabilities`, `for-humans/03-tutorial`, `for-humans/06-gotchas`)
    - `desktop/src/` old paths (5 hits across 4 files: `01-getting-started`, `03-adding-features`, `08-theming`, `for-humans/01-getting-started`, `for-humans/08-theming`)
    - `app/README.md` links point at `docs/DelightfulQtWebShell/` but actual path is `app/docs/DelightfulQtWebShell/`

---

## Open Questions

### ~~`app.cpp` hardcodes demo dev port~~ ✅ Fixed

Dev ports now flow from xmake `WEB_APPS` tables as compile-time defines (`WEB_APP_DEV_PORT_<NAME>`). Each app's `main.cpp` calls `app.registerDevPort()` with the defines. Framework has zero knowledge of "demo" or any specific app name.

### ~~QRC generators use absolute paths~~ ✅ Fixed

All three QRC generators (`build_helpers.lua`, `app/apps/demo/xmake.lua`, `app/apps/main/xmake.lua`) now use `path.relative()` instead of `path.absolute()`. Generated `.qrc` files are machine-independent.

### Demo menu items wired via title matching

The demo's `main.cpp` finds "Windows" and "Help" menus by iterating `findChildren<QMenu*>()` and matching on `menu->title().contains("Windows")`. Fragile — if someone renames the menu title, demo items silently disappear. Alternative: `buildMenuBar()` could set `objectName()` on each menu, or return menus in `MenuActions`.

### CI builds only the demo target

The three CI workflows (`ci.yml`, `nightly.yml`, `release.yml`) build `demo` but not `desktop` (the slate). Both targets should be verified in CI.
