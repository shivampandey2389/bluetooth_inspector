#include "signal_strength_widget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>

SignalBarsWidget::SignalBarsWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void SignalBarsWidget::setBars(int activeBars, const QColor &color)
{
    m_activeBars = qBound(0, activeBars, 4);
    m_barColor = color;
    update();
}

QSize SignalBarsWidget::sizeHint() const
{
    return QSize(38, 30);
}

void SignalBarsWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int barWidth = 6;
    const int spacing = 3;
    const int heights[4] = { 8, 14, 20, 26 };
    const int totalHeight = 30;

    QColor inactiveColor = QColor(160, 174, 192, 100);

    for (int i = 0; i < 4; ++i) {
        int x = i * (barWidth + spacing);
        int h = heights[i];
        int y = totalHeight - h - 2;

        if (i < m_activeBars) {
            painter.setBrush(m_barColor);
            painter.setPen(Qt::NoPen);
        } else {
            painter.setBrush(inactiveColor);
            painter.setPen(Qt::NoPen);
        }

        painter.drawRoundedRect(x, y, barWidth, h, 2, 2);
    }
}

SignalStrengthWidget::SignalStrengthWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(10);

    m_barsWidget = new SignalBarsWidget(this);
    mainLayout->addWidget(m_barsWidget, 0, Qt::AlignVCenter);

    auto textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(8);

    m_rssiLabel = new QLabel("-- dBm", this);
    QFont rssiFont = m_rssiLabel->font();
    rssiFont.setPointSize(13);
    rssiFont.setBold(true);
    m_rssiLabel->setFont(rssiFont);
    topRow->addWidget(m_rssiLabel);

    m_ratingBadge = new QLabel("N/A", this);
    m_ratingBadge->setStyleSheet("background-color: #718096; color: white; border-radius: 10px; padding: 2px 8px; font-weight: bold; font-size: 11px;");
    topRow->addWidget(m_ratingBadge);
    topRow->addStretch();

    textLayout->addLayout(topRow);

    m_descriptionLabel = new QLabel("Signal strength not measured", this);
    QFont descFont = m_descriptionLabel->font();
    descFont.setPointSize(9);
    m_descriptionLabel->setFont(descFont);
    m_descriptionLabel->setStyleSheet("color: #718096;");
    m_descriptionLabel->setWordWrap(true);
    textLayout->addWidget(m_descriptionLabel);

    m_extraMetricsLabel = new QLabel(this);
    QFont extraFont = m_extraMetricsLabel->font();
    extraFont.setPointSize(8);
    m_extraMetricsLabel->setFont(extraFont);
    m_extraMetricsLabel->setStyleSheet("color: #A0AEC0;");
    m_extraMetricsLabel->setVisible(false);
    textLayout->addWidget(m_extraMetricsLabel);

    mainLayout->addLayout(textLayout, 1);
}

void SignalStrengthWidget::setSignalStrength(const SignalStrengthInfo &info)
{
    if (!info.valid) {
        setUnavailable();
        return;
    }

    m_barsWidget->setBars(info.bars, QColor(info.hexColor));
    m_rssiLabel->setText(QString("%1 dBm").arg(info.rssi));
    m_ratingBadge->setText(info.rating);
    m_ratingBadge->setStyleSheet(QString(
        "background-color: %1; color: white; border-radius: 10px; padding: 2px 8px; font-weight: bold; font-size: 11px;"
    ).arg(info.hexColor));

    m_descriptionLabel->setText(info.description);

    QStringList extras;
    if (info.linkQuality >= 0) {
        extras << QString("Link Quality: %1/255").arg(info.linkQuality);
    }
    if (info.txPower != -1) {
        extras << QString("TX Power: %1 dBm").arg(info.txPower);
    }

    if (!extras.isEmpty()) {
        m_extraMetricsLabel->setText(extras.join("  |  "));
        m_extraMetricsLabel->setVisible(true);
    } else {
        m_extraMetricsLabel->setVisible(false);
    }
}

void SignalStrengthWidget::setUnavailable()
{
    m_barsWidget->setBars(0, QColor("#718096"));
    m_rssiLabel->setText("-- dBm");
    m_ratingBadge->setText("N/A");
    m_ratingBadge->setStyleSheet("background-color: #718096; color: white; border-radius: 10px; padding: 2px 8px; font-weight: bold; font-size: 11px;");
    m_descriptionLabel->setText("Signal measurement unavailable (not connected or not in range)");
    m_extraMetricsLabel->setVisible(false);
}
