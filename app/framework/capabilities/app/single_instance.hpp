// app_shell::SingleInstance — single-instance guard backed by QLocalServer.
//
// Constructed by the consumer as a child of App. The ctor performs the IPC
// handshake: if another instance is already running, it forwards this
// process's CLI args / activation to the primary and isPrimary() returns
// false (main() exits cleanly). Otherwise this process listens on a
// QLocalServer and emits activationRequested() / argsReceived() when
// future secondary instances try to launch.

#pragma once

#include <QObject>
#include <QStringList>

class QLocalServer;

namespace app_shell {

class App;

class SingleInstance : public QObject {
    Q_OBJECT

public:
    explicit SingleInstance(App* parent);

    // False when another instance was already running and this process
    // forwarded its args / activation to it. main() should exit cleanly.
    bool isPrimary() const { return isPrimary_; }

    // Forwards `args` through the same path a secondary-instance launch
    // would take. Used by App's macOS QEvent::FileOpen handler so external
    // URL/file activations route the same way as second-instance launches.
    void deliverExternalArgs(const QStringList& args);

signals:
    // Emitted when a secondary instance has tried to launch and is asking
    // the primary to raise / focus itself.
    void activationRequested();

    // Emitted with the args a secondary instance was launched with (or with
    // payloads delivered via deliverExternalArgs).
    void argsReceived(const QStringList& args);

private:
    App* app_;
    bool isPrimary_ = true;
    QLocalServer* server_ = nullptr;
};

}  // namespace app_shell
