/*
 * GPT Matrix FX - 40 WLED 2D effects (c) 2026.
 * Version: 0.3.0 (30 standard + 10 audio; 0 experimental audio).
 * License: MIT. Designed for WLED 0.15+ / modern WLED usermod FX API.
 * No WLED core edits, no display buffers outside WLED's segment storage.
 * The number on the effect name is a stable human label, not a WLED mode ID.
 */
#include "wled.h"
#include <cmath>
#include <string.h>
#include <stdlib.h>
#include "gptfx_trig_lut.h"

// Public JSON stays compact; internal profiling and diagnostics remain active.
#ifndef GPTFX_ENABLE_JSON_DIAGNOSTICS
#define GPTFX_ENABLE_JSON_DIAGNOSTICS 0
#endif
#if GPTFX_ENABLE_JSON_DIAGNOSTICS != 0 && GPTFX_ENABLE_JSON_DIAGNOSTICS != 1
#error "GPTFX_ENABLE_JSON_DIAGNOSTICS must be 0 or 1"
#endif

namespace gptfx {
constexpr const char* GPTFX_VERSION = "0.3.0";
constexpr const char* GPTFX_BUILD = "release";
constexpr float K_PI = 3.14159265358979323846f;
constexpr float K_TAU = 6.28318530717958647692f;

struct Point { float x, y; };

static inline float clip(float v, float lo = 0.0f, float hi = 1.0f) {
  return v < lo ? lo : (v > hi ? hi : v);
}
static inline float squaref(float v) { return v * v; }
static inline float frac(float v) { return v - floorf(v); }
// Experimental second-stage LUT. GPT24 remains independently controlled.
// 0: frozen dev.10 libm; 2: interpolated shared 1024-entry LUT.
#ifndef GPTFX_EXTRA_TRIG_BACKEND
#define GPTFX_EXTRA_TRIG_BACKEND 2
#endif
#if GPTFX_EXTRA_TRIG_BACKEND != 0 && GPTFX_EXTRA_TRIG_BACKEND != 2
#error "GPTFX_EXTRA_TRIG_BACKEND must be 0 (libm) or 2 (linear LUT)"
#endif
static inline float extraSine(float x) {
#if GPTFX_EXTRA_TRIG_BACKEND == 0
  return sinf(x);
#else
  return gptfx_trig::sineLinear(x);
#endif
}
static inline float extraCosine(float x) {
#if GPTFX_EXTRA_TRIG_BACKEND == 0
  return cosf(x);
#else
  return gptfx_trig::cosineLinear(x);
#endif
}
// GPT18's per-pixel sine input contains a time phase that grows with uptime.
// Bound ONLY the periodic temporal component, once per layer per frame.
// 0: preserve frozen dev.11 for A/B; 1: phase-bound both layer waves and folds.
#ifndef GPTFX_GPT18_PHASE_BOUNDING
#define GPTFX_GPT18_PHASE_BOUNDING 1
#endif
#if GPTFX_GPT18_PHASE_BOUNDING != 0 && GPTFX_GPT18_PHASE_BOUNDING != 1
#error "GPTFX_GPT18_PHASE_BOUNDING must be 0 or 1"
#endif
// Reduce a periodic phase without a potentially expensive reduction inside
// the pixel loop. The centered interval also minimizes floating-point drift.
// Stage-1 phase bounding for GPT05, GPT11, GPT13, GPT19, GPT23, GPTA05.
// This gate preserves original expressions and native libm when disabled.
// Keep temporal reductions outside the loops and never wrap hue, frac, noise
// seeds, spatial geometry, or the entire phase before frequency multiplication.
#ifndef GPTFX_STAGE1_PHASE_BOUNDING
#define GPTFX_STAGE1_PHASE_BOUNDING 1
#endif
#if GPTFX_STAGE1_PHASE_BOUNDING != 0 && GPTFX_STAGE1_PHASE_BOUNDING != 1
#error "GPTFX_STAGE1_PHASE_BOUNDING must be 0 or 1"
#endif
#if GPTFX_STAGE1_PHASE_BOUNDING
#define GPTFX_STAGE1_ARG(raw, bounded) (bounded)
#else
#define GPTFX_STAGE1_ARG(raw, bounded) (raw)
#endif
static inline float stage1BoundPhase(float phase) {
  float bounded=fmodf(phase,K_TAU);
  if(bounded>K_PI)bounded-=K_TAU;
  else if(bounded<-K_PI)bounded+=K_TAU;
  return bounded;
}
// Stage-2A phase bounding for GPT01, GPT02, GPT09, GPT10, GPT14, GPT16.
// Independently reversible and never applied to hashes, fract, hue or cells.
#ifndef GPTFX_STAGE2_PHASE_BOUNDING
#define GPTFX_STAGE2_PHASE_BOUNDING 1
#endif
#if GPTFX_STAGE2_PHASE_BOUNDING != 0 && GPTFX_STAGE2_PHASE_BOUNDING != 1
#error "GPTFX_STAGE2_PHASE_BOUNDING must be 0 or 1"
#endif
#if GPTFX_STAGE2_PHASE_BOUNDING
#define GPTFX_STAGE2_ARG(raw, bounded) (bounded)
#else
#define GPTFX_STAGE2_ARG(raw, bounded) (raw)
#endif
static inline float stage2BoundPhase(float phase) {
  float bounded=fmodf(phase,K_TAU);
  if(bounded>K_PI)bounded-=K_TAU;
  else if(bounded<-K_PI)bounded+=K_TAU;
  return bounded;
}
// Stage-2B: six remaining native per-pixel temporal-trig effects.
// 0 reproduces the frozen dev.14a math; 1 bounds only periodic time phases.
#ifndef GPTFX_STAGE2B_PHASE_BOUNDING
#define GPTFX_STAGE2B_PHASE_BOUNDING 1
#endif
#if GPTFX_STAGE2B_PHASE_BOUNDING != 0 && GPTFX_STAGE2B_PHASE_BOUNDING != 1
#error "GPTFX_STAGE2B_PHASE_BOUNDING must be 0 or 1"
#endif
#if GPTFX_STAGE2B_PHASE_BOUNDING
#define GPTFX_STAGE2B_ARG(raw, bounded) (bounded)
#else
#define GPTFX_STAGE2B_ARG(raw, bounded) (raw)
#endif
// GPT10-only experiment: the spatial hash is cached by *exact* integer cell.
// Both gates are independent of stage-2A and may be disabled for A/B runs.
#ifndef GPTFX_GPT10_CELL_CACHE
#define GPTFX_GPT10_CELL_CACHE 1
#endif
#ifndef GPTFX_GPT10_PACKET_BOUNDING
#define GPTFX_GPT10_PACKET_BOUNDING GPTFX_STAGE2_PHASE_BOUNDING
#endif
#if (GPTFX_GPT10_CELL_CACHE != 0 && GPTFX_GPT10_CELL_CACHE != 1) || \
    (GPTFX_GPT10_PACKET_BOUNDING != 0 && GPTFX_GPT10_PACKET_BOUNDING != 1)
#error "GPTFX GPT10 flags must be 0 or 1"
#endif
static inline float labyrinthBoundPhase(float phase) {
#if GPTFX_GPT18_PHASE_BOUNDING == 0
  return phase;
#else
  float bounded = fmodf(phase, K_TAU);
  if (bounded > K_PI) bounded -= K_TAU;
  else if (bounded < -K_PI) bounded += K_TAU;
  return bounded;
#endif
}
// GPT18 is separately opt-in: host LUT profiling did not demonstrate
// a consistent gain. Its per-layer temporal terms are still hoisted.
#ifndef GPTFX_GPT18_TRIG_BACKEND
#define GPTFX_GPT18_TRIG_BACKEND 0
#endif
#if GPTFX_GPT18_TRIG_BACKEND != 0 && GPTFX_GPT18_TRIG_BACKEND != 2
#error "GPTFX_GPT18_TRIG_BACKEND must be 0 (libm) or 2 (linear LUT)"
#endif
static inline float labyrinthSine(float x) {
#if GPTFX_GPT18_TRIG_BACKEND == 0
  return sinf(x);
#else
  return gptfx_trig::sineLinear(x);
#endif
}
static inline const char* labyrinthBackendName() {
#if GPTFX_GPT18_TRIG_BACKEND == 0
  return "libm";
#else
  return "lut1024-linear";
#endif
}
static inline const char* extraBackendName() {
#if GPTFX_EXTRA_TRIG_BACKEND == 0
  return "libm";
#else
  return "lut1024-linear";
#endif
}
static inline float ridge(float v) {
  float a = 1.0f - fabsf(sinf(v));
  return a * a * a * a;
}
static inline float smooth(float a, float b, float v) {
  float s = clip((v - a) / (b - a));
  return s * s * (3.0f - 2.0f * s);
}
static inline float timePhase(float scale = 1.0f) {
  return (float)strip.now * 0.001f * (0.15f + SEGMENT.speed * (1.45f / 255.0f)) * scale;
}
static inline float detail() { return (float)SEGMENT.intensity / 255.0f; }
static inline float tune1() { return (float)SEGMENT.custom1 / 255.0f; }
static inline float tune2() { return (float)SEGMENT.custom2 / 255.0f; }
static inline Point normal(int x, int y, float unit, float halfW, float halfH) {
  return { ((float)x + 0.5f - halfW) * unit, ((float)y + 0.5f - halfH) * unit };
}
// Palette state is sampled once at the beginning of each effect invocation.
// WLED draws effects sequentially, so this temporary state belongs to the
// current invocation rather than to a particular palette or segment cache.
static bool paletteIsBlack = false;

// Used only when a palette contains no non-black entries at all (e.g. the
// Primary Color palette with a black primary color). Dark entries within an
// otherwise valid palette are deliberately left untouched.
static inline uint32_t fallbackRainbow(uint8_t h, uint8_t bri) {
  const uint8_t sector = h / 43U;
  const uint8_t offset = (uint8_t)((h - sector * 43U) * 6U);
  uint8_t r=0, g=0, b=0;
  switch (sector) {
    case 0: r=255; g=offset; break;
    case 1: r=(uint8_t)(255U-offset); g=255; break;
    case 2: g=255; b=offset; break;
    case 3: g=(uint8_t)(255U-offset); b=255; break;
    case 4: r=offset; b=255; break;
    default: r=255; b=(uint8_t)(255U-offset); break;
  }
  return ((uint32_t)((uint16_t)r*bri/255U)<<16) |
         ((uint32_t)((uint16_t)g*bri/255U)<<8) |
         ((uint32_t)((uint16_t)b*bri/255U));
}
static inline uint32_t chroma(float hue, float brightness) {
  const uint8_t index = (uint8_t)((int32_t)floorf(hue) & 255);
  const uint8_t bri = (uint8_t)(clip(brightness) * 255.0f);
  if (paletteIsBlack) return fallbackRainbow(index, bri);
  // mcol = 255 is essential: WLED interprets mcol = 0 with palette 0
  // as "use primary segment color", which may be black. With 255 it uses
  // the default Party palette. Other palettes are still honored normally.
  return SEGMENT.color_from_palette(index, false, true, 255, bri);
}
static inline bool ready() {
  if (!strip.isMatrix || !SEGMENT.is2D() || SEG_W < 2 || SEG_H < 2) {
    SEGMENT.fill(SEGCOLOR(0));
    return false;
  }
  // Check the full 16-slot palette, including entries interpolated with
  // the WLED-selected blend/wrap behavior, before applying any brightness.
  // Never replace the black portions of a normally colored palette.
  paletteIsBlack = true;
  for (unsigned i=0; i<16; ++i) {
    if (SEGMENT.color_from_palette((uint16_t)(i*16U), false, true, 255, 255) != 0U) {
      paletteIsBlack = false;
      break;
    }
  }
  return true;
}
// Diagnostic counters only for GPT24. No per-pixel instrumentation.
// Updated by the WLED effects task; JSON reads are diagnostic snapshots.
struct GPT24Perf {
  static constexpr uint8_t RECENT_COUNT = 32;
  uint32_t calls = 0;
  uint32_t rendered = 0;
  uint32_t skipped = 0;
  uint32_t renderMaxUs = 0;
  uint64_t renderTotalUs = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  // Non-cumulative recent samples distinguish BLE advertising from connected.
  // Fixed storage: no allocations, no statistics calculated inside the pixel loop.
  uint32_t recentUs[RECENT_COUNT] = {};
  uint8_t recentWrite = 0;
  uint8_t recentValid = 0;
  uint32_t renderLastUs = 0;
  // Subframe wall-clock samples, not task CPU time. A preempted task can
  // produce high row times without extra trigonometric operations.
  uint32_t rowMaxUs = 0;
  uint64_t rowTotalUs = 0;
  uint32_t rowSamples = 0;
  int8_t coreStart = -1;
  int8_t coreEnd = -1;
  uint32_t coreChanges = 0;
  // Last GPT24 render: maximum magnitude of the five temporal inputs
  // before/after wrapping (radians; not including pixel-dependent angles).
  float phaseRawAbs = 0.0f;
  float phaseWrappedAbs = 0.0f;
};
static inline int8_t currentCore() {
#if defined(ARDUINO_ARCH_ESP32)
  return (int8_t)xPortGetCoreID();
#else
  return -1;  // Host test double has no FreeRTOS scheduler.
#endif
}
static GPT24Perf gpt24Perf;
// One-time microbenchmark: sampled at setup, never inside the hot pixel loop.
// Samples both negative and positive bounded angles representative of GPT24.
// Long-phase native libm is measured separately to expose costly reduction.
struct GPTTrigBenchmark {
  uint32_t nativeSmallNs = 0;
  uint32_t nativeLargeNs = 0;
  uint32_t nearestNs = 0;
  uint32_t linearNs = 0;
  volatile float sink = 0.0f;
};
static GPTTrigBenchmark trigBenchmark;
static inline uint32_t measureTrigNs(int which) {
  const uint32_t count = 2048U;
  float sum = 0.0f;
  const uint32_t start = micros();
  for(uint32_t i=0;i<count;++i) {
    // Alternate sine and cosine. Keep input values nonconstant for compiler.
    const float x = which==1 ? 160.0f+(float)(i%997U)*1.3f :
                (float)((int)(i%127U)-63)*0.975f;
    switch(which) {
      case 0: sum += (i&1) ? cosf(x) : sinf(x); break;
      case 1: sum += (i&1) ? cosf(x) : sinf(x); break;
      case 2: sum += (i&1) ? gptfx_trig::cosineNearest(x) : gptfx_trig::sineNearest(x); break;
      default: sum += (i&1) ? gptfx_trig::cosineLinear(x) : gptfx_trig::sineLinear(x); break;
    }
  }
  trigBenchmark.sink = sum; // Prevent dead-code elimination.
  const uint32_t elapsed = (uint32_t)(micros()-start);
  return (uint32_t)((uint64_t)elapsed * 1000ULL / count);
}
static inline void runTrigBenchmark() {
  trigBenchmark.nativeSmallNs = measureTrigNs(0);
  trigBenchmark.nativeLargeNs = measureTrigNs(1);
  trigBenchmark.nearestNs = measureTrigNs(2);
  trigBenchmark.linearNs = measureTrigNs(3);
}

// Light measurements for the four non-GPT24 candidates. Reset when the
// selected effect changes. No per-pixel timestamps or heap allocation.
struct ExtraPerf {
  static constexpr uint8_t RECENT_COUNT=32;
  const char* effect="none";
  uint32_t calls=0,rendered=0,skipped=0,lastUs=0,maxUs=0;
  uint32_t recentUs[RECENT_COUNT]={};
  uint8_t recentWrite=0,recentValid=0;
  uint16_t width=0,height=0;
  // Last GPT18 frame: maximum absolute raw/bounded phase input (radians).
  // Only the time-dependent portions are measured, not spatial coordinates.
  float phaseRawAbs=0.0f,phaseWrappedAbs=0.0f;
};
static ExtraPerf extraPerf;
// Last GPT10 frame only: no heap allocation, locks, or per-pixel timestamps.
// Counts distinguish redundant hash work from time-growing trig arguments.
struct GPT10Diag {
  uint32_t hashRequests=0, hashEvaluations=0, hashCacheHits=0;
  float hashArgAbs=0.0f, packetRawAbs=0.0f, packetBoundedAbs=0.0f;
};
static GPT10Diag gpt10Diag;

static bool extraTracking=false,extraTickReached=false,extraTickSkipped=false;
// Metadata sampled once per frame. Never touch trigonometric paths for JSON.
static inline void stage1TrackPhase(const float* raw,const float* bounded,unsigned n) {
  if(!extraTracking)return;
  float maxRaw=0.0f,maxBounded=0.0f;
  for(unsigned i=0;i<n;++i) {
    maxRaw=fmaxf(maxRaw,fabsf(raw[i]));
    maxBounded=fmaxf(maxBounded,fabsf(bounded[i]));
  }
  extraPerf.phaseRawAbs=maxRaw;
  extraPerf.phaseWrappedAbs=maxBounded;
}
static inline void stage2TrackPhase(const float* raw,const float* bounded,unsigned n) {
  stage1TrackPhase(raw,bounded,n);
}
static inline void profileExtra(const char* effect, void(*fn)()) {
  if (extraPerf.effect != effect || (SEGENV.call == 0 && extraPerf.calls != 0)) {
    extraPerf=ExtraPerf();
    extraPerf.effect=effect;
  }
  ++extraPerf.calls;
  extraTickReached=false;extraTickSkipped=false;extraTracking=true;
  const uint32_t start=micros();
  fn();
  const uint32_t elapsed=(uint32_t)(micros()-start);
  extraTracking=false;
  if(extraTickSkipped) ++extraPerf.skipped;
  else if(extraTickReached) {
    ++extraPerf.rendered;
    extraPerf.lastUs=elapsed;
    if(elapsed>extraPerf.maxUs)extraPerf.maxUs=elapsed;
    extraPerf.recentUs[extraPerf.recentWrite]=elapsed;
    extraPerf.recentWrite=(uint8_t)((extraPerf.recentWrite+1U)%ExtraPerf::RECENT_COUNT);
    if(extraPerf.recentValid<ExtraPerf::RECENT_COUNT)++extraPerf.recentValid;
    extraPerf.width=SEG_W;extraPerf.height=SEG_H;
  }
}
static bool gpt24Tracking = false;
static bool gpt24TickReached = false;
static bool gpt24TickSkipped = false;
static inline bool tick(uint32_t ms = 28) {
  if (gpt24Tracking) gpt24TickReached = true;
  if (extraTracking) extraTickReached = true;
  if (SEGENV.call && (uint32_t)(strip.now - SEGENV.step) < ms) {
    if (gpt24Tracking) gpt24TickSkipped = true;
    if (extraTracking) extraTickSkipped = true;
    return false;
  }
  SEGENV.step = strip.now;
  return true;
}
static inline float unitOf(int w, int h) {
  return 2.0f / (float)(w < h ? w : h);
}

// GPT01: Optical caustics from 3 orbiting emitters, refracted by a moving medium.
static void mode_gpt1() {
  if (!ready() || !tick(30)) return;
  int w = SEG_W, h = SEG_H;
  float unit = unitOf(w,h), t = timePhase(0.65f), d = detail(), lens = tune1();
  Point p0 = {0.53f*sinf(t*.39f), 0.54f*cosf(t*.27f)};
  Point p1 = {0.57f*cosf(t*.22f+2.1f), 0.48f*sinf(t*.44f)};
  Point p2 = {0.42f*sinf(t*.28f+4.0f), 0.52f*cosf(t*.33f)};
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[2]={t*.7f,-t*1.4f};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
#endif
  for (int y=0; y<h; ++y) for (int x=0; x<w; ++x) {
    Point p = normal(x,y,unit,w*.5f,h*.5f);
    float warp = (0.10f+0.30f*lens) * sinf(3.9f*p.x + 2.7f*p.y + GPTFX_STAGE2_ARG(t*.7f,phase[0]));
    float a = sqrtf(squaref(p.x-p0.x)+squaref(p.y-p0.y));
    float b = sqrtf(squaref(p.x-p1.x)+squaref(p.y-p1.y));
    float c = sqrtf(squaref(p.x-p2.x)+squaref(p.y-p2.y));
    float f = (12.0f+9.0f*d)*(a-b) + 9.0f*(b-c) + warp*3.0f;
    float g = (15.0f+8.0f*lens)*(a+c) + GPTFX_STAGE2_ARG(-t*1.4f,phase[1]);
    float light = clip(0.06f + 0.81f*ridge(f) + 0.32f*ridge(g));
    float prism = 44.0f*(a-b) + 22.0f*c + t*12.0f + 35.0f*tune2();
    SEGMENT.setPixelColorXY(x,y,chroma(prism,light));
  }
}
static const char m1[] PROGMEM = "GPT01 - Caustic Atlas@Motion,Interference,Refraction,Prism;!,!;!;2;sx=115,ix=145,c1=125,c2=155";

// GPT02: Moving Voronoi cell membranes, pulsed from the centers (not random noise).
static void mode_gpt2() {
  if (!ready() || !tick(33)) return;
  int w=SEG_W, h=SEG_H;
  float u=unitOf(w,h), t=timePhase(.55f), d=detail();
  constexpr int N=11;
  Point seeds[N];
  for(int i=0;i<N;++i) {
    float a = K_TAU*(float)i/N;
    float rad = .68f + .19f*sinf(t*.27f + i*1.8f);
    seeds[i]={rad*cosf(a + .21f*sinf(t*.31f+i)),rad*sinf(a + .17f*cosf(t*.29f+i*1.7f))};
  }
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={-t*1.7f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float first=1000.0f, second=1000.0f; int id=0;
    for(int i=0;i<N;++i) {
      float dist=squaref(p.x-seeds[i].x)+squaref(p.y-seeds[i].y);
      if(dist<first){second=first;first=dist;id=i;}
      else if(dist<second){second=dist;}
    }
    float wall=1.0f-smooth(.008f+.009f*tune1(),.052f+.075f*tune1(),second-first);
    float beat=ridge(14.0f*sqrtf(first)+GPTFX_STAGE2_ARG(-t*1.7f,phase[0]) + id*.4f);
    float glow=clip(.035f + wall*.78f + beat*(.14f+.31f*d));
    SEGMENT.setPixelColorXY(x,y,chroma(id*23.0f+t*9.0f+sqrtf(first)*20.0f+90.0f*tune2(),glow));
  }
}
static const char m2[] PROGMEM = "GPT02 - Voronoi Bloom@Migration,Pulse,Borders,Hues;!,!;!;2;sx=120,ix=145";

// GPT03: Folded facets and crisp creases rotate as one polygonal paper mechanism.
static void mode_gpt3() {
  if (!ready() || !tick(30)) return;
  int w=SEG_W,h=SEG_H; float u=unitOf(w,h),t=timePhase(.42f), d=detail();
  float folds=4.0f + floorf(tune1()*7.0f);
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float rad=sqrtf(squaref(p.x)+squaref(p.y));
    float ang=atan2f(p.y,p.x)+t*.19f;
    float step=K_TAU/folds;
    float local=fabsf(frac(ang/step+.5f)-.5f)*2.0f;
    float fan=1.0f-local;
    float crease=1.0f-smooth(.015f,.065f, fminf(local,1.0f-local));
    float level=frac(rad*(2.2f+3.1f*tune2())-fan*.61f-t*.12f);
    float foldline=1.0f-smooth(.025f,.12f,fminf(level,1.0f-level));
    float lit=.08f+.37f*fan+.43f*crease+.26f*foldline;
    lit*=1.0f-smooth(1.1f,2.5f,rad)*(.4f+.5f*d);
    SEGMENT.setPixelColorXY(x,y,chroma(55.0f*local+150.0f*level+t*8.0f,lit));
  }
}
static const char m3[] PROGMEM = "GPT03 - Origami Reactor@Rotation,Depth,Folds,Layers;!,!;!;2;sx=115,ix=150,c1=142";

