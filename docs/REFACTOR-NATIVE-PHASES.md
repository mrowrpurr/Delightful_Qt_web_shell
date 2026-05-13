# Native Refactor — Phasing 🏴‍☠️

Companion to `REFACTOR-NATIVE.md`.

The unit of work is the commit. Each numbered commit below is atomic: it builds green (`xmake build desktop` + `xmake build wasm-app`) and the demo still runs after it lands. Phases are narrative groupings.

The whole refactor is done when the 6 consumer scenarios in `REFACTOR-NATIVE.md`'s "What 'Done' Looks Like" all build without editing framework files.

---

## Phase 1 — Foundation

Pivot point for everything that follows. Every later phase wants `App&` available.

1. **Introduce `app_shell::App` as a real class.** `Application` becomes a thin transitional subclass calling the same internal setup methods as today. Behavior identical.
2. **Kill `qobject_cast<Application*>(qApp)`.** Every site that was fishing `qApp` for its real type takes an `App&` reference instead.
3. **`app.iconPath()` accessor.** Framework reads icons through it; the three hardcoded `:/icon.ico` literals are gone.

---

## Phase 2 — App-scoped subsystems opt-in

Each App-level capability becomes a `QObject` subclass parented to `App`, constructed by the consumer in `main()`, retrievable anywhere via `app.findChild<T*>()`. No framework-specific base class; composition is Qt's parent/child tree. Demo's `main.cpp` constructs each subsystem on every commit so behavior is preserved.

4. **Extract `Tray`.** Pulls the 60 lines of `setupSystemTray()` out of `App`'s ctor into `app_shell::Tray` (a `QObject` subclass owning a `QSystemTrayIcon` + `QMenu`). Demo's tray content (Alpha/Beta/Gamma submenus) moves into demo's `main.cpp`. Framework's tray class never sees demo strings again.
5. **Extract `UrlProtocol`.** Static methods on `App` (`isUrlProtocolRegistered`, `registerUrlProtocol`, `unregisterUrlProtocol`, `urlProtocolName`) become instance methods on the subsystem. The Tools menu register/unregister action retrieves the subsystem via `app.findChild<UrlProtocol*>()`.
6. **Extract `SingleInstance`.** ⚠️ Ordering caveat: today's `setupSingleInstance()` is the *first* thing App's ctor does and short-circuits the rest for secondary processes. After extraction, `App app(argc, argv)` runs to completion before `main()` constructs `SingleInstance` and checks `isPrimary()` — secondary processes pay for whatever heavy init App's ctor still does (web profile setup, bridges, dock manager) before exiting. This shrinks naturally as Phases 3 and 4 lighten App's ctor; until then it's a transitional regression. **Open question:** accept it, or land Phase 2.6 *after* Phases 3 and 4?
7. **Extract `WindowRegistry`.** `DockManager::restoreWindows()` and the `topLevelWidgets()`-iteration in `MainWindow::closeEvent` move into the subsystem. Constructed only by consumers who want it.

---

## Phase 3 — Theming

Heaviest opt-in. Carve before gating.

8. **Carve `ThemeBridge` from `SystemBridge`.** Theme methods and signals move to a dedicated bridge at `app/bridges/theme/`, registered alongside `SystemBridge`. JS-side calls update to `getBridge<ThemeBridge>('theme')`. `SystemBridge` becomes pure stateless OS I/O.
9. **Extract `Theming(app, baseline)`.** App-scoped `QObject` subsystem. Demo constructs it. Skip the construction → no StyleManager, no libsass, no watcher, no `ThemeBridge` registered.

---

## Phase 4 — Typed bridge access

10. **Add `app.addBridge<T>(name)` and `app.bridge<T>()`.** Both APIs available; the old fishing pattern still compiles.
11. **Migrate every fishing-cast site to typed `app.bridge<T>()`.** No remaining `static_cast<*Bridge*>(shell()->bridges().value(...))` anywhere.
12. **Extract `register_my_bridges(app)`.** Single registration function called from `application.cpp` and `test_server.cpp` both.

---

## Phase 5 — `MainWindow` untangle

The largest tangle in the codebase. Each commit unhooks one strand. Window-scoped subsystems follow the same shape as Phase 2's app-scoped ones: `QObject` subclasses parented to `MainWindow`, retrieved via `window->findChild<T*>()`.

13. **Bare `MainWindowBase` + `MainWindow` preset that composes window-scoped subsystems.** Base is a thin `QMainWindow` subclass with no widgets and no behavior installed. The preset's ctor `new`s the standard subsystems (`WebShell` as central, `MenuBar`, `StatusBar`, `DevToolsShortcut`, `ReactiveTitle`, `PersistedGeometry`) as children of `this`. Behavior identical to today; structure is just composed instead of hardcoded.
14. **Extract `WebShell` as the dedicated subsystem owning the `QWebEngineView`.** `DevToolsShortcut` and `ReactiveTitle` find it via `window->findChild<WebShell*>()` instead of `MainWindow` knowing `WebShellWidget` directly.
15. **`DockManager` accepts any `QWidget`.** No longer constructs `WebShellWidget` itself.
16. **Docks carry their host as a property.** `DockManager` ↔ `MainWindow` `topLevelWidgets()` iteration dies.
17. **`undockTab` uses `tabData()` quintptr.** `windowTitle()` string-match dies.
18. **`LoadingOverlay` reads its devtools shortcut from the menu** (or generalizes the message). The hardcoded "F12" literal dies.

---

## Phase 6 — Apps split + xmake consolidation

19. **Consolidate framework into one `app-shell` static-lib xmake target.** Transport-qt vs transport-wasm becomes platform-conditional `add_files` inside it. Bridges keep their own targets (`set_kind("object")` for WASM is non-negotiable).
20. **Create `app/apps/demo/`; move the desktop binary's xmake target there.** Demo content lives entirely under `app/apps/demo/`.
21. **Create `app/apps/main/` slate.** ~5–10 line `main.cpp`, registers nothing by default, opens a single `MainWindow`. Both binaries build by default.

---

## Phase 7 — Sweep

22. **Unify `kBackground`.** Three duplicates with **inconsistent hex values** (`0x24,0x24,0x24` vs `0x09,0x09,0x0b`) collapse to one source of truth matching `--bg` in `App.css`.
23. **Update agent + human docs.** `app/docs/DelightfulQtWebShell/for-{agents,humans}/` reference the new API; no stale `Application::` or fishing-cast snippets remain.
