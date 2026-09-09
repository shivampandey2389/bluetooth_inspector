#include "connected_device_card.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

ConnectedDeviceCard::ConnectedDeviceCard(const QDBusObjectPath &devicePath, QWidget *parent)
    : QFrame(parent)
    , m_devicePath(devicePath)
{
    setFrameShape(QFrame::StyledPanel);
    setStyleSheet(
        "ConnectedDeviceCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #E2E8F0;"
        "  border-radius: 12px;"
        "  padding: 12px;"
        "}"
    );

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(14);

    // Header Row: Icon + Name + Address + Connected Badge + Buttons
    auto headerRow = new QHBoxLayout();
    headerRow->setSpacing(12);

    m_iconLabel = new QLabel("🎧", this);
    QFont iconFont = m_iconLabel->font();
    iconFont.setPointSize(24);
    m_iconLabel->setFont(iconFont);
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet(
        "background-color: #EDF2F7; border-radius: 24px; border: 1px solid #CBD5E0;"
    );
    headerRow->addWidget(m_iconLabel);

    auto nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(2);
    nameLayout->setContentsMargins(0, 0, 0, 0);

    m_nameLabel = new QLabel("Bluetooth Device", this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setPointSize(14);
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    m_nameLabel->setStyleSheet("color: #1A202C;");
    nameLayout->addWidget(m_nameLabel);

    m_addressLabel = new QLabel("00:00:00:00:00:00", this);
    QFont addrFont = m_addressLabel->font();
    addrFont.setFamily("Monospace");
    addrFont.setPointSize(10);
    m_addressLabel->setFont(addrFont);
    m_addressLabel->setStyleSheet("color: #718096;");
    m_addressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    nameLayout->addWidget(m_addressLabel);

    headerRow->addLayout(nameLayout, 1);

    m_statusBadge = new QLabel("● CONNECTED", this);
    m_statusBadge->setStyleSheet(
        "background-color: #DEF7EC; color: #03543F; border: 1px solid #BCF0DA; "
        "border-radius: 12px; padding: 4px 10px; font-weight: bold; font-size: 11px;"
    );
    headerRow->addWidget(m_statusBadge);

    m_refreshSignalBtn = new QPushButton("⟳ Ping Signal", this);
    m_refreshSignalBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #EDF2F7; border: 1px solid #CBD5E0; border-radius: 6px;"
        "  padding: 5px 12px; font-size: 11px; font-weight: bold; color: #2D3748;"
        "}"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_refreshSignalBtn, &QPushButton::clicked, this, [this]() {
        emit refreshSignalRequested(m_macAddress);
    });
    headerRow->addWidget(m_refreshSignalBtn);

    m_disconnectBtn = new QPushButton("Disconnect", this);
    m_disconnectBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FFF5F5; border: 1px solid #FEB2B2; border-radius: 6px;"
        "  padding: 5px 12px; font-size: 11px; font-weight: bold; color: #C53030;"
        "}"
        "QPushButton:hover { background-color: #FED7D7; }"
    );
    connect(m_disconnectBtn, &QPushButton::clicked, this, [this]() {
        emit disconnectRequested(m_devicePath);
    });
    headerRow->addWidget(m_disconnectBtn);

    mainLayout->addLayout(headerRow);

    // Metric Columns: Signal Strength Panel & Battery Panel
    auto metricsRow = new QHBoxLayout();
    metricsRow->setSpacing(14);

    // Left Panel: Signal Strength & RSSI
    auto signalBox = new QGroupBox("Signal Strength (RSSI)", this);
    signalBox->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold; font-size: 12px; color: #2D3748; "
        "  border: 1px solid #E2E8F0; border-radius: 8px; margin-top: 8px; padding-top: 14px;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );
    auto signalLayout = new QVBoxLayout(signalBox);
    signalLayout->setContentsMargins(10, 10, 10, 10);
    m_signalWidget = new SignalStrengthWidget(signalBox);
    signalLayout->addWidget(m_signalWidget);
    metricsRow->addWidget(signalBox, 1);

    // Right Panel: Battery Level with clear Circle & Bar segmented options
    auto batteryBox = new QGroupBox(this);
    batteryBox->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold; font-size: 12px; color: #2D3748; "
        "  border: 1px solid #E2E8F0; border-radius: 8px; margin-top: 8px; padding-top: 8px;"
        "}"
    );
    auto batteryBoxOuterLayout = new QVBoxLayout(batteryBox);
    batteryBoxOuterLayout->setContentsMargins(10, 8, 10, 10);
    batteryBoxOuterLayout->setSpacing(8);

    // Header inside battery box: Title + Segmented View Buttons
    auto batteryHeader = new QHBoxLayout();
    batteryHeader->setContentsMargins(0, 0, 0, 0);

    auto batteryTitle = new QLabel("🔋 Battery Level", batteryBox);
    QFont bTitleFont = batteryTitle->font();
    bTitleFont.setBold(true);
    bTitleFont.setPointSize(10);
    batteryTitle->setFont(bTitleFont);
    batteryTitle->setStyleSheet("color: #2D3748;");
    batteryHeader->addWidget(batteryTitle);

    batteryHeader->addStretch();

    // Segmented Toggle: [ ⭕ Circle ] [ █ Bar ]
    auto btnGroupContainer = new QWidget(batteryBox);
    auto btnGroupLayout = new QHBoxLayout(btnGroupContainer);
    btnGroupLayout->setContentsMargins(0, 0, 0, 0);
    btnGroupLayout->setSpacing(4);

    m_cardCircleBtn = new QPushButton("⭕ Circle", btnGroupContainer);
    connect(m_cardCircleBtn, &QPushButton::clicked, this, [this]() {
        setBatteryDisplayMode(BatteryDisplayMode::Circle);
    });
    btnGroupLayout->addWidget(m_cardCircleBtn);

    m_cardBarBtn = new QPushButton("█ Bar", btnGroupContainer);
    connect(m_cardBarBtn, &QPushButton::clicked, this, [this]() {
        setBatteryDisplayMode(BatteryDisplayMode::NormalBar);
    });
    btnGroupLayout->addWidget(m_cardBarBtn);

    batteryHeader->addWidget(btnGroupContainer);
    batteryBoxOuterLayout->addLayout(batteryHeader);

    // Stacked Widget for the two battery display modes
    m_batteryStack = new QStackedWidget(batteryBox);

    // --- Page 0: Normal Linear Progress Bar ---
    auto normalPage = new QWidget(m_batteryStack);
    auto normalLayout = new QVBoxLayout(normalPage);
    normalLayout->setContentsMargins(4, 10, 4, 4);
    normalLayout->setSpacing(8);

    auto barRow = new QHBoxLayout();
    m_batteryBar = new QProgressBar(normalPage);
    m_batteryBar->setRange(0, 100);
    m_batteryBar->setValue(0);
    m_batteryBar->setTextVisible(false);
    m_batteryBar->setFixedHeight(16);
    m_batteryBar->setStyleSheet(
        "QProgressBar { background-color: #EDF2F7; border: 1px solid #CBD5E0; border-radius: 6px; }"
        "QProgressBar::chunk { background-color: #38A169; border-radius: 5px; }"
    );
    barRow->addWidget(m_batteryBar, 1);

    m_batteryLabel = new QLabel("N/A", normalPage);
    QFont batFont = m_batteryLabel->font();
    batFont.setPointSize(11);
    batFont.setBold(true);
    m_batteryLabel->setFont(batFont);
    m_batteryLabel->setStyleSheet("color: #2D3748;");
    barRow->addWidget(m_batteryLabel);
    normalLayout->addLayout(barRow);

    m_batteryStateLabel = new QLabel("Checking battery...", normalPage);
    m_batteryStateLabel->setStyleSheet("color: #718096; font-size: 10px;");
    normalLayout->addWidget(m_batteryStateLabel);
    normalLayout->addStretch();

    m_batteryStack->addWidget(normalPage);

    // --- Page 1: Circular Ring Gauge ---
    auto circlePage = new QWidget(m_batteryStack);
    auto circleLayout = new QVBoxLayout(circlePage);
    circleLayout->setContentsMargins(4, 4, 4, 4);
    circleLayout->setSpacing(4);

    m_circularBattery = new CircularBatteryWidget(circlePage);
    circleLayout->addWidget(m_circularBattery, 0, Qt::AlignCenter);

    m_circularStateLabel = new QLabel("Checking battery...", circlePage);
    m_circularStateLabel->setAlignment(Qt::AlignCenter);
    m_circularStateLabel->setStyleSheet("color: #718096; font-size: 10px;");
    circleLayout->addWidget(m_circularStateLabel);

    m_batteryStack->addWidget(circlePage);

    batteryBoxOuterLayout->addWidget(m_batteryStack, 1);
    metricsRow->addWidget(batteryBox, 1);

    mainLayout->addLayout(metricsRow);

    // Bottom Row: Chips for Paired, Trusted, Adapter
    auto chipsRow = new QHBoxLayout();
    chipsRow->setSpacing(8);

    auto createChip = [this](const QString &text) {
        auto chip = new QLabel(text, this);
        chip->setStyleSheet(
            "background-color: #EDF2F7; color: #4A5568; border-radius: 4px; "
            "padding: 2px 8px; font-size: 10px; font-weight: 500;"
        );
        return chip;
    };

    m_pairedChip = createChip("Paired: Yes");
    chipsRow->addWidget(m_pairedChip);

    m_trustedChip = createChip("Trusted: Yes");
    chipsRow->addWidget(m_trustedChip);

    m_adapterChip = createChip("Adapter: hci0");
    chipsRow->addWidget(m_adapterChip);

    chipsRow->addStretch();
    mainLayout->addLayout(chipsRow);

    // Default to Circular mode and apply styling
    setBatteryDisplayMode(BatteryDisplayMode::Circle);
}

