// app_shell::WindowRegistry — see window_registry.hpp for overview.

#include "shell/window_registry.hpp"

#include <QApplication>
#include <QSettings>
#include <QStringList>
#include <QWidget>

#include "shell/app.hpp"
#include "windows/main_window.hpp"

namespace app_shell {

WindowRegistry::WindowRegistry(App* parent)
    : QObject(parent), app_(parent)
{}

QList<MainWindow*> WindowRegistry::restoreWindows() {
    QSettings s(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);

    s.beginGroup("window");
    QStringList windowIds = s.childGroups();
    s.endGroup();

    QList<MainWindow*> windows;
    for (const auto& id : windowIds)
        windows.append(new MainWindow(*app_, id));

    return windows;
}

int WindowRegistry::visibleWindowCount() const {
    int count = 0;
    for (auto* w : QApplication::topLevelWidgets()) {
        if (auto* mw = qobject_cast<MainWindow*>(w))
            if (mw->isVisible()) ++count;
    }
    return count;
}

}  // namespace app_shell
