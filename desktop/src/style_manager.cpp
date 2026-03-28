// StyleManager — theme loading with three-source fallback and live reload.
//
// Priority: STYLES_DEV_PATH → AppData/Local → QRC embedded.
// SCSS files are compiled to CSS at runtime via libsass.
// QSS files are loaded directly.

#include "style_manager.hpp"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#include <sass.h>

StyleManager::StyleManager(QObject* parent)
    : QObject(parent)
{
    // ── Determine active source ──────────────────────────────

    // Source 1: compile-time dev path (only set during local development)
#ifdef STYLES_DEV_PATH
    devPath_ = QString(STYLES_DEV_PATH);
    if (QDir(devPath_).exists()) {
        qDebug() << "[StyleManager] Dev styles path:" << devPath_;
    } else {
        qDebug() << "[StyleManager] Dev styles path does not exist:" << devPath_;
        devPath_.clear();
    }
#endif

    // Source 2: user override folder in AppData
    userPath_ = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                + "/styles";

    // Pick the active live-reload source (if any)
    if (!devPath_.isEmpty()) {
        setupWatcher(devPath_);
    } else if (QDir(userPath_).exists() && !QDir(userPath_).isEmpty()) {
        setupWatcher(userPath_);
    }
    // Otherwise: QRC mode (no watcher)

    connect(&watcher_, &QFileSystemWatcher::fileChanged,
            this, &StyleManager::onFileChanged);
    connect(&watcher_, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString&) {
        // A file was added/removed — re-apply current theme in case it changed
        if (!currentTheme_.isEmpty())
            applyTheme(currentTheme_);
    });
}

void StyleManager::setupWatcher(const QString& dir) {
    watchedDir_ = dir;
    watcher_.addPath(dir);

    // Watch all files in the directory (and compiled/ subdirectory)
    QDirIterator it(dir, {"*.scss", "*.qss", "*.css"},
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        watcher_.addPath(it.next());

    qDebug() << "[StyleManager] Watching" << dir << "for live reload";
}

void StyleManager::onFileChanged(const QString& path) {
    qDebug() << "[StyleManager] File changed:" << path;

    // Re-add the path — QFileSystemWatcher drops watched files on some platforms
    // after they're modified (the OS deletes + recreates the file).
    if (QFile::exists(path) && !watcher_.files().contains(path))
        watcher_.addPath(path);

    // If the changed file is related to the current theme, re-apply
    if (!currentTheme_.isEmpty())
        applyTheme(currentTheme_);
}

void StyleManager::applyTheme(const QString& themeName) {
    QString qss = loadQss(themeName);

    if (qss.isEmpty()) {
        qDebug() << "[StyleManager] No QSS found for theme:" << themeName;
        return;
    }

    qApp->setStyleSheet(qss);
    currentTheme_ = themeName;
    emit themeChanged(themeName);
    qDebug() << "[StyleManager] Applied theme:" << themeName
             << (isLiveReload() ? "(live)" : "(QRC)");
}

QString StyleManager::loadQss(const QString& themeName) const {
    QString filePath = findThemeFile(themeName);
    if (filePath.isEmpty()) return {};

    // SCSS → compile at runtime
    if (filePath.endsWith(".scss"))
        return compileScssToCss(filePath);

    // QSS/CSS → load directly
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QTextStream(&file).readAll();
}

QString StyleManager::findThemeFile(const QString& themeName) const {
    // Check live-reload source first
    if (!watchedDir_.isEmpty()) {
        // Look for SCSS in themes/ subdirectory (dev layout)
        QString scssPath = watchedDir_ + "/themes/" + themeName + ".scss";
        if (QFile::exists(scssPath)) return scssPath;

        // Look for compiled QSS in compiled/ subdirectory
        QString compiledPath = watchedDir_ + "/compiled/" + themeName + ".qss";
        if (QFile::exists(compiledPath)) return compiledPath;

        // Look for QSS directly in the directory
        QString qssPath = watchedDir_ + "/" + themeName + ".qss";
        if (QFile::exists(qssPath)) return qssPath;
    }

    // Fall back to QRC
    QString qrcPath = ":/styles/" + themeName + ".qss";
    if (QFile::exists(qrcPath)) return qrcPath;

    return {};
}

QString StyleManager::compileScssToCss(const QString& scssPath) const {
    QByteArray pathUtf8 = scssPath.toUtf8();

    // Create a file context for libsass
    struct Sass_File_Context* ctx = sass_make_file_context(pathUtf8.constData());
    struct Sass_Options* opts = sass_file_context_get_options(ctx);

    // Set include path so @import "../shared/widgets" resolves
    QString includeDir = QFileInfo(scssPath).absolutePath();
    sass_option_set_include_path(opts, includeDir.toUtf8().constData());
    sass_option_set_output_style(opts, SASS_STYLE_EXPANDED);
    sass_option_set_precision(opts, 5);

    sass_file_context_set_options(ctx, opts);

    // Compile
    int status = sass_compile_file_context(ctx);
    QString result;

    if (status == 0) {
        result = QString::fromUtf8(sass_context_get_output_string(
            sass_file_context_get_context(ctx)));
    } else {
        const char* err = sass_context_get_error_message(
            sass_file_context_get_context(ctx));
        qWarning() << "[StyleManager] SCSS compile error:" << err;
    }

    sass_delete_file_context(ctx);
    return result;
}

QStringList StyleManager::availableThemes() const {
    QStringList themes;

    // From live-reload source
    if (!watchedDir_.isEmpty()) {
        themes += listThemesInDir(watchedDir_ + "/themes");
        themes += listThemesInDir(watchedDir_ + "/compiled");
        themes += listThemesInDir(watchedDir_);
    }

    // From QRC
    themes += listThemesInDir(":/styles");

    themes.removeDuplicates();
    themes.sort();
    return themes;
}

QStringList StyleManager::listThemesInDir(const QString& dir) const {
    QStringList themes;
    QDir d(dir);
    if (!d.exists()) return themes;

    for (const auto& entry : d.entryInfoList({"*.scss", "*.qss", "*.css"}, QDir::Files)) {
        themes.append(entry.baseName());
    }
    return themes;
}
