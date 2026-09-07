#include "main_window.h"
#include <QApplication>
#include <QStatusBar>
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Bluetooth Inspector - Connected Devices & Signal Monitor");
    resize(1020, 720);

    setupUi();

    // Wire up BlueZ ObjectManager events
    connect(&m_manager, &BluezObjectManager::deviceAdded, this, &MainWindow::onDeviceAdded);
    connect(&m_manager, &BluezObjectManager::deviceRemoved, this, &MainWindow::onDeviceRemoved);
    connect(&m_manager, &BluezObjectManager::devicePropertiesChanged, this, &MainWindow::onDevicePropertiesChanged);
    connect(&m_manager, &BluezObjectManager::batteryChanged, this, &MainWindow::onBatteryChanged);
    connect(&m_manager, &BluezObjectManager::batteryAdded, this, [this](const QDBusObjectPath &p, const BatteryInfo &b) {
        onBatteryChanged(p, b);
    });

    // Wire up SignalStrengthReader
    connect(&m_signalReader, &SignalStrengthReader::signalStrengthUpdated,
            this, &MainWindow::onSignalStrengthUpdated);

    // Initialize D-Bus ObjectManager
    if (!m_manager.initialize()) {
        m_statusLabel->setText("Error: Failed to connect to BlueZ system D-Bus");
        m_statusLabel->setStyleSheet("color: #E53E3E; font-weight: bold;");
    } else {
        m_statusLabel->setText("● Connected to BlueZ ObjectManager");
        m_statusLabel->setStyleSheet("color: #38A169; font-weight: bold;");
    }

    // Set initial adapter
    const auto adapters = m_manager.adapters();
    if (!adapters.isEmpty()) {
        m_currentAdapterPath = adapters.first();
    }

    // Configure auto-refresh polling timer
    connect(&m_pollTimer, &QTimer::timeout, this, &MainWindow::onRefreshTriggered);
    m_pollTimer.start(2000); // 2 seconds default

    // Initial load
    refreshDeviceViews();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto central = new QWidget(this);
    setCentralWidget(central);

    auto mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 16, 16, 12);
    mainLayout->setSpacing(14);

    // Top Header & Control Toolbar (2-row structured panel for maximum visibility)
    auto headerPanel = new QFrame(this);
    headerPanel->setStyleSheet(
        "QFrame {"
        "  background-color: #F7FAFC; border: 1px solid #E2E8F0; border-radius: 12px; padding: 6px 12px;"
        "}"
    );
    auto headerOuterLayout = new QVBoxLayout(headerPanel);
    headerOuterLayout->setContentsMargins(10, 10, 10, 10);
    headerOuterLayout->setSpacing(10);

    // --- Row 1: Brand & Adapter Status ---
    auto topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    auto titleLabel = new QLabel("📶 Bluetooth Inspector", headerPanel);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #1A202C; border: none;");
    topRow->addWidget(titleLabel);

    topRow->addStretch();

    m_adapterBadge = new QLabel("Adapter: ...", headerPanel);
    m_adapterBadge->setStyleSheet(
        "background-color: #EDF2F7; color: #2D3748; border-radius: 6px; padding: 5px 12px; font-weight: bold; font-size: 11px;"
    );
    topRow->addWidget(m_adapterBadge);

    m_powerBtn = new QPushButton("Power Off", headerPanel);
    m_powerBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF; border: 1px solid #CBD5E0; border-radius: 6px;"
        "  padding: 5px 14px; font-weight: bold; font-size: 11px; color: #2D3748;"
        "}"
        "QPushButton:hover { background-color: #EDF2F7; }"
    );
    connect(m_powerBtn, &QPushButton::clicked, this, &MainWindow::onPowerToggled);
    topRow->addWidget(m_powerBtn);

    headerOuterLayout->addLayout(topRow);

    // Subtle divider line
    auto line = new QFrame(headerPanel);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("color: #E2E8F0; background-color: #E2E8F0; max-height: 1px;");
    headerOuterLayout->addWidget(line);

    // --- Row 2: Action Controls & User Preferences ---
    auto bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(12);

    // Scan Button
    m_scanBtn = new QPushButton("🔍 Scan for Devices", headerPanel);
    m_scanBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #3182CE; border: none; border-radius: 6px;"
        "  padding: 7px 16px; font-weight: bold; font-size: 11px; color: white;"
        "}"
        "QPushButton:hover { background-color: #2B6CB0; }"
    );
    connect(m_scanBtn, &QPushButton::clicked, this, &MainWindow::onScanToggled);
    bottomRow->addWidget(m_scanBtn);

    // Refresh Button
    m_refreshBtn = new QPushButton("⟳ Refresh Now", headerPanel);
    m_refreshBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFFFFF; border: 1px solid #CBD5E0; border-radius: 6px;"
        "  padding: 7px 14px; font-weight: bold; font-size: 11px; color: #2D3748;"
        "}"
        "QPushButton:hover { background-color: #EDF2F7; }"
    );
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshTriggered);
    bottomRow->addWidget(m_refreshBtn);

    bottomRow->addStretch();

    // Battery Style Segmented Buttons
    auto batteryLabel = new QLabel("🔋 Battery Style:", headerPanel);
    batteryLabel->setStyleSheet("border: none; color: #2D3748; font-size: 12px; font-weight: bold;");
    bottomRow->addWidget(batteryLabel);

    auto batteryBtnBox = new QWidget(headerPanel);
    auto batteryBtnLayout = new QHBoxLayout(batteryBtnBox);
    batteryBtnLayout->setContentsMargins(0, 0, 0, 0);
    batteryBtnLayout->setSpacing(4);

    m_batteryCircleBtn = new QPushButton("⭕ Circle Gauge", batteryBtnBox);
    connect(m_batteryCircleBtn, &QPushButton::clicked, this, &MainWindow::onBatteryCircleBtnClicked);
    batteryBtnLayout->addWidget(m_batteryCircleBtn);

    m_batteryBarBtn = new QPushButton("█ Normal Bar", batteryBtnBox);
    connect(m_batteryBarBtn, &QPushButton::clicked, this, &MainWindow::onBatteryBarBtnClicked);
    batteryBtnLayout->addWidget(m_batteryBarBtn);

    bottomRow->addWidget(batteryBtnBox);
    updateBatteryModeButtons();

    // Spacer between preferences
    auto sep = new QLabel("|", headerPanel);
    sep->setStyleSheet("color: #CBD5E0; font-size: 14px; margin: 0 4px;");
    bottomRow->addWidget(sep);

    // Auto-Refresh Interval
    auto intervalLabel = new QLabel("⏱ Auto-Refresh:", headerPanel);
    intervalLabel->setStyleSheet("border: none; color: #2D3748; font-size: 12px; font-weight: bold;");
    bottomRow->addWidget(intervalLabel);

    m_intervalCombo = new QComboBox(headerPanel);
    m_intervalCombo->addItem("Every 1s", 1000);
    m_intervalCombo->addItem("Every 2s", 2000);
    m_intervalCombo->addItem("Every 5s", 5000);
    m_intervalCombo->addItem("Manual (Off)", 0);
    m_intervalCombo->setCurrentIndex(1); // 2s default
    m_intervalCombo->setMinimumWidth(125);
    m_intervalCombo->setStyleSheet(
        "QComboBox {"
        "  background-color: #FFFFFF; color: #1A202C; border: 1px solid #CBD5E0; border-radius: 6px;"
        "  padding: 5px 26px 5px 12px; min-height: 22px; font-size: 11px; font-weight: bold;"
        "}"
        "QComboBox:hover { border-color: #A0AEC0; }"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding; subcontrol-position: top right; width: 24px;"
        "  border-left: 1px solid #E2E8F0; border-top-right-radius: 6px; border-bottom-right-radius: 6px;"
        "  background-color: #F7FAFC;"
        "}"
        "QComboBox::down-arrow {"
        "  border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 5px solid #4A5568;"
        "  width: 0; height: 0;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF; color: #1A202C; border: 1px solid #CBD5E0;"
        "  selection-background-color: #EBF8FF; selection-color: #2B6CB0; padding: 4px; font-size: 11px;"
        "}"
    );
    connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAutoRefreshIntervalChanged);
    bottomRow->addWidget(m_intervalCombo);

    headerOuterLayout->addLayout(bottomRow);

    mainLayout->addWidget(headerPanel);

    // Tabs: Connected Devices & All Devices
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #E2E8F0; border-radius: 8px; background-color: white; }"
        "QTabBar::tab {"
        "  background-color: #EDF2F7; color: #4A5568; padding: 8px 18px; margin-right: 4px;"
        "  border-top-left-radius: 6px; border-top-right-radius: 6px; font-weight: bold; font-size: 12px;"
        "}"
        "QTabBar::tab:selected { background-color: white; color: #2B6CB0; border-bottom: 2px solid #3182CE; }"
    );

    // Tab 1: Connected Devices Scroll Area
    auto connectedScroll = new QScrollArea(m_tabWidget);
    connectedScroll->setWidgetResizable(true);
    connectedScroll->setFrameShape(QFrame::NoFrame);
    connectedScroll->setStyleSheet("background-color: #F7FAFC;");

    m_connectedContainer = new QWidget(connectedScroll);
    m_connectedCardsLayout = new QVBoxLayout(m_connectedContainer);
    m_connectedCardsLayout->setContentsMargins(14, 14, 14, 14);
    m_connectedCardsLayout->setSpacing(12);

    m_emptyConnectedLabel = new QLabel(
        "<h2>No Connected Devices Found</h2>"
        "<p style='color: #718096;'>Turn on your Bluetooth device or click <b>Scan for Devices</b> to discover and pair devices.</p>",
        m_connectedContainer
    );
    m_emptyConnectedLabel->setAlignment(Qt::AlignCenter);
    m_emptyConnectedLabel->setStyleSheet("padding: 40px;");
    m_connectedCardsLayout->addWidget(m_emptyConnectedLabel);
    m_connectedCardsLayout->addStretch();

    connectedScroll->setWidget(m_connectedContainer);
    m_tabWidget->addTab(connectedScroll, "Connected Devices (0)");

    // Tab 2: All Devices Table
    m_allDevicesTable = new QTableWidget(m_tabWidget);
    m_allDevicesTable->setColumnCount(7);
    m_allDevicesTable->setHorizontalHeaderLabels({
        "Type", "Device Name", "MAC Address", "Status", "Signal (RSSI)", "Battery", "Action"
    });
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_allDevicesTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_allDevicesTable->verticalHeader()->setVisible(false);
    m_allDevicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_allDevicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_allDevicesTable->setStyleSheet(
        "QTableWidget { gridline-color: #EDF2F7; font-size: 11px; }"
        "QHeaderView::section { background-color: #EDF2F7; padding: 6px; font-weight: bold; border: none; font-size: 11px; }"
    );

    m_tabWidget->addTab(m_allDevicesTable, "All Devices (0)");

    mainLayout->addWidget(m_tabWidget, 1);

    // Status Bar Setup
    auto sb = statusBar();
    m_statusLabel = new QLabel("Initializing...", this);
    sb->addWidget(m_statusLabel, 1);

    m_deviceCountsLabel = new QLabel("0 Connected | 0 Paired", this);
    m_deviceCountsLabel->setStyleSheet("color: #4A5568; margin-right: 14px; font-size: 11px;");
    sb->addPermanentWidget(m_deviceCountsLabel);

    m_lastUpdatedLabel = new QLabel("Updated: --:--:--", this);
    m_lastUpdatedLabel->setStyleSheet("color: #718096; font-size: 11px;");
    sb->addPermanentWidget(m_lastUpdatedLabel);
}

