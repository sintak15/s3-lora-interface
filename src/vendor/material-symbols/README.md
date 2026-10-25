# Material Symbols Rounded

Selected icons from Google's [Material Design Icons repository](https://github.com/google/material-design-icons), licensed under [Apache 2.0](LICENSE.txt). Original SVGs are unchanged. The exact upstream commit, paths and SHA-256 hashes are in [manifest.json](manifest.json).

The application bundles only this subset. The web UI uses inline SVG paths; the CYD uses generated 4-bit alpha masks at 16, 20 and 24 pixels, tinted by its theme. Neither interface downloads icons or fonts at runtime.

Regenerate both outputs without network access:

```powershell
python tools/generate_ui_icons.py
# Or verify the checked-in generated outputs:
python tools/generate_ui_icons.py --check
```

Generation requires Pillow, Playwright and its Chromium runtime. Generated outputs are `src/UiIcons.h` and the marked icon-path block in `src/WebUi.h`. Source files retain Google's icon names through the manifest; short application names identify their use.
