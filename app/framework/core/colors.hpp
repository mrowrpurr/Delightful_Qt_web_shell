#pragma once

#include <QColor>

namespace app_shell {

/// Must match <html style="background:#242424"> in each app's index.html.
/// Used by WebShellWidget, LoadingOverlay, and Theming to prevent white flash
/// before web content or themes load.
static constexpr QColor kDefaultBackground{0x24, 0x24, 0x24};

}  // namespace app_shell
