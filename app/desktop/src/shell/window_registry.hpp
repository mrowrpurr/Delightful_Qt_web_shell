// app_shell::WindowRegistry — host for app-wide window lifecycle concerns.
//
// Constructed by the consumer as a child of App. Owns:
//   - restoreWindows(): walks QSettings for saved window IDs and constructs
//     a MainWindow for each (was DockManager::restoreWindows).
//   - visibleWindowCount(): scans top-level widgets for visible MainWindow
//     instances — used by MainWindow::closeEvent to decide hide-to-tray vs
//     close (was an inline iteration inside MainWindow::closeEvent).
//
// Consumers who skip constructing WindowRegistry lose multi-window restore
// and lose the "last visible window hides to tray" behavior — every close
// is a plain close.

#pragma once

#include <QList>
#include <QObject>

class MainWindow;

namespace app_shell {

class App;

class WindowRegistry : public QObject {
    Q_OBJECT

public:
    explicit WindowRegistry(App* parent);

    // Reads saved window IDs from QSettings (under window/<id>) and
    // constructs one MainWindow per saved ID. Returns the list in saved
    // order. Empty list when no windows were saved.
    QList<MainWindow*> restoreWindows();

    // Counts MainWindow instances that are currently visible across all
    // top-level widgets. Used by MainWindow::closeEvent.
    int visibleWindowCount() const;

private:
    App* app_;
};

}  // namespace app_shell
