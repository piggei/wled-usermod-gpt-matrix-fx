// Exact pre-remediation GPT10 implementation from dev.14.
namespace gptfx {
static void mode_gpt10_dev14_reference() {
  if(!ready() || !tick(30))return;
  int w=SEG_W,h=SEG_H;float u=unitOf(w,h),t=timePhase(.65f);
  float scale=4.5f+6.0f*tune1();
#if GPTFX_STAGE2_PHASE_BOUNDING
  const float raw[1]={-t*4.1f};
  const float phase[1]={stage2BoundPhase(raw[0])};
  stage2TrackPhase(raw,phase,1);
#endif
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    Point p=normal(x,y,u,w*.5f,h*.5f);
    float gx=p.x*scale+t*.11f,gy=p.y*scale-t*.06f;
    float ix=floorf(gx),iy=floorf(gy),fx=frac(gx)-.5f,fy=frac(gy)-.5f;
    float seed=frac(sinf(ix*127.1f+iy*311.7f)*43758.5453f);
    float hor=1.0f-smooth(.038f,.13f,fabsf(fy)),ver=1.0f-smooth(.038f,.13f,fabsf(fx));
    float trace=(seed>.34f?hor:ver);
    float joint=(1.0f-smooth(.06f,.2f,sqrtf(fx*fx+fy*fy)));
    float lineCoord=(seed>.34f?gx:gy)*.65f;
    float packet=ridge(lineCoord*5.0f+GPTFX_STAGE2_ARG(-t*4.1f,phase[0])+seed*8.0f);
    float lum=.022f+trace*(.14f+.65f*packet)+joint*(.25f+.45f*detail());
    SEGMENT.setPixelColorXY(x,y,chroma(seed*210.0f+packet*48.0f+SEGMENT.custom2*.35f,lum));
  }
}
}
