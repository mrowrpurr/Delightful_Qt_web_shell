#pragma once

#include <QString>

// Install a Qt message handler that appends to `debug.log` in the current
// working directory. Also forwards to stderr so the console still shows
// output. Safe to call once, before QApplication is constructed.
void setupLogging(const QString& filename = "debug.log");
