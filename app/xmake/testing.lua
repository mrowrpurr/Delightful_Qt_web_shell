-- Capture at parse time — globals aren't available inside on_run closures
local _APP_NAME = APP_NAME
local _TEMPLATE_ROOT = TEMPLATE_ROOT

-- ── C++ unit tests (Catch2, no Qt) ──────────────────────────────────
-- lib.todos.test now lives with the pure-domain library at <repo>/lib/todos/xmake.lua
-- app.framework.test now lives with the qt-transport at app/framework/qt-transport/xmake.lua

-- ── pywinauto tests (native Qt window) ─────────────────────────────

target("app.test.automation")
    set_kind("phony")
    set_default(false)
    on_run(function()
        print(">>> uv run pytest tests/pywinauto/ (requires running demo app)")
        local base = _TEMPLATE_ROOT
        os.execv("uv", {"run", "pytest", "tests/pywinauto/", "-v"}, {curdir = base})
    end)

-- ── Playwright e2e tests (browser) ──────────────────────────────────

target("app.test.browser")
    set_kind("phony")
    set_default(false)
    on_run(function()
        print(">>> npx playwright test (browser)")
        local base = _TEMPLATE_ROOT
        os.execv("npx", {"playwright", "test"}, {curdir = base, envs = {VITE_APP_NAME = _APP_NAME}})
    end)

-- ── Playwright e2e tests (real Qt desktop app) ──────────────────────

target("app.test.desktop")
    set_kind("phony")
    set_default(false)
    on_run(function()
        print(">>> npx playwright test (demo via CDP)")
        local base = _TEMPLATE_ROOT
        os.execv("npx", {"playwright", "test"}, {curdir = base, envs = {DESKTOP = "1"}})
    end)

-- ── Bridge validation ───────────────────────────────────────────────
-- Checks that TypeScript bridge interfaces match C++ Q_INVOKABLE methods.
-- Catches drift between C++ and TS at dev time instead of runtime.

target("app.bridge.validate")
    set_kind("phony")
    set_default(false)
    add_deps("app.dev.server")
    on_run(function(target)
        local base = _TEMPLATE_ROOT
        local port = 19876  -- use a different port to avoid conflicts

        -- Start dev-server in background
        print(">>> Starting dev-server on port " .. port .. "...")
        local dev_server = target:dep("app.dev.server")
        local proc = os.runv(dev_server:targetfile(), {"--port", tostring(port)}, {detach = true})

        -- Wait for it to come online
        local start_time = os.time()
        while os.time() - start_time < 10 do
            local ok = try { function() os.runv("curl", {"-s", "-o", "/dev/null", "http://localhost:" .. port}) return true end }
            if ok then break end
            os.sleep(500)
        end

        -- Run the validator
        local ok = try {
            function()
                os.execv("bun", {"run", "tools/validate-bridges.ts"},
                    {curdir = base, envs = {BRIDGE_WS_URL = "ws://localhost:" .. port}})
                return true
            end
        }

        -- Kill the dev-server
        if is_plat("windows") then
            os.execv("taskkill", {"/IM", "dev-server.exe", "/F"}, {try = true, stdout = "/dev/null", stderr = "/dev/null"})
        else
            os.execv("pkill", {"-f", "dev-server.*" .. port}, {try = true})
        end

        if not ok then raise("Bridge validation failed") end
    end)

-- ── Bun unit tests ──────────────────────────────────────────────────

target("app.test.web")
    set_kind("phony")
    set_default(false)
    on_run(function()
        print(">>> bun test")
        local base = _TEMPLATE_ROOT
        os.execv("bun", {"test"}, {curdir = base})
    end)

