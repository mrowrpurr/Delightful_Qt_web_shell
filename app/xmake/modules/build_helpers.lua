function compile_styles_qrc(target)
    local styles_dir = path.join(TEMPLATE_ROOT, "framework", "styles")
    local compiled_dir = path.join(styles_dir, "compiled")
    local qrc_path = path.join(styles_dir, "styles.qrc")
    local cpp_path = path.join(styles_dir, "styles_resources.cpp")

    local lines = {'<RCC>', '    <qresource prefix="/styles">'}
    for _, f in ipairs(os.files(path.join(compiled_dir, "*.qss"))) do
        local name = path.filename(f)
        local abs = path.absolute(f):gsub("\\", "/")
        table.insert(lines, '        <file alias="' .. name .. '">' .. abs .. '</file>')
    end
    local names_json = path.join(compiled_dir, "theme-names.json")
    if os.isfile(names_json) then
        local abs = path.absolute(names_json):gsub("\\", "/")
        table.insert(lines, '        <file alias="theme-names.json">' .. abs .. '</file>')
    end
    table.insert(lines, '    </qresource>')
    table.insert(lines, '</RCC>')
    io.writefile(qrc_path, table.concat(lines, "\n") .. "\n")

    local qt_dir = target:data("qt.dir") or get_config("qt")
    local rcc
    if is_host("windows") then
        rcc = path.join(qt_dir, "bin", "rcc.exe")
    else
        rcc = path.join(qt_dir, "libexec", "rcc")
        if not os.isfile(rcc) then rcc = path.join(qt_dir, "bin", "rcc") end
    end
    os.runv(rcc, {"--name", "styles", "-o", cpp_path, qrc_path})
end
