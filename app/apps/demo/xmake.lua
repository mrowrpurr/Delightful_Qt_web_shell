-- Capture at parse time — globals aren't available inside before_build closures
local _APP_NAME    = APP_NAME
local _TEMPLATE_ROOT = TEMPLATE_ROOT
local _APP_SLUG    = APP_SLUG
local _APP_ORG     = APP_ORG
local _APP_VERSION = APP_VERSION


-- ── Web apps to build and embed ──────────────────────────────────────
-- Each entry becomes a separate Vite build + Qt resource bundle.
-- The port is the Vite dev server port — must match vite.config.ts.
-- Add/remove entries here to control which web apps ship in the binary.
local WEB_APPS = {
    {name = "demo",     port = 5173},
    {name = "settings", port = 5174},
    {name = "app",      port = 5175},
}

target("demo")
    set_kind("binary")
    add_rules("qt.widgetapp")
    add_deps("app-shell", "app.bridges.system", "app.bridges.todos")
    add_files("src/**.cpp", "src/**.hpp")
    add_files(
        "resources/resources.qrc",
        "web_dist_resources.cpp",
        path.join(os.projectdir(), "app/framework/styles/styles_resources.cpp")
    )
    add_includedirs("src")
    add_defines('APP_NAME="' .. APP_NAME:gsub('"', '\\"') .. '"')
    add_defines('APP_SLUG="' .. APP_SLUG:gsub('"', '\\"') .. '"')
    add_defines('APP_ORG="' .. APP_ORG:gsub('"', '\\"') .. '"')
    add_defines('APP_VERSION="' .. APP_VERSION:gsub('"', '\\"') .. '"')
    for _, wa in ipairs(WEB_APPS) do
        add_defines("WEB_APP_DEV_PORT_" .. wa.name:upper() .. "=" .. wa.port)
    end
    if is_plat("windows") then
        set_filename(APP_SLUG .. "-demo.exe")
        add_files("resources/app.rc")
    elseif is_plat("macosx") then
        set_filename(APP_SLUG .. "-demo")
    else
        set_filename(APP_SLUG .. "-demo")
    end

    before_build(function(target)
        import("build_helpers").compile_styles_qrc(target, _TEMPLATE_ROOT)

        -- ── Build web apps ───────────────────────────────────────
        local base = os.scriptdir()
        local web_dir = path.join(_TEMPLATE_ROOT, "web")
        local qrc_path = path.join(base, "web_dist.qrc")
        local cpp_path = path.join(base, "web_dist_resources.cpp")

        -- ── Skip Vite build when SKIP_VITE=1 ──────────────────────
        -- Use this when iterating on C++ only — saves ~30s per build.
        -- Requires a previous Vite build (web_dist_resources.cpp must exist).
        local skip_vite = os.getenv("SKIP_VITE") == "1" and os.isfile(cpp_path)
        if skip_vite then
            print("⚡ SKIP_VITE=1 — skipping Vite build (using existing web bundle)")
        else
            if os.getenv("SKIP_VITE") == "1" then
                print("⚠️  SKIP_VITE=1 but no previous web build found — building anyway")
            end

            -- Pass APP_NAME to Vite so index.html and React can use it
            os.setenv("VITE_APP_NAME", _APP_NAME)

            -- ── Build each web app ───────────────────────────────────
            -- Each app lives in web/apps/<name>/ with its own vite.config.ts.
            -- Always rebuild — Vite is fast (~3s) and stamp files are a footgun.
            -- Vite can import files from anywhere (?raw imports from root, docs/, etc.)
            -- so there's no reliable way to detect "nothing changed" without Vite itself.
            local all_qrc_lines = {'<RCC>'}

            os.execv("bun", {"install"}, {curdir = web_dir})

            for _, wa in ipairs(WEB_APPS) do
                local app_name = wa.name
                local app_dir = path.join(web_dir, "apps", app_name)
                local dist_dir = path.join(app_dir, "dist")

                os.execv("bun", {"run", "build:" .. app_name}, {curdir = web_dir})

                -- Add this app's dist files to the qrc with prefix /web-<name>
                table.insert(all_qrc_lines, '    <qresource prefix="/web-' .. app_name .. '">')
                for _, f in ipairs(os.files(path.join(dist_dir, "**"))) do
                    local rel = path.relative(f, dist_dir):gsub("\\", "/")
                    local rel_from_qrc = path.relative(f, base):gsub("\\", "/")
                    table.insert(all_qrc_lines, '        <file alias="' .. rel .. '">' .. rel_from_qrc .. '</file>')
                end
                table.insert(all_qrc_lines, '    </qresource>')
            end

            table.insert(all_qrc_lines, '</RCC>')

            -- Write a single qrc containing all web apps
            io.writefile(qrc_path, table.concat(all_qrc_lines, "\n") .. "\n")

            local qt_dir = target:data("qt.dir") or get_config("qt")
            local rcc
            if is_host("windows") then
                rcc = path.join(qt_dir, "bin", "rcc.exe")
            else
                rcc = path.join(qt_dir, "libexec", "rcc")
                if not os.isfile(rcc) then rcc = path.join(qt_dir, "bin", "rcc") end
            end
            os.runv(rcc, {"-o", cpp_path, qrc_path})
        end

        -- Generate Windows resource file (app.rc) from APP_NAME/APP_SLUG
        if is_plat("windows") then
            local version = _APP_VERSION
            local version_parts = version:split("%.")
            local v1 = version_parts[1] or "0"
            local v2 = version_parts[2] or "0"
            local v3 = version_parts[3] or "0"
            local rc_content = '// Auto-generated by xmake — do not edit manually\n'
                .. '#include <windows.h>\n\n'
                .. 'IDI_ICON1 ICON "icon.ico"\n\n'
                .. 'VS_VERSION_INFO VERSIONINFO\n'
                .. '    FILEVERSION    ' .. v1 .. ',' .. v2 .. ',' .. v3 .. ',0\n'
                .. '    PRODUCTVERSION ' .. v1 .. ',' .. v2 .. ',' .. v3 .. ',0\n'
                .. '    FILEFLAGSMASK  VS_FFI_FILEFLAGSMASK\n'
                .. '    FILEFLAGS      0\n'
                .. '    FILEOS         VOS_NT_WINDOWS32\n'
                .. '    FILETYPE       VFT_APP\n'
                .. '    FILESUBTYPE    VFT2_UNKNOWN\n'
                .. 'BEGIN\n'
                .. '    BLOCK "StringFileInfo"\n'
                .. '    BEGIN\n'
                .. '        BLOCK "040904B0"\n'
                .. '        BEGIN\n'
                .. '            VALUE "CompanyName",      "' .. _APP_ORG .. '"\n'
                .. '            VALUE "FileDescription",  "' .. _APP_NAME .. '"\n'
                .. '            VALUE "FileVersion",      "' .. version .. '"\n'
                .. '            VALUE "InternalName",     "' .. _APP_SLUG .. '"\n'
                .. '            VALUE "OriginalFilename", "' .. _APP_NAME .. '.exe"\n'
                .. '            VALUE "ProductName",      "' .. _APP_NAME .. '"\n'
                .. '            VALUE "ProductVersion",   "' .. version .. '"\n'
                .. '        END\n'
                .. '    END\n'
                .. '    BLOCK "VarFileInfo"\n'
                .. '    BEGIN\n'
                .. '        VALUE "Translation", 0x0409, 0x04B0\n'
                .. '    END\n'
                .. 'END\n'
            io.writefile(path.join(base, "resources", "app.rc"), rc_content)
        end
    end)

    -- Write the binary path so Playwright desktop tests can find and launch it.
    -- Also inject URL protocol declaration into the macOS .app's Info.plist —
    -- without it, LaunchServices can't route <slug>:// URLs to this bundle.
    after_build(function(target)
        local abs_exe = path.absolute(target:targetfile())
        -- Write to template root so tests (which run from there) can find it
        os.mkdir(path.join(_TEMPLATE_ROOT, "build"))
        io.writefile(path.join(_TEMPLATE_ROOT, "build", ".desktop-binary.txt"), abs_exe)

        if is_plat("macosx") then
            local plist_path = path.join(target:targetdir(),
                                          target:basename() .. ".app",
                                          "Contents", "Info.plist")
            if os.isfile(plist_path) then
                local scheme = _APP_SLUG:lower()
                local bundle_id = _APP_ORG .. "." .. _APP_SLUG
                -- plutil -replace removes any existing key first, so this is idempotent.
                local url_types_json = string.format(
                    [=[[{"CFBundleURLName":"%s","CFBundleURLSchemes":["%s"]}]]=],
                    bundle_id, scheme)
                os.vrunv("plutil", {"-replace", "CFBundleURLTypes",
                                     "-json", url_types_json, plist_path})
                os.vrunv("plutil", {"-replace", "CFBundleIdentifier",
                                     "-string", bundle_id, plist_path})
                print("✅ Injected CFBundleURLTypes (" .. scheme .. ") + CFBundleIdentifier ("
                      .. bundle_id .. ") into " .. plist_path)
            else
                print("⚠️  Info.plist not found at " .. plist_path
                      .. " — URL protocol will not work until plist is fixed")
            end
        end
    end)
