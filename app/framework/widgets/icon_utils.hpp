// Icon utilities — tint SVG icons for light/dark themes.

#pragma once

#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include <oclero/qlementine/icons/QlementineIcons.hpp>
#include <oclero/qlementine/icons/Icons16.hpp>

using oclero::qlementine::icons::iconPath;
using oclero::qlementine::icons::Icons16;

// Tint an SVG icon to a given color (default: white for dark themes).
// Loads the SVG as a pixmap, paints the color over it using SourceIn
// composition — only existing pixels get recolored, transparency preserved.
inline QIcon tintedIcon(Icons16 icon, const QColor& color = Qt::white) {
    QPixmap pix(iconPath(icon));
    QPainter painter(&pix);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pix.rect(), color);
    painter.end();
    return QIcon(pix);
}
