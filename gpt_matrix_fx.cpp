/*
 * GPT Matrix FX - 30 WLED 2D effects (c) 2026.
 * Version: 0.3.0-dev.6 (20 procedural + 5 audio + 5 experimental).
 * License: MIT. Designed for WLED 0.15+ / modern WLED usermod FX API.
 * No WLED core edits, no display buffers outside WLED's segment storage.
 * The number on the effect name is a stable human label, not a WLED mode ID.
 */
#include "wled.h"
#include <cmath>
#include <string.h>

namespace gptfx {
constexpr const char* GPTFX_VERSION = "0.3.0";
constexpr const char* GPTFX_BUILD = "dev.6";
constexpr float K_PI = 3.14159265358979323846f;
constexpr float K_TAU = 6.28318530717958647692f;

struct Point { float x, y; };

static inline float clip(float v, float lo = 0.0f, float hi = 1.0f) {
  return v < lo ? lo : (v > hi ? hi : v);
}
static inline float squaref(float v) { return v * v; }
static inline float frac(float v) { return v - floorf(v); }
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
static inline bool tick(uint32_t ms = 28) {
  if (SEGENV.call && (uint32_t)(strip.now - SEGENV.step) < ms) return false;
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
  for (int y=0; y<h; ++y) for (int x=0; x<w; ++x) {
    Point p = normal(x,y,unit,w*.5f,h*.5f);
    float warp = (0.10f+0.30f*lens) * sinf(3.9f*p.x + 2.7f*p.y + t*.7f);
    float a = sqrtf(squaref(p.x-p0.x)+squaref(p.y-p0.y));
    float b = sqrtf(squaref(p.x-p1.x)+squaref(p.y-p1.y));
    float c = sqrtf(squaref(p.x-p2.x)+squaref(p.y-p2.y));
    float f = (12.0f+9.0f*d)*(a-b) + 9.0f*(b-c) + warp*3.0f;
    float g = (15.0f+8.0f*lens)*(a+c) - t*1.4f;
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
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float first=1000.0f, second=1000.0f; int id=0;
    for(int i=0;i<N;++i) {
      float dist=squaref(p.x-seeds[i].x)+squaref(p.y-seeds[i].y);
      if(dist<first){second=first;first=dist;id=i;}
      else if(dist<second){second=dist;}
    }
    float wall=1.0f-smooth(.008f+.009f*tune1(),.052f+.075f*tune1(),second-first);
    float beat=ridge(14.0f*sqrtf(first)-t*1.7f + id*.4f);
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
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r=sqrtf(squaref(p.x)+squaref(p.y));
    float angle=atan2f(p.y,p.x);
    float bend=mass/(r+.20f);
    float spiral=angle+ bend + r*(4.5f+4.0f*tune2())-t;
    float rings=ridge((r+.08f*bend)*16.0f-t*.8f);
    float stream=ridge(spiral*2.7f+2.5f*sinf(r*6.0f-t*.6f));
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
  for(int y=0;y<h;++y) for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float ax=p.x,ay=p.y, ink=0.0f, hue=0.0f;
    for(int j=0;j<(int)nested;++j){
      float c=cosf(t*.09f+j*.75f),s=sinf(t*.09f+j*.75f);
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
    float warp=.31f*sinf(2.7f*p.x+1.9f*p.y+t*.48f);
    float xx=p.x+warp*tune1(), yy=p.y+.2f*cosf(3.3f*p.x-t*.31f);
    float field=.43f*sinf(3.0f*xx+1.2f*yy+t*.40f)
              +.31f*sinf(5.4f*yy-2.0f*xx-t*.23f)
              +.17f*sinf(9.8f*xx+7.1f*yy+t*.57f)
              +.09f*cosf(14.0f*yy-3.0f*xx+t*.71f);
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float zx=p.x+.16f*sinf(t*.18f),zy=p.y;
    float trap=10.0f;int it=0;
    for(;it<depth && zx*zx+zy*zy<16.0f;++it){
      float xx=zx*zx-zy*zy+cr,yy=2.0f*zx*zy+ci;
      zx=xx;zy=yy;
      trap=fminf(trap,fabsf(zx*zy));
    }
    float escaped=(float)it/depth;
    float veins=1.0f-smooth(.008f,.065f,trap);
    float glass=.10f+.42f*escaped+.38f*veins+.17f*ridge(trap*16.0f+t);
    SEGMENT.setPixelColorXY(x,y,chroma(it*18.0f+trap*180.0f+t*12.0f+SEGMENT.custom2*.5f,glass*(.55f+.45f*d)));
  }
}
static const char m9[] PROGMEM="GPT09 - Mandelglass@Drift,Glass,Iterations,Prism;!,!;!;2;sx=125,ix=160";

// GPT10: moving circuit-trace board; junctions illuminate as timed signals pass.
static void mode_gpt10() {
  if(!ready() || !tick(30))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.65f);
  float scale=4.5f+6.0f*tune1();
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float gx=p.x*scale+t*.11f,gy=p.y*scale-t*.06f;
    float ix=floorf(gx),iy=floorf(gy),fx=frac(gx)-.5f,fy=frac(gy)-.5f;
    float seed=frac(sinf(ix*127.1f+iy*311.7f)*43758.5453f);
    float hor=1.0f-smooth(.038f,.13f,fabsf(fy)),ver=1.0f-smooth(.038f,.13f,fabsf(fx));
    float trace=(seed>.34f?hor:ver);
    float joint=(1.0f-smooth(.06f,.2f,sqrtf(fx*fx+fy*fy)));
    float lineCoord=(seed>.34f?gx:gy)*.65f;
    float packet=ridge(lineCoord*5.0f-t*4.1f+seed*8.0f);
    float lum=.022f+trace*(.14f+.65f*packet)+joint*(.25f+.45f*detail());
    SEGMENT.setPixelColorXY(x,y,chroma(seed*210.0f+packet*48.0f+SEGMENT.custom2*.35f,lum));
  }
}
static const char m10[] PROGMEM="GPT10 - Glass Circuit@Signal,Energy,Density,Hue;!,!;!;2;sx=120,ix=170,c1=100";

