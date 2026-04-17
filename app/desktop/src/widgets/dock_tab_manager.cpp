// DockTabManager — see dock_tab_manager.hpp for overview.

#include "dock_tab_manager.hpp"
#include "floating_dock_titlebar.hpp"

#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPointer>
#include <QTabBar>
#include <QTimer>
#include <QCursor>

// Margin (px) outside the tab bar rect before a drag-to-undock triggers.
static constexpr int kDragMargin = 40;

DockTabManager::DockTabManager(QMainWindow* window)
    : QObject(window)
    , window_(window)
{
    // LayoutRequest on the MainWindow fires when dock layout changes
    // (tabification, splits, etc.) — no dedicated signal for these.
    window_->installEventFilter(this);
}

// ── Manage / unmanage ────────────────────────────────────────

void DockTabManager::manage(QDockWidget* dock) {
    dock->installEventFilter(this);

    connect(dock, &QDockWidget::topLevelChanged,
            this, [this](bool) { scheduleUpdate(); });

    connect(dock, &QDockWidget::dockLocationChanged,
            this, [this](Qt::DockWidgetArea) { scheduleUpdate(); });

    // Set initial title bar state (deferred — dock may not be laid out yet).
    scheduleUpdate();
}

void DockTabManager::unmanage(QDockWidget* dock) {
    dock->removeEventFilter(this);
    disconnect(dock, nullptr, this, nullptr);
}

void DockTabManager::manageTabBar(QTabBar* tabBar) {
    tabBar->installEventFilter(this);
}

// ── Title bar state machine ──────────────────────────────────

void DockTabManager::updateTitleBar(QDockWidget* dock) {
    if (dock->isFloating()) {
        // Don't swap the title bar while the user is mid-drag — we'd yank
        // the drag surface out from under their mouse. Re-check after a
        // short delay so we catch the release.
        if (QApplication::mouseButtons() & Qt::LeftButton) {
            QPointer<QDockWidget> guard(dock);
            QTimer::singleShot(100, this, [this, guard]() {
                if (guard && guard->isFloating())
                    updateTitleBar(guard);
            });
            return;
        }
        showFloatingTitleBar(dock);
        return;
    }

    // Check if this dock is tabified with others.
    bool tabified = !window_->tabifiedDockWidgets(dock).isEmpty();

    // Also check if this dock is the "first" in a tab group — tabifiedDockWidgets
    // returns siblings but not the dock itself. Another dock might list us as tabified.
    if (!tabified) {
        for (auto* other : window_->findChildren<QDockWidget*>()) {
            if (other != dock && !other->isFloating()
                && window_->tabifiedDockWidgets(other).contains(dock)) {
                tabified = true;
                break;
            }
        }
    }

    if (tabified)
        hideTitleBar(dock);
    else
        showNativeTitleBar(dock);
}

void DockTabManager::updateAllTitleBars() {
    updateScheduled_ = false;
    wireNewTabBars();
    for (auto* dock : window_->findChildren<QDockWidget*>()) {
        if (dock->parent() == window_ || dock->isFloating())
            updateTitleBar(dock);
    }
}

void DockTabManager::wireNewTabBars() {
    for (auto* tabBar : window_->findChildren<QTabBar*>()) {
        if (tabBar->parent() != window_) continue;
        if (tabBar->property("_tabManagerWired").toBool()) continue;
        tabBar->installEventFilter(this);
        tabBar->setProperty("_tabManagerWired", true);
    }
}

void DockTabManager::scheduleUpdate() {
    if (updateScheduled_) return;
    updateScheduled_ = true;
    QTimer::singleShot(0, this, &DockTabManager::updateAllTitleBars);
}

// ── Hide / show title bars ───────────────────────────────────

void DockTabManager::hideTitleBar(QDockWidget* dock) {
    if (dock->property("_titleBarHidden").toBool())
        return;

    auto* placeholder = new QWidget;
    placeholder->setFixedHeight(0);
    dock->setTitleBarWidget(placeholder);
    dock->setProperty("_titleBarHidden", true);
}

void DockTabManager::showFloatingTitleBar(QDockWidget* dock) {
    // Already has a FloatingDockTitleBar — nothing to do.
    if (qobject_cast<FloatingDockTitleBar*>(dock->titleBarWidget()))
        return;

    auto* titleBar = new FloatingDockTitleBar(dock);
    dock->setTitleBarWidget(titleBar);
    dock->setProperty("_titleBarHidden", false);
}

void DockTabManager::showNativeTitleBar(QDockWidget* dock) {
    if (!dock->property("_titleBarHidden").toBool()
        && !qobject_cast<FloatingDockTitleBar*>(dock->titleBarWidget()))
        return;  // Already native.

    // Restore Qt's default title bar.
    dock->setTitleBarWidget(nullptr);
    dock->setProperty("_titleBarHidden", false);
}

// ── Event filter ─────────────────────────────────────────────

bool DockTabManager::eventFilter(QObject* obj, QEvent* event) {
    // ── LayoutRequest on MainWindow — tab groups changed ─────
    if (obj == window_ && event->type() == QEvent::LayoutRequest) {
        scheduleUpdate();
        return false;
    }

    // ── Tab bar events — drag-to-undock ──────────────────────
    if (auto* tabBar = qobject_cast<QTabBar*>(obj)) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                int idx = tabBar->tabAt(me->pos());
                if (idx >= 0) {
                    dragTabBar_ = tabBar;
                    dragTabIndex_ = idx;
                    dragTabTitle_ = tabBar->tabText(idx);
                }
            }
            break;
        }
        case QEvent::MouseMove: {
            if (dragTabBar_ == tabBar && dragTabIndex_ >= 0) {
                auto* me = static_cast<QMouseEvent*>(event);
                QRect hotzone = tabBar->rect().adjusted(
                    -kDragMargin, -kDragMargin, kDragMargin, kDragMargin);
                if (!hotzone.contains(me->pos())) {
                    // Mouse left the tab bar area — undock.
                    QPointer<QTabBar> bar = dragTabBar_;
                    int idx = dragTabIndex_;
                    dragTabBar_ = nullptr;
                    dragTabIndex_ = -1;
                    QTimer::singleShot(0, this, [this, bar, idx]() {
                        if (bar) undockTab(bar, idx);
                    });
                    return true;  // Consume the event.
                }
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        case QEvent::Leave:
            dragTabBar_ = nullptr;
            dragTabIndex_ = -1;
            dragTabTitle_.clear();
            break;
        default:
            break;
        }
    }

    return QObject::eventFilter(obj, event);
}

// ── Drag-to-undock ───────────────────────────────────────────

void DockTabManager::undockTab(QTabBar* tabBar, int tabIndex) {
    if (!tabBar || tabIndex < 0 || tabIndex >= tabBar->count())
        return;

    QString title = tabBar->tabText(tabIndex);

    // Find the QDockWidget with this title.
    QDockWidget* target = nullptr;
    for (auto* dock : window_->findChildren<QDockWidget*>()) {
        if (dock->windowTitle() == title) {
            target = dock;
            break;
        }
    }
    if (!target) return;

    // The remove → add → float sequence is necessary on Windows.
    // Just setFloating(true) doesn't properly detach from the tab group.
    window_->removeDockWidget(target);
    window_->addDockWidget(Qt::RightDockWidgetArea, target);
    target->setFloating(true);
    target->show();

    // Position near the cursor so it feels natural.
    target->move(QCursor::pos() - QPoint(50, 15));

    scheduleUpdate();
}
