# Touchscreen map navigation

Firmware v0.1.86 replaces the two Zoom/Wide presets with individual zoom levels
10 through 14. The minus/plus buttons are 64 by 44 pixels, the current level is
shown between them, and buttons disable at the limits. Zoom selection is saved
to preferences after one second without another change and restored at boot.

Drag the map to explore. The image and its markers move together during the
gesture; on release the new center is calculated in Web Mercator coordinates
and requested from the background renderer. Dragging pauses GPS following.
Follow toggles between keeping the current view and tracking the device again.
My location recenters and enables following. Panning stays paused when visiting
another ordinary page and returning. Opening a node's map still centers that
node; My location returns to the device position.

The fixed layout keeps both zoom and location controls visible while dragging.
A distance scale in feet and miles uses the displayed tile zoom and latitude. The status line
identifies fresh local GPS (at most 30 seconds old), a device/node position, or
a saved location, and shows following/paused state. Missing tiles and unavailable
SD storage have an overlay rather than showing an old view as if it were current.
Since v0.1.87, missing detailed tiles fall back to an available coarser map while
preserving the requested zoom. Tap Coverage between the zoom buttons for actual
local availability at each level; see [MAP_COVERAGE.md](MAP_COVERAGE.md).
Since v0.1.88, [offline browser maps](WEB_OFFLINE_MAPS.md) use the same SD tiles.

Each explicit view change increments a revision. Render results from an older
revision, a remounted tile source, or an active drag are discarded. Marker
coordinates and the scale use the actual displayed image center/zoom, including
small GPS moves retained by the map pixel cache. Browsing and node views never
replace the persistent device-location map snapshot.

The firmware verification build includes GPS freshness/clock-wrap and zoom-bound
compile-time checks. Its USB `test-map` command exercises the LVGL controls and
layout, all five zooms, pan direction and coordinate limits, scale calculations,
GPS source/freshness, pause/recenter, and stale render rejection. Test coordinates
and zoom choices are temporary and do not update location history or preferences.
The existing `test-ui` and read-only `mapbench` checks remain available.

## Device verification (2026-10-17)

The connected CYD was flashed with the verified v0.1.86 image in its existing
app1 partition. All 20 map checks passed, including actual LVGL pointer events,
drag preview placement, pause/recenter, and rejection of an older queued render.
All ten existing UI checks passed with the Map screen included. The first
gesture test needed an explicit LVGL layout pass before checking preview
coordinates; the final check exercises both movement and release behavior.

The read-only cache benchmark passed all seven phases with identical pixels.
Warm repeat/pan/return took 6370/6744/6384 microseconds and made zero tile-data
SD reads. Device diagnostics recorded zero SD errors, lock timeouts or dropped
log appends, with about 165 KiB internal RAM and 6770 KiB PSRAM free.

Verified artifact: `dist/s3-lora-interface-v0.1.86-verified-20261017.bin`
(1742096 bytes). SHA256:
`33fa466f07dcd9d3ac96e1401f5ccace1a88766c12e5591193b6b2075247221c`.
The pre-flash settings/OTA/partition metadata backup and hardware logs are under
`build/`. The separate map download on H: continued throughout development.
