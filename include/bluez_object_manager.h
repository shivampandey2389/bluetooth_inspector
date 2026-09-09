#pragma once

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusMessage>
#include <memory>
#include <optional>
#include "bluez_types.h"

class QDBusInterface;

class BluezObjectManager : public QObject
{
    Q_OBJECT

public:
    explicit BluezObjectManager(QObject *parent = nullptr);
    ~BluezObjectManager() override;

    bool initialize();
    bool refresh();

    const ManagedObjectList &managedObjects() const;

    QList<QDBusObjectPath> objectsForInterface(const QString &interfaceName) const;
    QList<QDBusObjectPath> adapters() const;
    QList<QDBusObjectPath> devices() const;
    QList<QDBusObjectPath> connectedDevices() const;

    bool hasObject(const QDBusObjectPath &path) const;
    InterfaceList interfaces(const QDBusObjectPath &path) const;
    QVariantMap properties(const QDBusObjectPath &path, const QString &interfaceName) const;
    QVariant property(const QDBusObjectPath &path, const QString &interfaceName, const QString &propertyName) const;

    // Battery inspection
    std::optional<BatteryInfo> battery(const QDBusObjectPath &devicePath) const;
    QDBusObjectPath findDeviceForPath(const QDBusObjectPath &path) const;

    // Adapter Actions
    bool setAdapterPowered(const QDBusObjectPath &adapterPath, bool powered);
    bool startDiscovery(const QDBusObjectPath &adapterPath);
    bool stopDiscovery(const QDBusObjectPath &adapterPath);

    // Device Actions
    bool connectDevice(const QDBusObjectPath &devicePath);
    bool disconnectDevice(const QDBusObjectPath &devicePath);
    bool removeDevice(const QDBusObjectPath &adapterPath, const QDBusObjectPath &devicePath);

signals:
    void initialized();
    void interfacesAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfacesAndProperties);
    void interfacesRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces);
    void adapterAdded(const QDBusObjectPath &objectPath, const QVariantMap &properties);
    void adapterRemoved(const QDBusObjectPath &objectPath);
    void adapterPropertiesChanged(const QDBusObjectPath &objectPath, const QVariantMap &properties);
    void deviceAdded(const QDBusObjectPath &objectPath, const QVariantMap &properties);
    void deviceRemoved(const QDBusObjectPath &objectPath);
    void devicePropertiesChanged(const QDBusObjectPath &devicePath, const QVariantMap &changedProperties);
    void batteryAdded(const QDBusObjectPath &devicePath, const BatteryInfo &battery);
    void batteryChanged(const QDBusObjectPath &devicePath, const BatteryInfo &battery);
    void batteryRemoved(const QDBusObjectPath &devicePath);

private slots:
    void onInterfacesAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfacesAndProperties);
    void onInterfacesRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces);
    void onPropertiesChanged(const QString &interfaceName,
                             const QVariantMap &changedProperties,
                             const QStringList &invalidatedProperties,
                             const QDBusMessage &message);

private:
    std::unique_ptr<QDBusInterface> m_interface;
    ManagedObjectList m_managedObjects;
    bool m_initialized = false;
};
