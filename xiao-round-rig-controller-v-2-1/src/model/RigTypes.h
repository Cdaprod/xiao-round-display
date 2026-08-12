#pragma once

#include <Arduino.h>

#include <cstring>

namespace rig {

enum class RigState : uint8_t {
  Booting,
  WifiConnecting,
  WifiOffline,
  ApiOffline,
  NoSession,
  Previewing,
  Recording,
  Sending,
  Error,
};

enum class UiPage : uint8_t {
  Status,
  Network,
  Device,
  Help,
};

enum class ControlAction : uint8_t {
  Poll,
  StartRecording,
  StopRecording,
};

enum class TouchKind : uint8_t {
  None,
  Tap,
  LongPress,
};

struct RigConfig {
  String wifiSsid;
  String wifiPassword;
  String apiBase = "http://192.168.0.25:8787";
  String nodeId;
  String bearerToken;
  uint32_t pollMs = 1000;

  bool wifiConfigured() const {
    return wifiSsid.length() > 0 && wifiPassword != "CHANGE_ME";
  }

  bool tokenConfigured() const {
    return bearerToken.length() > 0 && bearerToken != "CHANGE_ME_DEVICE_TOKEN";
  }
};

struct StorageBootResult {
  bool sdReady = false;
  bool configLoaded = false;
  bool templateCreated = false;
  char message[32] = {0};
};

struct TouchEvent {
  TouchKind kind = TouchKind::None;
  uint16_t x = 0;
  uint16_t y = 0;
  uint32_t durationMs = 0;
};

// POD-only result so it can safely cross a FreeRTOS queue.
struct ApiUpdate {
  bool requestOk = false;
  bool sessionPresent = false;
  RigState state = RigState::ApiOffline;
  uint32_t chunkCount = 0;
  char error[48] = {0};
  char sessionId[48] = {0};
  char sessionStatus[24] = {0};
  char desiredAction[32] = {0};
};

// POD-only screen model. Zero initialization makes memcmp safe and cheap.
struct RigSnapshot {
  RigState state = RigState::Booting;
  bool sdReady = false;
  bool configLoaded = false;
  bool wifiConnected = false;
  bool sessionPresent = false;
  bool tokenConfigured = false;
  int16_t wifiRssi = -127;
  int16_t batteryPercent = -1;
  uint32_t chunkCount = 0;
  uint32_t recordingSeconds = 0;
  char detail[48] = {0};
  char nodeId[40] = {0};
  char apiBase[72] = {0};
  char ipAddress[20] = {0};
  char sessionId[48] = {0};
};

template <size_t Size>
inline void copyText(char (&destination)[Size], const char *source) {
  if (!source) source = "";
  std::strncpy(destination, source, Size - 1);
  destination[Size - 1] = '\0';
}

template <size_t Size>
inline void copyText(char (&destination)[Size], const String &source) {
  copyText(destination, source.c_str());
}

}  // namespace rig
