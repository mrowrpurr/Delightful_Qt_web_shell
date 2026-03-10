#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaMethod>
#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>

#include "web_shell.hpp"

// ── SignalForwarder ───────────────────────────────────────────────────
// Bridges a Qt signal to a WebSocket JSON event.
// Includes the bridge name so clients know which bridge emitted it.
// Destroyed when the socket disconnects (parent = socket).
class SignalForwarder : public QObject {
    Q_OBJECT
    QWebSocket* socket_;
    QString     bridge_name_;
    QString     event_name_;
public:
    SignalForwarder(QWebSocket* socket, const QString& bridge, const QString& event, QObject* parent = nullptr)
        : QObject(parent), socket_(socket), bridge_name_(bridge), event_name_(event) {}
public slots:
    void forward() {
        if (!socket_ || !socket_->isValid()) return;
        QJsonObject msg;
        if (!bridge_name_.isEmpty())
            msg["bridge"] = bridge_name_;
        msg["event"] = event_name_;
        socket_->sendTextMessage(
            QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact)));
    }
};

// ── invoke_bridge_method ──────────────────────────────────────────────
// Calls a Q_INVOKABLE method by name with QString args from a JSON array.
// Bridge methods return QJsonObject or QJsonArray.
inline QJsonValue invoke_bridge_method(QObject* bridge, const QString& method_name, const QJsonArray& args) {
    const QMetaObject* meta = bridge->metaObject();

    // Find the method
    int method_index = -1;
    for (int i = meta->methodOffset(); i < meta->methodCount(); ++i) {
        QMetaMethod m = meta->method(i);
        if (m.name() == method_name.toLatin1() &&
            (m.methodType() == QMetaMethod::Slot || m.methodType() == QMetaMethod::Method)) {
            method_index = i;
            break;
        }
    }
    if (method_index < 0)
        return QJsonObject{{"error", "Unknown method"}};

    QMetaMethod method = meta->method(method_index);

    // Extract QString args from the JSON array
    QStringList string_args;
    for (const auto& a : args)
        string_args.append(a.isString() ? a.toString() : QString::number(a.toInt()));

    // Invoke based on return type and parameter count.
    // All parameters are QString by convention.
    bool ok = false;
    QByteArray returnType = method.typeName();

    #define INVOKE(RetType, result) \
        switch (method.parameterCount()) { \
            case 0: ok = method.invoke(bridge, Qt::DirectConnection, \
                Q_RETURN_ARG(RetType, result)); break; \
            case 1: ok = method.invoke(bridge, Qt::DirectConnection, \
                Q_RETURN_ARG(RetType, result), \
                Q_ARG(QString, string_args.value(0))); break; \
            case 2: ok = method.invoke(bridge, Qt::DirectConnection, \
                Q_RETURN_ARG(RetType, result), \
                Q_ARG(QString, string_args.value(0)), \
                Q_ARG(QString, string_args.value(1))); break; \
            case 3: ok = method.invoke(bridge, Qt::DirectConnection, \
                Q_RETURN_ARG(RetType, result), \
                Q_ARG(QString, string_args.value(0)), \
                Q_ARG(QString, string_args.value(1)), \
                Q_ARG(QString, string_args.value(2))); break; \
            default: return QJsonObject{{"error", "Too many parameters"}}; \
        }

    if (returnType == "QJsonObject") {
        QJsonObject result;
        INVOKE(QJsonObject, result)
        if (!ok) return QJsonObject{{"error", "Method invocation failed"}};
        return result;
    }

    if (returnType == "QJsonArray") {
        QJsonArray result;
        INVOKE(QJsonArray, result)
        if (!ok) return QJsonObject{{"error", "Method invocation failed"}};
        return result;
    }

    #undef INVOKE
    return QJsonObject{{"error", "Unsupported return type"}};
}

// ── collect_signal_names ─────────────────────────────────────────────
inline QJsonArray collect_signal_names(const QObject* obj) {
    const QMetaObject* meta = obj->metaObject();
    QJsonArray names;
    for (int i = meta->methodOffset(); i < meta->methodCount(); ++i) {
        QMetaMethod m = meta->method(i);
        if (m.methodType() == QMetaMethod::Signal && m.parameterCount() == 0)
            names.append(QString::fromLatin1(m.name()));
    }
    return names;
}

