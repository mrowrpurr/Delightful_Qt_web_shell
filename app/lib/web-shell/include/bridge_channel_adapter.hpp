// bridge_channel_adapter.hpp — QObject wrapper for typed_bridge over QWebChannel.
//
// QWebChannel requires QObject instances. This adapter wraps a typed_bridge
// with a single Q_INVOKABLE dispatch method that routes calls through the
// typed_bridge dispatch engine.
//
// The TS side calls: channel.objects.todos.dispatch("addList", {name: "Groceries"})

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

#include "json_adapter.hpp"
#include "typed_bridge.hpp"

class BridgeChannelAdapter : public QObject {
    Q_OBJECT
    web_shell::typed_bridge* bridge_;

public:
    BridgeChannelAdapter(web_shell::typed_bridge* bridge, QObject* parent = nullptr)
        : QObject(parent), bridge_(bridge) {}

    Q_INVOKABLE QJsonObject dispatch(const QString& method, const QJsonObject& args) {
        auto result = bridge_->dispatch(
            method.toStdString(),
            web_shell::from_qt_json(args));
        return web_shell::to_qt_json(result);
    }
};
