// ReactiveTitle — window-scoped subsystem that syncs dock titles from document.title.
//
// Constructed as a child of a QMainWindow. When a dock is added and its content
// has a QWebEngineView, connects to titleChanged so the dock tab reflects the
// page title.

#pragma once

#include <QDockWidget>
#include <QMainWindow>
#include <QObject>
#include <QWebEnginePage>
#include <QWebEngineView>

class ReactiveTitle : public QObject {
    Q_OBJECT

public:
    explicit ReactiveTitle(QMainWindow* window)
        : QObject(window) {}

    // Call this when a dock is added to the window.
    void wireDock(QDockWidget* dock) {
        if (!dock->widget()) return;
        if (auto* view = dock->widget()->findChild<QWebEngineView*>()) {
            connect(view->page(), &QWebEnginePage::titleChanged,
                    this, [dock](const QString& title) {
                dock->setWindowTitle(title.isEmpty() ? APP_NAME : title);
            });
        }
    }
};
