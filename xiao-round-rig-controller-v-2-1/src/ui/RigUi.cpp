#include "ui/RigUi.h"

#include <WiFi.h>

#include "ui/UiTheme.h"

namespace rig {

void RigUi::begin(const RigSnapshot &snapshot) {
  snapshot_ = snapshot;
  Arduino_GFX &gfx = display_.gfx();
  gfx.fillScreen(theme::kBlack);
  gfx.drawCircle(120, 120, 118, theme::kPanel);
  halo_.begin(snapshot.state);
  dirty_ = true;
}

void RigUi::updateSnapshot(const RigSnapshot &snapshot) {
  if (memcmp(&snapshot_, &snapshot, sizeof(snapshot_)) == 0) return;
  const RigState previousState = snapshot_.state;
  snapshot_ = snapshot;
  if (snapshot_.state != previousState) {
    halo_.setState(snapshot_.state);
  }
  if (snapshot_.sessionPresent &&
      (snapshot_.state == RigState::Previewing ||
       snapshot_.state == RigState::Recording ||
       snapshot_.state == RigState::Sending)) {
    page_ = UiPage::Status;
  }
  dirty_ = true;
}

void RigUi::tick(uint32_t nowUs) {
  if (dirty_) drawContent();
  halo_.tick(nowUs);
}

void RigUi::cyclePage() {
  page_ = static_cast<UiPage>((static_cast<uint8_t>(page_) + 1) % 4);
  dirty_ = true;
}

void RigUi::showStatus() {
  if (page_ == UiPage::Status) return;
  page_ = UiPage::Status;
  dirty_ = true;
}

void RigUi::drawContent() {
  Arduino_GFX &gfx = display_.gfx();
  gfx.fillCircle(120, 120, 103, theme::kBlack);
  gfx.drawCircle(120, 120, 102, theme::kPanel);
  gfx.drawCircle(120, 120, 91, theme::kPanel);

  switch (page_) {
    case UiPage::Status: drawStatusPage(gfx); break;
    case UiPage::Network: drawNetworkPage(gfx); break;
    case UiPage::Device: drawDevicePage(gfx); break;
    case UiPage::Help: drawHelpPage(gfx); break;
  }
  dirty_ = false;
}

void RigUi::drawStatusPage(Arduino_GFX &gfx) {
  centerText(gfx, "CDAPROD RIG", 28, 1, theme::kMuted);
  centerText(
      gfx,
      stateLabel(),
      snapshot_.state == RigState::Recording ? 72 : 79,
      snapshot_.state == RigState::Recording ? 4 : 3,
      stateColor());

  char detail[56] = {0};
  if (snapshot_.state == RigState::Recording) {
    snprintf(
        detail,
        sizeof(detail),
        "%02lu:%02lu",
        static_cast<unsigned long>(snapshot_.recordingSeconds / 60),
        static_cast<unsigned long>(snapshot_.recordingSeconds % 60));
  } else {
    snprintf(detail, sizeof(detail), "%.45s", snapshot_.detail);
  }
  centerText(
      gfx,
      detail,
      137,
      snapshot_.state == RigState::Recording ? 2 : 1,
      theme::kWhite);

  char network[32] = {0};
  if (snapshot_.wifiConnected) {
    snprintf(network, sizeof(network), "WIFI %ddBm", snapshot_.wifiRssi);
  } else {
    snprintf(network, sizeof(network), "WIFI --");
  }
  centerText(
      gfx,
      network,
      174,
      1,
      snapshot_.wifiConnected ? theme::kGreen : theme::kMuted);

  char footer[40] = {0};
  if (snapshot_.batteryPercent >= 0) {
    snprintf(
        footer,
        sizeof(footer),
        "SD %s  BAT %d%%",
        snapshot_.sdReady ? "OK" : "--",
        snapshot_.batteryPercent);
  } else {
    snprintf(
        footer,
        sizeof(footer),
        "SD %s  BAT --",
        snapshot_.sdReady ? "OK" : "--");
  }
  centerText(gfx, footer, 193, 1, theme::kMuted);

  if (snapshot_.sessionPresent) {
    char chunks[28] = {0};
    snprintf(
        chunks,
        sizeof(chunks),
        "CHUNKS %lu",
        static_cast<unsigned long>(snapshot_.chunkCount));
    centerText(gfx, chunks, 210, 1, theme::kMuted);
  } else {
    centerText(gfx, "TAP FOR INFO", 210, 1, theme::kMuted);
  }
}

void RigUi::drawNetworkPage(Arduino_GFX &gfx) {
  centerText(gfx, "NETWORK", 31, 1, theme::kMuted);
  centerText(
      gfx,
      snapshot_.wifiConnected ? "ONLINE" : "OFFLINE",
      72,
      3,
      snapshot_.wifiConnected ? theme::kGreen : theme::kAmber);

  char ip[32] = {0};
  snprintf(ip, sizeof(ip), "IP %.19s", snapshot_.ipAddress[0] ? snapshot_.ipAddress : "--");
  centerText(gfx, ip, 125, 1, theme::kWhite);

  char rssi[32] = {0};
  snprintf(rssi, sizeof(rssi), "RSSI %ddBm", snapshot_.wifiRssi);
  centerText(gfx, rssi, 145, 1, theme::kMuted);

  char api[44] = {0};
  snprintf(api, sizeof(api), "API %.32s", snapshot_.apiBase);
  centerText(gfx, api, 171, 1, theme::kMuted);
  centerText(gfx, "TAP: NEXT", 202, 1, theme::kMuted);
}

void RigUi::drawDevicePage(Arduino_GFX &gfx) {
  centerText(gfx, "DEVICE", 31, 1, theme::kMuted);
  centerText(gfx, "XIAO C3", 70, 3, theme::kCyan);

  char node[44] = {0};
  snprintf(
      node,
      sizeof(node),
      "NODE %.30s",
      snapshot_.nodeId[0] ? snapshot_.nodeId : "NOT SET");
  centerText(gfx, node, 126, 1, theme::kWhite);

  centerText(
      gfx,
      snapshot_.sdReady ? "SD CONFIG: OK" : "SD CONFIG: --",
      151,
      1,
      snapshot_.sdReady ? theme::kGreen : theme::kAmber);
  centerText(
      gfx,
      snapshot_.tokenConfigured ? "DEVICE TOKEN: OK" : "DEVICE TOKEN: --",
      171,
      1,
      snapshot_.tokenConfigured ? theme::kGreen : theme::kAmber);
  centerText(gfx, "TAP: NEXT", 202, 1, theme::kMuted);
}

void RigUi::drawHelpPage(Arduino_GFX &gfx) {
  centerText(gfx, "CONTROLS", 31, 1, theme::kMuted);
  centerText(gfx, "TOUCH", 69, 3, theme::kViolet);
  centerText(gfx, "NO SESSION:", 125, 1, theme::kMuted);
  centerText(gfx, "TAP CYCLES INFO", 143, 1, theme::kWhite);
  centerText(gfx, "LIVE SESSION:", 165, 1, theme::kMuted);
  centerText(gfx, "TAP STARTS / STOPS", 183, 1, theme::kWhite);
  centerText(gfx, "LONG: STATUS", 207, 1, theme::kMuted);
}

void RigUi::centerText(
    Arduino_GFX &gfx,
    const char *text,
    int y,
    uint8_t size,
    uint16_t color) {
  gfx.setTextSize(size);
  gfx.setTextColor(color);
  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  gfx.getTextBounds(text, 0, y, &x1, &y1, &width, &height);
  gfx.setCursor(max(0, (240 - static_cast<int>(width)) / 2 - x1), y);
  gfx.print(text);
}

const char *RigUi::stateLabel() const {
  switch (snapshot_.state) {
    case RigState::Booting: return "BOOT";
    case RigState::WifiConnecting: return "JOINING";
    case RigState::WifiOffline: return "NO WIFI";
    case RigState::ApiOffline: return "API OFFLINE";
    case RigState::NoSession: return "RIG READY";
    case RigState::Previewing: return "STANDBY";
    case RigState::Recording: return "REC";
    case RigState::Sending: return "SENDING";
    case RigState::Error: return "ERROR";
  }
  return "UNKNOWN";
}

uint16_t RigUi::stateColor() const {
  switch (snapshot_.state) {
    case RigState::Recording: return theme::kRed;
    case RigState::NoSession:
    case RigState::Previewing: return theme::kGreen;
    case RigState::Sending:
    case RigState::Booting: return theme::kBlue;
    case RigState::WifiConnecting:
    case RigState::WifiOffline:
    case RigState::ApiOffline: return theme::kAmber;
    case RigState::Error: return theme::kRed;
  }
  return theme::kWhite;
}

}  // namespace rig
