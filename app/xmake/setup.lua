-- Capture at parse time — globals aren't available inside on_run closures
local _TEMPLATE_ROOT = TEMPLATE_ROOT

-- ── Setup (install all dependencies) ──────────────────────────────────

target("setup")
    set_kind("phony")
    set_default(false)
    on_run(function()
        import("lib.detect.find_tool")
        local root = _TEMPLATE_ROOT

        print("── uv sync ──")
        os.execv("uv", {"sync"}, {curdir = root})

        print("── bun install ──")
        os.execv("bun", {"install"}, {curdir = root})

        print("── tools/playwright-cdp npm install ──")
        os.execv("npm", {"install"}, {curdir = path.join(root, "tools", "playwright-cdp")})

        print("── playwright install chromium ──")
        os.execv("npx", {"playwright", "install", "chromium"}, {curdir = root})

        print("✅ All dependencies installed")
    end)
