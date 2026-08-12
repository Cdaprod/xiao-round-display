#pragma once

#include <HTTPClient.h>

#include "model/RigTypes.h"

namespace rig {

class RigApiClient {
 public:
  ApiUpdate poll(const RigConfig &config);
  ApiUpdate sendControl(
      const RigConfig &config,
      const char *sessionId,
      ControlAction action);

 private:
  void addDeviceHeaders(HTTPClient &http, const RigConfig &config);
  ApiUpdate errorUpdate(RigState state, const String &message);
};

}  // namespace rig
