// Frozen GPT Matrix FX dev.12 implementation of six stage-1 candidate effects.
// Generated from the validated dev.12 source. Do not edit these references.

namespace gptfx {
static void mode_gpt5_dev12_reference() {
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

static void mode_gpt11_dev12_reference() {
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

static void mode_gpt13_dev12_reference() {
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

static void mode_gpt19_dev12_reference() {
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

static void mode_gpt23_dev12_reference() {
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

static void mode_audio5_dev12_reference() {
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

} // namespace gptfx
