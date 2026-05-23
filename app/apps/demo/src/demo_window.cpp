// DemoWindow — see demo_window.hpp for overview.

#include "demo_window.hpp"
#include "zoom_actions.hpp"
#include "dock_actions.hpp"
#include "devtools_shortcut.hpp"
#include "icon_utils.hpp"
#include "web_dialog.hpp"
#include "url_protocol.hpp"
#include "system_bridge.hpp"
#include "style_manager.hpp"
#include "app.hpp"
#include "dialogs/about_dialog.hpp"
#include "dialogs/demo_widget_dialog.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QToolButton>

DemoWindow::DemoWindow(app_shell::App& app, const QString& windowId, QWidget* parent)
    : MainWindow(app, windowId, parent)
{
    // ── File ──────────────────────────────────────────────────
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* save = fileMenu->addAction(
        tintedIcon(Icons16::Action_Save), tr("&Save..."));
    save->setShortcut(QKeySequence("Ctrl+S"));
    save->setToolTip("Save file (Ctrl+S)");
    auto* sysBridge = app.bridge<SystemBridge>();
    connect(save, &QAction::triggered, this, [this, sysBridge]() {
        if (sysBridge && sysBridge->has_listeners("saveRequested")) {
            sysBridge->emitSaveRequested();
        } else {
            QString path = QFileDialog::getSaveFileName(
                this, "Save File", "", "JSON Files (*.json);;All Files (*)");
            if (!path.isEmpty())
                QMessageBox::information(this, "Save", "You selected file: " + path);
        }
    });

    auto* openFolder = fileMenu->addAction(
        tintedIcon(Icons16::File_FolderOpen), tr("Open &Folder..."));
    openFolder->setShortcut(QKeySequence("Ctrl+O"));
    openFolder->setToolTip("Open folder (Ctrl+O)");
    connect(openFolder, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(
            this, "Open Folder", "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            QMessageBox::information(this, "Open Folder", "You selected folder: " + path);
    });

    fileMenu->addSeparator();

    auto* newWindow = fileMenu->addAction(tr("New &Window"));
    newWindow->setShortcut(QKeySequence("Ctrl+N"));
    connect(newWindow, &QAction::triggered, this, [&app]() {
        auto* win = new DemoWindow(app);
        win->show();
    });

    new app_shell::DockActions(this, fileMenu);

    fileMenu->addSeparator();

    auto* quit = fileMenu->addAction(tr("&Quit"));
    quit->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quit, &QAction::triggered, &app, [&app]() { app.requestQuit(); });

    // ── View ─────────────────────────────────────────────────
    auto* viewMenu = menuBar()->addMenu("&View");
    new app_shell::ZoomActions(this, viewMenu);

    // ── Windows ──────────────────────────────────────────────
    auto* windowsMenu = menuBar()->addMenu("&Windows");
    new DevToolsShortcut(this, windowsMenu);

    windowsMenu->addSeparator();

    auto* reactDialog = windowsMenu->addAction(tr("&React Dialog..."));
    connect(reactDialog, &QAction::triggered, this, [this, &app]() {
        WebDialog dlg(app, this);
        dlg.exec();
    });

    auto* demoWidget = windowsMenu->addAction(tr("&Demo Widget..."));
    connect(demoWidget, &QAction::triggered, this, []() {
        auto* d = new DemoWidgetDialog(nullptr);
        d->setAttribute(Qt::WA_DeleteOnClose);
        d->show();
    });

    // ── Tools ────────────────────────────────────────────────
    auto* toolsMenu = menuBar()->addMenu("&Tools");
    auto* urlProtocol = app.findChild<app_shell::UrlProtocol*>();
    auto* protocolAction = toolsMenu->addAction("");
    protocolAction->setEnabled(urlProtocol != nullptr);
    auto updateProtocolLabel = [protocolAction, urlProtocol]() {
        if (!urlProtocol) { protocolAction->setText("URL Protocol Unavailable"); return; }
        bool registered = urlProtocol->isRegistered();
        QString protocol = urlProtocol->name();
        protocolAction->setText(registered
            ? QString("Unregister %1:// Protocol").arg(protocol)
            : QString("Register %1:// Protocol").arg(protocol));
    };
    updateProtocolLabel();
    connect(protocolAction, &QAction::triggered, this,
            [urlProtocol, updateProtocolLabel]() {
        if (!urlProtocol) return;
        if (urlProtocol->isRegistered())
            urlProtocol->unregisterProtocol();
        else
            urlProtocol->registerProtocol();
        updateProtocolLabel();
    });

    // ── Help ─────────────────────────────────────────────────
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* about = helpMenu->addAction(tr("&About"));
    connect(about, &QAction::triggered, this, [this, &app]() {
        AboutDialog dlg(app.brandingImagePath(), this);
        dlg.exec();
    });

    // ── Toolbar ──────────────────────────────────────────────
    auto* toolBar = addToolBar("Main");
    toolBar->setObjectName("MainToolBar");
    toolBar->setMovable(false);

    toolBar->addAction(save);
    toolBar->addAction(openFolder);

    if (app.styleManager()) {
        toolBar->addSeparator();

        auto* themeLabel = new QLabel(" Theme: ");
        toolBar->addWidget(themeLabel);

        auto* themeCombo = new QComboBox;
        themeCombo->setEditable(true);
        themeCombo->setInsertPolicy(QComboBox::NoInsert);
        themeCombo->setMinimumWidth(250);
        themeCombo->setMaxVisibleItems(20);

        QStringList baseThemes = app.styleManager()->availableBaseThemes();
        themeCombo->addItems(baseThemes);

        QString currentBase = app.styleManager()->currentBaseName();
        int idx = baseThemes.indexOf(currentBase);
        if (idx >= 0) themeCombo->setCurrentIndex(idx);

        auto* completer = new QCompleter(baseThemes, themeCombo);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        themeCombo->setCompleter(completer);

        connect(themeCombo, &QComboBox::activated,
                this, [&app, themeCombo]() {
            QString baseName = themeCombo->currentText();
            if (!baseName.isEmpty())
                app.styleManager()->applyTheme(baseName, app.styleManager()->isDarkMode());
        });

        toolBar->addWidget(themeCombo);

        auto* darkToggle = new QToolButton;
        darkToggle->setCheckable(true);
        darkToggle->setChecked(app.styleManager()->isDarkMode());
        darkToggle->setText(app.styleManager()->isDarkMode() ? "🌙" : "☀️");
        darkToggle->setToolTip("Toggle dark/light mode");

        connect(darkToggle, &QToolButton::clicked,
                this, [&app, darkToggle]() {
            app.styleManager()->toggleDarkMode();
            darkToggle->setChecked(app.styleManager()->isDarkMode());
            darkToggle->setText(app.styleManager()->isDarkMode() ? "🌙" : "☀️");
        });

        // Retint all icon-bearing actions to match the current theme mode.
        auto retintIcons = [save, openFolder, &app]() {
            QColor color = app.styleManager()->isDarkMode() ? Qt::white : QColor(40, 40, 40);
            save->setIcon(tintedIcon(Icons16::Action_Save, color));
            openFolder->setIcon(tintedIcon(Icons16::File_FolderOpen, color));
        };

        connect(app.styleManager(), &StyleManager::themeChanged,
                darkToggle, [&app, darkToggle, themeCombo, retintIcons]() {
            darkToggle->setChecked(app.styleManager()->isDarkMode());
            darkToggle->setText(app.styleManager()->isDarkMode() ? "🌙" : "☀️");
            retintIcons();
            QString base = app.styleManager()->currentBaseName();
            if (themeCombo->currentText() != base) {
                themeCombo->blockSignals(true);
                themeCombo->setCurrentText(base);
                themeCombo->blockSignals(false);
            }
        });

        toolBar->addWidget(darkToggle);
    }
}
