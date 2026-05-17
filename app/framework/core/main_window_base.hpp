// MainWindowBase — bare QMainWindow with only an App& reference.
//
// No widgets, no behavior installed. Consumers who want full control
// inherit from this and compose their own subsystems. For the typical
// case, use MainWindow (inherits this, installs the standard set).

#pragma once

#include <QMainWindow>

namespace app_shell { class App; }

class MainWindowBase : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindowBase(app_shell::App& app, QWidget* parent = nullptr)
        : QMainWindow(parent), app_(app) {}

    app_shell::App& app() const { return app_; }

private:
    app_shell::App& app_;
};
