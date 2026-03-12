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

// ── coerce_arg ───────────────────────────────────────────────────────
// Converts a QJsonValue into a QGenericArgument matching the method's
// expected parameter type. Supports QString, int, double, bool, and
// QJsonObject/QJsonArray for complex types.
//
// The QVariant is used as stable storage — its lifetime must outlast the
// invoke call. We fill one QVariant per parameter and point the
// QGenericArgument at the QVariant's internal data.
// Uses QGenericArgument directly (not Q_ARG) for compatibility across Qt 6.x.
// Q_ARG returns QMetaMethodArgument in Qt 6.5+ which is a different type.
inline QGenericArgument coerce_arg(const QJsonValue& json_val, const QByteArray& param_type, QVariant& storage) {
    if (param_type == "QString") {
        storage = QVariant(json_val.isString() ? json_val.toString() : QString::number(json_val.toDouble()));
        return QGenericArgument("QString", storage.constData());
    }
    if (param_type == "int") {
        storage = QVariant(json_val.isDouble() ? json_val.toInt() : json_val.toString().toInt());
        return QGenericArgument("int", storage.constData());
    }
    if (param_type == "double") {
        storage = QVariant(json_val.toDouble());
        return QGenericArgument("double", storage.constData());
    }
    if (param_type == "bool") {
        storage = QVariant(json_val.isBool() ? json_val.toBool() : json_val.toString() == "true");
        return QGenericArgument("bool", storage.constData());
    }
    if (param_type == "QJsonObject") {
        storage = QVariant::fromValue(json_val.toObject());
        return QGenericArgument("QJsonObject", storage.constData());
    }
    if (param_type == "QJsonArray") {
        storage = QVariant::fromValue(json_val.toArray());
        return QGenericArgument("QJsonArray", storage.constData());
    }
    // Fallback: treat as QString
    storage = QVariant(json_val.toString());
    return QGenericArgument("QString", storage.constData());
}

// ── invoke_bridge_method ──────────────────────────────────────────────
// Calls a Q_INVOKABLE method by name with typed args from a JSON array.
//
// Supported parameter types: QString, int, double, bool, QJsonObject, QJsonArray
// Supported return types:    QJsonObject, QJsonArray, QString, int, double, bool, void
// Up to 10 parameters.
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
        return QJsonObject{{"error", "Unknown method: " + method_name}};

    QMetaMethod method = meta->method(method_index);
    int param_count = method.parameterCount();

    if (param_count > 10)
        return QJsonObject{{"error", method_name + ": too many parameters (max 10)"}};

    // Build typed QGenericArguments from the JSON array
    QVariant storage[10];
    QGenericArgument ga[10];
    for (int i = 0; i < param_count; ++i) {
        QJsonValue val = i < args.size() ? args[i] : QJsonValue();
        ga[i] = coerce_arg(val, method.parameterTypeName(i), storage[i]);
    }

    // Invoke with the right return type
    // Uses QGenericReturnArgument directly (not Q_RETURN_ARG) for Qt 6.5+ compat.
    QByteArray returnType = method.typeName();
    bool ok = false;

    #define BRIDGE_INVOKE(result) \
        method.invoke(bridge, Qt::DirectConnection, \
            QGenericReturnArgument(returnType.constData(), &result), \
            ga[0], ga[1], ga[2], ga[3], ga[4], ga[5], ga[6], ga[7], ga[8], ga[9])

    if (returnType == "QJsonObject") {
        QJsonObject result;
        ok = BRIDGE_INVOKE(result);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return result;
    }
    if (returnType == "QJsonArray") {
        QJsonArray result;
        ok = BRIDGE_INVOKE(result);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return result;
    }
    if (returnType == "QString") {
        QString result;
        ok = BRIDGE_INVOKE(result);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return QJsonObject{{"value", result}};
    }
    if (returnType == "int") {
        int result = 0;
        ok = BRIDGE_INVOKE(result);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return QJsonObject{{"value", result}};
    }
    if (returnType == "double") {
        double result = 0;
        ok = BRIDGE_INVOKE(result);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return QJsonObject{{"value", result}};
    }
    if (returnType == "bool") {
        bool result = false;
        ok = BRIDGE_INVOKE(result);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return QJsonObject{{"value", result}};
    }
    #undef BRIDGE_INVOKE

    // void return — invoke without return arg
    if (returnType == nullptr || QByteArray(returnType).isEmpty()) {
        ok = method.invoke(bridge, Qt::DirectConnection,
            ga[0], ga[1], ga[2], ga[3], ga[4], ga[5], ga[6], ga[7], ga[8], ga[9]);
        if (!ok) return QJsonObject{{"error", method_name + ": invocation failed"}};
        return QJsonObject{{"ok", true}};
    }

    return QJsonObject{{"error", method_name + ": unsupported return type '" + returnType + "'"}};
}

// ── collect_signal_names ─────────────────────────────────────────────
// Collects ALL signals — including those with parameters.
inline QJsonArray collect_signal_names(const QObject* obj) {
    const QMetaObject* meta = obj->metaObject();
    QJsonArray names;
    for (int i = meta->methodOffset(); i < meta->methodCount(); ++i) {
        QMetaMethod m = meta->method(i);
        if (m.methodType() == QMetaMethod::Signal)
            names.append(QString::fromLatin1(m.name()));
    }
    return names;
}

// ── forward_signals ──────────────────────────────────────────────────
// Forwards parameterless signals from a bridge to a WebSocket client.
// Signals with parameters are still listed in __meta__ so clients know
// they exist, but only parameterless signals are auto-forwarded.
// (Qt's QMetaObject::connect requires matching slot signatures, so
// forwarding arbitrary parameter types would need dynamic slot generation.
// For now, emit parameterless notification signals alongside data-carrying ones.)
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
                QJsonParseError parseErr;
                QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseErr);
                if (parseErr.error != QJsonParseError::NoError) {
                    QJsonObject errResp{{"error", "Invalid JSON: " + parseErr.errorString()}};
                    socket->sendTextMessage(
                        QString::fromUtf8(QJsonDocument(errResp).toJson(QJsonDocument::Compact)));
                    return;
                }
                QJsonObject request = doc.object();
                QString bridgeName = request["bridge"].toString();
                QString method = request["method"].toString();
                QJsonArray args = request["args"].toArray();
                qint64 id = request["id"].toInteger(-1);

                if (method.isEmpty()) {
                    QJsonObject errResp{{"error", "Missing 'method' field"}};
                    if (id >= 0) errResp["id"] = id;
                    socket->sendTextMessage(
                        QString::fromUtf8(QJsonDocument(errResp).toJson(QJsonDocument::Compact)));
                    return;
                }

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
