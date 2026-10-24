# Firmware implementation history

These notes record how earlier versions were implemented and verified. They
may describe an older UI, wiring arrangement or learning behavior; use the
[current build guide](BUILD.md), [documentation index](README.md) and
[project README](../README.md) for present behavior. Hardware observations are
limited to the conditions recorded at the time.

## Battery screen regression check (v0.1.81)

Both build scripts treat printf format mismatches as errors (`-Werror=format`).
The verification build also accepts USB command `test-ui`. It exercises the
Battery renderer with the gauge-present flag both true and false, then renders
Mesh Health, System Interface and System Serial before restoring the current
screen. Each step prints PASS/FAIL. It neither changes stored settings nor tests
battery charging, capacity or discharge. Runtime/discharge validation requires
the appropriate physical power configuration; CYD USB does not charge the cell.

## Interface refresh (v0.1.82)

The touchscreen home dashboard shows live Heltec and battery status, unread
message count and the most recent message preview. Nodes, Map, System and
Health remain on the home screen; TX Log is under Messages. Home actions are at
least 44 pixels tall, and menu/page headings use the selected normal text font.
The existing Display text-mode preference remains available.

The web dashboard uses a four-card live summary and expandable radio details.
Messages, Nodes, Map and Health are primary navigation; Settings, SD storage,
Serial, Firmware and Logs are under Tools. Delivery states have text and colored
badges, and the header warns when displayed readings may be stale. Navigation,
forms and delivery rows adapt to narrow screens. The page explicitly declares
UTF-8 and supports keyboard focus and reduced-motion preferences.

`test-ui` additionally renders Home and Messages and checks that the home
widgets fit within the screen with adequate action-button heights. Browser
layout checks during this update covered 390px and 1280px viewports, navigation,
delivery rendering and offline recovery with sample status data.

## Home launcher adjustment (v0.1.83)

Home now uses four uniform full-width rows: radio response age, battery/runtime,
mesh/GPS status, and unread messages/latest preview. Messages uses the same
neutral styling as the other rows and opens chat when tapped. The bottom Menu
button opens five consistently styled function buttons (Messages, Nodes, Map,
TX Log, System); Health is under System, and Home returns to the status launcher. `test-ui` checks
this menu navigation, nested Back navigation, rendering and bounds checks.

## PSRAM tile cache and profiling (v0.1.84)

Offline map rendering now has an eight-tile LRU cache holding 1 MiB of raw
RGB565 pixels in PSRAM. It is optional: allocation requires 2 MiB of additional
PSRAM headroom and never falls back to internal RAM. Without the cache, rendering
uses the original cropped row-read path. Display transfer buffers and task
stacks remain in internal RAM.

Cache misses read one complete 131072-byte tile sequentially in 4 KiB chunks;
hits copy the needed rows from PSRAM. Only complete, correctly sized files become
valid entries. Missing neighbours are not cached. Root rescans and SD remounts
change the storage generation and invalidate entries and the displayed map;
results from an older generation are discarded. SD remount returns HTTP 409
while the map worker/benchmark is active. The existing SD mutex still serializes
file access during a render.

USB command `mapbench` starts a background, read-only benchmark at the cached,
local-GPS or saved map location. It compares the uncached row reader, a cold tile
cache, a warm repeat, a 40-pixel horizontal pan, and returning to the original
viewport. Cached results must match the corresponding uncached render byte for
byte; warm repeat and return must perform zero tile-data reads. The benchmark
uses a temporary PSRAM reference buffer and never publishes its test viewports
to the display or saved map image. Normal application logging continues.

`[mapbench]` lines report microseconds, tile-data read calls/bytes, validity and
pixel comparison. The last benchmark is also included in `/status` and the field
report as `mapBenchmark`. Tile hit/miss/load/eviction counts, residency, read errors,
SD data reads/bytes and last render read time are available there and in USB
`[TILECACHE]` diagnostics. Web Health shows cache usage and read counters.
Counters include benchmark activity. Cold loading reads entire tiles, so it can
read more bytes than the uncached cropped viewport; measure warm panning as well
as cold rendering when evaluating the tradeoff.

The verification build checks LRU replacement, failed/uncommitted fills, distinct
zoom/coordinate keys, cache clearing and sustained recency without timer wrap.

## USB tile upload and read-only SD export (v0.1.85)

See [USB_MAP_TRANSFER.md](USB_MAP_TRANSFER.md) for the upload, replacement-card
copy and direct USGS download commands. USB RX buffering is 8192 bytes; command
processing is bounded per main-loop iteration. Tile receives stage one 128 KiB
tile in PSRAM, and file exports use about 56 KiB in PSRAM. SD operations use the
existing mutex. Remount and OTA are blocked during an active USB transfer;
benchmarks wait for USB transfers to finish. Pure compile-time checks cover
standard CRC32, coordinate bounds and sequential bounded chunks. Python tests
cover interleaved/partial replies, skipped tiles, acknowledgements/checksums,
safe destination paths, export retries and preservation of existing files.

## Touchscreen map controls (v0.1.86)

