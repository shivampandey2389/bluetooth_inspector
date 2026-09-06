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

    if (!addedConnected || !removedConnected) {
        qWarning() << "Failed to connect to ObjectManager signals:"
                   << "InterfacesAdded:" << addedConnected
                   << "InterfacesRemoved:" << removedConnected;
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
}

void BluezObjectManager::onInterfacesRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces)
{
    bool hadAdapter = interfaces.contains(Bluez::AdapterInterface);
    bool hadDevice = interfaces.contains(Bluez::DeviceInterface);

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
}
