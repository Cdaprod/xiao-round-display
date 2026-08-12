#include "display/DisplayDevice.h"

#include "RigBuildConfig.h"

namespace rig {

bool DisplayDevice::begin() {
  pinMode(build::kPinLcdCs, OUTPUT);
  digitalWrite(build::kPinLcdCs, HIGH);
  pinMode(build::kPinLcdBacklight, OUTPUT);
  digitalWrite(build::kPinLcdBacklight, LOW);

#if RIG_ENABLE_DMA
  // The SD card has already been unmounted before this is called, so the
  // display can exclusively own the ESP32-C3 FSPI host for fast transfers.
  bus_ = new Arduino_ESP32SPIDMA(
      build::kPinLcdDc,
      build::kPinLcdCs,
      SCK,
      MOSI,
      MISO,
      FSPI,
      false);
#else
  bus_ = new Arduino_ESP32SPI(
      build::kPinLcdDc,
      build::kPinLcdCs,
      SCK,
      MOSI,
      MISO,
      FSPI);
#endif

  display_ = new Arduino_GC9A01(bus_, GFX_NOT_DEFINED, 0, true);
  if (!display_->begin(RIG_LCD_SPI_HZ)) {
    display_ = nullptr;
    return false;
  }

  display_->setTextWrap(false);
  display_->fillScreen(0x0000);
  digitalWrite(build::kPinLcdBacklight, HIGH);
  return true;
}

Arduino_GFX &DisplayDevice::gfx() {
  return *display_;
}

bool DisplayDevice::dmaEnabled() const {
#if RIG_ENABLE_DMA
  return true;
#else
  return false;
#endif
}

}  // namespace rig
