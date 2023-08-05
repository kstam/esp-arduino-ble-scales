#include "remote_scales.h"
#include "remote_scales_plugin_registry.h"

// ---------------------------------------------------------------------------------------
// ------------------------   Common RemoteScales methods    ------------------------------
// ---------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------
// ------------------------   RemoteScales methods    ------------------------------
// ---------------------------------------------------------------------------------------

void RemoteScalesScanner::initializeAsyncScan() {
  if (isRunning) return;
  cleanupDiscoveredScales();

  BLEDevice::getScan()->setAdvertisedDeviceCallbacks(this);
  BLEDevice::getScan()->setInterval(200);
  BLEDevice::getScan()->setWindow(100);
  BLEDevice::getScan()->setActiveScan(false);
  BLEDevice::getScan()->start(0, [](BLEScanResults) {}); // Set to 0 for continuous
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

void RemoteScalesScanner::onResult(BLEAdvertisedDevice advertisedDevice) {
  RemoteScales* newScales = RemoteScalesPluginRegistry::getInstance()->initialiseRemoteScales(advertisedDevice);
  if (newScales != nullptr) {
    discoveredScales.push_back(newScales);
  }
}

void RemoteScalesScanner::cleanupDiscoveredScales() {
  for (const auto& scale : discoveredScales) {
    delete scale;
  }
  discoveredScales.clear();
}

std::vector<RemoteScales*> RemoteScalesScanner::syncScan(uint16_t timeout) {
  BLEScan* scanner = BLEDevice::getScan();
  isRunning = true;
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);

  BLEScanResults scanResults = scanner->start(timeout, false);

  std::vector<RemoteScales*> scales;
  for (int i = 0; i < scanResults.getCount(); i++) {
    onResult(scanResults.getDevice(i));
  }

  scanner->clearResults();
  scanner->setActiveScan(false);

  isRunning = false;
  return scales;
}

bool RemoteScalesScanner::isScanRunning() {
  return isRunning;
}
