// x86 host-only GPT10 dev.14 vs dev.14a measurements. ESP32 numbers may differ.
#include "wled.h"
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <vector>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"
#include "gpt10_dev14_reference.h"

static double median(std::vector<double>& v){std::sort(v.begin(),v.end());return v[v.size()/2];}
static double sample(void(*fn)(),Segment& s,uint32_t sec){
  strip.now=sec*1000U;strip._currentSegment=&s;
  std::vector<double> measurements;
  for(int rep=0;rep<23;++rep){
    s.call=0;
    auto start=std::chrono::steady_clock::now();fn();auto end=std::chrono::steady_clock::now();
    if(rep>=5)measurements.push_back(std::chrono::duration<double,std::micro>(end-start).count());
  }
  return median(measurements);
}
int main(){
 if(!globalMod)return 1;
 globalMod->setup();
 std::puts("uptime_s,speed,palette,original_us,candidate_us,ratio_old_over_new,hash_evaluations,hash_cache_hits");
 for(uint32_t sec:{16U,137U,1800U,7200U,8543U,43200U,172800U})
  for(uint8_t speed:{120U,235U}){
    Segment orig(64,64),now(64,64);
    for(Segment* s:{&orig,&now}){s->speed=speed;s->intensity=190;s->custom1=128;s->custom2=128;s->palette=6;}
    const double oldus=sample(gptfx::mode_gpt10_dev14_reference,orig,sec);
    const double newus=sample(gptfx::mode_gpt10,now,sec);
    std::printf("%u,%u,6,%.2f,%.2f,%.3f,%u,%u\n",sec,speed,oldus,newus,oldus/newus,
                gptfx::gpt10Diag.hashEvaluations,gptfx::gpt10Diag.hashCacheHits);
  }
 return 0;
}
