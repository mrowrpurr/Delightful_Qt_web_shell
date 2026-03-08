target("qt-web-shell-example")
    set_kind("binary")
    set_filename("Qt Web Shell Example.exe")
    add_rules("qt.widgetapp")
    if is_plat("windows") then
        set_runtimes("MD")
    end
    add_files(
        "cpp/main.cpp",
        "cpp/bridge.hpp",
        "cpp/resources/resources.qrc",
        "cpp/web_dist_resources.cpp"
    )
    add_frameworks(
        "QtWidgets", "QtCore", "QtGui",
        "QtWebEngineCore", "QtWebEngineWidgets", "QtWebChannel"
    )

    before_build(function(target)
        local base = os.scriptdir()
        local web_dir = path.join(base, "web")
        local dist_dir = path.join(web_dir, "dist")

        -- 1. Build the web app
        os.execv("bun", {"install"}, {curdir = web_dir})
        os.execv("bun", {"run", "build"}, {curdir = web_dir})

        -- 2. Generate a .qrc listing every file in dist/
        local qrc_lines = {'<RCC>', '    <qresource prefix="/web">'}
        for _, f in ipairs(os.files(path.join(dist_dir, "**"))) do
            local rel = path.relative(f, dist_dir):gsub("\\", "/")
            local abs = path.absolute(f):gsub("\\", "/")
            table.insert(qrc_lines, '        <file alias="' .. rel .. '">' .. abs .. '</file>')
        end
        table.insert(qrc_lines, '    </qresource>')
        table.insert(qrc_lines, '</RCC>')

        local qrc_path = path.join(base, "cpp", "web_dist.qrc")
        io.writefile(qrc_path, table.concat(qrc_lines, "\n") .. "\n")

        -- 3. Compile the .qrc into a .cpp via rcc
        local qt_dir = target:data("qt.dir") or get_config("qt")
        local rcc = path.join(qt_dir, "bin", "rcc.exe")
        local cpp_path = path.join(base, "cpp", "web_dist_resources.cpp")
        os.execv(rcc, {"-o", cpp_path, qrc_path})
    end)
