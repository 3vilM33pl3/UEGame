# M0/M1 Deck Checkpoint + M1 Reliability Validation

This runbook covers:

- `3VI-273` (`CCV-024`) playable sandbox checkpoint on Deck.
- `3VI-300` (`CCV-046`) reliable hex topology generation + rendering checkpoint.

## One-Command Validation

```bash
./scripts/run_m1_checkpoint.sh
```

What it runs:

1. WFC batch reliability at 128 hex (`16x8`) in `m1_relaxed` profile.
2. WFC batch reliability at 512 hex (`32x16`) in `m1_relaxed` profile.
3. Perf capture on `StartMap` using `scripts/run_perf_capture.sh`.
4. Automation test bucket `UEGame.Canal.M1` (includes reliability, spline/socket, material profile, towpath prop checks).

Outputs:

- Summary JSON: `Saved/Reports/m1_checkpoint_summary_<timestamp>.json`
- Perf report: `Saved/Reports/m1_checkpoint_perf_<timestamp>.json` (+ CSV)
- WFC reports: `/tmp/wfc_reports/m1_relaxed_128_<timestamp>.json` and `/tmp/wfc_reports/m1_relaxed_512_<timestamp>.json`
- Automation log: `Saved/Reports/m1_checkpoint_automation_<timestamp>.log`

Default pass thresholds:

- solve rate >= `0.90` for both 128 and 512 hex runs
- average FPS >= `30.0`
- automation test exit code `0`

Environment overrides:

- `M1_MIN_SOLVE_RATE` (default `0.90`)
- `M1_MIN_FPS` (default `30.0`)
- `M1_PERF_MAP` (default `/Game/StartMap`)
- `M1_PERF_DURATION` (default `5`)
- `M1_PERF_WARMUP` (default `1`)
- `M1_AUTOMATION_FILTER` (default `UEGame.Canal.M1`)

## Deck Manual Checklist (`3VI-273`)

Run this checklist directly on Steam Deck after deployment:

1. Launch the game and open the deck UI.
2. Press `Generate` and confirm world regeneration is visible.
3. Explore with Freecam and BoatCam controls.
4. Press `Start Capture` from the deck UI.
5. Press `Stop Capture` from the deck UI.
6. Confirm no external keyboard was needed for steps 2-5.

If steps above pass and perf threshold is met in the summary JSON, `3VI-273` is satisfied.

## Reliability Profile (`3VI-300`)

Current M1 reliability validation profile is:

- `RequireEntryExitPath=false`
- `RequireSingleWaterComponent=false`
- `AutoSelectBoundaryPorts=true`
- `DisallowUnassignedBoundaryWater=false`

Socket correctness, material profile determinism, and towpath prop coverage are validated by:

- `UEGame.Canal.M1`

These tests verify:

- topology solve succeeds
- generated spline has at least two points
- all neighboring solved sockets match
- prototype material settings remain deterministic per seed and expose wetness/tint variation
- towpath prop placement includes semantic coverage for: bollard, ring, sign, lamp, bench, reeds, bin, fence
