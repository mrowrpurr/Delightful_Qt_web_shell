// app_shell::ZoomActions — see zoom_actions.hpp for overview.

#include "zoom_actions.hpp"
#include "icon_utils.hpp"

#include <QAction>
#include <QKeySequence>
#include <QMenu>

namespace app_shell {

ZoomActions::ZoomActions(QWidget* parent, QMenu* menu)
    : QObject(parent)
{
    in_ = new QAction(tintedIcon(Icons16::Action_ZoomIn), tr("Zoom &In"), this);
    in_->setShortcuts({QKeySequence::ZoomIn, QKeySequence("Ctrl+=")});

    out_ = new QAction(tintedIcon(Icons16::Action_ZoomOut), tr("Zoom &Out"), this);
    out_->setShortcut(QKeySequence::ZoomOut);

    reset_ = new QAction(tintedIcon(Icons16::Action_ZoomOriginal), tr("&Reset Zoom"), this);
    reset_->setShortcut(QKeySequence("Ctrl+0"));

    menu->addAction(in_);
    menu->addAction(out_);
    menu->addAction(reset_);

    // Callback wiring happens in MainWindow::wireToActiveDock(),
    // which finds us via findChild<ZoomActions*>() and connects
    // the actions to the active QWebEngineView's zoomFactor.
}

}  // namespace app_shell
