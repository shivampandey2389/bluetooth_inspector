#pragma once

#include <QWidget>
#include <QColor>
#include <QString>

class CircularBatteryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CircularBatteryWidget(QWidget *parent = nullptr);

    void setBattery(int percentage, const QString &state = QString(), const QString &source = QString());
    void setUnavailable();

    int percentage() const { return m_percentage; }
    bool isValid() const { return m_valid; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    int m_percentage = 0;
    bool m_valid = false;
    QString m_state;
    QString m_source;
    QColor m_ringColor = QColor("#718096");
};
