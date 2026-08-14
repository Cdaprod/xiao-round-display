#include "network/LocalDeviceService.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <cstring>

#include "network/NetworkPolicy.h"

namespace rig {
namespace {
const char kFirmwareVersion[] = "2.1.0";
const char kIndexPage[] PROGMEM =
    "<!doctype html><meta name=viewport content='width=device-width'>"
    "<title>CDA Rig Controller</title><style>body{background:#080b10;color:#eef;"
    "font:16px monospace;max-width:42rem;margin:3rem auto;padding:1rem}"
    "dt{color:#48d9ff;margin-top:1rem}</style><h1>CDA Rig Controller</h1>"
    "<p>Local XIAO ESP32-C3 controller service.</p>"
    "<dl><dt>Health</dt><dd><a href=/health>/health</a></dd>"
    "<dt>Status</dt><dd><a href=/api/status>/api/status</a></dd>"
    "<dt>Configuration</dt><dd><a href=/api/config>/api/config</a></dd></dl>";
}

bool LocalDeviceService::begin(const char *hostname, const RigConfig &config,
                               const RigSnapshot &snapshot,
                               const RenderCounters &renderCounters) {
  config_ = &config;
  snapshot_ = &snapshot;
  renderCounters_ = &renderCounters;
  copyText(hostname_, hostname);
  if (!routesRegistered_) registerRoutes();
  mdnsRunning_ = MDNS.begin(hostname_);
  if (mdnsRunning_) MDNS.addService("http", "tcp", 80);
  server_.begin();
  running_ = true;
  return true;
}

void LocalDeviceService::stop() {
  if (!running_) return;
  server_.stop();
  if (mdnsRunning_) MDNS.end();
  mdnsRunning_ = false;
  running_ = false;
}

void LocalDeviceService::handleClient() {
  if (running_) server_.handleClient();
}

LocalRequest LocalDeviceService::takeRequest() {
  const LocalRequest request = pendingRequest_;
  pendingRequest_ = LocalRequest::None;
  return request;
}

bool LocalDeviceService::takeConfigUpdate(LocalConfigUpdate &update) {
  if (pendingRequest_ != LocalRequest::UpdateConfig) return false;
  update = pendingConfig_;
  pendingRequest_ = LocalRequest::None;
  pendingConfig_ = LocalConfigUpdate();
  return true;
}

void LocalDeviceService::registerRoutes() {
  const char *headers[] = {"Authorization", "Content-Length"};
  server_.collectHeaders(headers, 2);
  server_.on("/", HTTP_GET, [this]() {
    server_.send_P(200, "text/html; charset=utf-8", kIndexPage);
  });
  server_.on("/health", HTTP_GET, [this]() {
    StaticJsonDocument<128> document;
    document["ok"] = true;
    document["firmware"] = kFirmwareVersion;
    document["uptime_s"] = snapshot_ ? snapshot_->uptimeSeconds : 0;
    String response;
    serializeJson(document, response);
    server_.send(200, "application/json", response);
  });
  server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
  server_.on("/api/config", HTTP_GET, [this]() { sendConfig(); });
  server_.on("/api/config", HTTP_POST, [this]() {
    if (!authorized()) return;
    const String body = server_.arg("plain");
    if (body.length() > 256) return sendJsonError(413, "request too large");
    StaticJsonDocument<320> document;
    if (deserializeJson(document, body)) return sendJsonError(400, "invalid json");
    const char *field = document["field"] | "";
    const char *value = document["value"] | "";
    if (strlen(value) >= sizeof(pendingConfig_.value))
      return sendJsonError(400, "value too long");
    if (strcmp(field, "ssid") == 0) pendingConfig_.field = LocalConfigField::WifiSsid;
    else if (strcmp(field, "password") == 0) pendingConfig_.field = LocalConfigField::WifiPassword;
    else if (strcmp(field, "api_base") == 0) pendingConfig_.field = LocalConfigField::ApiBase;
    else if (strcmp(field, "node_id") == 0) pendingConfig_.field = LocalConfigField::NodeId;
    else if (strcmp(field, "bearer_token") == 0) pendingConfig_.field = LocalConfigField::BearerToken;
    else return sendJsonError(400, "unknown field");
    copyText(pendingConfig_.value, value);
    pendingRequest_ = LocalRequest::UpdateConfig;
    server_.send(202, "application/json", "{\"accepted\":true}");
  });
  server_.on("/api/wifi/retry", HTTP_POST, [this]() {
    if (!authorized()) return;
    pendingRequest_ = LocalRequest::RetryWifi;
    server_.send(202, "application/json", "{\"accepted\":true}");
  });
  server_.on("/api/api/retry", HTTP_POST, [this]() {
    if (!authorized()) return;
    pendingRequest_ = LocalRequest::RetryApi;
    server_.send(202, "application/json", "{\"accepted\":true}");
  });
  server_.onNotFound([this]() { sendJsonError(404, "not found"); });
  routesRegistered_ = true;
}

bool LocalDeviceService::authorized() {
  const String contentLength = server_.header("Content-Length");
  if (contentLength.length() > 8 || contentLength.toInt() > 256) {
    sendJsonError(413, "request too large");
    return false;
  }
  if (!config_ || !config_->tokenConfigured()) {
    sendJsonError(403, "device token not configured");
    return false;
  }
  const String authorization = server_.header("Authorization");
  if (authorization.length() > 160 ||
      !mutatingRequestAuthorized(authorization.c_str(),
                                 config_->bearerToken.c_str())) {
    sendJsonError(401, "unauthorized");
    return false;
  }
  return true;
}

void LocalDeviceService::sendStatus() {
  if (!snapshot_) return sendJsonError(503, "status unavailable");
  StaticJsonDocument<1024> document;
  document["firmware"] = kFirmwareVersion;
  document["node_id"] = snapshot_->nodeId;
  document["hostname"] = hostname_;
  document["ip"] = snapshot_->ipAddress;
  document["subnet"] = snapshot_->subnet;
  document["gateway"] = snapshot_->gateway;
  document["rssi"] = snapshot_->wifiRssi;
  document["wifi_stage"] = wifiStageLabel(snapshot_->wifiStage);
  document["lan_ready"] = snapshot_->lanReady;
  document["api_ready"] = snapshot_->apiReachable;
  document["sd_ready"] = snapshot_->sdReady;
  document["config_parsed"] = snapshot_->configParsed;
  document["battery_percent"] = snapshot_->batteryPercent;
  document["session_present"] = snapshot_->sessionPresent;
  document["category"] = snapshot_->selectedCategory;
  document["ui_mode"] = snapshot_->uiMode;
  document["uptime_s"] = snapshot_->uptimeSeconds;
  if (renderCounters_) {
    document["full_redraws"] = renderCounters_->fullRedraws;
    document["viewport_redraws"] = renderCounters_->viewportRedraws;
    document["dirty_rows"] = renderCounters_->rowRedraws;
    document["bytes_transferred"] = renderCounters_->bytesTransferred;
  }
  String response;
  serializeJson(document, response);
  server_.send(200, "application/json", response);
}

void LocalDeviceService::sendConfig() {
  if (!config_) return sendJsonError(503, "configuration unavailable");
  StaticJsonDocument<384> document;
  document["ssid"] = config_->wifiSsid;
  document["password_configured"] = config_->wifiConfigured();
  document["token_configured"] = config_->tokenConfigured();
  document["api_base"] = config_->apiBase;
  document["node_id"] = config_->nodeId;
  document["generation"] = config_->configGeneration;
  String response;
  serializeJson(document, response);
  server_.send(200, "application/json", response);
}

void LocalDeviceService::sendJsonError(int status, const char *message) {
  StaticJsonDocument<128> document;
  document["error"] = message;
  String response;
  serializeJson(document, response);
  server_.send(status, "application/json", response);
}

}  // namespace rig
