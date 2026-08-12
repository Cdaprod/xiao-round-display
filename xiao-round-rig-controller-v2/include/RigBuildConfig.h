#pragma once

#include <Arduino.h>

#ifndef RIG_ENABLE_DMA
#define RIG_ENABLE_DMA 1
#endif

#ifndef RIG_LCD_SPI_HZ
#define RIG_LCD_SPI_HZ 80000000
#endif

#ifndef RIG_HALO_TARGET_FPS
#define RIG_HALO_TARGET_FPS 60
#endif

namespace rig::build {

constexpr int kScreenSize = 240;
constexpr int kScreenCenter = 120;

// Seeed Studio Round Display for XIAO hardware map.
constexpr int kPinLcdCs = D1;
constexpr int kPinLcdDc = D3;
constexpr int kPinLcdBacklight = D6;
constexpr int kPinTouchInterrupt = D7;
constexpr int kPinSdCs = D2;
constexpr int kPinBattery = D0;

constexpr uint8_t kTouchAddress = 0x2E;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kTouchDebounceMs = 80;
constexpr uint32_t kTouchLongPressMs = 700;
constexpr uint32_t kRuntimeSnapshotMs = 250;
constexpr uint32_t kBatterySampleMs = 5000;

}  // namespace rig::build
