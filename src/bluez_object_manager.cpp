#include "bluez_object_manager.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

BluezObjectManager::BluezObjectManager(QObject *parent)
    : QObject(parent)
{
    Bluez::registerMetaTypes();
}

BluezObjectManager::~BluezObjectManager() = default;

bool BluezObjectManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!QDBusConnection::systemBus().isConnected()) {
        qWarning() << "Cannot initialize BluezObjectManager: system D-Bus is not connected.";
        return false;
    }

    // Connect to ObjectManager signals on org.bluez /
    bool addedConnected = QDBusConnection::systemBus().connect(
        Bluez::Service,
        Bluez::RootPath,
        Bluez::DBusObjectManagerInterface,
        "InterfacesAdded",
        this,
        SLOT(onInterfacesAdded(QDBusObjectPath, InterfaceList))
    );

    bool removedConnected = QDBusConnection::systemBus().connect(
        Bluez::Service,
        Bluez::RootPath,
        Bluez::DBusObjectManagerInterface,
        "InterfacesRemoved",
        this,
        SLOT(onInterfacesRemoved(QDBusObjectPath, QStringList))
    );

    // Connect to PropertiesChanged signals to track live updates (battery, RSSI, connection status, etc.)
    bool propsConnected = QDBusConnection::systemBus().connect(
        Bluez::Service,
        QString(),
        Bluez::DBusPropertiesInterface,
        "PropertiesChanged",
        this,
        SLOT(onPropertiesChanged(QString, QVariantMap, QStringList, QDBusMessage))
    );

    if (!addedConnected || !removedConnected || !propsConnected) {
        qWarning() << "ObjectManager signal connection status:"
                   << "InterfacesAdded:" << addedConnected
                   << "InterfacesRemoved:" << removedConnected
                   << "PropertiesChanged:" << propsConnected;
    }

    m_interface = std::make_unique<QDBusInterface>(
        Bluez::Service,
        Bluez::RootPath,
        Bluez::DBusObjectManagerInterface,
        QDBusConnection::systemBus(),
        this
    );

    if (!m_interface->isValid()) {
        qWarning() << "ObjectManager interface is invalid:" << m_interface->lastError().message();
        return false;
    }

    if (!refresh()) {
        return false;
    }

    m_initialized = true;
    emit initialized();
    return true;
}

bool BluezObjectManager::refresh()
{
    if (!m_interface || !m_interface->isValid()) {
        return false;
    }

    QDBusReply<ManagedObjectList> reply = m_interface->call("GetManagedObjects");
    if (!reply.isValid()) {
        qWarning() << "GetManagedObjects call failed:" << reply.error().message();
        return false;
    }

    m_managedObjects = reply.value();
    return true;
}

const ManagedObjectList &BluezObjectManager::managedObjects() const
{
    return m_managedObjects;
}

QList<QDBusObjectPath> BluezObjectManager::objectsForInterface(const QString &interfaceName) const
{
    QList<QDBusObjectPath> result;
    for (auto it = m_managedObjects.cbegin(); it != m_managedObjects.cend(); ++it) {
        if (it.value().contains(interfaceName)) {
            result.append(it.key());
        }
    }
    return result;
}

QList<QDBusObjectPath> BluezObjectManager::adapters() const
{
    return objectsForInterface(Bluez::AdapterInterface);
}

QList<QDBusObjectPath> BluezObjectManager::devices() const
{
    return objectsForInterface(Bluez::DeviceInterface);
}

QList<QDBusObjectPath> BluezObjectManager::connectedDevices() const
{
    QList<QDBusObjectPath> result;
    const auto allDevices = devices();
    for (const auto &devPath : allDevices) {
        if (property(devPath, Bluez::DeviceInterface, "Connected").toBool()) {
            result.append(devPath);
        }
    }
    return result;
}

bool BluezObjectManager::hasObject(const QDBusObjectPath &path) const
{
    return m_managedObjects.contains(path);
}

InterfaceList BluezObjectManager::interfaces(const QDBusObjectPath &path) const
{
    return m_managedObjects.value(path);
}