// GPT04: 2D excitable-media automaton. Its two 64x64 maximum pages belong to SEGENV.
static void mode_gpt4() {
  if (!ready() || !tick(47)) return;
  int w=SEG_W,h=SEG_H;
  const unsigned sw=(unsigned)(w<64?w:64), sh=(unsigned)(h<64?h:64);
  const unsigned n=sw*sh;
  if(!SEGENV.allocateData(n*2U)) { SEGMENT.fill(SEGCOLOR(0)); return; }
  uint8_t *buffer=SEGENV.data;
  if (SEGENV.call == 0) {
    memset(buffer,0,n*2U);
    for(unsigned yy=0;yy<sh;++yy) for(unsigned xx=0;xx<sw;++xx) {
      unsigned seed=(xx*37U+yy*109U+(xx*yy)*11U)%127U;
      if(seed==0U || (squaref((float)xx-sw*.5f)+squaref((float)yy-sh*.5f)<4.0f)) buffer[yy*sw+xx]=255;
    }
    SEGENV.aux0=0;
    SEGENV.aux1=0;
  }
  uint8_t *cur=buffer+((SEGENV.aux0&1U)?n:0);
  uint8_t *next=buffer+((SEGENV.aux0&1U)?0:n);
  const uint8_t decay=(uint8_t)(6U+(unsigned)(SEGMENT.custom1>>4));
  for(unsigned yy=0;yy<sh;++yy) for(unsigned xx=0;xx<sw;++xx) {
    unsigned idx=yy*sw+xx;
    // Nonwrapping boundary avoids spurious jumps between opposite edges.
    unsigned active=0;
    if(xx && cur[idx-1U]>224U) ++active;
    if(xx+1U<sw && cur[idx+1U]>224U) ++active;
    if(yy && cur[idx-sw]>224U) ++active;
    if(yy+1U<sh && cur[idx+sw]>224U) ++active;
    uint8_t val=cur[idx];
    if(val==0U && active >= (SEGMENT.intensity > 150 ? 1U:2U)) next[idx]=255U;
    else next[idx]=(uint8_t)((val>decay)?(val-decay):0U);
  }
  ++SEGENV.aux1;
  if ((SEGENV.aux1%((SEGMENT.custom2>>3)+9U))==0) {
    unsigned xx=(unsigned)hw_random16((uint16_t)sw), yy=(unsigned)hw_random16((uint16_t)sh);
    next[yy*sw+xx]=255;
  }
  SEGENV.aux0 ^= 1U;
  float t=timePhase(.15f);
  for(int y=0;y<h;++y) {
    unsigned iy=(unsigned)((uint32_t)y*sh/(unsigned)h);
    for(int x=0;x<w;++x) {
      unsigned ix=(unsigned)((uint32_t)x*sw/(unsigned)w);
      uint8_t v=next[iy*sw+ix];
      float light=(v==0U)?0.018f : .10f+ .9f*smooth(.1f,1.0f, v/255.0f);
      SEGMENT.setPixelColorXY(x,y,chroma((255U-v)*.60f + x*.8f + t*10.0f,light));
    }
  }
}
static const char m4[] PROGMEM = "GPT04 - Ink Tectonics@Evolution,Propagation,Refractory,Seeds;!,!;!;2;sx=120,ix=190,c1=115,c2=140";

// GPT05: Inverse radial remapping and gravitationally-lensed counterflow rings.
static void mode_gpt5() {
  if (!ready() || !tick(32)) return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.40f);
  float mass=.25f+.85f*tune1();
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[3]={-t*.8f,-t*.6f,-t*2.7f};
  const float phase[3]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2])};
  stage1TrackPhase(raw,phase,3);
#endif
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r=sqrtf(squaref(p.x)+squaref(p.y));
    float angle=atan2f(p.y,p.x);
    float bend=mass/(r+.20f);
#if !GPTFX_STAGE1_PHASE_BOUNDING
    float spiral=angle+ bend + r*(4.5f+4.0f*tune2())-t;
#endif
    float rings=ridge((r+.08f*bend)*16.0f+GPTFX_STAGE1_ARG(-t*.8f,phase[0]));
    float stream=ridge(GPTFX_STAGE1_ARG(
        spiral*2.7f+2.5f*sinf(r*6.0f-t*.6f),
        (angle+bend+r*(4.5f+4.0f*tune2()))*2.7f+phase[2]+2.5f*sinf(r*6.0f+phase[1])));
    float horizon=1.0f-smooth(.14f,.34f+.12f*detail(),r);
    float accretion=smooth(.26f,.40f,r)*(1.0f-smooth(.95f,1.52f,r));
    float light=(.045f + .52f*rings + .55f*stream)*accretion + horizon*.12f;
    SEGMENT.setPixelColorXY(x,y,chroma(r*92.0f+bend*65.0f+t*13.0f,light));
  }
}
static const char m5[] PROGMEM = "GPT05 - Event Horizon@Orbit,Glow,Mass,Spiral;!,!;!;2;sx=120,ix=160,c1=135,c2=145";

// GPT06: a drifting epicyclic clockwork mechanism.
// The center gear spins while both satellites revolve around its axis. Relative
// rotation is derived from the no-slip constraint at the pitch circles; the
// satellites do NOT spin independently of the orbital carrier.
static inline float gearTooth(float phase) {
  // A tooth top, rounded flanks and a root gap. Distance from nearest crest.
  const float d=fabsf(frac(phase + .5f)-.5f)*2.0f;
  return 1.0f-smooth(.29f,.70f,d);
}
struct GearPose {
  Point translation;      // Group-wide screen-space drift, normalized units.
  Point satelliteOffset;  // Pitch-center displacement in local gear coordinates.
  float scale;            // Leaves a margin for drift and orbit on 64x64.
  float sunAngle;         // Absolute rotation of the central 14-tooth gear.
  float satelliteAngle;   // Absolute rotation of both 10-tooth planet gears.
  float carrierAngle;     // Orbital angle of the moving centers.
};
static GearPose gearPose(uint32_t now, uint8_t rawSpeed) {
  constexpr float pitchGap=.385f+.275f;
  constexpr double gearRatio=14.0/10.0;
  const float speed=rawSpeed/255.0f;
  const double seconds=(double)now*.001;
  // Preserve the qualified v0.2.1 center-gear rotation law, including top speed.
  const double sunOmega=(double)(.13f+5.9f*speed*speed);
  // A slower, independently paced planetary carrier makes the mechanism travel
  // without disguising the actual rotation of the gears.
  const double carrierOmega=(double)(.10f+.48f*speed);
  GearPose pose;
  pose.carrierAngle=(float)fmod(seconds*carrierOmega,(double)K_TAU);
  pose.sunAngle=(float)fmod(seconds*sunOmega,(double)K_TAU);
  // External-gear rolling in a rotating frame:
  // omega_planet = (1+Zsun/Zplanet)*omega_carrier
  //                - (Zsun/Zplanet)*omega_sun.
  // Calculate directly from absolute time to avoid a phase discontinuity every
  // time the sun angle wraps at 2*pi. The half-tooth offset preserves meshing.
  pose.satelliteAngle=(float)fmod(
    seconds*((1.0+gearRatio)*carrierOmega-gearRatio*sunOmega)
      +(double)K_PI/10.0, (double)K_TAU);
  pose.satelliteOffset={pitchGap*cosf(pose.carrierAngle),
                        pitchGap*sinf(pose.carrierAngle)};
  // A gentle two-frequency spatial trajectory; bounds allow both orbiting
  // satellites to remain in view for the usual square 64x64 matrix.
  const float t=(float)seconds;
  pose.translation={.12f*sinf(t*.24f+.4f)+.03f*sinf(t*.49f),
                    .10f*cosf(t*.18f+.7f)+.04f*sinf(t*.36f)};
  pose.scale=.83f+.035f*sinf(t*.17f);
  return pose;
}
static void mode_gpt6() {
  if (!ready() || !tick(22)) return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h), d=detail();
  constexpr float r0=.385f, r1=.275f;
  constexpr int z0=14,z1=10;
  const GearPose pose=gearPose(strip.now, SEGMENT.speed);
  // Two diametrically opposite planetary gears travel together on the carrier.
  // Teeth stay engaged at both pitch contacts throughout the revolution.
  const Point centers[3]={{0.0f,0.0f},
                          {-pose.satelliteOffset.x,-pose.satelliteOffset.y},
                          { pose.satelliteOffset.x, pose.satelliteOffset.y}};
  const float radii[3]={r0,r1,r1};
  const int teeth[3]={z0,z1,z1};
  const float angles[3]={pose.sunAngle,pose.satelliteAngle,pose.satelliteAngle};
  const float toothDepth=.034f+.008f*tune1();
  const float bore=.078f;
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    // Inverse transform keeps tooth geometry, bore radii, spoke thickness and
    // contact distances coherent while the complete assembly drifts/zooms.
    p.x=(p.x-pose.translation.x)/pose.scale;
    p.y=(p.y-pose.translation.y)/pose.scale;
    float lum=.013f, hue=0.0f;
    for (int j=0;j<3;++j) {
      float dx=p.x-centers[j].x,dy=p.y-centers[j].y;
      float r=sqrtf(dx*dx+dy*dy);
      if (r>radii[j]+toothDepth+.045f) continue;
      float a=atan2f(dy,dx)-angles[j];
      float tooth=gearTooth(a*(float)teeth[j]/K_TAU);
      float boundary=radii[j]-toothDepth*.70f+toothDepth*1.6f*tooth;
      float outer=1.0f-smooth(boundary-.010f,boundary+.018f,r);
      float edge=1.0f-smooth(.004f,.032f,fabsf(r-boundary));
      // A perforated web: five rotating spokes, hollow windows and a hub.
      float spoke=1.0f-smooth(.045f,.145f,fabsf(sinf(5.0f*a)));
      float web=smooth(.120f,.155f,r)*(1.0f-smooth(radii[j]*.63f,radii[j]*.75f,r));
      float innerRim=1.0f-smooth(.008f,.026f,fabsf(r-radii[j]*.72f));
      float hub=1.0f-smooth(.016f,.030f,fabsf(r-bore));
      float centerHole=smooth(.034f,.053f,r);
      float material=outer*centerHole*(
        (1.0f-smooth(.115f,.158f,r))*.52f + web*spoke*.75f
        +smooth(radii[j]*.70f,radii[j]*.82f,r)*(.35f+.30f*tooth));
      material+=.56f*edge + .50f*innerRim*outer + .54f*hub;
      // Pin lights and enamel glitter remain attached to each moving gear.
      const float rimPhase=a*(float)teeth[j]/K_TAU;
      const float pin=1.0f-smooth(.08f,.24f,
                                fabsf(frac(rimPhase+.5f)-.5f)*2.0f);
      const float dots=pin*(1.0f-smooth(.006f,.025f,fabsf(r-radii[j]*.86f)));
      const float fleck=1.0f-smooth(.94f,.992f,
                         sinf(a*23.0f+3.3f*j)*sinf(r*251.0f+j*10.0f));
      material+=.70f*dots + fleck*.30f*outer*d;
      material=clip(material);
      if(material>lum){lum=material;hue=35.0f+j*60.0f+20.0f*tooth+
        SEGMENT.custom2*.6f + 7.0f*sinf(a*2.0f);}
    }
    SEGMENT.setPixelColorXY(x,y,chroma(hue,lum));
  }
}
static const char m6[] PROGMEM = "GPT06 - Clockwork Cathedral@Spin + Orbit,Metal,Tooth Depth,Color;!,!;!;2;sx=150,ix=185,c1=125,c2=80";

