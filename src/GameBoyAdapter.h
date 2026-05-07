#pragma once

// Web-first Game Boy integration scaffold.
//
// The upstream Peanut-GB core is vendored as src/peanut_gb.h, but the emulator
// runtime is intentionally not linked into the sketch yet. The current firmware
// only scans SD-card ROM files and reads safe cartridge metadata from the Web UI.
//
// Next adapter step:
// - load a selected ROM into PSRAM or a bounded SD-backed reader
// - define ENABLE_SOUND 0 before including peanut_gb.h
// - implement gb_rom_read, cart RAM read/write, gb_error, and lcd_draw_line
// - run frames from an explicit Web-triggered test path, not from LVGL startup