QVariantMap BluezObjectManager::properties(const QDBusObjectPath &path, const QString &interfaceName) const
{
    return m_managedObjects.value(path).value(interfaceName);
}

QVariant BluezObjectManager::property(const QDBusObjectPath &path, const QString &interfaceName, const QString &propertyName) const
{
    return properties(path, interfaceName).value(propertyName);
}

QDBusObjectPath BluezObjectManager::findDeviceForPath(const QDBusObjectPath &path) const
{
    if (m_managedObjects.value(path).contains(Bluez::DeviceInterface)) {
        return path;
    }

    for (const auto &ifaceProps : m_managedObjects.value(path)) {
        if (ifaceProps.contains("Device")) {
            QDBusObjectPath devRef = ifaceProps.value("Device").value<QDBusObjectPath>();
            if (!devRef.path().isEmpty() && m_managedObjects.value(devRef).contains(Bluez::DeviceInterface)) {
                return devRef;
            }
        }
    }

    for (auto it = m_managedObjects.cbegin(); it != m_managedObjects.cend(); ++it) {
        if (it.value().contains(Bluez::DeviceInterface)) {
            if (path.path().startsWith(it.key().path() + "/")) {
                return it.key();
            }
        }
    }

    return QDBusObjectPath();
}

std::optional<BatteryInfo> BluezObjectManager::battery(const QDBusObjectPath &devicePath) const
{
    // 1. Direct org.bluez.Battery1 on device
    if (m_managedObjects.contains(devicePath)) {
        const auto &devInterfaces = m_managedObjects.value(devicePath);
        if (devInterfaces.contains(Bluez::BatteryInterface)) {
            const auto &props = devInterfaces.value(Bluez::BatteryInterface);
            BatteryInfo info;
            info.valid = true;
            info.percentage = props.value("Percentage").toInt();
            info.source = props.value("Source", "BlueZ Battery1").toString();
            info.state = props.value("BatteryState").toString();
            info.path = devicePath;
            return info;
        }
    }

    // 2. Child objects or references with org.bluez.Battery1
    for (auto it = m_managedObjects.cbegin(); it != m_managedObjects.cend(); ++it) {
        if (it.value().contains(Bluez::BatteryInterface)) {
            const auto &batteryProps = it.value().value(Bluez::BatteryInterface);
            QDBusObjectPath devRef = batteryProps.value("Device").value<QDBusObjectPath>();
            if (devRef == devicePath || it.key().path().startsWith(devicePath.path() + "/")) {
                BatteryInfo info;
                info.valid = true;
                info.percentage = batteryProps.value("Percentage").toInt();
                info.source = batteryProps.value("Source", "BlueZ Battery1").toString();
                info.state = batteryProps.value("BatteryState").toString();
                info.path = it.key();
                return info;
            }
        }
    }

    // 3. Query UPower (Linux system battery service)
    QString devAddress = property(devicePath, Bluez::DeviceInterface, "Address").toString();
    QDBusInterface upower(Bluez::UPowerService, Bluez::UPowerPath, Bluez::UPowerInterface, QDBusConnection::systemBus());
    if (upower.isValid()) {
        QDBusReply<QList<QDBusObjectPath>> reply = upower.call("EnumerateDevices");
        if (reply.isValid()) {
            for (const auto &uPath : reply.value()) {
                QDBusInterface uDev(Bluez::UPowerService, uPath.path(), Bluez::UPowerDeviceInterface, QDBusConnection::systemBus());
                if (!uDev.isValid()) continue;

                QString nativePath = uDev.property("NativePath").toString();
                QString serial = uDev.property("Serial").toString();

                if (nativePath == devicePath.path() || (!devAddress.isEmpty() && serial.compare(devAddress, Qt::CaseInsensitive) == 0)) {
                    double pct = uDev.property("Percentage").toDouble();
                    uint stateVal = uDev.property("State").toUInt();
                    QString stateStr = "Discharging";
                    if (stateVal == 1) stateStr = "Charging";
                    else if (stateVal == 4) stateStr = "Fully Charged";

                    BatteryInfo info;
                    info.valid = true;
                    info.percentage = qRound(pct);
                    info.source = "UPower";
                    info.state = stateStr;
                    info.path = uPath;
                    return info;
                }
            }
        }
    }

    // 4. Fallback: GATT Battery Service characteristic (UUID 0x2A19)
    for (auto it = m_managedObjects.cbegin(); it != m_managedObjects.cend(); ++it) {
        if (it.key().path().startsWith(devicePath.path() + "/") &&
            it.value().contains(Bluez::GattCharacteristicInterface)) {
            const auto &charProps = it.value().value(Bluez::GattCharacteristicInterface);
            QString uuid = charProps.value("UUID").toString().toLower();
            if (uuid == "00002a19-0000-1000-8000-00805f9b34fb" || uuid == "2a19") {
                QByteArray val = charProps.value("Value").toByteArray();
                if (!val.isEmpty()) {
                    BatteryInfo info;
                    info.valid = true;
                    info.percentage = static_cast<uint8_t>(val[0]);
                    info.source = "GATT (0x2A19)";
                    info.path = it.key();
                    return info;
                }
            }
        }
    }

    return std::nullopt;
}

