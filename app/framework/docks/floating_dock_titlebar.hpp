// FloatingDockTitleBar — custom title bar for floating dock widgets.
//
// Replaces the native OS title bar when a dock is floating. Provides:
//   - Title label (reactive via windowTitleChanged)
//   - Dock-back button (▣) — re-docks the widget
//   - Close button (×)
//   - Mouse drag to reposition the floating dock
//   - Double-click to dock back
//
// The native OS title bar swallows double-click events at the window manager
// level, making them impossible to intercept with Qt event filters. This
// custom widget gives us full control.

#pragma once

#include <QDockWidget>
#include <QPoint>
#include <QWidget>

class QLabel;
class QToolButton;

class FloatingDockTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit FloatingDockTitleBar(QDockWidget* dock, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    QDockWidget* dock_;
    QLabel* titleLabel_;
    QToolButton* dockBackButton_ = nullptr;
    QToolButton* closeButton_ = nullptr;
    QPoint dragStartPos_;
    bool dragging_ = false;
};
