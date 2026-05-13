// app_shell::Tray — see tray.hpp for overview.

#include "shell/tray.hpp"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

#include "shell/app.hpp"

namespace app_shell {

Tray::Tray(App* parent)
    : QObject(parent), app_(parent)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    icon_ = new QSystemTrayIcon(QIcon(parent->iconPath()), this);
    icon_->setToolTip(QApplication::applicationName());

    menu_ = new QMenu;  // no parent — Tray (QObject) can't be a QWidget parent
    icon_->setContextMenu(menu_);

    connect(icon_, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger ||
            reason == QSystemTrayIcon::DoubleClick) {
            emit activated();
        }
    });
}

Tray::~Tray() {
    delete menu_;
}

QAction* Tray::addItem(const QString& label, std::function<void()> onTriggered) {
    if (!menu_) return nullptr;
    auto* action = menu_->addAction(label);
    connect(action, &QAction::triggered, this, [cb = std::move(onTriggered)]{ cb(); });
    return action;
}

QAction* Tray::addLabel(const QString& label) {
    if (!menu_) return nullptr;
    auto* action = menu_->addAction(label);
    action->setEnabled(false);
    return action;
}

void Tray::addSeparator() {
    if (menu_) menu_->addSeparator();
}

QMenu* Tray::addMenu(const QString& label) {
    if (!menu_) return nullptr;
    return menu_->addMenu(label);
}

void Tray::show() {
    if (icon_) icon_->show();
}

bool Tray::isAvailable() const {
    return icon_ != nullptr;
}

}  // namespace app_shell
