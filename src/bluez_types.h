#pragma once

#include <QMap>
#include <QString>
#include <QVariantMap>
#include <QDBusObjectPath>
#include <QDBusMetaType>

using InterfaceList = QMap<QString, QVariantMap>;
using ManagedObjectList = QMap<QDBusObjectPath, InterfaceList>;

Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)

namespace Bluez {
    inline constexpr auto Service = "org.bluez";
    inline constexpr auto RootPath = "/";
    inline constexpr auto DBusObjectManagerInterface = "org.freedesktop.DBus.ObjectManager";
    inline constexpr auto DBusPropertiesInterface = "org.freedesktop.DBus.Properties";

    inline constexpr auto AdapterInterface = "org.bluez.Adapter1";
    inline constexpr auto DeviceInterface = "org.bluez.Device1";
    inline constexpr auto GattServiceInterface = "org.bluez.GattService1";
    inline constexpr auto GattCharacteristicInterface = "org.bluez.GattCharacteristic1";
    inline constexpr auto GattDescriptorInterface = "org.bluez.GattDescriptor1";

    inline void registerMetaTypes() {
        static bool registered = false;
        if (!registered) {
            qDBusRegisterMetaType<InterfaceList>();
            qDBusRegisterMetaType<ManagedObjectList>();
            registered = true;
        }
    }
}
