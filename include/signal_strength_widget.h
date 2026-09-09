#pragma once

#include <QWidget>
#include <QLabel>
#include "bluez_types.h"

class SignalBarsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SignalBarsWidget(QWidget *parent = nullptr);
    void setBars(int activeBars, const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    int m_activeBars = 0;
    QColor m_barColor = QColor("#718096");
};

class SignalStrengthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SignalStrengthWidget(QWidget *parent = nullptr);

    void setSignalStrength(const SignalStrengthInfo &info);
    void setUnavailable();

private:
    SignalBarsWidget *m_barsWidget = nullptr;
    QLabel *m_rssiLabel = nullptr;
    QLabel *m_ratingBadge = nullptr;
    QLabel *m_descriptionLabel = nullptr;
    QLabel *m_extraMetricsLabel = nullptr;
};
