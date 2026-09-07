#include "circular_battery_widget.h"
#include <QPainter>
#include <QPen>
#include <QFontMetrics>

CircularBatteryWidget::CircularBatteryWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMinimumSize(90, 90);
}

void CircularBatteryWidget::setBattery(int percentage, const QString &state, const QString &source)
{
    m_valid = true;
    m_percentage = qBound(0, percentage, 100);
    m_state = state;
    m_source = source;

    if (m_percentage > 50) {
        m_ringColor = QColor("#38A169"); // Green
    } else if (m_percentage >= 20) {
        m_ringColor = QColor("#DD6B20"); // Amber
    } else {
        m_ringColor = QColor("#E53E3E"); // Red
    }

    update();
}

void CircularBatteryWidget::setUnavailable()
{
    m_valid = false;
    m_percentage = 0;
    m_state = "Unavailable";
    m_source.clear();
    m_ringColor = QColor("#A0AEC0");
    update();
}

QSize CircularBatteryWidget::sizeHint() const
{
    return QSize(100, 100);
}

void CircularBatteryWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int strokeWidth = 8;
    int side = qMin(width(), height()) - strokeWidth - 8;
    if (side < 30) side = 30;

    qreal x = (width() - side) / 2.0;
    qreal y = (height() - side) / 2.0;
    QRectF ringRect(x, y, side, side);

    // 1. Draw outer background track circle along the edge
    QPen bgPen(QColor("#EDF2F7"), strokeWidth, Qt::SolidLine, Qt::FlatCap);
    painter.setPen(bgPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(ringRect, 0, 360 * 16);

    // 2. Draw active battery progress arc connecting start (top 12 o'clock) to current percentage
    if (m_valid && m_percentage > 0) {
        QPen fgPen(m_ringColor, strokeWidth, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(fgPen);

        // In Qt drawArc, positive is counter-clockwise, negative is clockwise.
        // Start angle: 90 degrees (12 o'clock).
        int startAngle = 90 * 16;
        int spanAngle = -qRound((m_percentage / 100.0) * 360.0 * 16);
        painter.drawArc(ringRect, startAngle, spanAngle);
    }

    // 3. Draw start/end connection marker at the top edge
    QPen tickPen(QColor("#CBD5E0"), 2, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(tickPen);
    qreal centerX = width() / 2.0;
    painter.drawLine(QPointF(centerX, y - 2), QPointF(centerX, y + strokeWidth + 2));

    // 4. Draw Center Battery Percentage Text
    painter.setPen(QColor("#1A202C"));
    QFont pctFont = painter.font();
    pctFont.setPointSize(side >= 90 ? 14 : 11);
    pctFont.setBold(true);
    painter.setFont(pctFont);

    QString text = m_valid ? QString("%1%").arg(m_percentage) : "--%";
    QRectF textRect(x, y + (side * 0.22), side, side * 0.35);
    painter.drawText(textRect, Qt::AlignCenter, text);

    // 5. Draw icon / status indicator inside the circle
    QFont subFont = painter.font();
    subFont.setPointSize(side >= 90 ? 9 : 8);
    subFont.setBold(false);
    painter.setFont(subFont);

    QString iconText = "🔋";
    if (m_state.contains("charge", Qt::CaseInsensitive) || m_state.contains("charging", Qt::CaseInsensitive)) {
        iconText = "⚡";
        painter.setPen(QColor("#D69E2E"));
    } else if (m_percentage == 100) {
        iconText = "✓ Full";
        painter.setPen(QColor("#38A169"));
    } else {
        painter.setPen(QColor("#718096"));
    }

    QRectF subRect(x, y + (side * 0.55), side, side * 0.3);
    painter.drawText(subRect, Qt::AlignCenter, iconText);
}
