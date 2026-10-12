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
#include "gpt24_dev7_reference.h"
#include "gpt24_dev8_reference.h"
#include "gpt24_dev9_reference.h"
#include "gpt_multi_dev10_reference.h"

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
    assert(fields.at("u.gptMatrixFX.build")=="rc.1");
    assert(fields.at("u.gptMatrixFX.effects")=="40");
    assert(fields.at("u.gptMatrixFX.standard")=="30");
    assert(fields.at("u.gptMatrixFX.audio")=="10");
    assert(fields.at("u.gptMatrixFX.experimental")=="0");
    assert(fields.at("u.gptMatrixFX.performance.effect")=="GPT24");
    assert(fields.at("u.gptMatrixFX.performance.rendered")=="0");
    assert(fields.at("u.gptMatrixFX.trigonometry.effect")=="GPT24");
    assert(fields.at("u.gptMatrixFX.trigonometry.tableSamples")=="1024");
    assert(fields.at("u.gptMatrixFX.trigonometry.tableBytes")=="4096");
    assert(fields.at("u.gptMatrixFX.trigonometry.backend")==gptfx_trig::backendName());
    assert(fields.at("u.gptMatrixFX.trigonometry.extraBackend")==gptfx::extraBackendName());
    assert(fields.at("u.gptMatrixFX.trigonometry.extraEffects")=="GPT08,GPT20");
    assert(fields.at("u.gptMatrixFX.trigonometry.labyrinthBackend")==gptfx::labyrinthBackendName());
    assert(fields.at("u.gptMatrixFX.extraPerformance.effect")=="none");
    std::puts("JSON_INFO_TEST_PASS");
  }
  constexpr size_t NORMAL=20,AUDIO=5,EXPERIMENTAL=10,NEW_AUDIO=5,COUNT=NORMAL+AUDIO+EXPERIMENTAL+NEW_AUDIO;
  assert(strip.effects.size()==COUNT);
  for(size_t n=0;n<COUNT;n++) {
    const auto& fx=strip.effects[n];
    char want[36];
    if(n<NORMAL) std::snprintf(want,sizeof(want),"GPT%02zu - ",n+1);
    else if(n<NORMAL+AUDIO) std::snprintf(want,sizeof(want),"GPTA%02zu - ",n-NORMAL+1);
    else if(n<NORMAL+AUDIO+EXPERIMENTAL) std::snprintf(want,sizeof(want),"GPT%02zu - ",n-AUDIO+1);
    else std::snprintf(want,sizeof(want),"GPTA%02zu - ",n-(NORMAL+AUDIO+EXPERIMENTAL)+6);
    assert(std::strstr(fx.name,want)==fx.name);
    assert(fx.id==255);
    if((n>=NORMAL && n<NORMAL+AUDIO) || n>=NORMAL+AUDIO+EXPERIMENTAL) assert(std::strstr(fx.name,";2v;")!=nullptr);
    else assert(std::strstr(fx.name,";2;")!=nullptr);
  }
  // Frozen baseline: dev.8 must still match dev.7 pixel-for-pixel.
  // dev.9 uses periodic phase reduction, so tiny FP rounding differences
  // versus dev.8 are permitted but large geometry changes are forbidden.
  {
    struct Case { int w,h; uint32_t time; uint8_t speed,intensity,petals,size,palette; };
    const Case cases[]={
      {8,8,1234,0,0,0,0,0},
      {16,32,50000,255,255,255,255,6},
      {64,64,700000,135,190,128,128,6},
      {64,64,2100123,240,40,220,255,8},
      {64,64,4200000,19,250,0,0,0},
      {96,48,1234567,255,150,190,90,11},
      {128,128,960000,80,215,210,220,7}
    };
    for(const Case& c:cases) {
      Segment oldSeg(c.w,c.h),dev8Seg(c.w,c.h),newSeg(c.w,c.h);
      auto setControls=[&](Segment& seg) {
        seg.speed=c.speed; seg.intensity=c.intensity;
        seg.custom1=c.petals; seg.custom2=c.size;
        seg.palette=c.palette; seg.call=0;
      };
      setControls(oldSeg);setControls(dev8Seg);setControls(newSeg);
      strip.now=c.time;
      strip._currentSegment=&oldSeg;
      gptfx::mode_gpt24_dev7_reference();
      strip._currentSegment=&dev8Seg;
      gptfx::mode_gpt24_dev8_reference();
      if(difference(oldSeg.frameBuffer,dev8Seg.frameBuffer)!=0) {
        std::fprintf(stderr,"DEV8 REFERENCE REGRESSION %dx%d t=%u\n",c.w,c.h,c.time);
        return 14;
      }
      strip._currentSegment=&newSeg;
      gptfx::mode_gpt24();
      unsigned maxDiff=0;
      for(size_t i=0;i<newSeg.frameBuffer.size();++i) {
        unsigned d=(unsigned)std::abs((int)newSeg.frameBuffer[i]-(int)dev8Seg.frameBuffer[i]);
        if(d>maxDiff) maxDiff=d;
      }
      if(maxDiff>
#if GPTFX_GPT24_TRIG_BACKEND == 1
         10
#else
         2
#endif
        ) {
        std::fprintf(stderr,"GPT24 PHASE REGRESSION %dx%d t=%u maxDiff=%u\n",c.w,c.h,c.time,maxDiff);
        return 14;
      }
    }
    std::puts("GPT24_DEV8_REFERENCE_PASS 7 dev.7 pixel-identical comparisons; dev.9 <=2/channel");
  }
  // Time-accumulation regression: 48 deliberately spaced uptime/speed
  // samples on a 64x64 segment, from a cold boot up to two hours.
  // The framebuffer must retain its shape and remain within a visually
  // negligible 2/255 per-channel error compared with original dev.8.
  {
    constexpr uint32_t uptimeSeconds[]={0,14,40,90,150,300,400,700,900,1800,3600,7200};
    constexpr uint8_t speeds[]={0,30,135,255};
    unsigned worstDiff=0, worstChangedPixels=0;
    for(uint8_t speed:speeds) for(uint32_t sec:uptimeSeconds) {
      Segment original(64,64),bounded(64,64);
      for(Segment* seg:{&original,&bounded}) {
        seg->speed=speed;seg->intensity=190;seg->custom1=128;
        seg->custom2=128;seg->palette=6;seg->call=0;
      }
      strip.now=sec*1000U;
      strip._currentSegment=&original;
      gptfx::mode_gpt24_dev8_reference();
      strip._currentSegment=&bounded;
      gptfx::mode_gpt24();
      unsigned maxDiff=0,changedPx=0;
      for(size_t pixel=0;pixel<64U*64U;++pixel) {
        bool changed=false;
        for(size_t c=0;c<4;++c) {
          const size_t i=pixel*4U+c;
          unsigned d=(unsigned)std::abs((int)original.frameBuffer[i]-(int)bounded.frameBuffer[i]);
          if(d>maxDiff) maxDiff=d;
          if(d) changed=true;
        }
        if(changed) ++changedPx;
      }
      if(maxDiff>worstDiff) worstDiff=maxDiff;
      if(changedPx>worstChangedPixels) worstChangedPixels=changedPx;
      if(maxDiff >
#if GPTFX_GPT24_TRIG_BACKEND == 1
         10 || changedPx>4096U
#else
         2 || changedPx>64U*64U/25U
#endif
       ) {
        std::fprintf(stderr,"GPT24 LONG-UPTIME REGRESSION speed=%u sec=%u maxDiff=%u changedPx=%u\n",
                     speed,sec,maxDiff,changedPx);
        return 15;
      }
      const float t=gptfx::timePhase(.6f);
      const float phases[]={t*.91f,t*-.72f,
        t*(.91f*1.7f+.4f),t*(-.72f*1.7f+.4f),t*3.0f};
      for(float raw:phases) {
        const float boundedPhase=gptfx::rosettePhase(raw);
        assert(std::isfinite(boundedPhase));
        assert(std::fabs(boundedPhase)<gptfx::K_TAU);
      }
    }
    std::printf("GPT24_LONG_UPTIME_PASS 48 cases 0..7200s worstChannelDelta=%u worstChangedPixels=%u/4096\n",
                worstDiff,worstChangedPixels);
  }
  // Standalone LUT numeric reference on the actual bounded angle range.
  // Sweeps negative and positive phases across many table cells, including
  // the +/- wrap boundaries used by the two rosette layers.
  {
    float worstNear = 0.0f, worstLinear = 0.0f;
    for(int i=-16000;i<=16000;++i) {
      const float a = (float)i * .005f;
      for(int cosine=0;cosine<2;++cosine) {
        const float exact=cosine?cosf(a):sinf(a);
        const float near=cosine?gptfx_trig::cosineNearest(a):gptfx_trig::sineNearest(a);
        const float interpolated=cosine?gptfx_trig::cosineLinear(a):gptfx_trig::sineLinear(a);
        worstNear=std::max(worstNear,fabsf(near-exact));
        worstLinear=std::max(worstLinear,fabsf(interpolated-exact));
      }
    }
    if(worstNear>0.0032f || worstLinear>0.00005f) {
      std::fprintf(stderr,"TRIG_LUT_NUMERIC_FAIL near=%f linear=%f\n",worstNear,worstLinear);
      return 18;
    }
    // Exceptional inputs must use native math (avoids undefined int casts).
    assert(std::isfinite(gptfx_trig::sineLinear(200000.0f)));
    assert(std::isfinite(gptfx_trig::cosineNearest(-200000.0f)));
    assert(std::isnan(gptfx_trig::sineLinear(std::numeric_limits<float>::quiet_NaN())));
    std::printf("TRIG_LUT_NUMERIC_PASS worstNearest=%.8f worstLinear=%.8f\n",worstNear,worstLinear);
  }
  // Compare the actual new frame against frozen dev.9, not only dev.8.
  // No palette, tick, color, spatial radius or other effect code changed.
  {
    const uint32_t uptimes[]={0,14,90,300,421,697,900,1800,3600,7200};
    const uint8_t speeds[]={0,30,135,255};
    const uint8_t palettes[]={0,6,8};
    unsigned worstDelta=0,worstPixels=0;
    for(uint8_t speed:speeds)for(uint32_t secs:uptimes)for(uint8_t palette:palettes) {
      Segment oldSeg(64,64),newSeg(64,64);
      for(Segment* p:{&oldSeg,&newSeg}) {
        p->speed=speed;p->intensity=190;p->custom1=128;
        p->custom2=128;p->palette=palette;p->call=0;
      }
      strip.now=secs*1000U;
      strip._currentSegment=&oldSeg;
      gptfx::mode_gpt24_dev9_reference();
      strip._currentSegment=&newSeg;
      gptfx::mode_gpt24();
      unsigned maxDelta=0,changedPixels=0;
      for(size_t pixel=0;pixel<4096;++pixel) {
        bool changed=false;
        for(size_t c=0;c<4;++c) {
          const size_t offset=pixel*4+c;
          unsigned d=(unsigned)abs((int)newSeg.frameBuffer[offset]-(int)oldSeg.frameBuffer[offset]);
          maxDelta=std::max(maxDelta,d);
          if(d)changed=true;
        }
        if(changed)++changedPixels;
      }
      worstDelta=std::max(worstDelta,maxDelta);
      worstPixels=std::max(worstPixels,changedPixels);
#if GPTFX_GPT24_TRIG_BACKEND == 0
      const unsigned channelThreshold=0,changedThreshold=0;
#elif GPTFX_GPT24_TRIG_BACKEND == 1
      const unsigned channelThreshold=12,changedThreshold=4096;
#else
      const unsigned channelThreshold=2,changedThreshold=164;
#endif
      if(maxDelta>channelThreshold || changedPixels>changedThreshold) {
        std::fprintf(stderr,"TRIG_LUT_FRAME_REGRESSION backend=%s speed=%u uptime=%u palette=%u maxDelta=%u changedPixels=%u\n",
           gptfx_trig::backendName(),speed,secs,palette,maxDelta,changedPixels);
        return 19;
      }
    }
    std::printf("GPT24_LUT_DEV9_FRAME_PASS backend=%s 120 cases maxChannelDelta=%u maxChangedPixels=%u/4096\n",
       gptfx_trig::backendName(),worstDelta,worstPixels);
  }
  // Frozen dev.10 pixel comparison for each new candidate. Native mode
  // must be byte-for-byte unchanged. LUT mode allows tiny quantization.
  {
    const uint32_t upSec[]={0,14,207,421,754,1800,7200};
    const uint8_t speeds[]={0,30,135,255};
    const uint8_t palettes[]={0,6,8};
    unsigned worst[4]={},changedMax[4]={};
    unsigned highPixelMax[4]={};
    double worstMeanAbs[4]={};
    const int fx[]={7,8,18,20};
    void(*base[])()={gptfx::mode_gpt7_dev10_reference,gptfx::mode_gpt8_dev10_reference,
                     gptfx::mode_gpt18_dev10_reference,gptfx::mode_gpt20_dev10_reference};
    void(*candidate[])()={gptfx::mode_gpt7,gptfx::mode_gpt8,gptfx::mode_gpt18,gptfx::mode_gpt20};
    for(int n=0;n<4;++n) for(uint32_t uptime:upSec) for(uint8_t speed:speeds)
      for(uint8_t palette:palettes) {
      Segment original(64,64),modified(64,64);
      for(Segment* seg:{&original,&modified}) {
        seg->speed=speed;seg->intensity=190;seg->custom1=128;
        seg->custom2=128;seg->palette=palette;seg->call=0;
      }
      strip.now=uptime*1000U;
      strip._currentSegment=&original;base[n]();
      strip._currentSegment=&modified;candidate[n]();
      unsigned delta=0,changed=0,highPixels=0;
      uint64_t sumAbs=0;
      for(size_t pixel=0;pixel<4096;++pixel) {
        bool differed=false,high=false;
        for(size_t channel=0;channel<4;++channel) {
          const size_t i=pixel*4+channel;
          unsigned diff=(unsigned)std::abs((int)original.frameBuffer[i]-(int)modified.frameBuffer[i]);
          delta=std::max(delta,diff);differed|=(diff!=0);high|=(diff>16);sumAbs+=diff;
        }
        if(differed)++changed;
        if(high)++highPixels;
      }
      worst[n]=std::max(worst[n],delta);
      changedMax[n]=std::max(changedMax[n],changed);
      highPixelMax[n]=std::max(highPixelMax[n],highPixels);
      worstMeanAbs[n]=std::max(worstMeanAbs[n],(double)sumAbs/(4096.0*3.0));
      // In native mode / exact-hoisting cases require bit identity.
      // In LUT modes, palette quantization can cause isolated jumps.
      bool isNative=(n==0) ||
          (n==2 && GPTFX_GPT18_TRIG_BACKEND==0 && GPTFX_GPT18_PHASE_BOUNDING==0) ||
          (n!=0 && n!=2 && GPTFX_EXTRA_TRIG_BACKEND==0);
      if((isNative && delta!=0) ||
         (!isNative && (changed>200 || highPixels>5 ||
                        ((double)sumAbs/(4096.0*3.0))>0.09))) {
        std::fprintf(stderr,"EXTRA_TRIG_REGRESSION GPT%02d uptime=%u speed=%u pal=%u delta=%u changed=%u highPixels=%u mean=%.5f\n",
                     fx[n],uptime,speed,palette,delta,changed,highPixels,(double)sumAbs/(4096.0*3.0));
        return 26;
      }
    }
    for(int n=0;n<4;++n)
      std::printf("EXTRA_TRIG_FRAME_PASS GPT%02d cases=84 maxChannelDelta=%u changedPixels=%u/4096 highPixelMax=%u meanAbsMax=%.4f\n",
                  fx[n],worst[n],changedMax[n],highPixelMax[n],worstMeanAbs[n]);
  }
  // Larger-host-resolution fidelity sampling (not a device FPS claim).
  {
    const int ids[]={7,8,18,20};
    void(*original[])()={gptfx::mode_gpt7_dev10_reference,gptfx::mode_gpt8_dev10_reference,
                         gptfx::mode_gpt18_dev10_reference,gptfx::mode_gpt20_dev10_reference};
    void(*modified[])()={gptfx::mode_gpt7,gptfx::mode_gpt8,gptfx::mode_gpt18,gptfx::mode_gpt20};
    for(int side: {64,128}) for(int n=0;n<4;++n) {
      const int w=128,h=side;
      unsigned worstDelta=0,worstChanged=0,worstHigh=0;
      double maxMean=0.0;
      for(uint32_t uptime: {14U,421U,1800U}) for(uint8_t speed: {uint8_t(0),uint8_t(135),uint8_t(255)}) {
        Segment a(w,h),b(w,h);
        for(Segment* seg:{&a,&b}){
          seg->speed=speed;seg->intensity=190;seg->custom1=128;
          seg->custom2=128;seg->palette=6;seg->call=0;
        }
        strip.now=uptime*1000U;
        strip._currentSegment=&a;original[n]();
        strip._currentSegment=&b;modified[n]();
        unsigned changed=0,high=0,delta=0;
        uint64_t sum=0;
        for(int pixel=0;pixel<w*h;++pixel){
          bool differ=false,highDiff=false;
          for(int c=0;c<3;++c){
            int d=std::abs((int)a.frameBuffer[(size_t)pixel*4+c]-(int)b.frameBuffer[(size_t)pixel*4+c]);
            delta=std::max(delta,(unsigned)d);sum+=(unsigned)d;
            differ|=d!=0;highDiff|=d>16;
          }
          changed+=differ;high+=highDiff;
        }
        const double mean=(double)sum/(w*h*3.0);
        worstDelta=std::max(worstDelta,delta);
        worstChanged=std::max(worstChanged,changed);
        worstHigh=std::max(worstHigh,high);
        maxMean=std::max(maxMean,mean);
        bool native=(n==0)||(n==2 && GPTFX_GPT18_TRIG_BACKEND==0 && GPTFX_GPT18_PHASE_BOUNDING==0)||
          (n!=0&&n!=2&&GPTFX_EXTRA_TRIG_BACKEND==0);
        if((native&&delta) || (!native && (changed>(unsigned)(w*h/20) ||
            high>(unsigned)(w*h/250) || mean>0.12))) {
          std::fprintf(stderr,"EXTRA_LARGE_REGRESSION GPT%02d %dx%d sec=%u changed=%u high=%u mean=%.4f delta=%u\n",
                       ids[n],w,h,uptime,changed,high,mean,delta);
          return 28;
        }
      }
      std::printf("EXTRA_LARGE_FRAME_PASS GPT%02d %dx%d cases=9 maxChannelDelta=%u changed=%u high=%u meanAbsMax=%.4f\n",
        ids[n],w,h,worstDelta,worstChanged,worstHigh,maxMean);
    }
  }
  // Verify the new wrapper's rendered/skipped accounting and rolling window.
  {
    Segment seg(64,64);strip._currentSegment=&seg;
    strip.now=123456;seg.call=0;strip.effects[7].fn();
    ++seg.call;strip.now+=10;strip.effects[7].fn();
    ++seg.call;strip.now+=50;strip.effects[7].fn();
    assert(gptfx::extraPerf.calls==3 && gptfx::extraPerf.rendered==2 && gptfx::extraPerf.skipped==1);
    for(int i=0;i<35;++i){++seg.call;strip.now+=50;strip.effects[7].fn();}
    assert(gptfx::extraPerf.recentValid==32);
    std::map<std::string,std::string> fields;
    JsonObject info{&fields,""};globalMod->addToJsonInfo(info);
    assert(fields.at("u.gptMatrixFX.extraPerformance.effect")=="GPT08");
    assert(std::stoul(fields.at("u.gptMatrixFX.extraPerformance.recentAvgUs"))>0);
    strip.now+=100;seg.call=0;strip.effects[6].fn();
    assert(gptfx::extraPerf.calls==1 && gptfx::extraPerf.rendered==1);
    assert(std::strcmp(gptfx::extraPerf.effect,"GPT07")==0);
    std::puts("EXTRA_TRIG_TELEMETRY_PASS");
  }
  // GPT24 profiling: first draw, throttled call, then another draw.
  {
    Segment seg(64,64); strip._currentSegment=&seg;
    const uint32_t calls0=gptfx::gpt24Perf.calls;
    const uint32_t drawn0=gptfx::gpt24Perf.rendered;
    const uint32_t skipped0=gptfx::gpt24Perf.skipped;
    strip.now=50000; seg.call=0; strip.effects[28].fn();
    ++seg.call; strip.now+=10; strip.effects[28].fn();
    ++seg.call; strip.now+=50; strip.effects[28].fn();
    assert(gptfx::gpt24Perf.calls==calls0+3);
    assert(gptfx::gpt24Perf.rendered==drawn0+2);
    assert(gptfx::gpt24Perf.skipped==skipped0+1);
    assert(gptfx::gpt24Perf.width==64 && gptfx::gpt24Perf.height==64);
    assert(gptfx::gpt24Perf.renderMaxUs>0);
    assert(gptfx::gpt24Perf.recentValid>=2);
    assert(gptfx::gpt24Perf.renderLastUs>0);
    assert(gptfx::gpt24Perf.rowSamples>=128);
    assert(gptfx::gpt24Perf.rowMaxUs>0);
    assert(gptfx::gpt24Perf.phaseRawAbs>0.0f);
    assert(gptfx::gpt24Perf.phaseWrappedAbs<gptfx::K_TAU);
    assert(gptfx::gpt24Perf.coreStart==-1); // Host has no FreeRTOS cores.
    assert(gptfx::gpt24Perf.coreEnd==-1);
    // Rolling window must retain at most 32 recent renders, not all history.
    for(int n=0;n<37;++n) {
      seg.call++;strip.now+=50;
      strip.effects[28].fn();
    }
    assert(gptfx::gpt24Perf.recentValid==32);
    assert(gptfx::gpt24Perf.recentWrite<32);
    std::map<std::string,std::string> fields;
    JsonObject info{&fields, ""};globalMod->addToJsonInfo(info);
    assert(std::stoul(fields.at("u.gptMatrixFX.performance.recentCount"))>=2);
    assert(std::stoul(fields.at("u.gptMatrixFX.performance.recentAvgUs"))>0);
    assert(std::stoul(fields.at("u.gptMatrixFX.performance.rowMaxUs"))>0);
    assert(std::stof(fields.at("u.gptMatrixFX.performance.phaseRawAbs"))>0.0f);
    assert(std::stof(fields.at("u.gptMatrixFX.performance.phaseWrappedAbs"))<gptfx::K_TAU);
    std::puts("GPT24_TELEMETRY_TEST_PASS");
  }
  initAudio();
  const std::pair<int,int> sizes[]={{8,8},{16,32},{64,64},{96,48},{128,128}};
  for(const auto& dim:sizes) {
    const int w=dim.first,h=dim.second;
    for(size_t n=0;n<COUNT;n++) {
      Segment seg(w,h);strip._currentSegment=&seg;
      seg.call=0;strip.now=700000;
      UsermodManager::audioOn=(n>=NORMAL && n<NORMAL+AUDIO) || n>=NORMAL+AUDIO+EXPERIMENTAL;
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
  std::puts("PALETTE_CHANGE_TEST_PASS 40 FX x 9 palette states + default-black regression + dark-entry preservation");
  // Audio absent: 5 effects stay safe and safely subdued, no fake FFT signal.
  UsermodManager::audioOn=false;
  for(int idx=NORMAL;idx<(int)COUNT;++idx){
    if(idx>=(int)(NORMAL+AUDIO) && idx<(int)(NORMAL+AUDIO+EXPERIMENTAL))continue;
    Segment seg(64,64);strip._currentSegment=&seg;seg.call=0;
    strip.now=970000;strip.effects[idx].fn();assert(colorCount(seg)>1);
  }
  // Audio metadata inaccessible: must not dereference u_data or stale pointers.
  UsermodManager::audioOn=true;
  UsermodManager::audio.u_size=0;
  for(int idx=NORMAL;idx<(int)COUNT;++idx){
    if(idx>=(int)(NORMAL+AUDIO) && idx<(int)(NORMAL+AUDIO+EXPERIMENTAL))continue;
    Segment seg(64,64);strip._currentSegment=&seg;seg.call=0;strip.now=980000;
    strip.effects[idx].fn();
  }
  // Phase regression and fixed reference checks ran above.
  // GPT24 profiling: first draw, throttled call, then another draw.
  {
    Segment seg(64,64); strip._currentSegment=&seg;
    const uint32_t calls0=gptfx::gpt24Perf.calls;
    const uint32_t drawn0=gptfx::gpt24Perf.rendered;
    const uint32_t skipped0=gptfx::gpt24Perf.skipped;
    strip.now=50000; seg.call=0; strip.effects[28].fn();
    ++seg.call; strip.now+=10; strip.effects[28].fn();
    ++seg.call; strip.now+=50; strip.effects[28].fn();
    assert(gptfx::gpt24Perf.calls==calls0+3);
    assert(gptfx::gpt24Perf.rendered==drawn0+2);
    assert(gptfx::gpt24Perf.skipped==skipped0+1);
    assert(gptfx::gpt24Perf.width==64 && gptfx::gpt24Perf.height==64);
    assert(gptfx::gpt24Perf.renderMaxUs>0);
    assert(gptfx::gpt24Perf.recentValid>=2);
    assert(gptfx::gpt24Perf.renderLastUs>0);
    assert(gptfx::gpt24Perf.rowSamples>=128);
    assert(gptfx::gpt24Perf.rowMaxUs>0);
    assert(gptfx::gpt24Perf.coreStart==-1); // Host has no FreeRTOS cores.
    assert(gptfx::gpt24Perf.coreEnd==-1);
    // Rolling window must retain at most 32 recent renders, not all history.
    for(int n=0;n<37;++n) {
      seg.call++;strip.now+=50;
      strip.effects[28].fn();
    }
    assert(gptfx::gpt24Perf.recentValid==32);
    assert(gptfx::gpt24Perf.recentWrite<32);
    std::map<std::string,std::string> fields;
    JsonObject info{&fields, ""};globalMod->addToJsonInfo(info);
    assert(std::stoul(fields.at("u.gptMatrixFX.performance.recentCount"))>=2);
    assert(std::stoul(fields.at("u.gptMatrixFX.performance.recentAvgUs"))>0);
    assert(std::stoul(fields.at("u.gptMatrixFX.performance.rowMaxUs"))>0);
    std::puts("GPT24_TELEMETRY_TEST_PASS");
  }
  initAudio();
  // Direct proof each audio effect responds to changes in FFT at fixed time.
  for(int idx=NORMAL;idx<(int)COUNT;++idx){
    if(idx>=(int)(NORMAL+AUDIO) && idx<(int)(NORMAL+AUDIO+EXPERIMENTAL))continue;
    std::memset(fft,0,sizeof(fft));vol=0.0f;peak=0;
    Segment a(64,64);strip._currentSegment=&a;strip.now=990000;a.call=0;
    strip.effects[idx].fn();auto quiet=a.frameBuffer;
    std::memset(fft,255,sizeof(fft));vol=220.0f;peak=1;
    Segment b(64,64);strip._currentSegment=&b;strip.now=990000;b.call=0;
    strip.effects[idx].fn();
    size_t changed=difference(quiet,b.frameBuffer);
    if(changed<400){std::fprintf(stderr,"Audio FX %d insufficient response: %zu bytes changed\n",idx+1,changed);return 3;}
    std::printf("GPTA%02d FFT_TEST_PASS changed_channels=%zu\n",idx<(int)(NORMAL+AUDIO) ? idx-(int)NORMAL+1 : idx-(int)(NORMAL+AUDIO+EXPERIMENTAL)+6,changed);
  }
  // Audio not finite: sanitize from external source before math conversion.
  vol=std::numeric_limits<float>::quiet_NaN();
  for(int idx=NORMAL;idx<(int)COUNT;++idx){
    if(idx>=(int)(NORMAL+AUDIO) && idx<(int)(NORMAL+AUDIO+EXPERIMENTAL))continue;
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
  std::puts("HOST_TEST_PASS 40 FX x 5 resolutions + 40 FX x 9 palette states + 10 FFT proofs + no-audio + NaN + epicyclic mesh + nonmatrix");
}
