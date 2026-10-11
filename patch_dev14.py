#!/usr/bin/env python3
"""Apply reversible stage-2A temporal phase bounding to dev.13 baseline.
Run once in the project root. Refuse when input does not match expected code.
"""
from pathlib import Path
import re

root=Path(__file__).resolve().parent
p=root/'gpt_matrix_fx.cpp'
s=p.read_text()
assert 'GPTFX_BUILD = "dev.13"' in s
assert 'GPTFX_STAGE2_PHASE_BOUNDING' not in s
original=s
names=[1,2,9,10,14,16]
refs=[]
for n in names:
    pattern=r'(static void mode_gpt'+str(n)+r'\(\) \{)'
    m=re.search(pattern,s)
    assert m, n
    brace=m.end()-1
    dep=0
    end=None
    for i in range(brace,len(s)):
        if s[i]=='{': dep+=1
        elif s[i]=='}':
            dep-=1
            if dep==0:
                end=i+1
                break
    assert end is not None
    raw=s[m.start():end]
    refs.append(raw.replace('mode_gpt'+str(n)+'()', 'mode_gpt'+str(n)+'_dev13_reference()',1))
(root/'tests'/'gpt_stage2_dev13_reference.h').write_text(
    '// Frozen dev.13 originals for six stage-2A effects. Generated from the baseline.\n'
    '// DO NOT EDIT: comparisons run in the same WLED host test process.\n'
    '#pragma once\nnamespace gptfx {\n'+'\n\n'.join(refs)+'\n} // namespace gptfx\n')

def replace_one(old,new):
    global s
    count=s.count(old)
    if count!=1:raise ValueError('wanted once, got %d: %s'%(count,old[:140]))
    s=s.replace(old,new,1)

s=s.replace('Version: 0.3.0-dev.13','Version: 0.3.0-dev.14',1).replace('GPTFX_BUILD = "dev.13"','GPTFX_BUILD = "dev.14"',1)
marker='static inline float labyrinthBoundPhase(float phase) {'
insert='''// Stage-2A phase bounding for GPT01, GPT02, GPT09, GPT10, GPT14, GPT16.
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
'''
replace_one(marker,insert+marker)
replace_one('static inline void profileExtra(const char* effect, void(*fn)()) {', '''static inline void stage2TrackPhase(const float* raw,const float* bounded,unsigned n) {
  stage1TrackPhase(raw,bounded,n);
}
static inline void profileExtra(const char* effect, void(*fn)()) {''')
# GPT01: bounded phases for hot-path warp and ridge; orbit trigs are per-frame.
replace_one('Point p2 = {0.42f*sinf(t*.28f+4.0f), 0.52f*cosf(t*.33f)};','''Point p2 = {0.42f*sinf(t*.28f+4.0f), 0.52f*cosf(t*.33f)};
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[2]={t*.7f,-t*1.4f};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
#endif''')
replace_one('sinf(3.9f*p.x + 2.7f*p.y + t*.7f)', 'sinf(3.9f*p.x + 2.7f*p.y + GPTFX_STAGE2_ARG(t*.7f,phase[0]))')
replace_one('float g = (15.0f+8.0f*lens)*(a+c) - t*1.4f;', 'float g = (15.0f+8.0f*lens)*(a+c) + GPTFX_STAGE2_ARG(-t*1.4f,phase[1]);')
# GPT02: only hot path. seed orbit is only 11*3 trigs per frame.
replace_one('  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {\n    Point p=normal(x,y,u,w*.5f,h*.5f);\n    float first=1000.0f, second=1000.0f; int id=0;', '''#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={-t*1.7f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float first=1000.0f, second=1000.0f; int id=0;''')
replace_one('ridge(14.0f*sqrtf(first)-t*1.7f + id*.4f)', 'ridge(14.0f*sqrtf(first)+GPTFX_STAGE2_ARG(-t*1.7f,phase[0]) + id*.4f)')
# GPT09: retain exact original per-pixel sine in the disabled variant;
# bounded enabled mode calculates the time-only sine once per frame.
replace_one('  int depth=9+(SEGMENT.custom1>>5);', '''  int depth=9+(SEGMENT.custom1>>5);
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[2]={t*.18f,t};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
  const float juliaOffset=.16f*sinf(phase[0]);
#endif''')
replace_one('float zx=p.x+.16f*sinf(t*.18f),zy=p.y;', '''#if GPTFX_STAGE2_PHASE_BOUNDING
    float zx=p.x+juliaOffset,zy=p.y;
#else
    float zx=p.x+.16f*sinf(t*.18f),zy=p.y;
#endif''')
replace_one('ridge(trap*16.0f+t)', 'ridge(trap*16.0f+GPTFX_STAGE2_ARG(t,phase[1]))')
# GPT10: preserve the exact spatial pseudo-random seed and grid geometry.
replace_one('  float scale=4.5f+6.0f*tune1();', '''  float scale=4.5f+6.0f*tune1();
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={-t*4.1f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif''')
replace_one('ridge(lineCoord*5.0f-t*4.1f+seed*8.0f)', 'ridge(lineCoord*5.0f+GPTFX_STAGE2_ARG(-t*4.1f,phase[0])+seed*8.0f)')
# GPT14: stripe and hue still use original, time-growing yy.
replace_one('int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.58f);\n  for(int y=0;y<h;++y)', '''int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.58f);
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={t*.7f+t*.4f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y)''')
replace_one('.18f*ridge(yy+t*.4f)', '''.18f*ridge(GPTFX_STAGE2_ARG(yy+t*.4f,
       (p.y<0?-1.0f:1.0f)*z+phase[0]))''')
