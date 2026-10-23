# Notification audio (v0.1.94)

The speaker connects to the CYD's dedicated speaker port. No speaker-wire
changes are required. Internal playback is ESP32 GPIO8 to ES8311 DSDIN, then
the codec's differential analog output to the board amplifier. GPIO6 is the
codec's recording output; firmware leaves it as an input and tri-states the
unused codec ADC data output. Clocks remain MCLK GPIO4, BCLK GPIO5 and LRCLK
GPIO7, with active-low amplifier enable on GPIO1.

## Sound controls

The Sound page provides volume and separate message/direct tone selections.
Digital codec gain stays at unity (0 dB); the generated PCM peak is bounded to
75% of full scale even at 100% volume. Positive boost and experimental codec
profiles have been retired. Legacy API boost/profile values resolve to this
same fixed playback configuration.

The first v0.1.93 boot migrates old gain settings and limits the saved volume
to 50%, preserving quieter or muted settings. Later volume changes are saved
normally. Tone selections are preserved.

In v0.1.94, notifications offer both short, rounded sine-wave boops and bubble
pops. Boops have fixed pitches, a smooth 12.5 ms onset and a gentle decay to
silence. Pops sweep continuously from 600 to 1200 Hz over 32 ms, with rounded
onsets/endings instead of abrupt square-wave switching. Their onset is limited
to one third of the short note. Neither type uses bell overtones. The whole
notification, including settling/trailing silence and the DMA flush, lasts less
than 750 ms. Test Sine remains a separate diagnostic choice with a steady 1 kHz
body and smooth 20 ms endpoints.

Dropdown labels and playback now come from the same preset catalog:

| Sound | What it plays | Previous label / saved ID |
| --- | --- | --- |
| Soft Boop | One soft 140 ms note | Low Bell / 0 |
| Double Boop | Two short notes at the same pitch | Chime / 1 |
| Two Note | Two notes, low then high | Two Step / 2 |
| Triple Boop | Three quick notes at the same pitch | Beacon / 3 |
| Gentle Alert | Two notes, high then low | Pager / 4 |
| Rising Boop | Three short notes stepping upward | Bell / 5 |
| Long Boop | One rounded 320 ms note | Sustain / 6 |
| Test Sine | A steady 300 ms diagnostic tone | Test Sine (formerly Raw) / 7 |
| Bubble Pop | One quick upward-sweeping 32 ms pop | New / 8 |
| Double Pop | Two quick upward-sweeping pops | New / 9 |

Saved IDs, selected slots and volume are preserved; older API names remain
accepted as aliases. Notification choices change their sound as described above.

Touchscreen test buttons respect saved volume and do not interrupt an active
tone. Rapid repeated test presses are ignored while a sound is outstanding.
The browser button is explicitly labeled Test Message Tone and previews the
saved message selection at saved volume. It previously forced Chime regardless
of the selection; that override is removed. Confirmation names the queued sound
and volume. Neither test changes the user's selections or volume.
Muted, busy or recovering requests return HTTP 409; accepted requests return
HTTP 202. Normal notifications have a bounded queue of two outstanding sounds,
including the current one. Direct notifications take priority in the pending
queue. No audio buffer is allocated for each button press.

## Playback and recovery

A dedicated FreeRTOS task generates stereo 16-bit Philips I2S at 16 kHz and
feeds DMA independently of LVGL, HTTP, SD and radio work. It uses a fixed PCM
block and retains the unsent tail on a partial write. Native I2S byte counts
are used because the Arduino wrapper can omit bytes accepted during a timeout.
Each write has a 40 ms timeout; unfinished blocks fault after a bounded 160 ms.
Silence is queued to drain DMA at the end of a sound or after cancellation.

The ES8311 is a slave with external 4.096 MHz MCLK (256 times the sample rate).
DAC oversampling register 0x04 is 0x20. The unused ADC clocks/modulator are
powered down; DAC output uses line drive into the board power amplifier. Codec
identity and key configuration registers are checked during initialization.

