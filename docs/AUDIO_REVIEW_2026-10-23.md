# Audio investigation — October 23, 2026

The findings below describe the original v0.1.92 implementation. Corrections
are implemented in v0.1.93; [notification audio](AUDIO.md) documents the new
playback task, fixed routing/gain, request handling, diagnostics and recovery.
After flashing v0.1.93, a controlled USB tone completed with no driver/control
errors and the user confirmed clear sound. The original findings remain below
as the pre-repair investigation.

The current audio path contains independently actionable firmware defects. The
distortion has strong software explanations: the playback data pin disagrees
with the documented board, and positive codec gain is combined with nearly
full-scale PCM. The reported loss of sound also exposes a missing recovery path.
The exact event that puts the hardware into silence has not been captured.

User observations: default Max boost is in use; lower settings also distort;
restarting the device has restored sound after repeated test-button use. The
follow-up report is complete silence with an otherwise responsive screen and
touchscreen. The speaker is connected to the CYD's dedicated speaker port, which
is the correct external connection. The GPIO finding concerns internal digital
audio routing and does not call for moving the speaker wires.

This investigation reviewed the workspace source, installed ESP32 core 3.3.7,
manufacturer schematic/datasheets, and Espressif's ES8311 driver. It did not run
speaker stress tests, change audio settings, or flash another firmware image.

Follow-up USB inspection found the ESP32 on COM12 running v0.1.92, with responsive
radio and available SD storage. The first report showed uptime 23 seconds and
reset reason 1 (power-on), so the earlier failed state was not preserved through
reconnection. The readout opened serial with DTR/RTS false and sent only
`version` and `diagnostics`; it did not request a reset or play a tone. This
firmware's USB health report omits audio counters and codec registers, so that
readout does not establish whether the DAC, amplifier or I2S was functioning.
The filtered capture is under ignored `build/audio-silent-device-report-20261023.json`.

## 1. P1 — Playback is assigned to the codec's output signal

`constants.h:59` defines `CYD_AUDIO_DOUT_PIN` as GPIO6.
`Audio.inc:650` passes it to the ESP32 I2S transmitter. No local configuration
override is present.

For the documented LCDWiki ES3C28P board, the intended signal chain is:

```text
ESP32 GPIO8 -> ES8311 DSDIN -> DAC OUTP/OUTN -> board amplifier -> speaker
ESP32 GPIO6 <- ES8311 ASDOUT (recording path)
```

GPIO6 is the wrong direction for playback. The codec's output is muted by
register 0x0A = 0x4C, but muting its data does not tri-state the output; the
firmware leaves the separate tri-state control clear. Driving GPIO6 as an ESP32
output can therefore create electrical contention. Successful I2S initialization
or DMA writes do not prove samples reached the DAC.

This is a reason to correct the signal path before interpreting perceived
differences between tone styles or codec profiles. Noise or distorted output
with this configuration does not demonstrate a working PCM playback connection.

