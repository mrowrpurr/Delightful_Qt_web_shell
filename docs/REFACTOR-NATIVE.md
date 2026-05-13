# Native-Side Refactor — Making Qt Delightful for Template Users 🏴‍☠️

> **The Qt-side framework gets in your way today.** This refactor inverts it: opt-in, composable, member-style. After it lands, your `main.cpp` is a recipe — every line is a feature you asked for. Tray-only mini app or full IDE-style multi-window dock UI, both feel native. **Forking this template should be delightful at any scale.**

---

## The Principle

Today: god-class `Application` + monolithic `MainWindow`. Every feature is hardcoded ON. To opt out, you edit framework code. Demo content (Alpha/Beta/Gamma tray submenus, demo dialogs, "Demo Widget" menu actions) lives mixed inside framework files.

Target: `app_shell::App` is tiny. Every capability is a `QObject` subclass the consumer constructs and parents to its host (`App` or `MainWindow`) — composition via Qt's parent/child tree. Retrieval anywhere with the host is plain `findChild<T*>()`. Lifetime is automatic. The framework is silent about every choice you didn't make. Demo content lives in its own app binary, not in framework classes.

> **Opt-in framework, not opt-out god-class.** Same thesis as the frontend's slate-app story — a consumer should be able to start from nothing and add only what they reach for.

---

## The Consumer Story

Day one for someone forking this template. Three concrete shapes, same framework, three scales:

### Scenario 1 — Tray-only utility with a tiny web settings popup

```cpp
app_shell::App app(argc, argv);
app.addBridge<MySettingsBridge>("settings");

auto* tray = new app_shell::Tray(&app);
tray->addItem("Settings…", [&]{ app.openWebDialog(app.appUrl("settings")); });
tray->addItem("Quit",      [&]{ app.requestQuit(); });
tray->show();

return app.exec();
```

### Scenario 2 — Single-window app, no docks

```cpp
app_shell::App app(argc, argv);
app.addBridge<MyBridge>("my");

app_shell::MainWindow window(app, app.url("main"));
window.show();

return app.exec();
```

### Scenario 4 — IDE-like (current demo)

```cpp
app_shell::App app(argc, argv);

auto* singleInstance = new app_shell::SingleInstance(&app);
if (!singleInstance->isPrimary()) return 0;

auto* urlProtocol = new app_shell::UrlProtocol(&app);
urlProtocol->promptIfNeeded();

new app_shell::Theming(&app, "default-dark");
auto* tray = new app_shell::Tray(&app);
// (demo wires its tray menu items here)

app.addBridge<MyBridge>("my");
app.addBridge<app_shell::SystemBridge>("system");

auto* windows = new app_shell::WindowRegistry(&app);
auto mainWindows = windows->restoreOrCreate<app_shell::MainWindow>();
for (auto* w : mainWindows) w->show();

return app.exec();
```

All under 20 lines. **No `qobject_cast<Application*>(qApp)`. No editing of framework files. No demo content lurking in your tray menu.**

