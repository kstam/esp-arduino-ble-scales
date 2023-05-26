#ifndef REMOTE_SCALES_H
#define REMOTE_SCALES_H

#include <BLEDevice.h>
#include <Arduino.h>
#include <vector>

class RemoteScales {

public:
  using WeightUpdatedCallback = void (*)(float);
  RemoteScales(BLEAdvertisedDevice device) : device(device) {}

  BLEAdvertisedDevice* getDevice() { return &device; }
  void setDebugPort(Stream* debugPort) { this->debugPort = debugPort; }
  void setWeightUpdatedCallback(WeightUpdatedCallback callback) { weightCallback = callback; }
  void setWeight(float newWeight);
  float getWeight() { return weight; }

  virtual bool tare() = 0;
  virtual bool isConnected() = 0;
  virtual bool connect() = 0;
  virtual void disconnect() = 0;
  virtual void update() = 0;
  void log(const char* format, ...);

  static RemoteScales* getInstance(BLEAdvertisedDevice device);

private:
  BLEAdvertisedDevice device;
  Stream* debugPort;
  WeightUpdatedCallback weightCallback;
  float weight = -3000.f;
};


// ---------------------------------------------------------------------------------------
// ---------------------------   RemoteScalesScanner    -----------------------------------
// ---------------------------------------------------------------------------------------

class RemoteScalesScanner : BLEAdvertisedDeviceCallbacks {
private:
  std::vector<RemoteScales*> discoveredScales;
  void cleanupDiscoveredScales();
  void onResult(BLEAdvertisedDevice advertisedDevice);

public:
  std::vector<RemoteScales*> getDiscoveredScales() { return discoveredScales; }
  std::vector<RemoteScales*> syncScan(uint16_t timeout);

  void initializeAsyncScan();
  void stopAsyncScan();
  void restartAsyncScan();
};

#endif
