# Web UI and display

Firmware uses a navy/blue palette on both the CYD and its local web UI.
Open the CYD's IP address in a browser and use its configured web login.

## Finding settings

**Settings** is a main navigation destination. Its landing page offers five
categories rather than displaying every configuration field:

| Category | Editors |
| --- | --- |
| Radio & location | Radio/identity, channels, GPS/position |
| Device & screen | CYD interface, Heltec behavior, Heltec display, Heltec power |
| Connections | Heltec Wi-Fi, Bluetooth, serial link, MQTT |
| Messages & alerts | External alerts, store/forward, canned messages, status message |
| Sensors & tools | Telemetry, detection sensor, ambient LED, Heltec voice audio, Paxcounter, neighbor information, range test |

Choose a category, then an editor. Only that editor is shown. **All settings**
returns to the category list; the category button returns to its section list.
Search finds sections by their titles, descriptions and field labels. Selecting
a result also opens an advanced group when the matching field is inside it.
Changing editors keeps drafts in the browser, including pending passwords.
Background status updates do not erase those drafts.

The CYD interface editor controls the CYD's identity, access point and web login.
Editors explicitly named Heltec control the connected Meshtastic radio. Heltec
voice audio is separate from the CYD's notification sounds.

Common fields appear first. Advanced groups hold GPS timing, serial pins, sleep
timings, display details, MQTT extras, notification outputs and telemetry
reporting intervals. Their values still load from the radio and remain included
when their section is applied. Blank password fields retain existing secrets.
Apply/save feedback appears above the active editor. A lost connection reports
an unknown outcome; refresh settings before repeating the change.

Channel Save remains in its editor. General save/restart and diagnostic commands
are under **Tools > Serial console > Heltec commands**.

## Health and maintenance

**Tools** contains Health & diagnostics, SD storage, event logs, serial console
and firmware update. The menu closes when a destination is selected, when
Escape is pressed or when another part of the page is clicked.

Health initially shows eight essential readings. Battery/runtime, GPS/position,
radio traffic/serial data, maps/cache, system resources and audio/hardware tests
have separate expandable topics. Field reports remain directly downloadable.
SD storage initially shows status, type, usage and capacity; saved files and
maintenance counters have their own expandable groups.

The Battery & runtime topic shows the battery level trend reported by the MAX sensor and sampled voltage
variation, plus power-health logging controls. Start/stop logging and optionally
mark physical charger changes; download the readable log or detailed CSV. There
is no dedicated before/after mode. Both files continue across logging sessions
and enabled recording resumes after restart. See [battery statistics](BATTERY_STATS.md)
for measurement limits and file size limits. CYD controls are under System > Battery.

## CYD label/value colors

Structured readouts use light blue (`#85BFFF`) for the label, including its
colon, and white (`#F4F8FF`) for the reading on the same line. For example,
**Charge rate:** and **3.14%/hr** have different colors. This covers battery,
GPS, Wi-Fi, interface/memory/SD statistics, serial/radio statistics, mesh health,
node details, message diagnostics, channel readouts, map coverage, brightness,
sound volume and receiver-current-setting readouts. Existing warning/fault
indicators retain amber/red. The palette also covers home cards, navigation,
inputs, keyboard, node lists and map markers; map imagery keeps its source colors.

The shared `DiagnosticText` formatter distinguishes field-label colons from
colons inside MAC addresses and times. It also handles multiple label/value
pairs on a line. Literal `#` characters in values are escaped before LVGL
recoloring, so SSIDs and node names remain literal data. Formatting uses a
bounded shared UI buffer; an oversized future readout falls back to complete
plain text rather than truncating its measurements. Editable inputs and freeform
message/log bodies retain ordinary text handling.

## Verification

The original live v0.1.96 Settings page exposed 158 controls, including a single
46-control additional-modules form. It measured 15,366 pixels tall at 390 px
width. The new landing page shows five categories, retains all existing controls
across 22 editors, and measures under 1,500 pixels in the browser layout checks.

`tests/test_web_navigation.py` exercises desktop/mobile navigation, advanced
search results, all editors, draft/secret preservation, module save requests,
the channel shortcut, grouped diagnostics, label/value colors, UTF-8 symbols and
the initial-password setup restriction. Existing browser tests cover status
polling, escaping, retries, sound previews and offline maps. Compile-time
`DiagnosticTextTests.h` checks label boundaries, literal data and buffer limits.
See [build and verification](BUILD.md) for the commands.

### Installed-device verification

On October 23, 2026, v0.1.97 was installed over authenticated Wi-Fi on
`192.168.1.192`. The CYD booted from `app1`; its radio link remained responsive,
SD storage was available, and its local GPS had a fresh fix with ten satellites.
Reported CYD identity, network and sound preferences, the cached Heltec
configuration and channel summaries matched the snapshot taken before upload.

Read-only browser checks against the installed device passed at 390 px and
1,280 px widths: category navigation, one visible editor, advanced-field search,
eight summary diagnostics and the blue-label/white-value colors. The mobile
Settings landing page measured 1,150 px, versus the previous 15,366 px. Neither
viewport produced JavaScript errors or configuration writes. The firmware build,
compile-time readout tests, web regressions and offline-map browser checks also
passed. The physical CYD display has not been visually inspected for this release.

GPS sharing reported **Use CYD GPS setup required** both before and after this
update: the Heltec's existing position configuration had GPS mode `ENABLED`.
The UI update preserved that configuration; it did not rerun the GPS-sharing
setup or verify new relay acknowledgements. Local CYD GPS reception was working.

The installed application is 1,871,040 bytes; its SHA-256 is
`81923f511a5de53aa77e536fffa12e296425a16d7b195332f6c9b901df4ab821`.
