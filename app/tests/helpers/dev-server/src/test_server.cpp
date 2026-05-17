// Headless WebSocket server — exposes bridges without a GUI.
// Used for dev mode (browser + C++ backend) and Playwright e2e tests.
//
// Usage: dev-server [--port 9876]

#include <QCoreApplication>
#include <QCommandLineParser>

#include "system_bridge.hpp"
#include "theme_bridge.hpp"
#include "todo_bridge.hpp"
#include "expose_as_ws.hpp"
#include "ready_signal.hpp"
#include "bridge_registry.hpp"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("dev-server");

    QCommandLineParser parser;
    parser.addOption({{"p", "port"}, "WebSocket port", "port", "9876"});
    parser.process(app);

    int port = parser.value("port").toInt();

    app_shell::BridgeRegistry registry;
    ReadySignal lifecycle;
    registry.add("todos", new TodoBridge);
    registry.add("system", new SystemBridge);

    auto* themeBridge = new ThemeBridge;
    registry.add("theme", themeBridge);
    // Self-wire: qtThemeRequested echoes back as qtThemeChanged so the full
    // signal round-trip is testable without a StyleManager.
    themeBridge->on_signal("qtThemeRequested", [themeBridge](const nlohmann::json& data) {
        auto displayName = data["displayName"].get<std::string>();
        bool isDark = data["isDark"].get<bool>();
        themeBridge->updateQtThemeState(displayName, isDark);
    });

    auto* server = expose_as_ws(&registry, &lifecycle, port);
    if (!server) return 1;

    return app.exec();
}
