#pragma once
#include <cstdint>
namespace rig {
class LatestFrameScheduler {public:explicit LatestFrameScheduler(uint32_t intervalUs):intervalUs_(intervalUs){}bool tick(uint32_t nowUs){if(!started_){started_=true;lastUs_=nowUs;accumulatorUs_=intervalUs_;}else{accumulatorUs_+=nowUs-lastUs_;lastUs_=nowUs;}if(accumulatorUs_<intervalUs_)return false;const uint32_t due=accumulatorUs_/intervalUs_;if(due>1)coalesced_+=due-1;accumulatorUs_=0;return true;}uint32_t coalesced()const{return coalesced_;}private:uint32_t intervalUs_,lastUs_=0,accumulatorUs_=0,coalesced_=0;bool started_=false;};
}
