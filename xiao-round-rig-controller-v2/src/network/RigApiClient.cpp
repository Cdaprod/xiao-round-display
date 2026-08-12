#include "network/RigApiClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace rig {

ApiUpdate RigApiClient::poll(const RigConfig &config) {
  if (WiFi.status() != WL_CONNECTED) {
    return errorUpdate(RigState::WifiOffline, "WIFI LOST");
  }

  HTTPClient http;
  http.setConnectTimeout(1200);
  http.setTimeout(1800);
  if (!http.begin(config.apiBase + "/api/live_sessions")) {
    return errorUpdate(RigState::ApiOffline, "BAD API URL");
  }
  addDeviceHeaders(http, config);

  const int statusCode = http.GET();
  const String payload = statusCode > 0 ? http.getString() : "";
  http.end();

  if (statusCode != HTTP_CODE_OK) {
    return errorUpdate(
        RigState::ApiOffline,
        statusCode > 0 ? String("HTTP ") + statusCode : "NO RESPONSE");
  }

  JsonDocument document;
  const DeserializationError jsonError = deserializeJson(document, payload);
  if (jsonError) {
    return errorUpdate(RigState::Error, "BAD SESSION JSON");
  }

  JsonArrayConst sessions;
  if (document.is<JsonArray>()) {
    sessions = document.as<JsonArrayConst>();
  } else if (document["items"].is<JsonArray>()) {
    sessions = document["items"].as<JsonArrayConst>();
  } else {
    return errorUpdate(RigState::Error, "BAD SESSION JSON");
  }

  ApiUpdate update{};
  update.requestOk = true;
  update.state = RigState::NoSession;

  for (JsonObjectConst session : sessions) {
    const char *candidateNode = session["node_id"] | "";
    if (config.nodeId.length() && config.nodeId != candidateNode) continue;

    update.sessionPresent = true;
    copyText(update.sessionId, session["session_id"] | "");
    copyText(update.sessionStatus, session["status"] | "");
    copyText(update.desiredAction, session["desired_action"] | "");
    update.chunkCount = session["chunk_count"] | 0;

    if (update.desiredAction[0] != '\0') {
      update.state = RigState::Sending;
    } else if (strcmp(update.sessionStatus, "recording") == 0) {
      update.state = RigState::Recording;
    } else {
      update.state = RigState::Previewing;
    }
    break;
  }

  return update;
}

ApiUpdate RigApiClient::sendControl(
    const RigConfig &config,
    const char *sessionId,
    ControlAction action) {
  if (!sessionId || !sessionId[0]) {
    return errorUpdate(RigState::NoSession, "NO LIVE SESSION");
  }
  if (!config.tokenConfigured()) {
    return errorUpdate(RigState::Error, "TOKEN REQUIRED");
  }
  if (WiFi.status() != WL_CONNECTED) {
    return errorUpdate(RigState::WifiOffline, "WIFI LOST");
  }

  const char *actionName = action == ControlAction::StopRecording
      ? "stop_recording"
      : "start_recording";
  const String url = config.apiBase + "/api/live_sessions/" + sessionId + "/control";

  HTTPClient http;
  http.setConnectTimeout(1200);
  http.setTimeout(2500);
  if (!http.begin(url)) {
    return errorUpdate(RigState::Error, "BAD CONTROL URL");
  }
  http.addHeader("Content-Type", "application/json");
  addDeviceHeaders(http, config);

  JsonDocument body;
  body["action"] = actionName;
  String serialized;
  serializeJson(body, serialized);

  const int statusCode = http.POST(serialized);
  const String response = statusCode > 0 ? http.getString() : "";
  http.end();

  if (statusCode < 200 || statusCode >= 300) {
    ApiUpdate update = errorUpdate(
        RigState::Error,
        statusCode > 0 ? String("CONTROL HTTP ") + statusCode : "CONTROL FAILED");
    if (response.length()) {
      Serial.printf("control response: %.160s\n", response.c_str());
    }
    return update;
  }

  ApiUpdate update{};
  update.requestOk = true;
  update.sessionPresent = true;
  update.state = RigState::Sending;
  copyText(update.sessionId, sessionId);
  return update;
}

void RigApiClient::addDeviceHeaders(HTTPClient &http, const RigConfig &config) {
  if (config.tokenConfigured()) {
    http.addHeader("Authorization", String("Bearer ") + config.bearerToken);
  }
  if (config.nodeId.length()) {
    http.addHeader("X-Media-Sync-Node-Id", config.nodeId);
  }
}

ApiUpdate RigApiClient::errorUpdate(RigState state, const String &message) {
  ApiUpdate update{};
  update.state = state;
  copyText(update.error, message);
  return update;
}

}  // namespace rig
