// WebDialog — React inside a popup dialog.
//
// Same bridges, same React app, different container. The WebShellWidget
// handles all the plumbing (QWebChannel, qwebchannel.js injection, etc.)
// — this dialog just sets the size and overlay style.
//
// To customize what React renders in this dialog, you could:
//   - Load a different URL hash: app://shell/#/dialog
//   - Pass a query param: app://shell/?mode=popup
//   - Use a completely different React entry point
//
// For now, it loads the same app to demonstrate the pattern.

#include "web_dialog.hpp"
#include "application.hpp"
#include "widgets/web_shell_widget.hpp"

#include <QVBoxLayout>

WebDialog::WebDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString("%1 — Dialog").arg(APP_NAME));
    resize(600, 400);

    // Remove the "?" help button (Windows)
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create a WebShellWidget with Spinner overlay — same bridges as the main window
    auto* app = qobject_cast<Application*>(qApp);
    webShell_ = new WebShellWidget(
        app->webProfile(), app->shell(), app->devMode(),
        WebShellWidget::SpinnerOverlay, this);
    layout->addWidget(webShell_);
}