// GPT11: luminous woven fibers, with alternating over-under crossings.
static void mode_gpt11() {
  if(!ready() || !tick(31))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.46f),freq=5.0f+7.0f*tune1();
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float dx=p.x+.16f*sinf(3.2f*p.y+t),dy=p.y+.16f*cosf(3.6f*p.x-t*.71f);
    float warp=fabsf(sinf(freq*dx)),weft=fabsf(sinf(freq*dy));
    float hline=1.0f-smooth(.04f,.22f,warp),vline=1.0f-smooth(.04f,.22f,weft);
    float over=sinf(freq*(dx+dy)*.50f+t*.3f);
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float f=p.x+.11f*sinf(5.2f*p.y+t*.31f),g=p.y+.14f*sinf(4.8f*p.x-t*.43f);
    float crackX=frac(4.2f*f+.37f*sinf(6.2f*g+t*.42f));
    float crackY=frac(3.7f*g+.24f*cosf(6.0f*f-t*.34f));
    float seamX=1.0f-smooth(.01f,.09f,fminf(crackX,1.0f-crackX));
    float seamY=1.0f-smooth(.01f,.09f,fminf(crackY,1.0f-crackY));
    float facet=frac(floorf(4.2f*f)*.618f+floorf(3.7f*g)*.382f);
    float gold=fmaxf(seamX,seamY);
    float b=.07f+.30f*facet+.58f*gold+.08f*ridge(8.0f*f+t);
    SEGMENT.setPixelColorXY(x,y,chroma(125.0f*facet+SEGMENT.custom2*.56f+t*5.0f,b));
  }
}
static const char m13[] PROGMEM="GPT13 - Chromatic Kintsugi@Drift,Seams,Fracture,Gold;!,!;!;2;sx=120,ix=180";

