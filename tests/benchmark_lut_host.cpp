// Development-only host benchmark. Never infer ESP32-S3 performance from it.
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
#include "gpt24_dev9_reference.h"
using Clock=std::chrono::steady_clock;
using ns=std::chrono::nanoseconds;
static long long p50(std::vector<long long>& samples){
  std::sort(samples.begin(),samples.end());
  return samples[samples.size()/2];
}
int main(){
  globalMod->setup();
  Segment original(64,64),experimental(64,64);
  for(Segment* s:{&original,&experimental}){
    s->palette=6;s->speed=135;s->intensity=190;s->custom1=128;s->custom2=128;
  }
  volatile uint64_t digest=0;
  std::printf("backend,uptime_s,dev9_us,selected_us,speedup_ratio\n");
  for(uint32_t seconds:{14U,421U,900U}){
    std::vector<long long> base,mode;
    for(int j=0;j<100;++j){
      strip.now=seconds*1000U+(uint32_t)j*53U;
      for(int k=0;k<2;++k){
        bool opt=((j&1)==0)==(k==1);
        strip._currentSegment=opt?&experimental:&original;
        strip._currentSegment->call=0;
        auto a=Clock::now();
        if(opt)gptfx::mode_gpt24();else gptfx::mode_gpt24_dev9_reference();
        auto b=Clock::now();
        (opt?mode:base).push_back(std::chrono::duration_cast<ns>(b-a).count());
        digest+=strip._currentSegment->frameBuffer[((j*149)+k*7)%(4096*4)];
      }
    }
    double baseUs=p50(base)/1000.0,modeUs=p50(mode)/1000.0;
    std::printf("%s,%u,%.2f,%.2f,%.4f\n",gptfx_trig::backendName(),seconds,baseUs,modeUs,baseUs/modeUs);
  }
  std::fprintf(stderr,"host benchmark digest=%llu\n",(unsigned long long)digest);
}
