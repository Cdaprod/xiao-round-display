#include "storage/ConfigStore.h"

#include <SD.h>
#include <SPI.h>
#include <Preferences.h>

#include "RigBuildConfig.h"

namespace rig {
namespace {

String trimCopy(String value) {
  value.trim();
  return value;
}

}  // namespace

StorageBootResult ConfigStore::loadAtBoot(RigConfig &config) {
  StorageBootResult result{};

  pinMode(build::kPinLcdCs, OUTPUT);
  digitalWrite(build::kPinLcdCs, HIGH);
  pinMode(build::kPinSdCs, OUTPUT);
  digitalWrite(build::kPinSdCs, HIGH);

  SPI.begin(SCK, MISO, MOSI, build::kPinSdCs);
  result.sdReady = SD.begin(build::kPinSdCs, SPI, 4000000);

  if (!result.sdReady) {
    copyText(result.message, "SD NOT FOUND");
    result.nvsOverrides = loadOverrides(config);
    SPI.end();
    return result;
  }

  result.configFound = SD.exists("/rig.cfg");
  if (!result.configFound) {
    result.templateCreated = writeTemplate();
  }

  result.configParsed = loadConfigFile(config);
  result.configLoaded = result.configParsed;
  result.nvsOverrides = loadOverrides(config);
  if (result.configParsed) {
    copyText(result.message, "CONFIG READY");
    appendBootEvent("config_ready", config.nodeId.c_str());
  } else if (result.templateCreated) {
    copyText(result.message, "EDIT /rig.cfg");
    appendBootEvent("config_template_created");
  } else {
    copyText(result.message, "CONFIG INVALID");
    appendBootEvent("config_invalid");
  }

  // DMA owns the same physical SPI pins after boot. Finish every SD operation,
  // unmount, and release Arduino SPI before the display driver starts.
  SD.end();
  SPI.end();
  digitalWrite(build::kPinSdCs, HIGH);
  return result;
}

bool ConfigStore::loadConfigFile(RigConfig &config) {
  File file = SD.open("/rig.cfg", FILE_READ);
  if (!file) return false;

  while (file.available()) {
    String line = trimCopy(file.readStringUntil('\n'));
    if (!line.length() || line.startsWith("#")) continue;
    const int separator = line.indexOf('=');
    if (separator <= 0) continue;
    assignValue(
        config,
        trimCopy(line.substring(0, separator)),
        trimCopy(line.substring(separator + 1)));
  }
  file.close();

  if (config.apiBase.endsWith("/")) {
    config.apiBase.remove(config.apiBase.length() - 1);
  }
  if (config.nodeId.startsWith("CHANGE_ME")) config.nodeId = "";
  if (config.bearerToken.startsWith("CHANGE_ME")) config.bearerToken = "";
  return true;
}

bool ConfigStore::writeTemplate() {
  File file = SD.open("/rig.cfg", FILE_WRITE);
  if (!file) return false;
  file.println("# CDAProd XIAO Round Rig Controller");
  file.println("wifi_ssid=cda_Lab");
  file.println("wifi_password=CHANGE_ME");
  file.println("api_base=http://192.168.0.25:8787");
  file.println("node_id=CHANGE_ME_CAMERA_NODE_ID");
  file.println("bearer_token=CHANGE_ME_DEVICE_TOKEN");
  file.println("poll_ms=1000");
  file.close();
  return true;
}

void ConfigStore::appendBootEvent(const char *event, const char *detail) {
  File log = SD.open("/rig-events.csv", FILE_APPEND);
  if (!log) return;
  log.printf("%lu,%s,%s\n", millis(), event, detail ? detail : "");
  log.close();
}

void ConfigStore::assignValue(
    RigConfig &config,
    const String &key,
    const String &value) {
  if (key == "wifi_ssid") config.wifiSsid = value;
  else if (key == "wifi_password") config.wifiPassword = value;
  else if (key == "api_base") config.apiBase = value;
  else if (key == "node_id") config.nodeId = value;
  else if (key == "bearer_token") config.bearerToken = value;
  else if (key == "poll_ms") {
    config.pollMs = constrain(value.toInt(), 500L, 10000L);
  }
}

bool ConfigStore::loadOverrides(RigConfig &config) {
  Preferences prefs; if (!prefs.begin("rig-config", true)) return false;
  const bool active = prefs.getBool("active", false);
  if (prefs.isKey("ssid")) config.wifiSsid = prefs.getString("ssid", config.wifiSsid);
  if (prefs.isKey("pass")) config.wifiPassword = prefs.getString("pass", config.wifiPassword);
  if (prefs.isKey("api")) config.apiBase = prefs.getString("api", config.apiBase);
  if (prefs.isKey("node")) config.nodeId = prefs.getString("node", config.nodeId);
  if (prefs.isKey("token")) config.bearerToken = prefs.getString("token", config.bearerToken);
  prefs.end(); return active;
}

bool ConfigStore::saveOverride(ConfigField field, const char *value, RigConfig &config) {
  if (!value) return false; String candidate(value);
  if (field != ConfigField::WifiPassword) candidate.trim();
  if (field == ConfigField::ApiBase && !(candidate.startsWith("http://") || candidate.startsWith("https://"))) return false;
  Preferences prefs; if (!prefs.begin("rig-config", false)) return false;
  const char *key = field==ConfigField::WifiSsid?"ssid":field==ConfigField::WifiPassword?"pass":field==ConfigField::ApiBase?"api":field==ConfigField::NodeId?"node":"token";
  const String current = field==ConfigField::WifiSsid?config.wifiSsid:field==ConfigField::WifiPassword?config.wifiPassword:field==ConfigField::ApiBase?config.apiBase:field==ConfigField::NodeId?config.nodeId:config.bearerToken;
  if (current == candidate && prefs.getBool("active", false)) { prefs.end(); return true; }
  const bool ok = prefs.putString(key, candidate) > 0 && prefs.putBool("active", true); prefs.end();
  if (ok) { if(field==ConfigField::WifiSsid)config.wifiSsid=candidate;else if(field==ConfigField::WifiPassword)config.wifiPassword=candidate;else if(field==ConfigField::ApiBase)config.apiBase=candidate;else if(field==ConfigField::NodeId)config.nodeId=candidate;else config.bearerToken=candidate; }
  return ok;
}

bool ConfigStore::clearOverrides(){Preferences p;if(!p.begin("rig-config",false))return false;const bool ok=p.clear();p.end();return ok;}

}  // namespace rig