void MainWindow::updateBatteryModeButtons()
{
    const QString activeStyle =
        "QPushButton {"
        "  background-color: #3182CE; color: white; border: 1px solid #2B6CB0; border-radius: 6px;"
        "  padding: 5px 12px; font-size: 11px; font-weight: bold;"
        "}";
    const QString inactiveStyle =
        "QPushButton {"
        "  background-color: #FFFFFF; color: #4A5568; border: 1px solid #CBD5E0; border-radius: 6px;"
        "  padding: 5px 12px; font-size: 11px; font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #EDF2F7; color: #1A202C; }";

    if (m_globalBatteryMode == BatteryDisplayMode::Circle) {
        m_batteryCircleBtn->setStyleSheet(activeStyle);
        m_batteryBarBtn->setStyleSheet(inactiveStyle);
    } else {
        m_batteryCircleBtn->setStyleSheet(inactiveStyle);
        m_batteryBarBtn->setStyleSheet(activeStyle);
    }
}

void MainWindow::setGlobalBatteryMode(BatteryDisplayMode mode)
{
    m_globalBatteryMode = mode;
    updateBatteryModeButtons();
    for (auto card : m_deviceCards) {
        card->setBatteryDisplayMode(mode);
    }
}

void MainWindow::onBatteryCircleBtnClicked()
{
    setGlobalBatteryMode(BatteryDisplayMode::Circle);
}

