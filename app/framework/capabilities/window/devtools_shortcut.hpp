// DevToolsShortcut — window-scoped capability for F12 developer tools toggle.
//
// Creates its own QAction, adds it to the given menu (if provided),
// and wires to activeDockChanged to toggle devtools on the active dock's
// content widget via QMetaObject (content-agnostic — doesn't know WebShellWidget).

#pragma once

#include <QAction>
#include <QDockWidget>
#include <QMetaObject>
#include <QObject>

class QMenu;
class MainWindow;

class DevToolsShortcut : public QObject {
    Q_OBJECT

public:
    explicit DevToolsShortcut(MainWindow* parent, QMenu* menu = nullptr);

    QAction* action() const { return action_; }

private:
    void wireToDock(QDockWidget* dock);
    QAction* action_;
};
