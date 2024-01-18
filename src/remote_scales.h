#pragma once
#include <BLEDevice.h>
#include <Arduino.h>
#include <vector>
#include <memory>
#include <lru_cache.h>
class RemoteScales {

public:
  using LogCallback = void (*)(std::string);

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
  RemoteScales(BLEAdvertisedDevice device);
  BLEAdvertisedDevice& getDevice() { return device; }

  bool clientConnect();
  void clientCleanup();
  bool clientIsConnected();
  BLERemoteService* clientGetService(const BLEUUID uuid);
  
  void clientSetMTU(uint16_t mtu);

  void setWeight(float newWeight);
  void log(std::string msgFormat, ...);

private:

  using WeightCallback = void (*)(float);

  float weight = 0.f;

  BLEAdvertisedDevice device;

  std::unique_ptr<BLEClient> client;
  LogCallback logCallback;
  WeightCallback weightCallback;
  bool weightCallbackOnlyChanges = false;
};


// ---------------------------------------------------------------------------------------
// ---------------------------   RemoteScalesScanner    -----------------------------------
// ---------------------------------------------------------------------------------------
class RemoteScalesScanner : BLEAdvertisedDeviceCallbacks {
private:
  bool isRunning = false;
  LRUCache alreadySeenAddresses = LRUCache(100);
  std::vector<BLEAdvertisedDevice> discoveredScales;
  void cleanupDiscoveredScales();
  void onResult(BLEAdvertisedDevice advertisedDevice);

public:
  std::vector<BLEAdvertisedDevice> getDiscoveredScales() { return discoveredScales; }
  std::vector<BLEAdvertisedDevice> syncScan(uint16_t timeout);

  void initializeAsyncScan();
  void stopAsyncScan();
  void restartAsyncScan();
  bool isScanRunning();
};

// ---------------------------------------------------------------------------------------
// ---------------------------   RemoteScalesFactory    ----------------------------------
// ---------------------------------------------------------------------------------------

// This is a singleton class that is used to create RemoteScales objects from BLEAdvertisedDevice objects.
class RemoteScalesFactory {
public:
  std::unique_ptr<RemoteScales> create(const BLEAdvertisedDevice& device);

  static RemoteScalesFactory* getInstance() {
    if (instance == nullptr) {
      instance = new RemoteScalesFactory();
    }
    return instance;
  }

  RemoteScalesFactory(RemoteScalesFactory& other) = delete;
  void operator=(const RemoteScalesFactory&) = delete;

private:
  static RemoteScalesFactory* instance;
  RemoteScalesFactory() {}  // Private constructor to enforce singleton
};
