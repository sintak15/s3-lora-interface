# Replacement SD card

The replacement card is the FAT32 card prepared as **H:**. Its new USGS map
pack stays under `/s3-lora/tiles`; live non-map files are copied from the CYD
over authenticated, read-only SD export routes. Existing destination files
that differ are backed up on the PC before replacement. No card is formatted
and no maps are overwritten by the data sync.

## Verified map pack — October 25, 2026

All **901,112 present tiles** have the expected 131,072-byte RGB565 size.
The original plan contains 901,120 tiles. Eight source requests remain HTTP
404; a coarser parent tile is present for every missing tile. Center tiles
around St. Paul were also read at every zoom.

| Zoom | Planned tiles | Present tiles |
| --- | ---: | ---: |
| 10 | 9,891 | 9,891 |
| 11 | 37,372 | 37,370 |
| 12 | 81,124 | 81,124 |
| 13 | 166,407 | 166,401 |
| 14 | 606,326 | 606,326 |

## Preserved device data

The recursive export found 15 non-map files, including root autorun assets,
public/secondary/legacy channel logs, direct messages and their new thread
journal/archive, events, position history, saved location, cached map image,
and current battery/power logs. Each copied file is verified against the
captured source bytes with SHA-256 after writing to H:.

CYD preferences remain in the same board's internal flash, including Wi-Fi,
battery learning and pet state. Heltec configuration remains on the Heltec.
Replacing only the SD card does not require re-entering these settings.

Private captures, original destination backups and verification manifests are
under ignored `build/sd-sync-20261025/`. The full copy manifest is
`full-sync-manifest.json`; map counts are in `map-verification.json` and the
eight missing source tiles are recorded in `tile-recheck.json`.

## Physical swap

After the final sync is confirmed, safely eject H: from Windows, fully power
off the CYD, replace its card, then restart it. Keep the old card as a backup.
Open Map and Messages to confirm the new maps and existing histories. The
data copy is a snapshot: activity recorded on the old card after the final
capture is not automatically mirrored to the replacement.

If the swap is postponed while the CYD continues recording, refresh the data
copy before swapping. The private sync helper requires `CYD_TEST_PASSWORD`
in its environment and checks that H: still contains the expected map pack.

Final production-firmware refresh completed October 25, 2026 at 07:46 PM Central Daylight Time. All 15 files (5,394,854 bytes) passed SHA-256 readback verification.

## If the replacement fails to mount

Successful PC file verification does not establish that the card will mount in
the CYD socket. A `mount failed` status affects maps, logging and ROM uploads.
Fully power off the CYD, reseat the card until it latches, and restart it.

v0.1.108 adds automatic SD bus retries: configured mode first, then lower speed
and one-data-line modes. Every attempt preserves the existing filesystem.
**SD Storage > Mount SD** reruns these attempts without a reboot. Diagnostics
report `sdBusWidth`, `sdFrequencyKhz` and `sdMountAttempts` to identify the mode
that succeeded. Games reports the mount failure separately from temporary SD
lock contention or an active USB transfer.

If all bus modes fail after reseating, inspect the card's partition/filesystem
and hardware connection before changing its contents. The original card and
PC captures remain the recovery copies.

### GPT compatibility in v0.1.110

The 128 GB replacement uses GPT with a FAT32 data partition. The bundled
FatFs configuration has GPT parsing disabled (`FF_LBA64 = 0`), so Windows could
read the card while the original CYD mount failed. v0.1.109 confirmed this by
reading the protective partition table directly, with no communication error.

v0.1.110 validates the GPT header and entry-array checksums, finds the single
writable Basic Data FAT32 partition, checks its geometry and rejects overlapping
entries. It exposes only that existing partition to FatFs through a bounded
block driver. All file access uses the selected filesystem; normal MBR cards
continue using the Arduino SD_MMC filesystem. The GPT tables stay in place.
This card mounts at the normal four-data-line, 20 MHz setting.

Diagnostics report `sdGptMounted`, `sdPartitionStart`, `sdPartitionSectors`,
`sdGptHeaderCrc` and `sdGptTableCrc`. Authenticated `GET /sd/gpt/verify` re-reads
the protective MBR, primary/backup GPT headers and both entry arrays, then checks
their combined fingerprint against the mount-time baseline. `unchanged: true`
confirms that metadata stayed intact across file operations.

Card removal and partition conversion are not needed for this replacement.

## On-device verification — v0.1.110

With the 128 GB card still enclosed, the CYD mounted its FAT32 partition at LBA
2048 (249,735,040 sectors) in four-data-line mode at 20 MHz. All 15 original
non-map captures passed readback verification from the CYD; log growth was
allowed only after the verified original bytes. Center map tiles at zooms
10–14 read at their expected sizes.

A 4 MiB original test ROM was uploaded through the same authenticated endpoint
used by the browser, listed, downloaded and SHA-256 verified, then deleted.
The GPT metadata fingerprint matched before and after these operations. Parser
checks cover corrupt checksums, out-of-range/oversized tables, invalid FAT32
geometry, read-only partitions and overflowing writes.

A software remount and the production-firmware reboot also retained the GPT
mount and the same metadata fingerprint. Final diagnostics reported zero SD
write errors, lock timeouts, dropped appends and radio decode/frame errors.
Eighteen existing settings/status fields matched before and after installation.
The installed production image is
`dist/s3-lora-interface-gpt-sd-support-v0.1.110-20261025.bin`.
