#include "logging.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

#include <cstdio>

namespace {

QFile* g_logFile = nullptr;
QMutex g_logMutex;

const char* levelTag(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:    return "DEBUG";
        case QtInfoMsg:     return "INFO ";
        case QtWarningMsg:  return "WARN ";
        case QtCriticalMsg: return "ERROR";
        case QtFatalMsg:    return "FATAL";
    }
    return "?????";
}

void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    QMutexLocker lock(&g_logMutex);

    const QString line = QString("%1 [%2] %3%4")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
        .arg(levelTag(type))
        .arg(ctx.category && *ctx.category ? QString("(%1) ").arg(ctx.category) : QString())
        .arg(msg);

    if (g_logFile && g_logFile->isOpen()) {
        QTextStream out(g_logFile);
        out << line << '\n';
        out.flush();
    }

    std::fprintf(stderr, "%s\n", qUtf8Printable(line));
    if (type == QtFatalMsg) std::fflush(stderr);
}

} // namespace

void setupLogging(const QString& filename) {
    if (g_logFile) return; // idempotent

    const QString path = QDir::current().filePath(filename);
    g_logFile = new QFile(path);
    if (!g_logFile->open(QIODevice::Append | QIODevice::Text)) {
        std::fprintf(stderr, "setupLogging: could not open %s for append\n",
                     qUtf8Printable(path));
        delete g_logFile;
        g_logFile = nullptr;
        return;
    }

    qInstallMessageHandler(messageHandler);
    qInfo("──── log started: %s v%s ────", APP_NAME, APP_VERSION);
}
