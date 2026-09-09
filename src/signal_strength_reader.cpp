#include "signal_strength_reader.h"
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QtConcurrent>

SignalStrengthReader::SignalStrengthReader(QObject *parent)
    : QObject(parent)
{
    Bluez::registerMetaTypes();
}

SignalStrengthInfo SignalStrengthReader::readSignalStrength(const QString &macAddress, int dbusRssi)
{
    if (macAddress.trimmed().isEmpty()) {
        return evaluateSignalStrength(0);
    }

    int rssi = 0;
    int linkQuality = -1;
    int txPower = -1;
    bool foundViaHci = false;

    // 1. Try querying connected ACL RSSI via hcitool
    QProcess rssiProc;
    rssiProc.start("hcitool", {"rssi", macAddress});
    if (rssiProc.waitForFinished(150)) {
        QString out = QString::fromUtf8(rssiProc.readAllStandardOutput());
        static const QRegularExpression reRssi(R"(RSSI return value:\s*(-?\d+))");
        auto match = reRssi.match(out);
        if (match.hasMatch()) {
            rssi = match.captured(1).toInt();
            foundViaHci = true;
        }
    }

    // 2. Query Link Quality if connected
    if (foundViaHci) {
        QProcess lqProc;
        lqProc.start("hcitool", {"lq", macAddress});
        if (lqProc.waitForFinished(100)) {
            QString out = QString::fromUtf8(lqProc.readAllStandardOutput());
            static const QRegularExpression reLq(R"(Link quality:\s*(\d+))");
            auto match = reLq.match(out);
            if (match.hasMatch()) {
                linkQuality = match.captured(1).toInt();
            }
        }

        QProcess tplProc;
        tplProc.start("hcitool", {"tpl", macAddress});
        if (tplProc.waitForFinished(100)) {
            QString out = QString::fromUtf8(tplProc.readAllStandardOutput());
            static const QRegularExpression reTpl(R"(Current transmit power level:\s*(-?\d+))");
            auto match = reTpl.match(out);
            if (match.hasMatch()) {
                txPower = match.captured(1).toInt();
            }
        }
    }

    // 3. Fallback to cached D-Bus RSSI if hcitool wasn't applicable
    if (!foundViaHci && dbusRssi != 0) {
        rssi = dbusRssi;
    }

    SignalStrengthInfo info = evaluateSignalStrength(rssi, linkQuality, txPower);
    m_cache[macAddress] = info;
    return info;
}

void SignalStrengthReader::requestSignalStrength(const QString &macAddress, int dbusRssi)
{
    // Run asynchronously to ensure zero UI hiccups
    QThreadPool::globalInstance()->start([this, macAddress, dbusRssi]() {
        SignalStrengthInfo info = readSignalStrength(macAddress, dbusRssi);
        QMetaObject::invokeMethod(this, [this, macAddress, info]() {
            emit signalStrengthUpdated(macAddress, info);
        });
    });
}

void SignalStrengthReader::refreshAll(const QStringList &macAddresses)
{
    for (const QString &mac : macAddresses) {
        requestSignalStrength(mac);
    }
}
