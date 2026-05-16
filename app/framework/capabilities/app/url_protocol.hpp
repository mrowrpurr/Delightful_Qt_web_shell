// app_shell::UrlProtocol — registers the app as a handler for <slug>:// URLs.
//
// Constructed by the consumer as a child of App. Owns the platform-specific
// registration (Windows registry / Linux .desktop file), exposes register /
// unregister / query operations, and offers a first-launch prompt.

#pragma once

#include <QObject>
#include <QString>

namespace app_shell {

class App;

class UrlProtocol : public QObject {
    Q_OBJECT

public:
    explicit UrlProtocol(App* parent);

    // The scheme name (e.g. "delightful-qt-web-shell" → "delightful-qt-web-shell://").
    // Derived from APP_SLUG, lowercased.
    QString name() const;

    // True if this binary is currently the registered handler for name()://.
    bool isRegistered() const;

    // Register this binary as the handler for name():// URLs.
    void registerProtocol();

    // Remove this binary as the handler.
    void unregisterProtocol();

    // Shows a one-shot confirmation dialog asking the user whether to register
    // the protocol. No-op when already registered or when the user previously
    // chose "Don't ask me again" (persisted in QSettings under urlProtocol/dontAsk).
    void promptIfNeeded();

private:
    App* app_;
};

}  // namespace app_shell
