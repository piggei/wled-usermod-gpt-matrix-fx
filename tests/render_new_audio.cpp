#include "wled.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=true;
um_data_t UsermodManager::audio;
static float vol=160.0f;
static uint8_t fft[16];
static uint8_t peak=0;
static void* audioPointers[4]={&vol,nullptr,fft,&peak};
#include "../gpt_matrix_fx.cpp"
int main(int argc,char** argv){
  if(argc!=3)return 2;
  int index=atoi(argv[1]);if(index<35||index>39)return 3;
  FILE* f=fopen(argv[2],"wb");if(!f)return 4;
  UsermodManager::audio.u_size=4;UsermodManager::audio.u_data=audioPointers;
  globalMod->setup();
  Segment seg(64,64);seg.palette=11;seg.speed=126;seg.intensity=195;
  seg.custom1=130;seg.custom2=130;
  strip._currentSegment=&seg;
  for(int frame=0;frame<360;++frame){
    float t=frame/15.0f;
    for(int k=0;k<16;++k){
      float e=std::sin(t*(.72f+.037f*k)+k*.56f),b=std::sin(t*2.4f-k*.19f);
      float signal=85+75*e+28*b;
      if(k<4)signal+=42*std::sin(t*1.3f);
      fft[k]=(uint8_t)std::max(0.f,std::min(255.f,signal));
    }
    vol=155+35*std::sin(t*1.3f);peak=((frame%17)==0)?1:0;
    strip.now=10000U+(uint32_t)(frame*66.6666667f);
    strip.effects[index].fn();++seg.call;
    for(size_t i=0;i<64U*64U;++i)fwrite(&seg.frameBuffer[i*4],1,3,f);
  }
  fclose(f);return 0;
}
