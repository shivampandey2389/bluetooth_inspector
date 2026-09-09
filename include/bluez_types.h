#pragma once

#include <QMap>
#include <QString>
#include <QVariantMap>
#include <QDBusObjectPath>
#include <QDBusMetaType>

using InterfaceList = QMap<QString, QVariantMap>;
using ManagedObjectList = QMap<QDBusObjectPath, InterfaceList>;

struct BatteryInfo {
    bool valid = false;
    int percentage = -1;       // 0 - 100%
    QString source;            // e.g. "UPower", "BlueZ", "GATT"
    QString state;             // e.g. "Charging", "Discharging", "Fully Charged"
    QDBusObjectPath path;
};

struct SignalStrengthInfo {
    bool valid = false;
    int rssi = 0;              // in dBm (e.g. -42)
    int linkQuality = -1;      // 0 - 255 (if available)
    int txPower = -1;          // dBm (if available)
    int bars = 0;              // 0 to 4
    QString rating;            // "Excellent", "Good", "Fair", "Poor"
    QString description;       // Explanatory signal interpretation
    QString hexColor;          // UI color code
};

inline SignalStrengthInfo evaluateSignalStrength(int rssi, int linkQuality = -1, int txPower = -1) {
    SignalStrengthInfo info;
    info.valid = (rssi != 0);
    info.rssi = rssi;
    info.linkQuality = linkQuality;
    info.txPower = txPower;

    if (!info.valid) {
        info.bars = 0;
        info.rating = "Unknown";
        info.description = "Signal reading not available";
        info.hexColor = "#718096";
        return info;
    }

    if (rssi >= -50) {
        info.bars = 4;
        info.rating = "Excellent";
        info.description = "Strong signal, very close range, optimal throughput & zero packet loss.";
        info.hexColor = "#38A169"; // Emerald green
    } else if (rssi >= -65) {
        info.bars = 3;
        info.rating = "Good";
        info.description = "Good signal, reliable connection with low latency.";
        info.hexColor = "#48BB78"; // Light green
    } else if (rssi >= -80) {
        info.bars = 2;
        info.rating = "Fair";
        info.description = "Moderate signal. Increased distance or minor physical interference.";
        info.hexColor = "#DD6B20"; // Amber/Orange
    } else {
        info.bars = 1;
        info.rating = "Poor";
        info.description = "Weak signal. High chance of audio glitches or dropouts.";
        info.hexColor = "#E53E3E"; // Red
    }

    return info;
}

Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)
Q_DECLARE_METATYPE(BatteryInfo)
Q_DECLARE_METATYPE(SignalStrengthInfo)

namespace Bluez {
    inline constexpr auto Service = "org.bluez";
    inline constexpr auto RootPath = "/";
    inline constexpr auto DBusObjectManagerInterface = "org.freedesktop.DBus.ObjectManager";
    inline constexpr auto DBusPropertiesInterface = "org.freedesktop.DBus.Properties";

    inline constexpr auto AdapterInterface = "org.bluez.Adapter1";
    inline constexpr auto DeviceInterface = "org.bluez.Device1";
    inline constexpr auto BatteryInterface = "org.bluez.Battery1";
    inline constexpr auto GattServiceInterface = "org.bluez.GattService1";
    inline constexpr auto GattCharacteristicInterface = "org.bluez.GattCharacteristic1";
    inline constexpr auto GattDescriptorInterface = "org.bluez.GattDescriptor1";

    inline constexpr auto UPowerService = "org.freedesktop.UPower";
    inline constexpr auto UPowerPath = "/org/freedesktop/UPower";
    inline constexpr auto UPowerInterface = "org.freedesktop.UPower";
    inline constexpr auto UPowerDeviceInterface = "org.freedesktop.UPower.Device";

    inline void registerMetaTypes() {
        static bool registered = false;
        if (!registered) {
            qDBusRegisterMetaType<InterfaceList>();
            qDBusRegisterMetaType<ManagedObjectList>();
            qRegisterMetaType<BatteryInfo>();
            qRegisterMetaType<SignalStrengthInfo>();
            registered = true;
        }
    }
}
