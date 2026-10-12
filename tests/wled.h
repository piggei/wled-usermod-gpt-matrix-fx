#pragma once
// Minimal host-only API test double, NOT shipped in firmware or searched by PIO.
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <utility>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <map>
// Reproduce the Arduino core macro to detect naming collisions in host tests.
#define sq(x) ((x) * (x))
#define VERSION 2607201
#define PROGMEM
inline uint32_t micros() { static uint32_t clockUs=0; clockUs+=1000U; return clockUs; }
constexpr uint16_t USERMOD_ID_AUDIOREACTIVE = 25;
constexpr uint16_t USERMOD_ID_UNSPECIFIED = 0;
struct um_data_t { unsigned u_size=0; void** u_data=nullptr; };
struct UsermodManager {
  static bool audioOn;
  static um_data_t audio;
  static bool getUMData(um_data_t** data, uint16_t id) {
    if (data && audioOn && id==USERMOD_ID_AUDIOREACTIVE) { *data=&audio; return true; }
    return false;
  }
};
struct Segment {
  int width = 64, height = 64;
  uint8_t speed=128, intensity=180, custom1=135, custom2=150, palette=6;
  uint32_t primaryColor=0xFFA000; // Typical default color; tests also force black.
  uint16_t aux0=0,aux1=0;
  uint32_t step=0,call=0;
  uint8_t* data=nullptr;
  std::vector<uint8_t> frameBuffer;
  std::vector<uint8_t> scratch;
  Segment(int w=64,int h=64):width(w),height(h),frameBuffer((size_t)w*h*4,0) {}
  bool is2D() const {return width>1 && height>1;}
  bool allocateData(size_t n) {
    if(n>16384) return false;
    if(scratch.size()!=n) scratch.assign(n,0);
    data=scratch.data();return true;
  }
  void fill(uint32_t c) {
    for(int y=0;y<height;++y) for(int x=0;x<width;++x) setPixelColorXY(x,y,c);
  }
  void setPixelColorXY(int x,int y,uint32_t c) {
    if(x<0||x>=width||y<0||y>=height) return;
    size_t off=((size_t)y*width+x)*4;
    frameBuffer[off]=(c>>16)&255;
    frameBuffer[off+1]=(c>>8)&255;
    frameBuffer[off+2]=c&255;
    frameBuffer[off+3]=(c>>24)&255;
  }
  uint32_t color_from_palette(uint16_t idx,bool,bool,uint8_t mcol,uint8_t bri=255) const {
    // Match WLED's important special case: the Default palette with mcol=0
    // returns the primary color. Other selections return the palette itself.
    if ((palette==0 && mcol<3) || palette==2 || palette==250) {
      const uint32_t c=(palette==250)?0U:primaryColor;
      return ((uint32_t)(((c>>16)&255U)*bri/255U)<<16) |
             ((uint32_t)(((c>>8)&255U)*bri/255U)<<8) |
             ((uint32_t)((c&255U)*bri/255U));
    }
    // Valid sampled palette: one black band, which the effect must preserve.
    if (palette==8 && idx>=32 && idx<64) return 0;
    uint8_t r=(uint8_t)(((idx*3U+81U+palette)&255U)*bri/255U);
    uint8_t g=(uint8_t)(((idx*5U+43U+palette*2U)&255U)*bri/255U);
    uint8_t b=(uint8_t)(((idx*7U+201U+palette*3U)&255U)*bri/255U);
    return ((uint32_t)r<<16)|((uint32_t)g<<8)|b;
  }
};
struct MockStrip {
  Segment* _currentSegment=nullptr;
  bool isMatrix=true;
  uint32_t now=0;
  struct Fx { uint8_t id; void(*fn)(); const char* name;};
  std::vector<Fx> effects;
  void addEffect(uint8_t id,void (*fn)(),const char* name){effects.push_back({id,fn,name});}
};
extern MockStrip strip;
#define SEGMENT (*strip._currentSegment)
#define SEGENV SEGMENT
#define SEG_W SEGMENT.width
#define SEG_H SEGMENT.height
#define SEGCOLOR(x) (uint32_t)0
inline uint16_t hw_random16(uint16_t range){ static uint32_t seed=114641; seed=seed*1664525U+1013904223U;return (uint16_t)((seed>>8)%range);}
struct JsonObject {
  struct Value {
    std::map<std::string, std::string>* fields;
    std::string key;
    template<typename T> void operator=(T v) { (*fields)[key] = std::to_string(v); }
    void operator=(const char* v) { (*fields)[key] = v; }
    template<typename T> JsonObject to() { return JsonObject{fields, key + "."}; }
  };
  std::map<std::string,std::string>* fields;
  std::string prefix;
  Value operator[](const char* key) { return Value{fields, prefix+key}; }
};
struct Usermod {
  virtual ~Usermod() =default;
  virtual void setup() {}
  virtual void loop() {}
  virtual void addToJsonInfo(JsonObject&) {}
  virtual uint16_t getId(){return USERMOD_ID_UNSPECIFIED;}
};
extern Usermod* globalMod;
#define REGISTER_USERMOD(instance) namespace { struct AutoRegister {AutoRegister(){globalMod=&instance;}} autoRegister; }
