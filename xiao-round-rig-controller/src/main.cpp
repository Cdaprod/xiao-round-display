#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>

// Seeed Studio Round Display for XIAO hardware map.
// LCD: GC9A01, 240x240, shared SPI bus.
// Touch: CHSC6X at I2C address 0x2E.
constexpr int PIN_LCD_CS = D1;
constexpr int PIN_LCD_DC = D3;
constexpr int PIN_LCD_BL = D6;
constexpr int PIN_TOUCH_INT = D7;
constexpr int PIN_SD_CS = D2;
constexpr int PIN_BATTERY = D0;

constexpr uint8_t TOUCH_ADDRESS = 0x2E;
constexpr int SCREEN_SIZE = 240;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t UI_REFRESH_MS = 250;
constexpr uint32_t TOUCH_DEBOUNCE_MS = 80;

constexpr uint16_t COLOR_BLACK = 0x0000;
constexpr uint16_t COLOR_WHITE = 0xFFFF;
constexpr uint16_t COLOR_RED = 0xF800;
constexpr uint16_t COLOR_GREEN = 0x07E0;
constexpr uint16_t COLOR_AMBER = 0xFD20;
constexpr uint16_t COLOR_BLUE = 0x04FF;
constexpr uint16_t COLOR_MUTED = 0x7BEF;
constexpr uint16_t COLOR_PANEL = 0x1082;

Arduino_DataBus *displayBus = new Arduino_ESP32SPI(
    PIN_LCD_DC,
    PIN_LCD_CS,
    SCK,
    MOSI,
    MISO,
    FSPI);
Arduino_GFX *display = new Arduino_GC9A01(displayBus, GFX_NOT_DEFINED, 0, true);

struct RigConfig {
  String wifiSsid;
  String wifiPassword;
  String apiBase = "http://192.168.0.25:8787";
  String nodeId;
  String bearerToken;
  uint32_t pollMs = 1000;
};

enum class RigState {
  Booting,
  WifiOffline,
  ApiOffline,
  NoSession,
  Previewing,
  Recording,
  Sending,
  Error,
};

struct LiveSession {
  bool present = false;
  String sessionId;
  String nodeId;
  String status;
  String desiredAction;
  uint32_t chunkCount = 0;
};

RigConfig config;
LiveSession liveSession;
RigState rigState = RigState::Booting;
String lastError;
bool sdReady = false;
uint32_t lastPollAt = 0;
uint32_t lastUiAt = 0;
uint32_t lastTouchAt = 0;
uint32_t lastWifiRetryAt = 0;
uint32_t recordingObservedAt = 0;
bool lastObservedRecording = false;
String lastUiSignature;

String trimCopy(String value) {
  value.trim();
  return value;
}

void centerText(const String &text, int y, uint8_t size, uint16_t color) {
  display->setTextSize(size);
  display->setTextColor(color);
  const int estimatedWidth = static_cast<int>(text.length()) * 6 * size;
  display->setCursor(max(0, (SCREEN_SIZE - estimatedWidth) / 2), y);
  display->print(text);
}

String stateLabel() {
  switch (rigState) {
    case RigState::Booting: return "BOOT";
    case RigState::WifiOffline: return "NO WIFI";
    case RigState::ApiOffline: return "API OFFLINE";
    case RigState::NoSession: return "NO SESSION";
    case RigState::Previewing: return "STANDBY";
    case RigState::Recording: return "REC";
    case RigState::Sending: return "SENDING";
    case RigState::Error: return "ERROR";
  }
  return "UNKNOWN";
}

uint16_t stateColor() {
  switch (rigState) {
    case RigState::Recording: return COLOR_RED;
    case RigState::Previewing: return COLOR_GREEN;
    case RigState::Sending: return COLOR_BLUE;
    case RigState::Booting: return COLOR_BLUE;
    case RigState::NoSession: return COLOR_AMBER;
    case RigState::WifiOffline:
    case RigState::ApiOffline:
    case RigState::Error: return COLOR_AMBER;
  }
  return COLOR_WHITE;
}

float batteryVoltage() {
  // Round Display routes VBAT to D0 through an approximately 1:1 divider.
  // This is a useful indication, not a fuel-gauge-quality measurement.
  return (analogReadMilliVolts(PIN_BATTERY) * 2.0f) / 1000.0f;
}