// ── forward_signals ──────────────────────────────────────────────────
inline void forward_signals(QObject* source, const QString& bridgeName, QWebSocket* socket) {
    const QMetaObject* meta = source->metaObject();
    const int forwardSlot = SignalForwarder::staticMetaObject.indexOfSlot("forward()");
    for (int i = meta->methodOffset(); i < meta->methodCount(); ++i) {
        QMetaMethod m = meta->method(i);
        if (m.methodType() == QMetaMethod::Signal && m.parameterCount() == 0) {
            auto* fwd = new SignalForwarder(socket, bridgeName, QString::fromLatin1(m.name()), socket);
            QMetaObject::connect(source, i, fwd, forwardSlot);
        }
    }
}

// ── expose_as_ws ──────────────────────────────────────────────────────
// Exposes a WebShell and all its registered bridges as a WebSocket
// JSON-RPC server. Each message can target a specific bridge by name.
//
// Protocol:
//   → {"bridge": "todos", "method": "listLists", "args": [], "id": 1}
//   ← {"id": 1, "result": [...]}
//
//   → {"method": "appReady", "args": [], "id": 2}         (no bridge = shell)
//   ← {"id": 2, "result": {}}
//
//   ← {"bridge": "todos", "event": "dataChanged"}         (pushed on signal)
//
//   → {"method": "__meta__", "args": [], "id": 0}
//   ← {"id": 0, "result": {"bridges": {"todos": {"signals": ["dataChanged"]}}}}
//
// Convention: Q_INVOKABLE methods take QStrings, return QJsonObject or QJsonArray.
//             Parameterless signals are forwarded as events.
inline QWebSocketServer* expose_as_ws(WebShell* shell, int port, QObject* parent = nullptr) {
    auto* server = new QWebSocketServer(
        QStringLiteral("BridgeServer"), QWebSocketServer::NonSecureMode, parent);

    if (!server->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "expose_as_ws: failed to listen on port" << port;
        delete server;
        return nullptr;
    }

    QObject::connect(server, &QWebSocketServer::newConnection, server, [shell, server]() {
        auto* socket = server->nextPendingConnection();
        if (!socket) return;

        // ── Method dispatch ──────────────────────────────────────
        QObject::connect(socket, &QWebSocket::textMessageReceived, server,
            [shell, socket](const QString& message) {
                QJsonObject request = QJsonDocument::fromJson(message.toUtf8()).object();
                QString bridgeName = request["bridge"].toString();
                QString method = request["method"].toString();
                QJsonArray args = request["args"].toArray();
                qint64 id = request["id"].toInteger(-1);

                QJsonValue result_value;

                if (method == "__meta__") {
                    // Return all bridges and their signals
                    QJsonObject bridges;
                    for (auto it = shell->bridges().begin(); it != shell->bridges().end(); ++it)
                        bridges[it.key()] = QJsonObject{{"signals", collect_signal_names(it.value())}};
                    result_value = QJsonObject{{"bridges", bridges}};
                } else {
                    // Route: no bridge name → shell, otherwise → named bridge
                    QObject* target = bridgeName.isEmpty()
                        ? static_cast<QObject*>(shell)
                        : shell->bridges().value(bridgeName);

                    if (!target)
                        result_value = QJsonObject{{"error", "Unknown bridge: " + bridgeName}};
                    else
                        result_value = invoke_bridge_method(target, method, args);
                }

                // Promote error responses to the top level so clients
                // can distinguish errors from successful results.
                QJsonObject response;
                if (id >= 0) response["id"] = id;
                if (auto obj = result_value.toObject(); obj.contains("error"))
                    response["error"] = obj["error"];
                else
                    response["result"] = result_value;

                socket->sendTextMessage(
                    QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
            });

        // ── Forward signals from all registered bridges ──────────
        for (auto it = shell->bridges().begin(); it != shell->bridges().end(); ++it)
            forward_signals(it.value(), it.key(), socket);

        // ── Cleanup on disconnect ────────────────────────────────
        QObject::connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
    });

    qInfo() << "Bridge WebSocket server listening on port" << port;
    return server;
}
