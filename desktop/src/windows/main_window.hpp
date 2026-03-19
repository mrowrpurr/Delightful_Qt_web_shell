// MainWindow — the primary application window.
//
// This is thin on purpose. It wires together:
//   - menu bar   (from menus/)
//   - tool bar   (from menus/)
//   - status bar (from widgets/)
//   - central widget (WebShellWidget — React app with bridges)
//
// Business logic, bridges, and app-level concerns live in Application.
// Window-level concerns (geometry, zoom) live here.

#pragma once

#include <QMainWindow>

class WebShellWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    WebShellWidget* webShell_ = nullptr;
};
