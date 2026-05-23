-- app-shell — one static library for the entire desktop framework.
--
-- On WASM, only the bridge + wasm-transport headers are needed (no Qt).
-- Bridges keep their own targets (WASM needs set_kind("object")
-- to prevent dead-stripping of EMSCRIPTEN_BINDINGS blocks).

if is_plat("wasm") then

    -- WASM: headeronly bridge + transport only (no Qt, no desktop framework)
    target("app-shell-wasm")
        set_kind("headeronly")
        add_includedirs("bridge", "transport/wasm", {public = true})
        add_packages("def_type", {public = true})

else

    -- Desktop: the full framework
    target("app-shell")
        set_kind("static")
        add_rules("qt.static")

        -- ── Source files ─────────────────────────────────────────
        add_files(
            "core/*.cpp", "core/*.hpp",
            "docks/*.cpp", "docks/*.hpp",
            "widgets/*.cpp", "widgets/*.hpp",
            "transport/qt/*.cpp", "transport/qt/*.hpp",
            "capabilities/app/*.cpp", "capabilities/app/*.hpp",
            "capabilities/window/*.cpp", "capabilities/window/*.hpp"
        )

        -- Platform-conditional sources
        if is_plat("windows") then
            add_files("capabilities/app/url_protocol_windows.cpp")
        end
        if is_plat("macosx") then
            add_files("capabilities/app/url_protocol_macos.mm", {rules = {"objc++"}})
            add_frameworks("CoreServices", "Foundation")
        end

        -- ── Include directories (public — consumers inherit) ────
        add_includedirs(
            "bridge",
            "core",
            "docks",
            "widgets",
            "transport/qt",
            "transport/wasm",
            "capabilities/app",
            "capabilities/window",
            {public = true}
        )

        -- ── Dependencies ────────────────────────────────────────
        -- SystemBridge: used by WebShellWidget (drag-drop) and MenuBar (save).
        -- ThemeBridge: used by Theming subsystem.
        add_deps("app.bridges.system", "app.bridges.theme", {public = true})
        add_packages("def_type", "qlementine-icons", "libsass", {public = true})

        -- ── Qt frameworks ───────────────────────────────────────
        add_frameworks(
            "QtCore", "QtGui", "QtWidgets", "QtNetwork",
            "QtWebEngineCore", "QtWebEngineWidgets", "QtWebChannel",
            "QtWebSockets",
            {public = true}
        )

        -- ── Consumer identity ───────────────────────────────────
        add_defines('APP_NAME="' .. APP_NAME:gsub('"', '\\"') .. '"')
        add_defines('APP_SLUG="' .. APP_SLUG:gsub('"', '\\"') .. '"')
        add_defines('APP_ORG="' .. APP_ORG:gsub('"', '\\"') .. '"')
        add_defines('APP_VERSION="' .. APP_VERSION:gsub('"', '\\"') .. '"')

        -- Point at the repo's styles folder for live SCSS reload.
        if not os.getenv("CI") then
            local styles_path = path.join(os.scriptdir(), "styles"):gsub("\\", "/")
            add_defines('STYLES_DEV_PATH="' .. styles_path .. '"')
        end

        -- ── Embedded QSS themes ──────────────────────────────────
        -- StyleManager falls back to :/styles/<theme>.qss at runtime.
        -- The QRC resources must live in the final binary (not a static lib)
        -- to avoid dead-stripping of the static initializer that registers them.
        -- Consumers add framework/styles/styles_resources.cpp to their own
        -- target's add_files(). The before_build hook in each app target calls
        -- build_helpers.compile_styles_qrc() to generate the QRC + cpp.

    -- ── Test targets ────────────────────────────────────────────

    target("test-bridge-channel-adapter")
        set_kind("binary")
        set_default(false)
        add_rules("qt.console")
        add_deps("app-shell")
        add_files("tests/unit/bridge_channel_adapter_test.cpp")
        add_frameworks("QtCore", "QtTest")
        add_packages("catch2")
        set_rundir("$(projectdir)")

end
