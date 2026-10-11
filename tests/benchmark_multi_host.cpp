// Frozen dev.10 references compared against dev.11 implementations.
// Host-only timings; no inference about ESP32-S3 frequency or HUB75 FPS.
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
using Ns=std::chrono::nanoseconds;
static long long median(std::vector<long long>& a) {
  std::sort(a.begin(),a.end()); return a[a.size()/2];
}
int main(){
  globalMod->setup();
  void(*reference[])()={gptfx::mode_gpt7_dev10_reference,gptfx::mode_gpt8_dev10_reference,
                        gptfx::mode_gpt18_dev10_reference,gptfx::mode_gpt20_dev10_reference};
  void(*candidate[])()={gptfx::mode_gpt7,gptfx::mode_gpt8,gptfx::mode_gpt18,gptfx::mode_gpt20};
  const int names[]={7,8,18,20};
  volatile unsigned long long digest=0;
  std::puts("backend,effect,width,height,uptime_s,dev10_us,dev11_us,speedup");
  for(int pixels:{64,128})for(uint32_t uptime:{14U,420U,900U})for(int fx=0;fx<4;++fx){
    Segment old(pixels,pixels),next(pixels,pixels);
    for(Segment* seg:{&old,&next}){
      seg->speed=135;seg->intensity=190;seg->custom1=128;
      seg->custom2=128;seg->palette=6;
    }
    std::vector<long long> original,modified;
    for(int rep=0;rep<50;++rep){
      strip.now=uptime*1000U+(uint32_t)(rep*53);
      // Alternate reference/candidate order to reduce drift bias.
      for(int k=0;k<2;++k){
        bool opt=((rep&1)!=0)==(k==0);
        Segment& seg=opt?next:old;
        strip._currentSegment=&seg;
        seg.call=0;
        auto a=Clock::now();
        if(opt)candidate[fx]();else reference[fx]();
        auto b=Clock::now();
        (opt?modified:original).push_back(std::chrono::duration_cast<Ns>(b-a).count());
        digest+=seg.frameBuffer[((size_t)rep*157U+fx*19U+k) % seg.frameBuffer.size()];
      }
    }
    double baseUs=median(original)/1000.,optUs=median(modified)/1000.;
    std::printf("%s,GPT%02d,%d,%d,%u,%.3f,%.3f,%.4f\n",
      gptfx::extraBackendName(),names[fx],pixels,pixels,uptime,baseUs,optUs,baseUs/optUs);
  }
  std::fprintf(stderr,"checksum=%llu\n",digest);
}
