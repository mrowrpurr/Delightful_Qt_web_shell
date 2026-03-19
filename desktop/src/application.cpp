// Application — app-wide setup that applies regardless of which windows are open.
//
// Identity, dark theme, web profile, bridges — all live here.
// Widgets and windows are created separately and pull what they need from here.

#include "application.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QPalette>
#include <QStandardPaths>
#include <QStyleHints>
#include <QWebEngineProfile>

// @scaffold:include
#include "todo_bridge.hpp"
#include "web_shell.hpp"

// Must match --bg in App.css — prevents white flash before web content loads.
static constexpr QColor kBackground{0x24, 0x24, 0x24};

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    // ── Identity ─────────────────────────────────────────────
    setOrganizationName(APP_ORG);
    setApplicationName(APP_NAME);
    setApplicationVersion(APP_VERSION);
    setWindowIcon(QIcon(":/icon.ico"));

    // ── Command line ─────────────────────────────────────────
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption devOption("dev",
        "Dev mode: load from Vite dev server (localhost:5173) with hot reload");
    parser.addOption(devOption);
    parser.process(*this);
    devMode_ = parser.isSet(devOption);

    // ── Dark theme ───────────────────────────────────────────
    // setColorScheme handles menus and system widgets.
    // The palette ensures the window background is dark before any content paints,
    // preventing the white flash (FOUC) on startup.
    styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, kBackground);
    darkPalette.setColor(QPalette::Base, kBackground);
    setPalette(darkPalette);

    // ── Web profile ──────────────────────────────────────────
    // Named profile = persistent localStorage and IndexedDB across sessions.
    // Data lives in the platform's standard app data directory:
    //   Windows: AppData/Local/<org>/<app>/
    //   macOS:   ~/Library/Application Support/<app>/
    //   Linux:   ~/.local/share/<app>/
    profile_ = new QWebEngineProfile(APP_SLUG, this);
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    profile_->setCachePath(dataDir + "/cache");
    profile_->setPersistentStoragePath(dataDir + "/webdata");
    profile_->setHttpCacheType(QWebEngineProfile::DiskHttpCache);

    // ── Shell + bridges ──────────────────────────────────────
    // The shell owns all bridges. Every WebShellWidget registers the same
    // bridge instances on its own QWebChannel — one source of truth,
    // signals reach all connected views.
    shell_ = new WebShell(this);
    // @scaffold:bridge
    auto* todoBridge = new TodoBridge;
    shell_->addBridge("todos", todoBridge);
}
