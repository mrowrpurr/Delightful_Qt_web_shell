// DockTabManager — IDE-style dock tab behavior.
//
// Installed on a MainWindow. Coordinates three features:
//   1. Title bar hiding — tabified docks get a 0-height title bar,
//      floating docks get a custom FloatingDockTitleBar, standalone
//      docks get the native Qt title bar.
//   2. Drag-to-undock — dragging a tab outside the tab bar area
//      floats it (like Chrome/VS Code tabs).
//   3. LayoutRequest tracking — catches tab group changes that
//      have no dedicated Qt signal.

#pragma once

#include <QObject>
#include <QPoint>

class QDockWidget;
class QMainWindow;
class QTabBar;

class DockTabManager : public QObject {
    Q_OBJECT

public:
    explicit DockTabManager(QMainWindow* window);

    // Start managing a dock — install filters, connect signals, set initial title bar.
    void manage(QDockWidget* dock);

    // Stop managing a dock — disconnect signals, remove filter.
    void unmanage(QDockWidget* dock);

    // Install this as event filter on a tab bar (for drag-to-undock).
    void manageTabBar(QTabBar* tabBar);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void updateTitleBar(QDockWidget* dock);
    void updateAllTitleBars();
    void scheduleUpdate();

    void hideTitleBar(QDockWidget* dock);
    void showFloatingTitleBar(QDockWidget* dock);
    void showNativeTitleBar(QDockWidget* dock);

    void undockTab(QTabBar* tabBar, int tabIndex);
    void wireNewTabBars();

    QMainWindow* window_;
    bool updateScheduled_ = false;

    // Drag-to-undock state.
    QTabBar* dragTabBar_ = nullptr;
    int dragTabIndex_ = -1;
    QString dragTabTitle_;
};
