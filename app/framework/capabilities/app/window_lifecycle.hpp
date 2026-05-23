// app_shell::WindowLifecycle — host for app-wide window lifecycle concerns.
//
// Constructed by the consumer as a child of App. Two helpers:
//   - restoreWindows<T>(): walks QSettings for saved window IDs and constructs
//     a T (MainWindow subclass) for each.
//   - visibleWindowCount(): scans top-level widgets for visible MainWindow
//     instances — used by MainWindow::closeEvent to decide hide-to-tray vs
//     close.
//
// Consumers who skip constructing WindowLifecycle lose multi-window restore
// and lose the "last visible window hides to tray" behavior — every close
// is a plain close.

#pragma once

#include <QList>
#include <QObject>
#include <QSettings>
#include <QStringList>

#include "app.hpp"

class MainWindow;

namespace app_shell {

class WindowLifecycle : public QObject {
    Q_OBJECT

public:
    explicit WindowLifecycle(App* parent)
        : QObject(parent), app_(parent) {}

    // Reads saved window IDs from QSettings and constructs one T per saved ID.
    // T must be constructible as T(App&, const QString& windowId).
    template<typename T = MainWindow>
    QList<T*> restoreWindows() {
        QSettings s(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);
        s.beginGroup("window");
        QStringList windowIds = s.childGroups();
        s.endGroup();

        QList<T*> windows;
        for (const auto& id : windowIds)
            windows.append(new T(*app_, id));
        return windows;
    }

    // Counts MainWindow instances that are currently visible across all
    // top-level widgets. Used by MainWindow::closeEvent.
    int visibleWindowCount() const;

private:
    App* app_;
};

}  // namespace app_shell
