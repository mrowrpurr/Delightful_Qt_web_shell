// app_shell::ZoomActions — see zoom_actions.hpp for overview.

#include "zoom_actions.hpp"
#include "main_window.hpp"
#include "icon_utils.hpp"

#include <QAction>
#include <QDockWidget>
#include <QKeySequence>
#include <QMenu>
#include <QWebEngineView>

namespace app_shell {

ZoomActions::ZoomActions(MainWindow* parent, QMenu* menu)
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

    auto wire = [this](QDockWidget* dock) {
        in_->disconnect(SIGNAL(triggered()));
        out_->disconnect(SIGNAL(triggered()));
        reset_->disconnect(SIGNAL(triggered()));

        if (!dock || !dock->widget()) return;
        auto* view = dock->widget()->findChild<QWebEngineView*>();
        if (!view) return;

        connect(in_, &QAction::triggered, view, [view]() {
            view->setZoomFactor(qMin(view->zoomFactor() + 0.1, 5.0));
        });
        connect(out_, &QAction::triggered, view, [view]() {
            view->setZoomFactor(qMax(view->zoomFactor() - 0.1, 0.25));
        });
        connect(reset_, &QAction::triggered, view, [view]() {
            view->setZoomFactor(1.0);
        });
    };

    connect(parent, &MainWindow::activeDockChanged, this, wire);

    if (parent->activeDock())
        wire(parent->activeDock());
}

}  // namespace app_shell
