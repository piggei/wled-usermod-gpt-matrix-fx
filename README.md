# GPT Matrix FX - Sparse Ripple Experiment

Experimental continuation of the stable **v0.2.2** release. This package includes all 25 original effects plus two new visual comparison candidates. **Do not publish this package as v0.2.2.**

- GPT21 - Sparse Ripple Brightness
- GPT22 - Sparse Ripple Size

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
