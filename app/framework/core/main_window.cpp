// MainWindow — preset window with status bar, tabified docks, and subsystems.
//
// Does NOT build menus or toolbars — consumers (or subclasses) own those.
// Framework capabilities (ZoomActions, DockActions, DevToolsShortcut)
// are constructed by the consumer and placed into whatever menus they choose.

#include "main_window.hpp"
#include "persisted_geometry.hpp"
#include "reactive_title.hpp"
#include "app.hpp"
#include "window_lifecycle.hpp"
#include "dock_manager.hpp"
#include "dock_tab_manager.hpp"
#include "status_bar.hpp"

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QScreen>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTabBar>
#include <QTimer>
#include <QUuid>
#include <QWebEnginePage>
#include <QWebEngineView>

MainWindow::MainWindow(app_shell::App& app, const QString& windowId, QWidget* parent)
    : MainWindowBase(app, parent)
{
    // Assign or generate a UUID for this window.
    if (windowId.isEmpty())
        setObjectName(QUuid::createUuid().toString(QUuid::WithoutBraces));
    else
        setObjectName(windowId);

    setWindowTitle(APP_NAME);

    // ── Window-scoped subsystems ─────────────────────────────
    new PersistedGeometry(this, objectName());
    reactiveTitle_ = new ReactiveTitle(this, APP_NAME);

    // ── Status bar ───────────────────────────────────────────
    statusBar_ = new StatusBar(this);
    setStatusBar(statusBar_);

    // ── Dock-based tab system ────────────────────────────────
    // Hide the central widget — all content lives in docks.
    auto* placeholder = new QWidget(this);
    placeholder->setMaximumSize(0, 0);
    setCentralWidget(placeholder);

    setDockNestingEnabled(true);
    setTabPosition(Qt::TopDockWidgetArea, QTabWidget::North);
    tabManager_ = new DockTabManager(this);

    // ── Docks ─────────────────────────────────────────────────
    auto* dm = app.dockManager();

    if (!windowId.isEmpty())
        dm->restoreDocks(this);

    // If no docks were restored (or this is a fresh window), create a default one.
    if (docks_.isEmpty()) {
        auto* dock = dm->createDock({}, this);
        Q_UNUSED(dock);
    }

    activeDock_ = docks_.first();
}

// ── Dock hosting ─────────────────────────────────────────────

void MainWindow::addDock(QDockWidget* dock) {
    addDockWidget(Qt::TopDockWidgetArea, dock);
    if (!docks_.isEmpty())
        tabifyDockWidget(docks_.first(), dock);

    docks_.append(dock);

    // Event filter for floating dock activation and close detection.
    dock->installEventFilter(this);
    tabManager_->manage(dock);

    // ── Track active dock ────────────────────────────────────
    connect(dock, &QDockWidget::topLevelChanged, this, [this, dock](bool) {
        setActiveDock(dock);
    });

    connect(dock, &QDockWidget::visibilityChanged, this, [this, dock](bool visible) {
        if (visible) setActiveDock(dock);
    });

    // ── Reactive dock title from document.title ──────────────
    reactiveTitle_->wireDock(dock);

    // ── Wire tab bar (deferred — Qt may not have created it yet) ──
    QTimer::singleShot(0, this, [this]() { wireTabBar(); });
}

void MainWindow::removeDock(QDockWidget* dock) {
    tabManager_->unmanage(dock);
    docks_.removeOne(dock);

    if (activeDock_ == dock) {
        activeDock_ = nullptr;  // clear before setActiveDock so the guard doesn't skip
        setActiveDock(docks_.isEmpty() ? nullptr : docks_.last());
    }
}

// ── Tab bar wiring ───────────────────────────────────────────

QDockWidget* MainWindow::dockForTab(QTabBar* tabBar, int index) const {
    if (index < 0 || index >= tabBar->count()) return nullptr;
    quintptr id = tabBar->tabData(index).value<quintptr>();
    if (!id) return nullptr;
    for (auto* dock : docks_)
        if (reinterpret_cast<quintptr>(dock) == id)
            return dock;
    return nullptr;
}

void MainWindow::wireTabBar() {
    auto* dm = app().dockManager();

    for (auto* tabBar : findChildren<QTabBar*>()) {
        if (!tabBar->property("dockWired").toBool()) {
            tabBar->setProperty("dockWired", true);
            tabBar->setTabsClosable(true);
            tabBar->installEventFilter(this);
            tabManager_->manageTabBar(tabBar);

            connect(tabBar, &QTabBar::tabCloseRequested, this, [this, tabBar, dm](int index) {
                if (auto* dock = dockForTab(tabBar, index))
                    dm->closeDock(dock);
            });

            connect(tabBar, &QTabBar::currentChanged, this, [this, tabBar](int index) {
                if (auto* dock = dockForTab(tabBar, index))
                    setActiveDock(dock);
            });
        }
    }
}

// ── Active dock management ────────────────────────────────────

void MainWindow::setActiveDock(QDockWidget* dock) {
    if (activeDock_ == dock) return;
    activeDock_ = dock;

    // ── Status bar zoom display ──────────────────────────────
    auto* view = activeView();
    if (view) {
        auto updateZoom = [this, view]() {
            statusBar_->setZoomLevel(qRound(view->zoomFactor() * 100));
        };
        connect(view->page(), &QWebEnginePage::zoomFactorChanged, this, updateZoom);
        updateZoom();
    }

    emit activeDockChanged(dock);
}