int batteryPercent(float volts) {
  if (volts < 2.5f || volts > 4.5f) return -1;
  const float normalized = (volts - 3.20f) / (4.20f - 3.20f);
  return constrain(static_cast<int>(normalized * 100.0f), 0, 100);
}

bool wifiConfigured() {
  return config.wifiSsid.length()
      && config.wifiPassword.length()
      && config.wifiPassword != "CHANGE_ME";
}

String uiSignature() {
  String signature = String(static_cast<int>(rigState)) + "|" + lastError
      + "|" + String(sdReady ? 1 : 0)
      + "|" + String(WiFi.status() == WL_CONNECTED ? 1 : 0)
      + "|" + liveSession.sessionId
      + "|" + liveSession.status
      + "|" + liveSession.desiredAction
      + "|" + String(liveSession.chunkCount);

  // Advance the on-screen recording timer once per second without repainting
  // the LCD continuously between visible changes.
  if (rigState == RigState::Recording) {
    const uint32_t elapsed = recordingObservedAt == 0
        ? 0
        : (millis() - recordingObservedAt) / 1000;
    signature += "|" + String(elapsed);
  }
  return signature;
}

void drawUi() {
  display->fillScreen(COLOR_BLACK);
  const uint16_t accent = stateColor();

  display->drawCircle(120, 120, 116, COLOR_PANEL);
  display->drawCircle(120, 120, 115, accent);
  display->drawCircle(120, 120, 91, COLOR_PANEL);

  centerText("CDAPROD RIG", 28, 1, COLOR_MUTED);
  centerText(stateLabel(), 79, rigState == RigState::Recording ? 4 : 3, accent);

  String detail;
  if (rigState == RigState::Recording) {
    const uint32_t elapsed = recordingObservedAt == 0 ? 0 : (millis() - recordingObservedAt) / 1000;
    char timerText[16];
    snprintf(timerText, sizeof(timerText), "%02lu:%02lu", elapsed / 60, elapsed % 60);
    detail = timerText;
  } else if (rigState == RigState::Previewing) {
    detail = "TAP TO RECORD";
  } else if (rigState == RigState::NoSession) {
    detail = config.nodeId.length() ? config.nodeId : "SET NODE_ID";
  } else if (lastError.length()) {
    detail = lastError.substring(0, 24);
  } else {
    detail = "INITIALIZING";
  }
  centerText(detail, 137, rigState == RigState::Recording ? 2 : 1, COLOR_WHITE);

  String network = WiFi.status() == WL_CONNECTED
      ? String("WIFI ") + String(WiFi.RSSI()) + "dBm"
      : "WIFI --";
  centerText(network, 174, 1, WiFi.status() == WL_CONNECTED ? COLOR_GREEN : COLOR_MUTED);

  const int battery = batteryPercent(batteryVoltage());
  String footer = String("SD ") + (sdReady ? "OK" : "--") + "  BAT ";
  footer += battery >= 0 ? String(battery) + "%" : "--";
  centerText(footer, 193, 1, COLOR_MUTED);

  if (liveSession.present) {
    centerText(String("CHUNKS ") + liveSession.chunkCount, 210, 1, COLOR_MUTED);
  }
}

void redrawUiIfChanged(bool force = false) {
  const String signature = uiSignature();
  if (!force && signature == lastUiSignature) return;
  lastUiSignature = signature;
  drawUi();
}

void logEvent(const String &event, const String &detail = "") {
  Serial.printf("[%lu] %s %s\n", millis(), event.c_str(), detail.c_str());
  if (!sdReady) return;

  File log = SD.open("/rig-events.csv", FILE_APPEND);
  if (!log) return;
  log.printf("%lu,%s,%s\n", millis(), event.c_str(), detail.c_str());
  log.close();
}

void writeConfigTemplate() {
  if (!sdReady || SD.exists("/rig.cfg")) return;
  File file = SD.open("/rig.cfg", FILE_WRITE);
  if (!file) return;
  file.println("wifi_ssid=cda_Lab");
  file.println("wifi_password=CHANGE_ME");
  file.println("api_base=http://192.168.0.25:8787");
  file.println("node_id=CHANGE_ME_CAMERA_NODE_ID");
  file.println("bearer_token=CHANGE_ME_DEVICE_TOKEN");
  file.println("poll_ms=1000");
  file.close();
}

