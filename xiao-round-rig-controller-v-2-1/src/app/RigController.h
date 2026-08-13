#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "display/DisplayDevice.h"
#include "input/TouchController.h"
#include "model/RigTypes.h"
#include "network/RigApiClient.h"
#include "storage/ConfigStore.h"
#include "ui/RigUi.h"

namespace rig {

class RigController {
 public:
  RigController(
      DisplayDevice &display,
      ConfigStore &configStore,
      TouchController &touch,
      RigApiClient &api,
      RigUi &ui);

  bool begin();
  void tick(uint32_t nowUs);

 private:
  static void apiWorkerThunk(void *context);
  void apiWorkerLoop();
  void startWifi(uint32_t nowMs);
  void updateWifi(uint32_t nowMs);
  void drainApiUpdates(uint32_t nowMs);
  void applyApiUpdate(const ApiUpdate &update, uint32_t nowMs);
  void handleTouch(uint32_t nowMs);
  void updateTelemetry(uint32_t nowMs);
  void requestApi(ControlAction action);
  void handleUiCommand(const UiCommand &command, uint32_t nowMs);
  float batteryVoltage() const;
  int batteryPercent(float volts) const;

  DisplayDevice &display_;
  ConfigStore &configStore_;
  TouchController &touch_;
  RigApiClient &api_;
  RigUi &ui_;

  RigConfig config_;
  StorageBootResult storage_{};
  RigSnapshot snapshot_{};

  QueueHandle_t actionQueue_ = nullptr;
  QueueHandle_t updateQueue_ = nullptr;
  TaskHandle_t apiWorker_ = nullptr;

  bool started_ = false;
  bool wifiAttempting_ = false;
  bool wifiWasConnected_ = false;
  bool recordingObserved_ = false;
  uint32_t wifiAttemptAt_ = 0;
  uint32_t wifiAttemptNumber_ = 0;
  uint32_t lastWifiDiagnosticAt_ = 0;
  int lastWifiStatus_ = -1;
  uint32_t lastWifiRetryAt_ = 0;
  uint32_t lastSnapshotAt_ = 0;
  uint32_t lastBatteryAt_ = 0;
  uint32_t recordingStartedAt_ = 0;
  WifiRetryState wifiRetry_{};
};

}  // namespace rig
