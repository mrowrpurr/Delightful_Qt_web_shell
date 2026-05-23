// app_shell::App — see app.hpp for overview.

#include "app.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QEvent>
#include <QFileOpenEvent>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>
#include <QWebEngineProfile>

#include "dock_manager.hpp"
#include "single_instance.hpp"
#include "theming.hpp"
#include "scheme_handler.hpp"
#include "web_shell_widget.hpp"

#include <oclero/qlementine/icons/QlementineIcons.hpp>

#include "ready_signal.hpp"

namespace app_shell {

App::App(int& argc, char** argv)
    : QApplication(argc, argv)
{
    // ── Icons ──────────────────────────────────────────────────
    // Initialize Qlementine icon theme — must happen before any QIcon usage.
    // After this, use QIcon(iconPath(Icons16::...)) anywhere in the app.
    oclero::qlementine::icons::initializeIconTheme();

    // ── Identity ─────────────────────────────────────────────
    setOrganizationName(APP_ORG);
    setApplicationName(APP_NAME);

    // Use INI files for QSettings instead of the Windows registry.
    // Settings file lives in AppData/Local/<org>/<app>.ini.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    setApplicationVersion(APP_VERSION);
    setWindowIcon(QIcon(iconPath_));

    // ── Command line ─────────────────────────────────────────
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption devOption("dev",
        "Dev mode: load from Vite dev servers (main=5173) with hot reload");
    parser.addOption(devOption);
    parser.parse(arguments());
    devMode_ = parser.isSet(devOption);

    // ── Web profile ──────────────────────────────────────────
    profile_ = new QWebEngineProfile(APP_SLUG, this);
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    profile_->setCachePath(dataDir + "/cache");
    profile_->setPersistentStoragePath(dataDir + "/webdata");
    profile_->setHttpCacheType(QWebEngineProfile::DiskHttpCache);

    if (!devMode_) {
        auto* handler = new SchemeHandler(profile_);
        profile_->installUrlSchemeHandler("app", handler);
    }

    // ── Lifecycle ──────────────────────────────────────────────
    lifecycle_ = new ReadySignal(this);

    // ── Dock manager ─────────────────────────────────────────
    dockManager_ = new DockManager(*this, this);
    dockManager_->setWidgetFactory([this](const QUrl& url) -> QWidget* {
        return new WebShellWidget(
            webProfile(), registry(), lifecycle(), url,
            brandingImagePath(), WebShellWidget::FullOverlay);
    });

    // ── Shutdown ─────────────────────────────────────────────
    connect(this, &QApplication::aboutToQuit, this, [this]() {
        dockManager_->shutdownAll();
    });
}

StyleManager* App::styleManager() const {
    if (auto* theming = findChild<Theming*>())
        return theming->styleManager();
    return nullptr;
}

void App::requestQuit() {
    dockManager_->shutdownAll();
    quit();
}

bool App::event(QEvent* event) {
    if (event->type() == QEvent::FileOpen) {
        auto* openEvent = static_cast<QFileOpenEvent*>(event);
        QString payload = openEvent->url().toString();
        if (payload.isEmpty())
            payload = openEvent->file();
        if (!payload.isEmpty()) {
            if (auto* si = findChild<SingleInstance*>())
                si->deliverExternalArgs({payload});
        }
        return true;
    }
    return QApplication::event(event);
}

void App::registerDevPort(const QString& appName, int port) {
    devPorts_.insert(appName, port);
}

QUrl App::appUrl(const QString& appName) const {
    if (devMode_ && devPorts_.contains(appName)) {
        int port = devPorts_.value(appName);
        return QUrl(QString("http://localhost:%1").arg(port));
    }
    return QUrl(QString("app://%1/").arg(appName));
}

}  // namespace app_shell
