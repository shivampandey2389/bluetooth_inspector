#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include "bluez_types.h"

class SignalStrengthReader : public QObject
{
    Q_OBJECT

public:
    explicit SignalStrengthReader(QObject *parent = nullptr);

    // Synchronous reading (fast, takes ~4ms via hcitool or instant from cached dbus)
    SignalStrengthInfo readSignalStrength(const QString &macAddress, int dbusRssi = 0);

    // Asynchronous reading
    void requestSignalStrength(const QString &macAddress, int dbusRssi = 0);

    // Query all connected devices
    void refreshAll(const QStringList &macAddresses);

signals:
    void signalStrengthUpdated(const QString &macAddress, const SignalStrengthInfo &info);

private:
    QMap<QString, SignalStrengthInfo> m_cache;
};
