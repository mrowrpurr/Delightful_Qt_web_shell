#include <catch2/catch_test_macros.hpp>
#include "theme_bridge.hpp"

TEST_CASE("getQtTheme returns stored state after updateQtThemeState") {
    ThemeBridge bridge;
    bridge.updateQtThemeState("Synthwave '84", true);

    auto result = bridge.dispatch("getQtTheme", nlohmann::json::object());
    REQUIRE(result["displayName"] == "Synthwave '84");
    REQUIRE(result["isDark"] == true);
}

TEST_CASE("getQtTheme returns defaults before any update") {
    ThemeBridge bridge;

    auto result = bridge.dispatch("getQtTheme", nlohmann::json::object());
    REQUIRE(result["displayName"] == "");
    REQUIRE(result["isDark"] == true);
}

TEST_CASE("setQtTheme emits qtThemeRequested signal with payload") {
    ThemeBridge bridge;
    nlohmann::json received;
    bridge.on_signal("qtThemeRequested", [&](const nlohmann::json& data) {
        received = data;
    });

    bridge.dispatch("setQtTheme", {{"displayName", "Zinc"}, {"isDark", false}});

    REQUIRE(received["displayName"] == "Zinc");
    REQUIRE(received["isDark"] == false);
}

TEST_CASE("setQtTheme returns ok") {
    ThemeBridge bridge;

    auto result = bridge.dispatch("setQtTheme", {{"displayName", "Zinc"}, {"isDark", true}});
    REQUIRE(result["ok"] == true);
}

TEST_CASE("updateQtThemeState emits qtThemeChanged signal") {
    ThemeBridge bridge;
    nlohmann::json received;
    bridge.on_signal("qtThemeChanged", [&](const nlohmann::json& data) {
        received = data;
    });

    bridge.updateQtThemeState("Catppuccin", false);

    REQUIRE(received["displayName"] == "Catppuccin");
    REQUIRE(received["isDark"] == false);
}

TEST_CASE("getQtThemeFilePath returns stored file path") {
    ThemeBridge bridge;
    bridge.setQtThemeFilePath("/styles/compiled/zinc-dark.qss", false);

    auto result = bridge.dispatch("getQtThemeFilePath", nlohmann::json::object());
    REQUIRE(result["path"] == "/styles/compiled/zinc-dark.qss");
    REQUIRE(result["embedded"] == false);
}

TEST_CASE("getQtThemeFilePath returns embedded flag correctly") {
    ThemeBridge bridge;
    bridge.setQtThemeFilePath(":/styles/default-dark.qss", true);

    auto result = bridge.dispatch("getQtThemeFilePath", nlohmann::json::object());
    REQUIRE(result["path"] == ":/styles/default-dark.qss");
    REQUIRE(result["embedded"] == true);
}

TEST_CASE("unknown method returns error") {
    ThemeBridge bridge;

    auto result = bridge.dispatch("nonexistent", nlohmann::json::object());
    REQUIRE(result.contains("error"));
    REQUIRE(result["error"].get<std::string>().find("Unknown method") != std::string::npos);
}