// GPT14: infinite ascending corridor with depth-perspective tiled beams.
static void mode_gpt14() {
  if(!ready() || !tick(33))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.58f);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float z=1.0f/(.21f+fabsf(p.y+.07f));
    float xx=p.x*z,yy=(p.y<0?-1.0f:1.0f)*z+t*.7f;
    float stripeX=frac(xx*(.48f+.4f*tune1()));
    float stripeY=frac(yy*.53f);
    float vx=1.0f-smooth(.012f,.07f,fminf(stripeX,1.0f-stripeX));
    float vy=1.0f-smooth(.014f,.09f,fminf(stripeY,1.0f-stripeY));
    float vanish=1.0f-smooth(.01f,.20f,fabsf(p.y));
    float lum=.025f+.48f*fmaxf(vx,vy)+.24f*vanish+.18f*ridge(yy+t*.4f);
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float ax=p.x-north.x,ay=p.y-north.y,bx=p.x-south.x,by=p.y-south.y;
    float da=sqrtf(ax*ax+ay*ay)+.04f,db=sqrtf(bx*bx+by*by)+.04f;
    float potential=1.0f/da-1.0f/db;
    float vector=atan2f(ay,ax)-atan2f(by,bx);
    float spikes=ridge(vector*(5.0f+9.0f*tune1())+potential*.65f-t*.42f);
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r0=sqrtf((p.x+.40f)*(p.x+.40f)+p.y*p.y)+.03f;
    float r1=sqrtf((p.x-.40f)*(p.x-.40f)+p.y*p.y)+.03f;
    float a0=atan2f(p.y,p.x+.40f),a1=atan2f(p.y,p.x-.40f);
    float l0=ridge(8.0f*a0+8.0f*logf(r0)-t*1.6f);
    float l1=ridge(8.0f*a1-8.0f*logf(r1)+t*1.6f);
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float xx=p.x,yy=p.y,glow=0.0f,energy=0.0f;
    for(int k=0;k<4;++k){
      float f=2.4f+(float)k*.85f;
      float wave=sinf(xx*f+yy*(1.7f+.3f*k)+t*(.4f+.1f*k));
      float stroke=1.0f-smooth(.02f,.11f,fabsf(wave));
      glow=fmaxf(glow,stroke*(1.0f-.14f*k));
      energy+=stroke*.17f;
      xx=fabsf(xx)-(.40f-.06f*k)+.08f*sinf(t*.38f+k);
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float a=atan2f(p.y,p.x),r=sqrtf(p.x*p.x+p.y*p.y);
    float v=.025f,shade=0.0f;
    for(int k=1;k<=4;++k){
      float radius=k*.225f,an=a+(k&1?1.0f:-1.0f)*t*(.25f+.12f*k);
      float dial=1.0f-smooth(.012f,.044f,fabsf(r-radius));
      float marks=1.0f-smooth(.025f,.20f,fabsf(sinf((8.0f+k*3.0f)*an)));
      float glyph=marks*(1.0f-smooth(.017f,.045f,fabsf(r-(radius+.054f))));
      v+=dial*.41f+glyph*.58f;
      shade+=dial*k*.05f;
    }
    float pointer=1.0f-smooth(.007f,.037f,fabsf(sinf(a-t*.64f)));
    v+=pointer*.23f*(1.0f-smooth(.75f,1.0f,r));
    SEGMENT.setPixelColorXY(x,y,chroma(r*88.0f+shade*110.0f+t*6.0f+SEGMENT.custom2*.52f,v));
  }
}
static const char m19[] PROGMEM="GPT19 - Celestial Runewheel@Winding,Shine,Marks,Prism;!,!;!;2;sx=135,ix=165";

