#pragma once

#include <WebServer.h>

#include "model/RigTypes.h"
#include "ui/UiInvalidation.h"

namespace rig {

enum class LocalRequest : uint8_t { None, RetryWifi, RetryApi, UpdateConfig };
enum class LocalConfigField : uint8_t {
  None,
  WifiSsid,
  WifiPassword,
  ApiBase,
  NodeId,
  BearerToken
};
struct LocalConfigUpdate {
  LocalConfigField field = LocalConfigField::None;
  char value[96] = {0};
};

class LocalDeviceService {
 public:
  bool begin(const char *hostname, const RigConfig &config,
             const RigSnapshot &snapshot, const RenderCounters &renderCounters);
  void stop();
  void handleClient();
  LocalRequest takeRequest();
  bool takeConfigUpdate(LocalConfigUpdate &update);
  bool running() const { return running_; }
  bool mdnsRunning() const { return mdnsRunning_; }

 private:
  void registerRoutes();
  bool authorized();
  void sendStatus();
  void sendConfig();
  void sendJsonError(int status, const char *message);

  WebServer server_{80};
  const RigConfig *config_ = nullptr;
  const RigSnapshot *snapshot_ = nullptr;
  const RenderCounters *renderCounters_ = nullptr;
  char hostname_[64] = {0};
  LocalRequest pendingRequest_ = LocalRequest::None;
  LocalConfigUpdate pendingConfig_{};
  bool routesRegistered_ = false;
  bool running_ = false;
  bool mdnsRunning_ = false;
};

}  // namespace rig
