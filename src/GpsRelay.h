#pragma once
#include <stdint.h>
#include "meshtastic/admin.pb.h"
#include "meshtastic/portnums.pb.h"

namespace GpsRelay {
static constexpr uint32_t FIX_MAX_AGE_MS = 6000;
static constexpr uint32_t SEND_INTERVAL_MS = 15000; // Heltec 2.7.15 throttles POSITION_APP to 10 s.
static constexpr uint32_t REPLY_TIMEOUT_MS = 5000;
static constexpr uint32_t LINK_MAX_AGE_MS = 45000;
static constexpr uint32_t SETUP_SETTLE_MS = 1000;
static constexpr uint32_t SETUP_TIMEOUT_MS = 30000;

struct SetupTimer {
  bool written = false;
  uint32_t writeMs = 0, waitMs = 0;
  constexpr void beginWrite(uint32_t now) { written = true; writeMs = waitMs = now; }
  constexpr void wait(uint32_t now) { waitMs = now; }
  constexpr bool ready(uint32_t now) const { return written && (uint32_t)(now - waitMs) >= SETUP_SETTLE_MS; }
  constexpr bool expired(uint32_t now) const { return written && (uint32_t)(now - writeMs) >= SETUP_TIMEOUT_MS; }
};

constexpr bool usableStatus(unsigned kind, const char* value) {
  if (!value || !value[0] || value[1]) return false;
  return kind == 0 ? value[0] == 'A' : kind == 1 ? (value[0] >= '1' && value[0] <= '5') :
         kind == 2 && (value[0] == '2' || value[0] == '3');
}

constexpr meshtastic_ToRadio localMessage(uint32_t node, uint32_t id) {
  meshtastic_ToRadio message = meshtastic_ToRadio_init_zero;
  if (!node || node == UINT32_MAX || !id) return message;
  message.which_payload_variant = meshtastic_ToRadio_packet_tag;
  message.packet.to = node; message.packet.id = id;
  message.packet.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
  message.packet.decoded.portnum = meshtastic_PortNum_POSITION_APP;
  return message;
}

struct Fix {
  bool valid = false;
  uint32_t capturedMs = 0, sequence = 0, epoch = 0;
  int32_t latitude = 0, longitude = 0, altitude = 0;
  bool hasAltitude = false, hasSpeed = false, hasTrack = false;
  uint32_t speed = 0, track = 0, hdop = 0, quality = 0, type = 0, satellitesInView = 0;
  constexpr bool fresh(uint32_t now) const {
    return valid && (uint32_t)(now - capturedMs) <= FIX_MAX_AGE_MS &&
           latitude >= -900000000 && latitude <= 900000000 &&
           longitude >= -1800000000 && longitude <= 1800000000;
  }
};

constexpr bool leap(unsigned year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }
constexpr uint32_t utcEpoch(unsigned year, unsigned month, unsigned day, unsigned hour, unsigned minute, unsigned second) {
  if (year < 2020 || year > 2099 || month < 1 || month > 12 || !day || hour > 23 || minute > 59 || second > 59) return 0;
  const unsigned lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (day > lengths[month - 1] + (month == 2 && leap(year) ? 1U : 0U)) return 0;
  uint32_t days = 0;
  for (unsigned y = 1970; y < year; ++y) days += leap(y) ? 366 : 365;
  for (unsigned m = 1; m < month; ++m) days += lengths[m - 1] + (m == 2 && leap(year) ? 1 : 0);
  return ((days + day - 1) * 24 + hour) * 3600 + minute * 60 + second;
}

constexpr meshtastic_Position position(const Fix& fix) {
  meshtastic_Position p = meshtastic_Position_init_zero;
  p.has_latitude_i = p.has_longitude_i = true;
  p.latitude_i = fix.latitude; p.longitude_i = fix.longitude;
  p.has_altitude = fix.hasAltitude; p.altitude = fix.altitude;
  p.location_source = meshtastic_Position_LocSource_LOC_EXTERNAL;
  if (fix.hasAltitude) p.altitude_source = meshtastic_Position_AltSource_ALT_EXTERNAL;
  p.time = p.timestamp = fix.epoch;
  p.HDOP = fix.hdop; p.fix_quality = fix.quality; p.fix_type = fix.type;
  p.sats_in_view = fix.satellitesInView;
  // Match the installed Heltec 2.7.15 GPS parser. Its implementation uses km/h
  // and degrees * 1e5 despite the older protobuf field comments.
  p.has_ground_speed = fix.hasSpeed; p.ground_speed = fix.speed;
  p.has_ground_track = fix.hasTrack; p.ground_track = fix.track;
  p.seq_number = fix.sequence; p.next_update = SEND_INTERVAL_MS / 1000;
  return p;
}

constexpr bool physicalGpsAbsent(const meshtastic_Config_PositionConfig& config) {
  return config.gps_mode == meshtastic_Config_PositionConfig_GpsMode_NOT_PRESENT && !config.fixed_position;
}
constexpr meshtastic_Config_PositionConfig externalConfig(meshtastic_Config_PositionConfig config) {
  config.gps_mode = meshtastic_Config_PositionConfig_GpsMode_NOT_PRESENT;
  config.gps_enabled = false;
  config.fixed_position = false;
  return config;
}

enum class State : uint8_t { OFF, NO_LINK, NEED_CONFIG, NEED_SETUP, NO_FIX, WAITING, PENDING, ACCEPTED, REJECTED, TIMEOUT, WRITE_FAILED };
constexpr const char* stateText(State state) {
  switch (state) {
    case State::OFF: return "Off";
    case State::NO_LINK: return "Waiting for Heltec link";
    case State::NEED_CONFIG: return "Read Heltec settings first";
    case State::NEED_SETUP: return "Use CYD GPS setup required";
    case State::NO_FIX: return "Waiting for a fresh GPS fix";
    case State::WAITING: return "Waiting for next update";
    case State::PENDING: return "Waiting for Heltec queue result";
    case State::ACCEPTED: return "Heltec accepted last update";
    case State::REJECTED: return "Heltec rejected last update";
    case State::TIMEOUT: return "No Heltec queue result";
    default: return "Serial write failed";
  }
}

struct Controller {
  bool enabled = false, attempted = false;
  uint32_t lastAttemptMs = 0, pendingId = 0, pendingSinceMs = 0, lastSequence = 0;
  uint32_t queued = 0, accepted = 0, rejected = 0, timeouts = 0, writeFailures = 0;
  State lastResult = State::WAITING;
  constexpr void resetLink() { pendingId = 0; lastSequence = 0; }
  constexpr void setEnabled(bool value) { enabled = value; pendingId = 0; }
  constexpr void tick(uint32_t now) {
    if (pendingId && (uint32_t)(now - pendingSinceMs) >= REPLY_TIMEOUT_MS) {
      pendingId = 0; ++timeouts; lastResult = State::TIMEOUT;
    }
  }
  constexpr State state(const Fix& fix, uint32_t now, bool link, bool known, bool ready) const {
    if (!enabled) return State::OFF;
    if (!link) return State::NO_LINK;
    if (!known) return State::NEED_CONFIG;
    if (!ready) return State::NEED_SETUP;
    if (!fix.fresh(now)) return State::NO_FIX;
    if (pendingId) return State::PENDING;
    return lastResult;
  }
  constexpr bool due(const Fix& fix, uint32_t now, bool link, bool known, bool ready) const {
    return enabled && link && known && ready && fix.fresh(now) && !pendingId &&
           (!attempted || (uint32_t)(now - lastAttemptMs) >= SEND_INTERVAL_MS) &&
           (!attempted || fix.sequence != lastSequence);
  }
  constexpr void sent(uint32_t id, uint32_t sequence, uint32_t now, bool success) {
    attempted = true; lastAttemptMs = now;
    if (success && id) {
      ++queued; pendingId = id; pendingSinceMs = now; lastSequence = sequence; lastResult = State::PENDING;
    } else { ++writeFailures; lastResult = State::WRITE_FAILED; }
  }
  constexpr bool queueResult(uint32_t id, int32_t result) {
    if (!pendingId || id != pendingId) return false;
    pendingId = 0;
    if (result == 0) { ++accepted; lastResult = State::ACCEPTED; }
    else { ++rejected; lastResult = State::REJECTED; }
    return true;
  }
};
} // namespace GpsRelay
