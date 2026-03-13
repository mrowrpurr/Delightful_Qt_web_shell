-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
--  Customize your app:
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
APP_NAME    = "Delightful Qt Web Shell"
APP_SLUG    = "delightful-qt-web-shell"
APP_VERSION = "0.1.0"
-- ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

set_project(APP_SLUG)
set_version(APP_VERSION)

add_rules("mode.release")
set_defaultmode("release")
set_languages("c++23")

if is_plat("windows") then
    set_runtimes("MD")
end

add_requires("catch2 3.x")

-- ── Libraries ────────────────────────────────────────────────────────

includes("lib/todos/xmake.lua")
includes("lib/web-shell/xmake.lua")
includes("lib/bridges/xmake.lua")

-- ── Desktop app ──────────────────────────────────────────────────────

includes("desktop/xmake.lua")

-- ── Test infrastructure ─────────────────────────────────────────────

includes("tests/helpers/dev-server/xmake.lua")

-- ── Build targets ───────────────────────────────────────────────────

includes("xmake/setup.lua")
includes("xmake/scaffold-bridge.lua")
includes("xmake/dev.lua")
includes("xmake/testing.lua")
