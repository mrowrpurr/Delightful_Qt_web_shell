// WebDialog — React inside a popup dialog.
//
// Same bridges, same React app, different container. The WebShellWidget
// handles all the plumbing (QWebChannel, qwebchannel.js injection, etc.)
// — this dialog just sets the size and overlay style.

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

    // Create a WebShellWidget with Spinner overlay — same bridges as the main window.
    // Loads the main app by default — change "main" to load a different web app.
    auto* app = qobject_cast<Application*>(qApp);
    webShell_ = new WebShellWidget(
        app->webProfile(), app->shell(), app->appUrl("main"),
        WebShellWidget::SpinnerOverlay, this);
    layout->addWidget(webShell_);
}
