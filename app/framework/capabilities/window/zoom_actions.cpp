// app_shell::ZoomActions — see zoom_actions.hpp for overview.

#include "zoom_actions.hpp"
#include "main_window.hpp"
#include "app.hpp"
#include "style_manager.hpp"
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
        disconnect(inConn_);
        disconnect(outConn_);
        disconnect(resetConn_);

        if (!dock || !dock->widget()) return;
        auto* view = dock->widget()->findChild<QWebEngineView*>();
        if (!view) return;

        inConn_ = connect(in_, &QAction::triggered, view, [view]() {
            view->setZoomFactor(qMin(view->zoomFactor() + 0.1, 5.0));
        });
        outConn_ = connect(out_, &QAction::triggered, view, [view]() {
            view->setZoomFactor(qMax(view->zoomFactor() - 0.1, 0.25));
        });
        resetConn_ = connect(reset_, &QAction::triggered, view, [view]() {
            view->setZoomFactor(1.0);
        });
    };

    connect(parent, &MainWindow::activeDockChanged, this, wire);

    if (parent->activeDock())
        wire(parent->activeDock());

    // Retint icons on theme change.
    if (auto* sm = parent->app().styleManager()) {
        auto retint = [this, sm]() {
            QColor c = sm->isDarkMode() ? Qt::white : QColor(40, 40, 40);
            in_->setIcon(tintedIcon(Icons16::Action_ZoomIn, c));
            out_->setIcon(tintedIcon(Icons16::Action_ZoomOut, c));
            reset_->setIcon(tintedIcon(Icons16::Action_ZoomOriginal, c));
        };
        connect(sm, &StyleManager::themeChanged, this, retint);
    }
}

}  // namespace app_shell
