/* Frozen GPT24b dev.8 implementation for numerical/visual regression tests.
 * Included from test translation units after gpt_matrix_fx.cpp.
 */
namespace gptfx {
static void mode_gpt24_dev8_reference() {
  if (!ready() || !tick(40)) return;
  const int w=SEG_W,h=SEG_H;
  const float scale=0.70f + 1.04f*tune2();
  const float u=unitOf(w,h)/scale,t=timePhase(.6f),d=detail();
  const float petals=4.0f+floorf(tune1()*6.0f);
  for(int y=0;y<h;++y) {
    const uint32_t rowStartUs = gpt24Tracking ? micros() : 0U;
    for(int x=0;x<w;++x) {
      Point p=normal(x,y,u,w*.5f,h*.5f);
      const float a=atan2f(p.y,p.x);
      const float r=sqrtf(p.x*p.x+p.y*p.y);
      float best=10.0f,bright=0.0f;
      for(int layer=0;layer<2;++layer) {
        const float phase=a*(petals+(float)layer)+t*(layer?-.72f:.91f);
        const float target=.48f+.16f*cosf(phase)+.075f*sinf(phase*1.7f+t*.4f);
        const float dist=fabsf(r-target);
        if(dist<best) best=dist;
        const float thread=1.0f-smooth(.012f,.045f+.018f*d,dist);
        const float spark=ridge(a*22.0f+(layer?1.0f:-1.0f)*t*3.0f);
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
} // namespace gptfx
