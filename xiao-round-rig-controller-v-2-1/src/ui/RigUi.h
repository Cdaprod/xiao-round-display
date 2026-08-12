#pragma once

#include "display/DisplayDevice.h"
#include "model/RigTypes.h"
#include "ui/StatusHalo.h"

namespace rig {

class RigUi {
 public:
  explicit RigUi(DisplayDevice &display) : display_(display), halo_(display) {}

  void begin(const RigSnapshot &snapshot);
  void updateSnapshot(const RigSnapshot &snapshot);
  void tick(uint32_t nowUs);
  void cyclePage();
  void showStatus();

 private:
  void drawContent();
  void drawStatusPage(Arduino_GFX &gfx);
  void drawNetworkPage(Arduino_GFX &gfx);
  void drawDevicePage(Arduino_GFX &gfx);
  void drawHelpPage(Arduino_GFX &gfx);
  void centerText(
      Arduino_GFX &gfx,
      const char *text,
      int y,
      uint8_t size,
      uint16_t color);
  const char *stateLabel() const;
  uint16_t stateColor() const;

  DisplayDevice &display_;
  StatusHalo halo_;
  RigSnapshot snapshot_{};
  UiPage page_ = UiPage::Status;
  bool dirty_ = true;
};

}  // namespace rig
