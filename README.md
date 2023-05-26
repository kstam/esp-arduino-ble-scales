# Bluetooth scales library for ESP on Arduino Framework

This library defines3 main abstract concepts:
* A `RemoteScales`  which is used as a common interface to connect to scales, retrieve their weight and tare. It also supports a callback that is triggered when a new weight is received. 
* A `RemoteScalesScanner` which is used to scan for `RemoteScales` instances that are supported, and
* A `RemoteScalesPluginRegistry` which holds all the scales that are supported by the library. 

This allows for easy extention of the library for more bluetooth enabled scales. 

### How do implement new scales

We can do this either in this repo or in a separate repo. In both cases we need to:
1. Create a class for the new Scales (i.e. `AcaiaScales`) that implements the protocol of the scales and extends `RemoteScales`
2. Create a plugin (i.e. `AcaiaScalesPlugin`) that extends `RemoteScalesPlugin` and implement an `apply()` method which should register the plugin to the `RemoteScalesPluginRegistry` singleton.
3. Import your new library together with the `remote_scales` library and apply your plugin (i.e. `MyScalesPlugin::apply()`) during the initialisaion phase. 