// GPT20: a 5-axis quasiperiodic projection creates wandering bright stars.
static void mode_gpt20() {
  if(!ready() || !tick(34))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.32f);
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float sum=0.0f,product=1.0f;
    for(int k=0;k<5;++k){
      float a=(float)k*K_TAU/5.0f;
      float phase=7.0f*(p.x*cosf(a)+p.y*sinf(a))+
                  t*(.37f+.08f*k)*(k&1?-1.0f:1.0f);
      float wave=cosf(phase);sum+=wave;
      product*=.78f+.22f*wave;
    }
    float star=1.0f-smooth(.35f,1.65f,fabsf(sum-3.7f));
    float near=1.0f-smooth(.0f,.22f,fabsf(product-.79f));
    float lum=.02f+.68f*star+.29f*near+.12f*ridge(sum*2.1f);
    SEGMENT.setPixelColorXY(x,y,chroma(sum*42.0f+product*145.0f+t*10.0f+SEGMENT.custom2*.55f,lum));
  }
}
// Experimental GPT21b/GPT22b: sparse, distance-phased ripple on an adaptive dot lattice.
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
static const char m21[] PROGMEM="GPT21b - Sparse Ripple Brightness@Wave Speed,Contrast,Dot Spacing,Color;!,!;!;2;sx=135,ix=205,c1=128";
static const char m22[] PROGMEM="GPT22b - Sparse Ripple Size@Wave Speed,Expansion,Dot Spacing,Color;!,!;!;2;sx=135,ix=205,c1=128";


// GPT23b: crossed light fibers with traveling, independently phase-shifted pulses.
// Sparse luminous threads alternate over/under at every crossing.
static void mode_gpt23() {
  if (!ready() || !tick(32)) return;
  const int w=SEG_W,h=SEG_H;
  const float t=timePhase(.75f), density=5.0f+5.0f*tune1();
  const float u=unitOf(w,h), energy=.25f+.75f*detail();
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    const float vx=p.x*density,vy=p.y*density;
    const float ix=floorf(vx+.5f),iy=floorf(vy+.5f);
    const float dx=fabsf(vx-ix),dy=fabsf(vy-iy);
    const float warp=.11f*sinf(vy*1.2f+t*.8f+ix*.73f);
    const float hThread=1.0f-smooth(.055f,.19f,fabsf(vy-iy+warp));
    const float vThread=1.0f-smooth(.055f,.19f,fabsf(vx-ix+.11f*sinf(vx*.9f-t*.7f+iy*.67f)));
    const float hPulse=ridge(vx*1.85f-t*2.8f+iy*2.1f);
    const float vPulse=ridge(vy*1.85f+t*2.5f+ix*1.7f);
    const bool over=(int(fabsf(ix)+fabsf(iy))&1)!=0;
    const float crossing=(1.0f-smooth(.08f,.29f,dx))*(1.0f-smooth(.08f,.29f,dy));
    const float lum=clip((hThread*(.09f+.7f*hPulse)+vThread*(.09f+.7f*vPulse))*.65f*energy + crossing*(over?hPulse:vPulse)*.30f);
    SEGMENT.setPixelColorXY(x,y,chroma(18.0f*ix+29.0f*iy+t*12.0f+SEGMENT.custom2*.35f,lum));
  }
}
static const char m23[] PROGMEM="GPT23b - Photon Loom@Pulse Speed,Glow,Thread Density,Hue;!,!;!;2;sx=145,ix=190";

