// Host-only, interleaved wall-clock comparison at simulated WLED uptimes.
// Different math libraries/CPU scheduling: NEVER extrapolate these times
// to ESP32-S3. The firmware-side wall-clock telemetry is authoritative.
#include "wled.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"
#include "gpt24_dev8_reference.h"
using Clock = std::chrono::steady_clock;
int main() {
  globalMod->setup();
  volatile uint64_t checksum=0;
  Segment reference(64,64),bounded(64,64);
  for(Segment* s:{&reference,&bounded}){
    s->palette=6;s->speed=135;s->intensity=190;s->custom1=128;s->custom2=128;
  }
  std::printf("uptime_s,dev8_host_median_us,dev9_host_median_us,dev8_p90_us,dev9_p90_us\n");
  for(uint32_t secs:{14U,150U,300U,400U,700U,900U,1800U}) {
    std::vector<long long> oldNs,newNs;
    for(int warm=0;warm<4;++warm) {
      strip.now=secs*1000U+(uint32_t)warm*51U;
      strip._currentSegment=&reference;reference.call=0;gptfx::mode_gpt24_dev8_reference();
      strip._currentSegment=&bounded;bounded.call=0;gptfx::mode_gpt24();
    }
    for(int j=0;j<90;++j) {
      strip.now=secs*1000U+(uint32_t)j*51U;
      for(int k=0;k<2;++k) {
        const bool opt = ((j&1)==0) == (k==1);
        strip._currentSegment=opt?&bounded:&reference;
        strip._currentSegment->call=0;
        const auto start=Clock::now();
        if(opt)gptfx::mode_gpt24();
        else gptfx::mode_gpt24_dev8_reference();
        const auto stop=Clock::now();
        (opt?newNs:oldNs).push_back(
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count());
        checksum+=strip._currentSegment->frameBuffer[(size_t)(j*97U+11U)%(64U*64U*4U)];
      }
    }
    std::sort(oldNs.begin(),oldNs.end());
    std::sort(newNs.begin(),newNs.end());
    std::printf("%u,%.2f,%.2f,%.2f,%.2f\n",secs,
      oldNs[45]/1000.,newNs[45]/1000.,oldNs[81]/1000.,newNs[81]/1000.);
  }
  std::fprintf(stderr,"benchmark checksum=%llu\n",(unsigned long long)checksum);
}
