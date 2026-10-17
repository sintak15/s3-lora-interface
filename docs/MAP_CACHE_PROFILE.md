# On-device PSRAM tile-cache profile

Measured October 16, 2026 on the connected ESP32-S3 with 8 MiB PSRAM,
firmware v0.1.84, using its existing SD tiles. These timings cover tile assembly
for a 224 by 136 pixel viewport, excluding display transfer and UI frame rate.

| First run | Render time | Tile-data reads | Tile-data bytes |
| --- | ---: | ---: | ---: |
| Uncached row reader | 218.3 ms | 272 | 60,928 |
| Cold PSRAM cache | 194.6 ms | 64 | 262,144 |
| Warm repeat | 6.5 ms | 0 | 0 |
| Uncached shifted viewport | 216.5 ms | 272 | 60,928 |
| Cached shifted viewport | 6.4 ms | 0 | 0 |
| Uncached original viewport | 227.4 ms | 272 | 60,928 |
| Cached return | 6.6 ms | 0 | 0 |

All cached buffers matched their uncached reference byte for byte. The cold
load fetched two complete tiles; it read more bytes than a cropped viewport
but used fewer, sequential reads. Warm renders avoided tile-data SD reads.
This benchmark does not count SD metadata operations or establish performance
under concurrent large downloads.

A tile-root rescan advanced the storage generation from 3 to 4 and reset
resident entries to zero. Repeating the benchmark after invalidation passed:
uncached renders took 215–220 ms and cached renders took 6.4–8.7 ms. Cold loading
took 191.2 ms. Warm repeat and return again performed zero tile-data reads.

The cache reserves 1 MiB exclusively in PSRAM and can hold eight tiles. Two
entries were resident in this test. After the repeat, free internal RAM was
177 KiB and free PSRAM was 6,775 KiB (about 6.62 MiB). No tile-read errors,
radio decoding errors, SD write errors or dropped SD appends were reported.
The benchmark reference buffer was released after each run.

The firmware build, browser regression checks, LRU compile-time checks and
all existing on-device UI smoke checks passed. Flash data passed hash
verification. The prior v0.1.83 recovery binary remains in `dist/`.

Local measurement artifacts:

- `build/v0.1.84-mapbench.json`: reports before/after the first run, including samples.
- `build/v0.1.84-cache-invalidation.json`: reports around invalidation and the repeat.
- `build/v0.1.84-mapbench.log` and `build/v0.1.84-cache-invalidation.log`: USB captures.

Run `mapbench` over USB to repeat the comparison. See [BUILD.md](BUILD.md) for
cache behavior, profiling details and limitations.
