#include <Arduino.h>

#include "app/RigController.h"
#include "display/DisplayDevice.h"
#include "input/TouchController.h"
#include "network/RigApiClient.h"
#include "storage/ConfigStore.h"
#include "ui/RigUi.h"

namespace {

rig::DisplayDevice display;
rig::ConfigStore configStore;
rig::TouchController touch;
rig::RigApiClient api;
rig::RigUi ui(display);
rig::RigController controller(display, configStore, touch, api, ui);

}  // namespace

void setup() {
  Serial.begin(115200);
  const uint32_t serialStartedAt = millis();
  while (!Serial && millis() - serialStartedAt < 2000) delay(10);
  Serial.println("\nCDAProd XIAO Round Rig Controller");
  controller.begin();
}

void loop() {
  controller.tick(micros());
  delay(1);
}