void MainWindow::onBatteryBarBtnClicked()
{
    setGlobalBatteryMode(BatteryDisplayMode::NormalBar);
}

void MainWindow::updateAdapterControls()
{
    if (m_currentAdapterPath.path().isEmpty()) {
        const auto adapters = m_manager.adapters();
        if (!adapters.isEmpty()) {
            m_currentAdapterPath = adapters.first();
        }
    }

    if (m_currentAdapterPath.path().isEmpty()) {
        m_adapterBadge->setText("No Bluetooth Adapter Found");
        m_powerBtn->setEnabled(false);
        m_scanBtn->setEnabled(false);
        return;
    }

    auto adapterProps = m_manager.properties(m_currentAdapterPath, Bluez::AdapterInterface);
    QString name = adapterProps.value("Name").toString();
    bool powered = adapterProps.value("Powered").toBool();
    bool discovering = adapterProps.value("Discovering").toBool();

    m_adapterBadge->setText(QString("%1: %2 (%3)")
        .arg(m_currentAdapterPath.path().section('/', -1))
        .arg(name)
        .arg(powered ? "ON" : "OFF"));
    m_adapterBadge->setStyleSheet(QString(
        "background-color: %1; color: %2; border-radius: 6px; padding: 5px 12px; font-weight: bold; font-size: 11px;"
    ).arg(powered ? "#DEF7EC" : "#FDE8E8")
     .arg(powered ? "#03543F" : "#9B1C1C"));

    m_powerBtn->setEnabled(true);
    m_powerBtn->setText(powered ? "Power Off" : "Power On");

    m_scanBtn->setEnabled(powered);
    if (discovering) {
        m_scanBtn->setText("⏹ Stop Scanning");
        m_scanBtn->setStyleSheet(
            "QPushButton { background-color: #E53E3E; border: none; border-radius: 6px; padding: 7px 16px; font-weight: bold; font-size: 11px; color: white; }"
            "QPushButton:hover { background-color: #C53030; }"
        );
    } else {
        m_scanBtn->setText("🔍 Scan for Devices");
        m_scanBtn->setStyleSheet(
            "QPushButton { background-color: #3182CE; border: none; border-radius: 6px; padding: 7px 16px; font-weight: bold; font-size: 11px; color: white; }"
            "QPushButton:hover { background-color: #2B6CB0; }"
        );
    }
}