// GPT07: Iterated folding transforms create impossible nested tessellations.
static void mode_gpt7() {
  if(!ready() || !tick(31)) return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.48f);
  float nested=2.0f+floorf(tune1()*3.0f);
  // The fold angles do not depend on pixels; compute once per frame.
  float foldCos[5], foldSin[5];
  for(int j=0;j<(int)nested;++j) {
    const float angle=t*.09f+j*.75f;
    foldCos[j]=cosf(angle); foldSin[j]=sinf(angle);
  }
  for(int y=0;y<h;++y) for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float ax=p.x,ay=p.y, ink=0.0f, hue=0.0f;
    for(int j=0;j<(int)nested;++j){
      float c=foldCos[j],s=foldSin[j];
      float xx=ax*c-ay*s,yy=ax*s+ay*c;
      xx=fabsf(frac(xx*.81f+.5f)-.5f);
      yy=fabsf(frac(yy*.81f+.5f)-.5f);
      float line=1.0f-smooth(.008f,.044f,fminf(xx,yy));
      float diagonal=1.0f-smooth(.005f,.037f,fabsf(xx-yy));
      ink=fmaxf(ink, (line*.58f+diagonal*.28f)*(1.0f-.13f*j));
      ax=xx*2.9f-.72f; ay=yy*2.9f-.72f;
      hue+=xx*13.0f+yy*24.0f;
    }
    SEGMENT.setPixelColorXY(x,y,chroma(hue*3.1f+t*13.0f+SEGMENT.custom2*.5f,clip(.03f+ink*(.55f+.40f*detail()))));
  }
}
static const char m7[] PROGMEM = "GPT07 - Impossible Lattice@Drift,Brightness,Recursion,Facet;!,!;!;2;sx=100,ix=172,c1=125";

// GPT08: Animated terrain heightfield with moving contour boundaries and escarpments.
static void mode_gpt8() {
  if(!ready() || !tick(34)) return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.33f),d=detail();
  for(int y=0;y<h;++y) for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float warp=.31f*extraSine(2.7f*p.x+1.9f*p.y+t*.48f);
    float xx=p.x+warp*tune1(), yy=p.y+.2f*extraCosine(3.3f*p.x-t*.31f);
    float field=.43f*extraSine(3.0f*xx+1.2f*yy+t*.40f)
              +.31f*extraSine(5.4f*yy-2.0f*xx-t*.23f)
              +.17f*extraSine(9.8f*xx+7.1f*yy+t*.57f)
              +.09f*extraCosine(14.0f*yy-3.0f*xx+t*.71f);
    float z=field*(2.0f+3.0f*tune2())+t*.20f;
    float iso=frac(z);
    float contour=1.0f-smooth(.010f,.11f,fminf(iso,1.0f-iso));
    float highland=clip(.35f+.45f*field);
    float bright=.07f+.63f*contour+highland*(.15f+.16f*d);
    SEGMENT.setPixelColorXY(x,y,chroma(100.0f*field+80.0f*iso+t*11.0f,bright));
  }
}
static const char m8[] PROGMEM = "GPT08 - Aether Topography@Drift,Relief,Warp,Contours;!,!;!;2;sx=110,ix=145,c1=120,c2=145";

// GPT09: a continuously deforming Julia set, rendered as luminous glass cells.
static void mode_gpt9() {
  if(!ready() || !tick(43))return;
  const int w=SEG_W,h=SEG_H;float u=unitOf(w,h)*1.18f,t=timePhase(.32f),d=detail();
  float cr=-.70f+.12f*cosf(t*.23f),ci=.23f+.12f*sinf(t*.17f);
  int depth=9+(SEGMENT.custom1>>5);
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[2]={t*.18f,t};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
  const float juliaOffset=.16f*sinf(phase[0]);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    #if GPTFX_STAGE2_PHASE_BOUNDING
    float zx=p.x+juliaOffset,zy=p.y;
#else
    float zx=p.x+.16f*sinf(t*.18f),zy=p.y;
#endif
    float trap=10.0f;int it=0;
    for(;it<depth && zx*zx+zy*zy<16.0f;++it){
      float xx=zx*zx-zy*zy+cr,yy=2.0f*zx*zy+ci;
      zx=xx;zy=yy;
      trap=fminf(trap,fabsf(zx*zy));
    }
    float escaped=(float)it/depth;
    float veins=1.0f-smooth(.008f,.065f,trap);
    float glass=.10f+.42f*escaped+.38f*veins+.17f*ridge(trap*16.0f+GPTFX_STAGE2_ARG(t,phase[1]));
    SEGMENT.setPixelColorXY(x,y,chroma(it*18.0f+trap*180.0f+t*12.0f+SEGMENT.custom2*.5f,glass*(.55f+.45f*d)));
  }
}
static const char m9[] PROGMEM="GPT09 - Mandelglass@Drift,Glass,Iterations,Prism;!,!;!;2;sx=125,ix=160";

// GPT10: moving circuit-trace board; junctions illuminate as timed signals pass.
static void mode_gpt10() {
  if(!ready() || !tick(30))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.65f);
  float scale=4.5f+6.0f*tune1();
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={-t*4.1f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
#if GPTFX_GPT10_PACKET_BOUNDING
  // The grid translates over time. Bounding only -4.1*t misses its motion.
  // Combine ALL periodic time terms for each possible trace axis once/frame.
  // Grid coordinates, floor(), hash and color selection remain UNBOUNDED.
  const float packetTimeX=(t*.11f)*(.65f*5.0f)-t*4.1f;
  const float packetTimeY=(-t*.06f)*(.65f*5.0f)-t*4.1f;
  const float packetPhaseX=stage2BoundPhase(packetTimeX);
  const float packetPhaseY=stage2BoundPhase(packetTimeY);
  // Report both the stage-2 legacy phase and the combined packet phases.
  const float packetRaw[3]={-t*4.1f,packetTimeX,packetTimeY};
  const float packetWrapped[3]={stage2BoundPhase(packetRaw[0]),packetPhaseX,packetPhaseY};
  stage2TrackPhase(packetRaw,packetWrapped,3);
#endif
#if GPTFX_GPT10_CELL_CACHE
  // Direct-mapped, frame-local cell cache. Same float seed expression and
  // exact cell coordinates as dev.14, just fewer duplicate native sinf calls.
  // 64 entries, fixed bounded stack, no persistence across frames/segments.
  struct Cell {float ix,iy,seed;bool valid;};
  Cell cells[64]={};
#endif
  GPT10Diag diag;
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float gx=p.x*scale+t*.11f,gy=p.y*scale-t*.06f;
    float ix=floorf(gx),iy=floorf(gy),fx=frac(gx)-.5f,fy=frac(gy)-.5f;
    float seed;
    ++diag.hashRequests;
#if GPTFX_GPT10_CELL_CACHE
    // The key uses integer-valued floor() outputs, without altering floats
    // passed to the original hash expression. Millis-wrap range is safe.
    uint32_t key=(uint32_t)(int32_t)ix*0x9E3779B1u ^
                 (uint32_t)(int32_t)iy*0x85EBCA77u;
    Cell& cell=cells[(key>>26)&63U];
    if(cell.valid && cell.ix==ix && cell.iy==iy){
      seed=cell.seed;
      ++diag.hashCacheHits;
    }else{
      float hashArg=ix*127.1f+iy*311.7f;
      seed=frac(sinf(hashArg)*43758.5453f);
      cell.ix=ix;cell.iy=iy;cell.seed=seed;cell.valid=true;
      ++diag.hashEvaluations;
      diag.hashArgAbs=fmaxf(diag.hashArgAbs,fabsf(hashArg));
    }
#else
    float hashArg=ix*127.1f+iy*311.7f;
    seed=frac(sinf(hashArg)*43758.5453f);
    ++diag.hashEvaluations;
    diag.hashArgAbs=fmaxf(diag.hashArgAbs,fabsf(hashArg));
#endif
    float hor=1.0f-smooth(.038f,.13f,fabsf(fy)),ver=1.0f-smooth(.038f,.13f,fabsf(fx));
    float trace=(seed>.34f?hor:ver);
    float joint=(1.0f-smooth(.06f,.2f,sqrtf(fx*fx+fy*fy)));
    float lineCoord=(seed>.34f?gx:gy)*.65f;
#if GPTFX_GPT10_PACKET_BOUNDING
    const float spatial=(seed>.34f?p.x:p.y)*scale*(.65f*5.0f);
    const float packetArg=spatial+(seed>.34f?packetPhaseX:packetPhaseY)+seed*8.0f;
    // Diagnostic sampling: 1/32 pixels to avoid affecting the hot loop.
    // Values in JSON are sample maxima, not exhaustive per-frame maxima.
    if((x&7)==0 && (y&3)==0){
      const float originalPacketArg=lineCoord*5.0f+
                         GPTFX_STAGE2_ARG(-t*4.1f,phase[0])+seed*8.0f;
      const float rawAbs=fabsf(originalPacketArg),boundedAbs=fabsf(packetArg);
      if(rawAbs>diag.packetRawAbs)diag.packetRawAbs=rawAbs;
      if(boundedAbs>diag.packetBoundedAbs)diag.packetBoundedAbs=boundedAbs;
    }
#else
    const float packetArg=lineCoord*5.0f+GPTFX_STAGE2_ARG(-t*4.1f,phase[0])+seed*8.0f;
    if((x&7)==0 && (y&3)==0){
      const float argAbs=fabsf(packetArg);
      if(argAbs>diag.packetRawAbs)diag.packetRawAbs=argAbs;
      diag.packetBoundedAbs=diag.packetRawAbs;
    }
#endif
    float packet=ridge(packetArg);
    float lum=.022f+trace*(.14f+.65f*packet)+joint*(.25f+.45f*detail());
    SEGMENT.setPixelColorXY(x,y,chroma(seed*210.0f+packet*48.0f+SEGMENT.custom2*.35f,lum));
  }
  gpt10Diag=diag;
}
static const char m10[] PROGMEM="GPT10 - Glass Circuit@Signal,Energy,Density,Hue;!,!;!;2;sx=120,ix=170,c1=100";

// GPT11: luminous woven fibers, with alternating over-under crossings.
static void mode_gpt11() {
  if(!ready() || !tick(31))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.46f),freq=5.0f+7.0f*tune1();
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[3]={t,-t*.71f,t*.3f};
  const float phase[3]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2])};
  stage1TrackPhase(raw,phase,3);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float dx=p.x+.16f*sinf(3.2f*p.y+GPTFX_STAGE1_ARG(t,phase[0])),
          dy=p.y+.16f*cosf(3.6f*p.x+GPTFX_STAGE1_ARG(-t*.71f,phase[1]));
    float warp=fabsf(sinf(freq*dx)),weft=fabsf(sinf(freq*dy));
    float hline=1.0f-smooth(.04f,.22f,warp),vline=1.0f-smooth(.04f,.22f,weft);
    float over=sinf(freq*(dx+dy)*.50f+GPTFX_STAGE1_ARG(t*.3f,phase[2]));
    float light=.025f+(over>0.0f?(.80f*hline+.27f*vline):(.80f*vline+.27f*hline));
    light+=.19f*hline*vline*(.5f+.5f*detail());
    SEGMENT.setPixelColorXY(x,y,chroma(50.0f*over+dx*40.0f+dy*42.0f+t*8.0f+SEGMENT.custom2*.3f,light));
  }
}
static const char m11[] PROGMEM="GPT11 - Aurora Loom@Flow,Shimmer,Threads,Color;!,!;!;2;sx=125,ix=170,c1=120";

// GPT12: contour map of a time-varying 3-body gravitational potential.
static void mode_gpt12() {
  if(!ready() || !tick(38))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.25f);
  Point bodies[3];
  for(int k=0;k<3;++k){float a=t*(.32f+.11f*k)+K_TAU*k/3;
    bodies[k]={(.38f+.13f*k)*cosf(a),(.38f+.08f*k)*sinf(a)};}
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);float potential=0.0f,nearest=3.0f;
    for(int k=0;k<3;++k){float dx=p.x-bodies[k].x,dy=p.y-bodies[k].y;
      float r=sqrtf(dx*dx+dy*dy);nearest=fminf(nearest,r);
      potential+=(.13f+.045f*k)/(r+.07f);}
    float lev=frac(potential*(3.0f+7.0f*tune1())-t*.09f);
    float lines=1.0f-smooth(.015f,.12f,fminf(lev,1.0f-lev));
    float core=1.0f-smooth(.01f,.09f,nearest);
    float glow=.025f+.71f*lines+.76f*core+.12f*ridge(potential*6.0f);
    SEGMENT.setPixelColorXY(x,y,chroma(potential*43.0f+t*8.0f+SEGMENT.custom2*.6f,glow));
  }
}
static const char m12[] PROGMEM="GPT12 - Lagrange Ballet@Orbit,Field,Contours,Hue;!,!;!;2;sx=130,ix=155";

