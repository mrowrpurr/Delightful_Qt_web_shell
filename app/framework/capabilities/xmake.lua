target("app.framework.capabilities")
    set_kind("static")
    add_rules("qt.static")
    add_includedirs("app", "window", {public = true})
    -- Capabilities reference App (shell/app.hpp) which still lives in
    -- the desktop src tree. This dependency inverts in Phase 6 when App
    -- moves to framework/shell/.
    add_includedirs("../../desktop/src")
    add_deps("app.framework.bridge-registry")
    add_files("app/*.cpp", "app/*.hpp", "window/*.hpp")
    remove_files("app/url_protocol_windows.cpp")
    if is_plat("windows") then
        add_files("app/url_protocol_windows.cpp")
    elseif is_plat("macosx") then
        add_files("app/url_protocol_macos.mm")
        add_frameworks("CoreServices", "Foundation")
    end
    add_frameworks("QtCore", "QtGui", "QtWidgets", "QtNetwork", "QtWebEngineCore", "QtWebEngineWidgets", {public = true})
    add_packages("qlementine-icons")
    -- Consumer identity — flows from the top-level xmake.lua APP_* variables.
    add_defines('APP_NAME="' .. APP_NAME:gsub('"', '\\"') .. '"')
    add_defines('APP_SLUG="' .. APP_SLUG:gsub('"', '\\"') .. '"')
    add_defines('APP_ORG="' .. APP_ORG:gsub('"', '\\"') .. '"')
    add_defines('APP_VERSION="' .. APP_VERSION:gsub('"', '\\"') .. '"')