void ConnectedDeviceCard::updateCardBatteryButtons()
{
    const QString activeStyle =
        "QPushButton {"
        "  background-color: #3182CE; color: white; border: 1px solid #2B6CB0; border-radius: 5px;"
        "  padding: 3px 10px; font-size: 10px; font-weight: bold;"
        "}";
    const QString inactiveStyle =
        "QPushButton {"
        "  background-color: #EDF2F7; color: #4A5568; border: 1px solid #CBD5E0; border-radius: 5px;"
        "  padding: 3px 10px; font-size: 10px; font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #E2E8F0; color: #1A202C; }";

    if (m_batteryMode == BatteryDisplayMode::Circle) {
        m_cardCircleBtn->setStyleSheet(activeStyle);
        m_cardBarBtn->setStyleSheet(inactiveStyle);
    } else {
        m_cardCircleBtn->setStyleSheet(inactiveStyle);
        m_cardBarBtn->setStyleSheet(activeStyle);
    }
}

void ConnectedDeviceCard::setBatteryDisplayMode(BatteryDisplayMode mode)
{
    m_batteryMode = mode;
    if (mode == BatteryDisplayMode::Circle) {
        m_batteryStack->setCurrentIndex(1);
    } else {
        m_batteryStack->setCurrentIndex(0);
    }
    updateCardBatteryButtons();
    emit batteryDisplayModeChanged(mode);
}

