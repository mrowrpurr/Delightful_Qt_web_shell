// app_shell::App — see app.hpp for overview.

#include "shell/app.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QEvent>
#include <QFileOpenEvent>
#include <QIcon>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QWebEngineProfile>

#include "dock_manager.hpp"
#include "shell/single_instance.hpp"
#include "style_manager.hpp"
#include "widgets/scheme_handler.hpp"

#include <oclero/qlementine/icons/QlementineIcons.hpp>

// @scaffold:include
#include "system_bridge.hpp"
#include "todo_bridge.hpp"
#include "app_lifecycle.hpp"

namespace app_shell {

// Must match --bg in App.css — prevents white flash before web content loads.
static constexpr QColor kBackground{0x24, 0x24, 0x24};

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

    // ── Dark theme ───────────────────────────────────────────
    styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, kBackground);
    darkPalette.setColor(QPalette::Base, kBackground);
    setPalette(darkPalette);

    // ── Style manager ──────────────────────────────────────────
    styleManager_ = new StyleManager(this);
    styleManager_->applyTheme("default-dark");

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

    // ── Bridges + lifecycle ──────────────────────────────────
    lifecycle_ = new AppLifecycle(this);
    // @scaffold:bridge
    auto* todoBridge = new TodoBridge;
    registry_.add("todos", todoBridge);
    auto* systemBridge = new SystemBridge;
    registry_.add("system", systemBridge);

    // ── Wire StyleManager ↔ SystemBridge ──────────────────────
    connect(styleManager_, &StyleManager::themeChanged, this, [this, systemBridge]() {
        systemBridge->updateQtThemeState(
            styleManager_->currentDisplayName(), styleManager_->isDarkMode());
        QString filePath = styleManager_->currentThemeFilePath();
        systemBridge->setQtThemeFilePath(
            filePath.toStdString(), filePath.startsWith(":/"));
    });
    systemBridge->on_signal("qtThemeRequested", [this](const nlohmann::json& data) {
        auto displayName = QString::fromStdString(data["displayName"].get<std::string>());
        bool isDark = data["isDark"].get<bool>();
        QMetaObject::invokeMethod(this, [this, displayName, isDark]() {
            styleManager_->applyThemeByDisplayName(displayName, isDark);
        }, Qt::QueuedConnection);
    });

    // ── Dock manager ─────────────────────────────────────────
    dockManager_ = new DockManager(*this, this);

    // ── Shutdown ─────────────────────────────────────────────
    connect(this, &QApplication::aboutToQuit, this, [this]() {
        dockManager_->shutdownAll();
    });
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

QUrl App::appUrl(const QString& appName) const {
    if (devMode_) {
        static const QHash<QString, int> devPorts = {
            {"demo", 5173},
        };
        int port = devPorts.value(appName, 5175);
        return QUrl(QString("http://localhost:%1").arg(port));
    }
    return QUrl(QString("app://%1/").arg(appName));
}

}  // namespace app_shell