// GPT13: continuous bent crack families carve up a crystalline surface.
static void mode_gpt13() {
  if(!ready() || !tick(32))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.33f);
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[5]={t*.31f,-t*.43f,t*.42f,-t*.34f,t};
  const float phase[5]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2]),
                        stage1BoundPhase(raw[3]),stage1BoundPhase(raw[4])};
  stage1TrackPhase(raw,phase,5);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float f=p.x+.11f*sinf(5.2f*p.y+GPTFX_STAGE1_ARG(t*.31f,phase[0])),
          g=p.y+.14f*sinf(4.8f*p.x+GPTFX_STAGE1_ARG(-t*.43f,phase[1]));
    float crackX=frac(4.2f*f+.37f*sinf(6.2f*g+GPTFX_STAGE1_ARG(t*.42f,phase[2])));
    float crackY=frac(3.7f*g+.24f*cosf(6.0f*f+GPTFX_STAGE1_ARG(-t*.34f,phase[3])));
    float seamX=1.0f-smooth(.01f,.09f,fminf(crackX,1.0f-crackX));
    float seamY=1.0f-smooth(.01f,.09f,fminf(crackY,1.0f-crackY));
    float facet=frac(floorf(4.2f*f)*.618f+floorf(3.7f*g)*.382f);
    float gold=fmaxf(seamX,seamY);
    float b=.07f+.30f*facet+.58f*gold+.08f*ridge(8.0f*f+GPTFX_STAGE1_ARG(t,phase[4]));
    SEGMENT.setPixelColorXY(x,y,chroma(125.0f*facet+SEGMENT.custom2*.56f+t*5.0f,b));
  }
}
static const char m13[] PROGMEM="GPT13 - Chromatic Kintsugi@Drift,Seams,Fracture,Gold;!,!;!;2;sx=120,ix=180";

// GPT14: infinite ascending corridor with depth-perspective tiled beams.
static void mode_gpt14() {
  if(!ready() || !tick(33))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.58f);
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={t*.7f+t*.4f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float z=1.0f/(.21f+fabsf(p.y+.07f));
    float xx=p.x*z,yy=(p.y<0?-1.0f:1.0f)*z+t*.7f;
    float stripeX=frac(xx*(.48f+.4f*tune1()));
    float stripeY=frac(yy*.53f);
    float vx=1.0f-smooth(.012f,.07f,fminf(stripeX,1.0f-stripeX));
    float vy=1.0f-smooth(.014f,.09f,fminf(stripeY,1.0f-stripeY));
    float vanish=1.0f-smooth(.01f,.20f,fabsf(p.y));
    float lum=.025f+.48f*fmaxf(vx,vy)+.24f*vanish+.18f*ridge(GPTFX_STAGE2_ARG(yy+t*.4f,
       (p.y<0?-1.0f:1.0f)*z+phase[0]));
    SEGMENT.setPixelColorXY(x,y,chroma(yy*9.0f+xx*6.0f+SEGMENT.custom2*.60f,lum));
  }
}
static const char m14[] PROGMEM="GPT14 - Phantom Elevator@Ascent,Perspective,Columns,Hue;!,!;!;2;sx=130,ix=155";

// GPT15: visualized ferrofluid field: attraction poles yield spikes and arcs.
static void mode_gpt15() {
  if(!ready() || !tick(34))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.37f);
  Point north={.55f*cosf(t*.42f),.55f*sinf(t*.42f)};
  Point south={-.57f*cosf(t*.34f+.4f),-.52f*sinf(t*.34f+.4f)};
#if GPTFX_STAGE2B_PHASE_BOUNDING
  const float raw[1]={-t*.42f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float ax=p.x-north.x,ay=p.y-north.y,bx=p.x-south.x,by=p.y-south.y;
    float da=sqrtf(ax*ax+ay*ay)+.04f,db=sqrtf(bx*bx+by*by)+.04f;
    float potential=1.0f/da-1.0f/db;
    float vector=atan2f(ay,ax)-atan2f(by,bx);
    float spikes=ridge(vector*(5.0f+9.0f*tune1())+potential*.65f+GPTFX_STAGE2B_ARG(-t*.42f,phase[0]));
    float iso=frac(potential*.18f+t*.13f);
    float contour=1.0f-smooth(.01f,.11f,fminf(iso,1.0f-iso));
    float metal=clip(.04f+.40f*spikes+.45f*contour + .17f/(1.0f+da*db*20.0f));
    SEGMENT.setPixelColorXY(x,y,chroma(120.0f*potential+30.0f*spikes+t*9.0f+SEGMENT.custom2*.4f,metal));
  }
}
static const char m15[] PROGMEM="GPT15 - Ferrofluid Theatre@Orbit,Magnetism,Spikes,Color;!,!;!;2;sx=120,ix=155";

// GPT16: counterwinding complex logarithmic streamlines with hollow centers.
static void mode_gpt16() {
  if(!ready() || !tick(30))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.48f);
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[2]={-t*1.6f,t*1.6f};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r0=sqrtf((p.x+.40f)*(p.x+.40f)+p.y*p.y)+.03f;
    float r1=sqrtf((p.x-.40f)*(p.x-.40f)+p.y*p.y)+.03f;
    float a0=atan2f(p.y,p.x+.40f),a1=atan2f(p.y,p.x-.40f);
    float l0=ridge(8.0f*a0+8.0f*logf(r0)+GPTFX_STAGE2_ARG(-t*1.6f,phase[0]));
    float l1=ridge(8.0f*a1-8.0f*logf(r1)+GPTFX_STAGE2_ARG(t*1.6f,phase[1]));
    float interference=1.0f-smooth(.01f,.13f,fabsf(l0-l1));
    float v=.025f+.42f*l0+.42f*l1+.20f*interference*detail();
    SEGMENT.setPixelColorXY(x,y,chroma((a0-a1)*44.0f+(r0+r1)*30.0f+t*9.0f+SEGMENT.custom2*.45f,v));
  }
}
static const char m16[] PROGMEM="GPT16 - Riemann Current@Drift,Energy,Vortices,Prism;!,!;!;2;sx=120,ix=160";

// GPT17: moving sky-chart: rotating points and dynamically connected chords.
static inline float segmentDistance(Point p, Point a, Point b) {
  float dx=b.x-a.x,dy=b.y-a.y,mag=dx*dx+dy*dy;
  float q=(mag>.000001f)?clip(((p.x-a.x)*dx+(p.y-a.y)*dy)/mag):0.0f;
  float x=p.x-a.x-q*dx,y=p.y-a.y-q*dy;
  return sqrtf(x*x+y*y);
}
static void mode_gpt17() {
  if(!ready() || !tick(39))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.42f);
  constexpr int N=11;Point stars[N];
  for(int i=0;i<N;++i){float a=K_TAU*i/N+t*(.11f+(i%3)*.07f);
    float r=.31f+.57f*(.5f+.5f*sinf(i*1.8f+t*.24f));
    stars[i]={r*cosf(a),r*sinf(a)};}
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);float light=.016f,hue=0;
    for(int i=0;i<N;++i){
      float dx=p.x-stars[i].x,dy=p.y-stars[i].y;
      float point=1.0f-smooth(.015f,.070f,sqrtf(dx*dx+dy*dy));
      float line=1.0f-smooth(.007f,.035f,segmentDistance(p,stars[i],stars[(i+3)%N]));
      light=fmaxf(light,point*.94f+line*(.30f+.30f*detail()));
      hue+=point*(i*22.0f+t*8.0f)+line*(4.0f+i*.3f);
    }
    SEGMENT.setPixelColorXY(x,y,chroma(hue+SEGMENT.custom2*.65f,light));
  }
}
static const char m17[] PROGMEM="GPT17 - Constellation Engine@Orbit,Stars,Web,Color;!,!;!;2;sx=145,ix=170";

// GPT18: nested mirrored corridors carved from a drifting signed grid.
static void mode_gpt18() {
  if(!ready() || !tick(35))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.27f);
  float foldOffset[4], wavePhase[4];
  float peakRaw=0.0f, peakBound=0.0f;
  for(int k=0;k<4;++k) {
    const float foldRaw=t*.38f+k;
    const float waveRaw=t*(.4f+.1f*k);
    const float foldBound=labyrinthBoundPhase(foldRaw);
    const float waveBound=labyrinthBoundPhase(waveRaw);
    foldOffset[k]=.08f*sinf(foldBound);
    wavePhase[k]=waveBound;
    peakRaw=fmaxf(peakRaw,fmaxf(fabsf(foldRaw),fabsf(waveRaw)));
    peakBound=fmaxf(peakBound,fmaxf(fabsf(foldBound),fabsf(waveBound)));
  }
  extraPerf.phaseRawAbs=peakRaw;
  extraPerf.phaseWrappedAbs=peakBound;
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float xx=p.x,yy=p.y,glow=0.0f,energy=0.0f;
    for(int k=0;k<4;++k){
      float f=2.4f+(float)k*.85f;
      float wave=labyrinthSine(xx*f+yy*(1.7f+.3f*k)+wavePhase[k]);
      float stroke=1.0f-smooth(.02f,.11f,fabsf(wave));
      glow=fmaxf(glow,stroke*(1.0f-.14f*k));
      energy+=stroke*.17f;
      xx=fabsf(xx)-(.40f-.06f*k)+foldOffset[k];
      yy=fabsf(yy)-(.40f-.07f*k);
    }
    float lum=.02f+.70f*glow+.30f*energy*detail();
    SEGMENT.setPixelColorXY(x,y,chroma((xx-yy)*87.0f+glow*75.0f+t*7.0f+SEGMENT.custom2*.36f,lum));
  }
}
static const char m18[] PROGMEM="GPT18 - Vitreous Labyrinth@Mutation,Glow,Folds,Color;!,!;!;2;sx=125,ix=165";

// GPT19: animated astronomical instrument with nested rune-bearing dials.
static void mode_gpt19() {
  if(!ready() || !tick(33))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.53f);
#if GPTFX_STAGE1_PHASE_BOUNDING
  float raw[5],phase[5];
  for(int k=1;k<=4;++k){
    raw[k-1]=(8.0f+k*3.0f)*(k&1?1.0f:-1.0f)*t*(.25f+.12f*k);
    phase[k-1]=stage1BoundPhase(raw[k-1]);
  }
  raw[4]=-t*.64f;
  phase[4]=stage1BoundPhase(raw[4]);
  stage1TrackPhase(raw,phase,5);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float a=atan2f(p.y,p.x),r=sqrtf(p.x*p.x+p.y*p.y);
    float v=.025f,shade=0.0f;
    for(int k=1;k<=4;++k){
      float radius=k*.225f;
#if !GPTFX_STAGE1_PHASE_BOUNDING
      float an=a+(k&1?1.0f:-1.0f)*t*(.25f+.12f*k);
#endif
      float dial=1.0f-smooth(.012f,.044f,fabsf(r-radius));
      float marks=1.0f-smooth(.025f,.20f,fabsf(sinf(GPTFX_STAGE1_ARG(
          (8.0f+k*3.0f)*an,(8.0f+k*3.0f)*a+phase[k-1]))));
      float glyph=marks*(1.0f-smooth(.017f,.045f,fabsf(r-(radius+.054f))));
      v+=dial*.41f+glyph*.58f;
      shade+=dial*k*.05f;
    }
    float pointer=1.0f-smooth(.007f,.037f,fabsf(sinf(a+GPTFX_STAGE1_ARG(-t*.64f,phase[4]))));
    v+=pointer*.23f*(1.0f-smooth(.75f,1.0f,r));
    SEGMENT.setPixelColorXY(x,y,chroma(r*88.0f+shade*110.0f+t*6.0f+SEGMENT.custom2*.52f,v));
  }
}
static const char m19[] PROGMEM="GPT19 - Celestial Runewheel@Winding,Shine,Marks,Prism;!,!;!;2;sx=135,ix=165";

// GPT20: a 5-axis quasiperiodic projection creates wandering bright stars.
static void mode_gpt20() {
  if(!ready() || !tick(34))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.32f);
  float axisCos[5], axisSin[5], phaseTime[5];
  for(int k=0;k<5;++k) {
    const float a=(float)k*K_TAU/5.0f;
    axisCos[k]=cosf(a); axisSin[k]=sinf(a);
    phaseTime[k]=t*(.37f+.08f*k)*(k&1?-1.0f:1.0f);
  }
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float sum=0.0f,product=1.0f;
    for(int k=0;k<5;++k){
      float phase=7.0f*(p.x*axisCos[k]+p.y*axisSin[k])+phaseTime[k];
      float wave=extraCosine(phase);sum+=wave;
      product*=.78f+.22f*wave;
    }
    float star=1.0f-smooth(.35f,1.65f,fabsf(sum-3.7f));
    float near=1.0f-smooth(.0f,.22f,fabsf(product-.79f));
    float lum=.02f+.68f*star+.29f*near+.12f*ridge(sum*2.1f);
    SEGMENT.setPixelColorXY(x,y,chroma(sum*42.0f+product*145.0f+t*10.0f+SEGMENT.custom2*.55f,lum));
  }
}
// Experimental GPT21/GPT22: sparse, distance-phased ripple on an adaptive dot lattice.
// No full-frame allocation; black space between cells is intentional.
// Shared temporal and radial wave, with two different light representations.
static void mode_sparse_ripple(bool variableSize) {
  if (!ready() || !tick(30)) return;
  const int w=SEG_W, h=SEG_H;
  const float shortSide=(float)(w<h?w:h);
  // At 64x64 the pitch is six pixels, leaving substantial negative space.
  // Custom1: Dot Spacing. 128 preserves the original 6-pixel baseline on 64x64.
  // Shared by both variants for a controlled geometry comparison.
  const float pitch=fmaxf(3.0f, (shortSide/10.7f) * (0.65f + 0.70f*tune1()));
  const float t=timePhase(1.15f);
  const float contrast=detail();
  const float phase=t*3.0f;
  const float centerX=((float)w-1.0f)*.5f;
  const float centerY=((float)h-1.0f)*.5f;
  const float baseRadius=variableSize ? .40f : .57f;
  const float radiusCap=variableSize ? fminf(2.10f,pitch*.35f) : baseRadius;
  // Expansion controls the geometric radius, not brightness.
  const float maxRadius=variableSize ? baseRadius+(radiusCap-baseRadius)*contrast : baseRadius;
  const float outerRadius=maxRadius+.75f;
  const float ringWave=K_TAU/3.0f;
  const float tint=SEGMENT.custom2*.50f;
  // Clear every frame: a small dot must not leave a trail when it shrinks.
  SEGMENT.fill(0);
  const int cols=(int)ceilf((float)w/pitch)+2;
  const int rows=(int)ceilf((float)h/pitch)+2;
  for(int j=0;j<rows;++j) for(int i=0;i<cols;++i) {
    const float cx=centerX+((float)i-(float)(cols-1)*.5f)*pitch;
    const float cy=centerY+((float)j-(float)(rows-1)*.5f)*pitch;
    if(cx < -outerRadius || cy < -outerRadius || cx > w-1+outerRadius || cy > h-1+outerRadius) continue;
    const float dist=sqrtf(squaref(cx-centerX)+squaref(cy-centerY))/pitch;
    const float wave=.5f+.5f*cosf(dist*ringWave-phase);
    const float envelope=wave*wave*(3.0f-2.0f*wave);
    const float radius=variableSize ? baseRadius+(maxRadius-baseRadius)*envelope : baseRadius;
    const float peak=variableSize ? .90f : (.10f+(.85f*contrast)*envelope);
    const float hue=dist*10.0f+phase*7.0f+tint;
    const int x0=(int)floorf(cx-radius-1.0f), x1=(int)ceilf(cx+radius+1.0f);
    const int y0=(int)floorf(cy-radius-1.0f), y1=(int)ceilf(cy+radius+1.0f);
    for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x) {
      if(x<0 || y<0 || x>=w || y>=h) continue;
      // Pixel-center coverage approximates subpixel dots without aliasing.
      const float dx=(float)x-cx, dy=(float)y-cy;
      const float d=sqrtf(dx*dx+dy*dy);
      const float coverage=clip(radius+.55f-d);
      if(coverage>0.0f) SEGMENT.setPixelColorXY(x,y,chroma(hue,peak*coverage));
    }
  }
}
static void mode_gpt21() { mode_sparse_ripple(false); }
static void mode_gpt22() { mode_sparse_ripple(true); }
static const char m21[] PROGMEM="GPT21 - Sparse Ripple Brightness@Wave Speed,Contrast,Dot Spacing,Color;!,!;!;2;sx=135,ix=205,c1=128";
static const char m22[] PROGMEM="GPT22 - Sparse Ripple Size@Wave Speed,Expansion,Dot Spacing,Color;!,!;!;2;sx=135,ix=205,c1=128";


