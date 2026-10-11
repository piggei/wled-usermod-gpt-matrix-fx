from pathlib import Path
p=Path(__file__).parent/'gpt_matrix_fx.cpp'
s=p.read_text()
base=s

def rep(old,new):
 global s
 n=s.count(old)
 if n!=1: raise RuntimeError(f'Expected once, got {n}: {old[:90]!r}')
 s=s.replace(old,new,1)

rep('Version: 0.3.0-dev.12','Version: 0.3.0-dev.13')
rep('constexpr const char* GPTFX_BUILD = "dev.12";', 'constexpr const char* GPTFX_BUILD = "dev.13";')
rep('static inline float labyrinthBoundPhase(float phase) {', '''// Stage-1 phase bounding for GPT05, GPT11, GPT13, GPT19, GPT23b, GPTA05.
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
static inline float labyrinthBoundPhase(float phase) {''')
rep('static bool extraTracking=false,extraTickReached=false,extraTickSkipped=false;', '''static bool extraTracking=false,extraTickReached=false,extraTickSkipped=false;
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
}''')
# GPT05: especially keep spatial spiral and palette unchanged, normalize after 2.7 scaling.
rep('  float mass=.25f+.85f*tune1();\n  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {', '''  float mass=.25f+.85f*tune1();
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[3]={-t*.8f,-t*.6f,-t*2.7f};
  const float phase[3]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2])};
  stage1TrackPhase(raw,phase,3);
#endif
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {''')
rep('    float rings=ridge((r+.08f*bend)*16.0f-t*.8f);\n    float stream=ridge(spiral*2.7f+2.5f*sinf(r*6.0f-t*.6f));', '''    float rings=ridge((r+.08f*bend)*16.0f+GPTFX_STAGE1_ARG(-t*.8f,phase[0]));
    float stream=ridge(GPTFX_STAGE1_ARG(
        spiral*2.7f+2.5f*sinf(r*6.0f-t*.6f),
        (angle+bend+r*(4.5f+4.0f*tune2()))*2.7f+phase[2]+2.5f*sinf(r*6.0f+phase[1])));''')
# GPT11
rep('  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.46f),freq=5.0f+7.0f*tune1();\n  for(int y=0;y<h;++y)', '''  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.46f),freq=5.0f+7.0f*tune1();
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[3]={t,-t*.71f,t*.3f};
  const float phase[3]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2])};
  stage1TrackPhase(raw,phase,3);
#endif
  for(int y=0;y<h;++y)''')
rep('    float dx=p.x+.16f*sinf(3.2f*p.y+t),dy=p.y+.16f*cosf(3.6f*p.x-t*.71f);', '''    float dx=p.x+.16f*sinf(3.2f*p.y+GPTFX_STAGE1_ARG(t,phase[0])),
          dy=p.y+.16f*cosf(3.6f*p.x+GPTFX_STAGE1_ARG(-t*.71f,phase[1]));''')
rep('    float over=sinf(freq*(dx+dy)*.50f+t*.3f);', '    float over=sinf(freq*(dx+dy)*.50f+GPTFX_STAGE1_ARG(t*.3f,phase[2]));')
# GPT13
rep('  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.33f);\n  for(int y=0;y<h;++y)', '''  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.33f);
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[5]={t*.31f,-t*.43f,t*.42f,-t*.34f,t};
  const float phase[5]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2]),
                        stage1BoundPhase(raw[3]),stage1BoundPhase(raw[4])};
  stage1TrackPhase(raw,phase,5);
#endif
  for(int y=0;y<h;++y)''')
rep('    float f=p.x+.11f*sinf(5.2f*p.y+t*.31f),g=p.y+.14f*sinf(4.8f*p.x-t*.43f);', '''    float f=p.x+.11f*sinf(5.2f*p.y+GPTFX_STAGE1_ARG(t*.31f,phase[0])),
          g=p.y+.14f*sinf(4.8f*p.x+GPTFX_STAGE1_ARG(-t*.43f,phase[1]));''')
