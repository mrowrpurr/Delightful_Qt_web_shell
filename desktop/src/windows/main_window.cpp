// MainWindow — wires together menu bar, tool bar, status bar, and central widget.
//
// This file should stay short. If you're adding logic here, ask yourself:
//   - App-level concern? → Application
//   - Menu/toolbar action? → menus/menu_bar.cpp
//   - Reusable widget? → widgets/
//   - Business logic? → lib/

#include "main_window.hpp"
#include "application.hpp"
#include "menus/menu_bar.hpp"
#include "widgets/status_bar.hpp"
#include "widgets/web_shell_widget.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(APP_NAME);
    resize(900, 640);

    // ── Menu bar + toolbar ───────────────────────────────────
    buildMenuBar(this);
    buildToolBar(this);

    // ── Status bar ───────────────────────────────────────────
    auto* status = new StatusBar(this);
    setStatusBar(status);

    // ── Central widget — React app with bridges ──────────────
    auto* app = qobject_cast<Application*>(qApp);
    webShell_ = new WebShellWidget(
        app->webProfile(), app->shell(), app->devMode(), this);
    setCentralWidget(webShell_);
}
