// Theming — see theming.hpp for overview.

#include "theming.hpp"

#include <QPalette>
#include <QStyleHints>

#include "shell/app.hpp"
#include "style_manager.hpp"
#include "theme_bridge.hpp"

namespace app_shell {

// Must match the HTML <html style="background:#242424"> — prevents white flash.
static constexpr QColor kBackground{0x24, 0x24, 0x24};

Theming::Theming(App& app, const QString& baseline)
    : QObject(&app)
{
    // ── Dark palette baseline ─────────────────────────────────────
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, kBackground);
    darkPalette.setColor(QPalette::Base, kBackground);
    app.setPalette(darkPalette);

    // ── StyleManager ──────────────────────────────────────────────
    sm_ = new StyleManager(&app);
    sm_->applyTheme(baseline);

    // ── Wire StyleManager ↔ ThemeBridge ───────────────────────────
    auto* themeBridge = app.bridge<ThemeBridge>();
    if (themeBridge) {
        QObject::connect(sm_, &StyleManager::themeChanged, &app, [this, themeBridge]() {
            themeBridge->updateQtThemeState(
                sm_->currentDisplayName().toStdString(), sm_->isDarkMode());
            QString filePath = sm_->currentThemeFilePath();
            themeBridge->setQtThemeFilePath(
                filePath.toStdString(), filePath.startsWith(":/"));
        });
        themeBridge->on_signal("qtThemeRequested", [&app, sm = sm_](const nlohmann::json& data) {
            auto displayName = QString::fromStdString(data["displayName"].get<std::string>());
            bool isDark = data["isDark"].get<bool>();
            QMetaObject::invokeMethod(&app, [sm, displayName, isDark]() {
                sm->applyThemeByDisplayName(displayName, isDark);
            }, Qt::QueuedConnection);
        });
    }
}

} // namespace app_shell
