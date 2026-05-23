// app_shell::DockActions — new tab / close tab for a dock-based window.
//
// Parented to a MainWindowBase (or subclass), adds its actions to the
// given menu, and wires to DockManager for tab creation/closure.

#pragma once

#include <QObject>

class QAction;
class QMenu;
class MainWindow;

namespace app_shell {

class DockActions : public QObject {
    Q_OBJECT

public:
    explicit DockActions(MainWindow* parent, QMenu* menu);

    QAction* newTabAction() const { return newTab_; }
    QAction* closeTabAction() const { return closeTab_; }

private:
    QAction* newTab_   = nullptr;
    QAction* closeTab_ = nullptr;
};

}  // namespace app_shell
