# USB map transfer and replacement SD cards

Firmware v0.1.85 adds two PC tools. Connect through the CYD USB port, close other
serial monitors, and use the current COM port. Neither tool formats a card.

## Upload prepared tiles while the card stays in the CYD

```powershell
python tools/upload_tiles_usb.py --port COM12 --tiles H:\s3-lora\tiles --overwrite
```

`--tiles` points to the folder containing `z/x/y.rgb565`, not the SD root.
`--dry-run` validates paths and exact 131072-byte sizes without connecting.
`--limit 5` transfers a small sample. Without `--overwrite`, a different existing
tile stops the upload. Identical device tiles are checked by CRC32 and skipped,
so rerunning the same command resumes completed tiles.

The device stages one tile in PSRAM. Sequential base64 chunks are acknowledged;
the complete CRC32 is checked before any tile data is written. A temporary SD
file must pass a complete readback before promotion. Replacement retains the old
tile as `.bak` until promotion succeeds. A failed promotion attempts restoration;
an interrupted replacement can be recovered by retrying that tile. This is not
a transactional filesystem guarantee across power failure. Unfinished receive
sessions expire after 30 seconds or can be explicitly aborted by the uploader.
Successful uploads invalidate the PSRAM map cache and displayed map.

Coordinates are limited to firmware zooms 10-14 and their valid XYZ bounds. The
host cannot choose arbitrary write paths. Data goes into the device's detected
map tile root, normally `/s3-lora/tiles`. Only the CYD USB command service accepts
the protocol; the radio UART and web serial console do not.

## Copy supporting files to a larger card

Mount the replacement FAT32 card in a PC reader. After verifying its drive letter:

```powershell
python tools/copy_sd_usb.py --port COM12 --destination H:\ `
  --manifest build/sd-copy-H.jsonl --progress build/sd-copy-H.json `
  --exclude-path /s3-lora/tiles
```

Omit `--exclude-path` to copy existing map tiles too. The exporter only reads the
installed card. The copy preserves directory structure and verifies each USB
chunk, the complete source file, and the destination readback with CRC32. A
different existing destination file is saved as `.before-usb-copy`. Incomplete
files retain `.usbcopy-part`; final names appear only after verification. The
journal allows already verified immutable map tiles to be skipped on resume;
logs/snapshots are copied again because the running firmware updates them.
Windows-managed `System Volume Information` and `$RECYCLE.BIN` are omitted.
The source is read live, so each log reflects its size when that file was opened,
rather than a single filesystem-wide snapshot.

## Download maps directly to the replacement card

This avoids transferring a large new tile pack through the CYD. Install Python
dependencies `requests pillow numpy pyshp`. Download Census state outlines from
`https://www2.census.gov/geo/tiger/GENZ2024/shp/cb_2024_us_state_500k.zip`.

```powershell
python tools/download_usgs_tiles.py --destination H:\ `
  --boundaries build/maps/cb_2024_us_state_500k.zip `
  --work build/maps/st-paul-usgs --budget-gib 110 --reserve-gib 8
```

The profile uses St. Paul (44.9537, -93.09) as its center, contiguous-U.S. coverage
at zooms 10-11, U.S. tiles within 1500 km at zoom 12 and 1000 km at zoom 13, then
fills the remaining tile budget with the nearest U.S. zoom-14 tiles. Country
coverage is based on Census state boundaries plus a one-tile edge margin. It
does not provide detailed Canadian coverage. Tiles download from nearest to
farthest across all levels, with four workers and at most four requests/second.

The initial 110 GiB plan contains 901120 tiles; its farthest zoom-14 tile center
is about 944 km from St. Paul. This distance describes the selected U.S. coverage,
not a complete international circle. `plan.json` describes requested coverage;
`progress.json` reports actual completion and failures. A complete pack takes
multiple days at the default request rate. Keep the reader connected and the PC
running. The download process prevents idle sleep while active without changing
global power settings. A stopped job can be rerun: complete 131072-byte tile files are skipped,
and unfinished `.download-part` files are rewritten. New tiles are written,
flushed and checked by readback before receiving their final filename.

For a background job, check `build/maps/st-paul-usgs/progress.json`. To stop
gracefully, create `build/maps/st-paul-usgs/STOP` and wait for progress to say
`stopped` before ejecting the reader. Remove that STOP file before resuming the
same command. Keep only one download process running for a given work folder.

Credit: U.S. Geological Survey, The National Map. Tile service metadata explicitly
allows tile export: https://basemap.nationalmap.gov/arcgis/rest/services/USGSTopo/MapServer?f=pjson

When the desired areas are downloaded, safely eject H:, shut down the CYD,
swap cards, then boot and check SD/map status. Keep the old card for rollback.

## Connected-device validation (2026-10-16)

The verified v0.1.85 build was flashed to app1 at `0x310000` on the confirmed
ESP32-S3, with NVS/partition/OTA metadata backed up first. Esptool verified the
written image hash. Artifact SHA256:
`f84e63e3c929d7ca0ad81f7d8493aa945f9e1549f450c8077d5a7da6d58efe06`.

Hardware checks passed for invalid zooms, wrong offsets, invalid base64,
incomplete files, full-file CRC rejection, preservation after failed upload,
successful upload/SD readback, identical-file skip, and independent USB export
matching the uploaded bytes. The test tile was restored from its original backup
and independently read back byte for byte. One 128 KiB upload took 3.12 seconds.
All ten UI smoke checks passed, including both Battery menu branches. Post-test
diagnostics recorded zero SD errors, lock timeouts or dropped log appends.

With explicit user approval for history/location files, 11 non-tile files
(1836929 bytes) were copied and verified to the 128 GB FAT32 card H:. The 110 GiB
USGS job then continued directly on H:, nearest tiles first. Runtime progress is
stored under `build/maps/st-paul-usgs`; these build artifacts are ignored by Git.
