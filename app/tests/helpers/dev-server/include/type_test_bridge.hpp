#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Test-only bridge exercising every supported param/return type.
// Registered in dev-server (NOT the desktop app) so bun tests can verify
// that QVariant-based type conversion works for any Q_INVOKABLE signature.
// See lib/web-shell/tests/web/type_conversion_test.ts for the test suite.
class TypeTestBridge : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    // ── Return type tests ─────────────────────────────────────────────

    Q_INVOKABLE QString returnString() const { return "hello"; }
    Q_INVOKABLE int returnInt() const { return 42; }
    Q_INVOKABLE double returnDouble() const { return 3.14; }
    Q_INVOKABLE bool returnBool() const { return true; }
    Q_INVOKABLE QJsonObject returnJsonObject() const { return {{"key", "value"}}; }
    Q_INVOKABLE QJsonArray returnJsonArray() const { return QJsonArray{"a", "b", "c"}; }
    Q_INVOKABLE QStringList returnStringList() const { return {"one", "two", "three"}; }
    Q_INVOKABLE void returnVoid() { last_call_ = "returnVoid"; }

    // ── Parameter type tests (echo back as JSON) ──────────────────────

    Q_INVOKABLE QJsonObject echoString(const QString& s) const {
        return {{"type", "QString"}, {"value", s}};
    }
    Q_INVOKABLE QJsonObject echoInt(int n) const {
        return {{"type", "int"}, {"value", n}};
    }
    Q_INVOKABLE QJsonObject echoDouble(double d) const {
        return {{"type", "double"}, {"value", d}};
    }
    Q_INVOKABLE QJsonObject echoBool(bool b) const {
        return {{"type", "bool"}, {"value", b}};
    }
    Q_INVOKABLE QJsonObject echoJsonObject(const QJsonObject& obj) const {
        return {{"type", "QJsonObject"}, {"value", obj}};
    }
    Q_INVOKABLE QJsonObject echoJsonArray(const QJsonArray& arr) const {
        QJsonObject result;
        result["type"] = "QJsonArray";
        result["value"] = arr;
        return result;
    }

    // ── Multi-parameter tests ─────────────────────────────────────────

    Q_INVOKABLE QJsonObject multiParams(const QString& s, int n, bool b) const {
        return {{"string", s}, {"int", n}, {"bool", b}};
    }
    Q_INVOKABLE QJsonObject fiveParams(const QString& a, const QString& b,
                                        const QString& c, const QString& d,
                                        const QString& e) const {
        return {{"concat", a + b + c + d + e}};
    }
    Q_INVOKABLE QJsonObject tenParams(int a, int b, int c, int d, int e,
                                       int f, int g, int h, int i, int j) const {
        return {{"sum", a + b + c + d + e + f + g + h + i + j}};
    }

    // ── Edge cases ────────────────────────────────────────────────────

    Q_INVOKABLE QJsonObject echoEmptyString() const {
        return {{"value", ""}};
    }
    Q_INVOKABLE QJsonObject echoLargeNumber(double n) const {
        return {{"value", n}};
    }
    Q_INVOKABLE QJsonObject noArgs() const {
        return {{"ok", true}};
    }

    // ── Verify void was called ────────────────────────────────────────
    Q_INVOKABLE QString lastCall() const { return last_call_; }

signals:
    void testSignal();

private:
    QString last_call_;
};
