// app_shell::UrlProtocol — Windows implementation.
//
// Writes/reads HKCU\Software\Classes\<scheme>, the per-user registry
// location for URL protocol handlers. No admin rights required.

#include "url_protocol.hpp"

#include <QApplication>
#include <QDir>
#include <QSettings>

#include "app.hpp"

namespace app_shell {

bool UrlProtocol::isRegistered() const {
    QString protocol = name();
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes\\" + protocol,
                   QSettings::NativeFormat);
    QString cmd = reg.value("shell/open/command/Default").toString();
    return cmd.contains(QDir::toNativeSeparators(QApplication::applicationFilePath()));
}

void UrlProtocol::registerProtocol() {
    QString protocol = name();
    QString exePath = QDir::toNativeSeparators(QApplication::applicationFilePath());

    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes\\" + protocol,
                   QSettings::NativeFormat);
    reg.setValue("Default", QString("URL:%1 Protocol").arg(APP_NAME));
    reg.setValue("URL Protocol", "");
    reg.setValue("shell/open/command/Default",
                 QString("\"%1\" \"%2\"").arg(exePath, "%1"));
}

void UrlProtocol::unregisterProtocol() {
    QString protocol = name();
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes",
                   QSettings::NativeFormat);
    reg.remove(protocol);
}

}  // namespace app_shell
