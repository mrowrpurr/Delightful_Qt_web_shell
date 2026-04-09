#pragma once

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

// Shell — owns bridge registration and lifecycle.
// Register domain bridges (QObjects with Q_INVOKABLE methods) by name,
// and the infrastructure exposes them via WebSocket or QWebChannel automatically.
//
// Usage:
//   WebShell shell;
//   shell.addBridge("todos", new TodoBridge(&shell));
//
// Infrastructure handles the rest — you never subclass or modify this.
class WebShell : public QObject {
    Q_OBJECT
    QMap<QString, QObject*> bridges_;

public:
    using QObject::QObject;

    void addBridge(const QString& name, QObject* bridge) {
        bridge->setParent(this);
        bridges_[name] = bridge;
    }

    const QMap<QString, QObject*>& bridges() const { return bridges_; }

    // Called by the transport layer after React renders its first frame.
    // The Qt desktop shell connects to ready() to fade the loading overlay.
    Q_INVOKABLE QJsonObject appReady() {
        emit ready();
        return {};
    }

signals:
    void ready();
};
