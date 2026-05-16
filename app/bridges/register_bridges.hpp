// register_bridges — single registration point for all app bridges.
//
// Called from both the desktop app (app.cpp) and the test server
// (test_server.cpp) so they always have the same set of bridges.

#pragma once

#include "bridge_registry.hpp"
#include "todo_bridge.hpp"
#include "system_bridge.hpp"

inline void register_bridges(app_shell::BridgeRegistry& registry) {
    // @scaffold:bridge
    registry.add("todos", new TodoBridge);
    registry.add("system", new SystemBridge);
}
