// BridgeRegistry — pure C++ map of named bridges.
//
// Holds the master list of registered bridges. Transports (qt-transport's
// QWebChannel adapter, qt-transport's WebSocket exposer, wasm-transport's
// Embind wrapper) read from this to wire bridges to their respective
// channels. No Qt, no Embind — just an std::map.

#pragma once

#include <map>
#include <string>
#include <typeindex>

#include "bridge.hpp"

namespace app_shell {

class BridgeRegistry {
    std::map<std::string, Bridge*> bridges_;
    std::map<std::type_index, Bridge*> typed_;

public:
    // Register a bridge by name (wire protocol) and type (compile-time retrieval).
    template<typename T>
    void add(const std::string& name, T* bridge) {
        bridges_[name] = bridge;
        typed_[std::type_index(typeid(T))] = bridge;
    }

    // Retrieve by type — no casting, no string lookup.
    template<typename T>
    T* get() const {
        auto it = typed_.find(std::type_index(typeid(T)));
        return it == typed_.end() ? nullptr : static_cast<T*>(it->second);
    }

    // Retrieve by name (wire protocol — used by transports).
    Bridge* get(const std::string& name) const {
        auto it = bridges_.find(name);
        return it == bridges_.end() ? nullptr : it->second;
    }

    const std::map<std::string, Bridge*>& all() const { return bridges_; }
};

} // namespace app_shell
