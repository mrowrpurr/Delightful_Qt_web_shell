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
#include <QWheelEvent>

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

    // Rewire to the active dock's QWebEngineView on every dock switch.
    connect(parent, &MainWindow::activeDockChanged, this, [this](QDockWidget* dock) {
        QWebEngineView* view = nullptr;
        if (dock && dock->widget())
            view = dock->widget()->findChild<QWebEngineView*>();
        wireToView(view);
    });

    if (parent->activeDock() && parent->activeDock()->widget()) {
        auto* view = parent->activeDock()->widget()->findChild<QWebEngineView*>();
        wireToView(view);
    }

    // Retint icons on theme change.
    if (auto* sm = parent->app().styleManager()) {
        connect(sm, &StyleManager::themeChanged, this, [this, sm]() {
            QColor c = sm->isDarkMode() ? Qt::white : QColor(40, 40, 40);
            in_->setIcon(tintedIcon(Icons16::Action_ZoomIn, c));
            out_->setIcon(tintedIcon(Icons16::Action_ZoomOut, c));
            reset_->setIcon(tintedIcon(Icons16::Action_ZoomOriginal, c));
        });
    }
}

void ZoomActions::wireToView(QWebEngineView* view) {
    // Disconnect previous action connections.
    disconnect(inConn_);
    disconnect(outConn_);
    disconnect(resetConn_);

    // Remove event filter from previous view's focus proxy.
    if (activeView_ && activeView_->focusProxy())
        activeView_->focusProxy()->removeEventFilter(this);

    activeView_ = view;

    if (!view) return;

    // Connect menu/shortcut actions.
    inConn_ = connect(in_, &QAction::triggered, view, [view]() {
        view->setZoomFactor(qMin(view->zoomFactor() + 0.1, 5.0));
    });
    outConn_ = connect(out_, &QAction::triggered, view, [view]() {
        view->setZoomFactor(qMax(view->zoomFactor() - 0.1, 0.25));
    });
    resetConn_ = connect(reset_, &QAction::triggered, view, [view]() {
        view->setZoomFactor(1.0);
    });

    // Install event filter on the focus proxy for Ctrl/Cmd+scroll zoom.
    // QWebEngineView delivers input events through its focusProxy(), not
    // the view itself — without this, wheelEvent never fires.
    if (view->focusProxy())
        view->focusProxy()->installEventFilter(this);
}

bool ZoomActions::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Wheel && activeView_) {
        auto* we = static_cast<QWheelEvent*>(event);

        // Ctrl on Windows/Linux, Cmd on macOS.
#ifdef Q_OS_MAC
        bool zoomModifier = we->modifiers().testFlag(Qt::MetaModifier);
#else
        bool zoomModifier = we->modifiers().testFlag(Qt::ControlModifier);
#endif

        if (zoomModifier) {
            int deltaY = we->angleDelta().y();
            if (deltaY != 0) {
                qreal factor = activeView_->zoomFactor();
                factor += (deltaY > 0) ? 0.1 : -0.1;
                activeView_->setZoomFactor(qBound(0.25, factor, 5.0));
            }
            event->accept();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

}  // namespace app_shell
