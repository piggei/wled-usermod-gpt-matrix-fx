// Comparative x86 measurements only. Device libm has different large-angle path.
#include "wled.h"
#include <chrono>
#include <cstdio>
#include <algorithm>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"
#include "gpt_stage2_dev13_reference.h"
struct Effect {const char* name;void(*original)();void(*bounded)();};
static const Effect fx[]={
 {"GPT01",gptfx::mode_gpt1_dev13_reference,gptfx::mode_gpt1_profiled},
 {"GPT02",gptfx::mode_gpt2_dev13_reference,gptfx::mode_gpt2_profiled},
 {"GPT09",gptfx::mode_gpt9_dev13_reference,gptfx::mode_gpt9_profiled},
 {"GPT10",gptfx::mode_gpt10_dev13_reference,gptfx::mode_gpt10_profiled},
 {"GPT14",gptfx::mode_gpt14_dev13_reference,gptfx::mode_gpt14_profiled},
 {"GPT16",gptfx::mode_gpt16_dev13_reference,gptfx::mode_gpt16_profiled}
};
static double median(std::vector<double>& vals){
 std::sort(vals.begin(),vals.end());return vals[vals.size()/2];
}
static double run(void(*fn)(),Segment& segment,uint32_t ms) {
 strip.now=ms;strip._currentSegment=&segment;
 std::vector<double> samples;samples.reserve(16);
 for(int i=0;i<20;++i){
  segment.call=0; // Each iteration draws a full independent frame.
  auto a=std::chrono::steady_clock::now();fn();auto b=std::chrono::steady_clock::now();
  if(i>=4)samples.push_back(std::chrono::duration<double,std::micro>(b-a).count());
 }
 return median(samples);
}
int main(){
 if(!globalMod)return 1;
 globalMod->setup();
 std::puts("effect,uptime_seconds,speed,original_us,bounded_us,ratio_original_over_bounded");
 for(const Effect& e:fx)
  for(uint32_t sec:{20U,600U,1800U,7200U})
   for(uint8_t speed:{135U,235U}){
    Segment a(64,64),b(64,64);
    for(Segment* seg:{&a,&b}){seg->speed=speed;seg->intensity=190;seg->custom1=128;seg->custom2=128;seg->palette=6;}
    double orig=run(e.original,a,sec*1000U),newTime=run(e.bounded,b,sec*1000U);
    std::printf("%s,%u,%u,%.3f,%.3f,%.4f\n",e.name,sec,speed,orig,newTime,orig/newTime);
   }
 return 0;
}
