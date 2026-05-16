// MainWindow — the standard preset window.
//
// Inherits MainWindowBase (bare) and installs the full set of standard
// subsystems: menu bar, tool bar, status bar, tabified docks.
//
// Consumers who want different behavior can inherit MainWindowBase directly
// and compose their own setup. This class is a recipe, not a requirement.

#pragma once

#include "main_window_base.hpp"

#include <QList>
#include <QUrl>

class DevToolsShortcut;
class DockTabManager;
class QDockWidget;
class QTabBar;
class QWebEngineView;
class ReactiveTitle;
class StatusBar;
struct MenuActions;

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

    // This window's unique ID (used as key in settings).
    QString windowId() const { return objectName(); }

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QWebEngineView* activeView() const;
    void wireToActiveDock();
    void wireTabBar();

    // Resolve a tab index to its QDockWidget without string matching.
    // Qt stamps tabData() with reinterpret_cast<quintptr>(dock) when it builds
    // the tab bar; we compare against our own live pointers in docks_ rather
    // than dereferencing whatever Qt handed us.
    QDockWidget* dockForTab(QTabBar* tabBar, int index) const;

    QList<QDockWidget*> docks_;
    QDockWidget* activeDock_ = nullptr;
    StatusBar* statusBar_ = nullptr;
    MenuActions* actions_ = nullptr;
    DockTabManager* tabManager_ = nullptr;
    DevToolsShortcut* devTools_ = nullptr;
    ReactiveTitle* reactiveTitle_ = nullptr;
};
