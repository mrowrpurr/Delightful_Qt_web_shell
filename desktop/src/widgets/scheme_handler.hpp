// SchemeHandler — serves embedded Qt resources via the app:// URL scheme.
//
// In production, the React build is compiled into the binary as Qt resources
// (:/web/...). This handler serves those files to QWebEngineView through
// a custom URL scheme so that the web app has a proper origin, localStorage,
// IndexedDB, and other web platform features work correctly.
//
// SPA fallback: unknown paths serve index.html (so React Router works).

#pragma once

#include <QWebEngineUrlSchemeHandler>

class SchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT

public:
    using QWebEngineUrlSchemeHandler::QWebEngineUrlSchemeHandler;

    void requestStarted(QWebEngineUrlRequestJob* job) override;

    // Call this once in main(), BEFORE QApplication is constructed.
    // Qt requires custom URL schemes to be registered before the app starts.
    static void registerUrlScheme();

private:
    static QByteArray mimeForExtension(const QString& ext);
};