void MainWindow::refreshDeviceViews()
{
    updateAdapterControls();
    updateConnectedDevicesView();
    updateAllDevicesTable();
    updateStatusBar();
}

void MainWindow::updateConnectedDevicesView()
{
    const auto connected = m_manager.connectedDevices();
    m_tabWidget->setTabText(0, QString("Connected Devices (%1)").arg(connected.size()));

    if (connected.isEmpty()) {
        m_emptyConnectedLabel->setVisible(true);
        for (auto card : m_deviceCards) {
            m_connectedCardsLayout->removeWidget(card);
            card->deleteLater();
        }
        m_deviceCards.clear();
        return;
    }

    m_emptyConnectedLabel->setVisible(false);

    QSet<QString> currentConnectedMacs;

    for (const auto &devPath : connected) {
        auto props = m_manager.properties(devPath, Bluez::DeviceInterface);
        QString address = props.value("Address").toString();
        QString name = props.value("Name").toString();
        if (name.isEmpty()) name = props.value("Alias").toString();
        QString icon = props.value("Icon").toString();
        bool paired = props.value("Paired").toBool();
        bool trusted = props.value("Trusted").toBool();
        QString adapterPath = props.value("Adapter").value<QDBusObjectPath>().path();
        QString adapterName = adapterPath.section('/', -1);

        currentConnectedMacs.insert(address);

        ConnectedDeviceCard *card = m_deviceCards.value(address, nullptr);
        if (!card) {
            card = new ConnectedDeviceCard(devPath, m_connectedContainer);
            card->setBatteryDisplayMode(m_globalBatteryMode);

            connect(card, &ConnectedDeviceCard::disconnectRequested, this, &MainWindow::onDisconnectRequested);
            connect(card, &ConnectedDeviceCard::refreshSignalRequested, this, [this](const QString &mac) {
                m_signalReader.requestSignalStrength(mac);
            });

            // Insert before stretch
            m_connectedCardsLayout->insertWidget(m_connectedCardsLayout->count() - 1, card);
            m_deviceCards.insert(address, card);
        }

        card->updateDeviceData(name, address, icon, paired, trusted, adapterName);

        // Update battery (both Circle & Bar updated internally)
        auto bat = m_manager.battery(devPath);
        card->updateBattery(bat);

        // Measure live signal strength
        int dbusRssi = props.value("RSSI").toInt();
        SignalStrengthInfo sig = m_signalReader.readSignalStrength(address, dbusRssi);
        card->updateSignalStrength(sig);
    }

    // Remove any disconnected device cards
    auto existingMacs = m_deviceCards.keys();
    for (const QString &mac : existingMacs) {
        if (!currentConnectedMacs.contains(mac)) {
            auto card = m_deviceCards.take(mac);
            m_connectedCardsLayout->removeWidget(card);
            card->deleteLater();
        }
    }
}

