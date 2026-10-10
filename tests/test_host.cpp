#include "wled.h"
#include <cstdio>
#include <chrono>
#include <set>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <limits>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
static float vol=130.0f;
static uint8_t fft[16]={210,195,160,122,90,70,45,30,15,40,60,85,115,144,178,225};
static uint8_t peak=1;
static void* audioPointers[4]={&vol, nullptr, fft, &peak};
static void initAudio(){UsermodManager::audio.u_size=4;UsermodManager::audio.u_data=audioPointers;}
#include "../gpt_matrix_fx.cpp"

static unsigned colorCount(const Segment& seg) {
  std::set<uint32_t> colors;
  for(size_t i=0;i<seg.frameBuffer.size();i+=4){
    uint32_t c=((uint32_t)seg.frameBuffer[i]<<16)|((uint32_t)seg.frameBuffer[i+1]<<8)|seg.frameBuffer[i+2];
    colors.insert(c);
  }
  return (unsigned)colors.size();
}
static size_t difference(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
  size_t changed=0;
  assert(a.size()==b.size());
  for(size_t j=0;j<a.size();++j) if(a[j]!=b[j])++changed;
  return changed;
}
int main(){
  if(!globalMod){std::fprintf(stderr,"Usermod not registered\n");return 1;}
  globalMod->setup();
  {
    std::map<std::string,std::string> fields;
    JsonObject info{&fields, ""};
    globalMod->addToJsonInfo(info);
    assert(fields.at("u.gptMatrixFX.version")=="0.3.0");
    assert(fields.at("u.gptMatrixFX.build")=="dev.6");
    assert(fields.at("u.gptMatrixFX.effects")=="30");
    assert(fields.at("u.gptMatrixFX.standard")=="20");
    assert(fields.at("u.gptMatrixFX.audio")=="5");
    assert(fields.at("u.gptMatrixFX.experimental")=="5");
    std::puts("JSON_INFO_TEST_PASS");
  }
  constexpr size_t NORMAL=20,AUDIO=5,EXPERIMENTAL=5,COUNT=NORMAL+AUDIO+EXPERIMENTAL;
  assert(strip.effects.size()==COUNT);
  for(size_t n=0;n<COUNT;n++) {
    const auto& fx=strip.effects[n];
    char want[36];
    if(n<NORMAL) std::snprintf(want,sizeof(want),"GPT%02zu - ",n+1);
    else if(n<NORMAL+AUDIO) std::snprintf(want,sizeof(want),"GPTA%02zu - ",n-NORMAL+1);
    else std::snprintf(want,sizeof(want),"GPT%02zub - ",n-AUDIO+1);
    assert(std::strstr(fx.name,want)==fx.name);
    assert(fx.id==255);
    if(n>=NORMAL && n<NORMAL+AUDIO) assert(std::strstr(fx.name,";2v;")!=nullptr);
    else assert(std::strstr(fx.name,";2;")!=nullptr);
  }
  initAudio();
  const std::pair<int,int> sizes[]={{8,8},{16,32},{64,64},{96,48},{128,128}};
  for(const auto& dim:sizes) {
    const int w=dim.first,h=dim.second;
    for(size_t n=0;n<COUNT;n++) {
      Segment seg(w,h);strip._currentSegment=&seg;
      seg.call=0;strip.now=700000;
      UsermodManager::audioOn=(n>=NORMAL && n<NORMAL+AUDIO);
      const auto start=std::chrono::steady_clock::now();
      strip.effects[n].fn();
      const auto finish=std::chrono::steady_clock::now();
      const auto dur=std::chrono::duration_cast<std::chrono::microseconds>(finish-start).count();
      assert(seg.frameBuffer.size()==(size_t)w*h*4);
      unsigned colors=colorCount(seg);
      if(colors<2){std::fprintf(stderr,"Blank FX %zu at %dx%d colors:%u\n",n+1,w,h,colors);return 2;}
      auto first=seg.frameBuffer;
      for(int i=0;i<6;++i) {++seg.call;strip.now+=66;strip.effects[n].fn();}
      size_t animated=difference(first,seg.frameBuffer);
      // At 8x8 some details alias; only require temporal change at >= 64 width.
      if(w>=64 && animated==0){std::fprintf(stderr,"Frozen FX %zu at %dx%d\n",n+1,w,h);return 4;}
      std::printf("%3dx%-3d %-7s pixels=%-5d colors=%-5u scratch=%-6zu first=%5lld us changed=%zu\n",
                  w,h,n<NORMAL?"NORMAL":(n<NORMAL+AUDIO?"AUDIO":"SPARSE"),w*h,colors,seg.scratch.size(),(long long)dur,animated);
    }
  }
  // Expansion regression: increasing Intensity must expand the actual dot
  // footprint at an identical wave phase; brightness remains independent.
  {
    auto lit=[](const Segment& seg) {
      size_t count=0;
      for(size_t i=0;i<seg.frameBuffer.size();i+=4)
        if(seg.frameBuffer[i] || seg.frameBuffer[i+1] || seg.frameBuffer[i+2]) ++count;
      return count;
    };
    Segment seg(64,64); strip._currentSegment=&seg;
    seg.palette=6; seg.speed=135; seg.intensity=0;
    strip.now=700000; seg.call=0; strip.effects[NORMAL+AUDIO+1].fn();
    const size_t small=lit(seg);
    seg.intensity=255; seg.call=0; strip.effects[NORMAL+AUDIO+1].fn();
    const size_t large=lit(seg);
    if (large<=small) {
      std::fprintf(stderr,"Expansion slider did not enlarge dot footprint: %zu vs %zu\n",small,large);
      return 12;
    }
    std::printf("SPARSE_EXPANSION_TEST_PASS low=%zu high=%zu\n",small,large);
  }
  // Dot Spacing must change lattice geometry in BOTH sparse variants.
  // Rosette Size must change its geometry while preserving default scale.
  for (int index : {25, 26, 28}) {
    Segment seg(64,64); strip._currentSegment=&seg;
    seg.palette=6; seg.speed=135; seg.intensity=205;
    seg.custom1=128; seg.custom2=128;
    strip.now=700000; seg.call=0;
    if(index==28) seg.custom2=0; else seg.custom1=0;
    strip.effects[index].fn(); auto first=seg.frameBuffer;
    seg.call=0;
    if(index==28) seg.custom2=255; else seg.custom1=255;
    strip.effects[index].fn();
    if(difference(first,seg.frameBuffer)==0) {
      std::fprintf(stderr,"Custom slider geometry regression: effect index %d\n",index);
      return 13;
    }
  }
  std::puts("CUSTOM_GEOMETRY_TEST_PASS");
  // Palette-change regression: render, switch several palettes and render
  // again WITHOUT resetting the effect or segment runtime. Primary color is
  // deliberately black, to expose mcol=0's incorrect Default behavior.
  UsermodManager::audioOn=true;
  const uint8_t paletteSequence[]={6,0,7,8,11,2,9,250,6};
  for(size_t n=0;n<COUNT;++n) {
    Segment seg(64,64); strip._currentSegment=&seg;
    seg.primaryColor=0; seg.call=0;
    strip.now=1000000U;
    for(unsigned pass=0; pass<sizeof(paletteSequence); ++pass) {
      seg.palette=paletteSequence[pass];
      strip.now+=100U;
      strip.effects[n].fn();
      const unsigned colors=colorCount(seg);
      if (colors<2) {
        std::fprintf(stderr,"Palette blackout: FX %zu pal=%u (pass %u), colors=%u\n",
                     n+1, seg.palette, pass, colors);
        return 5;
      }
      ++seg.call;
    }
  }
  // A palette containing only some black entries must NOT have its black
  // positions replaced by the fallback rainbow.
  {
    Segment seg(64,64); strip._currentSegment=&seg;
    seg.palette=8; gptfx::ready();
    assert(gptfx::chroma(40.0f, 1.0f)==0U);
    assert(gptfx::chroma(160.0f, 1.0f)!=0U);
    seg.palette=2; seg.primaryColor=0; gptfx::ready();
    assert(gptfx::chroma(40.0f, 1.0f)!=0U);
    seg.palette=0; gptfx::ready();
    assert(gptfx::chroma(40.0f, 1.0f)!=0U);
  }
  std::puts("PALETTE_CHANGE_TEST_PASS 30 FX x 9 palette states + default-black regression + dark-entry preservation");
  // Audio absent: 5 effects stay safe and safely subdued, no fake FFT signal.
  UsermodManager::audioOn=false;
  for(int idx=NORMAL;idx<(int)(NORMAL+AUDIO);++idx){
    Segment seg(64,64);strip._currentSegment=&seg;seg.call=0;
    strip.now=970000;strip.effects[idx].fn();assert(colorCount(seg)>1);
  }
  // Audio metadata inaccessible: must not dereference u_data or stale pointers.
  UsermodManager::audioOn=true;
  UsermodManager::audio.u_size=0;
  for(int idx=NORMAL;idx<(int)(NORMAL+AUDIO);++idx){
    Segment seg(64,64);strip._currentSegment=&seg;seg.call=0;strip.now=980000;
    strip.effects[idx].fn();
  }
  initAudio();
  // Direct proof each audio effect responds to changes in FFT at fixed time.
  for(int idx=NORMAL;idx<(int)(NORMAL+AUDIO);++idx){
    std::memset(fft,0,sizeof(fft));vol=0.0f;peak=0;
    Segment a(64,64);strip._currentSegment=&a;strip.now=990000;a.call=0;
    strip.effects[idx].fn();auto quiet=a.frameBuffer;
    std::memset(fft,255,sizeof(fft));vol=220.0f;peak=1;
    Segment b(64,64);strip._currentSegment=&b;strip.now=990000;b.call=0;
    strip.effects[idx].fn();
    size_t changed=difference(quiet,b.frameBuffer);
    if(changed<400){std::fprintf(stderr,"Audio FX %d insufficient response: %zu bytes changed\n",idx+1,changed);return 3;}
    std::printf("GPTA%02d FFT_TEST_PASS changed_channels=%zu\n",idx-(int)NORMAL+1,changed);
  }
  // Audio not finite: sanitize from external source before math conversion.
  vol=std::numeric_limits<float>::quiet_NaN();
  for(int idx=NORMAL;idx<(int)(NORMAL+AUDIO);++idx){
    Segment seg(32,32);strip._currentSegment=&seg;seg.call=0;strip.now=993000;
    strip.effects[idx].fn();
  }
  // Nonmatrix setup: all modes must fall back without OOB or allocations.
  for(int idx=0;idx<(int)COUNT;++idx){
    Segment seg(64,1);strip._currentSegment=&seg;seg.call=0;
    strip.isMatrix=false;strip.now=997000;strip.effects[idx].fn();
    assert(seg.scratch.empty());
  }
  strip.isMatrix=true;
  // GPT06: moving planetary gears must stay tangent and phase-meshed even
  // while their shared carrier revolves and the whole assembly drifts.
  const double rBig=.385, rSmall=.275, nBig=14, nSmall=10;
  assert(std::fabs(rBig/rSmall-nBig/nSmall)<.00001);
  const uint8_t speeds[]={0,30,90,150,255};
  const uint32_t times[]={0U,67U,470U,1000U,5000U,17777U,
                          120000U,700000U,12345678U,2000000000U,4294967000U};
  for(uint8_t speed:speeds) for(uint32_t at:times) {
    const gptfx::GearPose p=gptfx::gearPose(at,speed);
    const double pitch=hypot((double)p.satelliteOffset.x,
                             (double)p.satelliteOffset.y);
    assert(std::fabs(pitch-rBig-rSmall)<.00001);
    // The whole mechanism, including the orbiting satellites and their teeth,
    // stays within square-matrix bounds for all sampled time/speed combinations.
    const double margin=(rBig+rSmall+rSmall+.043)*p.scale;
    assert(std::fabs(p.translation.x)+margin<1.0);
    assert(std::fabs(p.translation.y)+margin<1.0);
    // At each gear contact the sum of tooth phases must equal an odd multiple
    // of pi: one tooth crest faces a gap, including during orbital motion.
    const double right=nBig*(p.carrierAngle-p.sunAngle) +
                       nSmall*(p.carrierAngle+gptfx::K_PI-p.satelliteAngle);
    const double left=nBig*(p.carrierAngle+gptfx::K_PI-p.sunAngle) +
                      nSmall*(p.carrierAngle-p.satelliteAngle);
    assert(std::fabs(std::cos(right)+1.0)<.0005);
    assert(std::fabs(std::cos(left)+1.0)<.0005);
  }
  const auto g0=gptfx::gearPose(700000U,150);
  const auto g1=gptfx::gearPose(706000U,150);
  assert(hypot(g1.translation.x-g0.translation.x,
               g1.translation.y-g0.translation.y)>.05f);
  assert(hypot(g1.satelliteOffset.x-g0.satelliteOffset.x,
               g1.satelliteOffset.y-g0.satelliteOffset.y)>.4f);
  // A fixed-view rendering must visibly change due to the moving gear centers.
  {
    Segment a(64,64);strip._currentSegment=&a;
    a.speed=150;a.call=0;strip.now=700000U;strip.effects[5].fn();
    Segment b(64,64);strip._currentSegment=&b;
    b.speed=150;b.call=0;strip.now=706000U;strip.effects[5].fn();
    assert(difference(a.frameBuffer,b.frameBuffer)>2500);
  }
  std::puts("GPT06_ORBIT_MESH_TEST_PASS 5 speeds x 11 times; contact phase, viewport, travel and rendering");
  std::puts("HOST_TEST_PASS 30 FX x 5 resolutions + 30 FX x 9 palette states + 5 FFT proofs + no-audio + NaN + epicyclic mesh + nonmatrix");
}
