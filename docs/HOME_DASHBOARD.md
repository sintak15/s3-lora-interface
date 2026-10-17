# Home dashboard (v0.1.89)

The touchscreen Home page uses a charcoal-to-green background, quiet rounded
cards, a restrained mint accent and a consistent Montserrat type hierarchy.
The heading introduces a status overview, with a full-width radio card, two
larger battery/node readings, and a full-width message preview. Cards share
the same surface, border, corner radius, chevron and press feedback. Messages
does not receive a separate accent treatment.

Radio state is a short label (Waiting, Connected, Quiet or No response) with a
separate last-heard age. Battery percentage has a small charge-level bar and a
separate charging, runtime or low-battery hint. Missing fuel-gauge readings
show `--` and hide the bar. The node card counts known nodes and reports GPS
readiness only for a fresh local fix. Message previews remain bounded with an
ellipsis, with unread count shown separately when present. On battery replaces
runtime-learning detail until an estimate is available, keeping Home focused
on the current power state.

All four cards open their corresponding detail page and support Back to Home.
The Menu button remains the main launcher; System still contains Health. The
home layout remains within the existing 240×320 portrait display, with card
touch targets at least 44 pixels tall. Other pages keep their current layouts
and font settings.

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
