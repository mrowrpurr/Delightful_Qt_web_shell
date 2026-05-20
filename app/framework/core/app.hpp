// app_shell::App — the framework's QApplication subclass.
//
// Holds identity (org / app name / version), the QWebEngineProfile, the
// bridge registry, and the dock manager. Forwards macOS URL scheme
// activations delivered via QEvent::FileOpen.

#pragma once

#include <utility>

#include <QApplication>
#include <QUrl>

#include "bridge_registry.hpp"

class ReadySignal;
class DockManager;
class QWebEngineProfile;
class StyleManager;

namespace app_shell {

class App : public QApplication {
    Q_OBJECT

public:
    App(int& argc, char** argv);

    bool devMode() const { return devMode_; }
    QWebEngineProfile* webProfile() const { return profile_; }
    BridgeRegistry* registry() { return &registry_; }
    const BridgeRegistry* registry() const { return &registry_; }

    // Typed bridge registration + retrieval.
    template<typename T, typename... Args>
    T* addBridge(const std::string& name, Args&&... args) {
        auto* bridge = new T(std::forward<Args>(args)...);
        registry_.add(name, bridge);
        return bridge;
    }

    template<typename T>
    T* bridge() const { return registry_.get<T>(); }
    ReadySignal* lifecycle() const { return lifecycle_; }
    StyleManager* styleManager() const;
    DockManager* dockManager() const { return dockManager_; }

    QUrl appUrl(const QString& appName) const;

    // The web app name used by MainWindow/DockManager when no URL is specified.
    QString defaultWebApp() const { return defaultWebApp_; }
    void setDefaultWebApp(const QString& name) { defaultWebApp_ = name; }

    // The window/tray icon (a Windows .ico bundle on Windows, a PNG elsewhere).
    // Framework reads the icon through this accessor so consumers can rename
    // their icon without editing framework code.
    QString iconPath() const { return iconPath_; }

    // The PNG used for in-app branding (about dialog, loading overlay).
    // Separate from iconPath() because Windows wants .ico for window/tray
    // metadata but Qt widgets want a real raster format.
    QString brandingImagePath() const { return brandingImagePath_; }

public slots:
    // Cleanly shut down all docks and windows, then quit.
    // Use this instead of QApplication::quit() so cleanup runs
    // while the event loop is still alive.
    void requestQuit();

protected:
    // macOS delivers URL scheme activations via QEvent::FileOpen. Forwarded
    // to the SingleInstance subsystem (if present) so external activations
    // route through the same path as a second-instance launch.
    bool event(QEvent* event) override;

private:
    bool devMode_ = false;
    QString defaultWebApp_ = "app";
    QString iconPath_ = ":/icon.ico";
    QString brandingImagePath_ = ":/icon.png";
    QWebEngineProfile* profile_ = nullptr;
    BridgeRegistry registry_;
    ReadySignal* lifecycle_ = nullptr;
    DockManager* dockManager_ = nullptr;
};

}  // namespace app_shell
