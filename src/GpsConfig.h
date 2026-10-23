#pragma once
#include "GpsUbx.h"

// u-blox 6 CFG-RATE, NAV5, SBAS, RXM, MSG and CFG. Only the connected UART's
// message rates are changed; baud, port protocols and reset masks are untouched.
namespace GpsConfig {
enum class Field : uint8_t { Rate, Navigation, Sbas, Power, Gga, Gll, Gsa, Gsv, Rmc, Vtg, Save };
enum class Profile : uint8_t { Keep, Standard, Compact, Custom, Unknown };
enum class Operation : uint8_t { None, Read, Apply, Save };
enum class Phase : uint8_t { Idle, Ready, WaitAck, ReadyReadback, WaitReply };
enum class Result : uint8_t { Idle, Working, Read, PartialRead, Applied, Saved, Failed };
enum class Error : uint8_t { None, Busy, ReadFirst, Invalid, Bandwidth, PowerRate, NoEdits, Rejected, Timeout, WriteFailed, Mismatch };
static constexpr uint8_t KEEP = 255;
static constexpr uint16_t ALL_KNOWN = (1U << 10) - 1;
static constexpr uint32_t RESPONSE_TIMEOUT_MS = 2000;

constexpr uint16_t fieldMask(Field field) { return field == Field::Save ? 0 : (uint16_t)(1U << (uint8_t)field); }
constexpr uint16_t read16(const uint8_t* bytes) { return bytes[0] | ((uint16_t)bytes[1] << 8); }
constexpr void write16(uint8_t* bytes, uint16_t value) { bytes[0] = (uint8_t)value; bytes[1] = (uint8_t)(value >> 8); }
constexpr bool validPeriod(uint16_t ms) { return ms == 200 || ms == 500 || ms == 1000 || ms == 2000 || ms == 5000; }
constexpr bool validModel(uint8_t model) { return model == 0 || (model >= 2 && model <= 8); }
constexpr bool validPower(uint8_t power) { return power == 0 || power == 1 || power == 4; }
constexpr uint8_t messageId(Field field) {
  return field == Field::Rate ? 8 : field == Field::Navigation ? 0x24 :
         field == Field::Sbas ? 0x16 : field == Field::Power ? 0x11 : field == Field::Save ? 9 : 1;
}
constexpr const char* fieldName(Field field) {
  switch (field) {
    case Field::Rate: return "Update rate";
    case Field::Navigation: return "Navigation mode";
    case Field::Sbas: return "SBAS";
    case Field::Power: return "Power mode";
    case Field::Gga: return "NMEA GGA";
    case Field::Gll: return "NMEA GLL";
    case Field::Gsa: return "NMEA GSA";
    case Field::Gsv: return "NMEA GSV";
    case Field::Rmc: return "NMEA RMC";
    case Field::Vtg: return "NMEA VTG";
    default: return "Receiver save";
  }
}
constexpr const char* modelName(uint8_t model) {
  return model == 0 ? "Portable" : model == 2 ? "Stationary" : model == 3 ? "Walking" :
         model == 4 ? "Driving" : model == 5 ? "At sea" : model == 6 ? "Airborne <1g" :
         model == 7 ? "Airborne <2g" : model == 8 ? "Airborne <4g" : "Other";
}
constexpr const char* powerName(uint8_t power) {
  return power == 0 ? "Maximum performance" : power == 4 ? "Eco" : power == 1 ? "Power save" : "Other";
}
constexpr const char* profileName(Profile profile) {
  return profile == Profile::Standard ? "Normal" : profile == Profile::Compact ? "Compact" :
         profile == Profile::Custom ? "Custom" : "Unknown";
}
constexpr const char* errorText(Error error) {
  switch (error) {
    case Error::Busy: return "Receiver busy; wait for completion.";
    case Error::ReadFirst: return "Read receiver settings first.";
    case Error::Invalid: return "Unsupported setting selection.";
    case Error::Bandwidth: return "Above 1 Hz needs Compact output in Adv.";
    case Error::PowerRate: return "Eco / Power save requires 1 Hz here.";
    case Error::NoEdits: return "Select a setting to change.";
    case Error::Rejected: return "Receiver rejected this setting.";
    case Error::Timeout: return "No reply; check GPS RX wiring and power.";
    case Error::WriteFailed: return "GPS command could not be sent.";
    case Error::Mismatch: return "Readback differs from the requested value.";
    default: return "";
  }
}

struct Changes {
  uint16_t periodMs = 0;
  uint8_t model = KEEP, sbas = KEEP, power = KEEP;
  Profile profile = Profile::Keep;
  constexpr bool any() const { return periodMs || model != KEEP || sbas != KEEP || power != KEEP || profile != Profile::Keep; }
};

struct Snapshot {
  uint16_t known = 0;
  uint8_t rate[6]{}, navigation[36]{}, sbas[8]{}, power[2]{}, messages[6]{};
  constexpr bool has(Field field) const { return (known & fieldMask(field)) != 0; }
  constexpr bool coreKnown() const { return (known & 7) == 7; }
  constexpr uint32_t period() const { return (uint32_t)read16(rate) * read16(rate + 2); }
  constexpr uint8_t secondaryRate() const {
    const uint32_t ms = period();
    const uint32_t cycles = ms ? (5000 + ms - 1) / ms : 1;
    return cycles > 255 ? 255 : (uint8_t)cycles;
  }
  constexpr Profile profile() const {
    if ((known & 0x3f0) != 0x3f0 || !has(Field::Rate)) return Profile::Unknown;
    bool normal = true;
    for (auto rateValue : messages) if (rateValue != 1) normal = false;
    if (normal) return Profile::Standard;
    if (messages[0] == 1 && messages[1] == 0 && messages[4] == 1 && messages[5] == 0 &&
        messages[2] >= secondaryRate() && messages[3] >= secondaryRate()) return Profile::Compact;
    return Profile::Custom;
  }
  constexpr bool accept(uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t size, Field& field) {
    if (cls != 6 || !payload) return false;
    uint8_t* destination = nullptr;
    if (id == 8 && size == 6 && read16(payload) && read16(payload + 2)) { field = Field::Rate; destination = rate; }
    else if (id == 0x24 && size == 36) { field = Field::Navigation; destination = navigation; }
    else if (id == 0x16 && size == 8) { field = Field::Sbas; destination = sbas; }
    else if (id == 0x11 && size == 2) { field = Field::Power; destination = power; }
    else if (id == 1 && (size == 3 || size == 8) && payload[0] == 0xf0 && payload[1] < 6) {
      field = (Field)((uint8_t)Field::Gga + payload[1]);
      messages[payload[1]] = payload[size == 3 ? 2 : 3]; // current UART / UART1 target
      known |= fieldMask(field); return true;
    } else return false;
    for (uint16_t i = 0; i < size; ++i) destination[i] = payload[i];
    known |= fieldMask(field); return true;
  }
};

constexpr GpsUbx::Packet poll(Field field) {
  if (field >= Field::Gga && field <= Field::Vtg) {
    const uint8_t payload[] = {0xf0, (uint8_t)((uint8_t)field - (uint8_t)Field::Gga)};
    return GpsUbx::packet(6, 1, payload, 2);
  }
  return field == Field::Save ? GpsUbx::Packet{} : GpsUbx::packet(6, messageId(field));
}

constexpr GpsUbx::Packet setting(Field field, const Snapshot& target) {
  uint8_t payload[36]{};
  size_t size = 0;
  if (field == Field::Rate) { size = 6; for (size_t i = 0; i < size; ++i) payload[i] = target.rate[i]; }
  else if (field == Field::Navigation) {
    size = 36;
    for (size_t i = 0; i < size; ++i) payload[i] = target.navigation[i];
    payload[0] = 1; payload[1] = 0; // NAV5 mask: dynamic model only
  } else if (field == Field::Sbas) { size = 8; for (size_t i = 0; i < size; ++i) payload[i] = target.sbas[i]; }
  else if (field == Field::Power) { size = 2; payload[0] = 8; payload[1] = target.power[1]; }
  else if (field == Field::Save) {
    size = 13;
    payload[4] = 0x1a; // msgConf | navConf | rxmConf; no ioPort, clear or load
    payload[12] = 0x17; // available BBR / flash / EEPROM / SPI flash
  } else {
    size = 3; payload[0] = 0xf0;
    payload[1] = (uint8_t)field - (uint8_t)Field::Gga;
    payload[2] = target.messages[payload[1]];
  }
  return GpsUbx::packet(6, messageId(field), payload, size);
}

constexpr bool same(Field field, const Snapshot& left, const Snapshot& right) {
  if (!left.has(field) || !right.has(field)) return false;
  if (field == Field::Navigation) return left.navigation[2] == right.navigation[2];
  if (field == Field::Power) return left.power[1] == right.power[1];
  if (field >= Field::Gga && field <= Field::Vtg)
    return left.messages[(uint8_t)field - 4] == right.messages[(uint8_t)field - 4];
  const uint8_t* a = field == Field::Rate ? left.rate : left.sbas;
  const uint8_t* b = field == Field::Rate ? right.rate : right.sbas;
  for (size_t i = 0; i < (field == Field::Rate ? 6U : 8U); ++i) if (a[i] != b[i]) return false;
  return true;
}

struct Controller {
  Snapshot current{}, target{};
  Operation operation = Operation::None;
  Phase phase = Phase::Idle;
  Result result = Result::Idle;
  Error error = Error::None;
  Field errorField = Field::Rate;
  uint32_t serial = 0, phaseStartedMs = 0;
  bool canSave = false;
  uint8_t count = 0, index = 0, readErrors = 0;
  struct Step { Field field = Field::Rate; bool write = false; };
  Step steps[10]{};

