#include "remote_scale_plugin_registry.h"
RemoteScalePluginRegistry* RemoteScalePluginRegistry::instance = nullptr; // or NULL, or nullptr in c++11

// ---------------------------------------------------------------------------------------
// ------------------------   RemoteScalePluginRegistry    -------------------------------
// ---------------------------------------------------------------------------------------
void RemoteScalePluginRegistry::registerPlugin(RemoteScalePlugin plugin) {
  // Check if a plugin with the same ID already exists
  for (const auto& existingPlugin : plugins) {
    if (existingPlugin.id == plugin.id) {
      return;
    }
  }

  plugins.push_back(plugin);
}

bool RemoteScalePluginRegistry::containsPluginForDevice(BLEAdvertisedDevice device) {
  for (const auto& plugin : plugins) {
    if (plugin.handles(device)) {
      return true;
    }
  }
  return false;
}

RemoteScale* RemoteScalePluginRegistry::initialiseRemoteScale(BLEAdvertisedDevice device) {
  for (const auto& plugin : plugins) {
    if (plugin.handles(device)) {
      return plugin.initialise(device);
    }
  }
  return nullptr;
}

