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

#include <QCloseEvent>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWebEngineView>

#include "dialogs/web_dialog.hpp"
#include "system_bridge.hpp"
#include "web_shell.hpp"

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
    buildToolBar(this, actions);

    // ── Status bar ───────────────────────────────────────────
    statusBar_ = new StatusBar(this);
    setStatusBar(statusBar_);

    // ── Central widget — QSplitter with two web apps ─────────
    // Left: main app (todo UI), Right: docs app.
    // Both share the same bridges — one source of truth, signals everywhere.
    auto* app = qobject_cast<Application*>(qApp);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(true);

    mainApp_ = new WebShellWidget(
        app->webProfile(), app->shell(), app->appUrl("main"),
        WebShellWidget::FullOverlay, splitter);

    docsApp_ = new WebShellWidget(
        app->webProfile(), app->shell(), app->appUrl("docs"),
        WebShellWidget::SpinnerOverlay, splitter);

    // Give the main app 2/3 of the space, docs 1/3
    splitter->setSizes({600, 300});
    setCentralWidget(splitter);

    // ── Wire zoom actions to the main web view ───────────────
    auto* view = mainApp_->view();

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
    connect(actions.devTools, &QAction::triggered, mainApp_, [this]() {
        mainApp_->toggleDevTools();
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

    // ── Wire React → native dialog ──────────────────────────
    // When React calls system.openDialog(), the bridge emits a signal.
    // We connect it here to open a WebDialog — React triggers Qt UI.
    auto* systemBridge = qobject_cast<SystemBridge*>(
        app->shell()->bridges().value("system"));
    if (systemBridge) {
        connect(systemBridge, &SystemBridge::openDialogRequested, this, [this]() {
            // Deferred — the bridge method must return before we block with exec().
            // Without this, the QWebChannel can't send the openDialog() response
            // back to React, and the dialog's own QWebChannel may not initialize.
            QTimer::singleShot(0, this, [this]() {
                WebDialog dlg(this);
                dlg.exec();
            });
        });
    }

    // ── Save state on exit ───────────────────────────────────
    connect(qApp, &QApplication::aboutToQuit, this, [this, view]() {
        QSettings s(APP_ORG, APP_SLUG);
        s.setValue("window/geometry", saveGeometry());
        s.setValue("window/zoomFactor", view->zoomFactor());
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Minimize to system tray instead of quitting.
    // The app stays running — quit via File > Quit, Ctrl+Q, or tray > Quit.
    //
    // To disable close-to-tray: remove this override. The default behavior
    // will close the window and quit the app (since it's the last window).
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        hide();
        event->ignore();  // don't close — just hide
    } else {
        // No system tray — fall back to default (quit)
        QMainWindow::closeEvent(event);
    }
}
