#pragma once
#include "../GpsConfig.h"

namespace GpsConfigTests {
using namespace GpsConfig;

constexpr Snapshot receiver() {
  Snapshot s;
  s.known = ALL_KNOWN;
  write16(s.rate, 1000); write16(s.rate + 2, 1); write16(s.rate + 4, 1);
  s.navigation[2] = 0; s.navigation[3] = 3; s.navigation[12] = 10;
  s.sbas[0] = 3; s.sbas[1] = 7; s.sbas[2] = 3; s.sbas[4] = 0x80;
  s.power[0] = 8;
  for (auto& value : s.messages) value = 1;
  return s;
}

constexpr bool maskedNavigationAndSbas() {
  Controller c; c.current = receiver();
  Changes change; change.model = 3; change.sbas = 0;
  if (!c.apply(change, 0) || c.count != 2) return false;
  auto p = c.outgoing();
  if (p.size != 44 || p.bytes[3] != 0x24 || p.bytes[6] != 1 || p.bytes[7] ||
      p.bytes[8] != 3 || p.bytes[9] != 3 || p.bytes[18] != 10) return false;
  const auto sbas = setting(Field::Sbas, c.target);
  return sbas.bytes[6] == 2 && sbas.bytes[7] == 7 && sbas.bytes[8] == 3 && sbas.bytes[10] == 0x80 &&
         c.current.navigation[2] == 0; // A queued request must not change confirmed state.
}

constexpr bool ackRequiresReadback() {
  Controller c; c.current = receiver();
  Changes change; change.model = 3;
  c.apply(change, 0); c.sent(true, 1);
  const uint8_t wrong[] = {6, 8}, correct[] = {6, 0x24};
  c.receive(5, 1, wrong, 2, 2);
  if (c.phase != Phase::WaitAck || c.current.navigation[2] != 0) return false;
  c.receive(5, 1, correct, 2, 3);
  if (c.phase != Phase::ReadyReadback || c.outgoing().size != 8 || c.current.navigation[2] != 0) return false;
  c.sent(true, 4);
  c.receive(6, 0x24, c.target.navigation, 36, 5);
  return !c.busy() && c.result == Result::Applied && c.current.navigation[2] == 3 && c.canSave;
}

constexpr bool rejectedOrMismatchedCannotSave(bool reject) {
  Controller c; c.current = receiver();
  Changes change; change.model = 4;
  c.apply(change, 0); c.sent(true, 1);
  const uint8_t ack[] = {6, 0x24};
  c.receive(5, reject ? 0 : 1, ack, 2, 2);
  if (!reject) {
    c.sent(true, 3);
    const auto old = receiver();
    c.receive(6, 0x24, old.navigation, 36, 4);
  }
  return !c.busy() && c.result == Result::Failed && !c.canSave && !c.save(10) &&
         c.error == (reject ? Error::Rejected : Error::Mismatch);
}

constexpr bool compactBeforeFasterRate() {
  Controller c; c.current = receiver();
  Changes change; change.periodMs = 200;
  if (c.validate(change) != Error::Bandwidth) return false;
  change.profile = Profile::Compact;
  if (!c.apply(change, 0) || c.count != 5) return false;
  if (c.steps[0].field != Field::Vtg || c.steps[1].field != Field::Gll || c.steps[4].field != Field::Rate) return false;
  const auto packet = c.outgoing();
  if (packet.size != 11 || packet.bytes[6] != 0xf0 || packet.bytes[7] != 5 || packet.bytes[8] != 0) return false;
  return c.target.messages[0] == 1 && c.target.messages[4] == 1 &&
         c.target.messages[2] == 25 && c.target.messages[3] == 25 && c.target.profile() == Profile::Compact;
}

constexpr bool normalAfterSlowerRate() {
  Controller c; c.current = receiver();
  write16(c.current.rate, 200);
  c.current.messages[1] = c.current.messages[5] = 0;
  c.current.messages[2] = c.current.messages[3] = 25;
  Changes change; change.profile = Profile::Standard;
  if (c.validate(change) != Error::Bandwidth) return false;
  change.periodMs = 1000;
  return c.apply(change, 0) && c.steps[0].field == Field::Rate;
}

constexpr bool powerConstraints() {
  Controller c; c.current = receiver();
  Changes change; change.periodMs = 500; change.power = 1; change.profile = Profile::Compact;
  if (c.validate(change) != Error::PowerRate) return false;
  c.current.power[1] = 4;
  change.power = 0;
  return c.apply(change, 0) && c.steps[0].field == Field::Power && c.outgoing().bytes[6] == 8;
}

constexpr bool timeoutWrapAndBusy() {
  Controller c;
  if (!c.read(UINT32_MAX - 1000) || c.read(1)) return false;
  c.sent(true, UINT32_MAX - 1000);
  c.tick(998);
  if (!c.busy()) return false;
  c.tick(999);
  return !c.busy() && c.error == Error::Timeout && !c.canSave && !c.outgoing().size;
}

constexpr bool preservePortAndSaveMasks() {
  const auto packet = setting(Field::Save, receiver());
  if (packet.size != 21 || packet.bytes[10] != 0x1a || packet.bytes[18] != 0x17) return false;
  for (uint8_t i = 6; i < 18; ++i) if (i != 10 && packet.bytes[i]) return false;
  Controller c; c.current = receiver(); c.canSave = true;
  if (!c.save(0)) return false;
  c.sent(true, 1);
  const uint8_t ack[] = {6, 9};
  c.receive(5, 1, ack, 2, 2);
  return c.result == Result::Saved && !c.busy();
}

constexpr bool messageReadbackTargetsUart1() {
  Snapshot s; Field field = Field::Save;
  const uint8_t values[] = {0xf0, 4, 9, 1, 8, 7, 6, 5};
  if (!s.accept(6, 1, values, sizeof(values), field)) return false;
  return field == Field::Rmc && s.messages[4] == 1 && !s.has(Field::Gga);
}

constexpr bool fullPayloadAndMalformedFrames() {
  const auto expected = receiver();
  auto packet = setting(Field::Navigation, expected);
  GpsUbx::Decoder decoder;
  for (size_t i = 0; i < packet.size; ++i) decoder.consume(packet.bytes[i]);
  if (!decoder.ready || decoder.length != 36 || decoder.payload[12] != 10) return false;
  decoder.consume('$');
  return !decoder.ready && !decoder.state && !GpsUbx::packet(6, 0x24, expected.navigation, 37).size;
}

constexpr bool readSequence(bool partial) {
  Controller c;
  c.read(0);
  const auto actual = receiver();
  for (uint8_t i = 0; i < 10; ++i) {
    if (c.field() != (Field)i || !c.readyToSend()) return false;
    c.sent(true, i * 10 + 1);
    if (partial && c.field() == Field::Power) {
      const uint8_t nak[] = {6, 0x11};
      c.receive(5, 0, nak, 2, i * 10 + 2);
    } else {
      const auto reply = setting(c.field(), actual);
      c.receive(reply.bytes[2], reply.bytes[3], reply.bytes + 6, reply.size - 8, i * 10 + 2);
    }
  }
  if (c.busy() || c.result != (partial ? Result::PartialRead : Result::Read)) return false;
  if (!partial) return c.current.known == ALL_KNOWN && c.canSave;
  if (!c.current.coreKnown() || c.canSave || c.save(101)) return false;
  // Refreshing one field must not make unknown power settings safe to save.
  c.read(110, true); c.sent(true, 111);
  c.receive(6, 8, actual.rate, 6, 112);
  return !c.canSave && !c.save(113);
}

constexpr bool failedSequenceRetainsConfirmedChanges() {
  Controller c; c.current = receiver();
  Changes change; change.model = 3; change.sbas = 0;
  c.apply(change, 0); c.sent(true, 1);
  const uint8_t ack[] = {6, 0x24};
  c.receive(5, 1, ack, 2, 2); c.sent(true, 3);
  c.receive(6, 0x24, c.target.navigation, 36, 4);
  c.sent(false, 5);
  return !c.busy() && c.result == Result::Failed && c.current.navigation[2] == 3 &&
         c.current.has(Field::Navigation) && !c.current.has(Field::Sbas) && !c.canSave && !c.outgoing().size;
}

constexpr bool unknownRateOrPowerPreventsUnsafeChanges() {
  Controller c; c.current = receiver();
  Changes change; change.power = 4;
  c.current.known &= ~fieldMask(Field::Rate);
  if (c.validate(change) != Error::ReadFirst || c.apply(change, 0) || c.busy()) return false;
  c.current = receiver(); c.current.known &= ~fieldMask(Field::Power);
  change = {}; change.periodMs = 500;
  if (c.validate(change) != Error::ReadFirst) return false;
  c.current = receiver(); write16(c.current.rate, 100);
  change = {}; change.profile = Profile::Compact;
  return c.validate(change) == Error::Invalid;
}

static_assert(maskedNavigationAndSbas(), "Only requested navigation/SBAS fields change");
static_assert(ackRequiresReadback(), "ACK alone never reports an applied setting");
static_assert(rejectedOrMismatchedCannotSave(true) && rejectedOrMismatchedCannotSave(false), "Failures block saving");
static_assert(compactBeforeFasterRate() && normalAfterSlowerRate(), "Keep the shared 9600-baud stream bounded");
static_assert(powerConstraints(), "Readback/constraints protect power-mode changes");
static_assert(timeoutWrapAndBusy(), "Bounded commands, timer wrap and operation exclusion");
static_assert(preservePortAndSaveMasks(), "Save changes no port, clear or load settings");
static_assert(messageReadbackTargetsUart1(), "Read current UART rates, not another port's rates");
static_assert(fullPayloadAndMalformedFrames(), "Retain NAV5 without permitting oversized transmit packets");
static_assert(readSequence(false) && readSequence(true), "Read each field and never save partially known settings");
static_assert(failedSequenceRetainsConfirmedChanges(), "Stop after a failure without claiming an atomic rollback");
static_assert(unknownRateOrPowerPreventsUnsafeChanges(), "Validate rate and power together before transmitting");
} // namespace GpsConfigTests
