// Host-only uptime regression profiler. No hardware timing claims.
#include "wled.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
static float vol=160.0f;
static uint8_t fft[16]={160,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10};
static uint8_t peak=0;
static void* ptrs[4]={&vol,nullptr,fft,&peak};
#include "../gpt_matrix_fx.cpp"
static volatile uint64_t consume=0;
int main() {
  globalMod->setup();
  UsermodManager::audioOn=true;
  UsermodManager::audio.u_size=4;
  UsermodManager::audio.u_data=ptrs;
  std::puts("effect_id,effect,seconds,speed,samples,median_us,p90_us");
  const unsigned seconds[]={20U,600U,1800U,7200U};
  const uint8_t speeds[]={135U,235U};
  for(unsigned second:seconds)for(uint8_t speed:speeds)for(size_t i=0;i<strip.effects.size();++i){
    Segment seg(64,64);
    seg.palette=6;seg.speed=speed;seg.intensity=190;seg.custom1=128;seg.custom2=128;
    strip._currentSegment=&seg;
    strip.now=second*1000U;
    std::vector<long long> samples;
    for(int j=0;j<24;++j){
      seg.call=(uint32_t)j;
      strip.now+=57U;
      auto t0=std::chrono::steady_clock::now();
      strip.effects[i].fn();
      auto t1=std::chrono::steady_clock::now();
      if(j>=6) samples.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count());
      consume+=seg.frameBuffer[(j*133U+17U)%seg.frameBuffer.size()];
    }
    std::sort(samples.begin(),samples.end());
    char name[80]={}; const char* n=strip.effects[i].name;
    size_t len=0;while(n[len]!='@' && n[len] && len<sizeof(name)-1)len++;
    std::memcpy(name,n,len);
    std::printf("%zu,%s,%u,%u,%zu,%.3f,%.3f\n",i+1,name,second,(unsigned)speed,samples.size(),samples[samples.size()/2]/1000.,samples[(samples.size()*9)/10]/1000.);
  }
  std::fprintf(stderr,"audit checksum=%llu\n",(unsigned long long)consume);
}
