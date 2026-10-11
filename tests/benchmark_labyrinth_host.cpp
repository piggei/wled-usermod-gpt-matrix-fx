// Host-only GPT18 reference/dev.12 A/B; x86 libm results do not predict ESP32-S3.
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
#include "gpt_multi_dev10_reference.h"
using Clock=std::chrono::steady_clock;
static double median(std::vector<long long>& values){
  std::sort(values.begin(),values.end()); return values[values.size()/2]/1000.0;
}
int main(){
  globalMod->setup();
  std::puts("mode,resolution,uptime_s,dev11_native_us,dev12_candidate_us,ratio");
  volatile unsigned long long digest=0;
  for(int side:{64,128})for(uint32_t sec:{14U,1048U,1806U,3600U,7200U}) {
    Segment old(side,side),newSeg(side,side);
    for(Segment* seg:{&old,&newSeg}){
      seg->speed=125;seg->intensity=165;seg->palette=6;seg->custom2=128;
    }
    std::vector<long long> baseline,candidate;
    for(int n=0;n<25;++n)for(int k=0;k<2;++k) {
      bool candidateFirst=((n&1)==0);
      bool optimized=(k==0)?candidateFirst:!candidateFirst;
      Segment& seg=optimized?newSeg:old;
      seg.call=0;strip.now=sec*1000U+(uint32_t)(n*41);
      strip._currentSegment=&seg;
      auto t0=Clock::now();
      if(optimized)gptfx::mode_gpt18();else gptfx::mode_gpt18_dev10_reference();
      auto t1=Clock::now();
      (optimized?candidate:baseline).push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count());
      digest+=seg.frameBuffer[((size_t)n*129U+k*71U)%seg.frameBuffer.size()];
    }
    const double a=median(baseline),b=median(candidate);
    std::printf("%s,%dx%d,%u,%.3f,%.3f,%.4f\n",
       gptfx::labyrinthBackendName(),side,side,sec,a,b,a/b);
  }
  std::fprintf(stderr,"checksum=%llu\n",digest);
}