rep('    float crackX=frac(4.2f*f+.37f*sinf(6.2f*g+t*.42f));', '    float crackX=frac(4.2f*f+.37f*sinf(6.2f*g+GPTFX_STAGE1_ARG(t*.42f,phase[2])));')
rep('    float crackY=frac(3.7f*g+.24f*cosf(6.0f*f-t*.34f));', '    float crackY=frac(3.7f*g+.24f*cosf(6.0f*f+GPTFX_STAGE1_ARG(-t*.34f,phase[3])));')
rep('    float b=.07f+.30f*facet+.58f*gold+.08f*ridge(8.0f*f+t);', '    float b=.07f+.30f*facet+.58f*gold+.08f*ridge(8.0f*f+GPTFX_STAGE1_ARG(t,phase[4]));')
# GPT19 requires reducing after frequency multiplication, not t before it.
rep('  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.53f);\n  for(int y=0;y<h;++y)', '''  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.53f);
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
  for(int y=0;y<h;++y)''')
rep('      float marks=1.0f-smooth(.025f,.20f,fabsf(sinf((8.0f+k*3.0f)*an)));', '''      float marks=1.0f-smooth(.025f,.20f,fabsf(sinf(GPTFX_STAGE1_ARG(
          (8.0f+k*3.0f)*an,(8.0f+k*3.0f)*a+phase[k-1]))));''')
rep('    float pointer=1.0f-smooth(.007f,.037f,fabsf(sinf(a-t*.64f)));', '    float pointer=1.0f-smooth(.007f,.037f,fabsf(sinf(a+GPTFX_STAGE1_ARG(-t*.64f,phase[4]))));')
# GPT23
rep('  const float u=unitOf(w,h), energy=.25f+.75f*detail();\n  for(int y=0;y<h;++y)', '''  const float u=unitOf(w,h), energy=.25f+.75f*detail();
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[4]={t*.8f,-t*.7f,-t*2.8f,t*2.5f};
  const float phase[4]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),
                        stage1BoundPhase(raw[2]),stage1BoundPhase(raw[3])};
  stage1TrackPhase(raw,phase,4);
#endif
  for(int y=0;y<h;++y)''')
rep('    const float warp=.11f*sinf(vy*1.2f+t*.8f+ix*.73f);', '    const float warp=.11f*sinf(vy*1.2f+GPTFX_STAGE1_ARG(t*.8f,phase[0])+ix*.73f);')
rep('    const float vThread=1.0f-smooth(.055f,.19f,fabsf(vx-ix+.11f*sinf(vx*.9f-t*.7f+iy*.67f)));', '    const float vThread=1.0f-smooth(.055f,.19f,fabsf(vx-ix+.11f*sinf(vx*.9f+GPTFX_STAGE1_ARG(-t*.7f,phase[1])+iy*.67f)));')
rep('    const float hPulse=ridge(vx*1.85f-t*2.8f+iy*2.1f);', '    const float hPulse=ridge(vx*1.85f+GPTFX_STAGE1_ARG(-t*2.8f,phase[2])+iy*2.1f);')
rep('    const float vPulse=ridge(vy*1.85f+t*2.5f+ix*1.7f);', '    const float vPulse=ridge(vy*1.85f+GPTFX_STAGE1_ARG(t*2.5f,phase[3])+ix*1.7f);')
# GPTA05
rep('  if(st.lastPulse && (uint32_t)(strip.now-st.lastPulse)<1700U)\n    pulse=1.0f-(float)(strip.now-st.lastPulse)/1700.0f;\n  for(int y=0;y<h;++y)', '''  if(st.lastPulse && (uint32_t)(strip.now-st.lastPulse)<1700U)
    pulse=1.0f-(float)(strip.now-st.lastPulse)/1700.0f;
#if GPTFX_STAGE1_PHASE_BOUNDING
  const float raw[3]={t,-t,-t*3.0f};
  const float phase[3]={stage1BoundPhase(raw[0]),stage1BoundPhase(raw[1]),stage1BoundPhase(raw[2])};
  stage1TrackPhase(raw,phase,3);
#endif
  for(int y=0;y<h;++y)''')
