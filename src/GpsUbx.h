#pragma once
#include <stdint.h>
#include <stddef.h>

namespace GpsUbx {
struct Packet { uint8_t bytes[14]{}; size_t size = 0; };
constexpr Packet ratePacket(unsigned hz) {
  Packet p{};
  if (hz != 0 && hz != 1 && hz != 2 && hz != 5) return p;
  p.size = hz ? 14 : 8;
  p.bytes[0]=0xb5; p.bytes[1]=0x62; p.bytes[2]=6; p.bytes[3]=8;
  p.bytes[4]=hz ? 6 : 0;
  if (hz) {
    const unsigned ms=1000/hz;
    p.bytes[6]=ms & 255; p.bytes[7]=ms >> 8;
    p.bytes[8]=1; p.bytes[10]=1; // navRate=1, GPS time reference
  }
  uint8_t a=0,b=0;
  for (size_t i=2;i<p.size-2;++i) { a+=p.bytes[i]; b+=a; }
  p.bytes[p.size-2]=a; p.bytes[p.size-1]=b;
  return p;
}

// Bounded streaming UBX decoder. Keep binary replies out of the NMEA parser.
struct Decoder {
  uint8_t state=0, cls=0, id=0, a=0,b=0, header=0;
  uint16_t length=0,pos=0;
  uint8_t payload[6]{};
  bool ready=false;
  uint32_t bad=0;
  constexpr void reset() { state=0; ready=false; }
  constexpr bool consume(uint8_t c) {
    ready=false;
    if (state==0) { if(c!=0xb5) return false; state=1; return true; }
    if (state==1) {
      if(c!=0x62) { state=c==0xb5 ? 1 : 0; return c==0xb5; }
      state=2; header=0; length=pos=0; a=b=0; return true;
    }
    if (state==2) {
      a+=c; b+=a;
      if(header==0) cls=c;
      else if(header==1) id=c;
      else if(header==2) length=c;
      else {
        length|=(uint16_t)c<<8;
        if(length>256) { ++bad; reset(); return true; }
        state=length ? 3 : 4;
      }
      ++header; return true;
    }
    if(state==3) {
      a+=c; b+=a;
      if(pos<sizeof(payload)) payload[pos]=c;
      if(++pos==length) state=4;
      return true;
    }
    if(state==4) {
      if(c!=a) { ++bad; reset(); } else state=5;
      return true;
    }
    state=0;
    if(c==b) ready=true; else ++bad;
    return true;
  }
};
} // namespace GpsUbx
