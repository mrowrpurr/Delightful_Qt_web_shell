// Menu bar setup — builds the main window's menus.
//
// Menus live here, not in MainWindow, so the window file stays short and
// you can see all menu structure in one place. Add new menus and actions here.
//
// Toolbar actions are defined here too — they're often the same actions that
// appear in menus (e.g. Save, Open), just exposed as buttons.

#pragma once

class QAction;
class QMainWindow;

// Actions that the caller may need to wire up to other widgets.
// buildMenuBar() creates them; MainWindow connects them.
struct MenuActions {
    QAction* zoomIn     = nullptr;
    QAction* zoomOut    = nullptr;
    QAction* zoomReset  = nullptr;
    QAction* devTools   = nullptr;
};

// Builds the full menu bar: File, View, Windows, Help.
// Returns actions that need wiring to widgets (zoom, devtools).
MenuActions buildMenuBar(QMainWindow* window);

// Builds the main toolbar with common actions.
void buildToolBar(QMainWindow* window);
