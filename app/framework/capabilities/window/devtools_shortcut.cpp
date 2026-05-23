// DevToolsShortcut — see devtools_shortcut.hpp for overview.

#include "devtools_shortcut.hpp"
#include "main_window.hpp"
#include "app.hpp"
#include "style_manager.hpp"
#include "icon_utils.hpp"

#include <QKeySequence>
#include <QMenu>

DevToolsShortcut::DevToolsShortcut(MainWindow* parent, QMenu* menu)
    : QObject(parent)
{
    action_ = new QAction(tintedIcon(Icons16::Navigation_Settings),
                          tr("&Developer Tools"), this);
    action_->setShortcut(QKeySequence("F12"));

    if (menu)
        menu->addAction(action_);

    connect(parent, &MainWindow::activeDockChanged, this, &DevToolsShortcut::wireToDock);

    if (parent->activeDock())
        wireToDock(parent->activeDock());

    // Retint icon on theme change.
    if (auto* sm = parent->app().styleManager()) {
        connect(sm, &StyleManager::themeChanged, this, [this, sm]() {
            QColor c = sm->isDarkMode() ? Qt::white : QColor(40, 40, 40);
            action_->setIcon(tintedIcon(Icons16::Navigation_Settings, c));
        });
    }
}

void DevToolsShortcut::wireToDock(QDockWidget* dock) {
    disconnect(actionConn_);
    if (!dock || !dock->widget()) return;
    auto* widget = dock->widget();
    actionConn_ = connect(action_, &QAction::triggered, widget, [widget]() {
        QMetaObject::invokeMethod(widget, "toggleDevTools");
    });
}
