#pragma once

#include "model/RigTypes.h"

namespace rig {

class ConfigStore {
 public:
  StorageBootResult loadAtBoot(RigConfig &config);

 private:
  bool loadConfigFile(RigConfig &config);
  bool writeTemplate();
  void appendBootEvent(const char *event, const char *detail = "");
  void assignValue(RigConfig &config, const String &key, const String &value);
};

}  // namespace rig
