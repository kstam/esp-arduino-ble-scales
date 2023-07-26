#include "acaia.h"
#include "remote_scales_plugin_registry.h"

enum class AcaiaHeader : uint8_t {
  HEADER1 = 0xef,
  HEADER2 = 0xdd,
};

enum class AcaiaEventType : uint8_t {
  WEIGHT = 5,
  BATTERY = 6,
  TIMER = 7,
  KEY = 8,
  ACK = 11,
};

enum class AcaiaEventKey : uint8_t {
  TARE = 5,
  START = 7,
  RESET = 8,
  STOP = 9,
};

const BLEUUID serviceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");
const BLEUUID weightCharacteristicUUID("49535343-1e4d-4bd9-ba61-23c647249616");
const BLEUUID commandCharacteristicUUID("49535343-8841-43f4-a8d4-ecbe34729bb3");

std::string byteArrayToHexString(const uint8_t* byteArray, size_t length) {
  std::string hexString;
  hexString.reserve(length * 3); // Reserve space for the resulting string

  char hex[4];
  for (size_t i = 0; i < length; i++) {
    snprintf(hex, sizeof(hex), "%02X ", byteArray[i]);
    hexString.append(hex);
  }

  return hexString;
}

//-----------------------------------------------------------------------------------/
//---------------------------        PUBLIC       -----------------------------------/
//-----------------------------------------------------------------------------------/
AcaiaScales::AcaiaScales(BLEAdvertisedDevice device) : RemoteScales(device) {}

bool AcaiaScales::connect() {
  if (client.get() != nullptr && client->isConnected()) {
    return true;
  }

  client.reset(BLEDevice::createClient());
  RemoteScales::log("Connecting to %s[%s]\n", RemoteScales::getDevice()->getName().c_str(), RemoteScales::getDevice()->getAddress().toString().c_str());
  bool result = client->connect(RemoteScales::getDevice());
  if (!result) {
    client.release();
    return false;
  }

  client->setMTU(247);

  if (!performConnectionHandshake()) {
    return false;
  }
  subscribeToNotifications();
  RemoteScales::setWeight(0.f);
  return true;
}

void AcaiaScales::disconnect() {
  if (client.get() != nullptr && client->isConnected()) {
    RemoteScales::log("Disconnecting and cleaning up BLE client\n");
    client->disconnect();
    client.release();
    RemoteScales::log("Disconnected\n");
  }
}

bool AcaiaScales::isConnected() {
  return client != nullptr && client->isConnected();
}

void AcaiaScales::update() {
  if (markedForReconnection) {
    RemoteScales::log("Marked for disconnection. Will attempt to reconnect.\n");
    disconnect();
    connect();
    markedForReconnection = false;
  } else {
    sendHeartbeat();
  }
}

bool AcaiaScales::tare() {
  if (!isConnected()) return false;
  uint8_t payload[] = { 0x00 };
  sendMessage(AcaiaMessageType::TARE, payload, sizeof(payload));
  return true;
};

//-----------------------------------------------------------------------------------/
//---------------------------       PRIVATE       -----------------------------------/
//-----------------------------------------------------------------------------------/
void AcaiaScales::notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify
) {
  decodeAndHandleNotification(pData, length);
}

void AcaiaScales::decodeAndHandleNotification(uint8_t* data, size_t length) {
  int messageStart = -1;

  for (size_t i = 0; i < length - 1; i++) {
    if (data[i] == static_cast<uint8_t>(AcaiaHeader::HEADER1) && data[i + 1] == static_cast<uint8_t>(AcaiaHeader::HEADER2)) {
      messageStart = i;
      break;
    }
  }

  if (messageStart < 0 || length - messageStart < 6) {
    RemoteScales::log("Invalid message - Unexpected header: %s\n", byteArrayToHexString(data, length).c_str());
    return;
  }

  size_t messageEnd = messageStart + data[messageStart + 3] + 5;
  size_t messageLength = messageEnd - messageStart;

  if (messageEnd > length) {
    RemoteScales::log("Invalid message - length out of bounds: %s\n", byteArrayToHexString(data, length).c_str());
    return;
  }

  AcaiaMessageType messageType = static_cast<AcaiaMessageType>(data[messageStart + 2]);

  if (messageType == AcaiaMessageType::EVENT) {
    uint8_t* eventData = data + messageStart + 4;
    size_t eventDataLength = messageEnd - (messageStart + 4);
    handleScaleEventPayload(eventData, eventDataLength);
    return;
  }

  if (messageType == AcaiaMessageType::STATUS) {
    uint8_t* statusData = data + messageStart + 3;
    size_t statusDataLength = messageEnd - (messageStart + 3);
    handleScaleStatusPayload(statusData, statusDataLength);
    return;
  }

  if (messageType == AcaiaMessageType::INFO) {
    RemoteScales::log("Got info message: %s\n", byteArrayToHexString(data + messageStart, messageLength).c_str());
    // This normally means that something went wrong with the establishing a connection so we disconnect.
    markedForReconnection = true;
  }

  RemoteScales::log("Unknown message type %02X: %s\n", messageType, byteArrayToHexString(data + messageStart, messageLength).c_str());
  return;
}