// GPT24b: layered hypotrochoid-like luminous traces, moving in opposite directions.
// Uses distance to parametric orbit samples, not a precomputed image or frame buffer.
static void mode_gpt24() {
  if (!ready() || !tick(40)) return;
  const int w=SEG_W,h=SEG_H;
  // Custom2 is now Size. Midpoint (128) matches the dev.4 1.22x scale.
  // Range: 0.70x to 1.735x; motion, petals and glow remain unchanged.
  const float scale=0.70f + 1.04f*tune2();
  const float u=unitOf(w,h)/scale,t=timePhase(.6f),d=detail();
  const float petals=4.0f+floorf(tune1()*6.0f);
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float best=10.0f,bright=0.0f;
    // Two contra-rotating rosettes with analytically modulated radius.
    for(int layer=0;layer<2;++layer) {
      const float a=atan2f(p.y,p.x),r=sqrtf(p.x*p.x+p.y*p.y);
      const float phase=a*(petals+(float)layer)+t*(layer?-.72f:.91f);
      const float target=.48f+.16f*cosf(phase)+.075f*sinf(phase*1.7f+t*.4f);
      const float dist=fabsf(r-target);
      if(dist<best) best=dist;
      const float thread=1.0f-smooth(.012f,.045f+.018f*d,dist);
      const float spark=ridge(a*22.0f+(layer?1.0f:-1.0f)*t*3.0f);
      bright+=thread*(.20f+.48f*spark);
    }
    const float core=(1.0f-smooth(.025f,.12f,sqrtf(p.x*p.x+p.y*p.y)))*.26f;
    const float halo=(1.0f-smooth(.04f,.17f,best))*.11f;
    SEGMENT.setPixelColorXY(x,y,chroma(atan2f(p.y,p.x)*35.0f+t*18.0f,clip((bright+core+halo)*(.45f+.55f*d))));
  }
}
static const char m24[] PROGMEM="GPT24b - Kinetic Rosettes@Orbit Speed,Trace Glow,Petals,Size;!,!;!;2;sx=135,ix=190,c2=128";

// GPT25b: an array of individual glyphs bending in a traveling tidal field.
// Alternating open arcs and square corners preserve negative space between cells.
static void mode_gpt25() {
  if (!ready() || !tick(35)) return;
  const int w=SEG_W,h=SEG_H;
  const float u=unitOf(w,h),t=timePhase(.55f),cells=4.0f+floorf(tune1()*5.0f);
  const float d=detail();
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    const float gx=p.x*cells,gy=p.y*cells;
    const float ix=floorf(gx+.5f),iy=floorf(gy+.5f);
    const float px=gx-ix,py=gy-iy;
    const float tidal=sinf(ix*.64f+iy*.83f-t*1.8f);
    const float theta=.52f*tidal;
    const float xx=px*cosf(theta)-py*sinf(theta);
    const float yy=px*sinf(theta)+py*cosf(theta);
    const float r=sqrtf(xx*xx+yy*yy),angle=atan2f(yy,xx);
    const float arc=1.0f-smooth(.035f,.10f,fabsf(r-(.24f+.045f*tidal)));
    const float opening=smooth(-.25f,.20f,sinf(angle*2.0f+t*.7f+ix+iy));
    const float corners=1.0f-smooth(.035f,.09f,fabsf(fmaxf(fabsf(xx),fabsf(yy))-.28f));
    const float shape=(int(fabsf(ix)+fabsf(iy))&1)?arc*opening:corners*(1.0f-opening*.7f);
    const float pulse=.30f+.70f*(.5f+.5f*tidal);
    const float lum=clip(shape*pulse*(.3f+.7f*d));
    SEGMENT.setPixelColorXY(x,y,chroma(ix*24.0f+iy*13.0f+t*9.0f+SEGMENT.custom2*.4f,lum));
  }
}
static const char m25[] PROGMEM="GPT25b - Tidal Glyphs@Tide Speed,Brightness,Grid Density,Hue;!,!;!;2;sx=125,ix=195";

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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float a=atan2f(p.y,p.x),r=sqrtf(squaref(p.x)+squaref(p.y));
    float idx=frac((a+K_PI)/K_TAU)*16.0f;
    unsigned band=((unsigned)idx)&15U;
    float amount=state.smoothBands[band]/255.0f;
    float coastline=.48f+.42f*amount+.06f*sinf(9.0f*a-t*1.1f);
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

