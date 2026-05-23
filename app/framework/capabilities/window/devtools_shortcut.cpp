// DevToolsShortcut — see devtools_shortcut.hpp for overview.

#include "devtools_shortcut.hpp"
#include "main_window.hpp"
#include "icon_utils.hpp"

#include <QKeySequence>
#include <QMenu>

DevToolsShortcut::DevToolsShortcut(QWidget* parent, QMenu* menu)
    : QObject(parent)
{
    action_ = new QAction(tintedIcon(Icons16::Navigation_Settings),
                          tr("&Developer Tools"), this);
    action_->setShortcut(QKeySequence("F12"));

    if (menu)
        menu->addAction(action_);

    // Wire to active dock changes.
    auto* win = qobject_cast<MainWindow*>(parent);
    if (win) {
        connect(win, &MainWindow::activeDockChanged, this, &DevToolsShortcut::wireToDock);

        if (win->activeDock())
            wireToDock(win->activeDock());
    }
}

void DevToolsShortcut::wireToDock(QDockWidget* dock) {
    action_->disconnect(SIGNAL(triggered()));
    if (!dock || !dock->widget()) return;
    auto* widget = dock->widget();
    connect(action_, &QAction::triggered, widget, [widget]() {
        QMetaObject::invokeMethod(widget, "toggleDevTools");
    });
}
