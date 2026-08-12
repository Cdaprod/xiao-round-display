#include "storage/ConfigStore.h"

#include <SD.h>
#include <SPI.h>

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
    SPI.end();
    return result;
  }

  if (!SD.exists("/rig.cfg")) {
    result.templateCreated = writeTemplate();
  }

  result.configLoaded = loadConfigFile(config);
  if (result.configLoaded) {
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
  return config.wifiConfigured();
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

}  // namespace rig
