// DemoWindow — the demo app's window, subclassing MainWindow.
//
// Builds its own menu bar and toolbar with:
// - Framework capabilities (ZoomActions, DockActions, DevToolsShortcut)
// - Demo-specific items (React Dialog, Demo Widget, About, Save, Open Folder)

#pragma once

#include "main_window.hpp"

namespace app_shell { class App; }

class DemoWindow : public MainWindow {
    Q_OBJECT

public:
    explicit DemoWindow(app_shell::App& app, const QString& windowId = {},
                        QWidget* parent = nullptr);
};
