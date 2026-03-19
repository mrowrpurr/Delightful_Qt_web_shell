// Qt desktop shell — the entry point.
// Everything interesting lives in the classes this file wires together.

#include <QApplication>
#include <QLabel>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName(APP_ORG);
    app.setApplicationName(APP_NAME);

    QLabel label("Hello from " APP_NAME " 👋");
    label.setAlignment(Qt::AlignCenter);
    label.resize(400, 200);
    label.show();

    return app.exec();
}
