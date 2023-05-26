#ifndef REMOTE_SCALE_PLUGIN_REGISTRY_H
#define REMOTE_SCALE_PLUGIN_REGISTRY_H

#include "remote_scale.h"

struct RemoteScalePlugin {
  using RemoteScaleFilter = bool (*)(BLEAdvertisedDevice device);
  using RemoteScaleInitialiser = RemoteScale * (*)(BLEAdvertisedDevice device);
  std::string id;
  RemoteScaleFilter handles;
  RemoteScaleInitialiser initialise;
};

class RemoteScalePluginRegistry {
public:
  static RemoteScalePluginRegistry* getInstance() {
    if (instance == nullptr) {
      instance = new RemoteScalePluginRegistry();
    }
    return instance;
  }

  RemoteScalePluginRegistry(RemoteScalePluginRegistry& other) = delete;
  void operator=(const RemoteScalePluginRegistry&) = delete;

  void registerPlugin(RemoteScalePlugin plugin);
  bool containsPluginForDevice(BLEAdvertisedDevice device);
  RemoteScale* initialiseRemoteScale(BLEAdvertisedDevice device);

private:
  static RemoteScalePluginRegistry* instance;
  std::vector<RemoteScalePlugin> plugins;
  RemoteScalePluginRegistry() {}  // Private constructor to enforce singleton
};

#endif
