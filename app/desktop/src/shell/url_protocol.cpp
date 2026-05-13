// app_shell::UrlProtocol — see url_protocol.hpp for overview.

#include "shell/url_protocol.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#include "shell/app.hpp"

namespace app_shell {

UrlProtocol::UrlProtocol(App* parent)
    : QObject(parent), app_(parent)
{}

QString UrlProtocol::name() const {
    return QString(APP_SLUG).toLower();
}

bool UrlProtocol::isRegistered() const {
    QString protocol = name();

#ifdef Q_OS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes\\" + protocol,
                   QSettings::NativeFormat);
    QString cmd = reg.value("shell/open/command/Default").toString();
    return cmd.contains(QDir::toNativeSeparators(QApplication::applicationFilePath()));
#endif

#ifdef Q_OS_LINUX
    QString desktopDir = QStandardPaths::writableLocation(
        QStandardPaths::ApplicationsLocation);
    return QFile::exists(desktopDir + "/" + protocol + ".desktop");
#endif

    return true;
}

void UrlProtocol::registerProtocol() {
    QString protocol = name();
    QString exePath = QDir::toNativeSeparators(QApplication::applicationFilePath());

#ifdef Q_OS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes\\" + protocol,
                   QSettings::NativeFormat);
    reg.setValue("Default", QString("URL:%1 Protocol").arg(APP_NAME));
    reg.setValue("URL Protocol", "");
    reg.setValue("shell/open/command/Default",
                 QString("\"%1\" \"%2\"").arg(exePath, "%1"));
#endif

#ifdef Q_OS_LINUX
    QString desktopDir = QStandardPaths::writableLocation(
        QStandardPaths::ApplicationsLocation);
    QDir().mkpath(desktopDir);
    QString desktopPath = desktopDir + "/" + protocol + ".desktop";

    QFile f(desktopPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << "[Desktop Entry]\n"
            << "Type=Application\n"
            << "Name=" << APP_NAME << "\n"
            << "Exec=\"" << exePath << "\" %u\n"
            << "MimeType=x-scheme-handler/" << protocol << "\n"
            << "NoDisplay=true\n";
        f.close();
        QProcess::startDetached("xdg-mime",
            {"default", protocol + ".desktop", "x-scheme-handler/" + protocol});
    }
#endif
}

void UrlProtocol::unregisterProtocol() {
    QString protocol = name();

#ifdef Q_OS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes",
                   QSettings::NativeFormat);
    reg.remove(protocol);
#endif

#ifdef Q_OS_LINUX
    QString desktopDir = QStandardPaths::writableLocation(
        QStandardPaths::ApplicationsLocation);
    QFile::remove(desktopDir + "/" + protocol + ".desktop");
#endif
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
