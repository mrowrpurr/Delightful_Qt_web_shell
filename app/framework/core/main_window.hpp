// MainWindow — the standard preset window.
//
// Inherits MainWindowBase (bare) and installs the core subsystems:
// status bar, tabified docks, persisted geometry, reactive title.
//
// Does NOT build menus or toolbars — consumers (or subclasses) own those.
// Framework capabilities (ZoomActions, DockActions, DevToolsShortcut)
// are constructed by the consumer and placed into whatever menus they choose.
//
// Consumers who want different behavior can inherit MainWindowBase directly
// and compose their own setup. This class is a recipe, not a requirement.

#pragma once

#include "main_window_base.hpp"

#include <QList>
#include <QUrl>

class DockTabManager;
class QDockWidget;
class QTabBar;
class QWebEngineView;
class ReactiveTitle;
class StatusBar;

class MainWindow : public MainWindowBase {
    Q_OBJECT

public:
    // windowId: if non-empty, restore this window's geometry from settings.
    // Empty = fresh window with default geometry.
    explicit MainWindow(app_shell::App& app, const QString& windowId = {},
                        QWidget* parent = nullptr);

    // Add a dock to this window's UI. Called by DockManager after creating the dock.
    void addDock(QDockWidget* dock);

    // Remove a dock from this window's UI tracking (does not delete it).
    void removeDock(QDockWidget* dock);

    // Docks currently hosted in this window.
    const QList<QDockWidget*>& docks() const { return docks_; }

    // The currently active dock (the one zoom/devtools/close-tab target).
    QDockWidget* activeDock() const { return activeDock_; }

    // This window's unique ID (used as key in settings).
    QString windowId() const { return objectName(); }

signals:
    // Emitted when the active dock changes — capabilities connect to this
    // to rewire their actions to the new dock's content.
    void activeDockChanged(QDockWidget* dock);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QWebEngineView* activeView() const;
    void setActiveDock(QDockWidget* dock);
    void wireTabBar();

    // Resolve a tab index to its QDockWidget without string matching.
    QDockWidget* dockForTab(QTabBar* tabBar, int index) const;

    QList<QDockWidget*> docks_;
    QDockWidget* activeDock_ = nullptr;
    StatusBar* statusBar_ = nullptr;
    DockTabManager* tabManager_ = nullptr;
    ReactiveTitle* reactiveTitle_ = nullptr;
};
