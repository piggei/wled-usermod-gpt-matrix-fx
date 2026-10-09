# GPT Matrix FX v0.2.2

First public source release of **GPT Matrix FX**, an external PlatformIO usermod providing **25 original effects for WLED 2D LED matrices**.

## Highlights

- **20 standalone effects**, named `GPT01` through `GPT20`.
- **5 AudioReactive effects**, named `GPTA01` through `GPTA05`, using the WLED AudioReactive usermod's 16 FFT bands.
- A redesigned **GPT06 - Clockwork Cathedral** with gear-meshed orbiting satellites, drifting assembly motion, moving metallic details, and glitter.
- Palette-change handling and fallback for entirely black palettes, without erasing intentional dark regions in non-black palettes.
- Scales from small to larger 2D segments; tuned visually around a 64x64 HUB75 display.
- External symlink-based PlatformIO integration; no WLED core modifications.

## Installation

Unpack `wled-usermod-gpt-matrix-fx` alongside your WLED checkout. Append the following entry to your existing `custom_usermods` in the selected PlatformIO environment (preserving all other entries):

```ini
  symlink://../wled-usermod-gpt-matrix-fx
```

Alternatively, use an absolute `symlink:///...` path. See `README.md` for details. Include and enable the existing WLED AudioReactive usermod for the five sound-responsive effects.

## Validation

- Host suite: 25 effects x 5 matrix resolutions; 25 effects x 9 live palette configurations.
- Differential FFT tests for all five audio modes.
- GPT06 meshing/orbital-motion geometry tests.
- C++11 compatibility checks and AddressSanitizer/UndefinedBehaviorSanitizer passed.
- The maintainer reports satisfactory results on the real LED matrix. This package has not been independently compiled against the maintainer's private WLED `waveshare_spotify` checkout; hardware FPS/soak tests have not been formally measured.

## Notes

- Effect IDs are dynamically allocated by WLED and can change when the effect set is rebuilt. Presets referencing numeric IDs may need review.
- The package includes an updated `.gitignore` that excludes `Zone.Identifier` filesystem artifacts (including Windows/WSL Mark-of-the-Web sidecars).
- The firmware environment is **not** bundled: this is a WLED usermod source library, not a prebuilt firmware image.
- License: MIT.