QWebEngineView* MainWindow::activeView() const {
    if (activeDock_ && activeDock_->widget())
        return activeDock_->widget()->findChild<QWebEngineView*>();
    return nullptr;
}

// ── Event handling ───────────────────────────────────────────

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // Floating dock activation — track which dock the user is interacting with.
    if (event->type() == QEvent::WindowActivate) {
        auto* dock = qobject_cast<QDockWidget*>(obj);
        if (dock && dock->isFloating() && docks_.contains(dock))
            setActiveDock(dock);
    }

    // Dock close — user clicked the X on a dock (floating or tabified).
    // Qt's default for the dock title bar X is hide(), not destroy. Without
    // this, tabified docks accumulate forever in the saved layout.
    // Skip during shutdown — shutdownAll handles cleanup.
    if (event->type() == QEvent::Close) {
        auto* dm = app().dockManager();
        auto* dock = qobject_cast<QDockWidget*>(obj);
        if (dock && !dm->isQuitting()
            && docks_.contains(dock) && docks_.size() > 1) {
            QTimer::singleShot(0, this, [this, dm, dock]() {
                if (!dm->isQuitting() && docks_.contains(dock) && docks_.size() > 1)
                    dm->closeDock(dock);
            });
        }
    }

    // Middle-click to close a tabified tab.
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* tabBar = qobject_cast<QTabBar*>(obj);
        if (tabBar) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::MiddleButton) {
                int index = tabBar->tabAt(me->pos());
                if (auto* dock = dockForTab(tabBar, index)) {
                    app().dockManager()->closeDock(dock);
                    return true;
                }
            }
        }
    }

    // Right-click context menu on tabified tabs.
    if (event->type() == QEvent::ContextMenu) {
        auto* tabBar = qobject_cast<QTabBar*>(obj);
        if (tabBar) {
            auto* ce = static_cast<QContextMenuEvent*>(event);
            int index = tabBar->tabAt(ce->pos());
            if (index >= 0) {
                auto* dm = app().dockManager();

                QDockWidget* clickedDock = dockForTab(tabBar, index);
                if (!clickedDock) return QMainWindow::eventFilter(obj, event);

                QMenu menu;
                auto* closeTab = menu.addAction("Close tab");
                auto* closeOthers = menu.addAction("Close other tabs");
                auto* closeRight = menu.addAction("Close to the right");
                auto* closeAll = menu.addAction("Close all");

                // Disabled rules:
                // "Close other tabs" — disabled if this is the only tab.
                closeOthers->setEnabled(tabBar->count() > 1);
                // "Close to the right" — disabled if this is the rightmost tab.
                closeRight->setEnabled(index < tabBar->count() - 1);

                auto* chosen = menu.exec(ce->globalPos());
                if (!chosen) return true;

                if (chosen == closeTab) {
                    dm->closeDock(clickedDock);
                } else if (chosen == closeOthers) {
                    QList<QDockWidget*> toClose;
                    for (int i = 0; i < tabBar->count(); ++i) {
                        if (i == index) continue;
                        if (auto* dock = dockForTab(tabBar, i))
                            toClose.append(dock);
                    }
                    for (auto* dock : toClose)
                        dm->closeDock(dock);
                } else if (chosen == closeRight) {
                    QList<QDockWidget*> toClose;
                    for (int i = index + 1; i < tabBar->count(); ++i) {
                        if (auto* dock = dockForTab(tabBar, i))
                            toClose.append(dock);
                    }
                    for (auto* dock : toClose)
                        dm->closeDock(dock);
                } else if (chosen == closeAll) {
                    QList<QDockWidget*> toClose(docks_);
                    for (auto* dock : toClose)
                        dm->closeDock(dock);
                }

                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
        for (auto* dock : docks_) {
            if (!dock->isFloating() && dock->isVisible() && !dock->visibleRegion().isEmpty()) {
                setActiveDock(dock);
                break;
            }
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // During shutdown, always accept the close — don't hide to tray.
    auto* dm = app().dockManager();
    if (dm->isQuitting()) {
        QMainWindow::closeEvent(event);
        return;
    }

    // Hide-to-tray on the last visible window — but only when WindowLifecycle
    // is present (consumer opted into multi-window lifecycle). Without it,
    // every close is a plain close.
    auto* lifecycle = app().findChild<app_shell::WindowLifecycle*>();
    int visibleCount = lifecycle ? lifecycle->visibleWindowCount() : 0;

    if (lifecycle && visibleCount <= 1 && QSystemTrayIcon::isSystemTrayAvailable()) {
        hide();
        event->ignore();
    } else {
        // Close all docks in this window via DockManager so it
        // cleans up its tracking and settings. Take a copy because
        // closeDock() modifies docks_ via removeDock().
        auto docksToClose = docks_;
        for (auto* dock : docksToClose)
            dm->closeDock(dock);

        // Remove this window's state from settings.
        QSettings s(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);
        s.remove("window/" + objectName());

        QMainWindow::closeEvent(event);
    }
}
