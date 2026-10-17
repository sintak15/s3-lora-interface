# Offline map coverage (v0.1.87)

The touchscreen keeps the requested zoom separate from the displayed zoom.
When detailed tiles are missing, it selects the nearest coarser level with a
complete viewport. If none is complete, it uses the most detailed readable
partial viewport with a valid center tile. It never selects a level above the
requested zoom. Missing coverage is shown explicitly.

Tap Coverage between the minus and plus buttons to inspect zooms 10–14 around
the current map center. Each row reports Ready, Partial, Missing, Unreadable or
Read error and the number of available tiles needed for that viewport. These
checks inspect installed files, rather than assuming a download plan has been
completed. Availability describes the current viewport, not an entire region.

The map overlay identifies the displayed level, coarser fallback and partial
coverage. Dragging, markers and the distance scale use the displayed image's
actual zoom and center. The requested level remains selected so detailed maps
return automatically when files become available. Scales use feet and miles on
both the touchscreen and the browser map.

The background map worker checks at most four paths per level (20 total),
including file size, under the existing SD mutex. Render reads verify that the
selected files can actually be read. Coverage expires after ten seconds and is
rechecked when the viewport footprint or storage generation changes. USB tile
uploads and SD remounts invalidate the storage generation. The main UI thread
does not scan directories to determine coverage.

The browser's Touchscreen coverage card shows the touchscreen's last checked
area, its check age, requested/displayed zoom and per-level status. Since
v0.1.88, the browser map also uses SD tiles; see
[WEB_OFFLINE_MAPS.md](WEB_OFFLINE_MAPS.md).

The verification firmware's USB `test-coverage` command creates a uniquely named
temporary fixture directory, separate from live tiles. It exercises complete,
partial, malformed and missing coverage; actual worker fallback; return to
detailed maps; coverage-page navigation and layout; and fixture cleanup. It
does not save test locations, preferences or fixture pixels to the device map
snapshot. Existing `test-map`, `test-ui` and `mapbench` checks remain available.

## Device verification (2026-10-17)

Installed v0.1.87 on the connected CYD in its existing app1 partition, preserving
settings and SD content. The flasher verified the application hash. All 13
coverage checks, 21 map/navigation checks and ten UI checks passed, including
both Battery menu branches. The temporary fixture directory was removed.
The original card reported Ready at all five levels for the current viewport;
requested and displayed zoom were both 14.

The cache benchmark passed all seven phases with identical pixels. Warm repeat,
pan and return took 6363/6774/7064 microseconds with zero tile-data SD reads.
Diagnostics recorded zero SD errors, lock timeouts, dropped log appends, tile
read errors or map-worker failures. About 164 KiB internal RAM and 6766 KiB
PSRAM remained free. Browser regression tests also passed, including coverage
escaping and imperial-only map-scale initialization.

Artifact: `dist/s3-lora-interface-v0.1.87-verified-20261017.bin` (1754144 bytes).
SHA256: `1330fbf0f0870516bd8e5f1a4685aef76a19c99b10bfafc86e5269894954b60d`.
Metadata backup: `build/pre-v0.1.87-flash-metadata.bin`.
Device results: `build/v0.1.87-ui-smoke.log` and
`build/v0.1.87-device-report.json`. The separate download to the replacement
card H: continued throughout verification.
