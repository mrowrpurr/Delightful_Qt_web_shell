// Request/response DTOs for ThemeBridge methods.

#pragma once

#include <def_type.hpp>

struct ThemeState                 { std::string displayName; bool isDark = false; };
struct SetQtThemeRequest         { std::string displayName; bool isDark; };
struct GetQtThemeResponse        { std::string displayName; bool isDark; };
struct GetQtThemeFilePathResponse { std::string path; bool embedded = false; };
