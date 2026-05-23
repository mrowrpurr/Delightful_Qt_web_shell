// DevToolsShortcut — see devtools_shortcut.hpp for overview.

#include "devtools_shortcut.hpp"
#include "icon_utils.hpp"

#include <QMenu>

DevToolsShortcut::DevToolsShortcut(QWidget* parent, QMenu* menu)
    : QObject(parent)
{
    action_ = new QAction(tintedIcon(Icons16::Navigation_Settings),
                          tr("&Developer Tools"), this);
    action_->setShortcut(QKeySequence("F12"));

    if (menu)
        menu->addAction(action_);
}