bool BluezObjectManager::setAdapterPowered(const QDBusObjectPath &adapterPath, bool powered)
{
    QDBusInterface adapter(Bluez::Service, adapterPath.path(), "org.freedesktop.DBus.Properties", QDBusConnection::systemBus());
    if (!adapter.isValid()) return false;
    QDBusReply<void> reply = adapter.call("Set", Bluez::AdapterInterface, "Powered", QVariant::fromValue(QDBusVariant(powered)));
    return reply.isValid();
}

bool BluezObjectManager::startDiscovery(const QDBusObjectPath &adapterPath)
{
    QDBusInterface adapter(Bluez::Service, adapterPath.path(), Bluez::AdapterInterface, QDBusConnection::systemBus());
    if (!adapter.isValid()) return false;
    QDBusReply<void> reply = adapter.call("StartDiscovery");
    return reply.isValid();
}

bool BluezObjectManager::stopDiscovery(const QDBusObjectPath &adapterPath)
{
    QDBusInterface adapter(Bluez::Service, adapterPath.path(), Bluez::AdapterInterface, QDBusConnection::systemBus());
    if (!adapter.isValid()) return false;
    QDBusReply<void> reply = adapter.call("StopDiscovery");
    return reply.isValid();
}

bool BluezObjectManager::connectDevice(const QDBusObjectPath &devicePath)
{
    QDBusInterface device(Bluez::Service, devicePath.path(), Bluez::DeviceInterface, QDBusConnection::systemBus());
    if (!device.isValid()) return false;
    QDBusReply<void> reply = device.call("Connect");
    return reply.isValid();
}

bool BluezObjectManager::disconnectDevice(const QDBusObjectPath &devicePath)
{
    QDBusInterface device(Bluez::Service, devicePath.path(), Bluez::DeviceInterface, QDBusConnection::systemBus());
    if (!device.isValid()) return false;
    QDBusReply<void> reply = device.call("Disconnect");
    return reply.isValid();
}

bool BluezObjectManager::removeDevice(const QDBusObjectPath &adapterPath, const QDBusObjectPath &devicePath)
{
    QDBusInterface adapter(Bluez::Service, adapterPath.path(), Bluez::AdapterInterface, QDBusConnection::systemBus());
    if (!adapter.isValid()) return false;
    QDBusReply<void> reply = adapter.call("RemoveDevice", QVariant::fromValue(devicePath));
    return reply.isValid();
}