# GPT16: independently bound +/-1.6*t, never wrap spatial log/angle.
replace_one('int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.48f);\n  for(int y=0;y<h;++y)', '''int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.48f);
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[2]={-t*1.6f,t*1.6f};
  const float phase[2]={stage2BoundPhase(raw[0]),stage2BoundPhase(raw[1])};
  stage2TrackPhase(raw,phase,2);
#endif
  for(int y=0;y<h;++y)''')
replace_one('ridge(8.0f*a0+8.0f*logf(r0)-t*1.6f)', 'ridge(8.0f*a0+8.0f*logf(r0)+GPTFX_STAGE2_ARG(-t*1.6f,phase[0]))')
replace_one('ridge(8.0f*a1-8.0f*logf(r1)+t*1.6f)', 'ridge(8.0f*a1-8.0f*logf(r1)+GPTFX_STAGE2_ARG(t*1.6f,phase[1]))')
# Stage2 integration and JSON, without changes to pre-existing effects.
needle='// Named wrappers keep registration order unchanged.'
wrapper='''static void mode_gpt1_profiled(){profileExtra("GPT01",mode_gpt1);}
static void mode_gpt2_profiled(){profileExtra("GPT02",mode_gpt2);}
static void mode_gpt9_profiled(){profileExtra("GPT09",mode_gpt9);}
static void mode_gpt10_profiled(){profileExtra("GPT10",mode_gpt10);}
static void mode_gpt14_profiled(){profileExtra("GPT14",mode_gpt14);}
static void mode_gpt16_profiled(){profileExtra("GPT16",mode_gpt16);}
'''
replace_one(needle,wrapper+needle)
for n in names:
    replace_one('&gptfx::mode_gpt'+str(n)+', gptfx::m'+str(n),
                '&gptfx::mode_gpt'+str(n)+'_profiled, gptfx::m'+str(n))
replace_one('    trig["stage1Effects"] = "GPT05,GPT11,GPT13,GPT19,GPT23b,GPTA05";', '''    trig["stage1Effects"] = "GPT05,GPT11,GPT13,GPT19,GPT23b,GPTA05";
    trig["stage2PhaseBounding"] = (bool)GPTFX_STAGE2_PHASE_BOUNDING;
    trig["stage2Effects"] = "GPT01,GPT02,GPT09,GPT10,GPT14,GPT16";''')
p.write_text(s)
lib=root/'library.json'
lib.write_text(lib.read_text().replace('0.3.0-dev.13','0.3.0-dev.14'))
print('DEV14_PATCH_OK original bytes',len(original),'new bytes',len(s),'reference effects',len(refs))
