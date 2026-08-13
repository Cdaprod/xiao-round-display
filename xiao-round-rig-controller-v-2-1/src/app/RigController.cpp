#include "app/RigController.h"

#include <WiFi.h>

#include "RigBuildConfig.h"

namespace rig {

RigController::RigController(
    DisplayDevice &display,
    ConfigStore &configStore,
    TouchController &touch,
    RigApiClient &api,
    RigUi &ui)
    : display_(display),
      configStore_(configStore),
      touch_(touch),
      api_(api),
      ui_(ui) {}

bool RigController::begin() {
  analogReadResolution(12);
  touch_.begin();

  storage_ = configStore_.loadAtBoot(config_);
  if (!display_.begin()) {
    Serial.println("Display initialization failed");
    return false;
  }

  snapshot_ = {};
  snapshot_.state = RigState::Booting;
  snapshot_.sdReady = storage_.sdReady;
  snapshot_.configLoaded = storage_.configLoaded;
  snapshot_.configFound = storage_.configFound;
  snapshot_.configParsed = storage_.configParsed;
  snapshot_.nvsOverrides = storage_.nvsOverrides;
  snapshot_.wifiConfigured = config_.wifiConfigured();
  snapshot_.nodeConfigured = config_.nodeId.length() > 0;
  snapshot_.apiConfigured = config_.apiBase.startsWith("http://") || config_.apiBase.startsWith("https://");
  snapshot_.tokenConfigured = config_.tokenConfigured();
  copyText(snapshot_.detail, storage_.message);
  copyText(snapshot_.nodeId, config_.nodeId);
  copyText(snapshot_.apiBase, config_.apiBase);
  copyText(snapshot_.wifiSsid, config_.wifiSsid);
  ui_.begin(snapshot_);

  actionQueue_ = xQueueCreate(4, sizeof(ControlAction));
  updateQueue_ = xQueueCreate(1, sizeof(ApiUpdate));
  if (!actionQueue_ || !updateQueue_) {
    snapshot_.state = RigState::Error;
    copyText(snapshot_.detail, "QUEUE INIT FAILED");
    ui_.updateSnapshot(snapshot_);
    return false;
  }

  // The ESP32-C3 is single-core. The worker is a separate FreeRTOS task so
  // blocking HTTP calls yield cleanly, but it deliberately is not core-pinned.
  const BaseType_t taskResult = xTaskCreate(
      apiWorkerThunk,
      "rig-api",
      12288,
      this,
      1,
      &apiWorker_);
  if (taskResult != pdPASS) {
    snapshot_.state = RigState::Error;
    copyText(snapshot_.detail, "API TASK FAILED");
    ui_.updateSnapshot(snapshot_);
    return false;
  }

  started_ = true;
  startWifi(millis());
  ui_.updateSnapshot(snapshot_);
  Serial.printf(
      "CDAProd Rig ready: dma=%s spi=%luHz halo=%dfps\n",
      display_.dmaEnabled() ? "on" : "off",
      static_cast<unsigned long>(RIG_LCD_SPI_HZ),
      RIG_HALO_TARGET_FPS);
  return true;
}

void RigController::tick(uint32_t nowUs) {
  if (!started_) return;
  const uint32_t nowMs = millis();
  updateWifi(nowMs);
  drainApiUpdates(nowMs);
  handleTouch(nowMs);
  updateTelemetry(nowMs);
  ui_.updateSnapshot(snapshot_);
  ui_.tick(nowUs);
}

void RigController::apiWorkerThunk(void *context) {
  static_cast<RigController *>(context)->apiWorkerLoop();
}

void RigController::apiWorkerLoop() {
  uint32_t lastPollAt = 0;
  char workerSessionId[48] = {0};

  for (;;) {
    ControlAction action = ControlAction::Poll;
    const bool hasAction = xQueueReceive(
        actionQueue_,
        &action,
        pdMS_TO_TICKS(25)) == pdTRUE;

    if (hasAction && WiFi.status() == WL_CONNECTED) {
      ApiUpdate update{};
      if (action == ControlAction::Poll) {
        update = api_.poll(config_);
      } else {
        update = api_.sendControl(config_, workerSessionId, action);
        xQueueOverwrite(updateQueue_, &update);
        if (update.requestOk) {
          vTaskDelay(pdMS_TO_TICKS(150));
          update = api_.poll(config_);
        }
      }

      if (update.sessionId[0]) copyText(workerSessionId, update.sessionId);
      if (!update.sessionPresent && update.requestOk) workerSessionId[0] = '\0';
      xQueueOverwrite(updateQueue_, &update);
      lastPollAt = millis();
    }

    const uint32_t nowMs = millis();
    if (WiFi.status() == WL_CONNECTED &&
        nowMs - lastPollAt >= config_.pollMs) {
      ApiUpdate update = api_.poll(config_);
      if (update.sessionId[0]) copyText(workerSessionId, update.sessionId);
      if (!update.sessionPresent && update.requestOk) workerSessionId[0] = '\0';
      xQueueOverwrite(updateQueue_, &update);
      lastPollAt = nowMs;
    }
  }
}

void RigController::startWifi(uint32_t nowMs) {
  if (wifiAttempting_) return;
  if (!config_.wifiConfigured()) {
    snapshot_.state = RigState::WifiOffline;
    copyText(snapshot_.detail, "EDIT /rig.cfg");
    wifiAttempting_ = false;
    snapshot_.wifiStage = WifiStage::Unconfigured;
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (config_.wifiPassword.length()) {
    WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str());
  } else {
    WiFi.begin(config_.wifiSsid.c_str());
  }

  wifiAttempting_ = true;
  ++wifiAttemptNumber_;
  wifiRetry_.connecting(nowMs);
  snapshot_.wifiStage = WifiStage::Connecting;
  wifiAttemptAt_ = nowMs;
  lastWifiRetryAt_ = nowMs;
  snapshot_.state = RigState::WifiConnecting;
  copyText(snapshot_.detail, String("JOINING ") + config_.wifiSsid);
  Serial.printf("wifi: attempt=%lu status=%d stage=connecting elapsed=0ms ssid=%s\n", static_cast<unsigned long>(wifiAttemptNumber_), static_cast<int>(WiFi.status()), config_.wifiSsid.c_str());
}

void RigController::updateWifi(uint32_t nowMs) {
  const int scanResult = WiFi.scanComplete();
  if (scanResult == WIFI_SCAN_RUNNING) { snapshot_.wifiStage = WifiStage::Scanning; return; }
  if (scanResult >= 0) {
    snapshot_.scanCount = static_cast<uint8_t>(min(scanResult, 5));
    for (uint8_t i=0;i<snapshot_.scanCount;i++) { copyText(snapshot_.scanSsid[i], WiFi.SSID(i).length()?WiFi.SSID(i):String("<HIDDEN>")); snapshot_.scanRssi[i]=WiFi.RSSI(i); snapshot_.scanSecure[i]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN; }
    WiFi.scanDelete();
  }
  const int wifiStatus = static_cast<int>(WiFi.status());
  if (wifiStatus != lastWifiStatus_ || nowMs - lastWifiDiagnosticAt_ >= 2000) {
    lastWifiStatus_ = wifiStatus;
    lastWifiDiagnosticAt_ = nowMs;
    Serial.printf("wifi: attempt=%lu status=%d stage=%s elapsed=%lums reason=%d\n", static_cast<unsigned long>(wifiAttemptNumber_), wifiStatus, wifiStageLabel(wifiRetry_.stage()), static_cast<unsigned long>(wifiAttempting_ ? nowMs - wifiAttemptAt_ : 0), wifiRetry_.reason());
  }
  const bool connected = wifiStatus == WL_CONNECTED;
  snapshot_.wifiConnected = connected;

  if (connected) {
    wifiAttempting_ = false;
    wifiRetry_.connected();
    snapshot_.wifiStage = WifiStage::Connected;
    snapshot_.wifiRssi = WiFi.RSSI();
    copyText(snapshot_.ipAddress, WiFi.localIP().toString());
    copyText(snapshot_.gateway, WiFi.gatewayIP().toString());
    copyText(snapshot_.dns, WiFi.dnsIP().toString());

    if (!wifiWasConnected_) {
      wifiWasConnected_ = true;
      snapshot_.state = RigState::NoSession;
      copyText(snapshot_.detail, "CONTACTING API");
      Serial.printf("wifi: association=complete dhcp=complete ip=%s elapsed=%lums\n", snapshot_.ipAddress, static_cast<unsigned long>(nowMs - wifiAttemptAt_));
      requestApi(ControlAction::Poll);
    }
    return;
  }

  snapshot_.wifiRssi = -127;
  snapshot_.ipAddress[0] = '\0';

  if (wifiWasConnected_) {
    wifiWasConnected_ = false;
    snapshot_.sessionPresent = false;
    snapshot_.state = RigState::WifiOffline;
    copyText(snapshot_.detail, "WIFI LOST");
    wifiRetry_.fail(WifiStage::Disconnected, 0, nowMs);
  }

  if (wifiAttempting_ &&
      nowMs - wifiAttemptAt_ >= build::kWifiConnectTimeoutMs) {
    wifiAttempting_ = false;
    snapshot_.state = RigState::WifiOffline;
    copyText(snapshot_.detail, "CHECK SSID/PASS");
    wifiRetry_.fail(WifiStage::ConnectionTimeout, 0, nowMs);
    snapshot_.wifiStage = WifiStage::ConnectionTimeout;
    Serial.println("wifi: connection timed out");
  }

  if (wifiAttempting_ && WiFi.status() == WL_NO_SSID_AVAIL) {
    wifiAttempting_ = false; wifiRetry_.fail(WifiStage::NoAccessPoint, 201, nowMs);
    copyText(snapshot_.wifiReasonText, "AP NOT FOUND"); snapshot_.wifiReason = 201;
  } else if (wifiAttempting_ && WiFi.status() == WL_CONNECT_FAILED) {
    wifiAttempting_ = false; wifiRetry_.fail(WifiStage::AuthenticationFailure, 202, nowMs);
    copyText(snapshot_.wifiReasonText, "AUTH FAILED"); snapshot_.wifiReason = 202;
  }

  const bool retryDue = wifiRetry_.update(nowMs);
  snapshot_.wifiStage = wifiRetry_.stage();
  snapshot_.retrySeconds = wifiRetry_.countdown(nowMs);
  if (!wifiAttempting_ && config_.wifiConfigured() && retryDue) {
    WiFi.disconnect();
    startWifi(nowMs);
  }
}

void RigController::drainApiUpdates(uint32_t nowMs) {
  ApiUpdate update{};
  while (xQueueReceive(updateQueue_, &update, 0) == pdTRUE) {
    if (WiFi.status() == WL_CONNECTED) applyApiUpdate(update, nowMs);
  }
}

void RigController::applyApiUpdate(const ApiUpdate &update, uint32_t nowMs) {
  snapshot_.state = update.state;
  snapshot_.sessionPresent = update.sessionPresent;
  snapshot_.chunkCount = update.chunkCount;
  copyText(snapshot_.sessionId, update.sessionId);
  copyText(snapshot_.sessionStatus, update.sessionStatus);
  copyText(snapshot_.desiredAction, update.desiredAction);
  snapshot_.apiReachable = update.requestOk;
  snapshot_.requestInProgress = update.state == RigState::Sending;

  if (update.error[0]) {
    copyText(snapshot_.detail, update.error);
    copyText(snapshot_.lastError, update.error);
  } else {
    switch (update.state) {
      case RigState::NoSession: copyText(snapshot_.detail, "NO SESSION - TAP INFO"); break;
      case RigState::Previewing: copyText(snapshot_.detail, "TAP TO RECORD"); break;
      case RigState::Recording: copyText(snapshot_.detail, "RECORDING"); break;
      case RigState::Sending: copyText(snapshot_.detail, "AWAITING CAMERA"); break;
      default: break;
    }
  }

  const bool recordingNow = update.state == RigState::Recording;
  if (recordingNow && !recordingObserved_) recordingStartedAt_ = nowMs;
  if (!recordingNow) recordingStartedAt_ = 0;
  recordingObserved_ = recordingNow;
}

void RigController::handleTouch(uint32_t nowMs) {
  const TouchEvent event = touch_.poll(nowMs);
  snapshot_.touchHealthy = touch_.healthy();
  if (event.kind != TouchKind::None) ui_.handleTouch(event);
  UiCommand command{};
  if (ui_.takeCommand(command)) handleUiCommand(command, nowMs);
}

void RigController::handleUiCommand(const UiCommand &command, uint32_t nowMs) {
  switch (command.action) {
    case UiAction::RetryWifi: WiFi.disconnect(); wifiRetry_.retryNow(); startWifi(nowMs); break;
    case UiAction::ScanWifi: snapshot_.wifiStage = WifiStage::Scanning; WiFi.scanNetworks(true); break;
    case UiAction::SelectWifi:
      if (snapshot_.scanCount) { configStore_.saveOverride(ConfigField::WifiSsid,snapshot_.scanSsid[0],config_);copyText(snapshot_.wifiSsid,config_.wifiSsid);copyText(snapshot_.detail,"SSID SELECTED - EDIT PASSWORD"); }
      else copyText(snapshot_.lastError,"SCAN NETWORKS FIRST");
      break;
    case UiAction::DisconnectWifi: WiFi.disconnect(); wifiRetry_.fail(WifiStage::Disconnected, 0, nowMs); break;
    case UiAction::TestApi:
    case UiAction::PollApi: requestApi(ControlAction::Poll); break;
    case UiAction::StartRecording: snapshot_.state=RigState::Sending; requestApi(ControlAction::StartRecording); break;
    case UiAction::StopRecording: snapshot_.state=RigState::Sending; requestApi(ControlAction::StopRecording); break;
    case UiAction::ClearError: snapshot_.lastError[0]='\0'; break;
    case UiAction::SaveConfig:
      if (configStore_.saveOverride(command.field, command.value, config_)) {
        storage_.nvsOverrides=true; snapshot_.nvsOverrides=true;
        snapshot_.wifiConfigured=config_.wifiConfigured(); snapshot_.tokenConfigured=config_.tokenConfigured();
        snapshot_.nodeConfigured=config_.nodeId.length()>0; copyText(snapshot_.wifiSsid,config_.wifiSsid);
        copyText(snapshot_.nodeId,config_.nodeId); copyText(snapshot_.apiBase,config_.apiBase);
      } else copyText(snapshot_.lastError,"INVALID VALUE");
      break;
    case UiAction::ForgetWifi: configStore_.saveOverride(ConfigField::WifiSsid,"",config_);configStore_.saveOverride(ConfigField::WifiPassword,"",config_);WiFi.disconnect();break;
    case UiAction::ClearOverrides: configStore_.clearOverrides(); ESP.restart(); break;
    case UiAction::ReloadSd: configStore_.clearOverrides(); ESP.restart(); break;
    case UiAction::Reboot: ESP.restart(); break;
    case UiAction::DisplayTest: copyText(snapshot_.detail,"DISPLAY TEST - TAP"); break;
    case UiAction::TouchTest: copyText(snapshot_.detail,"TOUCH TEST - HOLD EXIT"); break;
    case UiAction::None: break;
  }
}

void RigController::updateTelemetry(uint32_t nowMs) {
  if (nowMs - lastSnapshotAt_ >= build::kRuntimeSnapshotMs) {
    lastSnapshotAt_ = nowMs;
    if (snapshot_.wifiConnected) snapshot_.wifiRssi = WiFi.RSSI();
    if (recordingStartedAt_) {
      snapshot_.recordingSeconds = (nowMs - recordingStartedAt_) / 1000;
    } else {
      snapshot_.recordingSeconds = 0;
    }
  }

  if (lastBatteryAt_ == 0 ||
      nowMs - lastBatteryAt_ >= build::kBatterySampleMs) {
    lastBatteryAt_ = nowMs;
    snapshot_.batteryPercent = batteryPercent(batteryVoltage());
    snapshot_.batteryMv = static_cast<uint16_t>(batteryVoltage() * 1000.0f);
  }
  snapshot_.uptimeSeconds = nowMs / 1000;
  snapshot_.freeHeap = ESP.getFreeHeap();
  snapshot_.haloFrames = ui_.haloFrames();
  snapshot_.haloDropped = ui_.haloDropped();
}

void RigController::requestApi(ControlAction action) {
  if (!actionQueue_) return;
  if (xQueueSend(actionQueue_, &action, 0) != pdTRUE) {
    Serial.println("api: action queue full");
  }
}

float RigController::batteryVoltage() const {
  return (analogReadMilliVolts(build::kPinBattery) * 2.0f) / 1000.0f;
}

int RigController::batteryPercent(float volts) const {
  if (volts < 2.5f || volts > 4.5f) return -1;
  const float normalized = (volts - 3.20f) / (4.20f - 3.20f);
  return constrain(static_cast<int>(normalized * 100.0f), 0, 100);
}

}  // namespace rig
