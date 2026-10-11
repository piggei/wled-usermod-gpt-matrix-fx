// Realistic connected-AudioReactive and missing-AudioReactive GPTA05 regression.
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
#include "gpt_stage1_dev12_reference.h"

int main(){
  if(!globalMod)return 1;
  globalMod->setup();
  float volume=130.0f;
  uint8_t fft[16]={210,195,160,122,90,70,45,30,15,40,60,85,115,144,178,225};
  uint8_t peak=1;
  void* pointers[4]={&volume,nullptr,fft,&peak};
  UsermodManager::audio.u_size=4;
  UsermodManager::audio.u_data=pointers;
  const uint32_t times[]={0,20,600,1806,7200};
  unsigned cases=0,worst=0,worstChanged=0,worstHigh=0;
  double worstMean=0;
  for(bool connected:{false,true})
   for(int w:{64,128})
    for(uint32_t sec:times)
     for(uint8_t speed:{0U,135U,255U})
      for(uint8_t palette:{0U,6U,8U}) {
       UsermodManager::audioOn=connected;
       Segment oldSeg(w,w),newSeg(w,w);
       for(Segment* seg:{&oldSeg,&newSeg}){
         seg->speed=speed;seg->intensity=190;seg->custom1=128;
         seg->custom2=128;seg->palette=palette;seg->call=0;
       }
       strip.now=sec*1000U;
       strip._currentSegment=&oldSeg;
       gptfx::mode_audio5_dev12_reference();
       strip._currentSegment=&newSeg;
       gptfx::mode_audio5_profiled();
       unsigned maxDelta=0,changed=0,high=0;uint64_t sum=0;
       for(int pixel=0;pixel<w*w;++pixel){
         bool diff=false,large=false;
         for(int color=0;color<3;++color){
           const int idx=pixel*4+color;
           const unsigned d=std::abs((int)oldSeg.frameBuffer[idx]-(int)newSeg.frameBuffer[idx]);
           maxDelta=std::max(maxDelta,d);diff|=d>0;large|=d>16;sum+=d;
         }
         changed+=diff;high+=large;
       }
       const double mean=(double)sum/(3.0*w*w);
       worst=std::max(worst,maxDelta);worstChanged=std::max(worstChanged,changed);
       worstHigh=std::max(worstHigh,high);worstMean=std::max(worstMean,mean);
       if((GPTFX_STAGE1_PHASE_BOUNDING==0 && changed!=0) ||
          (GPTFX_STAGE1_PHASE_BOUNDING!=0 && (mean>0.25 || high>8 || gptfx::extraPerf.phaseWrappedAbs > gptfx::K_PI+0.0001f))){
         std::fprintf(stderr,"AUDIO_STAGE1_FAIL connected=%d size=%d sec=%u speed=%u palette=%u max=%u changed=%u high=%u mean=%.6f\n",
                connected,w,sec,speed,palette,maxDelta,changed,high,mean);
         return 2;
       }
       ++cases;
      }
  std::printf("AUDIO_STAGE1_PASS total=%u enabled=%d worstDelta=%u worstChanged=%u worstHigh=%u worstMean=%.6f\n",
        cases,GPTFX_STAGE1_PHASE_BOUNDING,worst,worstChanged,worstHigh,worstMean);
  return 0;
}
