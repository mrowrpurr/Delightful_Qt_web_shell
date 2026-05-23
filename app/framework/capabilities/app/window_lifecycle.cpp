// app_shell::WindowLifecycle — see window_lifecycle.hpp for overview.

#include "window_lifecycle.hpp"

#include <QApplication>
#include <QWidget>

#include "main_window.hpp"

namespace app_shell {

int WindowLifecycle::visibleWindowCount() const {
    int count = 0;
    for (auto* w : QApplication::topLevelWidgets()) {
        if (auto* mw = qobject_cast<MainWindow*>(w))
            if (mw->isVisible()) ++count;
    }
    return count;
}

}  // namespace app_shell
