# Home dashboard

The touchscreen Home page uses navy backgrounds, rounded blue panels,
light blue accents and a consistent Montserrat type hierarchy. The layout was
introduced in v0.1.89; v0.1.97 aligns it with the [shared blue theme](WEB_UI.md).
v0.1.104 adds bundled Material Symbols Rounded to card headers and navigation,
with a separate unread-count badge. Icons retain the cards' visible labels.
The heading introduces a status overview, with a full-width radio card, two
larger battery/node readings, and a full-width message preview. Cards share
the same surface, border, corner radius, chevron and press feedback. Messages
does not receive a separate accent treatment.

Radio state is a short label (Waiting, Connected, Quiet or No response) with a
separate last-heard age. Battery percentage has a small charge-level bar and a
separate charging, runtime or low-battery hint. Missing fuel-gauge readings
show `--` and hide the bar. The node card counts known nodes and reports GPS
readiness only for a fresh local fix. Message previews remain bounded with an
ellipsis, with unread count shown separately when present. Home shows a compact
runtime once available, or a short estimating/missing-reading hint. The Battery
detail screen retains the estimate basis and full gauge readings.

All four cards open their corresponding detail page and support Back to Home.
The Menu button remains the main launcher; System still contains Health. The
home layout remains within the existing 240×320 portrait display, with card
touch targets at least 44 pixels tall. The five main-menu tiles remain visible
together. System menus place icons above their labels and scroll when needed;
the existing font preferences remain available.

The verification firmware's USB `test-home` exercises missing sensors, charging,
critical battery, stale radio/GPS, long previews, maximum node count, card
bounds, consistent styling and detail/Back navigation. Test values and the
navigation stack are restored afterward; no test locations or messages are
written to SD. The existing `test-ui` checks cover the launcher and both Battery
branches.

For visual inspection, `python tools/capture_home_usb.py --port COM12 --output
build/home.png` captures pixels sent by LVGL to the TFT. This verification-only
path allocates a temporary 150 KiB PSRAM framebuffer, freezes it after capture,
and returns small requested chunks with CRC checks. Closing the capture or a
30-second idle timeout releases the buffer. Normal firmware does not allocate
a screenshot framebuffer.

## Device verification (2026-10-17)

Installed the final v0.1.89 image in the connected CYD's existing app1 partition;
the flasher verified the application hash. All five Home checks and ten existing
UI checks passed, including both Battery branches. Diagnostics recorded zero
SD errors, lock timeouts, dropped appends or map-worker failures. About 154 KiB
internal RAM and 6762 KiB PSRAM remained free.

The actual TFT-bound screen capture is `build/home-v0.1.89.png`. Visual review
confirmed spacing, hierarchy and card consistency, and prompted shorter idle
labels so normal battery/message states do not truncate. Capture chunks were
CRC-verified using buffered USB reads and retries, and the temporary framebuffer
was released. Settings and SD histories were preserved; the separate H: tile
download continued.

Artifact: `dist/s3-lora-interface-v0.1.89-verified-20261017.bin` (1854736 bytes).
SHA256: `47df90ea0aea68beb3414c9fe6b8a4c4b22966b6c46eb2c628be899e487072ba`.
Backup: `build/pre-v0.1.89-flash-metadata.bin`.
Logs: `build/v0.1.89-home-smoke.log` and `build/v0.1.89-device-report.json`.

## Text bounds after the font increase (v0.1.103)

Home labels have explicit one-line heights as well as widths. The Battery hint
uses compact remaining time such as `~4h55m`; the estimate basis remains on the
Battery detail screen. The hint and level bar occupy separate rows. Missing
readings, charging-trend symbols, low-battery warnings and long message previews
keep their own bounds. Chevrons and the unread count have separate label boxes.

Shared button/status labels also have explicit line heights. Compact node rows
use separate flexible columns, while detailed rows grow to fit their wrapped
text. Chat headers and send controls reserve room for the larger type.

Verification builds expose authenticated `POST /diagnostics/ui-layout` and
`GET /diagnostics/ui-frame`. These inspect actual LVGL coordinates and capture
TFT-bound pixels. They are absent from production builds. Temporary font and
sensor fixtures are restored without preference writes or simulated SD records.
Use `tools/check_display_layout.py` as described in [BUILD.md](BUILD.md).

On October 24, 2026, all 74 on-device layout checks passed across both font
modes: 30 screens, six additional Home states, and detailed node rows per mode.
Checks found no label clipping, control overlaps or truncated essential Home
readings. CRC-verified TFT captures confirmed the separate Battery hint/bar
rows, including a runtime fixture. The native Home, UI and scrolling regression
tests also passed on the device. Production v0.1.103 was then installed over Wi-Fi.
