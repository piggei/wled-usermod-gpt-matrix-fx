// Frozen dev.7 GPT24b reference for pixel-by-pixel visual equivalence tests.
// This file is test-only and is not compiled into firmware.
namespace gptfx {
static void mode_gpt24_dev7_reference() {
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
}
