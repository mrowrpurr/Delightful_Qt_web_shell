// WebShell — owns bridge registration and lifecycle.

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

#include "typed_bridge.hpp"

class WebShell : public QObject {
    Q_OBJECT
    QMap<QString, web_shell::typed_bridge*> bridges_;

public:
    using QObject::QObject;

    void addBridge(const QString& name, web_shell::typed_bridge* bridge) {
        bridges_[name] = bridge;
    }

    const QMap<QString, web_shell::typed_bridge*>& bridges() const { return bridges_; }

    // Called by the transport layer after React renders its first frame.
    Q_INVOKABLE QJsonObject appReady() {
        emit ready();
        return {};
    }

signals:
    void ready();
};