// GPT23: crossed light fibers with traveling, independently phase-shifted pulses.
// Sparse luminous threads alternate over/under at every crossing.
static void mode_gpt23() {
  if (!ready() || !tick(32)) return;
  const int w=SEG_W,h=SEG_H;
  const float t=timePhase(.75f), density=5.0f+5.0f*tune1();
  const float u=unitOf(w,h), energy=.25f+.75f*detail();
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[4]={t*.8f,-t*.7f,-t*2.8f,t*2.5f};
  const float phase[4]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),
                        stage1BoundPhase(raw[2]),stage1BoundPhase(raw[3])};
  stage1TrackPhase(raw,phase,4);
#endif
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    const float vx=p.x*density,vy=p.y*density;
    const float ix=floorf(vx+.5f),iy=floorf(vy+.5f);
    const float dx=fabsf(vx-ix),dy=fabsf(vy-iy);
    const float warp=.11f*sinf(vy*1.2f+GPTFX_STAGE1_ARG(t*.8f,phase[0])+ix*.73f);
    const float hThread=1.0f-smooth(.055f,.19f,fabsf(vy-iy+warp));
    const float vThread=1.0f-smooth(.055f,.19f,fabsf(vx-ix+.11f*sinf(vx*.9f+GPTFX_STAGE1_ARG(-t*.7f,phase[1])+iy*.67f)));
    const float hPulse=ridge(vx*1.85f+GPTFX_STAGE1_ARG(-t*2.8f,phase[2])+iy*2.1f);
    const float vPulse=ridge(vy*1.85f+GPTFX_STAGE1_ARG(t*2.5f,phase[3])+ix*1.7f);
    const bool over=(int(fabsf(ix)+fabsf(iy))&1)!=0;
    const float crossing=(1.0f-smooth(.08f,.29f,dx))*(1.0f-smooth(.08f,.29f,dy));
    const float lum=clip((hThread*(.09f+.7f*hPulse)+vThread*(.09f+.7f*vPulse))*.65f*energy + crossing*(over?hPulse:vPulse)*.30f);
    SEGMENT.setPixelColorXY(x,y,chroma(18.0f*ix+29.0f*iy+t*12.0f+SEGMENT.custom2*.35f,lum));
  }
}
static const char m23[] PROGMEM="GPT23 - Photon Loom@Pulse Speed,Glow,Thread Density,Hue;!,!;!;2;sx=145,ix=190";

// GPT24: layered hypotrochoid-like luminous traces, moving in opposite directions.
// Uses distance to parametric orbit samples, not a precomputed image or frame buffer.
// Algebraic common-subexpression elimination: each pixel has one polar angle
// and one radius, reused by both rosettes, core glow and palette mapping.
// dev.9 keeps the dev.8 geometry but bounds time-dependent trigonometric
// arguments once per frame. Avoid large-input argument reduction in sinf/cosf
// without changing the palette drift or the time scaling of the effect.
// Reduce the *time-only* contribution of each oscillation before entering
// the 64x64 pixel loop. The spatial terms stay untouched. fmodf is invoked
// only five times per frame, not once per pixel. Negative phases are valid.
// The two layer oscillations have different frequencies, and the secondary
// sine must be reduced at its own combined frequency (0.91*1.7+0.4 etc.).
static inline float rosetteRidge(float v) {
  const float a = 1.0f - fabsf(gptfx_trig::sine(v));
  return a * a * a * a;
}
static inline float rosettePhase(float angle) {
  return fmodf(angle, K_TAU);
}
static void mode_gpt24() {
  if (!ready() || !tick(40)) return;
  const int w=SEG_W,h=SEG_H;
  const float scale=0.70f + 1.04f*tune2();
  const float u=unitOf(w,h)/scale,t=timePhase(.6f),d=detail();
  const float petals=4.0f+floorf(tune1()*6.0f);
  const float rawTimes[5]={
    t*.91f, t*-.72f,
    t*(.91f*1.7f+.4f), t*(-.72f*1.7f+.4f), t*3.0f
  };
  const float baseTime[2]={rosettePhase(rawTimes[0]),rosettePhase(rawTimes[1])};
  const float detailTime[2]={rosettePhase(rawTimes[2]),rosettePhase(rawTimes[3])};
  const float sparkTime=rosettePhase(rawTimes[4]);
  if(gpt24Tracking) {
    float peakRaw=0.0f,peakWrapped=0.0f;
    const float reduced[5]={baseTime[0],baseTime[1],detailTime[0],detailTime[1],sparkTime};
    for(int i=0;i<5;++i) {
      peakRaw=fmaxf(peakRaw,fabsf(rawTimes[i]));
      peakWrapped=fmaxf(peakWrapped,fabsf(reduced[i]));
    }
    gpt24Perf.phaseRawAbs=peakRaw;
    gpt24Perf.phaseWrappedAbs=peakWrapped;
  }
  for(int y=0;y<h;++y) {
    const uint32_t rowStartUs = gpt24Tracking ? micros() : 0U;
    for(int x=0;x<w;++x) {
      Point p=normal(x,y,u,w*.5f,h*.5f);
      const float a=atan2f(p.y,p.x);
      const float r=sqrtf(p.x*p.x+p.y*p.y);
      float best=10.0f,bright=0.0f;
      for(int layer=0;layer<2;++layer) {
        const float spatialPhase=a*(petals+(float)layer);
        const float phase=spatialPhase+baseTime[layer];
        const float target=.48f+.16f*gptfx_trig::cosine(phase)
                           +.075f*gptfx_trig::sine(spatialPhase*1.7f+detailTime[layer]);
        const float dist=fabsf(r-target);
        if(dist<best) best=dist;
        const float thread=1.0f-smooth(.012f,.045f+.018f*d,dist);
        const float spark=rosetteRidge(a*22.0f+(layer?sparkTime:-sparkTime));
        bright+=thread*(.20f+.48f*spark);
      }
      const float core=(1.0f-smooth(.025f,.12f,r))*.26f;
      const float halo=(1.0f-smooth(.04f,.17f,best))*.11f;
      SEGMENT.setPixelColorXY(x,y,chroma(a*35.0f+t*18.0f,clip((bright+core+halo)*(.45f+.55f*d))));
    }
    // One pair of clock reads per rendered row, never per pixel.
    if (gpt24Tracking) {
      const uint32_t rowUs=(uint32_t)(micros()-rowStartUs);
      gpt24Perf.rowTotalUs += rowUs;
      ++gpt24Perf.rowSamples;
      if(rowUs>gpt24Perf.rowMaxUs) gpt24Perf.rowMaxUs=rowUs;
    }
  }
}
// Wrapper keeps the existing renderer unchanged and measures only actual draws.
static void mode_gpt24_profiled() {
  ++gpt24Perf.calls;
  gpt24TickReached = false;
  gpt24TickSkipped = false;
  gpt24Tracking = true;
  const int8_t startCore = currentCore();
  const uint32_t startUs = micros();
  mode_gpt24();
  const uint32_t elapsedUs = (uint32_t)(micros() - startUs);
  const int8_t endCore = currentCore();
  gpt24Tracking = false;
  if (gpt24TickSkipped) {
    ++gpt24Perf.skipped;
  } else if (gpt24TickReached) {
    ++gpt24Perf.rendered;
    gpt24Perf.renderTotalUs += elapsedUs;
    if (elapsedUs > gpt24Perf.renderMaxUs) gpt24Perf.renderMaxUs = elapsedUs;
    gpt24Perf.renderLastUs = elapsedUs;
    gpt24Perf.recentUs[gpt24Perf.recentWrite] = elapsedUs;
    gpt24Perf.recentWrite = (uint8_t)((gpt24Perf.recentWrite + 1U) % GPT24Perf::RECENT_COUNT);
    if(gpt24Perf.recentValid < GPT24Perf::RECENT_COUNT) ++gpt24Perf.recentValid;
    gpt24Perf.coreStart = startCore;
    gpt24Perf.coreEnd = endCore;
    if(startCore >= 0 && endCore >= 0 && startCore != endCore) ++gpt24Perf.coreChanges;
    gpt24Perf.width = SEG_W;
    gpt24Perf.height = SEG_H;
  }
}
static const char m24[] PROGMEM="GPT24 - Kinetic Rosettes@Orbit Speed,Trace Glow,Petals,Size;!,!;!;2;sx=135,ix=190,c2=128";

// GPT25: an array of individual glyphs bending in a traveling tidal field.
// Alternating open arcs and square corners preserve negative space between cells.
static void mode_gpt25() {
  if (!ready() || !tick(35)) return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.55f),cells=4.0f+floorf(tune1()*5.0f);
  const float d=detail();
#if GPTFX_STAGE2B_PHASE_BOUNDING
  const float raw[2]={-t*1.8f,t*.7f};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
#endif
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    const float gx=p.x*cells,gy=p.y*cells;
    const float ix=floorf(gx+.5f),iy=floorf(gy+.5f);
    const float px=gx-ix,py=gy-iy;
    const float tidal=sinf(ix*.64f+iy*.83f+GPTFX_STAGE2B_ARG(-t*1.8f,phase[0]));
    const float theta=.52f*tidal;
    const float xx=px*cosf(theta)-py*sinf(theta);
    const float yy=px*sinf(theta)+py*cosf(theta);
    const float r=sqrtf(xx*xx+yy*yy),angle=atan2f(yy,xx);
    const float arc=1.0f-smooth(.035f,.10f,fabsf(r-(.24f+.045f*tidal)));
    const float opening=smooth(-.25f,.20f,sinf(angle*2.0f+GPTFX_STAGE2B_ARG(t*.7f,phase[1])+ix+iy));
    const float corners=1.0f-smooth(.035f,.09f,fabsf(fmaxf(fabsf(xx),fabsf(yy))-.28f));
    const float shape=(int(fabsf(ix)+fabsf(iy))&1)?arc*opening:corners*(1.0f-opening*.7f);
    const float pulse=.30f+.70f*(.5f+.5f*tidal);
    const float lum=clip(shape*pulse*(.3f+.7f*d));
    SEGMENT.setPixelColorXY(x,y,chroma(ix*24.0f+iy*13.0f+t*9.0f+SEGMENT.custom2*.4f,lum));
  }
}
static const char m25[] PROGMEM="GPT25 - Tidal Glyphs@Tide Speed,Brightness,Grid Density,Hue;!,!;!;2;sx=125,ix=195";

// GPT26 - Iridescent Membranes (isolated visual prototype).
// A flowing thin-film field: a low-frequency warped cellular membrane,
// multiple colored interference bands and sparse bright boundary highlights.
// All time-driven trig arguments are bounded once/frame, not per pixel.
static void mode_gpt26() {
  if (!ready() || !tick(33)) return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.54f);
  const float flow=.36f+.74f*detail();
  const float density=1.6f+1.9f*tune1();
  const float chromatic=SEGMENT.custom2*.45f;
  const float phase[7]={
    stage2BoundPhase(t*.34f),stage2BoundPhase(t*.47f),
    stage2BoundPhase(-t*.29f),stage2BoundPhase(t*.23f),
    stage2BoundPhase(t*.54f),stage2BoundPhase(-t*.62f),
    stage2BoundPhase(t*.17f)
  };
  for(int y=0;y<h;++y) for(int x=0;x<w;++x){
    const Point p=normal(x,y,u,w*.5f,h*.5f);
    // Smooth, bounded domain distortion yields living rather than rigid cells.
    const float wx=p.x+.16f*sinf(2.7f*p.y+phase[0])+.10f*sinf(3.5f*p.x+1.9f*p.y+phase[1]);
    const float wy=p.y+.16f*sinf(2.9f*p.x+phase[2])+.09f*cosf(2.6f*p.y-1.4f*p.x+phase[3]);
    // Three overlapping curvilinear sheets, not a hard Voronoi edge.
    const float field=.54f*sinf(wx*(3.6f*density)+.36f*sinf(wy*5.2f+phase[4]))
                   +.32f*sinf(wy*(3.4f*density)-wx*1.8f+phase[5])
                   +.14f*cosf((wx+wy)*6.1f+phase[6]);
    // A narrow iridescent boundary with more restrained translucent interiors.
    const float edge=1.0f-smooth(.045f,.22f,fabsf(field));
    const float fringe=1.0f-smooth(.12f,.47f,fabsf(field));
    const float streak=.5f+.5f*sinf(field*15.0f+wx*1.5f-wy*1.1f+phase[4]);
    const float depth=.5f+.5f*cosf(2.6f*wx+2.1f*wy+field*3.4f+phase[5]);
    float lum=.025f+.22f*depth+.30f*fringe+.48f*edge*(.70f+.30f*streak);
    lum=clip(lum*flow);
    // Hue changes across film thickness; no post-render RGB enhancement.
    SEGMENT.setPixelColorXY(x,y,chroma(105.0f+58.0f*field+30.0f*depth+76.0f*wx-64.0f*wy+chromatic+fmodf(t*(.17f*12.0f),256.0f),lum));
  }
}
static const char m26[] PROGMEM="GPT26 - Iridescent Membranes@Flow,Glow,Film Scale,Hue;!,!;!;2;sx=125,ix=185,c1=135,c2=128";



// GPT27 - Crystal Recursion. Nested triangular crystal facets, sharp
// refractive ridges and tiny sparks. No framebuffer or heap allocations.
static void mode_gpt27(){
  if(!ready() || !tick(31))return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.62f),d=detail();
  const float a=stage2BoundPhase(t*.19f),b=stage2BoundPhase(-t*.27f);
  const float c=stage2BoundPhase(t*.39f), k=3.0f+2.5f*tune1();
  const float geometricSpin=stage2BoundPhase(t*(.19f*.16f));
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float px=p.x+.075f*sinf(p.y*3.8f+a),py=p.y+.075f*cosf(p.x*3.7f+b);
    const float r=sqrtf(px*px+py*py),theta=atan2f(py,px);
    const float spin=theta+.24f*sinf(2.6f*r+c)+geometricSpin;
    const float tri=cosf(3.0f*spin);
    const float fac=r*(1.0f+.26f*tri);
    const float layer=fac*k-.22f*sinf(spin*6.0f+b);
    const float cell=frac(layer);
    const float groove=1.0f-smooth(.018f,.115f,fminf(cell,1.0f-cell));
    const float facets=powf(.5f+.5f*cosf(spin*6.0f+c),2.5f);
    const float prism=.5f+.5f*sinf(9.0f*r-3.0f*spin+b);
    const float core=1.0f-smooth(.08f,.28f,r);
    const float sparkle=powf(fmaxf(0.0f,sinf(33.0f*spin+23.0f*fac+c)),24.0f);
    const float lum=clip(.025f+.20f*facets+.23f*prism*facets+.55f*groove*(.55f+.45f*prism)+.42f*core+.26f*sparkle*d);
    const float hue=30.0f+68.0f*facets+32.0f*prism+fac*93.0f+SEGMENT.custom2*.78f;
    SEGMENT.setPixelColorXY(x,y,chroma(hue,lum*(.52f+.48f*d)));
  }
}
static const char m27[] PROGMEM="GPT27 - Crystal Recursion@Rotation,Brilliance,Crystal Layers,Color;!,!;!;2;sx=115,ix=190,c1=150,c2=120";

