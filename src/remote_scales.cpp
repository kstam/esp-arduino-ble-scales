#include "remote_scales.h"
#include "remote_scales_plugin_registry.h"

// ---------------------------------------------------------------------------------------
// ------------------------   Common RemoteScales methods    ------------------------------
// ---------------------------------------------------------------------------------------

RemoteScales::RemoteScales(BLEAdvertisedDevice device) : device(device) {}

void RemoteScales::log(std::string msgFormat, ...) {
  if (!this->logCallback) return;
  va_list args;
  va_start(args, msgFormat);
  int length = vsnprintf(nullptr, 0, msgFormat.c_str(), args); // Find length of string
  std::string formattedMessage(length, '\0'); // Instantiate formatted strigng with correct length
  vsnprintf(&formattedMessage[0], length + 1, msgFormat.c_str(), args); // print formatted message in the string
  va_end(args);
  logCallback("Scale[" + device.getName() + "] " + formattedMessage);
}

void RemoteScales::setWeight(float newWeight) {
  float previousWeight = weight;
  weight = newWeight;

  if (weightCallback == nullptr) {
    return;
  }
  if (weightCallbackOnlyChanges && previousWeight == newWeight) {
    return;
  }
  weightCallback(newWeight);
}

void RemoteScales::setWeightUpdatedCallback(void (*callback)(float), bool onlyChanges) {
  weightCallbackOnlyChanges = onlyChanges;
  this->weightCallback = callback;
}

bool RemoteScales::clientConnect() {
  clientCleanup();
  log("Connecting to BLE client\n");
  client.reset(BLEDevice::createClient());
  return client->connect(&device);
}

void RemoteScales::clientCleanup() {
  if (client) {
    if (client->isConnected()) {
      log("Disconnecting from BLE client\n");
      client->disconnect();
    }
    log("Cleaning up BLE client\n");
    client.reset();
  }
}

BLERemoteService* RemoteScales::clientGetService(const BLEUUID uuid) {
  if (!clientIsConnected()) {
    log("Cannot get service, client is not connected\n");
    return nullptr;
  }
  return client->getService(uuid);
}

bool RemoteScales::clientIsConnected() { return client && client->isConnected(); };

void RemoteScales::clientSetMTU(uint16_t mtu) { client->setMTU(mtu); };

// ---------------------------------------------------------------------------------------
// ------------------------   RemoteScales methods    ------------------------------
// ---------------------------------------------------------------------------------------

void RemoteScalesScanner::initializeAsyncScan() {
  if (isRunning) return;
  cleanupDiscoveredScales();

  // We set the second parameter to true to prevent the library from storing BLEAdvertisedDevice objects
  // for devices we're not interested in. This is important because the library will otherwise run out of
  // memory after a while.
  BLEDevice::getScan()->setAdvertisedDeviceCallbacks(this, true);
  BLEDevice::getScan()->setInterval(500);
  BLEDevice::getScan()->setWindow(100);
  BLEDevice::getScan()->setActiveScan(false);
  BLEDevice::getScan()->start(0, nullptr); // Set to 0 for continuous
  isRunning = true;
}

void RemoteScalesScanner::stopAsyncScan() {
  if (!isRunning) return;
  BLEDevice::getScan()->stop();
  BLEDevice::getScan()->clearResults();
  isRunning = false;
}

void RemoteScalesScanner::restartAsyncScan() {
  stopAsyncScan();
  initializeAsyncScan();
}

void RemoteScalesScanner::onResult(BLEAdvertisedDevice device) {
  std::string addrStr(reinterpret_cast<const char*>(device.getAddress().getNative()), 6);
  if (alreadySeenAddresses.exists(addrStr)) {
    return;
  }
  if (RemoteScalesPluginRegistry::getInstance()->containsPluginForDevice(device)) {
    discoveredScales.push_back(device);
  }
}

void RemoteScalesScanner::cleanupDiscoveredScales() {
  discoveredScales.clear();
}

std::vector<BLEAdvertisedDevice> RemoteScalesScanner::syncScan(uint16_t timeout) {
  BLEScan* scanner = BLEDevice::getScan();
  isRunning = true;
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);

  BLEScanResults scanResults = scanner->start(timeout, false);

  cleanupDiscoveredScales();
  for (int i = 0; i < scanResults.getCount(); i++) {
    onResult(scanResults.getDevice(i));
  }

  scanner->clearResults();
  scanner->setActiveScan(false);

  isRunning = false;
  return discoveredScales;
}

bool RemoteScalesScanner::isScanRunning() {
  return isRunning;
}

// ---------------------------------------------------------------------------------------
// ------------------------   RemoteScalesFactory methods    -----------------------------
// ---------------------------------------------------------------------------------------
RemoteScalesFactory* RemoteScalesFactory::instance = nullptr;

std::unique_ptr<RemoteScales> RemoteScalesFactory::create(const BLEAdvertisedDevice& device) {
  if (!RemoteScalesPluginRegistry::getInstance()->containsPluginForDevice(device)) {
    return nullptr;
  }
  return RemoteScalesPluginRegistry::getInstance()->initialiseRemoteScales(device);
}
