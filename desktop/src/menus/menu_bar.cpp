// Menu bar + toolbar setup.
//
// All menu structure lives here. MainWindow just calls buildMenuBar() and
// buildToolBar() — it doesn't need to know what's in them.
//
// The toolbar reuses the SAME QAction objects as the menu bar. One action,
// two places — shortcut, tooltip, enabled state, and signal stay in sync.
//
// To add a new menu:
//   auto* myMenu = menuBar->addMenu("&MyMenu");
//   auto* myAction = myMenu->addAction("&Do Thing");
//   myAction->setShortcut(QKeySequence("Ctrl+D"));
//   QObject::connect(myAction, &QAction::triggered, window, [window]() { ... });
//
// To add a toolbar button for an existing menu action:
//   toolBar->addAction(actions.myAction);

#include "menu_bar.hpp"
#include "dialogs/about_dialog.hpp"
#include "dialogs/web_dialog.hpp"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyle>
#include <QToolBar>

MenuActions buildMenuBar(QMainWindow* window) {
    auto* menuBar = window->menuBar();
    MenuActions out;

    // ── File ─────────────────────────────────────────────────
    auto* fileMenu = menuBar->addMenu("&File");

    // File > Save — native file picker (testable with pywinauto)
    out.save = fileMenu->addAction(
        window->style()->standardIcon(QStyle::SP_DialogSaveButton), "&Save...");
    out.save->setShortcut(QKeySequence("Ctrl+S"));
    out.save->setToolTip("Save file (Ctrl+S)");
    QObject::connect(out.save, &QAction::triggered, window, [window]() {
        QString path = QFileDialog::getSaveFileName(
            window, "Save File", "", "JSON Files (*.json);;All Files (*)");
        if (!path.isEmpty())
            QMessageBox::information(window, "Save", "You selected file: " + path);
    });

    // File > Open Folder — native folder picker
    out.openFolder = fileMenu->addAction(
        window->style()->standardIcon(QStyle::SP_DirOpenIcon), "&Open Folder...");
    out.openFolder->setShortcut(QKeySequence("Ctrl+O"));
    out.openFolder->setToolTip("Open folder (Ctrl+O)");
    QObject::connect(out.openFolder, &QAction::triggered, window, [window]() {
        QString path = QFileDialog::getExistingDirectory(
            window, "Open Folder", "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            QMessageBox::information(window, "Open Folder", "You selected folder: " + path);
    });

    fileMenu->addSeparator();

    auto* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    QObject::connect(quitAction, &QAction::triggered,
                     QApplication::instance(), &QApplication::quit);

    // ── View ─────────────────────────────────────────────────
    auto* viewMenu = menuBar->addMenu("&View");

    // Zoom In — Ctrl+= and Ctrl+Shift+= (Ctrl++)
    out.zoomIn = viewMenu->addAction("Zoom &In");
    out.zoomIn->setShortcuts({QKeySequence::ZoomIn, QKeySequence("Ctrl+=")});
    out.zoomIn->setShortcutContext(Qt::ApplicationShortcut);

    // Zoom Out — Ctrl+-
    out.zoomOut = viewMenu->addAction("Zoom &Out");
    out.zoomOut->setShortcut(QKeySequence::ZoomOut);
    out.zoomOut->setShortcutContext(Qt::ApplicationShortcut);

    // Reset Zoom — Ctrl+0
    out.zoomReset = viewMenu->addAction("&Reset Zoom");
    out.zoomReset->setShortcut(QKeySequence("Ctrl+0"));
    out.zoomReset->setShortcutContext(Qt::ApplicationShortcut);

    // ── Windows ──────────────────────────────────────────────
    auto* windowsMenu = menuBar->addMenu("&Windows");

    out.devTools = windowsMenu->addAction("&Developer Tools");
    out.devTools->setShortcut(QKeySequence("F12"));
    out.devTools->setShortcutContext(Qt::ApplicationShortcut);

    windowsMenu->addSeparator();

    // React-in-a-dialog — demonstrates WebShellWidget inside a QDialog.
    // Same bridges, same React app, different container.
    auto* webDialogAction = windowsMenu->addAction("&React Dialog...");
    QObject::connect(webDialogAction, &QAction::triggered, window, [window]() {
        WebDialog dlg(window);
        dlg.exec();
    });

    // ── Help ─────────────────────────────────────────────────
    auto* helpMenu = menuBar->addMenu("&Help");

    auto* aboutAction = helpMenu->addAction("&About");
    QObject::connect(aboutAction, &QAction::triggered, window, [window]() {
        AboutDialog dlg(window);
        dlg.exec();
    });

    return out;
}

void buildToolBar(QMainWindow* window, const MenuActions& actions) {
    // Main toolbar — reuses QAction objects from the menu bar.
    // Same action = same shortcut, tooltip, enabled state, and signal.
    // No duplicate connections needed — click the toolbar button or the menu
    // item, same thing fires.

    auto* toolBar = window->addToolBar("Main");
    toolBar->setObjectName("MainToolBar");  // QSettings needs a stable name to save state
    toolBar->setMovable(false);             // Docking is a topic for another day

    toolBar->addAction(actions.save);
    toolBar->addAction(actions.openFolder);
}