Evidence: [LCDWiki schematic](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf)
and [manufacturer I/O table](https://www.lcdwiki.com/2.8inch_ESP32-S3_Display).

## 2. P1 — The gain structure is designed to saturate

`Audio.inc:47` maps Max to DAC register 0x32 = 0xFF. That is +32 dB, roughly
39.8 times amplitude. `tonePcmDrivePercent()` returns 100 for every boost level,
so no PCM headroom compensates for that gain. The waveform generator maps full
volume to amplitude 32700 and its steady shaped waveform peaks at approximately
0.93 of signed 16-bit full scale before codec gain.

The startup migrations at `s3-lora-interface.ino:107` and `:118` persist 100%
volume and Max boost. Lowering the volume slider scales PCM but leaves the
positive codec gain in place for every nonzero volume.

A numerical calculation of the generated steady waveform, followed by the
configured codec gain, predicts the following fractions beyond digital full
scale. These are calculations for the intended PCM path, not measurements of
the speaker or the currently misrouted data signal. Attack and release portions
are excluded.

| Volume | Codec gain | Steady samples beyond full scale |
| --- | --- | --- |
| 100% | 0 dB | 0.0% |
| 75% | +6 dB | 53.6% |
| 100% | +12 dB | 84.9% |
| 100% | Max / +32 dB | 98.5% |
| 10% | Max / +32 dB | 84.9% |

At Max, the calculated non-saturating volume ceiling is approximately 2.7%.
That explains why lowering the slider can leave distortion. Distortion at
0 dB would need the signal-routing, clock and analog path issues addressed too.

The Raw pattern is especially unsuitable as an everyday test: it produces a
full-amplitude square wave for three seconds and bypasses the calculated
attack/release envelope. This raises sustained signal energy and adds abrupt
edges. The other patterns already contain intentional harmonics; those alone
are not evidence of a broken speaker.

Register interpretation: [ES8311 datasheet, register 0x32](https://www.lcdwiki.com/res/PublicFile/ES8311_DS.pdf).

## 3. P1 — Audio faults are counted but never recovered

`initToneAudio()` runs only during setup. `toneI2sReady` and `toneCodecReady`
record initialization results and are not invalidated after runtime failures.
There is no I2S teardown/restart or codec reinitialization after a failed write.

At `Audio.inc:628–639`, the player records requested/written bytes and the I2S
error, but advances by all requested frames even when none or only some were
accepted. Oscillator phase has also advanced by the whole generated block.
It then finishes the pattern and accepts future button presses against the
same potentially failed backend. Runtime codec register writes also ignore
their return values, while failed register reads can appear as the valid byte
0xFF in status reports.

This is a concrete persistence-of-failure defect. Rebooting reinitializes both
drivers and the codec, matching the user's recovery observation. The observation
does not identify whether the initial failure is I2S, codec state, a control-bus
failure or amplifier/power behavior.

There is no per-button dynamic allocation or growing audio queue in this code:
PCM blocks use a fixed stack array and the pending notification is one enum.
A simple leak of one audio buffer per press is not supported by this review.

## 4. P2 — Playback timing depends on the busy main loop

`serviceToneAudio()` supplies up to 256 stereo frames, or 16 ms of audio, per
call at 16 kHz. It shares the main loop with HTTP, GPS/radio processing, SD
operations, battery reads and LVGL rendering.

The installed Arduino I2S configuration has six DMA descriptors of 240 frames,
about 90 ms of buffering. It enables automatic buffer clearing, so underruns
produce silence rather than a permanently repeated stale sample block. A main
loop delay long enough to exhaust queued audio can create gaps and crackle.
The default stream write timeout is one second, which can also stall the UI
when the backend stops accepting data.

No loop-gap trace was captured during distorted playback, so underruns remain
a plausible contributor rather than a measured explanation for every artifact.
The implementation should decouple PCM supply from display/network latency and
record underrun/timeout information. Codec control on the shared I2C bus must
remain serialized with touch and battery operations if a playback task is added.

## 5. P2 — Clock configuration differs from the reference driver

The Arduino I2S configuration uses a 256-times sample-rate MCLK: 4.096 MHz at
16 kHz. The firmware programs DAC oversampling register 0x04 to 0x10.
Espressif's coefficient table for that MCLK/sample-rate combination selects
DAC OSR 0x20, and its configuration routine writes that value into 0x04.

The complete clock configuration should be aligned with a known-good driver,
including the actual serial slot width. Slave-mode clock-output dividers alone
should not be mistaken for measured external clock frequencies. This review
found a configuration discrepancy, not an oscilloscope measurement proving the
resulting clock waveform or its individual contribution to the symptom.

Evidence: [Espressif ES8311 driver and clock table](https://raw.githubusercontent.com/espressif/esp-adf/release/v2.x/components/esp_codec_dev/device/es8311/es8311.c).

## 6. P2 — Repeated test presses restart playback without a controlled transition

Both touchscreen test buttons call `queueToneStyle(..., true)`; the web handler
does the same. Replacement resets the tone phase and starts the first segment
without draining or explicitly replacing samples already queued in DMA.

Most patterns begin with 120–160 ms of silence. Rapid repeated presses keep
restarting that silent lead-in and can prevent the audible portion from being
reached while pressing. This should resolve after presses stop if the backend
is healthy; it does not by itself explain persistent silence that needs reboot.
Replacing a Raw tone can also create an abrupt discontinuity.

Use a bounded request queue with a clear busy/retrigger policy and an enveloped
transition. Test-button activity should not repeatedly reset an active stream.

## 7. P2 — Codec profiles do not apply all their advertised differences

`toneProfileReg13()` describes different line/headphone output modes but is
never called; initialization always writes 0x13 = 0x10. `toneProfileReg15()` is
also unused and the profile writer hardcodes 0x15 = 0x40. Selecting Ref Line
does not actually apply its intended output-driver change. Several profile
differences affect recording rather than the DAC playback path.

The speaker is fed through a board power amplifier, so a single documented
playback configuration is preferable to partially applied experimental profiles.

## Repair order

1. Correct playback to GPIO8, keep GPIO6 as an input/high-impedance recording
   signal, and use a verified DAC-only configuration with matching clocks.
2. Remove positive codec boost for the generated near-full-scale PCM. Use one
   predictable volume scale, conservative startup loudness and bounded combined
   gain. Migrate existing Max settings rather than preserving the bad gain path.
3. Feed DMA reliably, preserve unsent samples/phase on short writes, and define
   controlled stop/retrigger/drain behavior. Keep test requests bounded.
4. On repeated write/control failures, mute the amplifier, mark audio unhealthy
   and perform a bounded driver/codec recovery. Expose actual errors and codec
   state over USB as well as the web UI, instead of reporting boot-time readiness.
5. Start diagnosis with a short, low-level enveloped sine. Only after the digital
   path is correct should speaker impedance/rating, 5 V supply sag and amplifier
   temperature be used to assess any remaining distortion or shutdown.

The amplifier enable polarity is consistent with the board's shutdown circuit;
this review does not recommend blindly reversing it. Amplifier heating or supply
collapse is possible under sustained overdrive, but neither was measured here.
The board schematic labels SC8002B while LCDWiki's amplifier document is for
FM8002E, so protection behavior must not be assumed identical from that document.

The next implementation should address these faults together. A volume-only
change would leave the incorrect data connection, configuration discrepancy and
missing recovery behavior in place.
