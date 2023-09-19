#include "lovense.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "muse.h"

/**
 * 
 * PROTOCOL SPECIFICATION SEE 
 * 
 * https://buttplug-spec.docs.buttplug.io/docs/stpihkal/protocols/lovense/
 * 
 **/

static const char* TAG = "lovense";

#define LOVENSE_SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fb"
#define LOVENSE_CHARACTERISTIC_TX_UUID "0000fff1-0000-1000-8000-00805f9b34fb"
#define LOVENSE_CHARACTERISTIC_RX_UUID "0000fff2-0000-1000-8000-00805f9b34fb"

static NimBLECharacteristic *pLovenseCharacteristic_tx = nullptr;
static NimBLECharacteristic *pLovenseCharacteristic_rx = nullptr;

class ConnectionEventHandler : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *pServer) {
        Serial.println("Client connected to the lovense service");
    }

    void onDisconnect(NimBLEServer *pServer) {
        Serial.println("Client disconnected from the lovense service");
    }
};

std::string generate_response(const std::string& command) {
    if (command == "DeviceType;") {
        return get_device_info();
    } else if (command == "Battery;") {
        return get_battery_level();
    } else if (command == "Status:1;") {
        return get_device_status();
    } else if (command.find("AutoSwitch:") == 0) {
        return set_auto_switch_options(command);
    } else if (command.find("Vibrate:") == 0) {
        return set_vibration_speed(command);
    } else if (command == "PowerOff;") {
        return set_vibration_speed(0);
    } else {
        return "OK;";
    }
}

std::string get_device_info() {
    std::string firmwareVersion = "01";
    std::string deviceType = "G";

    // Retrieve MAC address as bytes
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Generate the full MAC address
    char macAddressStr[18];
    sprintf(macAddressStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return deviceType + ":" + firmwareVersion + ":" + macAddressStr + ";";
}

std::string get_battery_level() {
    // Always reply with 100 for battery level
    return "100;";
}

std::string get_device_status() {
    // Always reply with 2 (normal)
    return "2;";
}

bool autoSwitchTurnOff = true;
bool autoSwitchLastLevel = true;

std::string get_auto_switch_options() {
    std::string response = "AutoSwitch:";
    response += autoSwitchTurnOff ? "1:" : "0:";
    response += autoSwitchLastLevel ? "1;" : "0;";
    return response;
}

std::string set_auto_switch_options(const std::string& command) {
    if (command == "AutoSwitch:On:Off;") {
        autoSwitchTurnOff = true;
        autoSwitchLastLevel = false;
    } else if (command == "AutoSwitch:On:On;") {
        autoSwitchTurnOff = true;
        autoSwitchLastLevel = true;
    } else if (command == "AutoSwitch:Off:On;") {
        autoSwitchTurnOff = false;
        autoSwitchLastLevel = true;
    } else if (command == "AutoSwitch:Off:Off;") {
        autoSwitchTurnOff = false;
        autoSwitchLastLevel = false;
    }
    return "OK;";
}

std::string set_vibration_speed(const std::string& command) {
    // Extract the speed value from the command (e.g., "Vibrate:10;" -> "10")
    size_t startPos = command.find(":") + 1;
    size_t endPos = command.find(";");
    if (startPos == std::string::npos || endPos == std::string::npos || endPos <= startPos) {
        return "Invalid command;";
    }

    std::string speedStr = command.substr(startPos, endPos - startPos);
    int speedInt = std::stoi(speedStr);

    // Convert the integer speed value to a floating-point value between 0.0 and 1.0
    float speed = static_cast<float>(speedInt) / 20.0f;

    // For demonstration purposes, let's just print the speedFloat value to Serial
    Serial.print("Vibration Speed: ");
    Serial.println(speed);

    muse_set_intensity(speed);

    return "OK;";
}

class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.empty()) {
            return; // No value received, nothing to do
        }

        Serial.print("Received Value: ");
        Serial.println(value.c_str());

        std::string response = generate_response(value);

        Serial.print("Response Value: ");
        Serial.println(response.c_str());

        // Send the response
        pCharacteristic->setValue(response);
        pCharacteristic->notify();
    }
};

void lovense_init() {
    // Get the global server
    NimBLEServer *pServer = NimBLEDevice::getServer();

    // Set the connection event handler
    pServer->setCallbacks(new ConnectionEventHandler());

    // Create a new service
    NimBLEService *pService = pServer->createService(LOVENSE_SERVICE_UUID);

    // Create a new tx characteristic for the lovense data
    pLovenseCharacteristic_tx = pService->createCharacteristic(
        LOVENSE_CHARACTERISTIC_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    pLovenseCharacteristic_tx->createDescriptor("2904");

    // Create a new rx characteristic for the lovense data
    pLovenseCharacteristic_rx = pService->createCharacteristic(
        LOVENSE_CHARACTERISTIC_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    pLovenseCharacteristic_rx->setCallbacks(new CharacteristicCallbacks());

    // Start the service
    pService->start();

    // Start advertising the services
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(LOVENSE_SERVICE_UUID);
}
