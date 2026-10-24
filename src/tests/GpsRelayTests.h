#pragma once
#include "../GpsRelay.h"

namespace GpsRelayTests {
using namespace GpsRelay;
constexpr Fix sample(uint32_t now, uint32_t sequence = 1) {
  Fix fix; fix.valid = true; fix.capturedMs = now; fix.sequence = sequence;
  fix.latitude = 449770000; fix.longitude = -932650000;
  return fix;
}
constexpr bool gatesAndInvalidation() {
  Controller relay; Fix fix = sample(0);
  if (relay.due(fix,0,true,true,true)) return false;
  relay.setEnabled(true);
  if (!relay.due(fix,0,true,true,true) || relay.due(fix,0,false,true,true) ||
      relay.due(fix,0,true,false,true) || relay.due(fix,0,true,true,false)) return false;
  if (relay.due(fix,FIX_MAX_AGE_MS+1,true,true,true)) return false;
  fix.valid = false;
  if (relay.due(fix,1,true,true,true)) return false;
  fix = sample(0); fix.latitude = 0; fix.longitude = 0;
  if (!fix.fresh(1)) return false; // Equator / Greenwich are valid coordinates.
  fix.longitude = 1800000001;
  return !fix.fresh(1);
}
constexpr bool deliveryAndCadence() {
  Controller relay; relay.setEnabled(true);
  relay.sent(42,1,0,true);
  if (relay.queueResult(43,0) || relay.accepted || !relay.pendingId) return false;
  if (!relay.queueResult(42,0) || relay.accepted != 1) return false;
  if (relay.queueResult(42,0) || relay.accepted != 1) return false;
  auto next = sample(14999,2);
  if (relay.due(next,14999,true,true,true)) return false;
  if (!relay.due(next,15000,true,true,true)) return false;
  relay.sent(44,2,15000,true);
  if (!relay.queueResult(44,5) || relay.rejected != 1 || relay.lastResult != State::REJECTED) return false;
  relay.setEnabled(false); relay.setEnabled(true);
  return !relay.due(sample(15001,3),15001,true,true,true);
}
constexpr bool timeoutAndWrap() {
  Controller relay; relay.setEnabled(true);
  relay.sent(10,1,UINT32_MAX-2000,true);
  relay.tick(2998);
  if (!relay.pendingId || relay.timeouts) return false;
  relay.tick(2999);
  if (relay.pendingId || relay.timeouts != 1 || relay.lastResult != State::TIMEOUT) return false;
  if (relay.due(sample(3000,2),3000,true,true,true)) return false;
  if (!relay.due(sample(13000,3),13000,true,true,true)) return false;
  relay.sent(11,3,13000,false);
  return relay.writeFailures == 1 && !relay.pendingId && relay.queued == 1;
}
constexpr bool packetIsLocal() {
  auto message = localMessage(0x12345678,123);
  const auto& p = message.packet;
  return message.which_payload_variant == meshtastic_ToRadio_packet_tag &&
         p.to == 0x12345678 && p.id == 123 && !p.from && !p.want_ack && !p.hop_limit && !p.channel &&
         p.decoded.portnum == meshtastic_PortNum_POSITION_APP && !p.decoded.want_response &&
         !localMessage(0,123).which_payload_variant && !localMessage(UINT32_MAX,123).which_payload_variant &&
         !localMessage(1,0).which_payload_variant;
}
constexpr bool dataAndConfig() {
  auto fix = sample(0); fix.epoch = 1792771200; fix.hasAltitude = true; fix.altitude = -12;
  fix.hasSpeed = true; fix.speed = 11; fix.hasTrack = true; fix.track = 9000000;
  auto p = position(fix);
  if (!p.has_latitude_i || !p.has_longitude_i || p.latitude_i != fix.latitude || p.longitude_i != fix.longitude ||
      p.location_source != meshtastic_Position_LocSource_LOC_EXTERNAL || p.altitude != -12 ||
      p.altitude_source != meshtastic_Position_AltSource_ALT_EXTERNAL || p.ground_speed != 11 || p.ground_track != 9000000 ||
      p.time != fix.epoch || p.timestamp != fix.epoch || p.fix_type || p.fix_quality || p.sats_in_view) return false;
  meshtastic_Config_PositionConfig config = meshtastic_Config_PositionConfig_init_zero;
  config.gps_mode = meshtastic_Config_PositionConfig_GpsMode_ENABLED;
  config.gps_enabled = true; config.fixed_position = true;
  config.rx_gpio = 5; config.tx_gpio = 4; config.position_broadcast_secs = 900;
  config.gps_update_interval = 30; config.position_flags = 0x1234;
  config.position_broadcast_smart_enabled = true;
  auto target = externalConfig(config);
  return physicalGpsAbsent(target) && !target.gps_enabled && target.rx_gpio == 5 && target.tx_gpio == 4 &&
         target.position_broadcast_secs == 900 && target.gps_update_interval == 30 && target.position_flags == 0x1234 &&
         target.position_broadcast_smart_enabled;
}
static_assert(gatesAndInvalidation(), "Only fresh valid fixes with a configured local destination may be forwarded");
static_assert(deliveryAndCadence(), "Match queue results and preserve rate limits across toggles and rejections");
static_assert(timeoutAndWrap(), "Timeout and retry cadence survive millis wrap and write failure");
static_assert(packetIsLocal(), "GPS input must never directly become a mesh broadcast");
constexpr bool asynchronousSetupReadback() {
  SetupTimer timer;
  if (timer.ready(10000) || timer.expired(10000)) return false;
  timer.beginWrite(UINT32_MAX - 500);
  if (timer.ready(498) || !timer.ready(499)) return false;
  timer.wait(10000); // A reboot / stale read restarts settling, not the overall deadline.
  return !timer.ready(10999) && timer.ready(11000) && !timer.expired(29498) && timer.expired(29499);
}
static_assert(asynchronousSetupReadback(), "Wait for asynchronous config apply; bounded read retries never extend the write deadline");
static_assert(dataAndConfig(), "Preserve units, unknown fields and unrelated Heltec settings");
static_assert(usableStatus(0,"A") && !usableStatus(0,"V") && usableStatus(1,"1") && !usableStatus(1,"0") &&
              !usableStatus(1,"6") && !usableStatus(1,"") && usableStatus(2,"3") && !usableStatus(2,"1"),
              "Reject no-fix and estimated/manual/simulated positions");
static_assert(utcEpoch(2024,2,29,0,0,0) == 1709164800 && utcEpoch(2026,10,23,12,0,0) == 1792756800 &&
              utcEpoch(2023,2,29,0,0,0) == 0 && utcEpoch(2026,0,1,0,0,0) == 0 &&
              utcEpoch(2026,1,1,24,0,0) == 0, "GPS timestamps use UTC and validate calendar fields");
} // namespace GpsRelayTests
