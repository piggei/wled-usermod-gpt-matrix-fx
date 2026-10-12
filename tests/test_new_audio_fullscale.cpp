#include "wled.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=true;
um_data_t UsermodManager::audio;
static float volume=160.0f;
static uint8_t fft[16]={};
static uint8_t peak=0;
static void* pointers[4]={&volume,nullptr,fft,&peak};
#include "../gpt_matrix_fx.cpp"
static void fill(int scenario) {
  const int lev[]={0,26,128,204,255};
  for(int k=0;k<16;k++) {
    int v= scenario<5?lev[scenario]:scenario==5?(k<4?255:0):scenario==6?(k>=4&&k<11?255:0):scenario==7?(k>=11?255:0):0;
    fft[k]=(uint8_t)v;
  }
}
int main(int argc,char** argv) {
  if(argc!=3)return 2;
  FILE* out=fopen(argv[1],"wb"); if(!out)return 3;
  FILE* csv=fopen(argv[2],"w"); if(!csv)return 4;
  fprintf(csv,"effect,scenario,brightness_sum,bright_pixels,upper_bright,lower_bright,top_row_max,render_us\n");
  UsermodManager::audio.u_size=4;UsermodManager::audio.u_data=pointers;
  globalMod->setup();
  uint64_t quiet[5]={};
  for(int effect=35;effect<=39;++effect){
    for(int scenario=0;scenario<9;++scenario){
      Segment seg(64,64);seg.palette=11;seg.speed=126;seg.intensity=195;seg.custom1=130;seg.custom2=130;
      strip._currentSegment=&seg;
      fill(scenario);volume=255.0f;peak=0;
      for(int frame=0;frame<45;++frame){
        strip.now=10000U+(unsigned)(frame*67);
        if(scenario==8){ for(int k=0;k<16;k++)fft[k]=frame<15?0:frame<30?255:0; }
        strip.effects[effect].fn();++seg.call;
      }
      uint64_t brightness=0;
      unsigned lit=0,up=0,down=0;unsigned top=0;
      for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){
        const unsigned pos=(y*64+x)*4;
        unsigned r=seg.frameBuffer[pos],g=seg.frameBuffer[pos+1],b=seg.frameBuffer[pos+2];
        unsigned v=std::max(r,std::max(g,b));brightness+=r+g+b;
        if(v>=56){++lit;if(y<32)++up;else ++down;}
        if(y==0)top=std::max(top,v);
        fwrite(&seg.frameBuffer[pos],1,3,out);
      }
      if(scenario==0)quiet[effect-35]=brightness;
      if(scenario==4 && brightness < quiet[effect-35]*3ULL){
        fprintf(stderr,"A%02d maximum does not expand dynamically\n",effect-29);return 9;
      }
      fprintf(csv,"A%02d,%d,%llu,%u,%u,%u,%u,%u\n",effect-29,scenario,(unsigned long long)brightness,lit,up,down,top,gptfx::extraPerf.lastUs);
      if(effect==35 && scenario==4 && top<45){fprintf(stderr,"A06 full-scale does not reach the top: %u\n",top);return 8;}
    }
  }
  fclose(out);fclose(csv);puts("FULLSCALE_FFT_5_EFFECTS_9_SCENARIOS_PASS");return 0;
}
