// Theming — opt-in app-scoped subsystem owning the full theme system.
//
// Constructs StyleManager, applies a baseline theme, sets the dark palette,
// and wires StyleManager ↔ ThemeBridge for React ↔ Qt theme sync.
//
// Consumer who skips constructing Theming gets no StyleManager, no libsass,
// no file watcher, no ThemeBridge wiring.

#pragma once

#include <QObject>
#include <QString>

class StyleManager;

namespace app_shell {
class App;

class Theming : public QObject {
    Q_OBJECT

public:
    explicit Theming(App& app, const QString& baseline = "default-dark");

    StyleManager* styleManager() const { return sm_; }

private:
    StyleManager* sm_ = nullptr;
};

} // namespace app_shell