// GPT28: Luminous Mycelium, row-wise segment rasterization.
// Distances and brightness retain the preview.3 formula. Precompute all
// segment geometry, then rasterize only each segment's bounding rectangle.
// Fixed row scratch (max 64 px per chunk); no heap/PSRAM allocation.
struct MycSeg{float x0,y0,x1,y1,seed;};
struct MycRasterSeg{float x0,y0,dx,dy,inv,seed;int xmin,xmax,ymin,ymax;};
// The two 108-element geometry tables are too large for a WLED effect
// callback stack. Allocate their workspace once on the heap, reuse on each
// frame, and bail out safely if the allocation is unavailable.
struct MycWorkspace { MycSeg lines[108]; MycRasterSeg raster[108]; };
static MycWorkspace* mycWorkspace = nullptr;
static bool mycWorkspaceAllocFailed = false;
static void mode_gpt28(){
  if(!mycWorkspace){
    if(mycWorkspaceAllocFailed)return;
    mycWorkspace=static_cast<MycWorkspace*>(malloc(sizeof(MycWorkspace)));
    if(!mycWorkspace){mycWorkspaceAllocFailed=true;return;}
  }
  if(!ready() || !tick(38))return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.44f),d=detail();
  const float a=stage2BoundPhase(t*.34f),b=stage2BoundPhase(-t*.21f);
  const float splitPhase=stage2BoundPhase(t*(.34f*.7f));
  MycSeg* const lines=mycWorkspace->lines;int count=0;
  const int branches=3+(int)(tune1()*2.0f);
  for(int root=0;root<4;++root){
    const float rootAngle=K_TAU*(float)root/4.0f+.18f*sinf(a+root);
    float sx=.42f*cosf(rootAngle),sy=.42f*sinf(rootAngle);
    for(int arm=0;arm<branches;++arm){
      float x=sx,y=sy;
      float angle=rootAngle+K_PI+.31f*(arm-(branches-1)*.5f);
      for(int depth=0;depth<5;++depth){
        float split=.28f*sinf(splitPhase+root*1.91f+arm*2.38f+depth*.75f);
        angle+=split+.18f*sinf(b+depth*1.6f+arm);
        float len=.13f*(1.0f-.085f*depth);
        float nx=x+len*cosf(angle),ny=y+len*sinf(angle);
        if(count<108)lines[count++]={x,y,nx,ny,root*14.0f+arm*8.0f+depth*4.0f};
        x=nx;y=ny;
      }
    }
  }
  const float thickness=.008f+.006f*d;
  const float travelPhase=stage2BoundPhase(t*1.9f);
  const float scale=.56f*u,halfW=w*.5f,halfH=h*.5f;
  const float minArea=.07f;
  MycRasterSeg* const raster=mycWorkspace->raster;
  for(int i=0;i<count;++i){
    const MycSeg &line=lines[i];
    const float dx=line.x1-line.x0,dy=line.y1-line.y0;
    const float minX=fminf(line.x0,line.x1)-minArea,maxX=fmaxf(line.x0,line.x1)+minArea;
    const float minY=fminf(line.y0,line.y1)-minArea,maxY=fmaxf(line.y0,line.y1)+minArea;
    // The pixel center must lie inside the same AABB used in preview.3.
    const int xmin=(int)ceilf(minX/scale+halfW-.5f);
    const int xmax=(int)floorf(maxX/scale+halfW-.5f);
    const int ymin=(int)ceilf(minY/scale+halfH-.5f);
    const int ymax=(int)floorf(maxY/scale+halfH-.5f);
    raster[i]={line.x0,line.y0,dx,dy,1.0f/(dx*dx+dy*dy),line.seed,
               xmin<0?0:xmin,xmax>=w?w-1:xmax,
               ymin<0?0:ymin,ymax>=h?h-1:ymax};
  }
  // Preserve the preview.3 winning-segment rule and RGB calculation.
  // Each segment touches only the rows and columns in its bounding box.
  for(int y=0;y<h;++y){
    const float py=((float)y+.5f-halfH)*scale;
    for(int x0=0;x0<w;x0+=32){
      const int n=w-x0<32?w-x0:32;
      float light[32],hue[32];
      for(int j=0;j<n;++j){light[j]=.008f;hue[j]=110.0f;}
      for(int i=0;i<count;++i){
        const MycRasterSeg &r=raster[i];
        if(y<r.ymin || y>r.ymax || x0>r.xmax || x0+n-1<r.xmin)continue;
        const int xmin=r.xmin>x0?r.xmin:x0;
        const int xmax=r.xmax<x0+n-1?r.xmax:x0+n-1;
        const float ry=py-r.y0;
        for(int x=xmin;x<=xmax;++x){
          const float tileX0=((float)x0+.5f-halfW)*scale;
          const float px=tileX0+(x-x0)*scale;
          const float rx=px-r.x0;
          const float q=clip((rx*r.dx+ry*r.dy)*r.inv,0.0f,1.0f);
          const float vx=rx-q*r.dx,vy=ry-q*r.dy;
          const float dist2=vx*vx+vy*vy;
          if(dist2>=.005f)continue;
          const float dist=sqrtf(dist2);
          const float core=1.0f-smooth(thickness,thickness*2.3f,dist);
          const float halo=.13f*(1.0f-smooth(thickness*2.0f,.07f,dist));
          const float travel=.45f+.55f*sinf(q*9.0f+travelPhase+r.seed*.9f);
          const float v=(core+halo)*(.38f+.62f*travel);
          const int j=x-x0;
          if(v>light[j]){light[j]=v;hue[j]=95.0f+r.seed*2.7f+q*42.0f+SEGMENT.custom2*.48f;}
        }
      }
      for(int j=0;j<n;++j)
        SEGMENT.setPixelColorXY(x0+j,y,chroma(hue[j],clip(light[j]*(.45f+.55f*d))));
    }
  }
}
static const char m28[] PROGMEM="GPT28 - Luminous Mycelium@Growth,Glow,Branch Count,Color;!,!;!;2;sx=130,ix=205,c1=155,c2=110";

// GPT29 - Prismatic Comets. A completely different visual design from
// Strange Attractor: sweeping comet heads with long chromatic curving tails.
// Five paths, evaluated analytically per pixel: O(5 * pixels), no history.
static void mode_gpt29(){
  if(!ready() || !tick(33))return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.48f),d=detail();
  const float speed=.65f+1.25f*tune1();
  const float headPhase=stage2BoundPhase(t*speed*.61f);
  const float sweepPhase=stage2BoundPhase(t*speed*.31f);
  const float haloPhase=stage2BoundPhase(t*.73f);
  struct Comet{float x,y,dx,dy,curve,hue;};
  Comet comet[5];
  for(int i=0;i<5;++i){
    const float k=(float)i;
    const float ang=headPhase+k*1.25663706f;
    const float radial=.25f+.18f*sinf(sweepPhase+k*2.12f);
    const float xx=radial*cosf(ang)+.21f*sinf(sweepPhase+k*1.63f);
    const float yy=radial*sinf(ang)+.18f*cosf(sweepPhase+k*2.07f);
    const float direction=ang+1.25f+.40f*sinf(sweepPhase+k*.71f);
    comet[i]={xx,yy,cosf(direction),sinf(direction),.20f*sinf(haloPhase+k*1.71f),k*51.0f+SEGMENT.custom2*.56f};
  }
  const float width=.044f+.024f*(1.0f-tune2());
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    const Point p=normal(x,y,u,w*.5f,h*.5f);
    float lum=.006f,hue=0.0f;
    for(int i=0;i<5;++i){
      const Comet &q=comet[i];
      const float vx=p.x-q.x,vy=p.y-q.y;
      const float longitudinal=-(vx*q.dx+vy*q.dy);
      const float lateral=vx*(-q.dy)+vy*q.dx;
      const float bent=lateral-q.curve*longitudinal*longitudinal;
      float v=0.0f;
      if(longitudinal>=0.0f && longitudinal<.88f){
        const float taper=(1.0f-longitudinal/.88f);
        const float trailWidth=width+longitudinal*.028f;
        const float line=1.0f-smooth(trailWidth*.25f,trailWidth, fabsf(bent));
        v=clip(1.32f*line*taper*(.60f+.40f*taper));
      }
      const float r2=vx*vx+vy*vy;
      if(r2<.015f){
        const float head=1.0f-smooth(.023f,.085f,sqrtf(r2));
        v=fmaxf(v,head);
      }
      if(v>lum){lum=v;hue=q.hue+longitudinal*105.0f;}
    }
    SEGMENT.setPixelColorXY(x,y,chroma(hue,clip(lum*(.67f+.33f*d))));
  }
}
static const char m29[] PROGMEM="GPT29 - Prismatic Comets@Orbit Speed,Brightness,Tail Width,Hue;!,!;!;2;sx=125,ix=220,c1=125,c2=120";

// GPT30 - Moire Engine. Superposed drifting rotated fine gratings generate
// large interference envelopes and localized iridescent maxima.
static void mode_gpt30(){
  if(!ready() || !tick(30))return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.48f),d=detail();
  const float a=stage2BoundPhase(t*.29f),b=stage2BoundPhase(-t*.24f);
  const float c=stage2BoundPhase(t*.43f),k=12.0f+13.0f*tune1();
  const float angle=.22f+.19f*sinf(a),co=cosf(angle),si=sinf(angle);
  const float co2=cosf(-angle+.16f),si2=sinf(-angle+.16f);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float rx=p.x*co-p.y*si,ry=p.x*si+p.y*co;
    float sx=p.x*co2-p.y*si2,sy=p.x*si2+p.y*co2;
    float wave1=sinf(rx*k+.5f*sinf(ry*4.0f+c)+a);
    float wave2=sinf(sx*(k+.55f)+.5f*cosf(sy*3.9f+a)+b);
    float rings=sinf(8.0f*sqrtf(p.x*p.x+p.y*p.y)+c);
    float interference=.5f+.5f*wave1*wave2;
    float threads=(1.0f-smooth(.62f,.95f,fabsf(wave1)))*(1.0f-smooth(.62f,.95f,fabsf(wave2)));
    float bright=powf(interference,2.7f)*(.42f+.58f*d)+.27f*(1.0f-threads);
    bright*=.68f+.32f*(.5f+.5f*rings);
    float hue=50.0f+140.0f*interference+48.0f*rings+SEGMENT.custom2*.55f;
    SEGMENT.setPixelColorXY(x,y,chroma(hue,clip(bright)));
  }
}
static const char m30[] PROGMEM="GPT30 - Moire Engine@Drift,Intensity,Interference Density,Hue;!,!;!;2;sx=120,ix=195,c1=130,c2=125";

static const char m20[] PROGMEM="GPT20 - Zenith Quasicrystal@Drift,Brilliance,Axes,Color;!,!;!;2;sx=115,ix=170";

// Audio data is read from the *real* AudioReactive usermod, never from a copied symbol.
// 16 GEQ bands, Smoothed Volume and Sample Peak are the standard WLED contract.
struct AudioState {
  uint8_t smoothBands[16];
  uint8_t lastPeak;
  uint8_t volume;
  uint32_t lastPulse;
};
static bool sampleAudio(AudioState& state) {
  um_data_t* data=nullptr;
  bool has=UsermodManager::getUMData(&data,USERMOD_ID_AUDIOREACTIVE);
  has=has && data && data->u_data && data->u_size>=4U &&
      data->u_data[0] && data->u_data[2] && data->u_data[3];
  const uint8_t* fft=has?static_cast<const uint8_t*>(data->u_data[2]):nullptr;
  float volume=has?*static_cast<const float*>(data->u_data[0]):0.0f;
  if (!std::isfinite(volume)) volume=0.0f;
  uint8_t peak=has?*static_cast<const uint8_t*>(data->u_data[3]):0U;
  // Attack 5/8, release 1/8, avoids single-bin strobing.
  for(int i=0;i<16;++i){
    int old=state.smoothBands[i], input=fft?fft[i]:0;
    state.smoothBands[i]=(uint8_t)((input>old)?((old*3+input*5+4)/8):((old*7+input+4)/8));
  }
  int vol=(int)(clip(volume/255.0f)*255.0f+.5f);
  state.volume=(uint8_t)((state.volume*3+vol+2)/4);
  if(peak && !state.lastPeak && (uint32_t)(strip.now-state.lastPulse)>100U)
    state.lastPulse=strip.now;
  state.lastPeak=peak;
  return has;
}

// GPTA01: 16 frequency sectors deform the edge of an illuminated polar landmass.
static void mode_audio1() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& state=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0) memset(&state,0,sizeof(state));
  bool connected=sampleAudio(state);
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.34f),d=detail();
  float bass=(state.smoothBands[0]+state.smoothBands[1]+state.smoothBands[2])/(3.0f*255.0f);
  float halo=.035f+.22f*bass+.11f*(state.volume/255.0f);
#if GPTFX_STAGE2B_PHASE_BOUNDING
  const float raw[1]={-t*1.1f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float a=atan2f(p.y,p.x),r=sqrtf(squaref(p.x)+squaref(p.y));
    float idx=frac((a+K_PI)/K_TAU)*16.0f;
    unsigned band=((unsigned)idx)&15U;
    float amount=state.smoothBands[band]/255.0f;
    float coastline=.48f+.42f*amount+.06f*sinf(9.0f*a+GPTFX_STAGE2B_ARG(-t*1.1f,phase[0]));
    float shore=1.0f-smooth(.018f,.063f,fabsf(r-coastline));
    float interior=1.0f-smooth(coastline-.1f,coastline+.03f,r);
    float delta=frac(r*(8.0f+10.0f*tune1())-t*.2f+amount);
    float terrain=(1.0f-smooth(.04f,.17f,fminf(delta,1.0f-delta)));
    float bright=halo*interior+(shore*(.32f+.68f*amount))+
                 terrain*interior*(.07f+.28f*amount)*(.5f+.5f*d);
    bright*=connected?1.0f:.40f;
    SEGMENT.setPixelColorXY(x,y,chroma(band*16.0f+t*10.0f+r*25.0f+SEGMENT.custom2*.4f,bright));
  }
}
static const char ma1[] PROGMEM = "GPTA01 - Bass Cartographer@Drift,Contour,Isolines,Color;!,!;!;2v;sx=105,ix=175,c1=150,si=0";

// GPTA02: Harmonic energy drives 16 portals; no beat-triggered full-screen ring.
static void mode_audio2() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& state=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0) memset(&state,0,sizeof(state));
  bool connected=sampleAudio(state);
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.52f),d=detail();
#if GPTFX_STAGE2B_PHASE_BOUNDING
  const float raw[2]={-t*1.6f,t};
  const float phaseBound[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phaseBound,2);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r=sqrtf(squaref(p.x)+squaref(p.y)),a=atan2f(p.y,p.x);
    float k=frac((a+K_PI)/K_TAU)*16.0f;
    unsigned sector=((unsigned)k)&15U;
    unsigned next=(sector+1U)&15U;
    float e=(state.smoothBands[sector]*(1.0f-frac(k))+state.smoothBands[next]*frac(k))/255.0f;
    float bend=2.0f*e/(r+.25f);
    float phase=13.0f*r+8.0f*a + bend + GPTFX_STAGE2B_ARG(-t*1.6f,phaseBound[0]);
    float arcs=ridge(phase)+.45f*ridge(24.0f*r-2.5f*bend+GPTFX_STAGE2B_ARG(t,phaseBound[1]));
    float portal=1.0f-smooth(.016f,.09f,fabsf(r-(.18f+.19f*tune1()+.62f*e)));
    float lum=.018f+(.10f+.62f*e)*arcs+portal*(.18f+.47f*e);
    lum*=(.55f+.45f*d)*(.75f+.25f*(state.volume/255.0f));
    if(!connected)lum*=.40f;
    SEGMENT.setPixelColorXY(x,y,chroma(sector*17.0f+90.0f*e+t*12.0f+60.0f*r+SEGMENT.custom2*.45f,lum));
  }
}
static const char ma2[] PROGMEM = "GPTA02 - Harmonic Wormholes@Orbit,Energy,Depth,Prism;!,!;!;2v;sx=128,ix=195,c1=130,si=0";
// GPTA03: a spectrum loom; each frequency band drives one moving braid.
static void mode_audio3() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.40f),light=.0f;
#if GPTFX_STAGE2B_PHASE_BOUNDING
  float raw[17],phase[17];
  for(int k=0;k<16;++k){raw[k]=t*(1.0f+k*.035f);phase[k]=stage2BoundPhase(raw[k]);}
  raw[16]=-t*3.4f;phase[16]=stage2BoundPhase(raw[16]);
  stage2TrackPhase(raw,phase,17);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float fx=(p.x+.98f)*8.0f;
    int k=(int)floorf(fx);k=k<0?0:(k>15?15:k);
    float amt=st.smoothBands[k]/255.0f;
    float center=-.80f+1.60f*k/15.0f;
    float braid=center+.16f*sinf(p.y*(6.0f+5.0f*tune1())+GPTFX_STAGE2B_ARG(t*(1.0f+k*.035f),phase[k])+k*1.8f)*(.2f+amt);
    float gap=fabsf(p.x-braid);
    float filament=1.0f-smooth(.012f,.075f,gap);
    float sparks=ridge(p.y*19.0f+GPTFX_STAGE2B_ARG(-t*3.4f,phase[16])+k);
    light=.018f+filament*(.12f+.75f*amt)+filament*sparks*.22f;
    if(!connected)light*=.4f;
    SEGMENT.setPixelColorXY(x,y,chroma(k*15.0f+t*7.0f+SEGMENT.custom2*.4f,light));
  }
}
static const char ma3[] PROGMEM="GPTA03 - Spectral Loom@Drift,Energy,Braids,Color;!,!;!;2v;sx=125,ix=175,si=0";

