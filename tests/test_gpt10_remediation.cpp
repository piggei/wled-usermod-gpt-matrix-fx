// GPT10 exact-cell-cache + full-packet phase correction vs frozen dev.14.
// All visuals remain host-only; hardware performance is a separate gate.
#include "wled.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <vector>
MockStrip strip;
Usermod* globalMod=nullptr;
bool UsermodManager::audioOn=false;
um_data_t UsermodManager::audio;
#include "../gpt_matrix_fx.cpp"
#include "gpt10_dev14_reference.h"

struct Case {int w,h;uint32_t seconds;uint8_t speed,intensity,custom1,custom2,palette;};
struct Comparison {unsigned maxDelta=0, changed=0, high=0, maxCluster=0;double mean=0;};
static Comparison compare(const Segment& a,const Segment& b){
  Comparison stats;unsigned long long sum=0;
  size_t pixels=(size_t)a.width*a.height;
  std::vector<unsigned char> high(pixels,0),seen(pixels,0);
  for(size_t i=0;i<pixels;++i){
    bool changed=false,large=false;
    for(size_t c=0;c<3;++c){
      int delta=std::abs((int)a.frameBuffer[4*i+c]-(int)b.frameBuffer[4*i+c]);
      stats.maxDelta=std::max(stats.maxDelta,(unsigned)delta);
      changed|=delta>0;large|=delta>16;sum+=(unsigned)delta;
    }
    stats.changed+=changed;stats.high+=large;high[i]=large;
  }
  stats.mean=(double)sum/(3.0*pixels);
  for(size_t i=0;i<pixels;++i){
    if(seen[i]||!high[i])continue;
    std::vector<size_t> q(1,i);seen[i]=1;
    for(size_t k=0;k<q.size();++k){
      size_t v=q[k],x=v%(size_t)a.width,y=v/(size_t)a.width;
      if(x>0 && high[v-1]&&!seen[v-1]){seen[v-1]=1;q.push_back(v-1);}
      if(x+1<(size_t)a.width && high[v+1]&&!seen[v+1]){seen[v+1]=1;q.push_back(v+1);}
      if(y>0 && high[v-a.width]&&!seen[v-a.width]){seen[v-a.width]=1;q.push_back(v-a.width);}
      if(y+1<(size_t)a.height && high[v+a.width]&&!seen[v+a.width]){seen[v+a.width]=1;q.push_back(v+a.width);}
    }
    stats.maxCluster=std::max(stats.maxCluster,(unsigned)q.size());
  }
  return stats;
}
int main(int argc,char**argv){
  FILE* csv=stdout;
  if(argc>1){csv=std::fopen(argv[1],"w");if(!csv)return 1;}
  if(!globalMod)return 2;
  globalMod->setup();
  std::vector<Case> testcases;
  const uint32_t uptime[]={0,16,137,180,600,1319,1800,2214,3600,7200,8543,14400,43200,86400,172800};
  const uint8_t speeds[]={0,120,235,255};
  const uint8_t palettes[]={0,6,8};
  for(uint32_t sec:uptime) for(uint8_t speed:speeds)for(uint8_t pal:palettes)
    testcases.push_back({64,64,sec,speed,190,128,128,pal});
  for(auto shape:{std::pair<int,int>{128,64},{128,128}})
   for(uint32_t sec:{16U,137U,2214U,8543U,43200U,172800U})
    for(uint8_t speed:{0U,235U})
     testcases.push_back({shape.first,shape.second,sec,speed,190,200,44,6});
  std::fprintf(csv,"w,h,seconds,speed,palette,max_delta,changed,high,cluster,mean,hash_requests,hash_evaluations,hash_hits,hash_arg_abs,packet_arg_abs,packet_bounded_abs\n");
  unsigned count=0,maxDelta=0,maxHigh=0,maxCluster=0,maxChanged=0;double maxMean=0;
  bool failed=false;
  for(const Case& c:testcases){
    Segment a(c.w,c.h),b(c.w,c.h);
    for(Segment* seg:{&a,&b}){
      seg->speed=c.speed;seg->intensity=c.intensity;
      seg->custom1=c.custom1;seg->custom2=c.custom2;
      seg->palette=c.palette;seg->call=0;
    }
    strip.now=c.seconds*1000u;
    strip._currentSegment=&a;
    gptfx::mode_gpt10_dev14_reference();
    strip._currentSegment=&b;
    gptfx::mode_gpt10_profiled();
    const Comparison d=compare(a,b);
    auto diag=gptfx::gpt10Diag;
    std::fprintf(csv,"%d,%d,%u,%u,%u,%u,%u,%u,%u,%.7f,%u,%u,%u,%.5f,%.5f,%.5f\n",
      c.w,c.h,c.seconds,c.speed,c.palette,d.maxDelta,d.changed,d.high,d.maxCluster,d.mean,
      diag.hashRequests,diag.hashEvaluations,diag.hashCacheHits,
      diag.hashArgAbs,diag.packetRawAbs,diag.packetBoundedAbs);
    maxDelta=std::max(maxDelta,d.maxDelta);maxChanged=std::max(maxChanged,d.changed);
    maxHigh=std::max(maxHigh,d.high);maxCluster=std::max(maxCluster,d.maxCluster);
    maxMean=std::max(maxMean,d.mean);
    if(diag.hashRequests!=(unsigned)(c.w*c.h) || diag.hashRequests!=diag.hashEvaluations+diag.hashCacheHits){
      std::fprintf(stderr,"HASH_DIAG_BAD %dx%d t=%u\n",c.w,c.h,c.seconds);failed=true;
    }
#if GPTFX_GPT10_CELL_CACHE
    if(diag.hashCacheHits==0){
      std::fprintf(stderr,"HASH_CACHE_NO_HITS %dx%d t=%u\n",c.w,c.h,c.seconds);failed=true;
    }
#else
    if(diag.hashCacheHits!=0){std::fprintf(stderr,"HASH_CACHE_DISABLED_BUT_HITS\n");failed=true;}
#endif
#if !GPTFX_GPT10_PACKET_BOUNDING
    // Hash caching is semantically exact at ALL phases and visual inputs.
    if(d.maxDelta||d.changed){
      std::fprintf(stderr,"GPT10_EXACT_REGRESSION %dx%d t=%u speed=%u diff=%u changed=%u\n",
         c.w,c.h,c.seconds,c.speed,d.maxDelta,d.changed);failed=true;
    }
#else
    if(!std::isfinite(diag.packetBoundedAbs) || diag.packetBoundedAbs>90.0f){
      std::fprintf(stderr,"GPT10_PACKET_UNBOUNDED t=%u bound=%.6f\n",c.seconds,diag.packetBoundedAbs);failed=true;
    }
    // Strict quality on screen-saver times <2h, diagnostic for >2h.
    if(c.seconds<=7200 && (d.mean>0.30 || d.high>(unsigned)(c.w*c.h/100) || d.maxCluster>14)){
      std::fprintf(stderr,"GPT10_VISUAL_ALERT t=%u speed=%u size=%dx%d mean=%.4f high=%u cluster=%u\n",
        c.seconds,c.speed,c.w,c.h,d.mean,d.high,d.maxCluster);
      failed=true;
    }
#endif
    ++count;
  }
  if(argc>1)std::fclose(csv);
  std::fprintf(stderr,"GPT10_%s cases=%u cache=%d packet=%d maxDelta=%u maxChanged=%u maxHigh=%u maxCluster=%u maxMean=%.6f\n",
    failed?"FAIL":"PASS",count,GPTFX_GPT10_CELL_CACHE,GPTFX_GPT10_PACKET_BOUNDING,
    maxDelta,maxChanged,maxHigh,maxCluster,maxMean);
  return failed?3:0;
}
