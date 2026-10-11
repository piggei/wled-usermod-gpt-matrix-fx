// GPT18 dev.12 phase stability and visual regression against frozen dev.10
// (which matches dev.11's default GPT18 renderer pixel for pixel).
#include "wled.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"
#include "gpt_multi_dev10_reference.h"

static unsigned checkFrames(int w, int h, unsigned& maxDelta, unsigned& maxChanged,
                            unsigned& maxHigh, double& maxMean) {
  const uint32_t seconds[]={0,14,207,421,754,1048,1806,3600,7200};
  const uint8_t speeds[]={0,60,125,255};
  const uint8_t palettes[]={0,6,8};
  unsigned cases=0;
  for(uint32_t sec:seconds) for(uint8_t speed:speeds) for(uint8_t palette:palettes) {
    Segment oldSeg(w,h),newSeg(w,h);
    for(Segment* seg:{&oldSeg,&newSeg}){
      seg->speed=speed;seg->intensity=190;seg->custom1=128;
      seg->custom2=128;seg->palette=palette;seg->call=0;
    }
    strip.now=sec*1000U;
    strip._currentSegment=&oldSeg;
    gptfx::mode_gpt18_dev10_reference();
    strip._currentSegment=&newSeg;
    gptfx::mode_gpt18();
    unsigned delta=0,changed=0,high=0;
    uint64_t sum=0;
    for(int i=0;i<w*h;++i) {
      bool different=false,large=false;
      for(int c=0;c<3;++c) {
        const size_t idx=(size_t)i*4+c;
        unsigned d=(unsigned)std::abs((int)newSeg.frameBuffer[idx]-(int)oldSeg.frameBuffer[idx]);
        delta=std::max(delta,d);sum+=d;
        different|=(d>0);large|=(d>16);
      }
      changed+=(unsigned)different;high+=(unsigned)large;
    }
    const double mean=(double)sum/(3.0*w*h);
    maxDelta=std::max(maxDelta,delta);
    maxChanged=std::max(maxChanged,changed);
    maxHigh=std::max(maxHigh,high);
    maxMean=std::max(maxMean,mean);
    const bool native=(GPTFX_GPT18_PHASE_BOUNDING==0 && GPTFX_GPT18_TRIG_BACKEND==0);
    if((native && (delta || changed || high)) ||
       (!native && (changed>(unsigned)(w*h/25) || high>(unsigned)(w*h/200) || mean>0.12))) {
      std::fprintf(stderr,"LABYRINTH_VISUAL_FAIL %dx%d t=%u speed=%u pal=%u delta=%u changed=%u high=%u mean=%.6f\n",
                   w,h,sec,speed,palette,delta,changed,high,mean);
      return 0;
    }
    if(GPTFX_GPT18_PHASE_BOUNDING==1) {
      if(!std::isfinite(gptfx::extraPerf.phaseRawAbs) ||
         !std::isfinite(gptfx::extraPerf.phaseWrappedAbs) ||
         gptfx::extraPerf.phaseWrappedAbs>gptfx::K_PI+0.00002f) {
        std::fprintf(stderr,"LABYRINTH_PHASE_FAIL t=%u speed=%u raw=%f bounded=%f\n",
              sec,speed,gptfx::extraPerf.phaseRawAbs,gptfx::extraPerf.phaseWrappedAbs);
        return 0;
      }
      if(sec==1806 && speed==125 && gptfx::extraPerf.phaseRawAbs < 120.0f) {
        std::fprintf(stderr,"LABYRINTH_RAW_PHASE_FAIL raw=%f\n",gptfx::extraPerf.phaseRawAbs);
        return 0;
      }
    }
    ++cases;
  }
  return cases;
}
int main(){
  if(!globalMod)return 1;
  globalMod->setup();
  unsigned total=0;
  for(auto size: {std::pair<int,int>{64,64}, {128,64}, {128,128}}){
    unsigned maxDelta=0,maxChanged=0,maxHigh=0;
    double maxMean=0.0;
    const unsigned cases=checkFrames(size.first,size.second,maxDelta,maxChanged,maxHigh,maxMean);
    if(!cases)return 2;
    total+=cases;
    std::printf("LABYRINTH_VISUAL_PASS size=%dx%d cases=%u maxDelta=%u changed=%u high=%u mean=%.5f\n",
      size.first,size.second,cases,maxDelta,maxChanged,maxHigh,maxMean);
  }
  std::map<std::string,std::string> fields;
  JsonObject root{&fields,""};
  globalMod->addToJsonInfo(root);
  if(fields.at("u.gptMatrixFX.build")!="dev.15" ||
      fields.at("u.gptMatrixFX.trigonometry.labyrinthPhaseBounding")!=
        (GPTFX_GPT18_PHASE_BOUNDING ? "1" : "0") ||
      fields.count("u.gptMatrixFX.extraPerformance.phaseRawAbs")==0 ||
      fields.count("u.gptMatrixFX.extraPerformance.phaseWrappedAbs")==0) {
    std::fprintf(stderr,"LABYRINTH_JSON_FAIL\n");return 3;
  }
  std::printf("LABYRINTH_PHASE_TEST_PASS total=%u phaseBounding=%d lut=%d\n",
    total,GPTFX_GPT18_PHASE_BOUNDING,GPTFX_GPT18_TRIG_BACKEND);
  return 0;
}
