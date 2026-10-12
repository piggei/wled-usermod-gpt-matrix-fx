// Frozen dev.13 image comparison against dev.14 stage-2A temporal phase bounding.
// Host simulated time: pixel fidelity is not an ESP32-S3 performance claim.
#include "wled.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <limits>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"
#include "gpt_stage2_dev13_reference.h"

struct Case { int w,h;uint32_t sec;uint8_t speed,palette,intensity,c1,c2; };
struct Stats { unsigned maximum=0,changed=0,high=0,largestHighCluster=0;double mean=0; };
static Stats compare(const Segment& a,const Segment& b) {
  Stats o;uint64_t sum=0;const int w=a.width,h=a.height;
  std::vector<uint8_t> high((size_t)w*h,0),seen((size_t)w*h,0);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    size_t idx=(size_t)y*w+x;bool change=false,big=false;
    for(int c=0;c<3;++c){
      size_t off=idx*4+c;
      unsigned delta=std::abs((int)a.frameBuffer[off]-(int)b.frameBuffer[off]);
      o.maximum=std::max(o.maximum,delta);sum+=delta;
      change|=delta>0;big|=delta>16;
    }
    o.changed+=change;o.high+=big;high[idx]=big;
  }
  o.mean=(double)sum/(3.0*w*h);
  std::vector<size_t> q;
  for(size_t i=0;i<high.size();++i){
    if(!high[i]||seen[i])continue;
    q.clear();q.push_back(i);seen[i]=1;
    for(size_t j=0;j<q.size();++j){
      size_t v=q[j],x=v%(size_t)w,y=v/(size_t)w;
      const size_t vneigh[4]={v==0?0:v-1,v+1,v<(size_t)w?0:v-w,v+(size_t)w};
      for(int z=0;z<4;++z){
        if((z==0&&x==0)||(z==1&&x+1==(size_t)w)||(z==2&&y==0)||(z==3&&y+1==(size_t)h))continue;
        size_t n=vneigh[z];if(n<high.size()&&high[n]&&!seen[n]){seen[n]=1;q.push_back(n);}
      }
    }
    o.largestHighCluster=std::max(o.largestHighCluster,(unsigned)q.size());
  }
  return o;
}
struct Effect { const char* name;void(*orig)();void(*newFn)(); };
static const Effect cases[]={
  {"GPT01",gptfx::mode_gpt1_dev13_reference,gptfx::mode_gpt1_profiled},
  {"GPT02",gptfx::mode_gpt2_dev13_reference,gptfx::mode_gpt2_profiled},
  {"GPT09",gptfx::mode_gpt9_dev13_reference,gptfx::mode_gpt9_profiled},
  {"GPT10",gptfx::mode_gpt10_dev13_reference,gptfx::mode_gpt10_profiled},
  {"GPT14",gptfx::mode_gpt14_dev13_reference,gptfx::mode_gpt14_profiled},
  {"GPT16",gptfx::mode_gpt16_dev13_reference,gptfx::mode_gpt16_profiled}
};