void ConnectedDeviceCard::updateDeviceData(const QString &name,
                                          const QString &address,
                                          const QString &iconName,
                                          bool paired,
                                          bool trusted,
                                          const QString &adapterName)
{
    m_macAddress = address;
    m_nameLabel->setText(name.isEmpty() ? "Unknown Device" : name);
    m_addressLabel->setText(address);

    QString icon = "📶";
    if (iconName.contains("headset") || iconName.contains("audio")) icon = "🎧";
    else if (iconName.contains("keyboard")) icon = "⌨️";
    else if (iconName.contains("mouse")) icon = "🖱️";
    else if (iconName.contains("gaming")) icon = "🎮";
    else if (iconName.contains("phone")) icon = "📱";
    m_iconLabel->setText(icon);

    m_pairedChip->setText(QString("Paired: %1").arg(paired ? "Yes" : "No"));
    m_trustedChip->setText(QString("Trusted: %1").arg(trusted ? "Yes" : "No"));
    m_adapterChip->setText(QString("Adapter: %1").arg(adapterName));
}

void ConnectedDeviceCard::updateBattery(const std::optional<BatteryInfo> &battery)
{
    if (!battery.has_value() || !battery->valid) {
        // Normal Bar
        m_batteryBar->setValue(0);
        m_batteryLabel->setText("N/A");
        m_batteryStateLabel->setText("Battery data not exposed by device");
        m_batteryBar->setStyleSheet(
            "QProgressBar { background-color: #EDF2F7; border: 1px solid #CBD5E0; border-radius: 6px; }"
            "QProgressBar::chunk { background-color: #A0AEC0; border-radius: 5px; }"
        );

        // Circular Ring
        m_circularBattery->setUnavailable();
        m_circularStateLabel->setText("Battery not exposed");
        return;
    }

    int pct = qBound(0, battery->percentage, 100);

    // 1. Update Normal Bar
    m_batteryBar->setValue(pct);
    m_batteryLabel->setText(QString("%1%").arg(pct));

    QString chunkColor = "#38A169";
    if (pct < 20) {
        chunkColor = "#E53E3E";
    } else if (pct < 50) {
        chunkColor = "#DD6B20";
    }

    m_batteryBar->setStyleSheet(QString(
        "QProgressBar { background-color: #EDF2F7; border: 1px solid #CBD5E0; border-radius: 6px; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 5px; }"
    ).arg(chunkColor));

    QString status = battery->state.isEmpty() ? "Normal" : battery->state;
    if (!battery->source.isEmpty()) {
        status += QString(" (Source: %1)").arg(battery->source);
    }
    m_batteryStateLabel->setText(status);

    // 2. Update Circular Ring
    m_circularBattery->setBattery(pct, battery->state, battery->source);
    m_circularStateLabel->setText(status);
}

void ConnectedDeviceCard::updateSignalStrength(const SignalStrengthInfo &signal)
{
    m_signalWidget->setSignalStrength(signal);
}
