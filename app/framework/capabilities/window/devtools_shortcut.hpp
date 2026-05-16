// DevToolsShortcut — window-scoped subsystem for F12 developer tools toggle.
//
// Wires a QAction to invoke toggleDevTools() on the active dock's content widget
// via QMetaObject (content-agnostic — doesn't know WebShellWidget).

#pragma once

#include <QAction>
#include <QDockWidget>
#include <QMetaObject>
#include <QObject>

class DevToolsShortcut : public QObject {
    Q_OBJECT

public:
    explicit DevToolsShortcut(QAction* action, QObject* parent = nullptr)
        : QObject(parent), action_(action) {}

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
