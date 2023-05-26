#ifndef REMOTE_SCALE_H
#define REMOTE_SCALE_H

#include <BLEDevice.h>
#include <Arduino.h>
#include <vector>

class RemoteScale {

public:
  using WeightUpdatedCallback = void (*)(float);
  RemoteScale(BLEAdvertisedDevice device) : device(device) {}

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

  static RemoteScale* getInstance(BLEAdvertisedDevice device);

private:
  BLEAdvertisedDevice device;
  Stream* debugPort;
  WeightUpdatedCallback weightCallback;
  float weight = -3000.f;
};


// ---------------------------------------------------------------------------------------
// ---------------------------   RemoteScaleScanner    -----------------------------------
// ---------------------------------------------------------------------------------------

class RemoteScaleScanner : BLEAdvertisedDeviceCallbacks {
private:
  std::vector<RemoteScale*> discoveredScales;
  void cleanupDiscoveredScales();
  void onResult(BLEAdvertisedDevice advertisedDevice);

public:
  std::vector<RemoteScale*> getDiscoveredScales() { return discoveredScales; }
  std::vector<RemoteScale*> syncScan(uint16_t timeout);

  void initializeAsyncScan();
  void stopAsyncScan();
  void restartAsyncScan();
};

#endif
