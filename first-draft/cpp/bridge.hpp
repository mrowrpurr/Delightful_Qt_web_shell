#pragma once
#include <QObject>
#include <QString>
#include <QTime>

class Bridge : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

public slots:
    QString greet(const QString& name) {
        return QString("Hello from C++, %1! The time is %2.")
            .arg(name, QTime::currentTime().toString("hh:mm:ss"));
    }
};
