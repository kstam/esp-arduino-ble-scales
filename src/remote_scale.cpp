#include "remote_scale.h"
#include "remote_scale_plugin_registry.h"

// ---------------------------------------------------------------------------------------
// ------------------------   Common RemoteScale methods    ------------------------------
// ---------------------------------------------------------------------------------------

void RemoteScale::log(const char* format, ...) {
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

void RemoteScale::setWeight(float newWeight) {
  if (weight != newWeight && weightCallback != nullptr) {
    weight = newWeight;
    weightCallback(newWeight);
  }
}


// ---------------------------------------------------------------------------------------
// ------------------------   RemoteScale methods    ------------------------------
// ---------------------------------------------------------------------------------------

void RemoteScaleScanner::initializeAsyncScan() {
  cleanupDiscoveredScales();

  BLEDevice::getScan()->setAdvertisedDeviceCallbacks(this);
  BLEDevice::getScan()->setInterval(200);
  BLEDevice::getScan()->setWindow(100);
  BLEDevice::getScan()->setActiveScan(false);
  BLEDevice::getScan()->start(0, [](BLEScanResults) {}); // Set to 0 for continuous
}

void RemoteScaleScanner::stopAsyncScan() {
  BLEDevice::getScan()->stop();
  BLEDevice::getScan()->clearResults();
}

void RemoteScaleScanner::restartAsyncScan() {
  stopAsyncScan();
  initializeAsyncScan();
}

void RemoteScaleScanner::onResult(BLEAdvertisedDevice advertisedDevice) {
  if (RemoteScalePluginRegistry::getInstance()->containsPluginForDevice(advertisedDevice)) {
    discoveredScales.push_back(RemoteScalePluginRegistry::getInstance()->initialiseRemoteScale(advertisedDevice));
  }
}

void RemoteScaleScanner::cleanupDiscoveredScales() {
  for (const auto& scale : discoveredScales) {
    delete scale;
  }
  discoveredScales.clear();
}

std::vector<RemoteScale*> RemoteScaleScanner::syncScan(uint16_t timeout) {
  BLEScan* scanner = BLEDevice::getScan();
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);

  BLEScanResults scanResults = scanner->start(timeout, false);

  std::vector<RemoteScale*> scales;
  for (int i = 0; i < scanResults.getCount(); i++) {
    RemoteScale* scale = RemoteScale::getInstance(scanResults.getDevice(i));

    if (scale != nullptr) {
      scales.push_back(scale);
    }
  }

  scanner->clearResults();
  scanner->setActiveScan(false);

  return scales;
}
