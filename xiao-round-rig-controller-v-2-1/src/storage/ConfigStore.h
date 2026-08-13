#pragma once
#include "model/RigTypes.h"
namespace rig {
enum class ConfigField:uint8_t{WifiSsid,WifiPassword,ApiBase,NodeId,BearerToken};
class ConfigStore {public:StorageBootResult loadAtBoot(RigConfig&);bool saveOverride(ConfigField,const char*,RigConfig&);bool clearOverrides();bool clearWifiOverrides(RigConfig&);
 private:bool loadConfigFile(RigConfig&);bool loadOverrides(RigConfig&);bool writeTemplate();void appendBootEvent(const char*,const char*="");void assignValue(RigConfig&,const String&,const String&);};
}
