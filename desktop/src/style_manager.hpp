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
//
// Dark/light mode:
//   Themes are named <slug>-dark and <slug>-light. Toggling mode switches
//   between them. If the opposite mode doesn't exist, falls back to default.
//   Last-used theme per mode is remembered so toggling back restores it.

#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QApplication;

class StyleManager : public QObject {
    Q_OBJECT

public:
    explicit StyleManager(QObject* parent = nullptr);

    // Apply a theme by full name (e.g. "synthwave-84-dark", "zinc-light").
    // Searches sources in priority order.
    void applyTheme(const QString& themeName);

    // Apply a theme by base name + mode (e.g. "synthwave-84", dark=true → "synthwave-84-dark").
    void applyTheme(const QString& baseName, bool dark);

    // Apply a theme from a React display name (e.g. "Mrowr Purr - Synthwave '84") + mode.
    // Slugifies the display name to find the matching QSS file.
    void applyThemeByDisplayName(const QString& displayName, bool dark);

    // Toggle between dark and light mode for the current theme.
    // If current is "synthwave-84-dark", switches to "synthwave-84-light" (if it exists).
    // Falls back to "default-light"/"default-dark" if the opposite doesn't exist.
    // Remembers last theme per mode so toggling back restores it.
    void toggleDarkMode();

    // Set dark mode explicitly.
    void setDarkMode(bool dark);

    // Get the currently applied full theme name (e.g. "synthwave-84-dark").
    QString currentTheme() const { return currentTheme_; }

    // Get the base theme name without -dark/-light suffix (slugified).
    QString currentBaseName() const;

    // Get the React display name for the current theme (if set via applyThemeByDisplayName).
    QString currentDisplayName() const { return currentDisplayName_; }

    // Whether we're currently in dark mode.
    bool isDarkMode() const { return isDark_; }

    // List available theme names from the active source.
    QStringList availableThemes() const;

    // List available base theme names (without -dark/-light, deduplicated).
    QStringList availableBaseThemes() const;

    // Whether we're using a live-reloaded source (dev path or AppData).
    bool isLiveReload() const { return !watchedDir_.isEmpty(); }

signals:
    // Emitted when the stylesheet changes (live reload or explicit apply).
    // Parameterless — callers use getters to read current state.
    void themeChanged();

private:
    void setupWatcher(const QString& dir);
    void onFileChanged(const QString& path);
    QString loadQss(const QString& themeName) const;
    QString compileScssToCss(const QString& scssPath) const;
    QString findThemeFile(const QString& themeName) const;
    QStringList listThemesInDir(const QString& dir) const;

    // Strips -dark or -light suffix from a theme name.
    static QString stripModeSuffix(const QString& name);
    // Convert a display name to a slug (e.g. "Mrowr Purr - Synthwave '84" → "mrowr-purr-synthwave-84")
    static QString slugify(const QString& name);
    // Whether a given theme exists in our sources.
    bool themeExists(const QString& name) const;

    void loadNameMapping();

    QString currentTheme_;
    QString currentDisplayName_;  // React display name (set via applyThemeByDisplayName)
    bool isDark_ = true;
    QHash<QString, QString> slugToDisplayName_;  // slug → React display name
    QString lastDarkTheme_;    // remember last dark theme for toggle-back
    QString lastLightTheme_;   // remember last light theme for toggle-back
    QString lastDarkDisplayName_;
    QString lastLightDisplayName_;
    QString watchedDir_;
    QString devPath_;
    QString userPath_;
    QFileSystemWatcher watcher_;
};
