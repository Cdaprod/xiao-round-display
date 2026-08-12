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
  snapshot_.tokenConfigured = config_.tokenConfigured();
  copyText(snapshot_.detail, storage_.message);
  copyText(snapshot_.nodeId, config_.nodeId);
  copyText(snapshot_.apiBase, config_.apiBase);
  ui_.begin(snapshot_);

  actionQueue_ = xQueueCreate(4, sizeof(ControlAction));
  updateQueue_ = xQueueCreate(1, sizeof(ApiUpdate));
  if (!actionQueue_ || !updateQueue_) {
    snapshot_.state = RigState::Error;
    copyText(snapshot_.detail, "QUEUE INIT FAILED");
    ui_.updateSnapshot(snapshot_);
    return false;
  }

  const BaseType_t taskResult = xTaskCreatePinnedToCore(
      apiWorkerThunk,
      "rig-api",
      12288,
      this,
      1,
      &apiWorker_,
      0);
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
  if (!config_.wifiConfigured()) {
    snapshot_.state = RigState::WifiOffline;
    copyText(snapshot_.detail, "EDIT /rig.cfg");
    wifiAttempting_ = false;
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
  wifiAttemptAt_ = nowMs;
  lastWifiRetryAt_ = nowMs;
  snapshot_.state = RigState::WifiConnecting;
  copyText(snapshot_.detail, String("JOINING ") + config_.wifiSsid);
  Serial.printf("wifi: joining %s\n", config_.wifiSsid.c_str());
}

void RigController::updateWifi(uint32_t nowMs) {
  const bool connected = WiFi.status() == WL_CONNECTED;
  snapshot_.wifiConnected = connected;

  if (connected) {
    wifiAttempting_ = false;
    snapshot_.wifiRssi = WiFi.RSSI();
    copyText(snapshot_.ipAddress, WiFi.localIP().toString());

    if (!wifiWasConnected_) {
      wifiWasConnected_ = true;
      snapshot_.state = RigState::NoSession;
      copyText(snapshot_.detail, "CONTACTING API");
      Serial.printf("wifi: ready %s\n", snapshot_.ipAddress);
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
    lastWifiRetryAt_ = nowMs;
  }

  if (wifiAttempting_ &&
      nowMs - wifiAttemptAt_ >= build::kWifiConnectTimeoutMs) {
    wifiAttempting_ = false;
    snapshot_.state = RigState::WifiOffline;
    copyText(snapshot_.detail, "CHECK SSID/PASS");
    Serial.println("wifi: connection timed out");
  }

  if (!wifiAttempting_ && config_.wifiConfigured() &&
      nowMs - lastWifiRetryAt_ >= build::kWifiRetryMs) {
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

  if (update.error[0]) {
    copyText(snapshot_.detail, update.error);
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
  if (event.kind == TouchKind::None) return;

  if (event.kind == TouchKind::LongPress) {
    ui_.showStatus();
    return;
  }

  const bool canControl = snapshot_.sessionPresent &&
      (snapshot_.state == RigState::Previewing ||
       snapshot_.state == RigState::Recording);
  if (!canControl) {
    ui_.cyclePage();
    return;
  }

  if (!config_.tokenConfigured()) {
    snapshot_.state = RigState::Error;
    copyText(snapshot_.detail, "TOKEN REQUIRED");
    return;
  }

  const ControlAction action = snapshot_.state == RigState::Recording
      ? ControlAction::StopRecording
      : ControlAction::StartRecording;
  snapshot_.state = RigState::Sending;
  copyText(
      snapshot_.detail,
      action == ControlAction::StopRecording ? "STOP REQUESTED" : "START REQUESTED");
  requestApi(action);
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
  }
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
