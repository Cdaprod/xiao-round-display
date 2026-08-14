#pragma once

#include <cstddef>
#include <cstdint>

namespace rig {

inline bool hostnameCharacter(char value) {
  return (value >= 'a' && value <= 'z') ||
         (value >= '0' && value <= '9') || value == '-';
}

inline void buildDeviceHostname(const char *nodeId, char *output,
                                size_t outputSize) {
  if (!output || outputSize == 0) return;
  const char prefix[] = "cda-rig-";
  size_t written = 0;
  for (size_t i = 0; prefix[i] && written + 1 < outputSize; ++i)
    output[written++] = prefix[i];
  bool previousDash = false;
  bool suffixWritten = false;
  if (!nodeId || !nodeId[0]) nodeId = "controller";
  for (size_t i = 0; nodeId[i] && written + 1 < outputSize; ++i) {
    char value = nodeId[i];
    if (value >= 'A' && value <= 'Z') value = static_cast<char>(value + ('a' - 'A'));
    if (!hostnameCharacter(value)) value = '-';
    if (value == '-' && (previousDash || written == sizeof(prefix) - 1)) continue;
    output[written++] = value;
    suffixWritten = true;
    previousDash = value == '-';
  }
  while (suffixWritten && written && output[written - 1] == '-') --written;
  if (!suffixWritten) {
    const char fallback[] = "controller";
    for (size_t i = 0; fallback[i] && written + 1 < outputSize; ++i)
      output[written++] = fallback[i];
  }
  output[written] = '\0';
}

inline bool constantTimeTokenMatch(const char *provided, const char *expected) {
  if (!provided || !expected || !expected[0]) return false;
  size_t providedLength = 0, expectedLength = 0;
  while (provided[providedLength] && providedLength < 192) ++providedLength;
  while (expected[expectedLength] && expectedLength < 192) ++expectedLength;
  uint8_t difference = static_cast<uint8_t>(providedLength ^ expectedLength);
  const size_t length = providedLength > expectedLength ? providedLength : expectedLength;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t left = i < providedLength ? static_cast<uint8_t>(provided[i]) : 0;
    const uint8_t right = i < expectedLength ? static_cast<uint8_t>(expected[i]) : 0;
    difference |= left ^ right;
  }
  return difference == 0;
}

inline bool mutatingRequestAuthorized(const char *authorization,
                                      const char *token) {
  static const char prefix[] = "Bearer ";
  if (!authorization) return false;
  for (size_t i = 0; prefix[i]; ++i)
    if (authorization[i] != prefix[i]) return false;
  return constantTimeTokenMatch(authorization + sizeof(prefix) - 1, token);
}

enum class ConnectivityState : uint8_t {
  WifiDisconnected,
  WifiAssociating,
  WifiAuthenticating,
  WifiDhcp,
  LanReady,
  ApiOffline,
  ApiReady
};

inline ConnectivityState connectivityState(bool wifiAttempting,
                                           bool lanReady,
                                           bool apiReady,
                                           uint32_t attemptElapsedMs) {
  if (lanReady) return apiReady ? ConnectivityState::ApiReady
                                : ConnectivityState::ApiOffline;
  if (!wifiAttempting) return ConnectivityState::WifiDisconnected;
  if (attemptElapsedMs < 1200) return ConnectivityState::WifiAssociating;
  if (attemptElapsedMs < 4000) return ConnectivityState::WifiAuthenticating;
  return ConnectivityState::WifiDhcp;
}

inline bool mayStartWifiAttempt(bool attemptAlreadyActive) {
  return !attemptAlreadyActive;
}

inline const char *redactedSecretState(bool configured) {
  return configured ? "configured" : "not configured";
}

}  // namespace rig
