/* Frozen dev.10 references for multi-effect experimental validation. */
#pragma once
namespace gptfx {
static void mode_gpt7_dev10_reference() {
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
static void mode_gpt8_dev10_reference() {
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
static void mode_gpt18_dev10_reference() {
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
static void mode_gpt20_dev10_reference() {
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
} // namespace gptfx
