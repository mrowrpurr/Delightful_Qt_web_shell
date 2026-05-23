// app_shell::ZoomActions — zoom in/out/reset for a QWebEngineView.
//
// Parented to a window, adds its actions to the given menu, and wires
// zoom to the active QWebEngineView. MainWindow::wireToActiveDock()
// finds this capability via findChild and reconnects on dock switch.

#pragma once

#include <QObject>

class QAction;
class QMenu;
class QWidget;

namespace app_shell {

class ZoomActions : public QObject {
    Q_OBJECT

public:
    explicit ZoomActions(QWidget* parent, QMenu* menu);

    QAction* inAction() const { return in_; }
    QAction* outAction() const { return out_; }
    QAction* resetAction() const { return reset_; }

private:
    QAction* in_    = nullptr;
    QAction* out_   = nullptr;
    QAction* reset_ = nullptr;
};

}  // namespace app_shell
