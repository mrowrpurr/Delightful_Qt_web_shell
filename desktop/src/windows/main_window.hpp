// MainWindow — the primary application window.
//
// This is thin on purpose. It wires together:
//   - menu bar   (from menus/)
//   - tool bar   (from menus/)
//   - status bar (from widgets/)
//   - QSplitter with two WebShellWidgets (main app + docs app)
//
// Business logic, bridges, and app-level concerns live in Application.
// Window-level concerns (geometry, zoom) live here.

#pragma once

#include <QMainWindow>

class StatusBar;
class WebShellWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    // Override close to minimize to system tray instead of quitting.
    // Quit via File > Quit, Ctrl+Q, or the tray icon's Quit action.
    // To disable close-to-tray: remove this override.
    void closeEvent(QCloseEvent* event) override;

private:
    WebShellWidget* mainApp_ = nullptr;
    WebShellWidget* docsApp_ = nullptr;
    StatusBar* statusBar_ = nullptr;
};
