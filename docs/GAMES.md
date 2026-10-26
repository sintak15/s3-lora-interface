# Games and conversations (v0.1.112)

Open **Games** on Home or Menu. **Pets** contains **Tamapetchi** and
**Giraffegotchi**; **Game Boy** opens the Peanut GB ROM browser.

## Pets

The upper portion keeps six live shortcuts: Radio, Battery, Nodes, GPS,
Messages and Map. The lower portion contains the pet, care stats and Feed,
Play and Care controls. Care opens the remaining actions, with Back returning
to the pet. Each pet has independent saved state. Care actions save immediately;
ordinary state saves once per minute. Powered-off time does not advance decay.

These are compact native touchscreen adaptations. Tamapetchi retains the
upstream hunger/happiness/health/energy/cleanliness care loop, sleeping and
age tracking, with a local blob avatar. Giraffegotchi uses its upstream care
model and giraffe artwork. The standalone projects' networking, OTA, community
features and additional minigames are not included.

## Game Boy

The 160×144 image is stretched to 240×160, filling the screen width above the
direction pad, A/B, Select and Start. On the emulator player only, Back and Home
move above the video, below the status bar. The lower area is reserved for game
controls. Other screens retain their bottom navigation.

Two touch contacts can combine a direction and a button. Back pauses and saves;
Resume returns to the same session. Home also pauses and saves. Battery RAM is
saved every 30 seconds when changed and restored when that ROM is opened again.
A failed save keeps the player open with an error so it can be retried.

**Web UI > Games** uploads, lists and deletes SD ROMs. Use original Game Boy
`.gb` files, 32 KiB–4 MiB, with a matching cartridge size and header checksum.
Color-only cartridges and unsupported cartridge controllers are rejected.
Files go under `/s3-lora/games/gb/`; saves use
`/s3-lora/games/saves/<filename>.sav`. Deleting a ROM keeps its save. Uploading
over an existing ROM is rejected until that ROM is explicitly deleted.

ROM names accept letters, digits, spaces, dots, dashes and underscores, up to
64 characters. Uploads are staged and validated before becoming visible.
The touchscreen browser shows up to 32 ROMs. The built-in **Demo** is original
controls-test homebrew: directional buttons change scrolling and A/B change
the checkerboard palette. No commercial ROMs are included.

This first frontend is silent and emulates the original Game Boy. It preserves
cartridge RAM rather than arbitrary emulator save states. Physical touch,
per-ROM compatibility and frame rate still require device testing.

v0.1.112 keeps the emulator's CPU state, working RAM and video RAM in internal
memory and compiles the interpreter for speed. Game timing follows the original
59.73 Hz clock independently of screen presentation, which targets up to 30 Hz.
Only the changed rectangle is invalidated; unchanged frames need no video
transfer. Catch-up batches are bounded to keep radio, GPS and touch serviced.
The enlarged image and controls retain the same layout.

Authenticated `/games/status` includes `fpsTenths`, `presented`, `frameUs` and
`peakFrameUs`. `frames` counts emulated frames; `presented` counts generated
video frames, including unchanged images. Physical screen refreshes can be fewer.

v0.1.111 corrects the original controller's swapped direction/action bit groups
to match Peanut-GB. Raw touch packets use the actual player content coordinates,
including multi-touch and individual lift events. Compile-time checks pin all
eight key masks to the upstream definitions. Device verification tests route
touch packets into the emulator and check both joypad register banks, releases,
video scaling and the player-only navigation position.

## Verification — October 25, 2026

Production v0.1.107 was installed on the CYD over Wi-Fi. Before installation,
all 88 screen/font layout checks and three additional pet/demo captures passed.
On-device emulator checks passed for video, emulated input, trapping an invalid
opcode and starting a fresh session afterward. Independent pet care passed.
Authenticated ROM upload/list/delete, invalid names/headers, overwrite
protection and unauthenticated access checks also passed. The test ROM was
removed. Browser regressions cover late conversation responses and per-peer
drafts. Tests did not transmit simulated mesh messages.

The production image excludes verification endpoints. Its radio link was
responsive with zero decode/frame errors; SD errors, lock timeouts and dropped
appends were zero. Eighteen existing preference/status fields matched before
and after installation.

### v0.1.111 controller and layout checks

Production v0.1.111 passed all eleven device game checks,
including each of the eight buttons through touch decoding into the emulator's
joypad registers, two contacts, individual lift/release, scaled video and Back/Home
navigation. Home/UI regressions passed. Six captures across Normal and Readable
fonts found no layout issues on Home, the ROM browser or the emulator player.

The existing 32 KiB cartridge save was backed up before updating and verified
unchanged afterward. The ROM is present, GPT metadata is unchanged, existing
settings matched, and final SD/radio error counters were zero.

### v0.1.112 performance checks

The previous firmware measured approximately 11–13 emulated frames per second
with the user's installed ROM running. Two neutral-input on-device tests of
v0.1.112 each advanced 478 frames in eight seconds: 59.74 and 59.75 emulated FPS.
Generated video frames averaged 22.62 and 22.75 FPS in that scene. Other scenes
and cartridges can have different presentation rates; this is not a claim of
60 physical screen updates per second.

All eleven game checks and Home/UI regressions passed again. The benchmark
loads cartridge RAM, suppresses autosaving during its short run and discards
its temporary RAM changes. The existing 32 KiB save was backed up before OTA
and verified byte for byte afterward. The performance endpoint exists only in
verification builds.

Production v0.1.112 was installed over Wi-Fi. All 18 compared settings/status
fields matched, the ROM and cartridge save remained intact, and the GPT metadata
fingerprint stayed unchanged. Final SD write/lock/drop and radio decode/frame
error counters were zero. Verification endpoints were absent from production.

## Direct conversations

**Messages > Direct** opens a per-device inbox. Select a conversation or start
a new one with a node ID. Incoming messages do not change the device currently
being addressed. Each conversation has its own unread count. Node details >
Msg opens that device's conversation. The browser provides the same selection.

New messages are indexed by numeric peer ID in `/s3-lora/direct_threads.log`,
with a bounded recent tail in RAM. The full journal remains on SD. Existing
combined history is preserved as **Earlier messages** because old outgoing
records did not store a recipient; those records cannot safely be assigned to
individual devices. The original combined log remains available for export.

## Sources

- [Peanut-GB v1.3.0](https://github.com/deltabeard/Peanut-GB/tree/v1.3.0), MIT,
  with copyright/license retained in [the vendored header](../src/vendor/peanut-gb/peanut_gb.h).
- [Giraffegotchi](https://github.com/elliotboney/giraffegotchi), MIT;
  [license](../src/vendor/giraffegotchi/LICENSE.txt).
- [ESP32-TamaPetchi](https://github.com/CyberXcyborg/ESP32-TamaPetchi), MIT;
  [license](../src/vendor/tamapetchi/LICENSE.txt).
