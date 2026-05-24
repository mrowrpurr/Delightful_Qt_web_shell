-- The consumer's desktop app — your starting point.
-- Add bridges, subsystems, and web apps as you grow.

local _APP_NAME    = APP_NAME
local _TEMPLATE_ROOT = TEMPLATE_ROOT
local _APP_SLUG    = APP_SLUG
local _APP_ORG     = APP_ORG
local _APP_VERSION = APP_VERSION

-- Web apps to embed in this binary.
-- The port is the Vite dev server port — must match vite.config.ts.
local WEB_APPS = {
    {name = "app", port = 5175},
}

target("app.desktop")
    set_kind("binary")
    add_rules("qt.widgetapp")
    add_deps("app.framework", "app.bridge.system")
    add_files("src/**.cpp")
    add_files(
        "web_dist_resources.cpp",
        path.join(os.projectdir(), "app/framework/styles/styles_resources.cpp")
    )
    add_defines('APP_NAME="' .. APP_NAME:gsub('"', '\\"') .. '"')
    add_defines('APP_SLUG="' .. APP_SLUG:gsub('"', '\\"') .. '"')
    add_defines('APP_ORG="' .. APP_ORG:gsub('"', '\\"') .. '"')
    add_defines('APP_VERSION="' .. APP_VERSION:gsub('"', '\\"') .. '"')
    for _, wa in ipairs(WEB_APPS) do
        add_defines("WEB_APP_DEV_PORT_" .. wa.name:upper() .. "=" .. wa.port)
    end
    if is_plat("windows") then
        set_filename(APP_NAME .. ".exe")
    elseif is_plat("macosx") then
        set_filename(APP_NAME)
    else
        set_filename(APP_SLUG)
    end
    
    before_build(function(target)
        import("build_helpers").compile_styles_qrc(target, _TEMPLATE_ROOT)

        -- ── Build web apps ───────────────────────────────────────
        local base = os.scriptdir()
        local web_dir = path.join(_TEMPLATE_ROOT, "web")
        local qrc_path = path.join(base, "web_dist.qrc")
        local cpp_path = path.join(base, "web_dist_resources.cpp")

        local skip_vite = os.getenv("SKIP_VITE") == "1" and os.isfile(cpp_path)
        if skip_vite then
            print("⚡ SKIP_VITE=1 — skipping Vite build (using existing web bundle)")
        else
            if os.getenv("SKIP_VITE") == "1" then
                print("⚠️  SKIP_VITE=1 but no previous web build found — building anyway")
            end

            os.setenv("VITE_APP_NAME", _APP_NAME)
            local all_qrc_lines = {'<RCC>'}

            os.execv("bun", {"install"}, {curdir = web_dir})

            for _, wa in ipairs(WEB_APPS) do
                local app_name = wa.name
                local app_dir = path.join(web_dir, "apps", app_name)
                local dist_dir = path.join(app_dir, "dist")

                os.execv("bun", {"run", "build:" .. app_name}, {curdir = web_dir})

                table.insert(all_qrc_lines, '    <qresource prefix="/web-' .. app_name .. '">')
                for _, f in ipairs(os.files(path.join(dist_dir, "**"))) do
                    local rel = path.relative(f, dist_dir):gsub("\\", "/")
                    local rel_from_qrc = path.relative(f, base):gsub("\\", "/")
                    table.insert(all_qrc_lines, '        <file alias="' .. rel .. '">' .. rel_from_qrc .. '</file>')
                end
                table.insert(all_qrc_lines, '    </qresource>')
            end

            table.insert(all_qrc_lines, '</RCC>')
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
    end)
