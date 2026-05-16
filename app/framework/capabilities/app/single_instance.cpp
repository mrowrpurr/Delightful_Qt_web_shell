// app_shell::SingleInstance — see single_instance.hpp for overview.

#include "single_instance.hpp"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>

#include "shell/app.hpp"

namespace app_shell {

SingleInstance::SingleInstance(App* parent)
    : QObject(parent), app_(parent)
{
    QString serverName = APP_SLUG;

    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        QStringList allArgs = app_->arguments().mid(1);
        if (allArgs.isEmpty()) {
            socket.write("activate\n");
        } else {
            for (const auto& arg : allArgs)
                socket.write(("arg:" + arg + "\n").toUtf8());
        }
        socket.waitForBytesWritten(1000);
        socket.disconnectFromServer();
        isPrimary_ = false;
        return;
    }

    QLocalServer::removeServer(serverName);

    server_ = new QLocalServer(this);
    server_->listen(serverName);
    connect(server_, &QLocalServer::newConnection, this, [this]() {
        auto* client = server_->nextPendingConnection();
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            QString data = QString::fromUtf8(client->readAll());
            QStringList lines = data.split('\n', Qt::SkipEmptyParts);
            QStringList args;
            for (const auto& line : lines) {
                if (line.startsWith("arg:"))
                    args.append(line.mid(4));
            }
            if (!args.isEmpty())
                emit argsReceived(args);
            emit activationRequested();
            client->deleteLater();
        });
    });
}

void SingleInstance::deliverExternalArgs(const QStringList& args) {
    if (!args.isEmpty())
        emit argsReceived(args);
}

}  // namespace app_shell
