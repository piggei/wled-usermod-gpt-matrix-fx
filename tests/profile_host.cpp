#include "wled.h"
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdint>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
static float vol=160.0f;
static uint8_t fft[16]={160,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10};
static uint8_t peak=0;
static void* ptrs[4]={&vol, nullptr, fft, &peak};
#include "../gpt_matrix_fx.cpp"
#include "gpt24_dev7_reference.h"

int main(){
  globalMod->setup();
  UsermodManager::audioOn=true;
  UsermodManager::audio.u_size=4;
  UsermodManager::audio.u_data=ptrs;
  std::puts("effect,name,host_median_us,host_p90_us");
  volatile uint64_t checksum=0;
  for(size_t i=0;i<strip.effects.size();++i){
    Segment seg(64,64);
    seg.palette=6;seg.speed=135;seg.intensity=190;
    seg.custom1=128;seg.custom2=128;
    strip._currentSegment=&seg;strip.now=2220000U;
    for(int warm=0;warm<8;warm++){
      seg.call=warm;strip.now+=53;
      strip.effects[i].fn();
    }
    std::vector<long long> us;
    for(int j=0;j<72;j++){
      seg.call++;
      strip.now+=53;
      auto t0=std::chrono::steady_clock::now();
      strip.effects[i].fn();
      auto t1=std::chrono::steady_clock::now();
      us.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count());
      checksum += seg.frameBuffer[(j*100U+17U)%seg.frameBuffer.size()];
    }
    std::sort(us.begin(),us.end());
    char name[80]={};
    const char* n=strip.effects[i].name;
    size_t len=0;while(n[len]!='@' && n[len] && len<sizeof(name)-1)len++;
    std::memcpy(name,n,len);
    std::printf("%zu,%s,%.2f,%.2f\n",i+1,name,us[us.size()/2]/1000.0,us[(us.size()*9)/10]/1000.0);
  }
  // A/B timing interleaved with the old source, matching phase and palette.
  // This is an x86 host diagnostic only, not an ESP32-S3 timing claim.
  Segment refSeg(64,64), optSeg(64,64);
  for(Segment* seg:{&refSeg,&optSeg}) {
    seg->palette=6;seg->speed=135;seg->intensity=190;
    seg->custom1=128;seg->custom2=128;
  }
  std::vector<long long> refNs,optNs;
  for(int j=0;j<120;++j){
    const bool swap=(j&1)!=0;
    for(int pass=0;pass<2;++pass){
      bool optimized=((pass==1)^swap);
      strip._currentSegment=optimized?&optSeg:&refSeg;
      strip.now=700000U+(uint32_t)j*51U;
      strip._currentSegment->call=0;
      auto t0=std::chrono::steady_clock::now();
      if(optimized)gptfx::mode_gpt24();
      else gptfx::mode_gpt24_dev7_reference();
      auto t1=std::chrono::steady_clock::now();
      (optimized?optNs:refNs).push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count());
    }
  }
  std::sort(refNs.begin(),refNs.end());std::sort(optNs.begin(),optNs.end());
  std::printf("AB,%s,%.2f,%.2f\n","GPT24b dev.7 reference",refNs[60]/1000.0,refNs[108]/1000.0);
  std::printf("AB,%s,%.2f,%.2f\n","GPT24b dev.8 optimized",optNs[60]/1000.0,optNs[108]/1000.0);
  std::fprintf(stderr,"checksum=%llu\n",(unsigned long long)checksum);
}
