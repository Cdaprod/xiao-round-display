#pragma once

#include <cctype>
#include <cstdint>
#include <cstring>

namespace rig {

struct PlainConfig {
  char ssid[40] = {};
  char password[65] = {};
  char api[96] = "http://192.168.0.25:8787";
  char node[40] = {};
  char token[96] = {};
};

struct ConfigStatus {
  bool sdMounted = false;
  bool fileFound = false;
  bool fileParsed = false;
  bool wifi = false;
  bool node = false;
  bool token = false;
  bool api = false;
  bool overrides = false;
};

inline bool validApiUrl(const char *value) {
  return value &&
      (std::strncmp(value, "http://", 7) == 0 ||
       std::strncmp(value, "https://", 8) == 0) &&
      std::strlen(value) > 8;
}

inline void mergeConfig(
    PlainConfig &destination,
    const PlainConfig &source,
    uint8_t mask) {
  char *destinations[] = {
      destination.ssid,
      destination.password,
      destination.api,
      destination.node,
      destination.token,
  };
  const char *values[] = {
      source.ssid,
      source.password,
      source.api,
      source.node,
      source.token,
  };
  const size_t sizes[] = {40, 65, 96, 40, 96};
  for (int index = 0; index < 5; ++index) {
    if (!(mask & (1u << index))) continue;
    std::strncpy(destinations[index], values[index], sizes[index] - 1);
    destinations[index][sizes[index] - 1] = '\0';
  }
}

inline uint32_t credentialFingerprint(
    const char *ssid,
    const char *password) {
  uint32_t hash = 2166136261u;
  const char *values[] = {ssid ? ssid : "", "|", password ? password : ""};
  for (const char *value : values) {
    while (*value) {
      hash ^= static_cast<uint8_t>(*value++);
      hash *= 16777619u;
    }
  }
  return hash;
}

inline bool diagnosticContainsSecret(
    const char *diagnostic,
    const char *password) {
  return password && password[0] && diagnostic &&
      std::strstr(diagnostic, password) != nullptr;
}

inline const char *redacted(bool set) {
  return set ? "SET" : "NOT SET";
}

inline ConfigStatus configStatus(
    const PlainConfig &config,
    bool mounted,
    bool found,
    bool parsed,
    bool overrides) {
  ConfigStatus status;
  status.sdMounted = mounted;
  status.fileFound = found;
  status.fileParsed = parsed;
  status.wifi = config.ssid[0] != '\0' &&
      std::strcmp(config.password, "CHANGE_ME") != 0;
  status.node = config.node[0] != '\0';
  status.token = config.token[0] != '\0';
  status.api = validApiUrl(config.api);
  status.overrides = overrides;
  return status;
}

}  // namespace rig