Codec control and amplifier GPIO changes stay on the main loop, preserving
serialization with touch and battery I2C traffic. The PCM task acknowledges a
pause before the main loop can stop/recreate I2S and reinitialize the codec.
Faults disable the amplifier and invalidate readiness. Recovery makes at most
three attempts at least two seconds apart. A later test request or USB
`audio-recover` can request another recovery after those attempts are exhausted.
Recovery does not replay the failed sound or play a startup tone.

## Diagnostics

At USB 115200 baud:

- `audio` or `tone-status`: read-only JSON with codec registers, actual errors,
  data pin, worker readiness, queued/dropped sounds and recovery counters.
- `audio-recover`: cancel outstanding sounds and request reinitialization.
- `audio-test`: play one short 1 kHz sine capped at 25% volume without changing
  saved settings. A muted device remains muted.
- `diagnostics`: the existing health report now includes an `audio` object.

The web `/tone/status` endpoint exposes the same detailed audio state. Main
status retains legacy audio fields and adds data-pin and recovery information.
Failed codec reads report -1 instead of masquerading as register value 0xFF.
The former arbitrary `/tone/codec` write endpoint returns HTTP 410; volume and
tone controls are the supported way to change playback.

The Sound page reports Ready, Playing, Recovering audio, Audio fault or Audio
unavailable. Initialization success does not establish the speaker's acoustic
quality or amplifier behavior under load. User-requested sound testing remains
necessary to confirm distortion and repeated-playback behavior on the hardware.

The earlier [audio investigation](AUDIO_REVIEW_2026-10-23.md) records the v0.1.92
faults and distinguishes software findings from unmeasured hardware causes.

## Installation observation — October 23, 2026

The production build compiled successfully and was flashed over COM12 with
esptool hash verification. The device reported v0.1.93, GPIO8 playback, ready
I2S/codec/task, unmuted DAC, 0 dB gain, expected oversampling/format registers,
and codec state 0x70. The quiet USB diagnostic produced 41,280 bytes over 42
successful writes, with no short writes, I2S failures or codec I2C failures.
The amplifier-enable pin went low during playback and returned high afterward.
The user confirmed the brief diagnostic tone was audible and clear.

This confirms one controlled playback. Repeated-playback stress and injected
fault recovery were not exercised during installation.

Local artifact: `dist/s3-lora-interface-current-v0.1.93-20261023.bin`
(1,844,336 bytes), SHA256
`01c028f3ac3b8b33ac79004b583cf87d07eb60c191c15da6d387de366f3dcdd9`.
The pre-flash boot/settings backup is
`build/pre-v0.1.93-flash-metadata.bin`; the diagnostic capture is
`build/v0.1.93-audio-diagnostic.json`. Generated artifacts remain ignored by Git.

## v0.1.94 installation — October 23, 2026

The boop/pop production build compiled successfully and was flashed over COM12
with esptool hash verification. USB readback confirmed v0.1.94, ready codec/I2S
and playback worker, preserved 40% volume, message slot 4 (Gentle Alert), and
direct slot 5 (Rising Boop). No sound was played automatically during this
installation; the new presets have not yet received a listening confirmation.
Browser regression checks passed, including saved-selection preview and busy/
network handling. Compile-time sound checks passed for stable saved IDs, short
durations, bounded pitches and smooth envelope endpoints/transitions.

Artifact: `dist/s3-lora-interface-current-v0.1.94-20261023.bin`
(1,845,152 bytes), SHA256
`27e2245ec3481c44c382979d1760a9778d66f2ce68528445581e7c80b2c773fa`.
Settings backup: `build/pre-v0.1.94-flash-metadata.bin`.
Readback: `build/v0.1.94-sound-options-status.json`.

References: [LCDWiki schematic](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf),
[ES8311 datasheet](https://www.lcdwiki.com/res/PublicFile/ES8311_DS.pdf), and
[Espressif ES8311 driver](https://raw.githubusercontent.com/espressif/esp-adf/release/v2.x/components/esp_codec_dev/device/es8311/es8311.c).
