#pragma once
#include "../GpsUbx.h"
namespace GpsUbxTests {
constexpr auto poll=GpsUbx::ratePacket(0);
constexpr auto one=GpsUbx::ratePacket(1);
constexpr auto five=GpsUbx::ratePacket(5);
static_assert(poll.size==8 && poll.bytes[6]==0x0e && poll.bytes[7]==0x30,"CFG-RATE poll checksum");
static_assert(one.size==14 && one.bytes[6]==0xe8 && one.bytes[7]==3 && one.bytes[12]==1 && one.bytes[13]==0x39,"1 Hz UBX reference vector");
static_assert(five.bytes[6]==200 && five.bytes[7]==0 && GpsUbx::ratePacket(3).size==0,"Rate validation");
constexpr bool roundTrip() {
  GpsUbx::Decoder d;
  if(d.consume('$')) return false;
  for(size_t i=0;i<one.size;++i) if(!d.consume(one.bytes[i])) return false;
  return d.ready && d.cls==6 && d.id==8 && d.length==6 && d.payload[0]==0xe8 && d.payload[1]==3;
}
constexpr bool rejectCorruptAndRecover() {
  GpsUbx::Decoder d;
  for(size_t i=0;i<one.size;++i) d.consume(one.bytes[i]^(i==one.size-1 ? 1 : 0));
  if(d.ready || d.bad!=1) return false;
  for(size_t i=0;i<five.size;++i) d.consume(five.bytes[i]);
  return d.ready && d.payload[0]==200 && !d.consume('$');
}
constexpr bool rejectOversize() {
  GpsUbx::Decoder d;
  const uint8_t bytes[]={0xb5,0x62,6,8,0xff,0xff};
  for(auto c:bytes) d.consume(c);
  return !d.state && d.bad==1 && !d.consume('$');
}
static_assert(roundTrip() && rejectCorruptAndRecover() && rejectOversize(),"Bounded UBX parser and NMEA recovery");
}
