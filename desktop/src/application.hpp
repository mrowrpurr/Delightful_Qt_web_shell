// Application — custom QApplication subclass.
//
// Owns app-level concerns: identity, appearance, web profile, bridges.
// Widgets and windows come later — the app can run without any visible window
// (e.g. system tray only).

#pragma once

#include <QApplication>

class QWebEngineProfile;
class WebShell;

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);

    // Whether --dev was passed (load from Vite instead of embedded resources)
    bool devMode() const { return devMode_; }

    // Shared web engine profile — persistent localStorage/IndexedDB
    QWebEngineProfile* webProfile() const { return profile_; }

    // The shell that owns all bridges — shared across all WebShellWidgets
    WebShell* shell() const { return shell_; }

private:
    bool devMode_ = false;
    QWebEngineProfile* profile_ = nullptr;
    WebShell* shell_ = nullptr;
};
