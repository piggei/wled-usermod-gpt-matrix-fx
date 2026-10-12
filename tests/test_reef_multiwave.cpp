#include "wled.h"
#include <cstdio>
#include <cassert>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"

int main(){
  Segment seg(64,64);
  strip._currentSegment=&seg;
  uint8_t fft[16]; for(unsigned i=0;i<16;++i)fft[i]=185;
  float volume=150.0f;uint8_t peak=0;
  void* data[4]={&volume,nullptr,fft,&peak};
  UsermodManager::audio.u_size=4;UsermodManager::audio.u_data=data;
  UsermodManager::audioOn=true;
  auto frame=[&](uint32_t ms,uint8_t p){strip.now=ms;peak=p;gptfx::mode_audio5();++seg.call;};
  frame(150,0);
  frame(200,1);
  auto* state=reinterpret_cast<gptfx::ReefWaveState*>(seg.data);
  assert(state->count==1 && state->launches[0]==200);
  frame(260,0);
  frame(470,1);
  assert(state->count==2 && state->launches[0]==200 && state->launches[1]==470);
  frame(510,0);
  frame(730,1);
  assert(state->count==3 && state->launches[2]==730);
  frame(1800,0);
  assert(state->count==3 && state->launches[0]==200 && state->launches[1]==470);
  for(unsigned i=0;i<12;++i){frame(2010+i*160,0);frame(2090+i*160,1);}
  assert(state->count==6 && state->next<6);
  printf("REEF_MULTI_WAVE_PASS latest=%u slots=%u stateBytes=%zu\n",state->lastRecorded,state->count,sizeof(*state));
}