See [MAP_NAVIGATION.md](MAP_NAVIGATION.md). Individual zoom levels 10-14 replace
the Zoom/Wide presets. Selection is saved after a one-second debounce. The map
supports dragging, pause/follow and My location, with a latitude-correct distance
scale and source/freshness status. A navigation revision prevents old worker
results from overwriting a newer view. Marker overlays use the displayed image's
center rather than a pending center. Manual/node views do not replace the saved
device map snapshot. USB `test-map` in the verification build checks real LVGL
pointer events, layout, source/freshness, scale, bounds and stale-result handling
without saving fake GPS coordinates or test zoom preferences.

## Offline coverage and imperial map scales (v0.1.87)

See [MAP_COVERAGE.md](MAP_COVERAGE.md). The background renderer checks real files
for the current viewport at zooms 10–14, prefers complete coarser coverage when
detail is missing, and reports partial or missing coverage explicitly. Requested
and displayed levels remain separate. The touchscreen Coverage page and browser
coverage card expose per-level availability. Touchscreen and browser map scales
use feet and miles. USB `test-coverage` uses temporary SD fixtures to verify
fallback, recovery, malformed files, partial coverage, UI navigation and cleanup.

## Offline browser maps (v0.1.88)

See [WEB_OFFLINE_MAPS.md](WEB_OFFLINE_MAPS.md). Bundled Leaflet assets and an
authenticated SD tile endpoint remove the browser map's internet dependency.
One PSRAM tile snapshot is sent through the existing nonblocking download
service; SD locks are released before network transmission. Browser tests cover
actual local assets, RGB565 conversion, parent fallback cropping, panning,
errors and storage-generation recovery with external requests blocked. USB
`test-webmap` verifies SD fixtures and actual local HTTP requests on the device.

## Home visual refresh (v0.1.89)

See [HOME_DASHBOARD.md](HOME_DASHBOARD.md). The home screen uses four consistent
cards, a larger type hierarchy, separate status/context lines and restrained
state colors. Each card opens its detail page. Montserrat 22 is enabled for
home headings and primary readings. USB `test-home` checks status edge cases,
long text, layout and navigation. The verification-only capture utility records
actual TFT-bound pixels using a temporary PSRAM buffer and CRC-checked chunks.

## GPS acquisition diagnostics (v0.1.90)

See [GPS_DIAGNOSTICS.md](GPS_DIAGNOSTICS.md). USB `gps` and the health report
distinguish receiver bytes, valid NMEA, checksum errors and fix-bearing
sentences from mesh position traffic. System > GPS now shows diagnostics and
available satellite counts before a fix. The UART receive buffer is 4096 bytes.
The connected receiver delivered 70 valid sentences with zero checksum errors
in 28.8 seconds, but had no position fix; the Heltec reported the expected
GPIO5/GPIO4 GPS configuration.

