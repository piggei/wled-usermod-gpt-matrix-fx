# GPT Matrix FX - Sparse Ripple Experiment

Experimental **v0.3.0-dev.6** continuation of the stable **v0.2.2** release. This package includes all 25 original effects plus five beta visual candidates. **Do not publish this package as v0.2.2.**

- GPT21b - Sparse Ripple Brightness
- GPT22b - Sparse Ripple Size
- GPT23b - Photon Loom
- GPT24b - Kinetic Rosettes
- GPT25b - Tidal Glyphs

See [EXPERIMENTAL_SPARSE_RIPPLE.md](EXPERIMENTAL_SPARSE_RIPPLE.md) for testing and attribution.

## PlatformIO

Place this directory next to the WLED project and add to the existing environment's `custom_usermods` list (keep existing entries):

```ini
  symlink://../wled-usermod-gpt-matrix-fx
```

Alternatively, use `symlink:///absolute/path/to/wled-usermod-gpt-matrix-fx`. Build with `pio run -e YOUR_WLED_ENV` from the WLED project directory.

## Testing

```sh
sh tests/run_host_tests.sh
sh tests/run_sanitizers.sh
```

No WLED core modifications are required. This is a test candidate, not a release.

## Runtime diagnostics

`/json/info` includes `u.gptMatrixFX` with `version`, `build`, `effects`, `standard`, `audio`, and `experimental`. This confirms the compiled usermod version.

## Experimental controls (dev.6)

- GPT21b / GPT22b: `Dot Spacing` adjusts lattice pitch; midpoint preserves the six-pixel baseline at 64x64.
- GPT24b: `Size` replaces `Hue` and changes rosette scale. Midpoint preserves the dev.4 scale.
- Existing GPT24b presets will interpret the old Hue slider value as Size.

## JSON information layout (dev.6)

The usermod writes its diagnostics to `/json/info` under `u.gptMatrixFX` alongside other usermod information. This is separate from `/json/state` and its `u` usermod state/configuration object. The module does not replace the existing `u` object.

```json
{ "u": { "gptMatrixFX": { "version": "0.3.0", "build": "dev.6", "effects": 30, "standard": 20, "audio": 5, "experimental": 5 } } }
```
