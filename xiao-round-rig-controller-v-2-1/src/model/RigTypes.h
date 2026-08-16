#pragma once

#include <Arduino.h>

#include <cstring>

#include "input/GestureRecognizer.h"
#include "network/WifiStateMachine.h"

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

enum class CredentialSource : uint8_t { Defaults, SdCard, NvsOverride, RuntimeEdit };
inline const char *credentialSourceLabel(CredentialSource source) { switch(source){case CredentialSource::Defaults:return "DEFAULT";case CredentialSource::SdCard:return "SD";case CredentialSource::NvsOverride:return "NVS";case CredentialSource::RuntimeEdit:return "RUNTIME";}return "UNKNOWN";}

struct RigConfig {
  String wifiSsid; String wifiPassword; String apiBase="http://192.168.0.25:8787"; String nodeId; String bearerToken; uint32_t pollMs=1000; CredentialSource credentialSource=CredentialSource::Defaults; uint32_t configGeneration=0;
  bool wifiConfigured()const{return wifiSsid.length()>0&&wifiPassword!="CHANGE_ME";}
  bool tokenConfigured()const{return bearerToken.length()>0&&bearerToken!="CHANGE_ME_DEVICE_TOKEN";}
};
struct StorageBootResult {bool sdReady=false,configLoaded=false,templateCreated=false,configFound=false,configParsed=false,nvsOverrides=false;char message[32]={0};};
enum class UiMode:uint8_t{Summary,TabHolding,Expanding,Expanded,Scrolling,Collapsing,Editing,Keyboard,Confirm};

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
  bool configFound = false;
  bool configParsed = false;
  bool nvsOverrides = false;
  bool wifiConfigured = false;
  bool nodeConfigured = false;
  bool apiConfigured = false;
  bool wifiConnected = false;
  bool sessionPresent = false;
  bool tokenConfigured = false;
  bool touchHealthy = false;
  bool apiReachable = false;
  bool lanReady = false;
  bool requestInProgress = false;
  WifiStage wifiStage = WifiStage::Unconfigured;
  int16_t wifiReason = 0;
  int16_t wifiRssi = -127;
  int16_t batteryPercent = -1;
  uint16_t batteryMv = 0;
  uint32_t uptimeSeconds = 0;
  uint32_t freeHeap = 0;
  uint32_t haloFrames = 0;
  uint32_t haloDropped = 0;
  uint16_t retrySeconds = 0;
  uint16_t passwordLength = 0;
  uint32_t credentialFingerprint = 0;
  uint32_t configGeneration = 0;
  uint32_t lastAttemptDurationMs = 0;
  uint32_t retryRemainingMs = 0;
  char credentialSource[12] = {0};
  uint32_t chunkCount = 0;
  uint32_t recordingSeconds = 0;
  uint8_t selectedCategory = 0;
  uint8_t uiMode = 0;
  char detail[48] = {0};
  char nodeId[40] = {0};
  char apiBase[72] = {0};
  char ipAddress[20] = {0};
  char subnet[20] = {0};
  char hostname[64] = {0};
  char gateway[20] = {0};
  char dns[20] = {0};
  char wifiSsid[40] = {0};
  char wifiReasonText[32] = {0};
  char lastError[48] = {0};
  char sessionStatus[24] = {0};
  char desiredAction[32] = {0};
  char lastControlResponse[48] = {0};
  uint8_t scanCount = 0;
  char scanSsid[5][33] = {{0}};
  int16_t scanRssi[5] = {0};
  bool scanSecure[5] = {false};
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