void AcaiaScales::handleScaleEventPayload(const uint8_t* payload, size_t length) {
  AcaiaEventType eventType = static_cast<AcaiaEventType>(payload[0]);
  if (eventType == AcaiaEventType::WEIGHT) {
    RemoteScales::setWeight(decodeWeight(payload + 1));
  }
  else if (eventType == AcaiaEventType::ACK) {
    // Ignore for now.
    // Example: 0B 00 E0 05 5C 17 00 00 01 02 29 48
    // RemoteScales::log("ACK - %s\n", byteArrayToHexString(payload, length).c_str());
    // RemoteScales::log("Heartbeat response (weight: %0.1f time: %0.1f)\n", weight, time);
    // RemoteScales::setWeight(decodeWeight(payload + 4));
  }
  else if (eventType == AcaiaEventType::TIMER) {
    // Ignore for now
    // time = decodeTime(payload + 1);
    // RemoteScales::log("TIMER - %s", byteArrayToHexString(payload, length).c_str());
    // RemoteScales::log("Time:  %0.1f\n", time);
  }
  else if (eventType == AcaiaEventType::KEY) {
    // Ignore for now
    // AcaiaEventKey eventKey = static_cast<AcaiaEventKey>(payload[1]);
    // if (eventKey == AcaiaEventKey::TARE) {
    //   RemoteScales::setWeight(decodeWeight(payload + 2));
    //   RemoteScales::log("TARE - %s", byteArrayToHexString(payload, length).c_str());
    //   RemoteScales::log("Tare (weight:  %0.1f)\n", RemoteScales::getWeight());
    // }
    // else if (eventKey == AcaiaEventKey::START) {
    //   RemoteScales::setWeight(decodeWeight(payload + 2));
    //   RemoteScales::log("START - %s", byteArrayToHexString(payload, length).c_str());
    //   RemoteScales::log("Start (weight:  %0.1f)\n", RemoteScales::getWeight());
    // }
    // else if (eventKey == AcaiaEventKey::STOP) {
    //   time = decodeTime(payload + 2);
    //   RemoteScales::setWeight(decodeWeight(payload + 6));
    //   RemoteScales::log("STOP - %s", byteArrayToHexString(payload, length).c_str());
    //   RemoteScales::log("Stop (weight:  %0.1f, time:  %0.1f)\n", RemoteScales::getWeight(), time);
    // }
    // else if (eventKey == AcaiaEventKey::RESET) {
    //   time = decodeTime(payload + 2);
    //   RemoteScales::setWeight(decodeWeight(payload + 6));
    //   // 08 08 05 00 00 00 00 01 01 13 0E
    //   // 08 0A 05 03 00 00 00 01 01 18 0E
    //   RemoteScales::log("RESET - %s", byteArrayToHexString(payload, length).c_str());
    //   RemoteScales::log("Reset (weight:  %0.1f, time:  %0.1f)\n", RemoteScales::getWeight(), time);
    // }
    // else {
    //   RemoteScales::log("Unknown key %02X(%d) - %s\n", eventKey, eventKey, byteArrayToHexString(payload, length).c_str());
    // }
  }
  else {
    RemoteScales::log("unknown event type %02x(%d): %s\n", eventType, eventType, byteArrayToHexString(payload, length).c_str());
  }
}

void AcaiaScales::handleScaleStatusPayload(const uint8_t* data, size_t length) {
  battery = data[1] & 0x7F;
  if (data[2] == 2) {
    weightUnits = "grams";
  }
  else if (data[2] == 5) {
    weightUnits = "ounces";
  }
  else {
    weightUnits = "";
  }
  uint8_t auto_off = data[4] * 5;
  bool beep_on = (data[6] == 1);
}

float AcaiaScales::decodeWeight(const uint8_t* weightPayload) {
  float value = (weightPayload[1] << 8) | weightPayload[0];
  uint8_t scaling = weightPayload[4];

  switch (scaling) {
  case 1:
    value /= 10.0f;
    break;
  case 2:
    value /= 100.0f;
    break;
  case 3:
    value /= 1000.0f;
    break;
  case 4:
    value /= 10000.0f;
    break;
  default:
    RemoteScales::log("Invalid scaling %02X - %s \n", scaling, byteArrayToHexString(weightPayload, 6).c_str());
    return -1;
  }

  if (weightPayload[5] & 0x02) {
    value *= -1.0f;
  }

  return value;
}