void BluezObjectManager::onInterfacesAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfacesAndProperties)
{
    auto &obj = m_managedObjects[objectPath];
    for (auto it = interfacesAndProperties.cbegin(); it != interfacesAndProperties.cend(); ++it) {
        obj.insert(it.key(), it.value());
    }

    emit interfacesAdded(objectPath, interfacesAndProperties);

    if (interfacesAndProperties.contains(Bluez::AdapterInterface)) {
        emit adapterAdded(objectPath, interfacesAndProperties.value(Bluez::AdapterInterface));
    }
    if (interfacesAndProperties.contains(Bluez::DeviceInterface)) {
        emit deviceAdded(objectPath, interfacesAndProperties.value(Bluez::DeviceInterface));
    }
    if (interfacesAndProperties.contains(Bluez::BatteryInterface)) {
        QDBusObjectPath devPath = findDeviceForPath(objectPath);
        const auto &props = interfacesAndProperties.value(Bluez::BatteryInterface);
        BatteryInfo info;
        info.valid = true;
        info.percentage = props.value("Percentage").toInt();
        info.source = props.value("Source", "BlueZ Battery1").toString();
        info.state = props.value("BatteryState").toString();
        info.path = objectPath;
        emit batteryAdded(devPath.path().isEmpty() ? objectPath : devPath, info);
    }
}

void BluezObjectManager::onInterfacesRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces)
{
    bool hadAdapter = interfaces.contains(Bluez::AdapterInterface);
    bool hadDevice = interfaces.contains(Bluez::DeviceInterface);
    bool hadBattery = interfaces.contains(Bluez::BatteryInterface);
    QDBusObjectPath devPath = hadBattery ? findDeviceForPath(objectPath) : QDBusObjectPath();

    if (m_managedObjects.contains(objectPath)) {
        auto &obj = m_managedObjects[objectPath];
        for (const QString &iface : interfaces) {
            obj.remove(iface);
        }
        if (obj.isEmpty()) {
            m_managedObjects.remove(objectPath);
        }
    }

    emit interfacesRemoved(objectPath, interfaces);

    if (hadAdapter) {
        emit adapterRemoved(objectPath);
    }
    if (hadDevice) {
        emit deviceRemoved(objectPath);
    }
    if (hadBattery) {
        emit batteryRemoved(devPath.path().isEmpty() ? objectPath : devPath);
    }
}

void BluezObjectManager::onPropertiesChanged(const QString &interfaceName,
                                             const QVariantMap &changedProperties,
                                             const QStringList &invalidatedProperties,
                                             const QDBusMessage &message)
{
    QDBusObjectPath path(message.path());
    if (path.path().isEmpty()) {
        return;
    }

    auto &obj = m_managedObjects[path];
    auto &props = obj[interfaceName];
    for (auto it = changedProperties.cbegin(); it != changedProperties.cend(); ++it) {
        props.insert(it.key(), it.value());
    }
    for (const QString &propName : invalidatedProperties) {
        props.remove(propName);
    }

    if (interfaceName == Bluez::BatteryInterface) {
        QDBusObjectPath devPath = findDeviceForPath(path);
        BatteryInfo info;
        info.valid = true;
        info.percentage = props.value("Percentage").toInt();
        info.source = props.value("Source", "BlueZ Battery1").toString();
        info.state = props.value("BatteryState").toString();
        info.path = path;

        emit batteryChanged(devPath.path().isEmpty() ? path : devPath, info);
    } else if (interfaceName == Bluez::GattCharacteristicInterface) {
        QString uuid = props.value("UUID").toString().toLower();
        if ((uuid == "00002a19-0000-1000-8000-00805f9b34fb" || uuid == "2a19") && changedProperties.contains("Value")) {
            QByteArray val = changedProperties.value("Value").toByteArray();
            if (!val.isEmpty()) {
                QDBusObjectPath devPath = findDeviceForPath(path);
                BatteryInfo info;
                info.valid = true;
                info.percentage = static_cast<uint8_t>(val[0]);
                info.source = "GATT (0x2A19)";
                info.path = path;

                emit batteryChanged(devPath.path().isEmpty() ? path : devPath, info);
            }
        }
    } else if (interfaceName == Bluez::DeviceInterface) {
        emit devicePropertiesChanged(path, changedProperties);
    } else if (interfaceName == Bluez::AdapterInterface) {
        emit adapterPropertiesChanged(path, changedProperties);
    }
}
