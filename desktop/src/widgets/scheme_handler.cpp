// SchemeHandler — serves embedded Qt resources via app:// URLs.
//
// Routes by host: app://main/ serves from :/web-main/, app://docs/ from :/web-docs/.
// This lets each web app have its own origin, localStorage, IndexedDB, etc.

#include "scheme_handler.hpp"

#include <QFile>
#include <QFileInfo>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

void SchemeHandler::registerUrlScheme() {
    // Must be called BEFORE QApplication is constructed — Qt enforces this.
    QWebEngineUrlScheme scheme("app");
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::HostAndPort);
    scheme.setDefaultPort(QWebEngineUrlScheme::PortUnspecified);
    scheme.setFlags(
        QWebEngineUrlScheme::SecureScheme |
        QWebEngineUrlScheme::LocalAccessAllowed |
        QWebEngineUrlScheme::CorsEnabled |
        QWebEngineUrlScheme::ContentSecurityPolicyIgnored
    );
    QWebEngineUrlScheme::registerScheme(scheme);
}

void SchemeHandler::requestStarted(QWebEngineUrlRequestJob* job) {
    // Route by host: app://main/ → :/web-main/, app://docs/ → :/web-docs/
    QString appName = job->requestUrl().host();
    QString urlPath = job->requestUrl().path();
    if (urlPath.isEmpty() || urlPath == "/") urlPath = "/index.html";

    // Try the exact resource path, fall back to index.html for SPA routing
    QString resPath = ":/web-" + appName + urlPath;
    if (!QFile::exists(resPath))
        resPath = ":/web-" + appName + "/index.html";

    auto* file = new QFile(resPath, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    job->reply(mimeForExtension(QFileInfo(urlPath).suffix()), file);
}

QByteArray SchemeHandler::mimeForExtension(const QString& ext) {
    static const QHash<QString, QByteArray> types = {
        {"html", "text/html"},       {"js",   "text/javascript"},
        {"mjs",  "text/javascript"}, {"css",  "text/css"},
        {"json", "application/json"},{"png",  "image/png"},
        {"jpg",  "image/jpeg"},      {"jpeg", "image/jpeg"},
        {"gif",  "image/gif"},       {"webp", "image/webp"},
        {"svg",  "image/svg+xml"},   {"ico",  "image/x-icon"},
        {"woff", "font/woff"},       {"woff2","font/woff2"},
        {"ttf",  "font/ttf"},        {"wasm", "application/wasm"},
        {"map",  "application/json"},
    };
    return types.value(ext.toLower(), "application/octet-stream");
}
