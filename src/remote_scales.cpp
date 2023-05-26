#include "remote_scales.h"
#include "remote_scales_plugin_registry.h"

// ---------------------------------------------------------------------------------------
// ------------------------   Common RemoteScales methods    ------------------------------
// ---------------------------------------------------------------------------------------

void RemoteScales::log(const char* format, ...) {
  if (!debugPort) return;

  std::array<char, 128>buffer;
  va_list args;
  va_start(args, format);
  vsnprintf(buffer.data(), buffer.size(), format, args);
  va_end(args);
  debugPort->print("Scale[");
  debugPort->print(device.getName().c_str());
  debugPort->print("]: ");
  debugPort->print(buffer.data());
}

void RemoteScales::setWeight(float newWeight) {
  if (weight != newWeight && weightCallback != nullptr) {
    weight = newWeight;
    weightCallback(newWeight);
  }
}


// ---------------------------------------------------------------------------------------
// ------------------------   RemoteScales methods    ------------------------------
// ---------------------------------------------------------------------------------------

void RemoteScalesScanner::initializeAsyncScan() {
  cleanupDiscoveredScales();

  BLEDevice::getScan()->setAdvertisedDeviceCallbacks(this);
  BLEDevice::getScan()->setInterval(200);
  BLEDevice::getScan()->setWindow(100);
  BLEDevice::getScan()->setActiveScan(false);
  BLEDevice::getScan()->start(0, [](BLEScanResults) {}); // Set to 0 for continuous
}

void RemoteScalesScanner::stopAsyncScan() {
  BLEDevice::getScan()->stop();
  BLEDevice::getScan()->clearResults();
}

void RemoteScalesScanner::restartAsyncScan() {
  stopAsyncScan();
  initializeAsyncScan();
}

void RemoteScalesScanner::onResult(BLEAdvertisedDevice advertisedDevice) {
  if (RemoteScalesPluginRegistry::getInstance()->containsPluginForDevice(advertisedDevice)) {
    discoveredScales.push_back(RemoteScalesPluginRegistry::getInstance()->initialiseRemoteScales(advertisedDevice));
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
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);

  BLEScanResults scanResults = scanner->start(timeout, false);

  std::vector<RemoteScales*> scales;
  for (int i = 0; i < scanResults.getCount(); i++) {
    RemoteScales* scale = RemoteScales::getInstance(scanResults.getDevice(i));

    if (scale != nullptr) {
      scales.push_back(scale);
    }
  }

  scanner->clearResults();
  scanner->setActiveScan(false);

  return scales;
}