float AcaiaScales::decodeTime(const uint8_t* timePayload) {
  return timePayload[0] * 60.0f + timePayload[1] + timePayload[2] / 10.0f;
}

bool AcaiaScales::performConnectionHandshake() {
  RemoteScales::log("Performing handshake\n");

  service = client->getService(serviceUUID);
  if (service == nullptr) {
    client->disconnect();
    return false;
  }
  RemoteScales::log("Got Service\n");

  weightCharacteristic = service->getCharacteristic(weightCharacteristicUUID);
  commandCharacteristic = service->getCharacteristic(commandCharacteristicUUID);
  if (weightCharacteristic == nullptr || commandCharacteristic == nullptr) {
    client->disconnect();
    return false;
  }
  RemoteScales::log("Got weightCharacteristic and commandCharacteristic\n");

  // Subscribe
  BLERemoteDescriptor* notifyDescriptor = weightCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
  RemoteScales::log("Got notifyDescriptor\n");
  if (notifyDescriptor != nullptr) {
    uint8_t value[2] = { 0x01, 0x00 };
    notifyDescriptor->writeValue(value, 2, true);
  }
  else {
    client->disconnect();
    return false;
  }

  // Identify
  sendId();
  RemoteScales::log("Send ID\n");
  sendNotificationRequest();
  RemoteScales::log("Sent notification request\n");
  lastHeartbeat = millis();
  return true;
}

void AcaiaScales::sendMessage(AcaiaMessageType msgType, const uint8_t* payload, size_t length, bool waitResponse) {
  size_t bufferSize = 5 + length;
  uint8_t* bytes = new uint8_t[bufferSize];

  bytes[0] = static_cast<uint8_t>(AcaiaHeader::HEADER1);
  bytes[1] = static_cast<uint8_t>(AcaiaHeader::HEADER2);
  bytes[2] = static_cast<uint8_t>(msgType);
  uint8_t cksum1 = 0;
  uint8_t cksum2 = 0;

  for (size_t i = 0; i < length; i++) {
    uint8_t val = static_cast<uint8_t>(payload[i]) & 0xFF;
    bytes[3 + i] = val;
    if (i % 2 == 0) {
      cksum1 += val;
    }
    else {
      cksum2 += val;
    }
  }

  bytes[length + 3] = (cksum1 & 0xFF);
  bytes[length + 4] = (cksum2 & 0xFF);

  // RemoteScales::log("Sending: %s\n", byteArrayToHexString(bytes, bufferSize).c_str());
  commandCharacteristic->writeValue(bytes, bufferSize, waitResponse);
};

void AcaiaScales::sendId() {
  const uint8_t payload[] = { 0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d };
  sendMessage(AcaiaMessageType::IDENTIFY, payload, 15, false);
}

void AcaiaScales::sendNotificationRequest() {
  uint8_t payload[] = { 0, 1, 1, 2, 2, 5, 3, 4 };
  sendEvent(payload, 8);
}

void AcaiaScales::sendEvent(const uint8_t* payload, size_t length) {
  uint8_t* bytes = new uint8_t[length + 1];
  bytes[0] = static_cast<uint8_t>(length + 1);

  for (size_t i = 0; i < length; ++i) {
    bytes[i + 1] = payload[i] & 0xFF;
  }

  sendMessage(AcaiaMessageType::EVENT, bytes, length + 1);
  delete[] bytes;
}

void AcaiaScales::sendHeartbeat() {
  if (!isConnected()) {
    return;
  }

  uint32_t now = millis();
  if (now - lastHeartbeat < 2000) {
    return;
  }

  uint8_t payload1[] = { 0x02,0x00 };
  sendMessage(AcaiaMessageType::SYSTEM, payload1, 2);
  sendNotificationRequest();
  uint8_t payload2[] = { 0x00 };
  sendMessage(AcaiaMessageType::HANDSHAKE, payload2, 1);
  lastHeartbeat = now;
}

void AcaiaScales::subscribeToNotifications() {
  RemoteScales::log("subscribeToNotifications\n");

  auto callback = [this](BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
    notifyCallback(characteristic, data, length, isNotify);
  };

  if (weightCharacteristic->canNotify()) {
    RemoteScales::log("Registering callback for weight characteristic\n");
    weightCharacteristic->registerForNotify(callback);
  }

  if (commandCharacteristic->canNotify()) {
    RemoteScales::log("Registering callback for command characteristic\n");
    commandCharacteristic->registerForNotify(callback);
  }
}
