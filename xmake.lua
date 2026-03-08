set_project("delightful-qt-web-shell")
set_version("0.1.0")

add_rules("mode.release")
set_defaultmode("release")
set_languages("c++23")

if is_plat("windows") then
    set_runtimes("MD")
end

add_requires("catch2 3.x")

-- ── C++ unit tests (Catch2, no Qt) ───────────────────────────────────

target("test-todo-store")
    set_kind("binary")
    set_default(false)
    add_files("tests/todo_store_test.cpp")
    add_packages("catch2")

-- ── Headless WebSocket test server (Qt, no GUI) ──────────────────────

target("test-server")
    set_kind("binary")
    set_default(false)
    add_rules("qt.console")
    add_files(
        "cpp/test_server.cpp",
        "cpp/bridge.hpp",
        "cpp/expose_as_ws.hpp"
    )
    add_frameworks("QtCore", "QtNetwork", "QtWebSockets")

-- ── Playwright e2e tests ─────────────────────────────────────────────

target("test-e2e")
    set_kind("phony")
    set_default(false)
    on_run(function()
        print(">>> npx playwright test")
        os.execv("npx", {"playwright", "test"})
    end)

-- ── Run all tests ────────────────────────────────────────────────────

target("test-all")
    set_kind("phony")
    set_default(false)
    on_run(function()
        print(">>> Catch2: TodoStore unit tests")
        os.execv("xmake", {"run", "test-todo-store"})
        print("")
        print(">>> Playwright: e2e tests")
        os.execv("xmake", {"run", "test-e2e"})
    end)