// GPTA04: hexagonal frequency tiles with per-cell FFT-driven facet highlights.
static void mode_audio4() {
  if(!ready() || !tick(36))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.24f);
  float scale=3.0f+4.0f*tune1();
#if GPTFX_STAGE2B_PHASE_BOUNDING
  const float raw[1]={-t*.5f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    // Offset-row hex cell coordinates, with six planar edge normals.
    float yy=p.y*scale*.8660254f,gy=floorf(yy+.5f);
    float xx=p.x*scale-(fmodf(fabsf(gy),2.0f)>.5f?.5f:0.0f);
    float gx=floorf(xx+.5f),rx=xx-gx,ry=yy-gy;
    unsigned band=((unsigned)(fabsf(gx*7.0f+gy*11.0f)))&15U;
    float e=st.smoothBands[band]/255.0f;
    float hex=fmaxf(fabsf(rx)*.866f+.5f*fabsf(ry),fabsf(ry));
    float edge=1.0f-smooth(.035f,.115f,fabsf(hex-.43f));
    float filling=1.0f-smooth(.20f,.48f,hex);
    float facets=ridge(atan2f(ry,rx)*3.0f+e*4.0f+GPTFX_STAGE2B_ARG(-t*.5f,phase[0]));
    float lum=.025f+edge*(.24f+.69f*e)+filling*(.04f+.39f*e)*facets;
    if(!connected)lum*=.4f;
    SEGMENT.setPixelColorXY(x,y,chroma(band*15.0f+e*57.0f+t*10.0f+SEGMENT.custom2*.55f,lum));
  }
}
static const char ma4[] PROGMEM="GPTA04 - Resonant Tessellator@Drift,Facet,Tile Size,Hue;!,!;!;2v;sx=120,ix=185,si=0";

// GPTA05: overlapping, independently expanding beat ripples. A new beat never
// rewinds any existing wave; memory belongs to the segment (not task stack).
struct ReefWaveState {
  AudioState audio; // first member retains the shared AudioReactive sampler
  uint32_t launches[6];
  uint32_t lastRecorded;
  uint8_t count;
  uint8_t next;
};
static void mode_audio5() {
  if(!ready() || !tick(36))return;
  if(!SEGENV.allocateData(sizeof(ReefWaveState))){SEGMENT.fill(SEGCOLOR(0));return;}
  ReefWaveState& reef=*reinterpret_cast<ReefWaveState*>(SEGENV.data);
  if(SEGENV.call==0) memset(&reef,0,sizeof(reef));
  AudioState& st=reef.audio;
  bool connected=sampleAudio(st);
  // sampleAudio() reports a rising peak by updating lastPulse. Capture each
  // distinct timestamp once, leaving all earlier waves' birth times intact.
  if(connected && st.lastPulse && st.lastPulse!=reef.lastRecorded) {
    reef.lastRecorded=st.lastPulse;
    reef.launches[reef.next]=st.lastPulse;
    reef.next=(uint8_t)((reef.next+1U)%6U);
    if(reef.count<6U) ++reef.count;
  }
  const uint32_t now=strip.now;
  float waveRadius[6],waveGain[6];
  uint8_t active=0;
  for(uint8_t i=0;i<reef.count;++i){
    const uint32_t elapsed=(uint32_t)(now-reef.launches[i]);
    if(elapsed>=1700U)continue;
    const float progress=(float)elapsed*(1.0f/1700.0f);
    waveRadius[active]=1.8f*progress;
    // A gentle decay prevents successive peaks from appearing as full-screen
    // strobes, while preserving the original ripple width and velocity.
    waveGain[active]=.84f*(1.0f-progress)*(1.0f-progress);
    ++active;
  }
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.42f);
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[3]={t,-t,-t*3.0f};
  const float phase[3]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2])};
  stage1TrackPhase(raw,phase,3);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r=sqrtf(p.x*p.x+p.y*p.y),a=atan2f(p.y,p.x);
    float coord=frac((a+K_PI)/K_TAU)*16.0f;
    int k=(int)coord;float e=st.smoothBands[k&15]/255.0f;
    float trunk=1.0f-smooth(.03f,.15f,fabsf(sinf(8.0f*a+.27f*sinf(r*7.0f+GPTFX_STAGE1_ARG(t,phase[0])))));
    float spines=1.0f-smooth(.03f,.17f,
              fabsf(sinf(23.0f*a+6.0f*r+3.0f*sinf(r*4.0f+GPTFX_STAGE1_ARG(-t,phase[1])))));
    float reach=1.0f-smooth(.27f+.49f*e,.46f+.70f*e,r);
    float wave=0.0f;
    for(uint8_t n=0;n<active;++n){
      const float delta=fabsf(r-waveRadius[n]);
      if(delta<.078f)wave+=waveGain[n]*(1.0f-smooth(.024f,.078f,delta));
    }
    float lum=.014f+reach*(trunk*(.15f+.65f*e)+spines*(.04f+.38f*e))+
              wave+ridge(r*21.0f+GPTFX_STAGE1_ARG(-t*3.0f,phase[2]))*reach*.08f;
    if(!connected)lum*=.4f;
    SEGMENT.setPixelColorXY(x,y,chroma(k*19.0f+e*83.0f+r*50.0f+SEGMENT.custom2*.35f,lum));
  }
}
static const char ma5[] PROGMEM="GPTA05 - Synaptic Reef@Drift,Growth,Spines,Color;!,!;!;2v;sx=135,ix=190,si=0";


// Five new beta audio effects only: full-scale response follows the WLED 8-bit
// FFT contract, with smooth low-level lift and unchanged zero/full-scale endpoints.
// This is deliberately NOT applied to the 5 qualified GPTA01-05 renderers.
static inline float newAudioDrive(uint8_t band) {
  const float e=(float)band*(1.0f/255.0f);
  return e*(1.52f-.52f*e); // 0 -> 0, 255 -> 1; increased mid-range sensitivity
}

// GPTA06: Pulse Architecture. A spectral skyline with secondary luminous
// terraces; energy changes column height, while the bass launches floor pulses.
// All geometry is calculated per column, outside the pixel loop.
static void mode_audio6() {
  if(!ready() || !tick(34))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  int w=SEG_W,h=SEG_H;float t=timePhase(.43f),d=detail();
  const float bass=(newAudioDrive(st.smoothBands[0])+newAudioDrive(st.smoothBands[1])+newAudioDrive(st.smoothBands[2]))/3.0f;
  float heights[16],rim[16];
  for(int k=0;k<16;++k){
    const float e=newAudioDrive(st.smoothBands[k]);
    // At 100% FFT each bar fills nearly the entire display, regardless
    // of breathing phase. Only intermediate energy changes the silhouette.
    const float breathing=extraSine(stage1BoundPhase(t*.42f+k*.67f));
    heights[k]=.085f+.905f*e-.080f*e*(1.0f-e)*(1.0f-breathing);
    rim[k]=.35f+.65f*e;
  }
  // The wave phase is bounded only where used as a periodic argument.
  const float pulse=stage1BoundPhase(t*1.8f);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    int k=(int)((int64_t)x*16/w);if(k>15)k=15;
    const float e=newAudioDrive(st.smoothBands[k]);
    float ny=1.0f-((float)y+.5f)/h;
    float colx=frac(((float)x+.5f)*16.0f/w);
    float edge=(1.0f-smooth(.05f,.19f,fminf(colx,1.0f-colx)));
    float inside=1.0f-smooth(heights[k]-.018f,heights[k]+.018f,ny);
    float top=1.0f-smooth(.006f,.027f,fabsf(ny-heights[k]));
    float ledge=1.0f-smooth(.012f,.042f,fabsf(ny-.5f*heights[k]));
    float wave=1.0f-smooth(.025f,.095f,fabsf(ny-(.12f+.035f*extraSine(pulse+colx*5.0f))));
    float lum=.009f + inside*(.09f+.58f*e)*(1.0f-.52f*edge) +top*rim[k]*.48f+
              ledge*inside*.14f*e + wave*bass*.19f;
    if(!connected)lum*=.32f;
    SEGMENT.setPixelColorXY(x,y,chroma(k*13.0f+t*5.0f+SEGMENT.custom2*.58f,lum*(.70f+.30f*d)));
  }
}
static const char ma6[] PROGMEM="GPTA06 - Pulse Architecture@Motion,Impact,Buildings,Hue;!,!;!;2v;sx=118,ix=185,c1=125,c2=125";

// GPTA07: Harmonic Constellations, expanded into a full-canvas stellar lattice.
// Each of the sixteen FFT bands moves one star and controls its halo. Band
// contrasts control luminous bridges; no full-screen shockwave overlay.
// Geometry is precomputed outside the 4096-pixel loop; no frame heap usage.
static void mode_audio7() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  const int w=SEG_W,h=SEG_H;
  const float t=timePhase(.32f),unit=unitOf(w,h);
  const float strength=.78f+.78f*tune1();
  Point stars[16];float energy[16];
  for(int k=0;k<16;++k){
    float e=newAudioDrive(st.smoothBands[k]);
    energy[k]=e;
    const float a=stage1BoundPhase(t*(.42f+.035f*(k%3))+k*1.17f);
    const float b=stage1BoundPhase(t*(.28f+.04f*(k%4))+k*2.21f);
    // Four by four full-frame distribution, broad but bounded spectral motion.
    const float bx=-.78f+.52f*(float)(k%4);
    const float by=-.76f+.51f*(float)(k/4);
    stars[k]={bx+(.055f+.22f*e)*extraSine(a)*strength,
              by+(.045f+.22f*e)*extraCosine(b)*strength};
  }
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,unit,w*.5f,h*.5f);
    float best=0.0f,lines=0.0f;int dominant=0;
    for(int k=0;k<16;++k){
      const float dx=p.x-stars[k].x,dy=p.y-stars[k].y;
      const float ds=dx*dx+dy*dy;
      const float e=energy[k];
      const float halo=(.0028f+.064f*e)/(ds*7.8f+.015f);
      if(halo>best){best=halo;dominant=k;}
      // Twelve long vertical beams; bass and band strength also affect width.
      if(k<12){
        const int j=k+4;
        const float vx=stars[j].x-stars[k].x,vy=stars[j].y-stars[k].y;
        const float f=clip((dx*vx+dy*vy)/(vx*vx+vy*vy+.0001f));
        const float nx=dx-f*vx,ny=dy-f*vy;
        const float dist=nx*nx+ny*ny;
        const float mean=(e+energy[j])*.5f;
        const float width=.003f+.018f*mean;
        const float beam=(.012f+.84f*mean)*(1.0f-smooth(width*.15f,width,dist));
        if(beam>lines){lines=beam;dominant=k;}
      }
      // Horizontal bridges, on alternate cells, appear at mid/high notes.
      if(k%4<3 && (k/4)%2==0){
        const int j=k+1;
        const float vx=stars[j].x-stars[k].x,vy=stars[j].y-stars[k].y;
        const float f=clip((dx*vx+dy*vy)/(vx*vx+vy*vy+.0001f));
        const float nx=dx-f*vx,ny=dy-f*vy;
        const float mean=(e+energy[j])*.5f;
        const float bridge=(.009f+.72f*mean)*(1.0f-smooth(.0005f,.006f+.009f*mean,nx*nx+ny*ny));
        if(bridge>lines){lines=bridge;dominant=k;}
      }
    }
    float lum=clip(.007f+best*.88f+lines);
    if(!connected)lum*=.35f;
    SEGMENT.setPixelColorXY(x,y,chroma(dominant*15.0f+t*4.0f+SEGMENT.custom2*.6f,lum));
  }
}
static const char ma7[] PROGMEM="GPTA07 - Harmonic Constellations@Orbit,Starlight,Links,Hue;!,!;!;2v;sx=120,ix=185,c1=125,c2=128";

// GPTA08: Spectral Vortex. FFT values create twelve rotating spiral arms;
// bounded phase and precomputed band weights avoid temporal slowdowns.
static void mode_audio8() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  int w=SEG_W,h=SEG_H;float unit=unitOf(w,h),t=timePhase(.30f);
  float energies[16];for(int k=0;k<16;++k)energies[k]=newAudioDrive(st.smoothBands[k]);
  const float spin=stage1BoundPhase(t*.85f);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,unit,w*.5f,h*.5f);
    float r=sqrtf(p.x*p.x+p.y*p.y),a=atan2f(p.y,p.x);
    float arm=frac((a+K_PI+spin+3.8f*logf(r+.18f))/K_TAU*8.0f);
    int k=(int)floorf(frac((a+K_PI)/K_TAU)*16.0f)&15;
    float e=energies[k];
    float thread=1.0f-smooth(.035f,.25f,fminf(arm,1.0f-arm));
    float ring=1.0f-smooth(.013f,.075f,fabsf(r-(.23f+.79f*e)));
    float core=1.0f-smooth(.02f,.22f,r);
    float outer=1.0f-smooth(.85f,1.33f,r);
    float lum=.009f+outer*(thread*(.095f+.87f*e)+ring*(.025f+.45f*e)) +core*.25f;
    if(!connected)lum*=.36f;
    SEGMENT.setPixelColorXY(x,y,chroma(k*14.5f+r*85.0f+t*8.0f+SEGMENT.custom2*.5f,lum));
  }
}
static const char ma8[] PROGMEM="GPTA08 - Spectral Vortex@Rotation,Energy,Spiral Density,Hue;!,!;!;2v;sx=115,ix=195,c1=120,c2=120";

// GPTA09: Spectral Ribbons. Completely new renderer in place of Fireflies:
// six full-width interacting light-streams, with spectral shape modulation,
// bass widening and transient traveling streaks. No per-pixel sin/sqrt,
// no extra framebuffer and no per-frame heap or large stack buffers.
static void mode_audio9() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  const int w=SEG_W,h=SEG_H;
  const float t=timePhase(.48f),motion=.7f+.95f*tune1();
  const float bass=(newAudioDrive(st.smoothBands[0])+newAudioDrive(st.smoothBands[1])+newAudioDrive(st.smoothBands[2]))/3.0f;
  const float pulse=st.lastPulse?clip(1.0f-(float)(uint32_t)(strip.now-st.lastPulse)/850.0f):0.0f;
  const float density=.9f+.85f*tune1();
  // Column-major: calculate six traveling ribbon centerlines only once per x.
  for(int x=0;x<w;++x){
    const float u=((float)x+.5f)/(float)w;
    float center[6],width[6],power[6];
    for(int k=0;k<6;++k){
      const float e=(newAudioDrive(st.smoothBands[k*2])+newAudioDrive(st.smoothBands[k*2+1]))*.5f;
      const float a=stage1BoundPhase(t*(.65f+.075f*k)*motion+k*.95f);
      const float b=stage1BoundPhase(t*(.29f+.035f*k)+k*2.33f);
      const float wave=extraSine(stage1BoundPhase(u*(6.5f+1.3f*k)*density+a));
      const float braid=extraCosine(stage1BoundPhase(u*11.0f-b));
      center[k]=(.12f+.15f*k)+(.022f+.23f*e)*wave+.038f*braid;
      width[k]=.010f+.019f*e+.015f*bass;
      power[k]=.095f+.91f*e;
    }
    const float traveling=frac(u*.72f-t*.10f);
    const float streak=(1.0f-smooth(.016f,.095f,fabsf(traveling-.5f)))*pulse;
    for(int y=0;y<h;++y){
      const float v=((float)y+.5f)/(float)h;
      float lum=.005f;int bestk=0;float high=0.0f;
      for(int k=0;k<6;++k){
        const float d=fabsf(v-center[k]);
        const float core=(1.0f-smooth(width[k]*.16f,width[k],d))*power[k];
        const float glow=(1.0f-smooth(width[k],width[k]*3.5f,d))*power[k]*.17f;
        const float val=core+glow;
        lum+=val*.67f;
        if(val>high){high=val;bestk=k;}
      }
      lum=clip(lum+streak*high*.50f);
      if(!connected)lum*=.35f;
      SEGMENT.setPixelColorXY(x,y,chroma(bestk*34.0f+u*34.0f+t*7.0f+SEGMENT.custom2*.55f,lum));
    }
  }
}
static const char ma9[] PROGMEM="GPTA09 - Spectral Ribbons@Flow,Resonance,Braids,Hue;!,!;!;2v;sx=130,ix=190,c1=128,c2=130";