// GPTA02: Harmonic energy drives 16 portals, beat detection adds a light shockwave.
static void mode_audio2() {
  if(!ready() || !tick(35))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& state=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0) memset(&state,0,sizeof(state));
  bool connected=sampleAudio(state);
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.52f),d=detail();
  float pulse=0.0f;
  if(state.lastPulse && (uint32_t)(strip.now-state.lastPulse)<1400U)
    pulse=1.0f-(float)(strip.now-state.lastPulse)/1400.0f;
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r=sqrtf(squaref(p.x)+squaref(p.y)),a=atan2f(p.y,p.x);
    float k=frac((a+K_PI)/K_TAU)*16.0f;
    unsigned sector=((unsigned)k)&15U;
    unsigned next=(sector+1U)&15U;
    float e=(state.smoothBands[sector]*(1.0f-frac(k))+state.smoothBands[next]*frac(k))/255.0f;
    float bend=2.0f*e/(r+.25f);
    float phase=13.0f*r+8.0f*a + bend - t*1.6f;
    float arcs=ridge(phase)+.45f*ridge(24.0f*r-2.5f*bend+t);
    float portal=1.0f-smooth(.016f,.09f,fabsf(r-(.18f+.19f*tune1()+.62f*e)));
    float shock= pulse*(1.0f-smooth(.025f,.09f,fabsf(r-(1.0f-pulse)*1.7f)));
    float lum=.018f+(.10f+.62f*e)*arcs+portal*(.18f+.47f*e)+shock*.9f;
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
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float fx=(p.x+.98f)*8.0f;
    int k=(int)floorf(fx);k=k<0?0:(k>15?15:k);
    float amt=st.smoothBands[k]/255.0f;
    float center=-.80f+1.60f*k/15.0f;
    float braid=center+.16f*sinf(p.y*(6.0f+5.0f*tune1())+t*(1.0f+k*.035f)+k*1.8f)*(.2f+amt);
    float gap=fabsf(p.x-braid);
    float filament=1.0f-smooth(.012f,.075f,gap);
    float sparks=ridge(p.y*19.0f-t*3.4f+k);
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
    float facets=ridge(atan2f(ry,rx)*3.0f+e*4.0f-t*.5f);
    float lum=.025f+edge*(.24f+.69f*e)+filling*(.04f+.39f*e)*facets;
    if(!connected)lum*=.4f;
    SEGMENT.setPixelColorXY(x,y,chroma(band*15.0f+e*57.0f+t*10.0f+SEGMENT.custom2*.55f,lum));
  }
}
static const char ma4[] PROGMEM="GPTA04 - Resonant Tessellator@Drift,Facet,Tile Size,Hue;!,!;!;2v;sx=120,ix=185,si=0";

// GPTA05: 16 audio-fed branching coral arms; peaks radiate traveling spores.
static void mode_audio5() {
  if(!ready() || !tick(36))return;
  if(!SEGENV.allocateData(sizeof(AudioState))){SEGMENT.fill(SEGCOLOR(0));return;}
  AudioState& st=*reinterpret_cast<AudioState*>(SEGENV.data);
  if(SEGENV.call==0)memset(&st,0,sizeof(st));
  bool connected=sampleAudio(st);
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.42f);
  float pulse=0.0f;
  if(st.lastPulse && (uint32_t)(strip.now-st.lastPulse)<1700U)
    pulse=1.0f-(float)(strip.now-st.lastPulse)/1700.0f;
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float r=sqrtf(p.x*p.x+p.y*p.y),a=atan2f(p.y,p.x);
    float coord=frac((a+K_PI)/K_TAU)*16.0f;
    int k=(int)coord;float e=st.smoothBands[k&15]/255.0f;
    float trunk=1.0f-smooth(.03f,.15f,fabsf(sinf(8.0f*a+.27f*sinf(r*7.0f+t))));
    float spines=1.0f-smooth(.03f,.17f,
              fabsf(sinf(23.0f*a+6.0f*r+3.0f*sinf(r*4.0f-t))));
    float reach=1.0f-smooth(.27f+.49f*e,.46f+.70f*e,r);
    float wave=pulse*(1.0f-smooth(.024f,.078f,fabsf(r-(1.0f-pulse)*1.8f)));
    float lum=.014f+reach*(trunk*(.15f+.65f*e)+spines*(.04f+.38f*e))+
              wave*.84f+ridge(r*21.0f-t*3.0f)*reach*.08f;
    if(!connected)lum*=.4f;
    SEGMENT.setPixelColorXY(x,y,chroma(k*19.0f+e*83.0f+r*50.0f+SEGMENT.custom2*.35f,lum));
  }
}
static const char ma5[] PROGMEM="GPTA05 - Synaptic Reef@Drift,Growth,Spines,Color;!,!;!;2v;sx=135,ix=190,si=0";

} // namespace gptfx

