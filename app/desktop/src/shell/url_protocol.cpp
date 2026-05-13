// app_shell::UrlProtocol — see url_protocol.hpp for overview.
//
// This file contains the platform-neutral methods. Each platform
// implements isRegistered() / registerProtocol() / unregisterProtocol()
// in its own file:
//   - url_protocol_windows.cpp  (HKCU\Software\Classes registry)
//   - url_protocol_macos.mm     (LaunchServices + Info.plist)

#include "shell/url_protocol.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>

#include "shell/app.hpp"

namespace app_shell {

UrlProtocol::UrlProtocol(App* parent)
    : QObject(parent), app_(parent)
{}

QString UrlProtocol::name() const {
    return QString(APP_SLUG).toLower();
}

void UrlProtocol::promptIfNeeded() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);
    if (settings.value("urlProtocol/dontAsk", false).toBool()) return;
    if (isRegistered()) return;

    QMessageBox box;
    box.setWindowTitle(APP_NAME);
    box.setIcon(QMessageBox::Question);
    box.setText(QString("Register <b>%1://</b> URL protocol?").arg(name()));
    box.setInformativeText(
        QString("This lets you open %1 from a browser or other apps by clicking "
                "<b>%2://</b> links.").arg(APP_NAME, name()));

    auto* dontAskCheck = new QCheckBox("Don't ask me again");
    box.setCheckBox(dontAskCheck);

    box.addButton(QMessageBox::Yes);
    box.addButton(QMessageBox::No);
    box.setDefaultButton(QMessageBox::Yes);

    int result = box.exec();

    if (dontAskCheck->isChecked())
        settings.setValue("urlProtocol/dontAsk", true);

    if (result == QMessageBox::Yes)
        registerProtocol();
}

}  // namespace app_shell
