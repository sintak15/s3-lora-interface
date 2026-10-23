#pragma once
#include <stdint.h>
#include <stddef.h>

namespace GpsUbx {
static constexpr size_t MAX_CONFIG_PAYLOAD = 36;
struct Packet { uint8_t bytes[MAX_CONFIG_PAYLOAD + 8]{}; size_t size = 0; };
constexpr Packet packet(uint8_t cls, uint8_t id, const uint8_t* payload = nullptr, size_t length = 0) {
  Packet p{};
  if (length > MAX_CONFIG_PAYLOAD || (length && !payload)) return p;
  p.size = length + 8;
  p.bytes[0] = 0xb5; p.bytes[1] = 0x62; p.bytes[2] = cls; p.bytes[3] = id;
  p.bytes[4] = (uint8_t)length;
  for (size_t i = 0; i < length; ++i) p.bytes[6 + i] = payload[i];
  uint8_t a = 0, b = 0;
  for (size_t i = 2; i < p.size - 2; ++i) { a += p.bytes[i]; b += a; }
  p.bytes[p.size - 2] = a; p.bytes[p.size - 1] = b;
  return p;
}
constexpr Packet ratePacket(unsigned hz) {
  if (hz != 0 && hz != 1 && hz != 2 && hz != 5) return {};
  if (!hz) return packet(6, 8);
  const unsigned ms = 1000 / hz;
  const uint8_t payload[] = {(uint8_t)ms, (uint8_t)(ms >> 8), 1, 0, 1, 0};
  return packet(6, 8, payload, sizeof(payload));
}

// Bounded streaming UBX decoder. Keep binary replies out of the NMEA parser.
struct Decoder {
  uint8_t state=0, cls=0, id=0, a=0,b=0, header=0;
  uint16_t length=0,pos=0;
  uint8_t payload[MAX_CONFIG_PAYLOAD]{};
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
