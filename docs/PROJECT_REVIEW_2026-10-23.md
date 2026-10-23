Project review, 2026-10-23

Status update: the GPIO6 playback finding below was corrected in v0.1.93.
The original review is preserved as a snapshot of v0.1.91. See
[notification audio](AUDIO.md) for the current signal path, gain and recovery.

Reviewed CYD firmware v0.1.91 at commit `78e8c524a2767ab473efe79343d7ea49bd2978e4`, its wiring instructions, configuration handlers, GPS control/parser, radio telemetry, SD position persistence, browser UI, and build/regression checks. This is a review report; application source has not been changed. The unrelated, previously untracked deployment script was excluded.

The shared GPS UART plan is internally consistent. Two high-priority defects need attention: saving GPS settings clears the Heltec's pin assignments, and the default CYD speaker data-output pin disagrees with the official schematic. Seven further issues concern configuration preservation, status accuracy, and error handling. P1 means fix before relying on the affected function; P2 means a reproducible functional or reliability issue with a narrower trigger.

1. **P1: Saving position settings clears the external GPS pin configuration.**

   [Radio.inc](../src/firmware/Radio.inc#L1379) creates a zero-initialized PositionConfig and fills only the exposed settings. It never copies `heltecConfig.position`, so `rx_gpio`, `tx_gpio`, `gps_en_gpio`, and `position_flags` are sent as zero. For the documented setup, saving even an unrelated position broadcast interval changes GPS RX from GPIO5 to zero. [Meshtastic's AdminModule](https://github.com/meshtastic/firmware/blob/master/src/modules/AdminModule.cpp#L662) replaces the entire position configuration; [GPS initialization](https://github.com/meshtastic/firmware/blob/master/src/gps/GPS.cpp#L1424) requires a nonzero RX assignment or a board default. The Heltec V3 variant has no built-in GPS RX default. External GPS reception can therefore disappear after the resulting reboot, despite correct wiring.

   Preserve the loaded PositionConfig before editing its exposed fields, and reject saves until the original configuration has been read. Regression coverage should start with RX5/TX4 and nonzero position flags, apply an interval change, decode the outgoing message, and verify those fields survive. The installed Heltec firmware version was not queried; the replacement behavior was checked against upstream source available during this review.

2. **P1: CYD speaker playback uses the codec's output line instead of its input.**

   [constants.h](../constants.h#L58) defaults `CYD_AUDIO_DOUT_PIN` to GPIO6, and [Audio.inc](../src/firmware/Audio.inc#L650) supplies it as the ESP32 I2S transmitter output. The [official LCDWiki schematic](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf) connects GPIO6 to `I2S_DO`, the ES8311 `ASDOUT` output. GPIO8 connects to `I2S_DI`, the ES8311 `DSDIN` DAC input. The firmware therefore sends speaker samples to the wrong signal. I2S initialization and I2C register writes can still report success while the speaker receives no samples; GPIO6 could also have two output drivers if the codec's ADC output is enabled.

   Use GPIO8 for ESP32 audio data output on this CYD revision; reserve GPIO6 for codec-to-ESP32 audio input if recording is added. Verify with an actual speaker test after correcting the pin. No local configuration override was present during the review build.

3. **P2: LoRa and power settings saves reset fields outside the edited form.**

   [sendHeltecLoraConfig](../src/firmware/Radio.inc#L1306) and [sendHeltecPowerConfig](../src/firmware/Radio.inc#L1397) also construct full configurations from zero. A hop-limit or TX-power edit clears a previously selected `channel_num`, `frequency_offset`, `override_frequency`, and other unexposed LoRa settings; the handler also forces `tx_enabled=true`. That can move a radio away from its peers or re-enable a deliberately disabled transmitter. A power edit clears `adc_multiplier_override`, `device_battery_ina_address`, and `powermon_enables`. Other handlers already copy cached configuration, making this behavior inconsistent. Upstream [AdminModule](https://github.com/meshtastic/firmware/blob/master/src/modules/AdminModule.cpp#L676) applies whole-section replacements here too.

   Copy the loaded section and change only the requested fields. Block writes when the section is unavailable. Test preservation of a nonzero LoRa channel and an existing battery calibration value.

4. **P2: A lost GPS fix stays labeled as live in the browser map.**

   [Gps.inc](../src/firmware/Gps.inc#L89) sets `gpsStats.valid` once a fix arrives and does not expire it. Retaining the last coordinates is useful, but [WebUi.h](../src/WebUi.h#L510) treats that historical flag as a current fix: it replaces the local node with a marker named `live`, setting both age fields to zero on every refresh. The separate `localGpsFreshFix` field is ignored. Executing the actual `drawMap` function with `localGpsFreshFix=false` and a one-hour-old node reproduced `Field node live` with `positionAge=0`. The touchscreen map already uses an explicit freshness check, so the two interfaces disagree.

   Publish a position age/freshness value and use it consistently. Keep old coordinates available with their real age. The touchscreen's [range helpers](../src/firmware/Ui.inc#L786) also use the historical flag alone, and [chat timestamps](../src/firmware/Radio.inc#L880) accept old GPS time without an age check; these consumers should follow the same freshness policy.

5. **P2: Every position update synchronously deletes and rewrites the saved-location file.**

   [Gps.inc](../src/firmware/Gps.inc#L120) calls `saveLastLocationToSd` on every location update. [Storage.inc](../src/firmware/Storage.inc#L660) removes `/s3-lora/last_location.txt`, recreates it, writes, and closes it synchronously on the main loop, with no throttling. TinyGPSPlus commits locations for both valid RMC and GGA sentences, so a receiver emitting both at 1 Hz can trigger two rewrites per second. Higher requested GPS rates increase this load. This bypasses the append worker, causes repeated filesystem metadata writes, and can delay UART/UI servicing when the card is slow. If power fails after removal, the previously saved location may be lost.

   Update the RAM location immediately, checkpoint to SD at a bounded interval, and preserve a recoverable prior file while replacing it. Validate under sustained GPS input while map reads or downloads are active. SD endurance and actual stall times were not measured.

6. **P2: Neighbor telemetry overwrites statistics presented as local radio health.**

   [updateTelemetry](../src/firmware/Radio.inc#L1832) writes every sender's device metrics into global `stats`, although it has already stored them in the correct NodeRecord. [Node-info processing](../src/firmware/Radio.inc#L2055) repeats this for every node in the received database. There is no `from == stats.myNodeNum` check. Consequently local channel utilization, air TX utilization, and radio uptime can become the most recently received neighbor's values. The local-stats branch also accepts any sender's packet/node counts. These globals feed [Mesh Health](../src/firmware/UiRefresh.inc#L451) and browser packet diagnostics. The separate MAX17048 battery reading is not affected by this overwrite.

   Retain all remote metrics in NodeRecords, but update local radio totals and freshness only for the Heltec's node number. Test with two nodes whose metrics differ and verify that remote updates leave the local summary unchanged.

7. **P2: Some readable radio settings cannot survive a save unchanged.**

   [loraRegionName](../src/firmware/Radio.inc#L175) reports `EU_433`, but [parseRegion](../src/firmware/Radio.inc#L1184) does not accept it and returns `UNSET`. [loraPresetName](../src/firmware/Radio.inc#L191) reports `SHORT_TURBO`, `LONG_MODERATE`, and `LONG_TURBO`, but [parseModemPreset](../src/firmware/Radio.inc#L1198) maps each to `LONG_FAST`. The browser's `setCfgSelect` adds readback values to the dropdown, so a user can see the correct setting and still overwrite it by saving an unrelated radio field. `VERY_LONG_SLOW` is also absent from the read mapping and is displayed as `LONG_FAST`.

   Make enum read/write mappings symmetric for the supported protocol snapshot, preserve unknown values, and reject unrecognized submissions instead of choosing a different setting. Source checks reproduced the named read/write mismatches. These cases do not affect a radio already using the supported US/MEDIUM_FAST settings, but they do affect other valid configurations.

8. **P2: Configuration sends report success after a failed UART write.**

   [writeStreamFrame](../src/firmware/Radio.inc#L952) returns false on a short header or payload write. [sendLocalAdmin](../src/firmware/Radio.inc#L1145) discards that result and returns true after encoding succeeds. Config requests and heartbeats have the same problem at lines 993 and 1009. Callers can return HTTP 200 or log that a save was sent even though the complete command never entered the UART. Chat sends already check this return value, so the error policy differs between paths.

   Propagate the write result and update successful-send counters/timestamps only after a complete frame is accepted. Track receiver acknowledgement/readback separately from UART acceptance. A short-write test should cover configuration, heartbeat, and request paths. A physical short write was not induced during review.

9. **P2: GPS fix quality and 2D/3D status are fabricated rather than decoded.**

   [Gps.inc](../src/firmware/Gps.inc#L100) always reports fix quality 1 and infers 3D whenever the cached satellite count is at least four. It sets the corresponding `has*` flags, so System GPS presents these as supplied measurements. Satellite count alone does not encode the receiver's navigation mode, and the count is not checked for age here. The [u-blox 6 protocol specification](https://content.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_%28GPS.G6-SW-10018%29_Public.pdf), NMEA GGA/GSA sections, defines actual quality and fix-mode fields. Differential quality or forced 2D mode can therefore be reported incorrectly.

   Decode the GGA quality and GSA fix-mode fields with age checks, or report those fields as unknown. Apply per-field age checks to cached altitude, speed, course, and dilution values too, so fresh RMC positions cannot make old auxiliary data appear current.

The following GPIO plan matches the current UART initialization and the official board connections, with the audio correction identified above. GPIO numbers belong to the named board; equal numbers on different boards are separate signals. References are the [LCDWiki schematic](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf), [Heltec V3.2 datasheet](https://resource.heltec.cn/download/WiFi_LoRa_32_V3/HTIT-WB32LA_V3.2.pdf), [constants.h](../constants.h), [tft_setup.h](../tft_setup.h), and [UART setup](../s3-lora-interface.ino#L85).

| Function | CYD GPIO | Other connection / assessment |
| --- | --- | --- |
| Heltec UART RX | 14 | From Heltec GPIO39 TX, 115200 baud |
| Heltec UART TX | 21 | To Heltec GPIO38 RX, 115200 baud |
| GPS UART RX | 3 | From GPS TX; same GPS output also feeds Heltec GPIO5 RX, 9600 baud |
| GPS UART TX | 2 | To GPS RX; Heltec GPIO4 stays physically disconnected |
| Shared I2C SDA / SCL | 16 / 15 | Touch 0x38, ES8311 0x18, MAX17048 0x36; distinct addresses |
| Touch reset / interrupt | 18 / 17 | Matches schematic |
| TFT CS / DC / MOSI / SCLK / MISO | 10 / 46 / 11 / 12 / 13 | Matches schematic |
| TFT backlight | 45 | Active high; matches schematic |
| SD CLK / CMD / D0 / D1 / D2 / D3 | 38 / 40 / 39 / 41 / 48 / 47 | Matches schematic; CYD pins are independent of the Heltec UART pins |
| Audio MCLK / BCLK / LRCLK | 4 / 5 / 7 | Matches schematic |
| Audio data ESP32 to codec | **8 required; currently 6** | GPIO8 feeds ES8311 DSDIN; GPIO6 is the reverse direction |
| Speaker amplifier enable | 1 | Active low; consistent with the schematic |

GPS and Heltec UARTs use different hardware UART instances, UART1 and UART2 respectively. GPS TX fans out to two receivers, while GPS RX has one transmitter. No additional UART/TFT/touch/SD pin collisions were found. The shared I2C bus needs 3.3 V-compatible pull-ups, as do the UART signals; a GPS breakout's 5 V power input does not imply 5 V UART compatibility. Existing power documentation explicitly specifies a single cell and sensing before the boost converter. Mechanical room for two batteries does not define an alternative electrical pack configuration.

GPIO3 is a strapping pin, but its documented JTAG-selection role depends on the corresponding eFuse configuration; this review does not identify it as a general boot-mode conflict. See [Espressif JTAG selection](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/configure-other-jtag.html). Board revision, actual signal levels, reset behavior, supply stability, and the installed Meshtastic version require bench confirmation.

Verification completed during this review:

- `tools/verify_firmware.ps1` passed using the installed Playwright Node executable via `-Node`, with the required access to the installed build tools. Dependency versions and nanopb hashes matched the lock. Firmware compiled with `FIRMWARE_RUN_TESTS` and `-Werror=format`, including the GPS UBX compile-time assertions. Flash usage was 1,859,754 / 3,145,728 bytes (59%); static RAM was 105,592 / 327,680 bytes (32%).
- Web UI Node regressions passed: escaping, request concurrency, timeout/backoff/recovery, delivery/retry controls, runtime rendering, and bundled map checks.
- `python -m unittest discover -s tests -p 'test_*.py'` passed all 11 tests for SD export and USB tile transfer.
- `python tests/test_web_offline_map.py` passed at 390 px and 1280 px, including offline assets, RGB565 conversion/cropping, panning, missing/corrupt/busy tiles, and recovery.
- The review reproduction script under ignored `build/project-review/reproduce-web-findings.js` executed the real browser `drawMap` function and confirmed the stale-fix display defect; its source checks confirmed enum round-trip mismatches. The official CYD schematic was downloaded and rendered for visual inspection of the audio connections.

Initial restricted-environment attempts encountered dependency/temp-directory permissions. Re-running the checks with the necessary access passed; these permission errors are not classified as firmware defects. The normal verification helper also passed from the repository root, so a prior restricted build failure is not reported as a source issue.

Passing checks do not exercise configuration preservation, sender isolation in telemetry, actual UART short writes, or the entire GPS ACK/NAK/timeout transaction. The UBX tests verify packet vectors, checksum rejection, bounds, and parser recovery. Add behavioral tests around the identified gaps while applying fixes. No device was flashed and no electrical measurements were taken. Enclosure geometry and older retained CAD revisions were outside this firmware/wiring review.
