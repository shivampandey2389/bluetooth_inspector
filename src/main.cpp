#include <QCoreApplication>
#include <QDebug>
#include <csignal>
#include "bluez_object_manager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Handle signals cleanly
    std::signal(SIGINT, [](int) {
        qInfo() << "\nShutting down Bluetooth Inspector...";
        QCoreApplication::quit();
    });
    std::signal(SIGTERM, [](int) {
        QCoreApplication::quit();
    });

    qInfo() << "========================================";
    qInfo() << "      Bluetooth Inspector (BlueZ)       ";
    qInfo() << "========================================";

    BluezObjectManager manager;

    QObject::connect(&manager, &BluezObjectManager::adapterAdded, [](const QDBusObjectPath &path, const QVariantMap &props) {
        qInfo() << "[+] Adapter added:" << path.path()
                << "Name:" << props.value("Name").toString()
                << "Address:" << props.value("Address").toString();
    });

    QObject::connect(&manager, &BluezObjectManager::adapterRemoved, [](const QDBusObjectPath &path) {
        qInfo() << "[-] Adapter removed:" << path.path();
    });

    QObject::connect(&manager, &BluezObjectManager::deviceAdded, [](const QDBusObjectPath &path, const QVariantMap &props) {
        QString name = props.value("Name", props.value("Alias")).toString();
        qInfo() << "[+] Device added:" << path.path()
                << "Name:" << (name.isEmpty() ? "<Unknown>" : name)
                << "Address:" << props.value("Address").toString()
                << "Paired:" << props.value("Paired").toBool()
                << "Connected:" << props.value("Connected").toBool();
    });

    QObject::connect(&manager, &BluezObjectManager::deviceRemoved, [](const QDBusObjectPath &path) {
        qInfo() << "[-] Device removed:" << path.path();
    });

    if (!manager.initialize()) {
        qCritical() << "Failed to initialize BluezObjectManager.";
        return 1;
    }

    qInfo() << "BlueZ ObjectManager initialized successfully.";

    const auto adapters = manager.adapters();
    qInfo() << "\nFound" << adapters.size() << "Bluetooth Adapter(s):";
    for (const auto &adapterPath : adapters) {
        const auto props = manager.properties(adapterPath, Bluez::AdapterInterface);
        qInfo().noquote() << QString("  * Path: %1").arg(adapterPath.path());
        qInfo().noquote() << QString("    Name: %1").arg(props.value("Name").toString());
        qInfo().noquote() << QString("    Address: %1").arg(props.value("Address").toString());
        qInfo().noquote() << QString("    Powered: %1").arg(props.value("Powered").toBool() ? "true" : "false");
        qInfo().noquote() << QString("    Discovering: %1").arg(props.value("Discovering").toBool() ? "true" : "false");
    }

    const auto devices = manager.devices();
    qInfo() << "\nFound" << devices.size() << "Bluetooth Device(s):";
    for (const auto &devicePath : devices) {
        const auto props = manager.properties(devicePath, Bluez::DeviceInterface);
        QString name = props.value("Name").toString();
        if (name.isEmpty()) {
            name = props.value("Alias").toString();
        }
        qInfo().noquote() << QString("  * Path: %1").arg(devicePath.path());
        qInfo().noquote() << QString("    Name: %1").arg(name.isEmpty() ? "<Unknown>" : name);
        qInfo().noquote() << QString("    Address: %1").arg(props.value("Address").toString());
        qInfo().noquote() << QString("    Paired: %1").arg(props.value("Paired").toBool() ? "true" : "false");
        qInfo().noquote() << QString("    Connected: %1").arg(props.value("Connected").toBool() ? "true" : "false");
    }

    if (app.arguments().contains("--oneshot") || app.arguments().contains("-1")) {
        return 0;
    }

    qInfo() << "\nListening for Bluetooth events in real-time... (pass --oneshot to exit immediately, or press Ctrl+C to quit)\n";
    return app.exec();
}
