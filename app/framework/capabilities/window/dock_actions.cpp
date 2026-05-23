// app_shell::DockActions — see dock_actions.hpp for overview.

#include "dock_actions.hpp"
#include "main_window.hpp"
#include "app.hpp"
#include "dock_manager.hpp"

#include <QAction>
#include <QKeySequence>
#include <QMenu>

namespace app_shell {

DockActions::DockActions(QWidget* parent, QMenu* menu)
    : QObject(parent)
{
    auto* win = qobject_cast<MainWindow*>(parent);
    auto* dm = win->app().dockManager();

    newTab_ = new QAction(tr("&New Tab"), this);
    newTab_->setShortcut(QKeySequence("Ctrl+T"));
    connect(newTab_, &QAction::triggered, win, [win, dm]() {
        auto* dock = dm->createDock({}, win);
        dock->raise();
        dock->setFocus();
    });

    closeTab_ = new QAction(tr("&Close Tab"), this);
    closeTab_->setShortcut(QKeySequence("Ctrl+W"));
    connect(closeTab_, &QAction::triggered, win, [win, dm]() {
        if (win->activeDock())
            dm->closeDock(win->activeDock());
    });

    menu->addAction(newTab_);
    menu->addAction(closeTab_);
}

}  // namespace app_shell
