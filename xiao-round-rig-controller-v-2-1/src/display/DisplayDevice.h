#pragma once

#include <Arduino_GFX_Library.h>

namespace rig {

class DisplayDevice {
 public:
  bool begin();
  Arduino_GFX &gfx();
  bool ready() const { return display_ != nullptr; }
  bool dmaEnabled() const;

 private:
  Arduino_DataBus *bus_ = nullptr;
  Arduino_GFX *display_ = nullptr;
};

}  // namespace rig