void MainWindow::updateAllDevicesTable()
{
    const auto allDevices = m_manager.devices();
    m_tabWidget->setTabText(1, QString("All Devices (%1)").arg(allDevices.size()));

    m_allDevicesTable->setRowCount(allDevices.size());

    int row = 0;
    for (const auto &devPath : allDevices) {
        auto props = m_manager.properties(devPath, Bluez::DeviceInterface);
        QString address = props.value("Address").toString();
        QString name = props.value("Name").toString();
        if (name.isEmpty()) name = props.value("Alias").toString();
        if (name.isEmpty()) name = "Unknown Device";
        QString iconName = props.value("Icon").toString();
        bool connected = props.value("Connected").toBool();
        bool paired = props.value("Paired").toBool();

        // 0. Icon
        QString icon = "📶";
        if (iconName.contains("headset") || iconName.contains("audio")) icon = "🎧";
        else if (iconName.contains("keyboard")) icon = "⌨️";
        else if (iconName.contains("mouse")) icon = "🖱️";
        else if (iconName.contains("phone")) icon = "📱";
        auto iconItem = new QTableWidgetItem(icon);
        iconItem->setTextAlignment(Qt::AlignCenter);
        m_allDevicesTable->setItem(row, 0, iconItem);

        // 1. Name
        auto nameItem = new QTableWidgetItem(name);
        QFont nf = nameItem->font();
        nf.setBold(connected);
        nameItem->setFont(nf);
        m_allDevicesTable->setItem(row, 1, nameItem);

        // 2. MAC Address
        auto addrItem = new QTableWidgetItem(address);
        QFont af = addrItem->font();
        af.setFamily("Monospace");
        addrItem->setFont(af);
        m_allDevicesTable->setItem(row, 2, addrItem);

        // 3. Status Badge
        QString status = connected ? "Connected" : (paired ? "Paired" : "Discovered");
        auto statusItem = new QTableWidgetItem(status);
        if (connected) {
            statusItem->setForeground(QBrush(QColor("#047857")));
        } else if (paired) {
            statusItem->setForeground(QBrush(QColor("#1D4ED8")));
        } else {
            statusItem->setForeground(QBrush(QColor("#6B7280")));
        }
        m_allDevicesTable->setItem(row, 3, statusItem);

        // 4. Signal (RSSI)
        int dbusRssi = props.value("RSSI").toInt();
        SignalStrengthInfo sig = connected ? m_signalReader.readSignalStrength(address, dbusRssi)
                                           : evaluateSignalStrength(dbusRssi);
        QString sigText = sig.valid ? QString("%1 dBm (%2)").arg(sig.rssi).arg(sig.rating) : "--";
        auto sigItem = new QTableWidgetItem(sigText);
        if (sig.valid) {
            sigItem->setForeground(QBrush(QColor(sig.hexColor)));
        }
        m_allDevicesTable->setItem(row, 4, sigItem);

        // 5. Battery
        auto bat = m_manager.battery(devPath);
        QString batText = (bat.has_value() && bat->valid) ? QString("%1%").arg(bat->percentage) : "--";
        auto batItem = new QTableWidgetItem(batText);
        m_allDevicesTable->setItem(row, 5, batItem);

        // 6. Action Button
        auto actionBtn = new QPushButton(connected ? "Disconnect" : "Connect", m_allDevicesTable);
        actionBtn->setStyleSheet(connected
            ? "background-color: #FFF5F5; color: #C53030; border: 1px solid #FEB2B2; border-radius: 4px; padding: 2px 8px; font-size: 10px;"
            : "background-color: #EBF8FF; color: #2B6CB0; border: 1px solid #BEE3F8; border-radius: 4px; padding: 2px 8px; font-size: 10px;");
        if (connected) {
            connect(actionBtn, &QPushButton::clicked, this, [this, devPath]() {
                onDisconnectRequested(devPath);
            });
        } else {
            connect(actionBtn, &QPushButton::clicked, this, [this, devPath]() {
                onConnectRequested(devPath);
            });
        }
        m_allDevicesTable->setCellWidget(row, 6, actionBtn);

        m_allDevicesTable->setRowHeight(row, 36);
        row++;
    }
}

