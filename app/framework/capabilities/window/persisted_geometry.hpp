// PersistedGeometry — window-scoped subsystem that saves/restores geometry.
//
// Constructed as a child of a QMainWindow. Restores geometry on construction,
// saves geometry + dock state + zoom level on aboutToQuit.

#pragma once

#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QObject>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QWebEngineView>

class PersistedGeometry : public QObject {
    Q_OBJECT

public:
    explicit PersistedGeometry(QMainWindow* window, const QString& windowId)
        : QObject(window), window_(window)
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);
        QString geoKey = "window/" + windowId + "/geometry";

        if (settings.contains(geoKey)) {
            window_->restoreGeometry(settings.value(geoKey).toByteArray());
        } else {
            window_->resize(900, 640);
            if (auto* screen = QApplication::primaryScreen()) {
                QRect geo = screen->availableGeometry();
                window_->move((geo.width() - 900) / 2 + geo.x(),
                              (geo.height() - 640) / 2 + geo.y());
            }
        }

        // Restore zoom on active view (deferred — dock might not exist yet).
        QString zoomKey = "window/" + windowId + "/zoomFactor";
        if (settings.contains(zoomKey)) {
            qreal zoom = settings.value(zoomKey, 1.0).toReal();
            QTimer::singleShot(0, this, [this, zoom]() {
                if (auto* view = activeView())
                    view->setZoomFactor(zoom);
            });
        }

        connect(qApp, &QApplication::aboutToQuit, this, [this]() {
            // Don't re-save if the window was already closed and its state removed.
            if (!window_->isVisible()) return;
            QSettings s(QSettings::IniFormat, QSettings::UserScope, APP_ORG, APP_SLUG);
            QString key = "window/" + window_->objectName();
            s.setValue(key + "/geometry", window_->saveGeometry());
            s.setValue(key + "/dockState", window_->saveState());
            if (auto* view = activeView())
                s.setValue(key + "/zoomFactor", view->zoomFactor());
        });
    }

private:
    QWebEngineView* activeView() const {
        // Find the first visible dock's web view.
        for (auto* dock : window_->findChildren<QDockWidget*>()) {
            if (dock->isVisible() && dock->widget())
                if (auto* view = dock->widget()->findChild<QWebEngineView*>())
                    return view;
        }
        return nullptr;
    }

    QMainWindow* window_;
};
