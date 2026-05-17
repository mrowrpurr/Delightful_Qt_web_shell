-- Capture at parse time — globals aren't available inside on_run closures
local _TEMPLATE_ROOT = TEMPLATE_ROOT

-- ── Scaffold a new bridge ────────────────────────────────────────────
--
-- xmake run scaffold-bridge notes
--
-- Creates a pure C++ bridge (extending app_shell::Bridge) with def_type
-- DTOs, wires it into main.cpp, test_server.cpp, xmake targets, and
-- creates a TS interface stub. Does NOT write into <repo>/lib/ — that's
-- consumer territory for pure domain logic.

target("scaffold-bridge")
    set_kind("phony")
    set_default(false)
    on_run(function()
        import("core.base.option")

        -- ── Parse name ──────────────────────────────────────────
        local args = option.get("arguments") or {}
        local name = args[1]
        if not name or name == "" then
            raise("Usage: xmake run scaffold-bridge <name>\n  e.g. xmake run scaffold-bridge notes")
        end

        -- ── Derive names ────────────────────────────────────────
        -- "notes" → class "NotesBridge", file "notes_bridge.hpp",
        --           TS bridge name "notes", getter "getNotesBridge"
        local slug = name:lower():gsub("[^%w]", "-")           -- notes
        local snake = slug:gsub("-", "_")                       -- notes
        local file_name = snake .. "_bridge"                    -- notes_bridge
        local dtos_name = snake .. "_dtos"                      -- notes_dtos
        local class_name = slug:gsub("(%a)([%w]*)",             -- NotesBridge
            function(a, b) return a:upper() .. b end):gsub("-", "") .. "Bridge"
        local target_name = "app.bridges." .. slug              -- app.bridges.notes

        local root = _TEMPLATE_ROOT
        local bridge_dir = path.join(root, "bridges", slug, "include")
        local hpp_path = path.join(bridge_dir, file_name .. ".hpp")
        local dtos_path = path.join(bridge_dir, dtos_name .. ".hpp")

        -- ── Guard against overwrite ─────────────────────────────
        if os.isfile(hpp_path) then
            raise(hpp_path .. " already exists!")
        end

        -- ── Create directory ────────────────────────────────────
        os.mkdir(bridge_dir)

        -- ── 1. Create DTOs header ───────────────────────────────
        io.writefile(dtos_path,
            '#pragma once\n\n'
            .. '#include <def_type.hpp>\n\n'
            .. '// Request/response DTOs for ' .. class_name .. '.\n'
            .. '// Each bridge method takes one request struct and returns one response struct.\n'
            .. '//\n'
            .. '// Example:\n'
            .. '//   struct GetDataRequest {\n'
            .. '//       std::string id;\n'
            .. '//   };\n'
            .. '//\n'
            .. '//   struct GetDataResponse {\n'
            .. '//       std::string name;\n'
            .. '//       int count = 0;\n'
            .. '//   };\n')

        -- ── 2. Create bridge header ─────────────────────────────
        io.writefile(hpp_path,
            '#pragma once\n\n'
            .. '#include "' .. dtos_name .. '.hpp"\n'
            .. '#include "bridge.hpp"\n\n'
            .. 'class ' .. class_name .. ' : public app_shell::Bridge {\n'
            .. 'public:\n'
            .. '    ' .. class_name .. '() {\n'
            .. '        // Register methods:\n'
            .. '        //   method("getData", &' .. class_name .. '::getData);\n'
            .. '        //\n'
            .. '        // Register signals — name them after what happened, not "changed":\n'
            .. '        //   signal("itemCreated");\n'
            .. '        //   signal("itemArchived");\n'
            .. '    }\n\n'
            .. '    // Each method takes a request DTO, returns a response DTO.\n'
            .. '    // def_type handles JSON serialization automatically.\n'
            .. '    //\n'
            .. '    // Example:\n'
            .. '    //   GetDataResponse getData(GetDataRequest req) {\n'
            .. '    //       auto result = do_something(req.id);\n'
            .. '    //       emit_signal("itemCreated", result);\n'
            .. '    //       return result;\n'
            .. '    //   }\n'
            .. '};\n')

        -- ── 3. Create xmake.lua for the bridge target ───────────
        local xmake_path = path.join(root, "bridges", slug, "xmake.lua")
        io.writefile(xmake_path,
            'target("' .. target_name .. '")\n'
            .. '    set_kind("headeronly")\n'
            .. '    add_deps("app.framework.bridge", {public = true})\n'
            .. '    add_includedirs("include", {public = true})\n'
            .. '    add_packages("def_type", {public = true})\n')

        -- ── 4. Wire includes into app/xmake.lua ─────────────────
        local app_xmake = path.join(root, "xmake.lua")
        local app_xmake_content = io.readfile(app_xmake)
        local includes_line = 'includes("bridges/' .. slug .. '/xmake.lua")'
        if not app_xmake_content:find(includes_line, 1, true) then
            -- Insert after the last bridges includes line (before platform-specific block)
            app_xmake_content = app_xmake_content:gsub(
                '(includes%("bridges/todos/xmake%.lua"%))',
                '%1\n' .. includes_line)
            io.writefile(app_xmake, app_xmake_content)
        end

        -- ── 5. Add dep to desktop/xmake.lua ─────────────────────
        local desktop_xmake = path.join(root, "desktop", "xmake.lua")
        local desktop_content = io.readfile(desktop_xmake)
        if not desktop_content:find(target_name, 1, true) then
            desktop_content = desktop_content:gsub(
                '(add_deps%("app%.bridges%.system")',
                '%1, "' .. target_name .. '"')
            io.writefile(desktop_xmake, desktop_content)
        end

        -- ── 6. Add dep to dev-server xmake.lua ──────────────────
        local devserver_xmake = path.join(root, "tests", "helpers", "dev-server", "xmake.lua")
        local devserver_content = io.readfile(devserver_xmake)
        if not devserver_content:find(target_name, 1, true) then
            devserver_content = devserver_content:gsub(
                '(add_deps%("app%.bridges%.system")',
                '%1, "' .. target_name .. '"')
            io.writefile(devserver_xmake, devserver_content)
        end

        -- ── 7. Wire into desktop main.cpp ───────────────────────
        local main_cpp = path.join(root, "desktop", "src", "main.cpp")
        local main_content = io.readfile(main_cpp)
        if not main_content:find(file_name, 1, true) then
            -- Insert #include after the last bridge include
            main_content = main_content:gsub(
                '(#include "todo_bridge%.hpp")',
                '%1\n#include "' .. file_name .. '.hpp"')
            -- Insert addBridge after the last addBridge call
            main_content = main_content:gsub(
                '(app%.addBridge<SystemBridge>%("system"%)%;)',
                '%1\n    app.addBridge<' .. class_name .. '>("' .. slug .. '");')
            io.writefile(main_cpp, main_content)
        end

        -- ── 8. Wire into test_server.cpp ────────────────────────
        local test_cpp = path.join(root, "tests", "helpers", "dev-server", "src", "test_server.cpp")
        local test_content = io.readfile(test_cpp)
        if not test_content:find(file_name, 1, true) then
            -- Insert #include after the last bridge include
            test_content = test_content:gsub(
                '(#include "todo_bridge%.hpp")',
                '%1\n#include "' .. file_name .. '.hpp"')
            -- Insert registry.add after the last registry.add call
            test_content = test_content:gsub(
                '(registry%.add%("system", new SystemBridge%)%;)',
                '%1\n    registry.add("' .. slug .. '", new ' .. class_name .. ');')
            io.writefile(test_cpp, test_content)
        end

        -- ── 9. Create TS interface stub ─────────────────────────
        local ts_path = path.join(root, "web", "packages", "bridge", "lib", "bridges", slug .. "-bridge.ts")
        io.writefile(ts_path,
            "import { getBridge } from '../transport/bridge'\n\n"
            .. "// TypeScript interface for the " .. class_name .. " C++ bridge.\n\n"
            .. "// export interface GetDataRequest {\n"
            .. "//   id: string\n"
            .. "// }\n\n"
            .. "// export interface GetDataResponse {\n"
            .. "//   name: string\n"
            .. "//   count: number\n"
            .. "// }\n\n"
            .. "export interface " .. class_name .. " {\n"
            .. "  // getData(req: GetDataRequest): Promise<GetDataResponse>\n"
            .. "}\n\n"
            .. "export async function get" .. class_name .. "(): Promise<" .. class_name .. "> {\n"
            .. "  return getBridge<" .. class_name .. ">('" .. slug .. "')\n"
            .. "}\n")

        print("")
        print("✅ Scaffolded bridge: " .. class_name)
        print("")
        print("   app/bridges/" .. slug .. "/include/" .. dtos_name .. ".hpp    ← Request/response DTOs")
        print("   app/bridges/" .. slug .. "/include/" .. file_name .. ".hpp    ← Bridge class")
        print("   app/bridges/" .. slug .. "/xmake.lua                         ← Build target")
        print("   app/web/packages/bridge/lib/bridges/" .. slug .. "-bridge.ts  ← TS interface")
        print("")
        print("   Wired into:")
        print("     desktop/src/main.cpp          (app.addBridge<" .. class_name .. ">)")
        print("     tests/helpers/dev-server/     (registry.add)")
        print("     desktop/xmake.lua             (add_deps)")
        print("     tests/helpers/dev-server/     (add_deps)")
        print("     app/xmake.lua                 (includes)")
        print("")
        print("Next:")
        print("  1. Define request/response structs in the DTOs header")
        print("  2. Write methods on the bridge class, register with method()")
        print("  3. Mirror method signatures in the TS interface")
        print("  4. xmake build desktop")
    end)