void MainWindow::updateStatusBar()
{
    int connectedCount = m_manager.connectedDevices().size();
    int totalCount = m_manager.devices().size();
    m_deviceCountsLabel->setText(QString("%1 Connected  |  %2 Total Devices").arg(connectedCount).arg(totalCount));

    m_lastUpdatedLabel->setText(QString("Updated: %1").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::onRefreshTriggered()
{
    m_manager.refresh();
    refreshDeviceViews();
}

void MainWindow::onScanToggled()
{
    if (m_currentAdapterPath.path().isEmpty()) return;

    auto props = m_manager.properties(m_currentAdapterPath, Bluez::AdapterInterface);
    bool discovering = props.value("Discovering").toBool();

    if (discovering) {
        m_manager.stopDiscovery(m_currentAdapterPath);
    } else {
        m_manager.startDiscovery(m_currentAdapterPath);
    }
    QTimer::singleShot(200, this, &MainWindow::updateAdapterControls);
}

void MainWindow::onPowerToggled()
{
    if (m_currentAdapterPath.path().isEmpty()) return;

    auto props = m_manager.properties(m_currentAdapterPath, Bluez::AdapterInterface);
    bool powered = props.value("Powered").toBool();

    m_manager.setAdapterPowered(m_currentAdapterPath, !powered);
    QTimer::singleShot(300, this, &MainWindow::updateAdapterControls);
}

void MainWindow::onAutoRefreshIntervalChanged(int index)
{
    int ms = m_intervalCombo->itemData(index).toInt();
    if (ms <= 0) {
        m_pollTimer.stop();
    } else {
        m_pollTimer.start(ms);
    }
}

void MainWindow::onSignalStrengthUpdated(const QString &macAddress, const SignalStrengthInfo &info)
{
    if (m_deviceCards.contains(macAddress)) {
        m_deviceCards[macAddress]->updateSignalStrength(info);
    }
}

void MainWindow::onDeviceAdded(const QDBusObjectPath &, const QVariantMap &)
{
    refreshDeviceViews();
}

void MainWindow::onDeviceRemoved(const QDBusObjectPath &)
{
    refreshDeviceViews();
}

void MainWindow::onDevicePropertiesChanged(const QDBusObjectPath &, const QVariantMap &)
{
    refreshDeviceViews();
}

void MainWindow::onBatteryChanged(const QDBusObjectPath &devPath, const BatteryInfo &battery)
{
    QString mac = m_manager.property(devPath, Bluez::DeviceInterface, "Address").toString();
    if (m_deviceCards.contains(mac)) {
        m_deviceCards[mac]->updateBattery(battery);
    }
}

void MainWindow::onDisconnectRequested(const QDBusObjectPath &path)
{
    m_manager.disconnectDevice(path);
    QTimer::singleShot(500, this, &MainWindow::refreshDeviceViews);
}

void MainWindow::onConnectRequested(const QDBusObjectPath &path)
{
    m_manager.connectDevice(path);
    QTimer::singleShot(1000, this, &MainWindow::refreshDeviceViews);
}

void MainWindow::onRemoveDeviceRequested(const QDBusObjectPath &path)
{
    if (!m_currentAdapterPath.path().isEmpty()) {
        m_manager.removeDevice(m_currentAdapterPath, path);
        QTimer::singleShot(500, this, &MainWindow::refreshDeviceViews);
    }
}