The other three scenarios — multi-window tools-and-inspectors (#3), WASM-only (#5), wizard-only (#6) — are equally trivial in the new shape. See `docs/refactor-recon/03-scenarios.md` for the full set.

---

## Target Shape: Folder Layout

```
<root>/
├── lib/                        ← pure C++ domains, reusable across projects
│   └── todos/                    (the demo's domain — no Qt, no GUI)
│
├── app/
│   ├── framework/              ← the toolbox (namespace: app_shell::)
│   │   ├── bridge/                  Bridge base — frontend agent owns naming
│   │   ├── transport/{qt,wasm}/     Transport adapters
│   │   ├── shell/                   App, MainWindow, MainWindowBase,
│   │   │                            WebShellWidget, SchemeHandler, LoadingOverlay
│   │   ├── services/                Tray, UrlProtocol, SingleInstance, Theming,
│   │   │                            WindowRegistry, DockSystem
│   │   └── ui/                      MenuBuilder, StatusBar, WebDialog
│   │
│   ├── bridges/                ← typed bridge wrappers per domain
│   │   ├── todos/                   wraps lib/todos
│   │   ├── system/                  SystemBridge (clipboard, files, drag-drop, ...)
│   │   └── theme/                   ThemeBridge (auto-registered by Theming ctor)
│   │
│   ├── apps/                   ← N desktop apps. Mirrors web/apps/.
│   │   ├── demo/                    the showcase (IDE-like)
│   │   └── main/                    the SLATE — consumer's primary, ships nearly empty
│   │   (consumer adds: app/apps/<my-utility>/, etc.)
│   │
│   ├── web/                    ← frontend (separate refactor, not in this scope)
│   ├── wasm/
│   └── tests/
│
└── xmake.lua
```

Every folder name announces its purpose. Consumer day-one journey: open `app/apps/main/`, that's where they live.

---

## Target Shape: Namespace

`app_shell::` — flat top-level for first-class concerns; sub-namespaces only for transport adapters.

```cpp
namespace app_shell {
  // Hosts
  class App;
  class MainWindow;
  class MainWindowBase;

  // App-scoped subsystems — QObject children parented to App, retrieved via app.findChild<T*>()
  class Tray;
  class UrlProtocol;
  class SingleInstance;
  class Theming;
  class WindowRegistry;

  // Window-scoped subsystems — QObject children parented to MainWindow, retrieved via window->findChild<T*>()
  class WebShell;
  class MenuBar;
  class StatusBar;
  class DevToolsShortcut;
  class ReactiveTitle;
  class PersistedGeometry;
  class DockSystem;

  // Plumbing (not composable subsystems — framework internals consumers don't construct directly)
  class WebDialog;
  class LoadingOverlay;
  class SchemeHandler;

  // Transport adapters
  namespace qt   { class BridgeChannelAdapter; /* expose_as_ws, json_adapter */ }
  namespace wasm { class BridgeWrapper;        /* wasm_bindings */ }
}
```

The `Bridge` base class and the bridge registry live in their own (transport-agnostic) namespace owned by the frontend refactor agent. `app_shell::App` knows about it via accessor methods; consumers never touch the registry directly.

Consumer bridges (`SystemBridge`, `ThemeBridge`, consumer's own) live in their own namespaces or no namespace — they're consumer extension points and should not be claimed by `app_shell::`. (`SystemBridge` and `ThemeBridge` ship in `app/bridges/` but are not framework-owned in the namespace sense — consumers can replace them.)

---

## Locked Decisions

For each: what we're doing + **why**.

### 1. `MainWindowBase` (bare) + `MainWindow` (preset)

`MainWindowBase` is a thin `QMainWindow` subclass with no widgets and no behavior installed. Consumers compose by constructing subsystem `QObject` children parented to the window. The framework ships a `MainWindow` preset that does the standard composition for the typical case.

```cpp
class MainWindowBase : public QMainWindow {
public:
    explicit MainWindowBase(App& app);
    App& app() const { return app_; }
private:
    App& app_;
};

class MainWindow : public MainWindowBase {
public:
    MainWindow(App& app, const QUrl& url) : MainWindowBase(app) {
        auto* shell = new WebShell(this, url);
        setCentralWidget(shell);
        new MenuBar(this);
        new StatusBar(this);
        new DevToolsShortcut(this);
        new ReactiveTitle(this, shell);
        new PersistedGeometry(this);
    }
};
```

Each `new Subsystem(this, ...)` is parented to the window. Qt's parent/child contract handles cleanup. Anywhere else with a `MainWindow*` retrieves via `findChild`:

```cpp
if (auto* status = window->findChild<StatusBar*>()) {
    status->showMessage("Saved", 2000);
}
```

**Why:** Today's `MainWindow` is 428 lines, knows `WebShellWidget` concretely, owns the dock-tab system, and has a 0×0 placeholder central widget — hostile to non-dock apps. The new shape gives consumers three usable starting points:

- **Bare `MainWindowBase`** for power users (e.g., Qt-native central widget + web content in docks — a real use case)
- **`MainWindow` preset** for the typical case (one line, you get the works)
- **Derive from `MainWindow` or `MainWindowBase`** and pick exactly which subsystems to `new` in your ctor

The preset is not magic — it's a tiny ctor that `new`s a small number of standard subsystems. Consumers read the preset's ctor to see what's installed and copy the pieces they want into their own subclass.

### 2. Opt-in composition via Qt's parent/child tree

`App`'s constructor does only the minimum: identity, web profile, scheme handler in production, the bridge dispatch contract, logging. Every other capability is a `QObject` subclass the consumer constructs and parents to its host. App-scoped subsystems parent to `App`; window-scoped subsystems parent to a `MainWindow`. No framework-specific base class. No custom registry. The composition primitive is Qt's parent/child tree.

```cpp
// App-scoped — parented to App
auto* tray  = new app_shell::Tray(&app);
auto* theme = new app_shell::Theming(&app, "default-dark");

// Window-scoped — parented to the window (see Decision #1)
new app_shell::StatusBar(this);
new app_shell::DevToolsShortcut(this);
```

Retrieval anywhere with the host is plain Qt:

```cpp
if (auto* tray = app.findChild<app_shell::Tray*>()) {
    tray->qIcon()->showMessage("Notice", msg);
}

if (auto* status = window->findChild<app_shell::StatusBar*>()) {
    status->showMessage("Saved", 2000);
}
```

Cleanup is automatic — when the host dies, Qt deletes its children. Consumers who don't construct a subsystem don't pay for it (no instance, no registration, no symbol referenced — the linker DCEs unused subsystem classes).

App still exposes a few non-subsystem conveniences for things every consumer wants:

```cpp
app.bridge<T>()          → typed bridge access (#3 below)
app.addBridge<T>(name)   → typed bridge registration
app.openWebDialog(url)   → modal web dialog (one-liner from anywhere)
app.requestQuit()        → cleanup-then-quit
app.appUrl(name)         → resolve dev/prod URL for a web app
```

**Why:** Today `Application::Application()` runs 14 setup steps in one constructor. Tray (with literal Alpha/Beta/Gamma demo submenus), URL protocol prompt, single-instance pipe, theme baseline, dock manager — all hardcoded ON. Consumer who doesn't want tray must edit framework code.

Two earlier drafts of this doc were wrong about the mechanism:

1. **Lazy `app.useTray()` accessor methods.** Each `useX()` was a stateful side-effecting registry call dressed up as a builder. It bloated `App`'s surface (one method per subsystem), lied about its imperative nature, and created a split-brain where direct construction and `app.useTray()` produced different state — the same `addItem` method silently behaving differently depending on which constructor ran.

2. **A `Feature` base class with a `typeid` registry on `App`.** Reinvented `QObject::findChild`. The parent/child tree already provides typed lookup, automatic cleanup, and Qt-idiomatic composition; adding a parallel custom registry was overengineering against a phantom benefit.

The honest mechanism is Qt's parent/child tree. Subsystems are normal `QObject` children of their host. Construction is explicit (`new Subsystem(host, ...)`), retrieval is the built-in `findChild`, cleanup is automatic. The framework adds zero machinery on top of what every Qt programmer already knows; it just provides the subsystem classes.

### 3. Typed bridge access

```cpp
app.addBridge<SystemBridge>("system");          // typed at compile, named on the wire
auto* sys = app.bridge<SystemBridge>();          // typed retrieval, no fishing
```

Replaces the fishing pattern that exists in 4 sites today:
```cpp
auto* bridge = static_cast<SystemBridge*>(shell()->bridges().value("system"));
```

**Why:** String-name + cast is error-prone (rename the bridge, four sites silently get nullptr) and forces framework code to know consumer-specific bridge types. Typed access kills that. The wire-protocol name is one string; consumer code uses the type. The "register the same bridge in two places" footgun (`application.cpp` + `test_server.cpp`) is solved by convention: the consumer writes one `register_my_bridges(app)` function and both call sites use it.

### 4. SystemBridge sheds theme control; `ThemeBridge` is separate

`SystemBridge` today holds `setQtTheme`/`getQtTheme`/`qtThemeChanged` — a category error on a "stateless OS I/O" bridge. **Themes go to a dedicated `ThemeBridge`**, auto-registered by `Theming`'s constructor. Consumer who skips constructing `Theming` has no theme bridge in their binary. Lives in `app/bridges/theme/`.

`SystemBridge` stays as one bridge for stateless OS I/O:

- File I/O — read/write text + binary, streaming handles for large files
- Folder listing + glob
- Native pickers — open file, open folder, save dialog
- Drag & drop — `filesDropped` signal + `getDroppedFiles()`
- Clipboard — copy/read
- CLI args / URL protocol forwarding — `appLaunchArgsReceived` signal
- Save signal — `saveRequested` (toolbar/menu Save → React)
- Native dialog request — `openDialog` signal (React asks for a Qt-native dialog)

Ships in `app/bridges/system/`. Consumer registers it via `app.addBridge<SystemBridge>("system")` if they want it.

**Why:** Theme control is coordination between React and StyleManager, not OS I/O. It snuck onto SystemBridge as the code grew. Separating restores the clean mental model and lets a tray-only consumer skip the entire theme system without touching a useless `setQtTheme` method.

### 5. Theming is opt-in (heavy + composable)

`new Theming(&app, baseline)` attaches the full theme system: StyleManager, libsass, file watcher, JSON name mapping, dark/light suffix convention, `ThemeBridge` auto-registered. Consumer who skips constructing `Theming` gets Qt's default style and zero theming dependencies in their binary.

Two axes, separately controllable:

| | Tiny app | Full app |
|---|---|---|
| **Feature richness** (Axis 1) | minimal — one QSS embedded, no live reload | full — StyleManager + libsass + watcher + JSON map + dark/light |
| **Theme set size** (Axis 2) | 1 (e.g. `catppuccin-dark`) | 1000+ shadcn |

Axis 1 = controlled by `Theming(...)` ctor options.
Axis 2 = controlled at build time via xmake config (consumer's `xmake.lua` declares which themes embed). **Tracked as future work** — not load-bearing for the headline refactor.

**Why:** The QSS theme system is genuinely production-grade and gets used in 100% of typical apps. But a tray-only utility shouldn't pay for libsass + 1000 themes + watcher just to exist. Opt-in keeps it heavy when wanted, invisible when not.

### 6. N desktop apps under `app/apps/<name>/`

Each desktop app is its own xmake binary, sharing framework + bridges + lib. Demo and slate are both apps under `app/apps/`. Consumer adds siblings as they grow.

**Why:** Mirrors the web side's `apps/` plural exactly. Consistent mental model across native + web. Opens the door for utility-app patterns — main app + tiny tray utility, both shipping from one codebase. Today's 1-app desktop convention is a lid on real use cases.

### 7. Demo as `app/apps/demo/`, slate as `app/apps/main/`

Both are full apps. Demo is the running showcase of every framework feature (Scenario 4 — full IDE-style). Slate is the consumer's empty starting point (~5–10 line `main.cpp`). Two separate binaries, both built by default.

**Why:** Mirrors the web-side `apps/demo` + `apps/app` story. Consumer keeps the slate, deletes/ignores the demo, adds whatever else. The demo binary is a *running reference* the consumer learns from — not buried in their product binary.

### 8. xmake — one framework target

**`app-shell` is one xmake target.** Static library. Everything inside it: `App`, `MainWindow` + `MainWindowBase`, all app- and window-scoped subsystems (`Tray`, `UrlProtocol`, `SingleInstance`, `Theming` with libsass, `WindowRegistry`, `WebShell`, `MenuBar`, `StatusBar`, `DevToolsShortcut`, `ReactiveTitle`, `PersistedGeometry`, `DockSystem`), ui plumbing, both transport adapters. Transport-qt vs transport-wasm is platform-conditional `add_files` *inside* `app-shell`'s `xmake.lua`, not separate targets.

**Why:** Opt-in is a class-level concept — consumer who doesn't construct `Tray` doesn't instantiate the class, and the linker DCEs unused symbols. xmake target granularity buys nothing on top of that. Splitting into per-subsystem targets was overengineering against a phantom benefit.

Bridges live in their own xmake targets per Phase 1's plan (must-preserve #10 — WASM bridges need `set_kind("object")` to keep `EMSCRIPTEN_BINDINGS` from being dead-stripped). Frontend-agent territory.

### 9. Branding + identity

Keep xmake-time `APP_NAME`/`APP_SLUG`/`APP_ORG`/`APP_VERSION` defines for Windows `.exe` metadata generation (which must happen at build time). Decouple framework code from `:/icon.ico` literal references — framework reads via `app.iconPath()` accessor, which returns whatever the consumer set or a built-in framework default.

**Why:** Today the framework hardcodes `:/icon.ico` in three places. Consumer who renames their icon needs to either keep that exact filename or edit framework code.

---

## Out of Scope

These came up and are explicitly NOT part of this refactor.

- **Lifecycle hooks** (signals on `App` like `beforeBridgeRegistration`, `afterFirstWindowShown`) — invented to solve a problem that doesn't exist. Dropped. If a real need surfaces later, add a signal then.
- **Multiple starting templates as folders** (`app/templates/tray-popup/` etc.) — slate + demo + docs is enough. If consumers need more example shapes, doc snippets handle it without shipping copyable folders that need maintenance.
- **Registry / Bridge base class naming** — owned by the frontend refactor agent (transport-agnostic concern). Whatever they pick, `app_shell::App` adapts to it.

---

## Cross-Cutting Cleanup (drops out of the bigger work)

These die naturally as the architectural pieces land:

- All `qobject_cast<Application*>(qApp)` sites die — replaced by `app_shell::App&` references passed explicitly
- All "fishing for bridges" sites (`shell()->bridges().value("system")`) die — replaced by typed `app.bridge<T>()`
- `kBackground` color constant duplicated in 3 places (with **inconsistent hex values** — `0x24,0x24,0x24` vs `0x09,0x09,0x0b`, both with "Must match --bg in App.css" comments) — single source of truth
- `windowTitle()` string-match in `dock_tab_manager.undockTab` — replaced by existing `tabData()` quintptr mechanism (already used elsewhere)
- `LoadingOverlay`'s "F12" reference in error message — read from menu shortcut, or genericized to "open developer tools"
- `MainWindow` knows `WebShellWidget` concretely — splits into the `WebShell` subsystem (constructed as a child of the window) plus separate `DevToolsShortcut` / `ReactiveTitle` subsystems that find the shell via `window->findChild<WebShell*>()`
- `DockManager` constructs `WebShellWidget` directly — becomes content-agnostic, accepts any `QWidget`
- `DockManager` ↔ `MainWindow` bidirectional concrete-type knowledge — docks carry their host as a property, no `topLevelWidgets()` iteration

---

## Patterns This Refactor MUST Preserve

The silent-failure list from `app/docs/DelightfulQtWebShell/for-agents/06-gotchas.md`. Don't regress on any of these:

1. `signalReady()` fires after mount in every web app
2. `getBridge<T>(...)` lives at module scope with top-level await on the JS side
3. Bridge registration mirrored in production main + dev-server (the consumer-side `register_my_bridges(app)` convention codifies this)
4. `QTimer::singleShot(0, ...)` when a bridge method opens a modal
5. Monaco worker registration before any `<MonacoEditor>` mounts
6. `playwright-cdp` runs under `npx tsx`, not `bun`
7. `assetsInlineLimit: 0` in every `vite.config.ts`
8. `qtSyncGuard` in the React→Qt theme listener
9. localStorage keys are persisted state — don't rename
10. `bridges/wasm` library uses `set_kind("object")`, not `static`
11. `QCommandLineParser::parse()`, never `process()`

---

## What "Done" Looks Like

A consumer forking this template can do all of these without editing framework files:

- ✅ Build a tray-only utility with one web dialog (Scenario 1)
- ✅ Build a single-window app with no docks (Scenario 2)
- ✅ Build a multi-window app with single-instance + tools (Scenario 3)
- ✅ Build an IDE-style multi-window dock UI with the works (Scenario 4)
- ✅ Build a WASM-only browser app (Scenario 5)
- ✅ Build a wizard/dialog-only app (Scenario 6)
- ✅ Add a custom bridge by writing one class + registering it
- ✅ Replace the demo's tray content (or skip tray entirely)
- ✅ Have multiple desktop apps under `app/apps/` sharing framework + bridges + lib
- ✅ (future) Pick which themes embed in their binary at build time

The framework gets out of the way. **The template becomes delightful.** 🏴‍☠️

---

## Reference

Recon scratchpads with deeper findings:

- `docs/refactor-recon/01-inventory.md` — every native-side class with responsibility tags (🎭 demo / 🎨 polish / 🔧 default-on / 🏗 core)
- `docs/refactor-recon/02-tangles.md` — 16 dependency tangles + untangle-first shortlist
- `docs/refactor-recon/03-scenarios.md` — six consumer scenarios stress-testing the API
- `docs/refactor-recon/04-proposals.md` — full proposal options + tradeoffs (this doc is the locked subset)
