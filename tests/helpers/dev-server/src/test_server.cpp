// Headless WebSocket server — exposes bridges without a GUI.
// Used for dev mode (browser + C++ backend) and Playwright e2e tests.
//
// Usage: dev-server [--port 9876]

#include <QCoreApplication>
#include <QCommandLineParser>

#include "bridge.hpp"
#include "expose_as_ws.hpp"
#include "web_shell.hpp"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("dev-server");

    QCommandLineParser parser;
    parser.addOption({{"p", "port"}, "WebSocket port", "port", "9876"});
    parser.process(app);

    int port = parser.value("port").toInt();

    WebShell shell;
    auto* bridge = new Bridge;
    shell.addBridge("todos", bridge);
    auto* server = expose_as_ws(&shell, port);
    if (!server) return 1;

    return app.exec();
}
