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

#include <QScreen>
#include <QSettings>
#include <QWebEngineView>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(APP_NAME);

    // ── Restore geometry or default to 900×640 centered ──────
    QSettings settings(APP_ORG, APP_SLUG);
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
    } else {
        resize(900, 640);
        if (auto* screen = QApplication::primaryScreen()) {
            QRect geo = screen->availableGeometry();
            move((geo.width() - 900) / 2 + geo.x(),
                 (geo.height() - 640) / 2 + geo.y());
        }
    }

    // ── Menu bar + toolbar ───────────────────────────────────
    auto actions = buildMenuBar(this);
    buildToolBar(this);

    // ── Status bar ───────────────────────────────────────────
    statusBar_ = new StatusBar(this);
    setStatusBar(statusBar_);

    // ── Central widget — React app with bridges ──────────────
    auto* app = qobject_cast<Application*>(qApp);
    webShell_ = new WebShellWidget(
        app->webProfile(), app->shell(), app->devMode(),
        WebShellWidget::FullOverlay, this);
    setCentralWidget(webShell_);

    // ── Wire zoom actions to the web view ────────────────────
    auto* view = webShell_->view();

    connect(actions.zoomIn, &QAction::triggered, view, [view]() {
        view->setZoomFactor(qMin(view->zoomFactor() + 0.1, 5.0));
    });
    connect(actions.zoomOut, &QAction::triggered, view, [view]() {
        view->setZoomFactor(qMax(view->zoomFactor() - 0.1, 0.25));
    });
    connect(actions.zoomReset, &QAction::triggered, view, [view]() {
        view->setZoomFactor(1.0);
    });

    // ── Wire F12 to dev tools toggle ─────────────────────────
    connect(actions.devTools, &QAction::triggered, webShell_, [this]() {
        webShell_->toggleDevTools();
    });

    // ── Restore zoom (before content loads, behind overlay) ──
    view->setZoomFactor(settings.value("window/zoomFactor", 1.0).toReal());

    // ── Live zoom % in status bar ────────────────────────────
    // Updates from both keyboard shortcuts and Ctrl+wheel.
    auto updateZoom = [this, view]() {
        statusBar_->setZoomLevel(qRound(view->zoomFactor() * 100));
    };
    connect(view->page(), &QWebEnginePage::zoomFactorChanged, this, updateZoom);
    updateZoom();  // set initial value

    // ── Save state on exit ───────────────────────────────────
    connect(qApp, &QApplication::aboutToQuit, this, [this, view]() {
        QSettings s(APP_ORG, APP_SLUG);
        s.setValue("window/geometry", saveGeometry());
        s.setValue("window/zoomFactor", view->zoomFactor());
    });
}
