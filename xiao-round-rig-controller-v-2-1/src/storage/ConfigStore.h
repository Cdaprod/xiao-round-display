#pragma once

#include "model/RigTypes.h"

namespace rig {

enum class ConfigField : uint8_t {
  WifiSsid,
  WifiPassword,
  ApiBase,
  NodeId,
  BearerToken,
};

class ConfigStore {
 public:
  StorageBootResult loadAtBoot(RigConfig &config);
  bool saveOverride(
      ConfigField field,
      const char *value,
      RigConfig &config);
  bool clearOverrides();
  bool clearWifiOverrides(RigConfig &config);

 private:
  bool loadConfigFile(RigConfig &config);
  bool loadOverrides(RigConfig &config);
  bool writeTemplate();
  void appendBootEvent(
      const char *event,
      const char *detail = "");
  void assignValue(
      RigConfig &config,
      const String &key,
      const String &value);
};

}  // namespace rig
