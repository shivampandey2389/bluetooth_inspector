#pragma once

#include <QObject>
#include <QDBusObjectPath>
#include <memory>
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

    bool hasObject(const QDBusObjectPath &path) const;
    InterfaceList interfaces(const QDBusObjectPath &path) const;
    QVariantMap properties(const QDBusObjectPath &path, const QString &interfaceName) const;
    QVariant property(const QDBusObjectPath &path, const QString &interfaceName, const QString &propertyName) const;

signals:
    void initialized();
    void interfacesAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfacesAndProperties);
    void interfacesRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces);
    void adapterAdded(const QDBusObjectPath &objectPath, const QVariantMap &properties);
    void adapterRemoved(const QDBusObjectPath &objectPath);
    void deviceAdded(const QDBusObjectPath &objectPath, const QVariantMap &properties);
    void deviceRemoved(const QDBusObjectPath &objectPath);

private slots:
    void onInterfacesAdded(const QDBusObjectPath &objectPath, const InterfaceList &interfacesAndProperties);
    void onInterfacesRemoved(const QDBusObjectPath &objectPath, const QStringList &interfaces);

private:
    std::unique_ptr<QDBusInterface> m_interface;
    ManagedObjectList m_managedObjects;
    bool m_initialized = false;
};
