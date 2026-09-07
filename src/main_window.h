#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>

#include "bluez_object_manager.h"
#include "signal_strength_reader.h"
#include "connected_device_card.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onRefreshTriggered();
    void onScanToggled();
    void onPowerToggled();
    void onAutoRefreshIntervalChanged(int index);
    void onBatteryCircleBtnClicked();
    void onBatteryBarBtnClicked();
    void onSignalStrengthUpdated(const QString &macAddress, const SignalStrengthInfo &info);

    // BlueZ event handlers
    void onDeviceAdded(const QDBusObjectPath &path, const QVariantMap &props);
    void onDeviceRemoved(const QDBusObjectPath &path);
    void onDevicePropertiesChanged(const QDBusObjectPath &path, const QVariantMap &props);
    void onBatteryChanged(const QDBusObjectPath &path, const BatteryInfo &battery);

    // Device actions
    void onDisconnectRequested(const QDBusObjectPath &path);
    void onConnectRequested(const QDBusObjectPath &path);
    void onRemoveDeviceRequested(const QDBusObjectPath &path);

private:
    void setupUi();
    void updateAdapterControls();
    void refreshDeviceViews();
    void updateConnectedDevicesView();
    void updateAllDevicesTable();
    void updateStatusBar();
    void setGlobalBatteryMode(BatteryDisplayMode mode);
    void updateBatteryModeButtons();

    BluezObjectManager m_manager;
    SignalStrengthReader m_signalReader;

    QTimer m_pollTimer;
    QDBusObjectPath m_currentAdapterPath;
    BatteryDisplayMode m_globalBatteryMode = BatteryDisplayMode::Circle;

    // Header widgets
    QLabel *m_adapterBadge = nullptr;
    QPushButton *m_powerBtn = nullptr;
    QPushButton *m_scanBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;

    // Preference widgets
    QPushButton *m_batteryCircleBtn = nullptr;
    QPushButton *m_batteryBarBtn = nullptr;
    QComboBox *m_intervalCombo = nullptr;

    // Tabs
    QTabWidget *m_tabWidget = nullptr;

    // Connected devices view
    QWidget *m_connectedContainer = nullptr;
    QVBoxLayout *m_connectedCardsLayout = nullptr;
    QLabel *m_emptyConnectedLabel = nullptr;
    QMap<QString, ConnectedDeviceCard*> m_deviceCards; // MAC -> Card

    // All devices table
    QTableWidget *m_allDevicesTable = nullptr;

    // Status bar
    QLabel *m_statusLabel = nullptr;
    QLabel *m_lastUpdatedLabel = nullptr;
    QLabel *m_deviceCountsLabel = nullptr;
};
