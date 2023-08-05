#ifndef REMOTE_SCALES_H
#define REMOTE_SCALES_H

#include <BLEDevice.h>
#include <Arduino.h>
#include <vector>

class RemoteScales {

public:
  using LogCallback = void (*)(std::string);
  RemoteScales(BLEAdvertisedDevice device) : device(device) {}

  float getWeight() { return weight; }

  void setWeightUpdatedCallback(void (*callback)(float), bool onlyChanges = false);
  void setLogCallback(LogCallback logCallback) { this->logCallback = logCallback; }

  std::string getDeviceName() { return device.getName(); }
  std::string getDeviceAddress() { return device.getAddress().toString(); }

  virtual bool tare() = 0;
  virtual bool isConnected() = 0;
  virtual bool connect() = 0;
  virtual void disconnect() = 0;
  virtual void update() = 0;

protected:
  BLEAdvertisedDevice* getDevice() { return &device; }

  void setWeight(float newWeight);
  void log(std::string msgFormat, ...);

private:
  using WeightCallback = void (*)(float);

  float weight = 0.f;

  BLEAdvertisedDevice device;

  LogCallback logCallback;
  WeightCallback weightCallback;
  bool weightCallbackOnlyChanges = false;
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