That is the historical observation's configuration. The current documented
wiring shares GPS TX with CYD GPIO3 and Heltec GPIO5. In v0.1.91, GPS RX connects
to CYD GPIO2 for CYD-only control; Heltec GPIO4 is disconnected from GPS RX.
USB `gps rate` polls UBX-CFG-RATE and `gps rate 1|2|5` requests a new rate
with ACK/NAK and readback reporting. Boot sends no configuration commands.
See [GPS wiring and control ownership](GPS_DIAGNOSTICS.md#gps-wiring-and-control-ownership).

## Battery statistics (v0.1.92)

MAX17048 SOC became the percentage authority. Voltage-derived SOC correction
and automatic quick-start were retired. Qualified discharge learning persists
across restarts, and runtime labels identify previous use, gauge trend or
current observations. See [battery statistics](BATTERY_STATS.md).

## CYD audio repair (v0.1.93)

Playback moved to the correct GPIO8 path with unity codec gain, bounded PCM,
a dedicated audio task and fault recovery. A controlled diagnostic tone was
confirmed clear. See [audio operation and installation records](AUDIO.md).

## Boop and pop choices (v0.1.94)

Short, rounded notification boops replaced misleading bell/chime labels.
Bubble Pop and Double Pop added quick 600-to-1200 Hz sweeps. Browser preview
now follows the saved message selection and volume. See [sound choices](AUDIO.md).

## GPS Config and Adv (v0.1.95)

System > GPS gains Config for update rate, navigation mode and SBAS, plus Adv
for receiver power mode and NMEA output. Changes use explicit Apply with
readback verification and a separate receiver Save. The command state machine
limits bandwidth, preserves unrelated settings and reports partial failures.
See [GPS configuration and installation](GPS_CONFIG.md). The production build
was flashed on October 23, 2026; USB identity and a full ten-field receiver read
passed without changing GPS settings. Touchscreen Apply/Save and persistence
after receiver power removal remain untested.

## CYD GPS sharing (v0.1.96)

The CYD can provide external positions over the existing Heltec Protobuf UART,
removing the need to split GPS TX. GPS Config > Adv adds verified Heltec setup
and a persisted sharing switch. Only fresh fixes are forwarded, at most once
per 15 seconds, addressed locally rather than directly broadcast. The Heltec
retains its configured mesh position broadcast policy. See [GPS sharing](GPS_SHARING.md).
Wi-Fi installation and one-request migration were verified on October 23, 2026.
Independent Heltec readback confirmed a fresh external position; its firmware,
unrelated configuration and channels were preserved.

## Blue theme and focused settings (v0.1.97)

Web Settings moves into the main navigation and opens to five searchable
categories with 22 individual editors. Existing fields and API operations are
retained; advanced groups open when a matching search result needs them.
Drafts and pending passwords survive navigation and background refreshes.
Health initially shows eight essential readings, with detailed topics folded
below. Storage similarly separates usage from files and maintenance.

The CYD and browser share navy/blue styling. Structured CYD readouts render
labels and colons in light blue and readings in white, including battery charge
rate, GPS, Wi-Fi, radio/serial diagnostics, node details and settings readouts.
Literal text remains literal under recoloring, and existing amber/red warning
indicators remain. See the [web UI and display guide](WEB_UI.md).

Wi-Fi installation and read-only desktop/mobile browser checks passed on
October 23, 2026. CYD preferences, cached Heltec configuration and channels
matched the pre-update snapshot. The pre-existing GPS-sharing setup requirement
was preserved; local GPS reception and the radio link were working.

## Battery flow inference (v0.1.98)

Positive MAX17048 CRATE no longer asserts charging by itself. Recent SOC and
cell-voltage evidence selects charging/discharging, with an explicit settling
state during transitions, abrupt model changes or missing evidence. A sustained
unplug voltage step can establish discharge within seconds despite a stale
positive rate. Runtime learning pauses while uncertain without erasing the
retained discharge rate. Native/browser diagnostics distinguish inferred flow
from the gauge's SOC-change rate. Gauge SOC remains authoritative for percentage;
no voltage-derived SOC trim, quick-start or charger-status claim is introduced.

The related [Heltec restart investigation](MESSAGE_RESTART_REVIEW_2026-10-24.md)
records separate radio/power observations and configuration trials; a CYD
application update does not change the installed Heltec firmware.

## MAX-only battery readings and cycle recording (v0.1.99)

Replaces v0.1.98's conservative flow classifier with direct MAX17048 SOC,
VCELL and signed CRATE readings. Percentage no longer chases gauge changes
through a display smoothing window. Both interfaces call the direction a MAX
SOC trend; no physical charger-status input exists. Remaining time is explicitly
a software estimate and retains its learned discharge rate across power cycles.

Adds an operator-controlled SD battery/power health recorder with 10-second samples,
raw register/validity fields, one-second voltage extrema, explicit manual phase
marks, restart continuation, readable-log and CSV downloads. It includes sampled
voltage span/standard deviation/largest step, MAX read failures, UART framing/
decode errors, codec/output errors/recoveries, Wi-Fi RSSI and SD errors. It never switches
charging or forces a deep discharge. See [battery statistics](BATTERY_STATS.md).

## Plain-language battery readings (v0.1.100)

Changes screen/web/readable-log state wording to Battery: Increasing, Battery:
Decreasing, Battery: Steady, or Battery: Unavailable. Level change shows the rate
per hour. Underlying MAX-only readings, runtime estimates and logging remain
as in v0.1.99; detailed CSV/API fields keep their register terminology.

Recording delays caused by an SD lock or unavailable card are counted separately
from failed log writes. Retry attempts are limited to once per second and sampled
voltage statistics remain available for the next successful record.

Wi-Fi installation and live desktop/mobile browser checks passed on October 24,
2026. Direct SD checks verified readable/CSV downloads, raw register agreement,
manual markers, start/stop, idempotent Start and restart continuation with all
previous bytes preserved. Logging remains enabled. The installed application is
1,884,640 bytes with SHA-256
`fb94d1fb3dadd3e98ff46808e1971a5a89ed8b1e0c60425a6cf6242c389eb406`.
A full physical charge/discharge cycle and oscilloscope/EMI measurements remain
outside these software checks. See the [investigation](MESSAGE_RESTART_REVIEW_2026-10-24.md).

## Continuous diagnostics and clearer controls (v0.1.101)

Power-health recording is always enabled, replacing manual start/stop/charger
note buttons and the 16 MiB stop. A persisted retention setting defaults to
48 hours of powered-on recording, retaining the current readable/CSV pair and
one previous pair. Unlimited mode keeps appending. Directory rotation uses a
persistent recovery journal; rotation age survives ordinary restarts and short
power cycles through flushed records and NVS fallback checkpoints. Existing
v0.1.100 logs migrate intact. The CYD has a bounded scrollable recent-log viewer.

Battery +, - and = indicators use bold glyphs, including a generated three-symbol
font with normal-text fallback on the CYD. Message histories show local entries
in blue and other devices in white. New persisted direction tags are derived
from actual node direction, independently of display names. User content is
escaped before web/LVGL styling. Clear action labels and short explanations are
recorded as the preferred approach for future interface work.
