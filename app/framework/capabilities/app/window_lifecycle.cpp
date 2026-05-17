// app_shell::WindowLifecycle — see window_lifecycle.hpp for overview.

#include "window_lifecycle.hpp"

#include <QApplication>
#include <QSettings>
#include <QStringList>
#include <QWidget>

#include "app.hpp"
#include "main_window.hpp"

namespace app_shell {

WindowLifecycle::WindowLifecycle(App* parent)
    : QObject(parent), app_(parent)
{}

QList<MainWindow*> WindowLifecycle::restoreWindows() {
    QSettings s(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);

    s.beginGroup("window");
    QStringList windowIds = s.childGroups();
    s.endGroup();

    QList<MainWindow*> windows;
    for (const auto& id : windowIds)
        windows.append(new MainWindow(*app_, id));

    return windows;
}

int WindowLifecycle::visibleWindowCount() const {
    int count = 0;
    for (auto* w : QApplication::topLevelWidgets()) {
        if (auto* mw = qobject_cast<MainWindow*>(w))
            if (mw->isVisible()) ++count;
    }
    return count;
}

}  // namespace app_shell
