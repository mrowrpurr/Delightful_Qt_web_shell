// register_bridges — registers the same bridges as the desktop app.
//
// Used by test_server.cpp so headless dev/test has the same bridges.
// The desktop app uses app.addBridge<T>("name") directly in main.cpp.

#pragma once

#include "bridge_registry.hpp"
#include "todo_bridge.hpp"
#include "system_bridge.hpp"

inline void register_bridges(app_shell::BridgeRegistry& registry) {
    // @scaffold:bridge
    registry.add("todos", new TodoBridge);
    registry.add("system", new SystemBridge);
}
