#ifndef REMOTE_SCALES_ACAIA_H
#define REMOTE_SCALES_ACAIA_H

#include "remote_scales.h"
#include "remote_scales_plugin_registry.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <vector>
#include <memory>

enum class AcaiaMessageType : uint8_t {
  SYSTEM = 0,
  TARE = 4,
  HANDSHAKE = 6,
  INFO = 7,
  STATUS = 8,
  IDENTIFY = 11,
  EVENT = 12,
};

class AcaiaScales : public RemoteScales {

public:
  AcaiaScales(BLEAdvertisedDevice device);
  void update() override;
  bool connect() override;
  void disconnect() override;
  bool isConnected() override;
  bool tare() override;
  unsigned char getBattery();
  unsigned char getSeconds();
  bool startTimer();
  bool pauseTimer();
  bool stopTimer();

private:
  std::string weightUnits;
  float time;
  uint8_t battery;

  uint32_t lastHeartbeat = 0;

  bool markedForReconnection = false;

  std::unique_ptr<BLEClient> client;
  BLERemoteService* service;
  BLERemoteCharacteristic* weightCharacteristic;
  BLERemoteCharacteristic* commandCharacteristic;

  bool performConnectionHandshake();
  void subscribeToNotifications();
  void log();

  void sendMessage(AcaiaMessageType msgType, const uint8_t* payload, size_t length, bool waitResponse = false);
  void sendEvent(const uint8_t* payload, size_t length);
  void sendHeartbeat();
  void sendNotificationRequest();
  void sendId();
  void sendTare();
  void sendTimerCommand(uint8_t command);
  void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
  void decodeAndHandleNotification(uint8_t* pData, size_t length);
  void handleScaleEventPayload(const uint8_t* pData, size_t length);
  void handleScaleStatusPayload(const uint8_t* pData, size_t length);
  float decodeWeight(const uint8_t* weightPayload);
  float decodeTime(const uint8_t* timePayload);
};

class ScaleStatus {
public:
  ScaleStatus(uint8_t battery, const std::string& units, uint8_t auto_off, bool beep_on)
    : battery(battery), units(units), auto_off(auto_off), beep_on(beep_on) {
  }

private:
  uint8_t battery;
  std::string units;
  uint8_t auto_off;
  bool beep_on;
};

class AcaiaScalesPlugin {
public:
  static void apply() {
    RemoteScalesPlugin plugin = RemoteScalesPlugin{
      .id = "plugin-acaia",
      .handles = [](BLEAdvertisedDevice device) { return AcaiaScalesPlugin::handles(device); },
      .initialise = [](BLEAdvertisedDevice device) { return (RemoteScales*) new AcaiaScales(device); },
    };
    RemoteScalesPluginRegistry::getInstance()->registerPlugin(plugin);
  }
private:
  static bool handles(BLEAdvertisedDevice device) {
    std::string deviceName = device.getName();
    return !deviceName.empty() && (
      deviceName.find("ACAIA") == 0
      || deviceName.find("PYXIS") == 0
      || deviceName.find("LUNAR") == 0
      || deviceName.find("PROCH") == 0);
  }
};
#endif