int main(int argc,char**argv){
  FILE* csv=stdout;
  if(argc>1){csv=std::fopen(argv[1],"w");if(!csv)return 1;}
  if(!globalMod)return 2;
  globalMod->setup();
  std::fprintf(csv,"effect,width,height,uptime_sec,speed,intensity,custom1,custom2,palette,max_channel_delta,changed_pixels,high_delta_pixels,largest_high_cluster,mean_channel_delta,raw_phase_abs,wrapped_phase_abs\n");
  const uint32_t times[]={0,14,207,421,754,1048,1806,2094,3600,7200};
  const uint8_t speeds[]={0,135,255};
  const uint8_t palettes[]={0,6,8};
  // 90 cases per effect on 64x64, including all phase-boundary regions.
  std::vector<Case> setups;
  for(uint32_t sec: times) for(uint8_t speed:speeds) for(uint8_t pal:palettes)
    setups.push_back({64,64,sec,speed,pal,190,128,128});
  // Larger matrices with control and intensity variation.
  for(auto sz:{std::pair<int,int>{128,64},{128,128}})
    for(uint32_t sec:{14U,421U,1806U,7200U})
      for(uint8_t speed:{0U,225U})
        setups.push_back({sz.first,sz.second,sec,speed,6,(uint8_t)(65U+speed/2U),210,44});
  unsigned long long total=0;bool failed=false;
  for(const Effect& fx:cases){
    unsigned worstDelta=0,worstHigh=0,worstCluster=0,worstChanged=0;double worstMean=0;
    for(const Case& c:setups){
      Segment oldSeg(c.w,c.h),newSeg(c.w,c.h);
      for(Segment* seg:{&oldSeg,&newSeg}){
        seg->speed=c.speed;seg->intensity=c.intensity;
        seg->palette=c.palette;seg->custom1=c.c1;seg->custom2=c.c2;
        seg->call=0;
      }
      strip.now=c.sec*1000U;
      strip._currentSegment=&oldSeg;
      fx.orig();
      strip._currentSegment=&newSeg;
      fx.newFn();
      const Stats v=compare(oldSeg,newSeg);
      worstDelta=std::max(worstDelta,v.maximum);
      worstChanged=std::max(worstChanged,v.changed);
      worstHigh=std::max(worstHigh,v.high);
      worstCluster=std::max(worstCluster,v.largestHighCluster);
      worstMean=std::max(worstMean,v.mean);
      std::fprintf(csv,"%s,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.7f,%.6f,%.6f\n",
            fx.name,c.w,c.h,c.sec,c.speed,c.intensity,c.c1,c.c2,c.palette,
            v.maximum,v.changed,v.high,v.largestHighCluster,v.mean,
            gptfx::extraPerf.phaseRawAbs,gptfx::extraPerf.phaseWrappedAbs);
      const bool exact=GPTFX_STAGE2_PHASE_BOUNDING==0;
      if(exact && (v.changed||v.maximum)){
        std::fprintf(stderr,"STAGE2_DISABLED_REGRESSION %s sec=%u speed=%u size=%dx%d max=%u changed=%u\n",
           fx.name,c.sec,c.speed,c.w,c.h,v.maximum,v.changed);
        failed=true;
      }
      if(!exact && (v.mean>0.25 || v.high>(unsigned)(c.w*c.h/100) || v.largestHighCluster>14)){
        std::fprintf(stderr,"STAGE2_VISUAL_ALERT %s sec=%u speed=%u size=%dx%d max=%u changed=%u high=%u cl=%u mean=%.5f\n",
           fx.name,c.sec,c.speed,c.w,c.h,v.maximum,v.changed,v.high,v.largestHighCluster,v.mean);
        failed=true;
      }
      if(!exact && (gptfx::extraPerf.phaseWrappedAbs>gptfx::K_PI+0.0001f ||
          !std::isfinite(gptfx::extraPerf.phaseWrappedAbs))){
        std::fprintf(stderr,"STAGE2_PHASE_BAD %s sec=%u raw=%.4f wrap=%.4f\n",
                fx.name,c.sec,gptfx::extraPerf.phaseRawAbs,gptfx::extraPerf.phaseWrappedAbs);
        failed=true;
      }
      ++total;
    }
    std::fprintf(stderr,"STAGE2_SUMMARY %s cases=%zu max=%u changed=%u high=%u largestCluster=%u maxMean=%.5f\n",
            fx.name,setups.size(),worstDelta,worstChanged,worstHigh,worstCluster,worstMean);
  }
  if(argc>1)std::fclose(csv);
  std::map<std::string,std::string> fields;
  JsonObject root{&fields,""};
  globalMod->addToJsonInfo(root);
  if(fields.at("u.gptMatrixFX.build")!="rc.1" ||
     fields.at("u.gptMatrixFX.trigonometry.stage2PhaseBounding") != (GPTFX_STAGE2_PHASE_BOUNDING ? "1" : "0") ||
     fields.at("u.gptMatrixFX.extraPerformance.effect") != "GPT16" ||
     fields.at("u.gptMatrixFX.trigonometry.stage2Effects") != "GPT01,GPT02,GPT09,GPT10,GPT14,GPT16" ||
     fields.at("u.gptMatrixFX.trigonometry.stage1Effects") != "GPT05,GPT11,GPT13,GPT19,GPT23,GPTA05") {
    std::fprintf(stderr,"STAGE2_JSON_FAIL\n");failed=true;
  }
  std::fprintf(stderr,"STAGE2_PHASE_%s total=%llu enabled=%d\n",failed?"FAIL":"PASS",total,GPTFX_STAGE2_PHASE_BOUNDING);
  return failed?3:0;
}
