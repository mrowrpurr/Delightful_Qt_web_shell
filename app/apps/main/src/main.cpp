// Your app starts here.

#include "app.hpp"
#include "scheme_handler.hpp"
#include "main_window.hpp"
#include "theming.hpp"

int main(int argc, char* argv[]) {
    SchemeHandler::registerUrlScheme();

    app_shell::App app(argc, argv);

    new app_shell::Theming(app, "default-dark");

    MainWindow window(app);
    window.show();

    return app.exec();
}
