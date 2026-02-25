# Baseline Perf Capture (M1)

`UCanalPerfCaptureSubsystem` provides an automated benchmark pass that records:

- average FPS
- average/min/max frame time (ms)
- captured frame count

and exports both JSON and CSV reports.

## Files

- `Source/UEGame/Public/CanalGen/CanalPerfCaptureSubsystem.h`
- `Source/UEGame/Private/CanalGen/CanalPerfCaptureSubsystem.cpp`
- `scripts/run_perf_capture.sh`

## Console Command

Run from Unreal console/ExecCmds:

```text
Canal.RunPerfCapture Duration=20 Warmup=2 Output=Saved/Reports/perf-baseline.json ExitOnComplete=1
```

Arguments:

- `Duration` capture duration in seconds
- `Warmup` warmup time before sampling
- `Output` JSON path (CSV uses same path with `.csv`)
- `ExitOnComplete` `1|0` to auto-exit process

## One-Command Wrapper

```bash
./scripts/run_perf_capture.sh --map /Game/StartMap --duration 20 --warmup 2 --output Saved/Reports/perf-baseline.json
```

Optional faster non-rendering path:

```bash
./scripts/run_perf_capture.sh --nullrhi
```

Headless behavior:

- If no display is available (`DISPLAY` and `WAYLAND_DISPLAY` unset), the script auto-enables `--nullrhi`.
- This avoids SDL initialization failures in CI/headless terminal sessions.

## Report Example

JSON fields:

- `map_name`
- `capture_seconds_requested`
- `capture_seconds_measured`
- `frames_captured`
- `avg_frame_time_ms`
- `min_frame_time_ms`
- `max_frame_time_ms`
- `avg_fps`
