// ThemeBridge — Qt theme control exposed to React.
// Coordinates React ↔ Qt theme state. No Qt dependencies — pure app_shell::Bridge.

#pragma once

#include "theme_dtos.hpp"
#include "bridge.hpp"

class ThemeBridge : public app_shell::Bridge {
    std::string qtDisplayName_;
    bool qtIsDark_ = true;
    std::string qtThemeFilePath_;
    bool qtThemeEmbedded_ = false;

public:
    ThemeBridge() {
        method("setQtTheme",         &ThemeBridge::setQtTheme);
        method("getQtTheme",         &ThemeBridge::getQtTheme);
        method("getQtThemeFilePath", &ThemeBridge::getQtThemeFilePath);

        signal("qtThemeChanged");
        signal("qtThemeRequested");
    }

    OkResponse setQtTheme(SetQtThemeRequest req) {
        ThemeState payload{.displayName = req.displayName, .isDark = req.isDark};
        emit_signal("qtThemeRequested", payload);
        return {};
    }

    GetQtThemeResponse getQtTheme() const {
        GetQtThemeResponse resp;
        resp.displayName = qtDisplayName_;
        resp.isDark = qtIsDark_;
        return resp;
    }

    // Called by the Theming subsystem when Qt theme changes.
    void updateQtThemeState(const std::string& displayName, bool isDark) {
        qtDisplayName_ = displayName;
        qtIsDark_ = isDark;
        ThemeState payload{.displayName = qtDisplayName_, .isDark = qtIsDark_};
        emit_signal("qtThemeChanged", payload);
    }

    GetQtThemeFilePathResponse getQtThemeFilePath() const {
        GetQtThemeFilePathResponse resp;
        resp.path = qtThemeFilePath_;
        resp.embedded = qtThemeEmbedded_;
        return resp;
    }

    void setQtThemeFilePath(const std::string& path, bool embedded) {
        qtThemeFilePath_ = path;
        qtThemeEmbedded_ = embedded;
    }
};
