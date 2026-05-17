// FloatingDockTitleBar — see floating_dock_titlebar.hpp for overview.

#include "floating_dock_titlebar.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QTimer>
#include <QToolButton>

FloatingDockTitleBar::FloatingDockTitleBar(QDockWidget* dock, QWidget* parent)
    : QWidget(parent ? parent : dock)
    , dock_(dock)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 2, 4, 2);
    layout->setSpacing(2);

    // Title label — fills available space.
    titleLabel_ = new QLabel(dock->windowTitle());
    titleLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(titleLabel_);

    // Dock-back button (▣) — only if the dock supports floating toggle.
    auto features = dock->features();
    if (features & QDockWidget::DockWidgetFloatable) {
        dockBackButton_ = new QToolButton;
        dockBackButton_->setText(QString::fromUtf8("\xe2\x96\xa3"));  // ▣
        dockBackButton_->setToolTip(tr("Dock"));
        dockBackButton_->setAutoRaise(true);
        layout->addWidget(dockBackButton_);

        // Defer setFloating(false) — this widget gets replaced (and deleted)
        // when the dock re-docks. Calling it synchronously crashes.
        connect(dockBackButton_, &QToolButton::clicked, this, [dock]() {
            QTimer::singleShot(0, dock, [dock]() { dock->setFloating(false); });
        });
    }

    // Close button (×) — only if the dock is closable.
    if (features & QDockWidget::DockWidgetClosable) {
        closeButton_ = new QToolButton;
        closeButton_->setText(QString::fromUtf8("\xc3\x97"));  // ×
        closeButton_->setToolTip(tr("Close"));
        closeButton_->setAutoRaise(true);
        layout->addWidget(closeButton_);

        connect(closeButton_, &QToolButton::clicked, dock, &QDockWidget::close);
    }

    // Reactive title — stays in sync when document.title changes.
    connect(dock, &QDockWidget::windowTitleChanged, titleLabel_, &QLabel::setText);

    setStyleSheet(QStringLiteral(
        "FloatingDockTitleBar {"
        "  background-color: palette(window);"
        "  border-bottom: 1px solid palette(mid);"
        "}"
        "QLabel { font-weight: bold; }"
        "QToolButton { border: none; padding: 2px 6px; }"
        "QToolButton:hover { background-color: palette(midlight); }"
    ));
}

// ── Mouse drag ───────────────────────────────────────────────

void FloatingDockTitleBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPos_ = event->globalPosition().toPoint();
        dragging_ = true;
    }
    QWidget::mousePressEvent(event);
}

void FloatingDockTitleBar::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->globalPosition().toPoint() - dragStartPos_;
        dock_->move(dock_->pos() + delta);
        dragStartPos_ = event->globalPosition().toPoint();
    }
    QWidget::mouseMoveEvent(event);
}

void FloatingDockTitleBar::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        dragging_ = false;
    QWidget::mouseReleaseEvent(event);
}

void FloatingDockTitleBar::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Defer — this widget gets deleted when the dock re-docks.
        QDockWidget* dock = dock_;
        QTimer::singleShot(0, dock, [dock]() { dock->setFloating(false); });
    }
    QWidget::mouseDoubleClickEvent(event);
}