class GPTMatrixFXUsermod : public Usermod {
public:
  void setup() override {
    // WLED allocates effect IDs dynamically; UI labels remain zero-padded.
    strip.addEffect(255, &gptfx::mode_gpt1, gptfx::m1);
    strip.addEffect(255, &gptfx::mode_gpt2, gptfx::m2);
    strip.addEffect(255, &gptfx::mode_gpt3, gptfx::m3);
    strip.addEffect(255, &gptfx::mode_gpt4, gptfx::m4);
    strip.addEffect(255, &gptfx::mode_gpt5, gptfx::m5);
    strip.addEffect(255, &gptfx::mode_gpt6, gptfx::m6);
    strip.addEffect(255, &gptfx::mode_gpt7, gptfx::m7);
    strip.addEffect(255, &gptfx::mode_gpt8, gptfx::m8);
    strip.addEffect(255, &gptfx::mode_gpt9, gptfx::m9);
    strip.addEffect(255, &gptfx::mode_gpt10, gptfx::m10);
    strip.addEffect(255, &gptfx::mode_gpt11, gptfx::m11);
    strip.addEffect(255, &gptfx::mode_gpt12, gptfx::m12);
    strip.addEffect(255, &gptfx::mode_gpt13, gptfx::m13);
    strip.addEffect(255, &gptfx::mode_gpt14, gptfx::m14);
    strip.addEffect(255, &gptfx::mode_gpt15, gptfx::m15);
    strip.addEffect(255, &gptfx::mode_gpt16, gptfx::m16);
    strip.addEffect(255, &gptfx::mode_gpt17, gptfx::m17);
    strip.addEffect(255, &gptfx::mode_gpt18, gptfx::m18);
    strip.addEffect(255, &gptfx::mode_gpt19, gptfx::m19);
    strip.addEffect(255, &gptfx::mode_gpt20, gptfx::m20);
    strip.addEffect(255, &gptfx::mode_audio1, gptfx::ma1);
    strip.addEffect(255, &gptfx::mode_audio2, gptfx::ma2);
    strip.addEffect(255, &gptfx::mode_audio3, gptfx::ma3);
    strip.addEffect(255, &gptfx::mode_audio4, gptfx::ma4);
    strip.addEffect(255, &gptfx::mode_audio5, gptfx::ma5);
    // Experimental effects are appended: existing dynamic IDs keep their order.
    strip.addEffect(255, &gptfx::mode_gpt21, gptfx::m21);
    strip.addEffect(255, &gptfx::mode_gpt22, gptfx::m22);
    strip.addEffect(255, &gptfx::mode_gpt23, gptfx::m23);
    strip.addEffect(255, &gptfx::mode_gpt24, gptfx::m24);
    strip.addEffect(255, &gptfx::mode_gpt25, gptfx::m25);
  }
  void loop() override {}
  void addToJsonInfo(JsonObject& root) override {
    JsonObject userInfo = root["u"].to<JsonObject>();
    JsonObject info = userInfo["gptMatrixFX"].to<JsonObject>();
    info["version"] = gptfx::GPTFX_VERSION;
    info["build"] = gptfx::GPTFX_BUILD;
    info["effects"] = 30;
    info["standard"] = 20;
    info["audio"] = 5;
    info["experimental"] = 5;
  }
  uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }
};
static GPTMatrixFXUsermod gptMatrixFX;
REGISTER_USERMOD(gptMatrixFX);
