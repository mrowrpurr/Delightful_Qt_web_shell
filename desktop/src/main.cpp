// Qt desktop shell — the entry point.
// Everything interesting lives in the classes this file wires together.

#include "application.hpp"
#include "widgets/scheme_handler.hpp"
#include "windows/main_window.hpp"

#include <QTimer>

int main(int argc, char* argv[]) {
    // Custom URL scheme must be registered BEFORE QApplication is constructed.
    // Qt enforces this — it's a hard requirement, not a suggestion.
    SchemeHandler::registerUrlScheme();

    Application app(argc, argv);

    // If another instance is already running, it was signaled to activate.
    // This process exits cleanly — the user sees the existing window raise.
    if (!app.isPrimaryInstance()) return 0;

    MainWindow window;

    // When another instance tries to launch, raise the existing window
    QObject::connect(&app, &Application::activationRequested, &window, [&window]() {
        window.show();
        window.raise();
        window.activateWindow();
    });

    // Show invisible, let Qt paint the dark background, then reveal.
    // This prevents a white flash on the first frame.
    window.setWindowOpacity(0.0);
    window.show();
    QTimer::singleShot(0, [&window]() { window.setWindowOpacity(1.0); });

    return app.exec();
}
