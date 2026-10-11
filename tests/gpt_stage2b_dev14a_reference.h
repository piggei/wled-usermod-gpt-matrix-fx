// Frozen dev.14a renderers for Stage-2B differential regression.
namespace gptfx {
static void mode_gpt15_dev14a_reference() {
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

static void mode_gpt25_dev14a_reference() {
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

static void mode_audio1_dev14a_reference() {
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

static void mode_audio2_dev14a_reference() {
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

static void mode_audio3_dev14a_reference() {
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

static void mode_audio4_dev14a_reference() {
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

}