  constexpr bool busy() const { return phase != Phase::Idle; }
  constexpr Field field() const { return index < count ? steps[index].field : errorField; }
  constexpr bool readyToSend() const { return phase == Phase::Ready || phase == Phase::ReadyReadback; }
  constexpr GpsUbx::Packet outgoing() const {
    if (!readyToSend()) return {};
    return steps[index].write && phase != Phase::ReadyReadback ? setting(field(), target) : poll(field());
  }
  constexpr void begin(Operation op, uint32_t now) {
    operation = op; phase = Phase::Ready; result = Result::Working; error = Error::None;
    count = index = readErrors = 0; phaseStartedMs = now; canSave = false; ++serial;
  }
  constexpr bool read(uint32_t now, bool rateOnly = false) {
    if (busy()) return false;
    begin(Operation::Read, now);
    count = rateOnly ? 1 : 10;
    for (uint8_t i = 0; i < count; ++i) { steps[i] = {(Field)i, false}; current.known &= ~fieldMask((Field)i); }
    return true;
  }
  constexpr Error validate(const Changes& change) const {
    if (busy()) return Error::Busy;
    if (!change.any()) return Error::NoEdits;
    if ((change.periodMs && !validPeriod(change.periodMs)) ||
        (change.model != KEEP && !validModel(change.model)) || (change.sbas != KEEP && change.sbas > 1) ||
        (change.power != KEEP && !validPower(change.power)) || change.profile > Profile::Compact) return Error::Invalid;
    if (((change.periodMs || change.power != KEEP) && !current.has(Field::Rate)) ||
        (change.periodMs && !current.has(Field::Power)) || (change.model != KEEP && !current.has(Field::Navigation)) ||
        (change.sbas != KEEP && !current.has(Field::Sbas)) || (change.power != KEEP && !current.has(Field::Power)) ||
        (change.profile != Profile::Keep && (current.known & 0x3f1) != 0x3f1)) return Error::ReadFirst;
    Snapshot proposed = proposedSettings(change);
    if (proposed.has(Field::Rate)) {
      if (change.profile != Profile::Keep && proposed.period() < 200) return Error::Invalid;
      if ((change.periodMs || change.power != KEEP) && proposed.has(Field::Power) &&
          proposed.power[1] != 0 && proposed.period() != 1000) return Error::PowerRate;
      if (proposed.period() < 1000 && (change.periodMs || change.profile != Profile::Keep)) {
        if (proposed.profile() != Profile::Compact) return Error::Bandwidth;
        if (!proposed.has(Field::Power)) return Error::ReadFirst;
        if (proposed.power[1] != 0) return Error::PowerRate;
      }
    }
    return Error::None;
  }
  constexpr Snapshot proposedSettings(const Changes& change) const {
    Snapshot proposed = current;
    if (change.periodMs) { write16(proposed.rate, change.periodMs); write16(proposed.rate + 2, 1); }
    if (change.model != KEEP) proposed.navigation[2] = change.model;
    if (change.sbas != KEEP) proposed.sbas[0] = (proposed.sbas[0] & 0xfe) | change.sbas;
    if (change.power != KEEP) proposed.power[1] = change.power;
    if (change.profile != Profile::Keep) {
      for (uint8_t i = 0; i < 6; ++i) proposed.messages[i] = 1;
      if (change.profile == Profile::Compact) {
        proposed.messages[1] = proposed.messages[5] = 0;
        proposed.messages[2] = proposed.messages[3] = proposed.secondaryRate();
      }
    }
    return proposed;
  }
  constexpr void addChange(Field selected) {
    if (!same(selected, current, target)) steps[count++] = {selected, true};
  }
  constexpr bool apply(const Changes& change, uint32_t now) {
    const Error failure = validate(change);
    if (failure != Error::None) { if (!busy()) error = failure; return false; }
    target = proposedSettings(change);
    begin(Operation::Apply, now);
    // Wake first; reduce the output stream before increasing the fix rate.
    if (change.power == 0) addChange(Field::Power);
    if (change.profile == Profile::Compact) {
      const uint8_t order[] = {5, 1, 2, 3, 0, 4};
      for (uint8_t i : order) addChange((Field)(i + 4));
    }
    if (change.periodMs) addChange(Field::Rate);
    if (change.model != KEEP) addChange(Field::Navigation);
    if (change.sbas != KEEP) addChange(Field::Sbas);
    if (change.profile == Profile::Standard)
      for (uint8_t i = 4; i < 10; ++i) addChange((Field)i);
    if (change.power != KEEP && change.power != 0) addChange(Field::Power);
    if (!count) finish();
    return true;
  }
  constexpr bool save(uint32_t now) {
    if (busy() || !canSave || current.known != ALL_KNOWN) return false;
    begin(Operation::Save, now); steps[0] = {Field::Save, true}; count = 1;
    return true;
  }
  constexpr void finish() {
    phase = Phase::Idle;
    result = operation == Operation::Save ? Result::Saved : operation == Operation::Apply ? Result::Applied :
             readErrors ? Result::PartialRead : Result::Read;
    canSave = current.known == ALL_KNOWN;
  }
  constexpr void next(uint32_t now) {
    if (++index == count) finish();
    else { phase = Phase::Ready; phaseStartedMs = now; }
  }
  constexpr void fail(Error failure, uint32_t now) {
    error = failure; errorField = field();
    current.known &= ~fieldMask(field()); canSave = false;
    if (operation == Operation::Read && field() != Field::Rate) { ++readErrors; next(now); }
    else { phase = Phase::Idle; result = Result::Failed; }
  }
  constexpr void sent(bool success, uint32_t now) {
    if (!readyToSend()) return;
    if (!success) { fail(Error::WriteFailed, now); return; }
    phase = steps[index].write && phase != Phase::ReadyReadback ? Phase::WaitAck : Phase::WaitReply;
    phaseStartedMs = now;
  }
  constexpr void tick(uint32_t now) {
    if (busy() && (uint32_t)(now - phaseStartedMs) >= RESPONSE_TIMEOUT_MS) fail(Error::Timeout, now);
  }
  constexpr void receive(uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t size, uint32_t now) {
    Field received = Field::Save;
    bool accepted = current.accept(cls, id, payload, size, received);
    if (!busy()) return;
    if (cls == 5 && size == 2 && payload[0] == 6 && payload[1] == messageId(field()) && (id == 0 || id == 1)) {
      if (id == 0 && (phase == Phase::WaitAck || phase == Phase::WaitReply)) { fail(Error::Rejected, now); return; }
      if (id == 1 && phase == Phase::WaitAck) {
        if (operation == Operation::Save) finish();
        else { phase = Phase::ReadyReadback; phaseStartedMs = now; }
      }
    } else if (accepted && phase == Phase::WaitReply && received == field()) {
      if (operation == Operation::Apply && !same(field(), current, target)) { fail(Error::Mismatch, now); return; }
      next(now);
    }
  }
};
} // namespace GpsConfig
