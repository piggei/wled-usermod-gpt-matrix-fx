// Frozen dev.13 originals for six stage-2A effects. Generated from the baseline.
// DO NOT EDIT: comparisons run in the same WLED host test process.
#pragma once
namespace gptfx {
static void mode_gpt1_dev13_reference() {
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

static void mode_gpt2_dev13_reference() {
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

static void mode_gpt9_dev13_reference() {
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

static void mode_gpt10_dev13_reference() {
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

static void mode_gpt14_dev13_reference() {
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

static void mode_gpt16_dev13_reference() {
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
} // namespace gptfx
