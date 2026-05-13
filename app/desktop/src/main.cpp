// Qt desktop shell — the entry point.
// Everything interesting lives in the classes this file wires together.

#include "application.hpp"
#include "dock_manager.hpp"
#include "logging.hpp"
#include "shell/tray.hpp"
#include "shell/url_protocol.hpp"
#include "system_bridge.hpp"
#include "widgets/scheme_handler.hpp"
#include "windows/main_window.hpp"

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char* argv[]) {
    setupLogging();

    // Custom URL scheme must be registered BEFORE QApplication is constructed.
    // Qt enforces this — it's a hard requirement, not a suggestion.
    SchemeHandler::registerUrlScheme();

    Application app(argc, argv);

    // If another instance is already running, it was signaled to activate.
    // This process exits cleanly — the user sees the existing window raise.
    if (!app.isPrimaryInstance()) return 0;

    // ── URL protocol ────────────────────────────────────────────────────
    // Lets users open the app from a browser via <slug>:// links. The prompt
    // shows once on first launch unless the user opted out previously.
    auto* urlProtocol = new app_shell::UrlProtocol(&app);
    urlProtocol->promptIfNeeded();

    // ── System tray ─────────────────────────────────────────────────────
    // The framework's Tray subsystem is a thin QObject wrapper around
    // QSystemTrayIcon + a top-level QMenu. Everything below is demo content
    // — replace with your own when forking this template.
    auto* tray = new app_shell::Tray(&app);
    QObject::connect(tray, &app_shell::Tray::activated,
                     &app, &Application::activationRequested);

    tray->addItem("&Show Window", [&app]{ app.activationRequested(); });
    tray->addLabel(QString("%1 %2").arg(APP_NAME).arg(APP_VERSION));
    tray->addSeparator();

    // Example Menu 1 — flat actions
    auto* exampleMenu1 = tray->addMenu("Example Menu 1");
    if (exampleMenu1) {
        for (const auto& name : {"Alpha", "Beta", "Gamma"}) {
            auto* action = exampleMenu1->addAction(name);
            QObject::connect(action, &QAction::triggered, &app, [name] {
                QMessageBox::information(nullptr, "Example Menu 1",
                    QString("You clicked: %1").arg(name));
            });
        }
    }

    // Nested Example 2 — submenus
    auto* nestedMenu = tray->addMenu("Nested Example 2");
    if (nestedMenu) {
        auto* topAction = nestedMenu->addAction("Top-Level Action");
        QObject::connect(topAction, &QAction::triggered, &app, [] {
            QMessageBox::information(nullptr, "Nested Example 2",
                "You clicked: Top-Level Action");
        });

        auto* subMenu1 = nestedMenu->addMenu("Sub-Menu");
        auto* subAction1 = subMenu1->addAction("Sub Action");
        QObject::connect(subAction1, &QAction::triggered, &app, [] {
            QMessageBox::information(nullptr, "Sub-Menu",
                "You clicked: Sub Action");
        });

        auto* subMenu2 = subMenu1->addMenu("Deeper Sub-Menu");
        auto* deepAction = subMenu2->addAction("Deep Action");
        QObject::connect(deepAction, &QAction::triggered, &app, [] {
            QMessageBox::information(nullptr, "Deeper Sub-Menu",
                "You clicked: Deep Action");
        });
    }

    tray->addSeparator();
    tray->addItem("&Quit", [&app]{ app.requestQuit(); });
    tray->show();

    // Restore saved windows, or create one default window.
    auto windows = app.dockManager()->restoreWindows();
    if (windows.isEmpty())
        windows.append(new MainWindow(app));

    // When another instance tries to launch, raise any visible MainWindow.
    QObject::connect(&app, &Application::activationRequested, windows.first(), [&windows]() {
        for (auto* w : QApplication::topLevelWidgets()) {
            if (auto* mw = qobject_cast<MainWindow*>(w); mw && mw->isVisible()) {
                mw->raise();
                mw->activateWindow();
                return;
            }
        }
        // No visible windows — show the first one
        if (!windows.isEmpty()) {
            windows.first()->show();
            windows.first()->raise();
            windows.first()->activateWindow();
        }
    });

    // Forward args to the SystemBridge so React can see them.
    // Handles: first launch args, second-instance args, and URL protocol activations.
    auto* systemBridge = static_cast<SystemBridge*>(
        app.registry()->get("system"));
    if (systemBridge) {
        QObject::connect(&app, &Application::appLaunchArgsReceived,
                         &app, [systemBridge](const QStringList& args) {
            systemBridge->handleAppLaunchArgs(args);
        });

        // Pass the primary instance's own args on first launch
        QStringList args = app.arguments().mid(1);
        if (!args.isEmpty())
            systemBridge->handleAppLaunchArgs(args);
    }

    // Show all windows. First one gets the anti-flash treatment.
    for (int i = 0; i < windows.size(); ++i) {
        auto* win = windows[i];
        if (i == 0) {
            win->setWindowOpacity(0.0);
            win->show();
            QTimer::singleShot(0, [win]() { win->setWindowOpacity(1.0); });
        } else {
            win->show();
        }
    }

    return app.exec();
}
