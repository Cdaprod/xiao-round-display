#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/config/ConfigPolicy.h"
#include "../src/network/WifiStateMachine.h"
using namespace rig;
int main(){PlainConfig base,sd,nvs;strcpy(sd.ssid,"lab");strcpy(sd.password," secret ");strcpy(nvs.api,"https://rig.local");mergeConfig(base,sd,3);mergeConfig(base,nvs,4);assert(!strcmp(base.password," secret "));assert(validApiUrl(base.api));assert(!validApiUrl("ftp://bad"));auto st=configStatus(base,true,true,true,true);assert(st.wifi&&st.api&&st.overrides&&!st.node);assert(!strcmp(redacted(true),"SET"));uint32_t fp=credentialFingerprint("lab"," secret ");assert(fp==credentialFingerprint("lab"," secret ")&&fp!=credentialFingerprint("lab","secret"));char diagnostic[96];snprintf(diagnostic,sizeof(diagnostic),"source=NVS ssid=lab passlen=%u fp=%08x",8u,fp);assert(!diagnosticContainsSecret(diagnostic," secret "));PlainConfig cleared{};assert(!cleared.ssid[0]);
WifiRetryState w;w.connecting(0);w.fail(WifiStage::ConnectionTimeout,201,15000);assert(w.stage()==WifiStage::ConnectionTimeout);assert(!w.update(16000));assert(w.countdown(16000)>0);w.update(21000);assert(w.stage()==WifiStage::RetryCountdown);w.retryNow();assert(w.stage()==WifiStage::Connecting);w.connected();assert(w.countdown(30000)==0);assert(!strcmp(disconnectReasonText(202),"AUTH FAILED"));std::cout<<"configuration/wifi tests passed\n";}
