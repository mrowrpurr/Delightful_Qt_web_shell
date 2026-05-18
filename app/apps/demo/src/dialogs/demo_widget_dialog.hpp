// DemoWidgetDialog — showcases common Qt widgets for theme preview.
//
// Opened via Window > Demo Widget. Each tab shows a different category
// of widgets so you can see how the current QSS theme affects everything.

#pragma once

#include <QWidget>

class DemoWidgetDialog : public QWidget {
    Q_OBJECT

public:
    explicit DemoWidgetDialog(QWidget* parent = nullptr);
};
