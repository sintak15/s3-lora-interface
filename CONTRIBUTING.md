# Contribution conventions

Run commands from the repository root. The Arduino sketch must retain the name
`s3-lora-interface.ino`; its root forwarding headers select the bundled nanopb
runtime and are required by generated Meshtastic code.

## File placement

| Location | Contents |
| --- | --- |
| Repository root | Sketch, board setup, nanopb forwarding headers and project metadata |
| `src/firmware/` | Firmware implementation included once by the sketch |
| `src/` | Shared headers, browser UI and portable helpers |
| `src/tests/`, `tests/` | Compile-time C++ checks and host-side tests |
| `tools/` | Reusable build, map-transfer, diagnostics and enclosure utilities |
| `config/` | Checked-in example configuration and dependency lock; ignored local overrides |
| `hardware/enclosure/` | OpenSCAD models, reviewed print exports, reports and fit coupons |
| `docs/` | Current guides, implementation history and dated reviews |
| `build/`, `dist/` | Ignored temporary work, captures, logs, downloads and generated firmware |

Keep print-ready STL exports and their JSON reports with their OpenSCAD sources.
Earlier enclosure variants are useful physical-fit records; they are not build
garbage. Use the [enclosure index](hardware/enclosure/README.md) to find the current
parts. New exploratory exports belong in `build/` until selected for publication.

## Names and formatting

- C++ headers and implementation modules use `PascalCase` filenames. Functions
  and variables use `camelCase`; constants and enumerators use `UPPER_SNAKE_CASE`.
- Python and PowerShell tools use `snake_case` filenames. Python functions and
  variables use `snake_case`; PowerShell variables follow the surrounding script.
- Documentation guides use `UPPER_SNAKE_CASE.md`; `README.md` files index folders.
  Dated reviews retain their dates. Published STL/JSON names use hyphens;
  OpenSCAD sources use underscores.
- Follow `.editorconfig` and `.gitattributes`: UTF-8, LF text, spaces, and a final
  newline. Match the surrounding code instead of reformatting unrelated files.
- Preserve stored preference keys, numeric sound IDs, serial commands and API
  field names when renaming internal code. Keep legacy aliases at API boundaries.
- Leave generated Meshtastic code and vendored nanopb/Leaflet/font sources alone
  during cleanup. Retain their licenses; nanopb bytes are dependency-hash checked.

Do not commit local credentials, tokens, generated build identity, firmware
binaries, diagnostic captures, Python caches or unrelated deployment scripts.
Use generic paths/ports in reusable tools and explain hardware-specific defaults.

## Verification

```powershell
python tools/check_repo.py
python -m unittest discover -s tests -p "test_*.py"
.\tools\verify_firmware.ps1
```

The verifier includes the repository check, browser regressions and the Arduino
build with compile-time checks. See [build dependencies](docs/BUILD.md) for Python,
Node.js, Arduino and test dependencies. These checks do not connect to hardware.
The separate offline-map browser exercise is
`python tests/test_web_offline_map.py` and requires Playwright Chromium.

For a firmware release, update `FIRMWARE_VERSION` in `constants.h`, the README's
current version and the relevant guide. Dated reviews and historical release
notes retain the version they assessed. Describe what was actually tested,
distinguishing compilation, device readback and listening/physical-fit checks.
