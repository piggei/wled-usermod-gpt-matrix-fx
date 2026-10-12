# v0.3.0-rc.1 verification checklist

Source baseline: `v0.3.0-dev.16-preview.7b` accepted on Waveshare 64×64.

- [x] Preserve all 40 renderer function bodies and their registration order; only promote the 5 audio names.
- [x] Promote menu labels and `extraPerformance.effect` to GPTA06–GPTA10.
- [x] `library.json` and `/json/info` report release candidate and `experimental=0`.
- [x] Preserve GPTA02 ring removal; GPTA05 overlapping waves; GPTA07 ring removal.
- [ ] Build with the maintainer's complete Kagayaki `pio run -e waveshare` environment (not bundled here).
- [ ] On hardware inspect `/json/info`: `version=0.3.0`, `build=rc.1`, `effects=40`, `standard=30`, `audio=10`, `experimental=0`.
- [ ] Inspect `/json/eff` and confirm 40 names including GPTA10; confirm free effect IDs / RSVD handling and preset mappings after any registration filter change.
- [ ] Run a multi-hour stability check for GPT28 and a representative selection of older FX.
- [ ] Check all five new audio effects with real music, silence and strong transients; verify A02, A05 and A07 beat-ring behavior.
- [ ] Only after passing gates, issue `v0.3.0` final release without modifying approved renderers.

**Known limitation:** historical long-uptime float phase drift (>48 hours) has not been formally qualified across all effects. Host simulations alone cannot prove stable hardware operation.

## Historical Stage-2B golden-reference mismatch

`tests/run_stage2b_phase_tests.sh` currently fails for GPTA02 even on the source baseline `dev.16-preview.7b`: its frozen `gpt_stage2b_dev14a_reference.h` depicts the earlier beat-ring output, while the accepted newer effect intentionally removed that ring. This is **not introduced by rc.1**. Stage-1 and Stage-2A suites pass. Refreshing Stage-2B reference data requires an explicit source-qualified golden update before declaring this gate passed. The RC should not be marked as having a fully green test suite.