// GPTA10: Resonance Cathedral. Stained-glass vaulted arches, translucent
// vertical columns and an FFT-controlled rose window. No per-frame heap work.
static void mode_audio10() {
  if(!ready() || !tick(36))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  int w=SEG_W,h=SEG_H;float unit=unitOf(w,h),t=timePhase(.20f);
  const float phase=stage1BoundPhase(t*.72f);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,unit,w*.5f,h*.5f);
    float a=atan2f(p.y+.14f,p.x),r=sqrtf(p.x*p.x+(p.y+.14f)*(p.y+.14f));
    int k=(int)(frac((a+K_PI)/K_TAU)*16.0f)&15;
    float e=newAudioDrive(st.smoothBands[k]);
    // A spectral expansion of the arch, no disconnected vertical pillars.
    float vault=1.0f-smooth(.012f,.055f,fabsf(p.y-(.45f+(.17f+.11f*e)*extraCosine(stage1BoundPhase(p.x*5.0f)))));
    float rose=1.0f-smooth(.018f,.070f,fabsf(r-(.38f+.13f*e)));
    float roseRays=1.0f-smooth(.10f,.37f,fabsf(extraSine(8.0f*a+phase)));
    float glass=1.0f-smooth(.22f,.68f,r);
    float lum=.009f+vault*(.10f+.75f*e)+
      rose*(.08f+.65f*e)+roseRays*glass*(.035f+.43f*e);
    if(!connected)lum*=.34f;
    SEGMENT.setPixelColorXY(x,y,chroma(k*16.0f+45.0f*e+t*6.0f+SEGMENT.custom2*.5f,lum));
  }
}
static const char ma10[] PROGMEM="GPTA10 - Resonance Cathedral@Breathing,Stained Glass,Arches,Hue;!,!;!;2v;sx=115,ix=195,c1=130,c2=128";

static void mode_gpt26_profiled(){profileExtra("GPT26",mode_gpt26);}
static void mode_gpt27_profiled(){profileExtra("GPT27",mode_gpt27);}
static void mode_gpt28_profiled(){profileExtra("GPT28",mode_gpt28);}
static void mode_gpt29_profiled(){profileExtra("GPT29",mode_gpt29);}
static void mode_gpt30_profiled(){profileExtra("GPT30",mode_gpt30);}
static void mode_audio6_profiled(){profileExtra("GPTA06",mode_audio6);}
static void mode_audio7_profiled(){profileExtra("GPTA07",mode_audio7);}
static void mode_audio8_profiled(){profileExtra("GPTA08",mode_audio8);}
static void mode_audio9_profiled(){profileExtra("GPTA09",mode_audio9);}
static void mode_audio10_profiled(){profileExtra("GPTA10",mode_audio10);}
static void mode_gpt1_profiled(){profileExtra("GPT01",mode_gpt1);}
static void mode_gpt2_profiled(){profileExtra("GPT02",mode_gpt2);}
static void mode_gpt9_profiled(){profileExtra("GPT09",mode_gpt9);}
static void mode_gpt10_profiled(){profileExtra("GPT10",mode_gpt10);}
static void mode_gpt14_profiled(){profileExtra("GPT14",mode_gpt14);}
static void mode_gpt16_profiled(){profileExtra("GPT16",mode_gpt16);}
static void mode_gpt15_profiled(){profileExtra("GPT15",mode_gpt15);}
static void mode_gpt25_profiled(){profileExtra("GPT25",mode_gpt25);}
static void mode_audio1_profiled(){profileExtra("GPTA01",mode_audio1);}
static void mode_audio2_profiled(){profileExtra("GPTA02",mode_audio2);}
static void mode_audio3_profiled(){profileExtra("GPTA03",mode_audio3);}
static void mode_audio4_profiled(){profileExtra("GPTA04",mode_audio4);}
// Named wrappers keep registration order unchanged.
static void mode_gpt5_profiled(){profileExtra("GPT05",mode_gpt5);}
static void mode_gpt11_profiled(){profileExtra("GPT11",mode_gpt11);}
static void mode_gpt13_profiled(){profileExtra("GPT13",mode_gpt13);}
static void mode_gpt19_profiled(){profileExtra("GPT19",mode_gpt19);}
static void mode_gpt23_profiled(){profileExtra("GPT23",mode_gpt23);}
static void mode_audio5_profiled(){profileExtra("GPTA05",mode_audio5);}
static void mode_gpt7_profiled(){profileExtra("GPT07",mode_gpt7);}
static void mode_gpt8_profiled(){profileExtra("GPT08",mode_gpt8);}
static void mode_gpt18_profiled(){profileExtra("GPT18",mode_gpt18);}
static void mode_gpt20_profiled(){profileExtra("GPT20",mode_gpt20);}

} // namespace gptfx

class GPTMatrixFXUsermod : public Usermod {
public:
  void setup() override {
    // WLED allocates effect IDs dynamically; UI labels remain zero-padded.
    strip.addEffect(255, &gptfx::mode_gpt1_profiled, gptfx::m1);
    strip.addEffect(255, &gptfx::mode_gpt2_profiled, gptfx::m2);
    strip.addEffect(255, &gptfx::mode_gpt3, gptfx::m3);
    strip.addEffect(255, &gptfx::mode_gpt4, gptfx::m4);
    strip.addEffect(255, &gptfx::mode_gpt5_profiled, gptfx::m5);
    strip.addEffect(255, &gptfx::mode_gpt6, gptfx::m6);
    strip.addEffect(255, &gptfx::mode_gpt7_profiled, gptfx::m7);
    strip.addEffect(255, &gptfx::mode_gpt8_profiled, gptfx::m8);
    strip.addEffect(255, &gptfx::mode_gpt9_profiled, gptfx::m9);
    strip.addEffect(255, &gptfx::mode_gpt10_profiled, gptfx::m10);
    strip.addEffect(255, &gptfx::mode_gpt11_profiled, gptfx::m11);
    strip.addEffect(255, &gptfx::mode_gpt12, gptfx::m12);
    strip.addEffect(255, &gptfx::mode_gpt13_profiled, gptfx::m13);
    strip.addEffect(255, &gptfx::mode_gpt14_profiled, gptfx::m14);
    strip.addEffect(255, &gptfx::mode_gpt15_profiled, gptfx::m15);
    strip.addEffect(255, &gptfx::mode_gpt16_profiled, gptfx::m16);
    strip.addEffect(255, &gptfx::mode_gpt17, gptfx::m17);
    strip.addEffect(255, &gptfx::mode_gpt18_profiled, gptfx::m18);
    strip.addEffect(255, &gptfx::mode_gpt19_profiled, gptfx::m19);
    strip.addEffect(255, &gptfx::mode_gpt20_profiled, gptfx::m20);
    strip.addEffect(255, &gptfx::mode_audio1_profiled, gptfx::ma1);
    strip.addEffect(255, &gptfx::mode_audio2_profiled, gptfx::ma2);
    strip.addEffect(255, &gptfx::mode_audio3_profiled, gptfx::ma3);
    strip.addEffect(255, &gptfx::mode_audio4_profiled, gptfx::ma4);
    strip.addEffect(255, &gptfx::mode_audio5_profiled, gptfx::ma5);
    // Additional standard effects stay appended: existing dynamic IDs keep their order.
    strip.addEffect(255, &gptfx::mode_gpt21, gptfx::m21);
    strip.addEffect(255, &gptfx::mode_gpt22, gptfx::m22);
    strip.addEffect(255, &gptfx::mode_gpt23_profiled, gptfx::m23);
    strip.addEffect(255, &gptfx::mode_gpt24_profiled, gptfx::m24);
    strip.addEffect(255, &gptfx::mode_gpt25_profiled, gptfx::m25);
    strip.addEffect(255, &gptfx::mode_gpt26_profiled, gptfx::m26);
    strip.addEffect(255, &gptfx::mode_gpt27_profiled, gptfx::m27);
    strip.addEffect(255, &gptfx::mode_gpt28_profiled, gptfx::m28);
    strip.addEffect(255, &gptfx::mode_gpt29_profiled, gptfx::m29);
    strip.addEffect(255, &gptfx::mode_gpt30_profiled, gptfx::m30);
    // Five promoted audio effects remain appended, so all previously registered IDs stay fixed.
    strip.addEffect(255, &gptfx::mode_audio6_profiled, gptfx::ma6);
    strip.addEffect(255, &gptfx::mode_audio7_profiled, gptfx::ma7);
    strip.addEffect(255, &gptfx::mode_audio8_profiled, gptfx::ma8);
    strip.addEffect(255, &gptfx::mode_audio9_profiled, gptfx::ma9);
    strip.addEffect(255, &gptfx::mode_audio10_profiled, gptfx::ma10);
    gptfx::runTrigBenchmark();
  }
  void loop() override {}
  void addToJsonInfo(JsonObject& root) override {
    JsonObject userInfo = root["u"].to<JsonObject>();
    JsonObject info = userInfo["gptMatrixFX"].to<JsonObject>();
    info["version"] = gptfx::GPTFX_VERSION;
    info["build"] = gptfx::GPTFX_BUILD;
    info["effects"] = 40;
    info["standard"] = 30;
    info["audio"] = 10;
#if GPTFX_ENABLE_JSON_DIAGNOSTICS
    info["experimental"] = 0;
    JsonObject trig = info["trigonometry"].to<JsonObject>();
    trig["effect"] = "GPT24";
    trig["backend"] = gptfx_trig::backendName();
    trig["tableSamples"] = gptfx_trig::TABLE_SIZE;
    trig["tableBytes"] = sizeof(gptfx_trig::sinTable);
    trig["nativeSmallNs"] = gptfx::trigBenchmark.nativeSmallNs;
    trig["nativeLargeNs"] = gptfx::trigBenchmark.nativeLargeNs;
    trig["nearestNs"] = gptfx::trigBenchmark.nearestNs;
    trig["linearNs"] = gptfx::trigBenchmark.linearNs;
    trig["extraBackend"] = gptfx::extraBackendName();
    trig["extraEffects"] = "GPT08,GPT20";
    trig["labyrinthBackend"] = gptfx::labyrinthBackendName();
    trig["labyrinthPhaseBounding"] = (bool)GPTFX_GPT18_PHASE_BOUNDING;
    trig["stage1PhaseBounding"] = (bool)GPTFX_STAGE1_PHASE_BOUNDING;
    trig["stage1Effects"] = "GPT05,GPT11,GPT13,GPT19,GPT23,GPTA05";
    trig["stage2PhaseBounding"] = (bool)GPTFX_STAGE2_PHASE_BOUNDING;
    trig["stage2Effects"] = "GPT01,GPT02,GPT09,GPT10,GPT14,GPT16";
    trig["stage2bPhaseBounding"] = (bool)GPTFX_STAGE2B_PHASE_BOUNDING;
    trig["stage2bEffects"] = "GPT15,GPT25,GPTA01,GPTA02,GPTA03,GPTA04";
    trig["gpt10CellCache"] = (bool)GPTFX_GPT10_CELL_CACHE;
    trig["gpt10PacketBounding"] = (bool)GPTFX_GPT10_PACKET_BOUNDING;
    JsonObject extra = info["extraPerformance"].to<JsonObject>();
    extra["effect"] = gptfx::extraPerf.effect;
    extra["calls"] = gptfx::extraPerf.calls;
    extra["rendered"] = gptfx::extraPerf.rendered;
    extra["skipped"] = gptfx::extraPerf.skipped;
    extra["renderLastUs"] = gptfx::extraPerf.lastUs;
    extra["renderMaxUs"] = gptfx::extraPerf.maxUs;
    extra["recentCount"] = gptfx::extraPerf.recentValid;
    uint64_t extraRecentSum=0;
    uint32_t extraRecentMax=0;
    for(uint8_t i=0;i<gptfx::extraPerf.recentValid;++i){
      extraRecentSum+=gptfx::extraPerf.recentUs[i];
      if(gptfx::extraPerf.recentUs[i]>extraRecentMax)extraRecentMax=gptfx::extraPerf.recentUs[i];
    }
    extra["recentAvgUs"] = gptfx::extraPerf.recentValid ?
      (uint32_t)(extraRecentSum/gptfx::extraPerf.recentValid) : 0U;
    extra["recentMaxUs"] = extraRecentMax;
    extra["width"] = gptfx::extraPerf.width;
    extra["height"] = gptfx::extraPerf.height;
    extra["phaseRawAbs"] = gptfx::extraPerf.phaseRawAbs;
    extra["phaseWrappedAbs"] = gptfx::extraPerf.phaseWrappedAbs;
    JsonObject glass = info["gpt10Diagnostics"].to<JsonObject>();
    glass["hashRequests"] = gptfx::gpt10Diag.hashRequests;
    glass["hashEvaluations"] = gptfx::gpt10Diag.hashEvaluations;
    glass["hashCacheHits"] = gptfx::gpt10Diag.hashCacheHits;
    glass["hashArgAbs"] = gptfx::gpt10Diag.hashArgAbs;
    glass["packetRawSampleAbs"] = gptfx::gpt10Diag.packetRawAbs;
    glass["packetBoundedSampleAbs"] = gptfx::gpt10Diag.packetBoundedAbs;
    JsonObject perf = info["performance"].to<JsonObject>();
    perf["effect"] = "GPT24";
    perf["calls"] = gptfx::gpt24Perf.calls;
    perf["rendered"] = gptfx::gpt24Perf.rendered;
    perf["skipped"] = gptfx::gpt24Perf.skipped;
    perf["renderAvgUs"] = gptfx::gpt24Perf.rendered ?
      (uint32_t)(gptfx::gpt24Perf.renderTotalUs / gptfx::gpt24Perf.rendered) : 0U;
    perf["renderMaxUs"] = gptfx::gpt24Perf.renderMaxUs;
    // The last 32 completed frames provide a more useful snapshot when BLE
    // connection status changes during the same uptime.
    uint64_t recentSum = 0;
    uint32_t recentMax = 0;
    for(uint8_t i=0; i<gptfx::gpt24Perf.recentValid; ++i) {
      const uint32_t sample=gptfx::gpt24Perf.recentUs[i];
      recentSum += sample;
      if(sample>recentMax) recentMax=sample;
    }
    perf["recentCount"] = gptfx::gpt24Perf.recentValid;
    perf["recentAvgUs"] = gptfx::gpt24Perf.recentValid ?
      (uint32_t)(recentSum / gptfx::gpt24Perf.recentValid) : 0U;
    perf["recentMaxUs"] = recentMax;
    perf["renderLastUs"] = gptfx::gpt24Perf.renderLastUs;
    perf["rowAvgUs"] = gptfx::gpt24Perf.rowSamples ?
      (uint32_t)(gptfx::gpt24Perf.rowTotalUs / gptfx::gpt24Perf.rowSamples) : 0U;
    perf["rowMaxUs"] = gptfx::gpt24Perf.rowMaxUs;
    perf["coreStart"] = (int)gptfx::gpt24Perf.coreStart;
    perf["coreEnd"] = (int)gptfx::gpt24Perf.coreEnd;
    perf["coreChanges"] = gptfx::gpt24Perf.coreChanges;
    perf["phaseRawAbs"] = gptfx::gpt24Perf.phaseRawAbs;
    perf["phaseWrappedAbs"] = gptfx::gpt24Perf.phaseWrappedAbs;
    perf["width"] = gptfx::gpt24Perf.width;
    perf["height"] = gptfx::gpt24Perf.height;
#endif // GPTFX_ENABLE_JSON_DIAGNOSTICS
  }
  uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }
};
static GPTMatrixFXUsermod gptMatrixFX;
REGISTER_USERMOD(gptMatrixFX);