void assignConfigValue(const String &key, const String &value) {
  if (key == "wifi_ssid") config.wifiSsid = value;
  else if (key == "wifi_password") config.wifiPassword = value;
  else if (key == "api_base") config.apiBase = value;
  else if (key == "node_id") config.nodeId = value;
  else if (key == "bearer_token") config.bearerToken = value;
  else if (key == "poll_ms") config.pollMs = constrain(value.toInt(), 500L, 10000L);
}

bool loadConfig() {
  if (!sdReady) return false;
  File file = SD.open("/rig.cfg", FILE_READ);
  if (!file) return false;

  while (file.available()) {
    String line = trimCopy(file.readStringUntil('\n'));
    if (!line.length() || line.startsWith("#")) continue;
    const int separator = line.indexOf('=');
    if (separator <= 0) continue;
    assignConfigValue(trimCopy(line.substring(0, separator)), trimCopy(line.substring(separator + 1)));
  }
  file.close();
  if (config.apiBase.endsWith("/")) config.apiBase.remove(config.apiBase.length() - 1);
  return config.wifiSsid.length() && config.wifiPassword != "CHANGE_ME";
}

void initSdCard() {
  pinMode(PIN_LCD_CS, OUTPUT);
  digitalWrite(PIN_LCD_CS, HIGH);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);

  SPI.begin(SCK, MISO, MOSI, PIN_SD_CS);
  sdReady = SD.begin(PIN_SD_CS, SPI, 4000000);
  logEvent(sdReady ? "sd_ready" : "sd_failed");
  writeConfigTemplate();
}

void connectWifi() {
  if (!wifiConfigured()) {
    rigState = RigState::WifiOffline;
    lastError = "EDIT /rig.cfg";
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    lastError = "";
    logEvent("wifi_ready", WiFi.localIP().toString());
  } else {
    rigState = RigState::WifiOffline;
    lastError = "CHECK SSID/PASS";
    logEvent("wifi_failed");
  }
}

void addDeviceHeaders(HTTPClient &http) {
  if (config.bearerToken.length() && config.bearerToken != "CHANGE_ME_DEVICE_TOKEN") {
    http.addHeader("Authorization", String("Bearer ") + config.bearerToken);
  }
  if (config.nodeId.length()) {
    http.addHeader("X-Media-Sync-Node-Id", config.nodeId);
  }
}

bool pollLiveSession() {
  if (WiFi.status() != WL_CONNECTED) {
    rigState = RigState::WifiOffline;
    lastError = "WIFI LOST";
    return false;
  }

  HTTPClient http;
  http.setConnectTimeout(1200);
  http.setTimeout(1800);
  if (!http.begin(config.apiBase + "/api/live_sessions")) {
    rigState = RigState::ApiOffline;
    lastError = "BAD API URL";
    return false;
  }

  const int statusCode = http.GET();
  const String payload = statusCode > 0 ? http.getString() : "";
  http.end();

  if (statusCode != HTTP_CODE_OK) {
    rigState = RigState::ApiOffline;
    lastError = statusCode > 0 ? String("HTTP ") + statusCode : "NO RESPONSE";
    liveSession = {};
    return false;
  }

  JsonDocument document;
  const DeserializationError jsonError = deserializeJson(document, payload);
  if (jsonError || !document.is<JsonArray>()) {
    rigState = RigState::Error;
    lastError = "BAD SESSION JSON";
    liveSession = {};
    return false;
  }

  LiveSession matched;
  for (JsonObject session : document.as<JsonArray>()) {
    const String candidateNode = session["node_id"] | "";
    if (!config.nodeId.length() || candidateNode == config.nodeId) {
      matched.present = true;
      matched.sessionId = String(session["session_id"] | "");
      matched.nodeId = candidateNode;
      matched.status = String(session["status"] | "");
      matched.desiredAction = String(session["desired_action"] | "");
      matched.chunkCount = session["chunk_count"] | 0;
      break;
    }
  }

  liveSession = matched;
  lastError = "";
  if (!matched.present) {
    rigState = RigState::NoSession;
  } else if (matched.desiredAction.length()) {
    rigState = RigState::Sending;
  } else if (matched.status == "recording") {
    rigState = RigState::Recording;
  } else {
    rigState = RigState::Previewing;
  }

  const bool nowRecording = matched.status == "recording";
  if (nowRecording && !lastObservedRecording) recordingObservedAt = millis();
  if (!nowRecording) recordingObservedAt = 0;
  lastObservedRecording = nowRecording;
  return true;
}

