#pragma once

#include <QFrame>
#include <QDBusObjectPath>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include "bluez_types.h"
#include "signal_strength_widget.h"
#include "circular_battery_widget.h"

enum class BatteryDisplayMode {
    NormalBar = 0,
    Circle = 1
};

class ConnectedDeviceCard : public QFrame
{
    Q_OBJECT

public:
    explicit ConnectedDeviceCard(const QDBusObjectPath &devicePath, QWidget *parent = nullptr);

    void updateDeviceData(const QString &name,
                          const QString &address,
                          const QString &iconName,
                          bool paired,
                          bool trusted,
                          const QString &adapterName);

    void updateBattery(const std::optional<BatteryInfo> &battery);
    void updateSignalStrength(const SignalStrengthInfo &signal);

    void setBatteryDisplayMode(BatteryDisplayMode mode);
    BatteryDisplayMode batteryDisplayMode() const { return m_batteryMode; }

    QDBusObjectPath devicePath() const { return m_devicePath; }
    QString macAddress() const { return m_macAddress; }

signals:
    void disconnectRequested(const QDBusObjectPath &devicePath);
    void refreshSignalRequested(const QString &macAddress);
    void batteryDisplayModeChanged(BatteryDisplayMode mode);

private:
    void updateCardBatteryButtons();

    QDBusObjectPath m_devicePath;
    QString m_macAddress;
    BatteryDisplayMode m_batteryMode = BatteryDisplayMode::Circle;

    // Header widgets
    QLabel *m_iconLabel = nullptr;
    QLabel *m_nameLabel = nullptr;
    QLabel *m_addressLabel = nullptr;
    QLabel *m_statusBadge = nullptr;
    QPushButton *m_disconnectBtn = nullptr;
    QPushButton *m_refreshSignalBtn = nullptr;

    // Signal widget
    SignalStrengthWidget *m_signalWidget = nullptr;

    // Battery widgets
    QPushButton *m_cardCircleBtn = nullptr;
    QPushButton *m_cardBarBtn = nullptr;
    QStackedWidget *m_batteryStack = nullptr;

    // Mode 0: Normal linear bar
    QProgressBar *m_batteryBar = nullptr;
    QLabel *m_batteryLabel = nullptr;
    QLabel *m_batteryStateLabel = nullptr;

    // Mode 1: Circle ring gauge
    CircularBatteryWidget *m_circularBattery = nullptr;
    QLabel *m_circularStateLabel = nullptr;

    // Bottom Chips
    QLabel *m_pairedChip = nullptr;
    QLabel *m_trustedChip = nullptr;
    QLabel *m_adapterChip = nullptr;
};