rep('    float trunk=1.0f-smooth(.03f,.15f,fabsf(sinf(8.0f*a+.27f*sinf(r*7.0f+t))));', '    float trunk=1.0f-smooth(.03f,.15f,fabsf(sinf(8.0f*a+.27f*sinf(r*7.0f+GPTFX_STAGE1_ARG(t,phase[0])))));')
rep('              fabsf(sinf(23.0f*a+6.0f*r+3.0f*sinf(r*4.0f-t))));', '              fabsf(sinf(23.0f*a+6.0f*r+3.0f*sinf(r*4.0f+GPTFX_STAGE1_ARG(-t,phase[1])))));')
rep('              wave*.84f+ridge(r*21.0f-t*3.0f)*reach*.08f;', '              wave*.84f+ridge(r*21.0f+GPTFX_STAGE1_ARG(-t*3.0f,phase[2]))*reach*.08f;')
# wrappers and registration to enable extraPerformance for these 6 effects
rep('static void mode_gpt7_profiled(){profileExtra("GPT07",mode_gpt7);}', '''static void mode_gpt5_profiled(){profileExtra("GPT05",mode_gpt5);}
static void mode_gpt11_profiled(){profileExtra("GPT11",mode_gpt11);}
static void mode_gpt13_profiled(){profileExtra("GPT13",mode_gpt13);}
static void mode_gpt19_profiled(){profileExtra("GPT19",mode_gpt19);}
static void mode_gpt23_profiled(){profileExtra("GPT23b",mode_gpt23);}
static void mode_audio5_profiled(){profileExtra("GPTA05",mode_audio5);}
static void mode_gpt7_profiled(){profileExtra("GPT07",mode_gpt7);}''')
for name in ('5','11','13','19','23'):
 rep(f'&gptfx::mode_gpt{name}, gptfx::m{name}',f'&gptfx::mode_gpt{name}_profiled, gptfx::m{name}')
rep('&gptfx::mode_audio5, gptfx::ma5','&gptfx::mode_audio5_profiled, gptfx::ma5')
rep('    trig["labyrinthPhaseBounding"] = (bool)GPTFX_GPT18_PHASE_BOUNDING;', '''    trig["labyrinthPhaseBounding"] = (bool)GPTFX_GPT18_PHASE_BOUNDING;
    trig["stage1PhaseBounding"] = (bool)GPTFX_STAGE1_PHASE_BOUNDING;
    trig["stage1Effects"] = "GPT05,GPT11,GPT13,GPT19,GPT23b,GPTA05";''')
p.write_text(s)
# Freeze exact six reference renderers from dev.12 for pixel-by-pixel comparison.
import re
parts=['// Frozen GPT Matrix FX dev.12 implementation of six stage-1 candidate effects.','// Generated from the validated dev.12 source. Do not edit these references.','']
for oldname in ('mode_gpt5','mode_gpt11','mode_gpt13','mode_gpt19','mode_gpt23','mode_audio5'):
 pattern=r'static void '+oldname+r'\(\)\s*\{'
 match=re.search(pattern,base)
 if not match: raise RuntimeError(oldname)
 start=match.start();i=match.end();depth=1
 while depth:
  if base[i]=='{':depth+=1
  if base[i]=='}':depth-=1
  i+=1
 origin=base[start:i]
 parts.append(origin.replace(oldname+'()',oldname+'_dev12_reference()',1))
 parts.append('')
(Path(__file__).parent/'tests/gpt_stage1_dev12_reference.h').write_text('\n'.join(parts))
# upgrade test version expectations; frozen dev12 reference remains byte-exact
for rel in ['tests/test_host.cpp','tests/test_labyrinth_phase.cpp']:
 q=Path(__file__).parent/rel
 t=q.read_text().replace('=="dev.12"','=="dev.13"').replace('!="dev.12"','!="dev.13"')
 q.write_text(t)
print('patched',p,'reference size',len('\n'.join(parts)))