bool sendControlAction(const String &action) {
  if (!liveSession.present || !liveSession.sessionId.length()) {
    lastError = "NO LIVE SESSION";
    rigState = RigState::NoSession;
    return false;
  }
  if (!config.bearerToken.length() || config.bearerToken == "CHANGE_ME_DEVICE_TOKEN") {
    lastError = "TOKEN REQUIRED";
    rigState = RigState::Error;
    return false;
  }

  const String url = config.apiBase + "/api/live_sessions/" + liveSession.sessionId + "/control";
  HTTPClient http;
  http.setConnectTimeout(1200);
  http.setTimeout(2500);
  if (!http.begin(url)) {
    lastError = "BAD CONTROL URL";
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  addDeviceHeaders(http);

  JsonDocument body;
  body["action"] = action;
  String serialized;
  serializeJson(body, serialized);

  rigState = RigState::Sending;
  redrawUiIfChanged(true);
  const int statusCode = http.POST(serialized);
  const String response = statusCode > 0 ? http.getString() : "";
  http.end();

  logEvent("control", action + ":" + String(statusCode));
  if (statusCode < 200 || statusCode >= 300) {
    rigState = RigState::Error;
    lastError = statusCode > 0 ? String("CONTROL HTTP ") + statusCode : "CONTROL FAILED";
    if (response.length()) logEvent("control_error", response.substring(0, 160));
    return false;
  }

  // The camera node acknowledges and applies the desired action. Poll quickly
  // so the dial reflects its authoritative status rather than guessing.
  delay(150);
  return pollLiveSession();
}

bool readTouch(uint16_t &x, uint16_t &y) {
  if (digitalRead(PIN_TOUCH_INT) != LOW) return false;

  const uint8_t requested = Wire.requestFrom(TOUCH_ADDRESS, static_cast<uint8_t>(5));
  if (requested != 5) return false;

  uint8_t data[5] = {0};
  for (uint8_t &value : data) value = Wire.read();
  if (data[0] != 0x01) return false;

  x = data[2];
  y = data[4];
  return x < SCREEN_SIZE && y < SCREEN_SIZE;
}

void handleTouch() {
  uint16_t x = 0;
  uint16_t y = 0;
  if (!readTouch(x, y)) return;
  if (millis() - lastTouchAt < TOUCH_DEBOUNCE_MS) return;

  // Only the central 180px circular area is the record control.
  const int dx = static_cast<int>(x) - 120;
  const int dy = static_cast<int>(y) - 120;
  if ((dx * dx) + (dy * dy) > (90 * 90)) return;

  lastTouchAt = millis();
  if (rigState == RigState::Sending || rigState == RigState::Booting) return;
  const String action = rigState == RigState::Recording ? "stop_recording" : "start_recording";
  sendControlAction(action);

  // Wait for release so one press cannot issue both start and stop.
  const uint32_t releaseDeadline = millis() + 1000;
  while (digitalRead(PIN_TOUCH_INT) == LOW && millis() < releaseDeadline) delay(10);
}

void setup() {
  Serial.begin(115200);
  const uint32_t serialWaitStartedAt = millis();
  while (!Serial && millis() - serialWaitStartedAt < 3000) delay(10);
  delay(100);
  Serial.println("\nCDAProd XIAO Round Rig Controller");

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
  analogReadResolution(12);
  Wire.begin();

  if (!display->begin(20000000)) {
    Serial.println("Display initialization failed");
  }
  display->fillScreen(COLOR_BLACK);
  redrawUiIfChanged(true);

  initSdCard();
  const bool configLoaded = loadConfig();
  logEvent(configLoaded ? "config_ready" : "config_missing");
  redrawUiIfChanged(true);

  connectWifi();
  if (WiFi.status() == WL_CONNECTED) pollLiveSession();
  redrawUiIfChanged(true);
}

void loop() {
  handleTouch();

  if (WiFi.status() != WL_CONNECTED) {
    rigState = RigState::WifiOffline;
    if (wifiConfigured() && millis() - lastWifiRetryAt >= 10000) {
      lastWifiRetryAt = millis();
      WiFi.disconnect();
      WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
    }
  } else if (millis() - lastPollAt >= config.pollMs) {
    lastPollAt = millis();
    pollLiveSession();
  }

  if (millis() - lastUiAt >= UI_REFRESH_MS) {
    lastUiAt = millis();
    redrawUiIfChanged();
  }

  delay(5);
}
