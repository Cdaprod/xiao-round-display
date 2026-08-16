#include <cassert>
#include <cstring>

#include "../src/network/NetworkPolicy.h"

using namespace rig;

int main() {
  char hostname[64] = {0};
  buildDeviceHostname("Studio Rig #2", hostname, sizeof(hostname));
  assert(std::strcmp(hostname, "cda-rig-studio-rig-2") == 0);
  buildDeviceHostname("---", hostname, sizeof(hostname));
  assert(std::strcmp(hostname, "cda-rig-controller") == 0);
  buildDeviceHostname("", hostname, sizeof(hostname));
  assert(std::strcmp(hostname, "cda-rig-controller") == 0);

  assert(mutatingRequestAuthorized("Bearer device-secret", "device-secret"));
  assert(!mutatingRequestAuthorized("device-secret", "device-secret"));
  assert(!mutatingRequestAuthorized("Bearer wrong", "device-secret"));
  assert(!mutatingRequestAuthorized("Bearer device-secret", ""));

  assert(connectivityState(false, true, false, 0) ==
         ConnectivityState::ApiOffline);
  assert(connectivityState(false, true, true, 0) ==
         ConnectivityState::ApiReady);
  assert(connectivityState(true, false, false, 500) ==
         ConnectivityState::WifiAssociating);
  assert(connectivityState(true, false, false, 2000) ==
         ConnectivityState::WifiAuthenticating);
  assert(connectivityState(true, false, false, 5000) ==
         ConnectivityState::WifiDhcp);
  assert(!mayStartWifiAttempt(true));
  assert(mayStartWifiAttempt(false));

  const char *password = "not-for-output";
  const char *token = "also-not-for-output";
  const char *passwordState = redactedSecretState(password[0] != '\0');
  const char *tokenState = redactedSecretState(token[0] != '\0');
  assert(std::strstr(passwordState, password) == nullptr);
  assert(std::strstr(tokenState, token) == nullptr);

  uint8_t selectedCategory = 2;
  (void)connectivityState(false, true, false, 0);
  assert(selectedCategory == 2);
  return 0;
}
