// MainWindow — the primary application window.
//
// This is thin on purpose. It wires together:
//   - menu bar   (from menus/)
//   - tool bar   (from menus/)
//   - status bar (from widgets/)
//   - QTabWidget with WebShellWidgets (main app tabs)
//   - QSplitter to show docs alongside
//
// Business logic, bridges, and app-level concerns live in Application.
// Window-level concerns (geometry, zoom, tabs) live here.

#pragma once

#include <QMainWindow>

class QTabWidget;
class StatusBar;
class WebShellWidget;
struct MenuActions;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    // Override close to minimize to system tray instead of quitting.
    // Quit via File > Quit, Ctrl+Q, or the tray icon's Quit action.
    // To disable close-to-tray: remove this override.
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    WebShellWidget* createTab();
    WebShellWidget* currentTab() const;
    void closeTabAt(int index);
    void wireZoomToCurrentTab(const MenuActions& actions);

    QTabWidget* tabs_ = nullptr;
    WebShellWidget* docsApp_ = nullptr;
    StatusBar* statusBar_ = nullptr;
};
