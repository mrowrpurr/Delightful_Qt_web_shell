// StyleManager — loads and applies QSS themes to the application.
//
// Theme sources, checked in order:
//   1. STYLES_DEV_PATH define (dev only) — points at desktop/styles/ in the repo
//   2. AppData/Local/<app>/styles/ — power user override folder
//   3. QRC :/styles/<theme>.qss — embedded compiled themes (default)
//
// Sources 1 and 2 activate QFileSystemWatcher for live reload.
// Source 1 compiles SCSS via libsass if the file is .scss.
// Source 3 is pre-compiled QSS embedded at build time.

#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QStringList>

class QApplication;

class StyleManager : public QObject {
    Q_OBJECT

public:
    explicit StyleManager(QObject* parent = nullptr);

    // Apply a theme by name (e.g. "synthwave-84-dark", "zinc-light").
    // Searches sources in priority order.
    void applyTheme(const QString& themeName);

    // Get the currently applied theme name.
    QString currentTheme() const { return currentTheme_; }

    // List available theme names from the active source.
    QStringList availableThemes() const;

    // Whether we're using a live-reloaded source (dev path or AppData).
    bool isLiveReload() const { return !watchedDir_.isEmpty(); }

signals:
    // Emitted when the stylesheet changes (live reload or explicit apply).
    void themeChanged(const QString& themeName);

private:
    void setupWatcher(const QString& dir);
    void onFileChanged(const QString& path);
    QString loadQss(const QString& themeName) const;
    QString compileScssToCss(const QString& scssPath) const;
    QString findThemeFile(const QString& themeName) const;
    QStringList listThemesInDir(const QString& dir) const;

    QString currentTheme_;
    QString watchedDir_;        // active live-reload directory (empty = QRC mode)
    QString devPath_;           // compile-time STYLES_DEV_PATH (empty in release)
    QString userPath_;          // AppData/Local/<app>/styles/
    QFileSystemWatcher watcher_;
};
