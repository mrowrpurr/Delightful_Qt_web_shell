// DevToolsShortcut — window-scoped subsystem for F12 developer tools toggle.
//
// Creates its own QAction, adds it to the given menu (if provided),
// and wires it to invoke toggleDevTools() on the active dock's content
// widget via QMetaObject (content-agnostic — doesn't know WebShellWidget).

#pragma once

#include <QAction>
#include <QDockWidget>
#include <QMetaObject>
#include <QObject>

class QMenu;

class DevToolsShortcut : public QObject {
    Q_OBJECT

public:
    explicit DevToolsShortcut(QWidget* parent, QMenu* menu = nullptr);

    QAction* action() const { return action_; }

    // Call when the active dock changes — rewires the action to the new dock.
    void setActiveDock(QDockWidget* dock) {
        action_->disconnect();
        if (!dock || !dock->widget()) return;
        auto* widget = dock->widget();
        connect(action_, &QAction::triggered, widget, [widget]() {
            QMetaObject::invokeMethod(widget, "toggleDevTools");
        });
    }

private:
    QAction* action_;
};
