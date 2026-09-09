#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <csignal>
#include <iostream>

#include "main_window.h"
#include "bluez_object_manager.h"
#include "signal_strength_reader.h"

int runCliMode(int argc, char *argv[])
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
    qInfo() << "      Bluetooth Inspector (CLI Mode)    ";
    qInfo() << "========================================";

    BluezObjectManager manager;
    SignalStrengthReader signalReader;

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
        QString address = props.value("Address").toString();
        QString name = props.value("Name").toString();
        if (name.isEmpty()) name = props.value("Alias").toString();
        bool connected = props.value("Connected").toBool();

        qInfo().noquote() << QString("  * Path: %1").arg(devicePath.path());
        qInfo().noquote() << QString("    Name: %1").arg(name.isEmpty() ? "<Unknown>" : name);
        qInfo().noquote() << QString("    Address: %1").arg(address);
        qInfo().noquote() << QString("    Paired: %1").arg(props.value("Paired").toBool() ? "true" : "false");
        qInfo().noquote() << QString("    Connected: %1").arg(connected ? "true" : "false");

        // Battery
        auto batteryOpt = manager.battery(devicePath);
        if (batteryOpt.has_value() && batteryOpt->valid) {
            QString batteryStr = QString("%1%").arg(batteryOpt->percentage);
            if (!batteryOpt->source.isEmpty()) batteryStr += QString(" (Source: %1)").arg(batteryOpt->source);
            if (!batteryOpt->state.isEmpty()) batteryStr += QString(" [%1]").arg(batteryOpt->state);
            qInfo().noquote() << QString("    Battery: %1").arg(batteryStr);
        } else {
            qInfo().noquote() << QString("    Battery: N/A%1")
                                     .arg(connected ? " (not reported)" : " (device disconnected)");
        }

        // Signal Strength / RSSI
        int dbusRssi = props.value("RSSI").toInt();
        SignalStrengthInfo sig = connected ? signalReader.readSignalStrength(address, dbusRssi)
                                           : evaluateSignalStrength(dbusRssi);
        if (sig.valid) {
            qInfo().noquote() << QString("    RSSI: %1 dBm (%2) - %3").arg(sig.rssi).arg(sig.rating).arg(sig.description);
        } else {
            qInfo().noquote() << QString("    RSSI: N/A%1").arg(connected ? "" : " (device disconnected)");
        }
    }

    if (app.arguments().contains("--oneshot") || app.arguments().contains("-1")) {
        return 0;
    }

    qInfo() << "\nListening for Bluetooth events... (Press Ctrl+C to quit)\n";
    return app.exec();
}

#include <QFile>

int main(int argc, char *argv[])
{
    bool forceCli = false;
    bool forceGui = false;

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--cli" || arg == "--oneshot" || arg == "-1") {
            forceCli = true;
            break;
        }
        if (arg == "--gui" || arg == "-g") {
            forceGui = true;
            break;
        }
    }

    // If on a Linux desktop but DISPLAY was not exported in the subshell, auto-set DISPLAY
    if (!forceCli && qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
        if (QFile::exists("/tmp/.X11-unix/X0")) {
            qputenv("DISPLAY", ":0");
        } else if (QFile::exists("/tmp/.X11-unix/X1")) {
            qputenv("DISPLAY", ":1");
        }
    }

    bool hasDisplay = !qEnvironmentVariableIsEmpty("DISPLAY") || !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");

    if (forceCli || (!forceGui && !hasDisplay)) {
        if (!hasDisplay && !forceCli) {
            std::cout << "[INFO] No display detected (DISPLAY/WAYLAND_DISPLAY unset). Running in CLI mode.\n"
                      << "[INFO] To open the GUI, run in a desktop terminal with DISPLAY set or pass --gui.\n\n";
        }
        return runCliMode(argc, argv);
    }

    // Launch GUI Application
    QApplication app(argc, argv);
    app.setApplicationName("Bluetooth Inspector");
    app.setApplicationDisplayName("Bluetooth Inspector");

    MainWindow window;
    window.show();

    return app.exec();
}
