// app_shell::Tray — a thin wrapper around QSystemTrayIcon + its context menu.
//
// Constructed by the consumer as a child of App. Add items, separators, and
// submenus to the top-level tray menu; nest deeper by calling addMenu() on the
// QMenu* returned. The tray is silent on platforms / sessions without system
// tray support — every mutating call becomes a no-op and show() does nothing.

#pragma once

#include <QObject>
#include <QString>

#include <functional>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace app_shell {

class App;

class Tray : public QObject {
    Q_OBJECT

public:
    explicit Tray(App* parent);
    ~Tray() override;

    // Adds a clickable item to the top-level tray menu. The returned QAction
    // lets the caller toggle enabled/checked/visible state.
    QAction* addItem(const QString& label, std::function<void()> onTriggered);

    // Adds a non-interactive label (e.g. a version row).
    QAction* addLabel(const QString& label);

    void addSeparator();

    // Adds a submenu. The returned QMenu* is owned by the tray menu; nest
    // deeper by calling addMenu() / addAction() on it.
    QMenu* addMenu(const QString& label);

    // Shows the tray icon. No-op when the system has no tray support.
    void show();

    // Returns false when the system has no tray support — every other method
    // on this class is a no-op in that case.
    bool isAvailable() const;

    // Escape hatch for direct QSystemTrayIcon access (e.g. showMessage).
    // Returns nullptr when !isAvailable().
    QSystemTrayIcon* qIcon() const { return icon_; }

signals:
    // Emitted on single- or double-click of the tray icon itself (not the
    // context menu). Wire this to whatever the host's "activate / raise"
    // behavior is.
    void activated();

private:
    App* app_;
    QSystemTrayIcon* icon_ = nullptr;
    QMenu* menu_ = nullptr;
};

}  // namespace app_shell
