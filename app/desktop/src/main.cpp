// Qt desktop shell — the entry point.
// Everything interesting lives in the classes this file wires together.

#include "dock_manager.hpp"
#include "logging.hpp"
#include "shell/app.hpp"
#include "shell/single_instance.hpp"
#include "shell/tray.hpp"
#include "shell/url_protocol.hpp"
#include "shell/window_lifecycle.hpp"
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

    app_shell::App app(argc, argv);

    // ── Single-instance guard ───────────────────────────────────────────
    // If another instance is already running, the ctor forwards this
    // process's args / activation to it and isPrimary() returns false —
    // we exit cleanly and the user sees the existing window raise.
    auto* singleInstance = new app_shell::SingleInstance(&app);
    if (!singleInstance->isPrimary()) return 0;

    // ── URL protocol ────────────────────────────────────────────────────
    // Lets users open the app from a browser via <slug>:// links. The prompt
    // shows once on first launch unless the user opted out previously.
    auto* urlProtocol = new app_shell::UrlProtocol(&app);
    urlProtocol->promptIfNeeded();

    // Restore saved windows, or create one default window.
    auto* windowLifecycle = new app_shell::WindowLifecycle(&app);
    auto windows = windowLifecycle->restoreWindows();
    if (windows.isEmpty())
        windows.append(new MainWindow(app));

    // Shared "raise / activate the visible main window" handler — invoked
    // by both SingleInstance::activationRequested (second-instance launch)
    // and Tray::activated (tray-icon click or Show Window menu item).
    //
    // Reads from QApplication::topLevelWidgets() rather than capturing the
    // startup `windows` list — that list contains raw pointers that go
    // dangling as MainWindows are deleted on close. Qt manages the
    // top-level widget list itself, so this stays correct as windows
    // come and go.
    auto raise = []() {
        MainWindow* fallback = nullptr;
        for (auto* w : QApplication::topLevelWidgets()) {
            auto* mw = qobject_cast<MainWindow*>(w);
            if (!mw) continue;
            if (mw->isVisible()) {
                mw->raise();
                mw->activateWindow();
                return;
            }
            if (!fallback) fallback = mw;
        }
        if (fallback) {
            fallback->show();
            fallback->raise();
            fallback->activateWindow();
        }
    };

    QObject::connect(singleInstance, &app_shell::SingleInstance::activationRequested,
                     windows.first(), raise);

    // Forward args to the SystemBridge so React can see them.
    // Handles: first launch args, second-instance args, and URL protocol activations.
    auto* systemBridge = static_cast<SystemBridge*>(
        app.registry()->get("system"));
    if (systemBridge) {
        QObject::connect(singleInstance, &app_shell::SingleInstance::argsReceived,
                         &app, [systemBridge](const QStringList& args) {
            systemBridge->handleAppLaunchArgs(args);
        });

        // Pass the primary instance's own args on first launch
        QStringList args = app.arguments().mid(1);
        if (!args.isEmpty())
            systemBridge->handleAppLaunchArgs(args);
    }

    // ── System tray ─────────────────────────────────────────────────────
    // The framework's Tray subsystem is a thin QObject wrapper around
    // QSystemTrayIcon + a top-level QMenu. Everything below is demo content
    // — replace with your own when forking this template.
    auto* tray = new app_shell::Tray(&app);
    QObject::connect(tray, &app_shell::Tray::activated, &app, raise);

    tray->addItem("&Show Window", raise);
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
