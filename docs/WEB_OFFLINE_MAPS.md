# Offline browser maps (v0.1.88)

Open Map while connected to the device Wi-Fi. The map viewer, its stylesheet
and map tiles are served by the device. Internet access is not required. The
viewer uses the same installed RGB565 SD tiles as the touchscreen, at zooms
10–14, with feet/miles scales and node markers.

Drag to explore. Status polling updates markers without recentering your view.
My location returns to the device position or saved location; Show nodes fits
the positioned nodes. Refresh maps clears the browser tile cache and retries
missing or unavailable areas. Without a GPS, saved location or positioned node,
the viewer waits for a location rather than selecting an arbitrary region.

Missing detailed tiles fall back individually to the closest readable parent
tile. The browser crops the correct portion of that parent and labels the tile
with its coarser level. Areas with no usable tiles say No map coverage. Busy SD
storage, unreadable files and interrupted requests show unavailable states.
The Touchscreen coverage card remains specific to the touchscreen's last view;
the browser map status describes the browser's visible tiles.

`GET /map/tile?z=14&x=3955&y=5898` uses the existing web authentication and
validates zoom and world coordinates before constructing a path. It accepts
only numeric tile coordinates, not arbitrary filenames. A successful response
contains one 131072-byte RGB565 little-endian tile, with `X-Tile-Zoom` indicating
the actual source level and `X-Map-Generation` its storage generation. Missing
coverage returns 404, malformed/unreadable files without a usable parent return
422, and unavailable/busy storage returns 503. Tile responses use no-store.

Each request reads at most five candidate paths, snapshots the selected tile
to 128 KiB of PSRAM, then releases the SD mutex. The existing download service
sends up to 4 KiB per main-loop pass with nonblocking socket writes. The tile
snapshot is freed on completion, disconnect or timeout. Network backpressure
does not hold the SD mutex or share the touchscreen worker's pixel buffers.
The browser loads one tile at a time, cancels requests for unloaded tiles,
retries transient 503 responses twice, and caches at most 24 successful raw
tiles (3 MiB). Storage-generation changes clear the browser cache. No browser
tile request writes SD files, preferences or location history.

Leaflet 1.9.4 JavaScript and CSS are bundled as gzip arrays in firmware.
The source assets, license and SHA256 manifest are under
`src/vendor/leaflet/`; `tools/vendor_leaflet.py` regenerates the bundle.
The upstream BSD-2-Clause license is retained in that directory. Distribution
must include that license. The map viewer uses canvas node markers and does
not require Leaflet's optional marker/layer-control images.

Browser verification: `python tests/test_web_offline_map.py` requires Playwright
and Chromium. It serves the actual bundled assets with all external network
requests blocked, at 390px and 1280px widths, and checks RGB565 colors, parent
cropping, pan preservation, missing/malformed/truncated/busy responses,
storage-generation recovery and imperial-only scales. The verification
firmware's USB `test-webmap` checks temporary SD fixtures and makes real
authenticated HTTP requests through the device's lwIP loopback interface,
including asset CRCs, tile CRC, authentication and coordinate rejection.

## Device verification (2026-10-17)

Installed the verified v0.1.88 image in the connected CYD's existing app1
partition, with settings and SD content preserved. The flasher verified its
hash. All 15 web-map checks passed, including actual local HTTP delivery of
both gzip assets and a full SD tile with matching CRCs. Status requests remained
responsive while a tile response was backpressured; disconnecting that client
released its buffer and the next tile request succeeded. Authentication and
invalid zoom, world-coordinate and path-traversal requests were checked.

The existing 13 coverage, 21 map/navigation and ten UI checks also passed.
Temporary fixture directories were removed. The seven-phase cache benchmark
produced identical pixels, with zero tile-data SD reads for warm repeat, pan
and return (8213/6369/8620 microseconds). Diagnostics recorded zero SD errors,
lock timeouts, dropped log appends, tile read errors or map-worker failures.
About 154 KiB internal RAM and 6763 KiB PSRAM remained free after the checks.

Browser checks passed at 390px and 1280px with all external network access
blocked. A separate preview rendered nine downloaded USGS St. Paul tiles;
`build/web-offline-st-paul.png` captures the result. The PC was not connected to
the device's Wi-Fi: hardware HTTP verification used the device's real lwIP
loopback interface, while browser tests used local routed fixtures and assets.

Artifact: `dist/s3-lora-interface-v0.1.88-verified-20261017.bin` (1815936 bytes).
SHA256: `3d0b408159eaa22cfa56dbae819f37b984ac53bd9082d522f44e0ab3ad26336d`.
The Leaflet license accompanies it as `dist/Leaflet-LICENSE.txt`.
Metadata backup: `build/pre-v0.1.88-flash-metadata.bin`.
Hardware log/report: `build/v0.1.88-ui-smoke.log` and
`build/v0.1.88-device-report.json`. The H: tile download continued separately.
